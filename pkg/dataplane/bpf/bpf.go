package bpf

import (
	"fmt"
	"log/slog"
	"net"
	"os"
	"strings"
	"structs"
	"time"

	"github.com/breeve/nfvcni/pkg/dataplane/config"
	"github.com/cilium/ebpf"
	"github.com/cilium/ebpf/link"
)

func Agent(cfg *config.Config) {
	objs := dataplaneObjects{}
	if err := loadDataplaneObjects(&objs, &ebpf.CollectionOptions{
		Programs: ebpf.ProgramOptions{
			LogLevel:     ebpf.LogLevelInstruction,
			LogSizeStart: 512 * 1024,
		},
	}); err != nil {
		slog.Error("loading dataplane objects", "error", err)
	}
	defer objs.Close()

	links := []link.Link{}
	for _, port := range cfg.Ports {
		slog.Info("Port information", "name", port.Name, "mode", port.Mode)

		iface, err := net.InterfaceByName(port.Name)
		if err != nil {
			slog.Error("Error finding interface", "interface", port.Name, "error", err)
			os.Exit(1)
		}
		item := &dataplaneIfaceConfigItem{
			Config: struct {
				_       structs.HostLayout
				Ifindex uint32
				Name    [64]uint8
				NodeMac [6]uint8
				Mode    uint16
				_       [4]byte
				L2      struct {
					_              structs.HostLayout
					VlanId         uint32
					VlanMode       uint32
					VlanRangeStart uint32
					VlanRangeEnd   uint32
				}
			}{
				Ifindex: uint32(iface.Index),
				Name:    [64]uint8{},
				NodeMac: [6]uint8{},
				Mode:    uint16(0),
				L2: struct {
					_              structs.HostLayout
					VlanId         uint32
					VlanMode       uint32
					VlanRangeStart uint32
					VlanRangeEnd   uint32
				}{
					VlanId:         uint32(port.L2.VlanID),
					VlanMode:       uint32(0), // TODO: support different vlan modes
					VlanRangeStart: uint32(0),
					VlanRangeEnd:   uint32(0),
				},
			},
		}

		// populate Name (NUL-terminated if space permits)
		nameBytes := []byte(port.Name)
		if len(nameBytes) >= len(item.Config.Name) {
			copy(item.Config.Name[:], nameBytes[:len(item.Config.Name)-1])
			item.Config.Name[len(item.Config.Name)-1] = 0
		} else {
			copy(item.Config.Name[:], nameBytes)
			// optional NUL terminator
			if len(nameBytes) < len(item.Config.Name) {
				item.Config.Name[len(nameBytes)] = 0
			}
		}

		// populate NodeMac (use first 6 bytes of HW address)
		hw := iface.HardwareAddr
		if len(hw) >= len(item.Config.NodeMac) {
			copy(item.Config.NodeMac[:], hw[:len(item.Config.NodeMac)])
		} else {
			// if MAC shorter (unlikely), copy what we have
			copy(item.Config.NodeMac[:], hw)
		}

		// set Mode based on port.Mode string (adjust mapping as needed)
		switch port.Mode {
		case config.InterfaceModeLayer2:
			item.Config.Mode = 0x01
		case config.InterfaceModeLayer3:
			item.Config.Mode = 0x02
		default:
			slog.Error("Unsupported interface mode", "interface", port.Name, "mode", port.Mode)
		}
		if item.Config.Mode == 0 {
			slog.Error("Invalid interface mode, skipping interface", "interface", port.Name, "mode", port.Mode)
			break
		}

		// update map
		if err := objs.IfaceConfigMap.Update(uint32(iface.Index), item, ebpf.UpdateAny); err != nil {
			slog.Error("failed to pre-configure map", "interface", port.Name, "error", err)
			os.Exit(1)
		}

		// attach XDP program to the interface
		lXdp, err := link.AttachXDP(link.XDPOptions{
			Program:   objs.XdpProcess,
			Interface: iface.Index,
		})
		if err != nil {
			slog.Error("could not attach XDP program", "error", err)
			break
		}
		links = append(links, lXdp)

		// attach TC program to the interface
		lXtc, err := link.AttachTCX(link.TCXOptions{
			Program:   objs.TcIngress,
			Attach:    ebpf.AttachTCXIngress,
			Interface: iface.Index,
		})
		if err != nil {
			slog.Error("could not attach TC program", "error", err)
		}
		links = append(links, lXtc)
	}
	if len(links) == 0 {
		slog.Error("Failed to attach BPF program to any interface, exiting")
		os.Exit(1)
	}
	if len(links) != len(cfg.Ports)*2 {
		for _, l := range links {
			l.Close()
		}
		slog.Error("Failed to attach XDP program to all interfaces, exiting", "attached", len(links), "expected", len(cfg.Ports)*2)
		os.Exit(1)
	}
	defer func() {
		for _, l := range links {
			l.Close()
		}
	}()

	ticker := time.NewTicker(1 * time.Second)
	defer ticker.Stop()
	for range ticker.C {
		s, err := formatFdbTables(objs.FdbTable)
		if err != nil {
			slog.Error("Error reading fdb map", "error", err)
			continue
		}
		slog.Info("FDB table", "data", s)
	}
}

func formatFdbTables(m *ebpf.Map) (string, error) {
	var sb strings.Builder
	var key dataplaneFdbKey
	var val dataplaneFdbValueItem

	iter := m.Iterate()
	for iter.Next(&key, &val) {
		msg := fmt.Sprintf("\t%s, %d => %d, %d\n", key.Mac, key.VlanId, val.Value.Ifindex, val.Value.LastSeen)
		sb.WriteString(msg)
	}

	return sb.String(), iter.Err()
}

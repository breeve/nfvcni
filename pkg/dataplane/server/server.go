package server

import (
	"context"
	"net"
	"sync"

	"github.com/breeve/nfvcni/pkg/dataplane/server/proto"
	"github.com/cilium/ebpf"
	"log/slog"
	"google.golang.org/grpc"
)

type Server struct {
	proto.UnimplementedDataplaneServiceServer
	grpcServer *grpc.Server

	ifaceConfigMap *ebpf.Map
	fdbMap         *ebpf.Map

	mu      sync.RWMutex
	ports   map[uint32]*PortInfo
	links   map[uint32]interface{}
}

type PortInfo struct {
	Name    string
	Mode    string
	Ifindex uint32
}

func NewServer() *Server {
	return &Server{
		ports: make(map[uint32]*PortInfo),
		links: make(map[uint32]interface{}),
	}
}

func (s *Server) SetMaps(ifaceConfigMap, fdbMap *ebpf.Map) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.ifaceConfigMap = ifaceConfigMap
	s.fdbMap = fdbMap
}

func (s *Server) Start(addr string) error {
	lis, err := net.Listen("tcp", addr)
	if err != nil {
		slog.Error("Failed to listen", "addr", addr, "error", err)
		return err
	}

	s.grpcServer = grpc.NewServer()
	proto.RegisterDataplaneServiceServer(s.grpcServer, s)

	slog.Info("Starting dataplane gRPC server", "addr", addr)
	return s.grpcServer.Serve(lis)
}

func (s *Server) Stop() {
	if s.grpcServer != nil {
		s.grpcServer.GracefulStop()
	}
}

func (s *Server) AttachPort(ctx context.Context, req *proto.AttachPortRequest) (*proto.AttachPortResponse, error) {
	slog.Info("AttachPort request", "ifindex", req.Ifindex)

	s.mu.Lock()
	defer s.mu.Unlock()

	if _, exists := s.ports[req.Ifindex]; exists {
		return &proto.AttachPortResponse{
			Success: false,
			Error:   "port already attached",
		}, nil
	}

	portInfo := &PortInfo{
		Name:    req.Config.GetName(),
		Mode:    req.Config.GetMode(),
		Ifindex: req.Ifindex,
	}
	s.ports[req.Ifindex] = portInfo

	return &proto.AttachPortResponse{
		Success: true,
	}, nil
}

func (s *Server) DetachPort(ctx context.Context, req *proto.DetachPortRequest) (*proto.DetachPortResponse, error) {
	slog.Info("DetachPort request", "ifindex", req.Ifindex)

	s.mu.Lock()
	defer s.mu.Unlock()

	if _, exists := s.ports[req.Ifindex]; !exists {
		return &proto.DetachPortResponse{
			Success: false,
			Error:   "port not found",
		}, nil
	}

	delete(s.ports, req.Ifindex)

	return &proto.DetachPortResponse{
		Success: true,
	}, nil
}

func (s *Server) AddFdbEntry(ctx context.Context, req *proto.AddFdbEntryRequest) (*proto.AddFdbEntryResponse, error) {
	slog.Info("AddFdbEntry request", "mac", req.Mac, "vlan", req.VlanId, "ifindex", req.Ifindex)

	s.mu.RLock()
	fdbMap := s.fdbMap
	s.mu.RUnlock()

	if fdbMap == nil {
		return &proto.AddFdbEntryResponse{
			Success: false,
			Error:   "fdb map not initialized",
		}, nil
	}

	mac, err := parseMAC(req.Mac)
	if err != nil {
		return &proto.AddFdbEntryResponse{
			Success: false,
			Error:   "invalid MAC address",
		}, nil
	}

	key := &dataplaneFdbKey{
		Mac:    mac,
		VlanId: uint16(req.VlanId),
	}

	value := &dataplaneFdbValueItem{
		Value: dataplaneFdbValue{
			Ifindex:  req.Ifindex,
			LastSeen:  0,
		},
	}

	if err := fdbMap.Update(key, value, ebpf.UpdateAny); err != nil {
		return &proto.AddFdbEntryResponse{
			Success: false,
			Error:   err.Error(),
		}, nil
	}

	return &proto.AddFdbEntryResponse{
		Success: true,
	}, nil
}

func (s *Server) DelFdbEntry(ctx context.Context, req *proto.DelFdbEntryRequest) (*proto.DelFdbEntryResponse, error) {
	slog.Info("DelFdbEntry request", "mac", req.Mac, "vlan", req.VlanId)

	s.mu.RLock()
	fdbMap := s.fdbMap
	s.mu.RUnlock()

	if fdbMap == nil {
		return &proto.DelFdbEntryResponse{
			Success: false,
			Error:   "fdb map not initialized",
		}, nil
	}

	mac, err := parseMAC(req.Mac)
	if err != nil {
		return &proto.DelFdbEntryResponse{
			Success: false,
			Error:   "invalid MAC address",
		}, nil
	}

	key := &dataplaneFdbKey{
		Mac:    mac,
		VlanId: uint16(req.VlanId),
	}

	if err := fdbMap.Delete(key); err != nil {
		return &proto.DelFdbEntryResponse{
			Success: false,
			Error:   err.Error(),
		}, nil
	}

	return &proto.DelFdbEntryResponse{
		Success: true,
	}, nil
}

func (s *Server) GetFdbTable(ctx context.Context, req *proto.GetFdbTableRequest) (*proto.GetFdbTableResponse, error) {
	slog.Info("GetFdbTable request")

	s.mu.RLock()
	fdbMap := s.fdbMap
	s.mu.RUnlock()

	if fdbMap == nil {
		return &proto.GetFdbTableResponse{
			Entries: []*proto.FdbEntry{},
		}, nil
	}

	var entries []*proto.FdbEntry
	var key dataplaneFdbKey
	var val dataplaneFdbValueItem

	iter := fdbMap.Iterate()
	for iter.Next(&key, &val) {
		mac := net.HardwareAddr(key.Mac[:6]).String()
		entries = append(entries, &proto.FdbEntry{
			Mac:      mac,
			VlanId:   uint32(key.VlanId),
			Ifindex:  val.Value.Ifindex,
			LastSeen: int64(val.Value.LastSeen),
		})
	}

	return &proto.GetFdbTableResponse{
		Entries: entries,
	}, nil
}

func (s *Server) GetPortConfig(ctx context.Context, req *proto.GetPortConfigRequest) (*proto.GetPortConfigResponse, error) {
	slog.Info("GetPortConfig request", "ifindex", req.Ifindex)

	s.mu.RLock()
	defer s.mu.RUnlock()

	portInfo, exists := s.ports[req.Ifindex]
	if !exists {
		return &proto.GetPortConfigResponse{
			Error: "port not found",
		}, nil
	}

	return &proto.GetPortConfigResponse{
		Config: &proto.PortConfig{
			Name: portInfo.Name,
			Mode: portInfo.Mode,
		},
	}, nil
}

func (s *Server) UpdatePortConfig(ctx context.Context, req *proto.UpdatePortConfigRequest) (*proto.UpdatePortConfigResponse, error) {
	slog.Info("UpdatePortConfig request", "ifindex", req.Ifindex)

	s.mu.Lock()
	defer s.mu.Unlock()

	portInfo, exists := s.ports[req.Ifindex]
	if !exists {
		return &proto.UpdatePortConfigResponse{
			Success: false,
			Error:   "port not found",
		}, nil
	}

	if req.Config != nil {
		portInfo.Name = req.Config.GetName()
		portInfo.Mode = req.Config.GetMode()
	}

	return &proto.UpdatePortConfigResponse{
		Success: true,
	}, nil
}

type dataplaneFdbKey struct {
	Mac    [6]uint8
	VlanId uint16
}

type dataplaneFdbValue struct {
	Ifindex  uint32
	LastSeen uint64
}

type dataplaneFdbValueItem struct {
	Lock  uint32
	Value struct {
		Ifindex  uint32
		LastSeen uint64
	}
}

func parseMAC(macStr string) ([6]uint8, error) {
	mac, err := net.ParseMAC(macStr)
	if err != nil {
		return [6]uint8{}, err
	}

	var result [6]uint8
	copy(result[:], mac)
	return result, nil
}
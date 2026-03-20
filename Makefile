.PHONY: bpf2go
bpf2go:
	go get github.com/cilium/ebpf/cmd/bpf2go
	go install github.com/cilium/ebpf/cmd/bpf2go@latest

.PHONY: cc
cc:
	sudo apt install clang llvm libbpf-dev libelf-dev zlib1g-dev linux-tools-common linux-tools-generic bear

.PHONY: cc_wsl
cc_wsl:
	sudo apt install linux-tools-virtual hwdata

.PHONY: gateway_dataplane_vmlinux_h
gateway_dataplane_vmlinux_h:
	/usr/lib/linux-tools/6.8.0-106-generic/bpftool btf dump file /sys/kernel/btf/vmlinux format c > pkg/dataplane/bpf/header/vmlinux.h
	
.PHONY: deps
deps: bpf2go cc cc_wsl

.PHONY: dataplane_bpf
dataplane_bpf: gateway_dataplane_vmlinux_h
	cd  pkg/dataplane/bpf; go generate

.PHONY: dataplane
dataplane: dataplane_bpf
	go build -o bin/dataplane pkg/dataplane/main.go

.PHONY: logout
logout:
	sudo /usr/lib/linux-tools/6.8.0-106-generic/bpftool prog tracelog

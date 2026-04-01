.PHONY: bpf2go
bpf2go:
	go get github.com/cilium/ebpf/cmd/bpf2go
	go install github.com/cilium/ebpf/cmd/bpf2go@latest

.PHONY: cc
cc:
	apt update && apt install -y clang llvm libbpf-dev libelf-dev zlib1g-dev linux-tools-common linux-tools-generic bear

.PHONY: cc_wsl
cc_wsl:
	apt update && apt install -y linux-tools-virtual hwdata

.PHONY: gateway_dataplane_vmlinux_h
gateway_dataplane_vmlinux_h:
	/usr/lib/linux-tools/6.8.0-107-generic/bpftool btf dump file /sys/kernel/btf/vmlinux format c > pkg/dataplane/bpf/header/vmlinux.h
	
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
	/usr/lib/linux-tools/6.8.0-106-generic/bpftool prog tracelog

.PHONY: bpf_check
bpf_check:
	rm -rf /sys/fs/bpf/test
	/usr/lib/linux-tools/6.8.0-106-generic/bpftool prog load pkg/dataplane/bpf/dataplane_bpfel.o /sys/fs/bpf/test

.PHONY: disable_xdp
disable_xdp:
	ip link set dev dummy0 xdp off

.PHONY: base_image
base_image:
	docker build -t nfvcni-base:latest -f dist/base.Dockerfile .

.PHONY: bpf_image
bpf_image: base_image
	docker build -t nfvcni-bpf:latest -f dist/bpf.Dockerfile .

.PHONY: dataplane_docker
dataplane_docker:
	docker run --rm -ti -v "$(shell pwd)":/workspaces/nfvcni -w /workspaces/nfvcni docker.io/library/nfvcni-bpf:latest make dataplane

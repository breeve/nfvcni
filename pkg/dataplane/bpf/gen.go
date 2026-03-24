package dataplane

//go:generate go run github.com/cilium/ebpf/cmd/bpf2go -cflags "-g -O2 -I/usr/include/x86_64-linux-gnu -I./header" -target bpfel dataplane dataplane.c

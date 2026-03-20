package dataplane

//go:generate go run github.com/cilium/ebpf/cmd/bpf2go -cflags "-I/usr/include/x86_64-linux-gnu"  -target bpfel bpf dataplane.c

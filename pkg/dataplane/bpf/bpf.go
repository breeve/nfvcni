package dataplane

import (
	"fmt"
	"log"
	"time"

	"github.com/cilium/ebpf/link"
	"github.com/cilium/ebpf/rlimit"
)

func Agent() {
	// 1. 移除内核对内存锁定的限制（eBPF 程序需要固定内存）
	if err := rlimit.RemoveMemlock(); err != nil {
		log.Fatal(err)
	}

	// 2. 加载生成的 eBPF 字节码到内核
	objs := bpfObjects{}
	if err := loadBpfObjects(&objs, nil); err != nil {
		log.Fatalf("loading objects: %v", err)
	}
	defer objs.Close()

	// 3. 将 eBPF 程序挂载到内核的 mkdir 系统调用入口
	// 注意：现代内核通常使用 __x64_sys_mkdir 或类似的符号
	kp, err := link.Kprobe("sys_mkdir", objs.CountMkdir, nil)
	if err != nil {
		log.Fatalf("opening kprobe: %v", err)
	}
	defer kp.Close()

	fmt.Println("正在监控 mkdir 调用... 按 Ctrl+C 退出")

	// 4. 定期从 Map 中读取计数
	ticker := time.NewTicker(1 * time.Second)
	for range ticker.C {
		var value uint64
		key := uint32(0)
		if err := objs.KprobeMap.Lookup(key, &value); err != nil {
			log.Fatalf("reading map: %v", err)
		}
		fmt.Printf("检测到 mkdir 累计调用次数: %d\n", value)
	}
}

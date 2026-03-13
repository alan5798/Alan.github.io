---
postTagTitle: 操作系统
title: xv6-lab4.1 
data: $(date '+%Y-%m-%d %H:%M:%S')
tags: [操作系统]
---
### 程序运行逻辑
在执行程序的时候，首先，它读取硬盘里的ELF（可执行文件），并把它拷贝到内存里，内存中单独开辟一个进程，进程仍然有MMU来进行内存分配，获得虚拟内存从0-MAX。这里0-MAX仍然按照从trampoline-0的分布。然后执行的时候，五步流水线的第一步，PC计数器（本质就是一个指针），指向text的最高位，然后按照指令的要求，执行。但是text当中往往没有数据，所以只是指示寄存器，比如add s1 s2 s3的意思是把s2,s3的寄存器中的数加和并放到s1中，s2,s3的数据从heap，stack，data中得到。
#### 内存分配图
![](memory_graph.png)
#### 运行逻辑图

![](logic_graph.png)

### 返回地址ra
对于具体运行中的程序，PC计数器（本质是指针）会随着函数的调用不断跳转，为了在执行完这个程序还能找到上一个函数位置在哪里，我们需要将上一个函数后面要执行代码的地址存入一个寄存器当中，我们通常将这个寄存器叫做ra。
但是由于函数可能层层调用，而ra会被复写，所以我们需要维护一个ra链表，以保证程序的顺利运行。ra等地址所在的位置就是在stack中。
它的结构大概是这样的
```
struct Node{
	void* ra;//返回地址
	Node* fp;//前一个节点的位置
}
```
---
## trap——程序中断处理
正如上图中的内存示意图，每一个进程都被分配了一个trapframe，trapframe中存储了这个进程当下的所有信息
```
struct trapframe {
  /*   0 */ uint64 kernel_satp;   // kernel page table
  /*   8 */ uint64 kernel_sp;     // top of process's kernel stack
  /*  16 */ uint64 kernel_trap;   // usertrap()
  /*  24 */ uint64 epc;           // saved user program counter
  /*  32 */ uint64 kernel_hartid; // saved kernel tp
  /*  40 */ uint64 ra;
  /*  48 */ uint64 sp;
  /*  56 */ uint64 gp;
  /*  64 */ uint64 tp;
  /*  72 */ uint64 t0;
  /*  80 */ uint64 t1;
  /*  88 */ uint64 t2;
  /*  96 */ uint64 s0;
  /* 104 */ uint64 s1;
  /* 112 */ uint64 a0;
  /* 120 */ uint64 a1;
  /* 128 */ uint64 a2;
  /* 136 */ uint64 a3;
  /* 144 */ uint64 a4;
  /* 152 */ uint64 a5;
  /* 160 */ uint64 a6;
  /* 168 */ uint64 a7;
  /* 176 */ uint64 s2;
  /* 184 */ uint64 s3;
  /* 192 */ uint64 s4;
  /* 200 */ uint64 s5;
  /* 208 */ uint64 s6;
  /* 216 */ uint64 s7;
  /* 224 */ uint64 s8;
  /* 232 */ uint64 s9;
  /* 240 */ uint64 s10;
  /* 248 */ uint64 s11;
  /* 256 */ uint64 t3;
  /* 264 */ uint64 t4;
  /* 272 */ uint64 t5;
  /* 280 */ uint64 t6;
}
```
我们对trapframe的调用可能会遇两种不同的情形

* 内核中断了这个进程，比如一个进程过长时间地占用了cpu，这个时候内核强制将PC程序计数器跳转，而为了保存当前局面，我们需要保留这个trapframe，而因为kalloc是上锁 ，所以不会有其他进程能修改这个trapframe，我们只需要p->trapframe就足够了

* 如果进程内想要中断程序，进入内核，并跳转，那我们就需要另一个单独的trapframe来保存目前的局面了，因为epc本身会时时刻刻更改。
























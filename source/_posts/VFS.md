---
postTagTitle: 操作系统
title: xv6-lab9 文件管理系统VFS
data: $(date '+%Y-%m-%d %H:%M:%S')
tags: [操作系统]
---
### VFS的基础逻辑图
![](VFS.png)
首先值得强调的一点是，这个逻辑图应该稍有问题，以我学下来的理解logging part应该在Buffer cache的下面。
这张图只是先从逻辑上先对OS如何操作硬盘进行一个分析，后面会具体分解
### 磁盘内部结构

**回答的问题：磁盘是如何储存数据的，内部结构如何？**
![](Disk.png)
这是xv6对于磁盘内部结构图的示意，对于磁盘而言，类似内存，我们也对其进行划分，和内存中page对应的，在磁盘中叫做block。那么自然而言的，大部分的磁盘空间肯定需要用来保存数据，所以被我们用来存放data，在图中也就是D。但是由于文件大小不一，不可能放在一起，同时文件又是我们操作系统操作的基本单位，所以就需要一些struct来管理文件，这些struct就是inode，也就是图中I的内容，在xv6中他们的大小是64Byte；关于inode后面会详细讲解。在处理完inode和data之后，我们还会想到需要一些数据来记录每一个block，是不是已经被分配了，还是未被处理，所以我们需要bit_map来记录信息，所以我们有了两个bit_map分别记录inode和data的信息，他们只有0，1组成，只记录每一个位置是不是被alloc了。
### inode的定义

**回答的问题：inode是什么，我们为什么需要inode**

```
struct inode {
  uint dev;           // Device numbr
  uint inum;          // Inode number
  int ref;            // Reference count
  struct sleeplock lock; // protects everything below here
  int valid;          // inode has been read from disk?

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint sizejj;
  uint addrs[NDIRECT+1];
};
```
inode的核心职能就是储存文件的关键信息，比如大小，比如设备号等等，而在这当中，有几个比较重要的信息，一个是inum，这是这个文件唯一的inode号，通过inum可以找到对应的inode，第二个是uint addrs，这是文件组织block的关键，所有的block都是按照addrs按照顺序组织。

值得注意的是对于磁盘中，我们还有一个dinode，而且这个dinode 的大小是64Byte，专门设计出来的，为了能够在一个1024的大小中刚好塞下16个dinode，所以相比上面的inode少了一些信息。
### 多级映射

**回答的问题：inode是如何组织文件的，inode怎么把文件所在的不同block连在一起**

关于addrs,在xv6中NDIRECT=12，但是一个文件显然可能不止$12\times4KB$，所以第12位就会留出来作为一个映射，它指向一个block的地址，而这个block则被用来专门储存地址。如果更大的文件，则可以以此类推，将倒数第二位流出来给一级映射，将最后一位留出来给二级映射。
具体的操作可以看下面这段代码，这是bmap（一级映射的逻辑）
```
static uint
bmap(struct inode *ip, uint bn)
{
  uint addr, *a;
  struct buf *bp;
  if(bn < NDIRECT){
    if((addr = ip->addrs[bn]) == 0){
      addr = balloc(ip->dev);
      if(addr == 0)
        return 0;
      ip->addrs[bn] = addr;
    }
    return addr;
  }
  bn -= NDIRECT;
  if(bn < NINDIRECT){
    // Load indirect block, allocating if necessary.
    if((addr = ip->addrs[NDIRECT]) == 0){
      addr = balloc(ip->dev);
      if(addr == 0)
        return 0;
      ip->addrs[NDIRECT] = addr;
    }
    bp = bread(ip->dev, addr);
    a = (uint*)bp->data;
    if((addr = a[bn]) == 0){
      addr = balloc(ip->dev);
      if(addr){
        a[bn] = addr;
        log_write(bp);
      }
    }
    brelse(bp);
    return addr;
  }
  panic("bmap: out of range");
 }
```

### Pathname and Directory

**回答的问题：当我们已经理解了文件是如何组织的，那操作系统是如何找到一个具体的文件的**

Pathname 和Directory 的解析就是一个递归的过程。首先，在UNIX哲学中，一切皆文件，文件夹本身也是一个文件，包含了各个文件的inumber和自己的以及父目录的内容
![](route_analysis.png)
如图所示，在记录好这些inumber后，解析路径的程序从根目录开始，依次递归地寻找所需要的文件，即根据自己的inumber，找到对应的inode，找到存储的data，读取出该目录下的name，进行比对，进而再找到对应的inumber，开启下一个循环。出于习惯，根目录的inumber一般是2。而由于IO操作是非常慢的，所以当进程找到对应的文件时，往往会保存每次解析得到的各文件夹和该文件的inumber，以避免反复重复解析。
### Buffer and Logging

**回答的问题：磁盘的每次IO操作是很慢的，同时由于断电风险引发的磁盘内部错位危机严重，我们有什么方法避免解决呢**

Buffer 通过LRU原则（Least Recent Used)，将最新用过的信息储存在cache（cache本身在内存里），再刷入硬盘，通过这样的方式，如果cache已经满了的话，那就删掉least recent used的block。由于绝大多数时候需要使用的文件会反复使用，cache可以极大地提高效率。

Logging则是用来解决断电危机的，由于一旦断电，内存里的内容将无法保存，所以我们需要保证重要的内容已经被写入硬盘。但是当我们需要传一个大文件的时候，大文件传到一半断电会导致硬盘中保存了错误的文件，为了解决该问题，我们设置了Logging 区，Logging区本身也在硬盘里，不过它不会立刻刷入data，它会等文件传输完之后的finished标签，如果得到了finished，则一起刷入data，如果没有，则在下次执行时把logging的部分全部删除，以保证文件的完整性。由于logging本身在一起，所以不同于需要磁针反复乱跳的data区，logging本身在硬盘上操作效率较高。
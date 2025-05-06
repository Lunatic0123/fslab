# FSLab

```
 ________ ________  ___       ________  ________     
|\  _____\\   ____\|\  \     |\   __  \|\   __  \    
\ \  \__/\ \  \___|\ \  \    \ \  \|\  \ \  \|\ /_   
 \ \   __\\ \_____  \ \  \    \ \   __  \ \   __  \  
  \ \  \_| \|____|\  \ \  \____\ \  \ \  \ \  \|\  \ 
   \ \__\    ____\_\  \ \_______\ \__\ \__\ \_______\
    \|__|   |\_________\|_______|\|__|\|__|\|_______|
            \|_________|                             
                                                                                                
```

中国人民大学（RUC）FSLab，原作者：Liang Junkai,RUC

模板与问题反馈仓库：[FSLab](https://github.com/RUCICS/FSLab)

## 环境指南

本实验对系统的要求如下：

| System | Support |
| ------ | ------- |
| Ubuntu 20.04 LTS | ✔️ |
| Ubuntu 22.04 LTS | ✔️ |
| Ubuntu 24.04 LTS | ✔️ |

典型场景是用 WSL2 进行开发，如果你不具有这个环境，比如你是 Mac 用户，请访问 [https://ics.ruc.panjd.net](https://ics.ruc.panjd.net) 获取服务器登陆信息，在服务器上完成本实验（该网站和服务器都仅能在校园网下访问，如果你不在校内，你应该使用校园网 VPN）。

> 提醒：在 Docker 里使用 fuse 是一个比较麻烦的事情，请有这个想法的同学提前做好心理准备

### 依赖项

本试验依赖 `fuse2.9.9`，`python3` 和 `gcc`

理论上，Ubuntu20.04 - 24.04 的 Ubuntu 系统，你可以通过以下命令来安装 `fuse2.9.9`，经过测试，其版本都是 `2.9.9`，如果你担心版本问题，最保险的是从源代码编译安装

```bash
sudo apt install fuse libfuse-dev
```

虽然目前（2025.5.2）网上的资料称 Ubuntu22.04 后会默认安装 `fuse3`，但是助教在 Ubuntu22.04 和 Ubuntu24.04 上测试了一下，目前按照 `apt install fuse` 或者 `apt install libfuse-dev` 安装的仍然是 `fuse2`，但是以防万一，安装后请自行检查一下这点，比如通过以下命令：

```bash
fusermount --version
# fusermount version: 2.9.9
```

> 经过测试，在上个月发布的 Ubuntu25.04 上，`apt install fuse` 确实开始安装 `3.14.0` 版本的了，而且似乎已经弃用了 fuse2，所以使用极新 Ubuntu 版本的同学注意，你可能需要从源代码编译安装。
>
> 考虑到 Ubuntu25.04 不是 LTS 版本，我们预计本次作业不会有人遇到这个问题。

#### 从源码编译安装

```bash
mkdir -p ~/tmp/fuse
cd ~/tmp/fuse # 不要直接在作业目录里安装，以免你不小心把它干进 git 里了
wget https://github.com/libfuse/libfuse/releases/download/fuse-2.9.9/fuse-2.9.9.tar.gz
tar -xvf fuse-2.9.9.tar.gz

# 下载地址2，如果你访问不了 github.com
# wget http://archive.ubuntu.com/ubuntu/pool/universe/f/fuse/fuse_2.9.9.orig.tar.gz
# tar -xvf fuse_2.9.9.orig.tar.gz

cd fuse-2.9.9
sed -i 's/closefrom/closefrom0/g' util/ulockmgr_server.c # 手动修复一个新版本系统上的编译错误
./configure
make -j
sudo make install # 如果你没有 sudo 权限，你可以 AI 一下怎么安装到用户目录下
fusermount --version
# fusermount version: 2.9.9
# 如果版本有误，请重启终端，它可能找到之前安装的 fuse3 去了
```

## 如何编译/测试

### 编译与挂载脚本

```bash
make                    # 编译文件系统
make mount              # 初始化虚拟块和文件系统，挂载文件系统到 mnt/ 文件夹上，后台执行（看不到文件系统里的 printf）
make umount             # 卸载文件系统（你可以理解为进到 make mount 的进程里按 Ctrl+C）
make debug              # 类似 make mount，但是在前台运行，方便查看输出和调试，此时你需要新开一个终端来操作
# 为了方便，每次 mount/debug 之前会自动 umount 一次

# 高级用法，开始写代码时再来看即可
make fuse               # 和 make 一样，编译文件系统
make mount_noinit       # 不初始化虚拟快和文件系统，只挂载已有的文件系统（make mount 类似于装机时执行的，make mount_noinit 类似每次开机执行的）
make debug_noinit       # 略
# 同样，mount_noinit/debug_noinit 也会自动 umount

make cleand             # 强制删除虚拟磁盘文件夹，虚拟磁盘运行时依赖的文件，以及挂载的 mnt/ 文件夹，make mount 和 make debug 会自动执行
make init               # 创建虚拟磁盘文件夹，，虚拟磁盘运行时依赖的文件，mnt/ 文件夹，make mount 和 make debug 会自动执行
make clean              # 在 make cleand 的基础上，删除编译产物

# 看到和 umount 有关报错是正常的，是脚本为了保险起见 umount 了可能没有 mount 的文件系统
# fusermount: entry for /home/jarden/fslab/mnt not found in /etc/mtab
# make: [Makefile:27: umount] Error 1 (ignored)
# 或者是
# umount: mnt: No such file or directory
# make: [Makefile:49: cleand] Error 1 (ignored)
# 他们都被 ignored 了
```

### 运行

当你执行完 `make mount` 或 `make debug` 后，`mnt/` 用的就是你的文件系统，比如

```bash
cd mnt
ls
ls -a
touch test
```

其中 `ls` 或者 `touch` 指令就会调用你写的文件系统中的 `getattr`，`readdir`，`opendir`，`mknod` 之类的接口函数来完成和文件系统的交互，达成显示当前文件夹所有文件，或者是创建文件的功能。但是我们并不需要知道这些接口是怎么被调用的，本实验只需要完成这些接口，实现一个文件系统。

### 测试

测试点是一些 shell 脚本，位于 `tests/traces/` 下。

我们有三类测试点，以下解释一下他们的测试流程。

#### 普通测试点

普通测试点假设执行之前文件系统都是空的，它的测试流程类似这样：

```bash
make mount
bash ./tests/traces/01.sh
```

#### 持久化测试点

持久化测试点测试是否正确实现了持久化，即卸载文件系统后重新挂载是否能正确运行，它的测试流程类似这样：

```bash
make mount
bash ./tests/traces/p0.sh
# Hello
make mount_noinit
bash ./tests/traces/p0.sh
# Hello
# Hello

cat ./tests/traces/p0.ans
# Hello
# Hello
# Hello
```

#### 开放测试点

这些测试点的测试流程和普通测试点是一样的，但是其不存在标准答案，所以我们需要你自己执行这些脚本，并在报告中解释其合理性

```bash
make mount
bash ./tests/traces/o0.sh
```

#### 测试脚本

为了避免一些怪问题，比如 `ls` 在不同系统版本上的输出格式不一样，我们会先用测试平台的默认文件系统生成一遍答案，
再和自己实现的对比，理论上我们的测试点保证不会比较我们没有规定的，文件系统之间可能不同的输出部分，
但如果确实发现这种问题，麻烦和助教反馈。

仅生成标准答案以便 debug，请先运行一遍：

```bash
python3 tests/test.py --answer
```

一次性测试所有点的脚本是：

```bash
python3 tests/test.py
```

如果你只想测试其中的一些点：

```bash
python3 tests/test.py -t 1,10,18,p0
```

脚本除了比较和默认文件系统的输出，还会分别输出耗时，以及你的实现速度达到了默认文件系统的百分之多少。

如果你想 debug，我们建议按照上面我们提到的流程，自己执行 shell 脚本，而不要在 python 脚本里尝试。

> 在一些比较差的实现下，有一些测试点执行起来可能非常非常非常耗时（数十分钟甚至更久），比如 `17`，`18` 和 `19`，
> 为此我们有对应的小规模测试点，分别是 `14`，`15` 和 `16`，但是小规模测试点能通过并不代表大规模测试点能通过。

## 代码同步

如果上游修改了，我们现在建议且仅建议手动用 git 同步，而不是用 github classroom：

```bash
git remote add template https://github.com/RUCICS/fslab.git
git pull template main
```

更多可能的问题请查看 [everything-you-should-know](https://github.com/RUCICS/everything-you-should-know)

## 问题简述

本次试验需要你实现一个文件系统，为了简化问题我们用一个虚拟的块设备，以及 fuse（Filesystem in Userspace）以在用户态实现文件系统逻辑并挂载到一个目录下，阅读这个章节可以让你对整个故事的背景有个快速的了解。

### 虚拟块设备

我们提供了接口来模拟块设备读写，他们定义在 `disk.c` 里，分别是 `int disk_read(int block_id, void* buffer)` 和 `int disk_write(int block_id, void* buffer)`，通过他们，你可以“读写”虚拟的块设备。

我们定义了这个虚拟块设备的参数在 `disk.h` 里，比如 `#define BLOCK_SIZE 4096` 表示一个块的大小是 4096 字节，`#define BLOCK_NUM 65536` 表示一共有 65536 个块，总共 256MB。

综上，你可以这样读写一个块，注意块设备以一个块为单位访问。

```c
char buf[BLOCK_SIZE];
disk_read(0, buf); // ?xxxxxxx
buf[0] = 'a';
disk_write(0, buf); // axxxxxxx
```

### FUSE 与文件系统

文件系统的作用是提供一些文件系统接口，并帮助使用这些接口的程序管理文件（的内容存储，布局，目录结构，读写，删除，权限，信息）。

> 文件系统其实不一定和硬盘这种一般的存储设备绑定，只要你提供了这些接口，你也可以在内存里管理这些东西，比如确实有一些内存文件系统，它的目的是提供极快的速度，不在乎持久化之类的问题。
> 
> 但是本问题中，你应该假设你做的是一个一般的文件系统，并具有持久化保存数据的功能。即课上教的那种。

#### 文件系统需要提供哪些接口

fuse 提供了一个方式让你可以在用户态实现一个文件系统并让 Linux 内核使用它，具体来说，他会拦截系统对文件系统的操作，并传递给你写的文件系统接口来处理，本实验中，你需要实现的接口如下：

| 函数           | 功能                                                         |
|----------------|--------------------------------------------------------------|
| fs_getattr     | 查询一个目录文件或常规文件的信息                             |
| fs_readdir     | 查询一个目录文件下的所有文件                                 |
| fs_read        | 对一个常规文件进行读取操作                                   |
| fs_mkdir       | 创建一个目录文件                                             |
| fs_rmdir       | 删除一个目录文件                                             |
| fs_unlink      | 删除一个常规文件                                             |
| fs_rename      | 更改一个目录文件或常规文件的名称（或路径）                  |
| fs_truncate    | 修改一个常规文件的大小信息                                   |
| fs_utimens     | 修改一个目录文件或常规文件的时间信息                         |
| fs_mknod       | 创建一个常规文件                                             |
| fs_write       | 对一个常规文件进行写操作                                     |
| fs_statfs      | 查询文件系统整体的统计信息                                   |
| fs_open        | 打开一个常规文件                                             |
| fs_release     | 关闭一个常规文件                                             |
| fs_opendir     | 打开一个目录文件                                             |
| fs_releasedir  | 关闭一个目录文件                                             |

当你正确实现这些函数后，你用这些函数指针构造一个结构体传给 fuse 运行，一个文件系统就完成了。只要这个程序在运行，他就会重复我们上面所说的，拦截文件系统的操作，传递给你处理，并用你的结果返回给调用文件系统的程序，比如 `ls` 之类的。

```c
static struct fuse_operations fs_operations = {.getattr = fs_getattr,
                                               .readdir = fs_readdir,
                                               .read = fs_read,
                                               .mkdir = fs_mkdir,
                                               .rmdir = fs_rmdir,
                                               .unlink = fs_unlink,
                                               .rename = fs_rename,
                                               .truncate = fs_truncate,
                                               .utimens = fs_utimens,
                                               .mknod = fs_mknod,
                                               .write = fs_write,
                                               .statfs = fs_statfs,
                                               .open = fs_open,
                                               .release = fs_release,
                                               .opendir = fs_opendir,
                                               .releasedir = fs_releasedir};

int main(int argc, char* argv[])
{
    return fuse_main(argc, argv, &fs_operations, NULL);
}
```

## 开发指南

本节将具体介绍项目的框架，你应该如何实现这个文件系统，每个接口应该做什么。

### 项目框架

- `disk.h`：声明了 `disk.c` 中的函数，以及虚拟块设备的参数如 `BLOCK_SIZE`、`BLOCK_NUM`，你不应该修改这个文件。
- `logger.h`：声明了 `logger.c` 中的函数，你可以用来打印调试信息，或者用来打印错误信息，你不应该修改这个文件。
- `fs.c`：文件系统的主体部分，你需要在此基础上实现文件系统的每一个接口，同时你需要定义要用到的所有数据结构（bitmap，INODE 之类的），并补全文件系统初始化之类的代码。

### 实现思路

这里我们给出一个经典的 bitmap + inode + data block 的文件系统的实现思路，你可以参考这个思路来实现你的文件系统。

我们快速回顾一下这种文件系统的设计：

文件系统将磁盘划分为固定大小的块（block），一般对应于磁盘的最小读写单位，本实验中就是 4K。

整个磁盘空间被分为以下区域：

- 超级块（Superblock）：存储文件系统的元数据，如块大小、总块数、inode 数等。
- inode 位图（Inode Bitmap）：记录 inode 的分配状态。
- 数据块位图（Data Bitmap）：记录数据块的分配状态（0 表示空闲，1 表示已分配）。
- inode 表（Inode Table）：存储每个文件的元数据（如文件大小、权限、时间戳、数据块指针等）。
- 数据块（Data Blocks）：存储文件的实际内容。

文件和目录被抽象为一个概念，本次实验我们称为条目（Entry）。

#### 文件条目

文件的元信息和数据块的指针被存储在 inode 中。

具体来说，每个 inode 由元数据，指向数据块的值接指针，和一级间接指针（和可能的更多级指针）组成。

直接指针直接指向数据块，你可以通过存数据块序号的方式来存储。

> 以下的指针都指这种序号，而不是一般意义的 64 位的指针

间接指针先指向一个数据块，这个数据块里存储了大量指向数据块的直接指针，这样你就可以通过间接指针来访问更多的数据块。

举个例子，假如一个 inode 有 3 个直接指针和 1 个一级间接指针，每个指针用 int32_t 类型存储，每个块的大小为 4KB，
那么 3 个直接指针可以直接指向 3 * 4KB = 12KB 的数据，1 个一级间接指针可以指向 1 * (4096 / 4) = 1024 个数据块，
即 1 * 1024 * 4KB = 4MB 的数据。一共可以指向 4MB + 12KB 的数据。

由此我们就有了文件的存储方法，只要找到文件对应的 inode，我们就可以读取他的元数据，并且通过这些数据块指针遍历出数据。

#### 目录条目

目录可以视为一种特殊的文件，普通的文件存储真实的内容数据，目录存储的则是其下所有的条目，具体来说，其需要存储条目的名称和 inode 指针。

一般，我们将根目录的 inode 固定在一个位置，比如 0 号 inode，假如我们需要读取 `/dir1/dir2/file` 的内容，我们先要找到他对应的 inode。

为此，我们可以先找到 `/` 的 inode，然后类似遍历文件的数据块一样遍历其下的所有条目，找到 `dir1` 后记录 `/dir1` 的 inode，递归下去，直到找到目标条目。

#### 增删改

以上我们描述了怎么在文件系统里查询，我们还需要实现文件系统的增删改。

文件和目录实际上可以被统一起来处理，他们几乎没有区别，以创建一个文件为例，大致流程是：

- 先分配一个 inode
    - 在 inode 的 bitmap 上找一个为 0 的位置（读到 buffer，查，改，写回磁盘）
    - 在内存上初始化一个 inode
    - 读 inode 的目标位置的磁盘块到 buffer 里
    - 把 inode 写到 buffer 里
    - 把 buffer 写回磁盘
- 通过 `path` 解析出其父目录的 inode
    - 遍历 entry，一级一级比较
- 在父目录的 inode 末尾记录这个新的 inode
    - 遍历 entry，直到找到最后一个位置
    - 分别处理在直接指针里，还是在间接指针的数据块里的情况（如果恰好分配到间接指针，还要分配间接指针的数据块）
        - 分配间接指针的数据块和分配文件的数据块是类似的
            - 在 data 的 bitmap 上找一个为 0 的位置（可能还要读两次 buffer，因为这个 bitmap 应该会超过一个 block）
            - 把这个位置的序号写到间接指针里
            - 把新的 entry 的内容（名字和 inode 序号）写到刚分配的数据块里

删除和修改的流程和这个也差不多。

#### 代码实现建议

整个文件系统有大量可以复用的重复逻辑，我们建议先把每个函数要做什么捋清楚了，把共同部分大致抽象出来后再开始写代码。

我们建议用回调函数的方式来实现一些复用的功能，比如实现一个遍历 data block 的函数，再传一个函数指针进去做下一步处理，
比如直接读到 buffer，或者是当作目录，读取 inode + 名称等等。如果回调函数的灵活性不够，你也可以用一个大型的宏定义来处理。

还有一种性能比较差但是简单的实现方式，你可以实现一个取第i个block或第i个entry出来的函数，但这可能导致每一次调用会有写放大的问题，你要小心性能问题。

#### 其他细节

我们在 `fs.c` 里写了大量的注释，这些注释大致解释了你应该在每个函数里实现什么功能，我们没有完整地把所有细节都列出来，而且错误处理部分我们只列了必须实现的部分。

如果你想知道完整的实现应该是什么样的，知道所有应该处理的错误，你可以参考 `man 2 xxx`，比如 `man 2 open` 可以查看 `.open = fs_open` 中这个 `open` 应该做什么。

但是理论上，按照注释里提到的实现就够了。

### 性能优化

你可以自己实现一个 cache 来缓存磁盘读写，比如使用 LRU 算法来缓存最近使用的磁盘块。

理论上，你只要封装一下原来的 `disk_read/write` 即可，不会影响到其他代码，但是注意在文件系统关闭的时候确保所有的 cache 落盘，你可以这部分逻辑写在 `fs_finalize` 函数里。

### `.` 和 `..` 的细节说明

我们不需要真的在每个目录里实现 `.` 和 `..` 条目，虽然一般的文件系统可能是要的，
但是如果你去试一下，`fuse` 的处理方法类似于在文件系统外先把这些相对路径的问题都处理好了，
`fuse` 调用我们写的函数时永远是以绝对地址传入的。

不过为了形式上的统一，我们建议 `fs_readdir` 的时候返回一下 `.` 和 `..`，这样 `ls -a` 的行为就是和正常文件系统一致的了。

### `fs_open` 的细节说明

```c
int fs_open(const char* path, struct fuse_file_info* fi)
```

#### 关于 `fi->fh` 的使用：

`fi->fh` 是 fuse 预留的一个 uint64_t 字段，他一般用于加速文件系统的实现（没有他不影响你实现你的文件系统）

我们以一段 C 代码举例其一种利用方式

```c
int fd = open(filename, O_RDWR);
char buffer[1024];
read(fd, buffer, sizeof(buffer) - 1);
read(fd, buffer, sizeof(buffer) - 1);
read(fd, buffer, sizeof(buffer) - 1);
```

fuse 在处理时会保证每一次调用 `fs_read` 的时候会传入和调用 `fs_open` 时相同的
`struct fuse_file_info* fi`，这样你可以保证某些操作只在 open 的时候执行一次。
比如你可以用来存储 inode 的位置，这样就不用每次都重复用 `path` 来查找 inode
了。 如果想存储更复杂的内容，你可以存储一个指向结构体的指针，并在 `fs_release`
的时候释放掉。

#### 关于 fi->flags

`fi->flags` 中记录了打开文件时的标记，比如 `O_RDONLY`，`O_WRONLY`，`O_RDWR`，还有 `O_APPEND`，`O_CREAT` 等等。

由于本实验不考虑权限和多线程问题，所以我们仅就几个可能涉及到的标记进行说明，本说明仅供参考，可能在不同的操作系统或者环境下有不一样的行为，如果你验证后发现不一样的行为请和助教反馈。

> 你可以使用指令 `man 2 open` 或者在 https://man7.org/linux/man-pages/man2/open.2.html 里查看标准 Linux 内核对 `open` 的定义。
>
> 建议的最保险的做法是都实现，虽然下文我们在解释为什么你可能可以什么都不实现。

##### O_CREAT

按照 Linux 规定的处理方式，`O_CREAT` 需要在文件不存在时创建一个文件，但在助教的测试里，fuse 会依次调用 `fs_mknod` 和 `fs_open`，并且调用 `fs_open` 时没有包含 `O_CREAT`，所以理论上你可以不在 `open` 里处理这点。

##### O_APPEND

按照 Linux 的规定，其要求对用这个标记打开的文件，在每一次调用 `write` 时，无论任何情况下都要将文件指针移动到文件末尾开始写。

比如

```c
int fd = open(filename, O_WRONLY | O_APPEND);
lseek(fd, 0, SEEK_SET);
write(fd, "appended\n", 9);
```

如果没有 `O_APPEND`，那么内容会从文件的开头开始写入，但是在使用 `O_APPEND` 时，内容应该写到文件的末尾。

但在助教的测试里，fuse 会在调用 `fs_write` 时自动传入正确的 `offset`，所以理论上你也可以不处理这点。

## 实现要求和分数

本次实验我们模拟在块大小 4096 Byte，一共 256MB 容量的磁盘上实现一个文件系统。

我们对这个文件系统有如下要求或规定：

- 至少 250MB 的可用空间（即数据块的空间）
- 至少支持 32768 个文件和目录（即 inode 的数量）
- 文件和目录名至多 24 字节，同目录下不存在同名文件或目录
- 你应该保证数据的一致性，比如一个操作进行到一半发现错误了，你应该保证这个操作不会对文件系统造成破坏，比如你申请了 inode 完发现 data 不够，你应该回滚 inode 的修改
- 不用支持链接，并发和权限管理
- 文件系统运行过程中占用的内存不得超过 128KB（比如，你不能把整个磁盘文件 mmap 到内存里，但是你可以用一些缓存机制来利用少量的内存加速）
- 代码编译时不应该有任何 Warning，运行过程中不应该出现内存泄漏

> 本实验不要求，也非常不建议尝试实现并发，如果你想实现并发，你可能需要自己查阅资料理解 `fuse` 的并发处理方法，并修改 `Makefile` 中的启动参数。但是和并发不同，你可以在一次调用里使用多线程，比如多线程遍历查找，当然这也远远超过了我们对本实验的要求，而且在本实验的测试用例里不一定会有什么性能提升。

### 分数

- 正确性（60%）：不包括开放测试点，每个点的得分相等
- 代码质量（10%）
- 报告（30%）：包括开放测试点，性能，代码质量，和额外实现的功能加分

> 正确性方面，没能 100% 正确也没有关系，在不抄的情况下，即使你参考了别人的实现，本次作业的难度也还是非常高的，不应该所有人都能在规定时间里完全正确地写出来，如果所有人都是全对反而有点不对劲

### 诚信

本实验会进行查重，如果你参考了网上的资料，或者是和同学讨论了实现思路，
请在报告中标注，这不会影响查重，查重是客观的，但是可以方便后续查重出现问题时的调查。

请保证你的报告是准确真实的，并让助教可以复现你提到的内容。

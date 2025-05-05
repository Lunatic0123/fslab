// clang-format off
#include "config.h"
// clang-format on

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <fuse/fuse.h>
#include <libgen.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

#include "disk.h"
#include "fs_opt.h"
#include "logger.h"

// 默认的文件和目录的标志
#define DIRMODE (S_IFDIR | 0755)
#define REGMODE (S_IFREG | 0644)

// 一些辅助宏定义
#define ceil_div(a, b) (((a) + (b) - 1) / (b))
#define min(a, b) ((a) < (b) ? (a) : (b))

// 初始化文件系统
//
// 参考实现：
// 当 init_flag 为 1 时，你应该：
// 1. 初始化一个超级块记录必要的参数
// 2. 初始化根节点，bitmap
//
// 当 init_flag 为 0 时，你应该：
// 1. 加载超级快
//
// 提示：
// 1. 由于我们没有脚本测试自定义初始化文件系统的参数，
// 所以你可以假设文件系统的参数是固定的，此时其实可以不要超级块（如果你确实不需要）
// 2. 如果你打算实现一个可以自定义初始化参数的文件系统，
// 你可以用环境变量来传递参数，或者参考 `fs_opt.c` 中的实现方法
int fs_mount(int init_flag) {
  fs_info("fs_mount is called\tinit_flag:%d)\n", init_flag);

  return 0;
}

// 关闭文件系统前的清理工作
//
// 如果你有一些工作要在文件系统被完全关闭前完成，比如确保所有数据都被写入磁盘，或是释放内存，请在
// fs_finalize 函数中完成，你可以假设 fuse_status 永远为 0，即 fuse
// 永远会正常退出，该函数当且仅当清理工作失败时返回非零值
int fs_finalize(int fuse_status) { return fuse_status; }

// 查询一个文件或目录的属性
//
// 错误处理：
// 1. 条目不存在时返回 -ENOENT
//
// 参考实现：
// 1. 根据 path 从根目录开始遍历，找到 inode
// 2. 通过 inode 中存储的信息，填充 attr 结构体
//
// 提示：
// 1. 所有接口中的 path 都是相对于该文件系统根目录开始的绝对路径，相关的讨论见
// README.md
//
// `stat` 会触发该函数，实际上 `cd` 的时候也会触发，这个函数被触发的情景特别多
int fs_getattr(const char *path, struct stat *attr) {
  fs_info("fs_getattr is called:%s\n", path);

  // 这是一个示例实现，模拟一个空文件系统，以保证你能正常执行 `cd mnt` 命令
  if (strcmp(path, "/") != 0) {
    return -ENOENT;
  }
  struct timespec ts;
  clock_gettime(CLOCK_REALTIME, &ts);
  *attr = (struct stat){
      .st_mode =
          DIRMODE, // 记录条目的类型，权限等信息，本实验由于不考虑权限等高级功能，你只需要返回
                   // DIRMODE 当条目是一个目录时；返回 REGMODE
                   // 当条目是一个文件时
      .st_nlink = 1,      // 固定返回 1 即可，因为我们不考虑链接
      .st_uid = getuid(), // 固定返回当前用户的 uid
      .st_gid = getgid(), // 固定返回当前用户的 gid
      .st_size = 0,       // 返回占据的数据块的大小
      .st_atim = ts,      // 最后访问时间
      .st_mtim = ts,      // 最后修改时间（内容）
      .st_ctim = ts,      // 最后修改时间（元数据）
      .st_blksize = 4096, // 文件的最小分配单位大小（字节记）
      .st_blocks = 0, // 占据的块数（以 512 字节为一块，这是历史原因的规定，和
                      // st_blksize 中的不一样）
  };

  return 0;
}

// 查询一个目录下的所有条目名（文件，目录）（忽略 offset 参数）
//
// 错误处理：
// 1. 目录不存在时返回 -ENOENT
//
// 参考实现：
// 1. 根据 path 从根目录开始遍历，找到 inode
// 2. 遍历该 inode（目录）下的所有条目（文件，目录），
// 对每一个条目名（文件名，目录名）name，调用 filler(buffer, name, NULL, 0)
// 3. 修改被查询目录的 atime（即被查询 inode 的 atime）
//
// `ls` 命令会触发这个函数
int fs_readdir(const char *path, void *buffer, fuse_fill_dir_t filler,
               off_t offset, struct fuse_file_info *fi) {
  fs_info("fs_readdir is called:%s\n", path);

  // 示例实现
  filler(buffer, ".", NULL, 0);
  filler(buffer, "..", NULL, 0);

  return 0;
}

// 从 offset 位置开始读取至多 size 字节内容到 buffer 中
//
// 错误处理：
// 文件不存在时返回 -ENOENT
//
// 参考实现：
// 1. 通过 path 找到 inode，或者通过之前 fs_open 记录的 fi->fh 直接找到 inode
// 2. 读取从 offset 开始的 size 字节内容到 buffer 中，但是不能超过 inode->size
// 3. 更新 inode 的 atime
// 4. 返回实际读取的字节数
//
// `cat` 命令会触发这个函数
int fs_read(const char *path, char *buffer, size_t size, off_t offset,
            struct fuse_file_info *fi) {
  fs_info("fs_read is called:%s\tsize:%d\toffset:%d\n", path, size, offset);

  return 0;
}

// 创建一个文件（忽略 mode 和 dev 参数）
//
// 错误处理：
// 1. 文件已存在时返回 -EEXIST
// 2. 没有足够的空间时返回 -ENOSPC
//
// 参考实现：
// 1. 通过 path 找到其父目录的 inode
// 2. 创建并初始化一个新的 inode，其为 stat 记录的 `st_mode` 为 `REGMODE`
// 3. 在父目录的 inode 中添加一个新的目录项，指向新创建的 inode
// 4. 更新父目录的 inode 的 mtime，ctime
//
// `touch` 命令会触发这个函数
int fs_mknod(const char *path, mode_t mode, dev_t dev) {
  fs_info("fs_mknod is called:%s\n", path);

  return 0;
}

// 创建一个目录（忽略 mode 参数）
//
// 和 fs_mknod 几乎一模一样，
// 唯一的区别是其对应的 stat 记录的 `st_mode` 为 `DIRMODE`
int fs_mkdir(const char *path, mode_t mode) {
  fs_info("fs_mkdir is called:%s\n", path);

  return 0;
}

// 删除一个文件
//
// 错误处理：
// 1. 文件不存在时返回 -ENOENT
//
// 参考实现：
// 1. 通过 path 找到其父目录的 inode，记作 parent_inode
// 2. 在 parent_inode 中删除该文件的 inode，记该 inode 为 child_inode
// 3. 遍历 child_inode 的 data_block 标记释放，最后标记释放 child_inode
// 4. 更新 parent_inode 的 mtime，ctime
//
// `rm` 命令会触发该函数
int fs_unlink(const char *path) {
  fs_info("fs_unlink is callded:%s\n", path);

  return 0;
}

// 删除一个目录
//
// 和 `fs_unlink` 的实现几乎一模一样
//
// 提示：
// 1. 调用该接口时系统会保证该目录下为空
// （即查询你实现的 readdir ，返回的内容为空）
//
// `rmdir` 命令会触发该函数
// 事实上，`rm -rf` 时的处理方法是系统自己调用 `ls, cd, rm, rmdir`
// 来处理递归删除，而不是交给文件系统来处理递归
int fs_rmdir(const char *path) {
  fs_info("fs_rmdir is called:%s\n", path);

  return 0;
}

// 移动一个条目（文件或目录）
//
// 错误处理：
// 略
//
// 参考实现：
// 一个代码复用性比较好的实现方式是
// 1. 先做一个不标记释放 data block 的 fs_unlink
// 2. 做一个用已有 inode 的 fs_mknod
// （即原本是创建一个新的 inode，现在是用 oldpath 对应的那个）
// 3. 记得同时更新新旧父目录的 mtime
//
// 思考：
// 1. 如果移动的是目录，目录下的内容要怎么处理
//
// `mv` 命令会触发该函数
int fs_rename(const char *oldpath, const char *newpath) {
  fs_info("fs_rename is called:%s\tnewpath:%s\n", oldpath, newpath);

  return 0;
}

// 从 offset 开始写入 size 字节的内容到文件中
//
// 错误处理：
// 1. 文件不存在时返回 -ENOENT
// 2. 没有足够的空间时返回 -ENOSPC
// 3. 超过单文件大小限制时返回 -EFBIG
//
// 参考实现：
// 1. 通过 path 找到 inode，或者通过之前 fs_open 记录的 fi->fh 直接找到 inode
// 2. 如果 fi->flags 中有 O_APPEND 标志，设置 offset 到文件末尾
// 3. 如果写入后的文件大小超过已经分配的数据块大小，新分配足够的数据块
// 4. 遍历 inode 的所有数据块，找到并修改对应的数据块
// 5. 更新 inode 的 mtime，ctime
// 6. 返回实际写入的字节数
//
// `echo "hello world" > test.txt` 命令会触发这个函数
int fs_write(const char *path, const char *buffer, size_t size, off_t offset,
             struct fuse_file_info *fi) {
  fs_info("fs_write is called:%s\tsize:%d\toffset:%d\n", path, size, offset);

  return 0;
}

// 修改一个文件的大小（即分配或释放数据块）
//
// 错误处理：
// 1. 文件不存在时返回 -ENOENT
// 2. 没有足够的空间时返回 -ENOSPC
// 3. 超过单文件大小限制时返回 -EFBIG
//
// 参考实现：
// 注意分别处理增大和减小的情况
// 1. 计算需要的数据块数
// 2. 分配或释放数据块（以及 inode 中的记录）
// 3. 修改 inode 的 ctime
int fs_truncate(const char *path, off_t size) {
  fs_info("fs_truncate is called:%s\tsize:%d\n", path, size);

  return 0;
}

// 修改条目的 atime 和 mtime
//
// 参考实现：
// 1. 通过 path 找到 inode
// 2. 根据传入的 tv 参数（分别是 atime 和 mtime）修改 inode 的 atime 和 mtime
// 3. 更新 inode 的 ctime（因为 utimens 本身修改了元数据）
int fs_utimens(const char *path, const struct timespec tv[2]) {
  fs_info("fs_utimens is called:%s\n", path);

  return 0;
}

// 获取文件系统的状态
//
// 根据自己的文件系统填写即可，实现这个函数是可选的
//
// `df mnt` 和 `df -i mnt` 会触发这个函数
int fs_statfs(const char *path, struct statvfs *stat) {
  fs_info("fs_statfs is called:%s\n", path);

  *stat = (struct statvfs){
      .f_bsize = 0,  // 块大小（字节记）
      .f_blocks = 0, // 总数据块数
      .f_bfree = 0, // 空闲的数据块数量（包括 root 用户可用的）
      .f_bavail = 0, // 空闲的数据块数量（不包括 root 用户可用的）
      // 由于我们要求实现权限管理，上面两个值应该是相同的
      .f_files = 0, // 文件系统可以创建的条目数量（相当于 inode 数量）
      .f_ffree = 0, // 空闲的 inode 数量（包括 root 用户可用的）
      .f_favail = 0, // 空闲的 inode 数量（不包括 root 用户可用的）
      .f_namemax = 0, // 文件名的最大长度
  };

  return 0;
}

// 会在打开一个文件时被调用，完整的细节见 README.md
//
// 参考实现：
// 不考虑 `fs->fh` 时，这个函数事实上可以什么都不干
int fs_open(const char *path, struct fuse_file_info *fi) {
  fs_info("fs_open is called:%s\tflag:%o\n", path, fi->flags);

  return 0;
}

// 会在一个文件被关闭时被调用，你可以在这里做相对于 `fs_open` 的一些清理工作
int fs_release(const char *path, struct fuse_file_info *fi) {
  fs_info("fs_release is called:%s\n", path);

  return 0;
}

// 类似于 `fs_open`，本实验中可以不做任何处理
int fs_opendir(const char *path, struct fuse_file_info *fi) {
  fs_info("fs_opendir is called:%s\n", path);

  return 0;
}

// 类似于 `fs_release`，本实验中可以不做任何处理
int fs_releasedir(const char *path, struct fuse_file_info *fi) {
  fs_info("fs_releasedir is called:%s\n", path);

  return 0;
}

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

int main(int argc, char *argv[]) {
  // 理论上，你不需要也不应该修改 main 函数内的代码，只需要实现对应的函数

  int init_flag = !has_noinit_flag(&argc, argv);
  // 通过 make mount 或者 make debug 启动时，该值为 1
  // 通过 make mount_noinit 或者 make debug_noinit 启动时，该值为 0

  if (disk_mount(init_flag)) { // 不需要修改
    fs_error("disk_mount failed!\n");
    return -1;
  }

  if (fs_mount(init_flag)) { // 该函数用于初始化文件系统，实现细节见函数定义
    fs_error("fs_mount failed!\n");
    return -2;
  }

  int fuse_status = fuse_main(argc, argv, &fs_operations, NULL);
  // Ctrl+C 或者 make umount（fusermount） 时，fuse_main
  // 会退出到这里而不是整个程序退出

  // 如果你有一些工作要在文件系统被完全关闭前完成，比如确保所有数据都被写入磁盘，请在
  // fs_finalize 函数中完成
  return fs_finalize(fuse_status);
}

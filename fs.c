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
#define MAX_FILENAME_LEN 24
#define INODE_COUNT 32768

#define INODE_SIZE sizeof(inode_t)
#define INODES_PER_BLOCK (BLOCK_SIZE / INODE_SIZE)
#define ENTRIES_PER_BLOCK (BLOCK_SIZE / sizeof(dir_entry_t))
#define POINTERS_PER_BLOCK (BLOCK_SIZE / sizeof(int))

#define DIRECT_POINTERS 12
#define INDIRECT_POINTERS 2
#define MAX_FILE_SIZE ((DIRECT_POINTERS + INDIRECT_POINTERS * POINTERS_PER_BLOCK) * BLOCK_SIZE)
typedef struct inode{
    uint32_t size;
    uint32_t atime;
    uint32_t mtime;
    uint32_t ctime;
    uint32_t mode;
    uint32_t direct_block_pointer[DIRECT_POINTERS];
    uint32_t indirect_block_pointer[INDIRECT_POINTERS];
} inode_t;
struct superblock{
    int num_inodes;
    int num_data_blocks;
    int inode_table_blocks;
    int data_bitmap_blocks;
    int data_blocks_start;
} sb;
typedef struct dir_entry {
    char name[26]; // 示例
    uint32_t inode_num;
} dir_entry_t;

// 磁盘布局: 块号
#define SUPERBLOCK_BLOCK 0
#define INODE_BITMAP_BLOCK 1
#define DATA_BITMAP_START_BLOCK 2 // 数据位图占用2块
#define INODE_TABLE_START_BLOCK 4


int get_inode_by_path(const char *path, int *parent_inode_num, char *filename);
int read_inode(int inode_num, inode_t *inode);
int write_inode(int inode_num, const inode_t *inode);
int alloc_inode();
uint32_t get_directory_block_addr(struct inode *dir_inode, uint32_t block_index);
int find_entry_in_directory(struct inode *dir_inode, const char *name, uint32_t *inode_index);
int find_inode_by_path(const char *path, uint32_t *inode_index);

void free_inode(int inode_num);
int alloc_data_block();
void free_data_block(int block_num);
void update_timestamp(inode_t *inode, bool access, bool modify, bool change);
int add_dir_entry(inode_t *parent_inode, int parent_inode_num, const char *filename, int new_inode_num);
int get_block_num(inode_t *inode, int file_block_idx, bool allocate);
void free_all_data_blocks(inode_t *inode);
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

    if(init_flag){
        sb.num_inodes = INODE_COUNT;
        sb.inode_table_blocks = ceil_div(sb.num_inodes * sizeof(inode_t), BLOCK_SIZE);
        sb.data_bitmap_blocks = 2; // 根据设计计算得出
        sb.data_blocks_start = INODE_TABLE_START_BLOCK + sb.inode_table_blocks;
        sb.num_data_blocks = BLOCK_NUM - sb.data_blocks_start;

        char block[BLOCK_SIZE];
        memset(block, 0, BLOCK_SIZE);
        memcpy(block, &sb, sizeof(sb));
        disk_write(SUPERBLOCK_BLOCK, block);

        // 初始化所有位图和Inode表
        memset(block, 0, BLOCK_SIZE);
        for (int i = INODE_BITMAP_BLOCK; i < sb.data_blocks_start; ++i) {
            disk_write(i, block);
        }

        // 初始化根目录
        int root_inode_num = alloc_inode();
        if (root_inode_num != 0) {
            fs_error("Root inode is not 0\n");
            return -1;
        }

        inode_t root_inode;
        memset(&root_inode, 0, sizeof(inode_t));
        root_inode.mode = DIRMODE;
        root_inode.size = 0; // Empty dir initially
        update_timestamp(&root_inode, true, true, true);
        write_inode(root_inode_num, &root_inode);
    }
    else{
        // 加载超级块
        disk_read(SUPERBLOCK_BLOCK, &sb);  
    }
    return 0;
}

// 关闭文件系统前的清理工作
//
// 如果你有一些工作要在文件系统被完全关闭前完成，比如确保所有数据都被写入磁盘，或是释放内存，请在
// fs_finalize 函数中完成，你可以假设 fuse_status 永远为 0，即 fuse
// 永远会正常退出，该函数当且仅当清理工作失败时返回非零值
int fs_finalize(int fuse_status) {
    return fuse_status;
}

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
int fs_getattr(const char* path, struct stat* attr) {
    fs_info("fs_getattr is called:%s\n", path);
    uint32_t inode_index;
    inode_t target;
      // 根据路径查找inode
    if (find_inode_by_path(path, &inode_index) != 0) {
        return -ENOENT; // 文件不存在
    }

  // 读取inode信息
    if (read_inode(inode_index, &target) != 0) {
        return -ENOENT;
    }
    *attr = (struct stat){
        .st_mode =
            target.mode,         // 记录条目的类型，权限等信息，本实验由于不考虑权限等高级功能，你只需要返回
                             // DIRMODE 当条目是一个目录时；返回 REGMODE
                             // 当条目是一个文件时
        .st_nlink = 1,       // 固定返回 1 即可，因为我们不考虑链接
        .st_uid = getuid(),  // 固定返回当前用户的 uid
        .st_gid = getgid(),  // 固定返回当前用户的 gid
        .st_size = target.size,        // 返回文件大小（字节记）
        .st_atim = target.atime,       // 最后访问时间
        .st_mtim = target.mtime,       // 最后修改时间（内容）
        .st_ctim = target.ctime,       // 最后修改时间（元数据）
        .st_blksize = BLOCK_SIZE,  // 文件的最小分配单位大小（字节记）
        .st_blocks = (target.size + 511) / 512,      // 实际占据的数据块数（以 512
                             // 字节为一块，这是历史原因的规定，和 st_blksize
                             // 中的不一样），这个块数需要考虑文件系统实现的实际情况，
                             // 比如间接指针分配的那个数据块也应该算在这里。
                             // 比如 `stat fs.c` 里的 `Blocks:` 显示的就是这个值
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
int fs_readdir(const char* path, void* buffer, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* fi) {
    fs_info("fs_readdir is called: %s\n", path);

    uint32_t inode_num;
    if (find_inode_by_path(path, &inode_num) != 0) {
        return -ENOENT;
    }

    inode_t dir_inode;
    if (read_inode(inode_num, &dir_inode) != 0) {
        return -ENOENT; 
    }

    if (!S_ISDIR(dir_inode.mode)) {
        return -ENOENT;
    }

    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    dir_entry_t entries[ENTRIES_PER_BLOCK];

    uint32_t num_blocks_to_check = ceil_div(dir_inode.size, BLOCK_SIZE);

    for (uint32_t i = 0; i < num_blocks_to_check; i++) {
        uint32_t block_addr = get_directory_block_addr(&dir_inode, i);

        if (block_addr == 0 || disk_read(block_addr, entries) != 0) {
         continue;
        }

        // 遍历块内的所有目录项
        for (int j = 0; j < ENTRIES_PER_BLOCK; j++) {
            if (entries[j].inode_num != 0) {
                if (filler(buffer, entries[j].name, NULL, 0) != 0) {
                    // 如果 FUSE 的缓冲区满了, 提前结束并返回成功
                    fs_warning("filler buffer is full, returning early.\n");
                    return 0;
                }
            }
        }
    }


    update_timestamp(&dir_inode, true, false, false);
    write_inode(inode_num, &dir_inode);

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
int fs_read(const char* path, char* buffer, size_t size, off_t offset, struct fuse_file_info* fi) {
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
int fs_mknod(const char* path, mode_t mode, dev_t dev) {
    fs_info("fs_mknod is called:%s\n", path);

    return 0;
}

// 创建一个目录（忽略 mode 参数）
//
// 和 fs_mknod 几乎一模一样，
// 唯一的区别是其对应的 stat 记录的 `st_mode` 为 `DIRMODE`
int fs_mkdir(const char* path, mode_t mode) {
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
int fs_unlink(const char* path) {
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
int fs_rmdir(const char* path) {
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
int fs_rename(const char* oldpath, const char* newpath) {
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
int fs_write(const char* path, const char* buffer, size_t size, off_t offset, struct fuse_file_info* fi) {
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
int fs_truncate(const char* path, off_t size) {
    fs_info("fs_truncate is called:%s\tsize:%d\n", path, size);

    return 0;
}

// 修改条目的 atime 和 mtime
//
// 参考实现：
// 1. 通过 path 找到 inode
// 2. 根据传入的 tv 参数（分别是 atime 和 mtime）修改 inode 的 atime 和 mtime
// 3. 更新 inode 的 ctime（因为 utimens 本身修改了元数据）
int fs_utimens(const char* path, const struct timespec tv[2]) {
    fs_info("fs_utimens is called:%s\n", path);

    return 0;
}

// 获取文件系统的状态
//
// 根据自己的文件系统填写即可，实现这个函数是可选的
//
// `df mnt` 和 `df -i mnt` 会触发这个函数
int fs_statfs(const char* path, struct statvfs* stat) {
    fs_info("fs_statfs is called:%s\n", path);

    *stat = (struct statvfs){
        .f_bsize = 0,   // 块大小（字节记）
        .f_blocks = 0,  // 总数据块数
        .f_bfree = 0,   // 空闲的数据块数量（包括 root 用户可用的）
        .f_bavail = 0,  // 空闲的数据块数量（不包括 root 用户可用的）
        // 由于我们要求实现权限管理，上面两个值应该是相同的
        .f_files = 0,    // 文件系统可以创建的条目数量（相当于 inode 数量）
        .f_ffree = 0,    // 空闲的 inode 数量（包括 root 用户可用的）
        .f_favail = 0,   // 空闲的 inode 数量（不包括 root 用户可用的）
        .f_namemax = 0,  // 文件名的最大长度
    };

    // 这里的块数量是以最开头 `f_bsize` 的块大小记的
    // 如果你实现了稀疏文件，这里的块指的都是实际分配的块

    return 0;
}

// 会在打开一个文件时被调用，完整的细节见 README.md
//
// 参考实现：
// 不考虑 `fs->fh` 时，这个函数事实上可以什么都不干
int fs_open(const char* path, struct fuse_file_info* fi) {
    fs_info("fs_open is called:%s\tflag:%o\n", path, fi->flags);

    return 0;
}

// 会在一个文件被关闭时被调用，你可以在这里做相对于 `fs_open` 的一些清理工作
int fs_release(const char* path, struct fuse_file_info* fi) {
    fs_info("fs_release is called:%s\n", path);

    return 0;
}

// 类似于 `fs_open`，本实验中可以不做任何处理
int fs_opendir(const char* path, struct fuse_file_info* fi) {
    fs_info("fs_opendir is called:%s\n", path);

    return 0;
}

// 类似于 `fs_release`，本实验中可以不做任何处理
int fs_releasedir(const char* path, struct fuse_file_info* fi) {
    fs_info("fs_releasedir is called:%s\n", path);

    return 0;
}

// ---- 辅助函数实现 ----

int read_inode(int inode_num, inode_t *inode) {
    if (inode_num >= INODE_COUNT) {
        return -1; // 索引越界
    }
    int block_num = INODE_TABLE_START_BLOCK + (inode_num / INODES_PER_BLOCK);
    int offset_in_block = inode_num % INODES_PER_BLOCK;
    char block[BLOCK_SIZE];
    if(disk_read(block_num, block) != 0){
        return -1;
    }
    memcpy(inode, block + offset_in_block * INODE_SIZE, INODE_SIZE);
    return 0;
}

int write_inode(int inode_num, const inode_t *inode) {
    if (inode_num >= INODE_COUNT) {
        return -1;
    }
    int block_num = INODE_TABLE_START_BLOCK + (inode_num / INODES_PER_BLOCK);
    int offset_in_block = inode_num % INODES_PER_BLOCK;
    char block[BLOCK_SIZE];
    if(disk_read(block_num, block) != 0){
        return -1;
    }
    memcpy(block + offset_in_block * INODE_SIZE, inode, INODE_SIZE);
    if(disk_write(block_num, block) != 0){
        return -1;
    }
    return 0;
}

uint32_t get_directory_block_addr(struct inode *dir_inode, uint32_t block_index) {
    if (block_index < DIRECT_POINTERS) {
        return dir_inode->direct_block_pointer[block_index];
    }

    block_index -= DIRECT_POINTERS;
    uint32_t pointers_per_block = BLOCK_SIZE / sizeof(uint32_t);

    uint32_t indirect_group = block_index / pointers_per_block;
    uint32_t indirect_offset = block_index % pointers_per_block;

    if (indirect_group < 2) {
        uint32_t indirect_block_addr = dir_inode->indirect_block_pointer[indirect_group];
        if (indirect_block_addr == 0) return 0;

        uint32_t pointers[pointers_per_block];
        if (disk_read(indirect_block_addr, pointers) != 0) return 0;
        return pointers[indirect_offset];
    }
    
    return 0; // 超出范围
}
int find_entry_in_directory(struct inode *dir_inode, const char *name, uint32_t *inode_index) {
    if (!S_ISDIR(dir_inode->mode)) {
        return -1;
    }

    dir_entry_t dir_block[ENTRIES_PER_BLOCK];
    uint32_t num_blocks_to_check = ceil_div(dir_inode->size, BLOCK_SIZE);

    for (uint32_t i = 0; i < num_blocks_to_check; i++) {
        uint32_t block_addr = get_directory_block_addr(dir_inode, i);
        if (block_addr == 0 || disk_read(block_addr, dir_block) != 0) {
            continue; // 跳过稀疏块或读取失败的块
        }

        for (int j = 0; j < ENTRIES_PER_BLOCK; j++) {
            if (dir_block[j].inode_num != 0 && strcmp(dir_block[j].name, name) == 0) {
                *inode_index = dir_block[j].inode_num;
                return 0; // 成功找到，立即返回
            }
        }
    }

    return -1; // 遍历完成仍未找到
}
// 根据路径获取 inode 编号
int find_inode_by_path(const char *path, uint32_t *inode_index) {
    if (path == NULL || path[0] != '/') {
        return -1;
    }
    
    if (strcmp(path, "/") == 0) {
        *inode_index = 0;
        return 0;
    }
    char *path_copy = strdup(path);
    if (!path_copy) return -1;

    uint32_t current_ino = 0;
    int status = -1; // 默认状态为失败
    bool found = true; // 假设路径查找会成功

    char *saveptr;
    char *token = strtok_r(path_copy + 1, "/", &saveptr); 

    while (token != NULL) {
        struct inode current_inode;
        if (read_inode(current_ino, &current_inode) != 0 ||
            find_entry_in_directory(&current_inode, token, &current_ino) != 0) {
            found = false; // 查找失败
            break; // 退出循环
        }
        token = strtok_r(NULL, "/", &saveptr);
    }

    if (found) {
        *inode_index = current_ino;
        status = 0; // 只有在完全成功时才设置状态为0
    }

    free(path_copy); // 统一释放资源
    return status;
}

int alloc_inode() {//1
    char bitmap[BLOCK_SIZE];
    disk_read(INODE_BITMAP_BLOCK, bitmap);
    for (int i = 0; i < sb.num_inodes; ++i) {
        if (!((bitmap[i / 8] >> (i % 8)) & 1)) {
            bitmap[i / 8] |= (1 << (i % 8));
            disk_write(INODE_BITMAP_BLOCK, bitmap);
            return i;
        }
    }
    return -ENOSPC;
}

void free_inode(int inode_num) {
    char bitmap[BLOCK_SIZE];
    disk_read(INODE_BITMAP_BLOCK, bitmap);
    bitmap[inode_num/8] &= ~(1 << (inode_num % 8));
    disk_write(INODE_BITMAP_BLOCK, bitmap);
}

// // 在父目录中添加一个条目
// int add_dir_entry(inode_t *parent_inode, int parent_inode_num, const char *filename, int new_inode_num) {
//     dir_entry_t new_entry;
//     strncpy(new_entry.name, filename, MAX_FILENAME_LEN);
//     new_entry.name[MAX_FILENAME_LEN] = '\0';
//     new_entry.inode_num = new_inode_num;

//     // 遍历父目录的数据块寻找空位
//     // ... 此处需要一个完整的实现
//     // 如果没有空位，需要通过 get_block_num 分配新块
    
//     // 简化版：假设总是在末尾添加
//     int block_idx = parent_inode->size / BLOCK_SIZE;
//     int offset = parent_inode->size % BLOCK_SIZE;
    
//     if (offset == 0) { // 需要新块
//         int new_block = get_block_num(parent_inode, block_idx, true);
//         if (new_block < 0) return -ENOSPC;
//         write_inode(parent_inode_num, parent_inode); // get_block_num 可能修改了 inode
//     }

//     // 更新大小并写入
//     parent_inode->size += sizeof(dir_entry_t);
//     // ... (将 new_entry 写入到正确的块和偏移)

//     return 0;
// }

// 释放一个 inode 所有的 data blocks
void free_all_data_blocks(inode_t *inode) {
    // ... 必须遍历所有直接、一级间接指针，并调用 free_data_block
}

// 更新时间戳
void update_timestamp(inode_t *inode, bool access, bool modify, bool change) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts); 

    if (access) inode->atime = (uint32_t)ts.tv_sec; 
    if (modify) inode->mtime = (uint32_t)ts.tv_sec; 
    if (change) inode->ctime = (uint32_t)ts.tv_sec; 
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

int main(int argc, char* argv[]) {
    // 理论上，你不需要也不应该修改 main 函数内的代码，只需要实现对应的函数

    int init_flag = !has_noinit_flag(&argc, argv);
    // 通过 make mount 或者 make debug 启动时，该值为 1
    // 通过 make mount_noinit 或者 make debug_noinit 启动时，该值为 0

    if (disk_mount(init_flag)) {  // 不需要修改
        fs_error("disk_mount failed!\n");
        return -1;
    }

    if (fs_mount(init_flag)) {  // 该函数用于初始化文件系统，实现细节见函数定义
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

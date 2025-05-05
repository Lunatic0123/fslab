/*
请不要修改此文件
*/

#include "disk.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "logger.h"

#define NO_CACHE
#define DISK_NAME_SIZE 256

static char disk_prefix[DISK_NAME_SIZE];

// 初始化虚拟磁盘，提供仿真的块设备读写，从系统层次上看，位于文件系统的低一层
//
// 1. 从文件 `fuse~` 中读取虚拟磁盘目录的绝对地址（`make init`
// 时会创建虚拟磁盘目录和 `fuse~` 文件）
// 2. 如果 `init_flag` 为 1，则依次创建虚拟磁盘目录下的 `block0` 到 `block65535`
// 文件作为虚拟的块，并写入 BLOCK_SIZE 字节的 0，否则不做任何操作
int disk_mount(int init_flag) {
    FILE* fp = fopen("fuse~", "r");
    if (fp == NULL)
        return 1;
    if (fscanf(fp, "%s", disk_prefix) != 1) {
        fs_error("disk_mount: read disk prefix failed\n");
        fclose(fp);
        return 1;
    }
    fclose(fp);
    strcpy(disk_prefix + strlen(disk_prefix), "/block");

    if (!init_flag)
        return 0;

    char name[DISK_NAME_SIZE];
    char buffer[BLOCK_SIZE];
    memset(buffer, 0, sizeof(buffer));
    for (int i = 0; i < BLOCK_NUM; ++i) {
        strcpy(name, disk_prefix);
        sprintf(name + strlen(name), "%d", i);
        FILE* disk = fopen(name, "w");
        if (disk == NULL) {
            fs_error("disk_mount: create disk file failed\n");
            return 1;
        }
        if(fwrite(&buffer, BLOCK_SIZE, 1, disk) != 1){
            fs_error("disk_mount: write disk file failed\n");
            fclose(disk);
            return 1;
        }
        fclose(disk);
    }
    return 0;
}

// 读第 block_id 块的数据到 buffer 中，大小为 BLOCK_SIZE
//
// 该接口保证直接读磁盘文件，即不使用缓存（禁用缓冲区）
int disk_read(int block_id, void* buffer) {
    if (block_id >= BLOCK_NUM || block_id < 0)
        return 1;
    char name[DISK_NAME_SIZE];
    strcpy(name, disk_prefix);
    sprintf(name + strlen(name), "%d", block_id);
    FILE* disk = fopen(name, "r");
    if (disk == NULL) {
        fs_error("disk_read: open disk file failed\n");
        return 1;
    }
#ifdef NO_CACHE
    setvbuf(disk, NULL, _IONBF, 0);
#endif
    if (fread(buffer, BLOCK_SIZE, 1, disk) != 1) {
        fs_error("disk_read: read disk file failed\n");
        fclose(disk);
        return 1;
    }
    fclose(disk);
    return 0;
}

// 将 buffer 中的数据写入第 block_id 块，大小为 BLOCK_SIZE
//
// 该接口保证直接写磁盘文件，即不使用缓存（禁用缓冲区）
int disk_write(int block_id, void* buffer) {
    if (block_id >= BLOCK_NUM || block_id < 0)
        return 1;
    char name[DISK_NAME_SIZE];
    strcpy(name, disk_prefix);
    sprintf(name + strlen(name), "%d", block_id);
    FILE* disk = fopen(name, "w");
    if (disk == NULL){
        fs_error("disk_write: open disk file failed\n");
        return 1;
    }
#ifdef NO_CACHE
    setvbuf(disk, NULL, _IONBF, 0);
#endif
    if(fwrite(buffer, BLOCK_SIZE, 1, disk) != 1){
        fs_error("disk_write: write disk file failed\n");
        fclose(disk);
        return 1;
    }
    fclose(disk);
    return 0;
}

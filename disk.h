/*
请不要修改此文件
*/

#define BLOCK_SIZE 4096
#define BLOCK_NUM 65536
#define DISK_SIZE (BLOCK_SIZE * BLOCK_NUM)

int disk_mount(int init_flag);
int disk_read(int block_id, void* buffer);
int disk_write(int block_id, void* buffer);
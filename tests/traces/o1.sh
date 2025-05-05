#!/bin/bash
set -e

cd mnt
touch file0
touch file1
echo "HelloWorld" >> file0
for ((i=0;i<14;++i)); do
    cat file0 >> file1
	cat file1 >> file0
done
stat file0 | awk 'NR==1{print} NR==2{print $1, $2} NR==4{print}'
stat file1 | awk 'NR==1{print} NR==2{print $1, $2} NR==4{print}'

# 过程中应该会超过文件系统容量从而报错，你应该能够正确地返回 -ENOSPC，终端应该输出类似 error: No space left on device
for ((i=0;i<64;++i)); do
    cat file0 > ofile$i
	stat ofile$i | awk 'NR==1{print} NR==2{print $1, $2} NR==4{print}'
done
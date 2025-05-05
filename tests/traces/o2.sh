#!/bin/bash
set -e

cd mnt
touch file0
touch file1
echo "HelloWorld" >> file0

# 过程中应该会超过单文件最大容量从而报错，你应该能够正确地返回 -EFBIG，终端应该输出类似 error: File too large
# 当然，可能你的实现里支持的单文件大小很大，这里就不会报错
for ((i=0;i<16;++i)); do
    cat file0 >> file1
	cat file1 >> file0
done
stat file0 | awk 'NR==1{print} NR==2{print $1, $2} NR==4{print}'
stat file1 | awk 'NR==1{print} NR==2{print $1, $2} NR==4{print}'
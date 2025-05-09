#!/bin/bash
set -e

# 该测试点想考察稀疏文件的实现

cd mnt
touch file
head -c 1024 /dev/urandom > file
stat file -c %s
ls -l file
cat file | hexdump -C
truncate file -s 32
stat file -c %s
ls -l file
cat file | hexdump -C
truncate file -s 10000
stat file -c %s
ls -l file
cat file | hexdump -C
truncate file -s 24
stat file -c %s
ls -l file
cat file | hexdump -C

#!/bin/bash
set -e

cd mnt
echo 12345 > file1
cat file1
echo 23456789 > file1
cat file1
echo abcdefg >> file1
cat file1
dd if=file1 of=file2 bs=1 skip=4 count=8 status=none
cat file2
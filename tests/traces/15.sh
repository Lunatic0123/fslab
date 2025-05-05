#!/bin/bash
set -e

cd mnt
touch file0
touch file1
echo "HelloWorld" >> file0
for ((i=0;i<8;++i)); do
    cat file0 >> file1
	cat file1 >> file0
done
stat file0 | awk 'NR==1{print} NR==2{print $1, $2} NR==4{print}'
stat file1 | awk 'NR==1{print} NR==2{print $1, $2} NR==4{print}'

for ((i=0;i<8;++i)); do
    cat file0 > ofile$i
	stat ofile$i | awk 'NR==1{print} NR==2{print $1, $2} NR==4{print}'
done

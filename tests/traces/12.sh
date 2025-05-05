#!/bin/bash
set -e

cd mnt
touch file1
stat file1 | awk 'NR==1{print} NR==2{print $1, $2} NR==4{print}'
echo 23333 > file1
stat file1 | awk 'NR==1{print} NR==2{print $1, $2} NR==4{print}'

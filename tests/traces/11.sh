#!/bin/bash
set -e

cd mnt
cp ../fs.c fs.c
stat fs.c | awk 'NR==1{print} NR==2{print $1, $2} NR==4{print}'
diff fs.c ../fs.c

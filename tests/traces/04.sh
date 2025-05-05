#!/bin/bash
set -e

cd mnt
TZ=Asia/Shanghai touch file1 -t 201212210000.36
TZ=Asia/Shanghai stat file1 -c %x%y
TZ=Asia/Shanghai touch file2 -r file1
TZ=Asia/Shanghai stat file2 -c %x%y
TZ=Asia/Shanghai touch -d "2025-05-05 12:00:00.123456789" file3
TZ=Asia/Shanghai stat file3 -c %x%y
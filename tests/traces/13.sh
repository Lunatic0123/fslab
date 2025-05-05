#!/bin/bash
set -e

dd if=/dev/urandom of=tests/workspace/test.in bs=1M count=4 2>/dev/null
cp tests/workspace/test.in mnt/file1
cd mnt
mkdir dir1
cp file1 dir1/file2
cp dir1/file2 ../tests/workspace/test.out
cd ..
diff tests/workspace/test.in tests/workspace/test.out
stat tests/workspace/test.in | sed -n '1,2p;4p'

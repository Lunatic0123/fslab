#!/bin/bash
set -e

cd mnt
for ((i=0;i<64;++i)); do
	mkdir -p dir$i
	cd dir$i
	done
pwd
echo "Hello" >> test_file
cat test_file

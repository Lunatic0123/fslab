#!/bin/bash
set -e

cd mnt
for ((i=0;i<32;++i)); do
	mkdir "dir$i"
	touch "file$i"
	done
ls

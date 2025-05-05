#!/bin/bash
set -e

cd mnt
for ((i=0;i<128;++i)); do
	mkdir dir$i
	cd dir$i
	done
pwd

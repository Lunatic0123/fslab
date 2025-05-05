#!/bin/bash
set -e

cd mnt
for ((i=0;i<16;++i)); do
	mkdir dir$i
	cd dir$i
	done
pwd

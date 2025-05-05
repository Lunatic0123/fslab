#!/bin/bash
set -e

cd mnt
mkdir -p dir1
echo "Hello" >> dir1/test
cat dir1/test

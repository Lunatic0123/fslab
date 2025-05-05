#!/bin/bash
set -e

cd mnt
mkdir -p dir1/dir2/dir3/dir4
touch dir1/dir2/dir3/dir4/file4
touch dir1/dir2/dir3/file3
touch dir1/dir2/file2
touch dir1/file1
ls dir1/dir2
rm -rf dir1/dir2/dir3
ls dir1/dir2
rm -rf dir1
ls

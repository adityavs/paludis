#!/usr/bin/env bash
# vim: set ft=sh sw=4 sts=4 et :

mkdir dir_iterator_TEST_dir || exit 2
cd dir_iterator_TEST_dir || exit 3
touch file1 file2 .file3 || exit 4
ln file1 file4 || cp file1 file4 || exit 5

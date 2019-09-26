#!/bin/bash

zip font.kfpkg flash-list.json 16.FON 24.FON 32.FON

python3 merge_kfpkg.py font.kfpkg
rm font.kfpkg

python3 pack_kfpkg.py 1048576 font.bin font.kfpkg
rm font.bin
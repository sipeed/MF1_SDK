#!/bin/bash

set -e

#
dirty=`git describe --long --tag --dirty --always | awk -F "-" '{print $4}'`
version_full=`git describe --long --tag --dirty --always`
version=`echo $version_full | awk -F "-" '{print $1}'`
version_dev=`echo $version_full | awk -F "-" '{print $2}'`
version_git_rev=`echo $version_full | awk -F "-" '{print $3}'`
if [[ "x$version_dev" != "x" ]]; then
    version=${version}_${version_dev}_${version_git_rev}
fi
if [[ "x$dirty" == "xdirty" ]]; then
    echo -e "\033[33m [WARNING] coding is dirty!!, please commit all code firstly \033[0m"
    git status
fi
echo $version_full
echo $version

release_dir=`pwd`/bin/mf1_$version
rm -rf $release_dir $release_dir/elf
mkdir -p $release_dir
mkdir -p $release_dir/elf

#
cd ../../projects


#
cd MF1

echo "-------------------"
echo "build project MF1"
echo "long press key clear face feature"
echo "-------------------"

cp config_defaults.mk.1 config_defaults.mk

python3 project.py distclean
python3 project.py build
cp build/MF1.bin $release_dir/mf1_clr1_$version.bin
cp build/MF1.elf $release_dir/elf/mf1_clr1_$version.elf

echo "-------------------"
echo "build project MF1"
echo "long press key not clear face feature"
echo "-------------------"

cp config_defaults.mk.0 config_defaults.mk

python3 project.py distclean
python3 project.py build
cp build/MF1.bin $release_dir/mf1_clr0_$version.bin
cp build/MF1.elf $release_dir/elf/mf1_clr0_$version.elf

cd $release_dir
7z a elf_maixpy_${version}.7z elf/*
rm -rf elf

ls -al




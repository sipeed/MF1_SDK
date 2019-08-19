#!/bin/bash
set -e

rm -rf ../version.h

compile_time=`date +%Y-%m-%d\ %T`
# echo $compile_time

version=`git describe --tag --dirty --always --long --abbrev=16`
# echo $version

str_ver=$compile_time'\\r\\n'$version
echo $str_ver

cat version.h.template | sed "s/VER_REPLACE/\"$str_ver\"/g" > ../../version.h

echo "Generated version.h"
#!/bin/bash
#第一个参数是要编译的文件名，第二个是输出到的文件
source_prefix="../luas/"
output_prefix="../luas/outs/"

source=${source_prefix}$1
output=${output_prefix}$2
echo "Compile " $source "to" $output
./compile $source $output

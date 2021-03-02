#!/bin/bash
echo "hello world"


# ${fname##*/} 根据文件的全目录名得到文件名，即去掉前面的目录 
# ${fname%%.*} 去掉最后一个点后面的内容

for fname in `find . -name "*.lua"`
do 
    fullname=${fname##*/}
    prefix=${fullname%%.*}
    oname=$prefix.out
    #compile fname to out
    luac -o $oname $fullname
    echo "Compile ${fullname} to ${oname} done!"

done
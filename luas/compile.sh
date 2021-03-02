#!/bin/bash
#./compile hello.lua
#编译个lua文件

fullname=$1
prefix=${fullname%%.*}
oname=$prefix.out
    #compile fname to out
luac -o $oname $fullname
#echo "Compile ${fullname} to ${oname} done!"

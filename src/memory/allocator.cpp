//
// Created by gpf on 2020/12/8.
//

#include "allocator.h"
#include <stdlib.h>



size_t Allocator::usedMemory = 0;

/**
 * 分配内存的核心函数，也可以释放内存
 * @param addr 分配的地址
 * @param oldSize 原来的大小
 * @param newSize 新的大小
 * @return 返回新分配的地址
 *
 * 如果newsize=0说明只是单纯的free，直接返回nullptr
 * 如果oldsize=0说明只是malloc，也可以调用relloc
 *
 */
void * Allocator::Realloc(void *addr, size_t oldSize, size_t newSize) {
    //如果addr是nullptr那么oldsize=0
    size_t realOldSize = (addr)?oldSize:0;
    void * res= nullptr;

    if(newSize==0){
        free(addr);
    }else{
         res = realloc(addr, newSize);
    }


    usedMemory += (newSize - realOldSize);
    return res;
}

void * Allocator::Alloc(size_t sz) {
    usedMemory+= sz;
    return realloc(NULL, sz);
}
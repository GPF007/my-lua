//
// Created by gpf on 2020/12/8.
//

#ifndef LUUA2_ALLOCATOR_H
#define LUUA2_ALLOCATOR_H
#include <stddef.h>

class Allocator {
public:
    static void* Realloc(void* addr, size_t oldSize, size_t newSize);
    static void* Alloc(size_t sz);
    static void  Free(void* addr, size_t size){
        Realloc(addr, size, 0);
    }

    //ReallocVector用来重新分配一个数组区域， 从oldsize 到newsize，每个元素的大小为itemsize。
    //如 int[4] => int[8]
    static inline void* ReallocVector(void *addr, int oldsize, int newsize, int itemsize){
        return  Realloc(addr, oldsize* itemsize, newsize* itemsize);

    }

    //分配一个新的数组区域
    static inline void* NewVector(void *addr, int newsize, int itemsize){
        return  Realloc(addr, 0, newsize* itemsize);

    }

    //members 一个用于统计累计使用内存的数
    static size_t usedMemory;

};


#endif //LUUA2_ALLOCATOR_H

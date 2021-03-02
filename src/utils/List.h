//
// Created by gpf on 2020/12/14.
//

#ifndef LUUA2_LIST_H
#define LUUA2_LIST_H

#include "memory/allocator.h"

/**
 * 泛型的list
 */
template<typename T, typename Alloc=Allocator>
class List {
public:

    List(){
        //初始化begin和end指向同一个节点
        begin_ = Alloc::Alloc(sizeof(T));
        end_   = Alloc::Alloc(sizeof(T));

        end_ = begin_;
    }

private:
    T* begin_;
    T* end_;
};


#endif //LUUA2_LIST_H

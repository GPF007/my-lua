//
// Created by gpf on 2020/12/9.
//

#include "stringTable.h"
#include "memory/allocator.h"
#include "arith.h"
#include "Value.h"
#include <string.h>
#include <limits.h>

namespace {
    //size一定是2的倍数
  inline int lmod(int s, int size){
      return (s & (size-1));
  }
};



//扩容
void StringTable::Resize(int newsize) {
    //如果newsize较大那么需要扩容，即重新分配bucketvector
    if(newsize > size_){
        buckets_ = reinterpret_cast<LuaString**>(Allocator::ReallocVector(buckets_, size_, newsize, sizeof(LuaString*)));
        //Allocator::ReallocVector(buckets_, size_, newsize, sizeof(LuaString*));
        //size -> newsize之间的数至空
        for(int i=size_;i<newsize;i++)
            buckets_[i] = nullptr;
    }

    //然后进行rehash操作
    for(int i=0;i<size_;i++){
        LuaString* node = buckets_[i];
        buckets_[i] = nullptr;
        while(node){
            //save next
            auto next = node->shrNext_;
            //auto newidx = node->hash_ % newsize;
            auto newidx = lmod(node->hash_, newsize);
            //每次插入到链表的头部
            node->shrNext_ = buckets_[newidx];
            buckets_[newidx] = node;
            node = next;
        }
    }
    //如果小于需要shrink
    if(newsize < size_){
        Allocator::ReallocVector(buckets_, size_, newsize, sizeof(LuaString*));
    }

    //最后设置新的size
    size_ = newsize;
}

//找到字符串，如果没有返回null
LuaString * StringTable::Get(const char *str, size_t l, unsigned int h) {
    LuaString* list = buckets_[arith::lmod(h, size_)];

    //遍历list
    LuaString* ls;
    for(ls = list;ls ;ls = ls->shrNext_){
        if(l == ls->shrlen_ && memcmp(str, ls->data_, l* sizeof(char)) == 0){
            return ls;
        }
    }
    return nullptr;
}

//添加一个短字符串对象
void StringTable::Add(LuaString* sobj) {
    ASSERT(sobj->type_ == LUA_TSHRSTR);
    //是否需要扩容
    if(nuse_ >= size_ && size_ <= INT_MAX/2){
        Resize(size_ * 2);
    }

    LuaString** list = &buckets_[arith::lmod(sobj->hash_, size_)];

    //添加到相应的位置
    sobj->shrNext_ = *list;
    *list = sobj;
    nuse_++;

}
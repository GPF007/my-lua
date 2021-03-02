//
// Created by gpf on 2020/12/9.
//

#ifndef LUUA2_STRINGTABLE_H
#define LUUA2_STRINGTABLE_H

#include "luaString.h"


//string table用来记录短字符串

class StringTable {
public:
    StringTable():buckets_(nullptr),nuse_(0),size_(0){}
    StringTable(int n);

    void Resize(int newsize);

    void Add(LuaString* sobj);
    LuaString* Get(const char* str, size_t l, unsigned int h);


    //getters
    int size()    const  {return size_;}
    int nuse()    const  {return nuse_;}

    //methods

private:
    LuaString** buckets_;
    int nuse_;
    int size_;


};


#endif //LUUA2_STRINGTABLE_H

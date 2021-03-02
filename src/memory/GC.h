//
// Created by gpf on 2020/12/8.
//

#ifndef LUUA2_GC_H
#define LUUA2_GC_H

#include <stddef.h>
#include "allocator.h"
#include "object/gcObject.h"
class LuaString;
class GCObject;
class LuaTable;

class GC {
public:
    GC();
    ~GC(){}

    LuaString* CreateLuaString(const char* str, size_t l, int tag,unsigned int h);

    GCObject* createObject(int tag, size_t size);

private:

    GCObject* begin_;
    GCObject* cur_;


    inline void appendObject(GCObject* obj){
        cur_->next_ = obj;
        cur_ = obj;
    }

private:
    //gc method
    void mark();
    void markTable(LuaTable* tbl);
    void markState(LuaState* state);
    void sweep();
    void sweepObject(GCObject* obj);

};


#endif //LUUA2_GC_H

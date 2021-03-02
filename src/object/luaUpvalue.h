//
// Created by gpf on 2020/12/16.
//

#ifndef LUUA2_LUAUPVALUE_H
#define LUUA2_LUAUPVALUE_H
#include "gcObject.h"
#include "Value.h"
class LuaUpvalue: public GCObject {
public:

    Value*  val()       const   {return val_;}
    //int     idx()       const   {return idx_;}
    LuaUpvalue* next()  const   {return next_;}
    void close()                {closedValue_ = *val_; val_=&closedValue_;}
    void setVal(const Value& v)       {*val_ = v;}
    void setVal(Value* v)       {val_ = v;}

    void markSelf() {
        makked_ = GC_BLACK;
        val_->markSelf();
    }


    //create upvalue
    static LuaUpvalue*  CreateUpvalue(Value* val, LuaUpvalue* next);
    static LuaUpvalue*  DestroyUpvalue(LuaUpvalue* uv);

private:
    Value* val_;
    union {
        struct {
            LuaUpvalue* next_;
        };
        Value closedValue_; //用作closue的value
    };
};


#endif //LUUA2_LUAUPVALUE_H

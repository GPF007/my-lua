//
// Created by gpf on 2020/12/16.
//

#ifndef LUUA2_CLOSURE_H
#define LUUA2_CLOSURE_H

#include "gcObject.h"
#include "luaUpvalue.h"
#include "utils/Array.h"

class Proto;


class Closure : public GCObject {
protected:
    GCObject* gclist_;
};

class LuaClosure: public Closure{
public:

    //static
    static LuaClosure* CreateLuaClosure(int n, Proto* p);
    static void DestroyLuaClosure(LuaClosure* cl);

    Proto* proto()  const {return proto_;}
    LuaUpvalue* getUpvalue(int idx)     {return upvals_[idx];}
    void setUpvalue(int idx, LuaUpvalue* upval)     {upvals_[idx] = upval;}
    Array<LuaUpvalue*>& upvals()                    {return upvals_;}

    void markSelf();

private:
    Proto* proto_;          //一个proto
    Array<LuaUpvalue*> upvals_;  //对应的upvalue*数组
};

class CClosure: public Closure{
public:
    static CClosure* CreateCClosure(Cfunction f);
    static void DestroyCClosure(CClosure* c);

    Cfunction func()    const  {return func_;}

    void markSelf()     {makked_ = GC_BLACK;}

private:
    Cfunction func_;
};


#endif //LUUA2_CLOSURE_H

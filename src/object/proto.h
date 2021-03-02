//
// Created by gpf on 2020/12/14.
//

#ifndef LUUA2_PROTO_H
#define LUUA2_PROTO_H

#include "gcObject.h"
#include "Value.h"
#include "utils/Array.h"
#include <string>
class Proto: public GCObject {
    friend class GC;
    friend class Undumper;
    friend class LuaState;

public:
    struct LocVar {
        LuaString *varname;
        int startpc;  /* first point where variable is active */
        int endpc;    /* first point where variable is dead */
    };

    struct Upvaldesc {
        LuaString *name;  /* upvalue name (for debug information) */
        LuaByte instack;  /* whether it is in stack (register) */
        LuaByte idx;  /* index of upvalue (in stack or in outer function's list) */
    };


public:

    //static methods
    static Proto* CreateProto();
    static void DestroyProto(Proto* p);
    std::string toString() const;

    //getters
    const Array<Value>& consts()     const    {return consts_;}
    int   nparams()                 const      {return nparams_;}
    Proto* subProtoAt(int idx)      const       {return subs_[idx];}
    int nsubs()                     const       {return subs_.len();}
    int nupval()                    const       {return upvalDescs_.len();}
    Upvaldesc& upvalDescAt(int idx) const       {return upvalDescs_[idx];}
    int maxStackSize()              const       {return maxStackSize_;}
    Instruction *  codes()          const       {return codes_.data();}
    int nlocal()                    const       {return localVars_.len();}
    LocVar * localVarAt(int idx)    const       {return &localVars_[idx];}

    //setters
    void addCode(int pc, Instruction i);
    void addLineInfo(int pc, Instruction i);
    int addConst(Value* val);
    int addLocalVar(LuaString* varname);
    //addupvalue会在upvalue列表中新建一个upvalue，然后返回该upvalue的引用
    Upvaldesc& addUpvalue() {   return upvalDescs_.push_back();}
    void addProto(Proto* p) {subs_.push_back(p);}
    void setVararg()        {isVararg_ = 1;}
    void setNparam(int np)  {nparams_ = np;}
    void setMaxStack(int n) {maxStackSize_ = n;}

    void markSelf();


    //debug method
    void printConstant(int i);
    void printCodes();
    void printCode(int idx,Instruction ins);


private:
    LuaByte nparams_;    //参数的数量
    LuaByte isVararg_;   //是否是vararg
    LuaByte maxStackSize_;   //栈最大值
    int     lastlinedefined_;
    int     linedefined_;
    LuaString* source_;

    //array PART
    Array<Value>            consts_;  //常量数组
    Array<Instruction>      codes_; //指令数组
    Array<int>              lineInfo_;      //行信息
    Array<Proto*>           subs_;          //subprotos
    Array<LocVar>           localVars_;     //局部变量列表
    Array<Upvaldesc>        upvalDescs_;    //upvalue描述


    GCObject* gclist_;

private:
    const char* upvalueNameAt(int i) const ;
    std::string constant2String(int i) const ;
    std::string code2string(int idx, Instruction ins) const ;
    std::string codes2string() const ;
    std::string header2string() const ;

};


#endif //LUUA2_PROTO_H

//
// Created by gpf on 2020/12/15.
//

#ifndef LUUA2_LUASTATE_H
#define LUUA2_LUASTATE_H
#include "object/Value.h"
#include "utils/Vector.h"
#include "object/luaString.h"

#include <fmt/core.h>

/* thread status */
#define LUA_OK		0
#define LUA_YIELD	1
#define LUA_ERRRUN	2
#define LUA_ERRSYNTAX	3
#define LUA_ERRMEM	4
#define LUA_ERRGCMM	5
#define LUA_ERRERR	6

/*
** Bits in CallInfo status
*/
#define CIST_OAH	(1<<0)	/* original value of 'allowhook' */
#define CIST_LUA	(1<<1)	/* call is running a Lua function */
#define CIST_HOOKED	(1<<2)	/* call is running a debug hook */
#define CIST_FRESH	(1<<3)	/* call is running on a fresh invocation
                                   of luaV_execute */
#define CIST_YPCALL	(1<<4)	/* call is a yieldable protected call */
#define CIST_TAIL	(1<<5)	/* call was tail called */
#define CIST_HOOKYIELD	(1<<6)	/* last hook called yielded */
#define CIST_LEQ	(1<<7)  /* using __lt for __le */
#define CIST_FIN	(1<<8)  /* call is running a finalizer */


class VM;


class Proto;
class LuaUpvalue;

struct CallInfo{

    static CallInfo* CreateCallInfo();
    int func=0;  //funcidx
    int top=0;   //top idx
    int base=0;
    CallInfo* previous= nullptr;
    CallInfo* next= nullptr;
    short nresult=0;
    unsigned short status=0;
    const Instruction * savedpc;

    //methods
    Instruction fetch()             {return *savedpc++;}
    void incpc()                    {++savedpc;}
    bool isLua()                    {return status & CIST_LUA;}
};


class LuaState : public GCObject {
    friend class VM;
public:

    //static method
    static LuaState* CreateLuaState();
    static void DestroyLuaState(LuaState* l);

    //method
    CallInfo* ExtendCallInfo();
    CallInfo* nextCallInfo();

    void growStack(int n);
    void reallocStack(int n);
    void checkStack(int n){
        if(lastFree_ - top_ <= n)
            growStack(n);
    }

    //debug method
    void errormsg();
    template<typename... Args>
    void runerror(Args... args){
        std::string str = fmt::format(args...);
        //push 一个string到栈中
        auto strobj = LuaString::CreateString(str.data(), str.size());
        stack_[top_++] = Value(strobj, LUA_TSTRING);
        //最后抛出异常
        errormsg();
    }
    void typeerror(const Value& o, const char* op);
    void Throw(int errcode) {throw errcode;}


    //call series method
    int precall(int func, int nresult);
    int poscall(CallInfo* ci, int firstResult, int nres);
    void call(int func, int nresult);
    int pcall(int func, int nresult);
    int adjustVararg(Proto* proto, int actual);


    //push and pop
    void push(const Value& val)         {stack_[top_++] = val;}
    void pushnil()                      {stack_[top_++] = Value();}

    //getter and setter
    Value& at(int idx)                  {return stack_[idx];}
    Value& operator[](int idx)          {return stack_[idx];}
    int top()           const           {return top_;}
    Value& pop()                        {return stack_[--top_];}
    CallInfo* ci()                      {return ci_;}


    //upvalues
    LuaUpvalue* findUpval(Value* val);
    void closeUpvalues(Value* level);

    //states apis
    bool next(int idx);

    //mark method
    void markSelf();

private:
    Value* stack_;//stack array
    int top_;
    int stackSize_;
    int base_;
    int lastFree_;
    int errfunc_; //当前错误处理函数的idx
    CallInfo* ci_;
    CallInfo* baseCi_;

    //open upvalues
    LuaUpvalue* openlist_;

    VM* vm_;

    //private methods
private:
    void initStack();
    void freeStack();
    int moveResults(int from, int to, int nres, int nwanted);

};


#endif //LUUA2_LUASTATE_H

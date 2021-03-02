//
// Created by gpf on 2020/12/20.
//

#include "basic.h"
#include "vm/globalState.h"
#include "vm/meta.h"
#include "object/closure.h"
#include "object/luaTable.h"

//internal functions
namespace {
    int ipairsAux(LuaState* s){
        auto base = s->ci()->base;
        LuaInteger i = s->at(base + 2).convertToInteger().ival +1 ;
        s->push(i);
        auto tbl = static_cast<LuaTable*>(s->at(base + 1).obj);
        auto val = tbl->getInt(i);
        s->push(*val);
        return s->at(s->top()-1).isNil()?1:2;
    }
};


namespace basicLibrary{
    /**
     * print 函数输出state base 到top之间的所有value的字符串
     * @param state
     * @return
     */
    int print(LuaState* state){
        int nargs = state->top() - (state->ci()->func+1);
        auto base = state->ci()->base;
        for(int i=1;i<=nargs;i++){
            printf("%s", state->at(base+i).toString().data());
            if(i<nargs)
                printf("\t");
        }
        printf("\n");
        return 0;
    }

    //讲idx为1,即第一个参数的metatble取出来,然后推入栈顶
    int getmetatable(LuaState* state){
        if(GlobalState::meta->getmetatable(1))
            state->pushnil();
        return 1;
    }

    int setmetatable(LuaState* state){
        GlobalState::meta->setmetatable(1);
        return 1;
    }


    int next(LuaState* state){
        auto base = state->ci()->base;
        //第一个参数必须是table
        ASSERT(state->at(base + 1).isTable());
        if(state->next(base + 1))
            return 2;
        else{
            state->pushnil();
            return 1;
        }
    }

    //传入一个表，生成一个next函数，然后是表，最后是nil值作为迭代的第一个键
    //push到栈中
    int pairs(LuaState* state){
        auto base = state->ci()->base;
        auto cclosure = CClosure::CreateCClosure(next);
        state->push(Value(cclosure, LUA_TCCL));
        state->push(state->at(base+1));
        state->pushnil();
        return 3;
    }

    //和pairs类似，只不过返回的next函数是另一个
    int ipairs(LuaState* state){
        auto base = state->ci()->base;
        auto cclosure = CClosure::CreateCClosure(ipairsAux);
        state->push(Value(cclosure, LUA_TCCL));
        state->push(state->at(base+1));
        state->push(Value(0));

        return 3;
    }




};
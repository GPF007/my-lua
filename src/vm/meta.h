//
// Created by gpf on 2020/12/17.
//

#ifndef LUUA2_META_H
#define LUUA2_META_H

#include "luaState.h"
#include "object/luaString.h"


class LuaTable;

class Meta {
public:
    enum TMS{
        TM_INDEX,
        TM_NEWINDEX,
        TM_GC,
        TM_MODE,
        TM_LEN,
        TM_EQ,  /* last tag method with fast access */
        TM_ADD,
        TM_SUB,
        TM_MUL,
        TM_MOD,
        TM_POW,
        TM_DIV,
        TM_IDIV,
        TM_BAND,
        TM_BOR,
        TM_BXOR,
        TM_SHL,
        TM_SHR,
        TM_UNM,
        TM_BNOT,
        TM_LT,
        TM_LE,
        TM_CONCAT,
        TM_CALL,
        TM_N		/* number of elements in the enum */
    };

public:
    void Init();
    void SetState(LuaState* s)          {state_ = s;}

    //call 方法
    void tryCallBinary(const Value& a, const Value& b, int idres, TMS event);
    bool callBinaryMetaMethod(const Value& a, const Value& b, int idres, TMS event);
    bool callOrderMetaMethod(const Value& a, const Value& b, TMS event);
    void callMetaMethod(const Value& f, const Value& a, const Value& b, int idxres, int hasres);
    const Value* getMtFromTable(LuaTable* tbl, TMS event);
    const Value * getMtFromObj(const Value& obj, TMS event);
    bool getmetatable(int idx);
    bool setmetatable(int idx);


private:
    LuaState* state_;
    LuaString* mtnames_[TM_N];

private:
    //method

};


#endif //LUUA2_META_H

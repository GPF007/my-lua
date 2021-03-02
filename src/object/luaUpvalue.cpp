//
// Created by gpf on 2020/12/16.
//

#include "luaUpvalue.h"
#include "vm/globalState.h"
/**
 * 创建一个upvalue
 * @param idx 指向的值所在的栈索引
 * @param next next链表
 * @param val value指针
 * @return 新创建的upvalue
 */
LuaUpvalue * LuaUpvalue::CreateUpvalue(Value* val, LuaUpvalue* next) {
    auto obj = GlobalState::gc->createObject(LUA_TUPVAL, sizeof(LuaUpvalue));
    auto upval = static_cast<LuaUpvalue*>(obj);
    upval->next_ = next;
    upval->val_  = val;

    return upval;
}

LuaUpvalue * LuaUpvalue::DestroyUpvalue(LuaUpvalue* uv) {
    //return next
    auto ret = uv->next_;
    Allocator::Free(uv, sizeof(LuaUpvalue));
    return ret;
}
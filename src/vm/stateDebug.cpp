//
// Created by gpf on 2020/12/16.
//
#include "luaState.h"
#include "object/luaString.h"
#include <fmt/core.h>

/**
 * 抛出一个异常
 */
void LuaState::errormsg() {
    //todo 检查是否有errfunc
    Throw(LUA_ERRRUN);
}

/*
template<typename... Args>
void LuaState::runerror(Args... args){
    std::string str = fmt::format(args...);
    //push 一个string到栈中
    auto strobj = LuaString::CreateString(str.data(), str.size());
    stack_[top_++] = Value(strobj, LUA_TSTRING);
    //最后抛出异常
    errormsg();
}
 */

//
// Created by gpf on 2020/12/20.
//

#ifndef LUUA2_BASIC_H
#define LUUA2_BASIC_H

#include "vm/luaState.h"

namespace basicLibrary{
    //一些基础的库
int print(LuaState* state);
int getmetatable(LuaState* state);
int setmetatable(LuaState* state);

int next(LuaState* state);
int pairs(LuaState* state);
int ipairs(LuaState* state);
};


#endif //LUUA2_BASIC_H

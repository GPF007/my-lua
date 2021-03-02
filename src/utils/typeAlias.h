//
// Created by gpf on 2020/12/8.
//

#ifndef LUUA2_TYPEALIAS_H
#define LUUA2_TYPEALIAS_H
#include <stddef.h>
#include <assert.h>

class LuaState;

using LuaByte = unsigned char;
typedef long long LuaInteger;
typedef double  LuaNumber;
typedef unsigned int Instruction;

//cfunction 的定义
using Cfunction = int(*)(LuaState* s);

//const value
const int kMaxShortLEN = 40;

const LuaByte  kLuacVersion  = 0x53;
const LuaByte  kLuaFormat   = 0;

//size value
const LuaByte  CINT_SIZE = 4;
const LuaByte  CSIZET_SIZE = 8;
const LuaByte  INSTRUCTION_SIZE = 4;
const LuaByte  LUA_INTEGER_SIZE = 8;
const LuaByte  LUA_NUMBER_SIZE = 8;//float number
const LuaInteger LUAC_INT = 0x5678;
const LuaNumber LUAC_NUM = 370.5;


#define ASSERT(x) assert(x)

#endif //LUUA2_TYPEALIAS_H

//
// Created by gpf on 2020/12/8.
//

#include "Value.h"
#include "luaString.h"
#include "luaTable.h"
#include "proto.h"
#include "closure.h"
#include "vm/globalState.h"
#include "vm/luaState.h"
#include <fmt/core.h>

//返回当前value的hash值，根据不同的类型讨论
unsigned int Value::HashValue() const {
    switch (type) {
        case LUA_TNUMINT:   return ival;
        case LUA_TNUMFLT:   return nval;
        case LUA_TSHRSTR:
        case LUA_TLNGSTR:   return static_cast<LuaString*>(obj)->hash();
        case LUA_TBOOLEAN:  return bval;
        default:{ //默认返回指针
            return ival;
        }
    }
}

bool Value::equalto(const Value *other) const{
    return type == other->type && ival == other->ival;
}
std::string Value::toString() const {
    std::string ret;
    switch (type) {
        case LUA_TNIL:
            return "nil";
        case LUA_TNUMINT:
            return fmt::format("{}",ival);
        case LUA_TNUMFLT:
            return fmt::format("{}",nval);
        case LUA_TSHRSTR:case LUA_TLNGSTR:
            return std::string(static_cast<LuaString*>(obj)->data());
        case LUA_TBOOLEAN:
            return bval?"true": "false";
        case LUA_TLCL:
            return "LUA_CLOSURE";
        default:
            return "unknown type";
    }

}

/*
 * 返回一个新的value，尝试转换为numflt类型，根据返回value的type确定是否转换成功了
 */
Value Value::convertToNumber() const {

    switch (type) {
        case LUA_TNUMFLT:   return Value(nval);
        case LUA_TNUMINT:   return Value(static_cast<LuaNumber>(ival));
        case LUA_TSTRING:   {
            //尝试转换str 2 num
            auto strobj = static_cast<LuaString*>(obj);
            auto val = strobj->convertToNumber();
            return val;
        }
        default:
            return *kNil;
    }
}

/**
 * 尝试讲当前值转化为整数，失败返回nil
 * @return
 */
Value Value::convertToInteger() const {

    switch (type) {
        case LUA_TNUMINT:   return Value(ival);
        case LUA_TNUMFLT:   return Value(static_cast<LuaInteger>(nval));
        case LUA_TSTRING:   {
            //尝试转换str 2 num
            auto strobj = static_cast<LuaString*>(obj);
            auto val = strobj->convertToInteger();
            return val;
        }
        default:
            return *kNil;
    }
}


Value Value::convertToString() const {
    std::string tmp;
    switch (type) {
        case LUA_TNUMINT:   tmp = fmt::format("{}",ival); break;
        case LUA_TNUMFLT:   tmp = fmt::format("{}",nval); break;
        case LUA_TLNGSTR: case LUA_TSHRSTR:
            {
            return *this;//返回一个copy
        }
    }
    if(tmp.empty())
        return *kNil;
    auto str = LuaString::CreateString(tmp.data(), tmp.size());
    return Value(static_cast<GCObject*>(str), str->type());
}


void Value::markSelf() {
    switch (type) {
        case LUA_TLNGSTR: case LUA_TSHRSTR:
            static_cast<LuaString*>(obj)->markSelf(); break;
        case LUA_TTABLE:
            static_cast<LuaTable*>(obj)->markSelf(); break;
        case LUA_TTHREAD:
            static_cast<LuaState*>(obj)->markSelf(); break;
        case LUA_TPROTO:
            static_cast<Proto*>(obj)->markSelf(); break;
        case LUA_TLCL:
            static_cast<LuaClosure*>(obj)->markSelf(); break;
        case LUA_TCCL:
            static_cast<CClosure*>(obj)->markSelf(); break;
        case LUA_TUPVAL:
            static_cast<LuaUpvalue*>(obj)->markSelf(); break;

        default:{
            throw std::runtime_error("Unknown type for gc mark stage!");
        }


    }
}
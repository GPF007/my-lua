//
// Created by gpf on 2020/12/8.
//

#ifndef LUUA2_VALUE_H
#define LUUA2_VALUE_H
#include "utils/typeAlias.h"
#include "gcObject.h"
#include <string>
class LuaString;

#define LUA_TPROTO	LUA_NUMTAGS
enum LUA_TYPE{
    LUA_TNONE = -1,
    LUA_TNIL =0,
    LUA_TBOOLEAN,
    LUA_TLIGHTUSERDATA,
    LUA_TNUMBER,
    LUA_TSTRING,
    LUA_TTABLE,
    LUA_TFUNCTION,
    LUA_TUSERDATA,
    LUA_TTHREAD,
    LUA_NUMTAGS,
    LUA_TUPVAL,

    //扩展类型
    LUA_TLCL = (LUA_TFUNCTION | (0<<4)), // LUA CLOSURE
    //LUA_TLCF = (LUA_TFUNCTION | (1<<4)),  // light c function
    LUA_TCCL = (LUA_TFUNCTION | (2<<4)),  // c closure

    LUA_TNUMFLT = (LUA_TNUMBER | (0<<4)),
    LUA_TNUMINT = (LUA_TNUMBER | (1<<4)),

    LUA_TSHRSTR =	(LUA_TSTRING | (0 << 4)),  /* short strings */
    LUA_TLNGSTR	=   (LUA_TSTRING | (1 << 4)),  /* long strings */

    LUA_TOPENUPVAL = (LUA_TUPVAL | (0<<4)),
    LUA_TCLOSEUPVAL = (LUA_TUPVAL | (0<<4)),

};

enum GC_COLOR{
    GC_WHITE,
    GC_BLACK,
};



//一些方便转化的宏
#define totable(val)    static_cast<LuaTable*>(val->obj)
#define tostring(val)   static_cast<LuaString*>(val->obj)


class Value {
public:
    Value():type(LUA_TNIL){}
    Value(LuaInteger i): ival(i), type(LUA_TNUMINT){}
    Value(LuaNumber i): nval(i), type(LUA_TNUMFLT){}
    Value(LuaByte i): bval(i), type(LUA_TBOOLEAN){}
    Value(int i): ival(i), type(LUA_TNUMINT){}
    Value(GCObject* o, int tt): obj(o), type(tt){}

    //methods
    //basic type指的是type的后三位
    int basicType()  const      {return type & 0x0F;}
    bool isShortString() const  {return type == LUA_TSHRSTR;}
    bool isLongString() const   {return type == LUA_TLNGSTR;}
    bool isInteger() const      {return type == LUA_TNUMINT;}
    bool isTable() const        {return type == LUA_TTABLE;}
    bool isNil() const          {return type == LUA_TNIL;}
    bool isFloat() const        {return type == LUA_TNUMFLT;}
    LuaByte isFalse()    const  {return type == LUA_TNIL || (type ==LUA_TBOOLEAN && bval==0);}
    bool isNumber()  const      {return basicType() == LUA_TNUMBER;}
    bool isString()  const      {return basicType() == LUA_TSTRING;}
    bool isFunction()  const {return basicType() == LUA_TFUNCTION;};
    bool needGC()   const       {return type > LUA_TNUMBER;}
    unsigned int HashValue() const;
    std::string toString() const;

    void setnil()               {type = LUA_TNIL;}
    void setInt(LuaInteger i)   {type = LUA_TNUMINT; ival = i;}


    //tmp method
    bool equalto(const Value* other) const ;
    friend bool operator ==(const Value& left, const Value& right){
        return left.equalto(&right);
    }



    //convert method
    Value convertToNumber() const;
    Value convertToInteger() const ;
    Value convertToString() const;

    //for gc method
    void markSelf();

public:


    //members
    union{
        GCObject* obj;
        LuaByte bval;
        LuaInteger ival;
        LuaNumber nval;
    };
    int type;
};


#endif //LUUA2_VALUE_H

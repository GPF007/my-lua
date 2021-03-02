//
// Created by gpf on 2020/12/8.
//

#include "luaString.h"
#include "vm/globalState.h"
#include "stringTable.h"
#include "arith.h"
#include "Value.h"
#include <limits.h>
#include <string.h>

#define SEED 5
#define LUAI_MAXSHORTLEN 40

//内部的函数
namespace {

};


//计算一个字符串的hash值然后返回 这个函数来源于lua源码
unsigned int LuaString::hash(const char *str, size_t len, unsigned int seed) {
    auto h = seed ^static_cast<unsigned int>(len);
    auto step = (len >>5) + 1;
    while (len>=step){
        h ^= ((h<<5) + (h>>2)) + static_cast<LuaByte>(str[len-1]);
        len -= step;
    }
    return h;
}

/**
 * 用来计算一个luastring的hash值，只用于longstr
 */
unsigned int LuaString::hash(LuaString *lstr) {
    ASSERT(lstr->type_ == LUA_TLNGSTR);
    if(lstr->extra_ == 0){//没有hash值计算一个
        lstr->hash_ = hash(lstr->data_, lstr->lnglen_, lstr->hash_);
        lstr->extra_ = 1;
    }
    return lstr->hash_;
}

/**
 * 比较两个长字符串是否相同
 * 先比较是否是同一个对象，然后比较长度和每个字符
 */
bool LuaString::equalLongString(LuaString *l, LuaString *r) {
    auto len = l->lnglen_;
    ASSERT(l->type_ == LUA_TLNGSTR && r->type_ == LUA_TLNGSTR);
    return (l==r) || ((len == r->lnglen_) && memcmp(l->data_, r->data_, len)==0);
}


//创建一个LuaString对象，一共调用了两次malloc一次是对象本身，一次是data数组
LuaString * LuaString::createStringObject(size_t l, int tag, unsigned int h) {
    auto gc = GlobalState::gc;
    auto obj = gc->createObject(tag, sizeof(LuaString));
    LuaString* strobj = static_cast<LuaString*>(obj);
    strobj->hash_ = h;
    strobj->extra_ = 0;
    strobj->type_ = tag;
    strobj->data_ = reinterpret_cast<char*>(Allocator::Alloc(sizeof(char) * (l+1)));
    strobj->data_[l]='\0';

    return strobj;
}

//创建一个长字符串对象
LuaString * LuaString::CreateLongString(const char *str, size_t l) {
    auto strobj = createStringObject(l, LUA_TLNGSTR, SEED);
    strobj->lnglen_ = l;
    memcpy(strobj->data_, str, l*sizeof(char));
    //计算一次hash值
    strobj->hash_ = hash(strobj->data_, l, strobj->hash_);
    return strobj;
}

//创建一个短字符串对象
LuaString * LuaString::createShortString(const char *str, size_t l) {
    auto h = hash(str, l, SEED);
    auto& stringTable = GlobalState::stringTable;
    auto ret = stringTable.Get(str,l, h);
    if(ret)
        return ret;
    //此时不存在需要创建一个
    auto sobj = createStringObject(l, LUA_TSHRSTR, h);
    sobj->shrlen_ = l;
    memcpy(sobj->data_, str, l*sizeof(char));
    stringTable.Add(sobj);
    return sobj;

}

/**创建一个字符串对象，根据长度创建
 *
 * @param str
 * @param l
 * @return 一个字符串对象
 * 对于短字符串的创建需要计算hash值，然后根据hash值讲字符串对象放到stringtable中
 * 对于长字符串，不需要计算hash值，hash字段赋值为SEED，即一个固定的值，不需要放在字符串表中
 */
LuaString * LuaString::CreateString(const char *str, size_t l) {
    if(l <= LUAI_MAXSHORTLEN)
        return createShortString(str, l);
    else
        return CreateLongString(str, l);
}

Value LuaString::convertToNumber() const {
    //首先尝试转换为整数 LuaInteger 是 long long
    //LuaInteger ival = atoll(data_);
    //if(ival)
    //   return Value(ival);
    //然后尝试转换为浮点数
    LuaNumber nval = atof(data_);
    if(nval)
        return Value(nval);
    return Value();
}

Value LuaString::convertToInteger() const {
    //首先尝试转换为整数 LuaInteger 是 long long
    auto ival = atoll(data_);
    if(ival)
        return Value(ival);
    return Value();
}

//compare method
bool operator <(const LuaString& l, const LuaString& r){
    return strcmp(l.data_, r.data_) <0;
}

bool operator <=(const LuaString& l, const LuaString& r){
    return strcmp(l.data_, r.data_) <=0;
}


void LuaString::DestroyLongString(LuaString *s) {
    Allocator::Free(s->data_, sizeof(char)*(s->lnglen_+1));
    Allocator::Free(s,sizeof(LuaString));
}


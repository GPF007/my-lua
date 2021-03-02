//
// Created by gpf on 2020/12/9.
//

#include "globalState.h"
#include "object/stringTable.h"
#include "object/Value.h"
#include "object/luaTable.h"
#include "object/closure.h"
#include "lib/basic.h"
#include "vm/luaState.h"
#include "vm/meta.h"
#include "compile/token.h"

#define LUA_RIDX_MAINTHREAD	1
#define LUA_RIDX_GLOBALS	2
#define LUA_RIDX_LAST		LUA_RIDX_GLOBALS

StringTable GlobalState::stringTable;
GC*  GlobalState::gc = nullptr;
int GlobalState::seed = 5;
Value GlobalState::knil;
Array<LuaByte> GlobalState::sig;
Array<LuaByte> GlobalState::LUAC_DATA;
Meta* GlobalState::meta = nullptr;
Value GlobalState::registry;
LuaTable* GlobalState::globalTable = nullptr;
LuaState* GlobalState::mainState = nullptr;

void GlobalState::InitAll() {
    gc = new GC;
    stringTable.Resize(4);
    auto nilobj = gc->createObject(LUA_TNIL, sizeof(GCObject));
    knil.obj = nilobj;

    //Array<LuaByte> sig{0x1b,0x4c,0x75,0x61};
    sig =  Array<LuaByte>(4);
    sig[0] = 0x1b;
    sig[1] = 0x4c;
    sig[2] = 0x75;
    sig[3] = 0x61;
    LUAC_DATA=  Array<LuaByte>(6);
    LUAC_DATA[0] = 0x19;
    LUAC_DATA[1] = 0x93;
    LUAC_DATA[2] = 0x0d;
    LUAC_DATA[3] = 0x0a;
    LUAC_DATA[4] = 0x1a;
    LUAC_DATA[5] = 0x0a;

    meta = (Meta*)Allocator::Alloc(sizeof(Meta));
    meta->Init();

    initRegistry();

    //init fcunctions一定在initregistry的后面啊
    initCFunctions();

    Token::InitToken();


}

void GlobalState::DestroyAll() {

}

/**
 * 这个函数用来初始注册表
 * 注册表是一个table有两个数组元素
 * 1  -> mathread（即state本身）
 * 2  -> new table (作为一个全局表) （ENV_）
 * @param state
 */
void GlobalState::initRegistry() {

    //初始化registry
    //数组部分大小为2

    auto regtable = LuaTable::CreateTable();
    regtable->resize(LUA_RIDX_LAST, 0);
    globalTable = LuaTable::CreateTable();
    //globalTable->resize(0, 16);
    auto globalTableValue = Value(static_cast<GCObject*>(globalTable),LUA_TTABLE);
    regtable->setInt(LUA_RIDX_GLOBALS, &globalTableValue);
    //最后初始化registry
    registry = Value(static_cast<GCObject*>(regtable), LUA_TTABLE);

}

void GlobalState::initCFunctions() {
    /*
    auto strobj = LuaString::CreateString("print");
    auto key = Value(strobj, LUA_TSHRSTR);
    auto fobj = CClosure::CreateCClosure(basicLibrary::print);
    auto val = Value(fobj, LUA_TLCF);
    globalTable->Set(key,val);
     */

    Value key,val;
#define ADD_FUNCTION(str, f)    \
    key = Value(LuaString::CreateString(#str), LUA_TSHRSTR);                      \
    val = Value(CClosure::CreateCClosure(f), LUA_TCCL);                           \
    globalTable->Set(key,val);



    ADD_FUNCTION(print, basicLibrary::print)
    ADD_FUNCTION(setmetatable, basicLibrary::setmetatable)
    ADD_FUNCTION(getmetatable, basicLibrary::getmetatable)
    ADD_FUNCTION(next, basicLibrary::next)
    ADD_FUNCTION(pairs, basicLibrary::pairs)
    ADD_FUNCTION(ipairs, basicLibrary::ipairs)



#undef ADD_FUNCTION

}

void GlobalState::SetMainthread(LuaState *state) {
    auto mainThread = Value(static_cast<GCObject*>(state), LUA_TTHREAD);
    mainState = state;
    auto regtable = static_cast<LuaTable*>(registry.obj);
    regtable->setInt(LUA_RIDX_MAINTHREAD, &mainThread);
}
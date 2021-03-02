//
// Created by gpf on 2020/12/9.
//

#ifndef LUUA2_GLOBALSTATE_H
#define LUUA2_GLOBALSTATE_H

#include "memory/GC.h"
#include "object/Value.h"
#include "utils/Array.h"


#define kNil &GlobalState::knil

class StringTable;
class LuaState;
class Meta;
class LuaTable;

class GlobalState {
public:

    //methods
    static void InitAll();
    static void DestroyAll();
    static void SetMainthread(LuaState* state);



    //members
    static StringTable stringTable;
    static GC* gc;
    static int seed;
    static Value knil;
    static Array<LuaByte> sig;
    static Array<LuaByte> LUAC_DATA;
    static Meta* meta;
    static Value registry;
    static LuaTable* globalTable;
    static LuaState* mainState;

private:
    static void initRegistry();
    static void initCFunctions();

};


#endif //LUUA2_GLOBALSTATE_H

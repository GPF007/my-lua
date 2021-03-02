//
// Created by gpf on 2021/1/11.
//

#ifndef LUUA2_FUNCSTATE_H
#define LUUA2_FUNCSTATE_H

#include "utils/typeAlias.h"
#include "object/luaString.h"
#include <vector>

/**
 * Funcstate是编译时候的中间变量
 */
#include "object/proto.h"
class LuaTable;


struct Vardesc{
    short idx;
};

//进入block需要改变的
struct BlockCnt{
    struct BlockCnt* previous;
    LuaByte nactvar;
    LuaByte upval;
    LuaByte isLoop;
};

class FuncState {
    friend class Compiler;
public:
    enum VarType{
        kLocal,
        kGloabl,
        kUpval,
    };
    using VarPair = std::pair<VarType, int>;

    FuncState(FuncState* prev, Proto* p, BlockCnt* bl);

    void fixJump(int pc, int offset);
    void fixSbx(int pc, int offset);
    void fixCallReturn(int pc, int nresult);
    void fixVarargReturn(int pc);

private:
    Proto* proto_;
    FuncState* prev_;
    int firstLocal_; //第一个local变量在activeVars中的索引
    int pc_;
    int nconst_; //number of constants
    int nsub_; //number of subprotos
    short nlocalvar_;
    LuaByte nups_;
    LuaByte freereg_;
    LuaByte nactvar_; //活跃变量的数量
    BlockCnt* blcnt_;
    LuaTable* ctable_;// Value -> intValue


    //活跃的局部变量, 这个是全局的
    static std::vector<Vardesc> activeVars_;

    //private method
private:
    void createLocalVar(LuaString* name);
    void createLocalVarLiteral(const char* str);
    void enterBlock(BlockCnt* bl);
    void removeVars(int tolevel);
    void leaveBlock();
    Proto::LocVar* getlocalvar(int i);
    VarPair singleVar(LuaString* name);
    int searchUpvalue(LuaString* name);
    int newUpvalue(LuaString* name, VarType vt, int idx);
    static VarPair findVar(FuncState* fs, LuaString* name);
    void adjustLocalVars(int nvars);
    void allocRegs(int n);
    void checkStack(int n);
    void freeRegs(int n)    {freereg_-=n;}

    Proto* addSubProto();



};


#endif //LUUA2_FUNCSTATE_H

//
// Created by gpf on 2021/1/11.
//

#ifndef LUUA2_COMPILER_H
#define LUUA2_COMPILER_H
#include "funcState.h"
#include "vm/opcodes.h"
#include "object/Value.h"
#include "object/luaTable.h"
#include "ast.h"

#include <unordered_map>
class Compiler {
    enum {
        kConst,
        kReg,
        kNewReg,
    };

public:
    Compiler();

    Proto* compile();
    Proto* compile(const char* fname);
    Block* block()      {return block_.get();}

private:
    FuncState* fs_;
    //LuaTable* ctable_;// Value -> intValue
    LuaString* env_;
    BlockP block_;

//private methods
private:
    struct ExprIdx{
        int idx; //该寄存器的索引, 可以是常数索引
        int kind; //索引类型，　常量索引、寄存索引、或者需要释放的寄存器
    };
    void freeExp(ExprIdx& e){
        if(e.kind == kNewReg)
            fs_->freereg_--;
    }

    //some code method
    int code(Instruction i);
    int codeABC(ByteCode::OpCode op, int a,int b,int c);
    int codeABx(ByteCode::OpCode op, int a, unsigned int bx);
    int codeAsBx(ByteCode::OpCode op, int a, unsigned int bx){
        return codeABx(op, a, bx+ ByteCode::MAXARG_sBx);
    }
    int codeRet(int start,int nret);
    int codek(int reg, int k);
    int codeJmp(int a, int offset);
    void codeSetList(int base ,int nelems, int tostore);
    int codeLoadBool(int a,int b,int c);
    //add constant method
    int addConst(Value* value);
    int addStringConst(LuaString* s);
    int addIntConst(LuaInteger ival);
    int addFloatConst(LuaNumber nval);
    int addBoolConst(bool bval);
    int addNilConst();



    //针对具体的ast
    void genExprToNextReg(Expr* expr);
    int getConstIdx(Expr* expr);

    //compile expressionv
    void genExprToReg(Expr* expr, int target);
    ExprIdx genExprToAnyReg(Expr* expr);
    void genBinaryExpr(BinaryOpExpr* expr, int target);
    void genUnaryExpr(UnaryOpExpr* expr, int target);
    void genTableConstructorExpr(TableConstructorExpr* expr, int target);
    void genFuncCallExpr(FunCallExpr* expr, int target);

    ExprIdx getVarReg(Var* var, int target);
    void genOtherExpr(Expr* expr, int target);




    //编译 statement
    void genStatement(Statement* stat);
    void genLocalNameStat(LocalNameStat* node);
    void genLocalFuncStat(LocalFuncStat* stat);
    void genWhileStat(WhileStat* stat);
    void genDoStat(DoStat* stat);
    void genIfStat(IfStat* stat);
    void genForEqStat(ForEqStat* stat);
    void genBlock(Block* block);
    void genFuncDefStat(FuncDefStat* stat);
    void genAssignStat(AssignStat* stat);
    void genRetStat(RetStat* stat);
    void genFuncCallStat(FunCallStat* stat);

    //其他的node
    void genFuncBody(FuncBody* fbody, int reg, bool isMethod = false);


    //fix
    void fixReturns(Expr* expr,int nresult);

    //storevar
    void storeVar(Var* var, Expr *exp, int src=-1);

};


#endif //LUUA2_COMPILER_H

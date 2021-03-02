//
// Created by gpf on 2020/12/29.
//

#ifndef LUUA2_AST_H
#define LUUA2_AST_H

#include <vector>
#include <memory>
#include "object/luaString.h"

#define NODE :public Node
#define STAT :public Statement

using std::vector;
using std::unique_ptr;

class Statement;
using StatP = unique_ptr<Statement>;
class Attr;
class Var;
class Expr;
using ExprP = unique_ptr<Expr>;
class PrefixExpr;
class ExprList;
class Args;
using ArgsP = unique_ptr<Args>;
class Block;
using BlockP = unique_ptr<Block>;
class Field;
using FieldP = unique_ptr<Field>;

enum NODE_TYPE{
    kChunk,
    kBlock,
    kFuncName,
    kVarList,
    kVar,
    kSimpleVar,
    kSubscribVar,
    kFieldVar,
    kNameList,
    kExpList,
    kArgs,
    kExprArgs,
    kTableArgs,
    kStringArgs,
    kParList,
    kField,
    kFieldList,
    kSingleField,
    kNameField,
    kExprField,
    kFuncBody,

    //Expressions
    kExpr,
    kNilExpr,
    kBoolExpr,
    kIntegerExpr,
    kNumeralExpr,
    kStringExpr,
    kEllipsisExpr,
    kFuncDefExpr,
    kTableConstructorExpr,
    kPrefixExpr,
    kFuncCallExpr,
    kFuncBodyExpr,
    kVarExpr,
    kParenExpr,
    kBinaryExpr,
    kUnaryExpr,
    kRegExpr,



    //Statement
    kStat,
    kEmptyStat,
    kAssignStat,
    kFuncCallStat,
    kBreakStat,
    kDoStat,
    kWhileStat,
    kCondStat,
    kIfStat,
    kForInStat,
    kForEqStat,
    kFuncDefStat,
    kLocalFuncStat,
    kLocalNameStat,
    kRetStat,

};

enum BinOpr {
    OPR_ADD, OPR_SUB, OPR_MUL, OPR_MOD, OPR_POW,
    OPR_DIV,
    OPR_IDIV,
    OPR_BAND, OPR_BOR, OPR_BXOR,
    OPR_SHL, OPR_SHR,
    OPR_CONCAT,
    OPR_EQ, OPR_LT, OPR_LE,
    OPR_NE, OPR_GT, OPR_GE,
    OPR_AND, OPR_OR,
    OPR_NOBINOPR
};


enum UnOpr { OPR_MINUS, OPR_BNOT, OPR_NOT, OPR_LEN, OPR_NOUNOPR };


class Node {
public:
    virtual std::string toString(){return "";}
    virtual ~Node(){}
    int type;
};

class Expr: public Node{
public:
    std::string toString() override {return "Expr";}
    Expr(){
        type = kExpr;
    }


    virtual bool isVar() const {return false;};
};

class Statement: public Node{
public:
    Statement(){
        type = kStat;
    }

    std::string toString() override {return "Stat";}
};



class Block: public Node{
public:
    //methods
    Block(){
        type = kBlock;
    }
    void AddStat(unique_ptr<Statement>&& stat){
        stats.push_back(std::move(stat));
    }

    std::string toString() override;

    vector<unique_ptr<Statement>> stats;
};

class Chunk: public Node{
public:
    Chunk(unique_ptr<Block> b){
        type = kChunk;
        block = std::move(b);
    }
    unique_ptr<Block> block;
};




class RetStat: public Statement{
public:
    RetStat(unique_ptr<ExprList> es = nullptr){
        type = kRetStat;
        exps = std::move(es);
    }
    std::string toString() override;
    unique_ptr<ExprList> exps;
};

class NameList: public Node{
public:
    NameList(){
        type = kNameList;
    }
    void AddName(LuaString* n)  {names.push_back(n);}
    vector<LuaString*> names;
};


//funcname ::= Name {‘.’ Name} [‘:’ Name]
class FuncName: public Node{
public:
    FuncName(){
        type = kFuncName;
    }
    void AddName(LuaString* n)  {names.push_back(n);}
    std::string toString() override;
    vector<LuaString*> names;
    LuaString* methodName;
};

class Var : public Expr{
public:
    std::string toString() override{return "var";}
    bool isVar() const override {return true;}
    Var(){
        type = kVar;
    }
};

class SimpleVar: public Var{
public:
    SimpleVar(LuaString* n){
        type = kSimpleVar;
        name = n;
    }
    bool isVar() const override {return true;}
    std::string toString() override;
    LuaString* name;
};

//var ::= prefixexp ‘[’ exp ‘]’
class SubscribVar: public Var{
public:
    SubscribVar(unique_ptr<Expr> p, unique_ptr<Expr> s= nullptr){
        type = kSubscribVar;
        prefix = std::move(p);
        sub = std::move(s);
    }
    std::string toString() override;
    bool isVar() const override {return true;}
    unique_ptr<Expr> prefix;
    unique_ptr<Expr> sub;
};
//prefixexp ‘.’ Name
class FieldVar: public Var{
public:
    FieldVar(unique_ptr<Expr> p, LuaString* n){
        type = kFieldVar;
        prefix = std::move(p);
        name = n;
    }
    bool isVar() const override {return true;}
    std::string toString() override;
    unique_ptr<Expr> prefix;
    LuaString* name;
};

class VarList: public Node{
public:
    VarList(){
        type = kVarList;
    }
    void AddVar(unique_ptr<Var> v){vars.push_back(std::move(v));}
    vector<unique_ptr<Var>> vars;
};


class Field: public Node{
public:
    Field(){type = kField;}
    std::string toString() override {return "Field";}
};

class SingleField: public Field{
public:
    SingleField(unique_ptr<Expr> e){
        type = kSingleField;
        exp = std::move(e);
    }
    std::string toString() override;

    unique_ptr<Expr> exp;
};
// Name ‘=’ exp
class NameField: public Field{
public:
    NameField(LuaString* n, unique_ptr<Expr> e){
        type = kNameField;
        name = n;
        exp = std::move(e);

    }
    std::string toString() override;
    LuaString* name;
    unique_ptr<Expr> exp;
};
//field ::= ‘[’ exp ‘]’ ‘=’ exp
class ExprField: public Field{
public:
    ExprField(unique_ptr<Expr> l, unique_ptr<Expr> r){
        type = kExprField;
        left = std::move(l);
        right = std::move(r);
    }
    std::string toString() override;
    unique_ptr<Expr> left;
    unique_ptr<Expr> right;
};

class FieldList: public Node{
public:
    FieldList(){
        type = kFieldList;
    }
    void AddField(unique_ptr<Field> f){fields.push_back(std::move(f)); }

    using FieldP  = std::unique_ptr<Field>;
    vector<FieldP> fields;
};

class ParList: public Node{
public:
    ParList(unique_ptr<NameList> ns, bool b){
        type = kParList;
        nameList = std::move(ns);
        hasVararg = b;
    }
    unique_ptr<NameList> nameList;
    bool hasVararg;
};

class FuncBody: public Node{
public:
    FuncBody(){
        type = kFuncBody;
        hasVararg = false;

    }
    void AddParam(LuaString* n)     {names.push_back(n);}
    std::string toString() override;
    bool hasVararg;
    vector<LuaString*> names;
    unique_ptr<Block> block;
};





/**
 * Expressions
 */
class ExprList: public Node{
public:
    ExprList(){
        type = kExpList;
    }
    int size()  {return exprList.size();}
    std::string toString() override;
    void AddExpr(unique_ptr<Expr> e){exprList.push_back(std::move(e));}
    vector<unique_ptr<Expr>> exprList;
};

class FuncBodyExpr: public Expr{
public:
    FuncBodyExpr(unique_ptr<FuncBody> f){
        type = kFuncBodyExpr;
        fbody = std::move(f);
    }
    unique_ptr<FuncBody> fbody;
};

class NilExpr: public Expr{
public:
    std::string toString() override {return "nil";}
    NilExpr(){
        type = kNilExpr;
    }
};
class BoolExpr: public Expr{
public:
    BoolExpr(bool v){
        type = kBoolExpr;
        bval = v;
    }
    std::string toString() override;

    bool bval;
};

class NumeralExpr: public Expr{
public:
    NumeralExpr(LuaNumber v){
        type = kNumeralExpr;
        nval = v;
    }
    std::string toString() override;
    LuaNumber nval;
};

class RegExpr: public Expr{
public:
    RegExpr(int r){
        type = kRegExpr;
        reg = r;
    }
    //std::string toString() override;
    int reg;
};

class IntegerExpr: public Expr{
public:
    IntegerExpr(LuaInteger v){
        type = kIntegerExpr;
        ival = v;
    }
    std::string toString() override;


    LuaInteger ival;
};

class StringExpr: public Expr{
public:
    StringExpr(LuaString* v){
        type = kStringExpr;
        sval = v;
    }
    std::string toString() override;
    LuaString* sval;
};

class EllipsisExpr: public Expr{
public:

    EllipsisExpr(){
        type = kEllipsisExpr;
        pc = -1;
    }
    int pc;
    std::string toString() override {return "...";}
};

class FunctionDefExpr: public Expr{
public:
    FunctionDefExpr(unique_ptr<FuncBody> f){
        type = kFuncDefExpr;
        body = std::move(f);
    }
    std::string toString() override;
    unique_ptr<FuncBody> body;
};

class TableConstructorExpr: public Expr{
public:
    TableConstructorExpr(){
        type = kTableConstructorExpr;
        varargField = false;
    }
    std::string toString() override;
    //std::string toString() override;
    vector<FieldP> listFields;
    vector<FieldP> recFields;
    bool varargField;

};

class Args : public Node{
public:
    Args(){
        type = kArgs;
    }

    std::string toString() override {return "args";}
};

class ExprArgs : public Args{
public:
    ExprArgs(unique_ptr<ExprList> es= nullptr){
        type = kExprArgs;
        exps = std::move(es);
    }

    std::string toString() override;

    unique_ptr<ExprList> exps;
};

class TableArgs: public Args{
public:
    TableArgs(unique_ptr<Expr> t){
        type = kTableArgs;
        tc = std::move(t);
    }
    std::string toString() override;
    unique_ptr<Expr> tc;
};

class StringArgs: public Args{
public:
    StringArgs(LuaString* s){
        type = kStringArgs;
        sarg = s;
    }
    std::string toString() override;
    LuaString* sarg;
};
class PrefixExpr: public Expr{
public:
    PrefixExpr(){type = kPrefixExpr;}
};

class VarExpr: public Expr{
public:
    VarExpr(unique_ptr<Var> v){
        type = kVarExpr;
        var = std::move(v);
    }
    bool isVar() const override  {return true;}
    std::string toString() override ;
    unique_ptr<Var> var;
};

class FunCallExpr: public Expr{
public:
    FunCallExpr(unique_ptr<Expr> p, unique_ptr<ExprList> as,  LuaString* mn = nullptr){
        type = kFuncCallExpr;
        prefix = std::move(p);
        args = std::move(as);
        methodName = mn;
        pc = -1;
    }


    std::string toString() override;


    unique_ptr<Expr> prefix;
    unique_ptr<ExprList> args;
    LuaString* methodName;//with colon
    int pc;
};

class ParenExpr: public Expr{
public:
    ParenExpr(unique_ptr<Expr> e){
        type = kParenExpr;
        expr = std::move(e);
    }
    std::string toString() override;
    unique_ptr<Expr> expr;
};

class BinaryOpExpr: public Expr{
public:
    BinaryOpExpr(unique_ptr<Expr> l, unique_ptr<Expr> r, int o){
        type = kBinaryExpr;
        left = std::move(l);
        right = std::move(r);
        op = o;
        pc = -1;
    }
    std::string toString() override;
    unique_ptr<Expr> left;
    unique_ptr<Expr> right;
    int op;
    int pc;//只有 concat操作才会用的pc的
};

class UnaryOpExpr: public Expr{
public:
    UnaryOpExpr(unique_ptr<Expr> e, int o){
        type = kUnaryExpr;
        exp = std::move(e);
        op = o;
    }
    std::string toString() override;
    unique_ptr<Expr> exp;
    int op;
};





/**
 * Statements
 */

class EmptyStat: public Statement{
public:
    EmptyStat(){
        type = kEmptyStat;
    }
    std::string toString() override{return "empty{}";}
};

class BreakStat: public Statement{
public:
    BreakStat(){type = kBreakStat;}
    std::string toString() override{return "break{}";}
};


class FunCallStat: public Statement{
public:
    FunCallStat(unique_ptr<Expr> f){
        type = kFuncCallStat;
        fcall = std::move(f);
    }
    std::string toString() override;
    unique_ptr<Expr> fcall;
};


// do blocak end
class DoStat: public Statement{
public:
    DoStat(unique_ptr<Block> b){
        type = kDoStat;
        block = std::move(b);
    }
    std::string toString() override;
    unique_ptr<Block> block;
};

class WhileStat: public Statement{
public:
    WhileStat(unique_ptr<Expr> e, unique_ptr<Block> b){
        type = kWhileStat;
        cond = std::move(e);
        block = std::move(b);
    }
    std::string toString() override;
    unique_ptr<Expr> cond;
    unique_ptr<Block> block;
};



class CondStat: public Statement{
public:
    CondStat(unique_ptr<Expr> e, unique_ptr<Block> b){
        type = kCondStat;
        cond = std::move(e);
        block = std::move(b);
    }
    std::string toString() override;
    unique_ptr<Expr> cond;
    unique_ptr<Block> block;
};

class IfStat: public Statement{
public:
    IfStat(unique_ptr<Expr> e, unique_ptr<Block> then, vector<CondStat> && elfs,
            unique_ptr<Block> els= nullptr){
        type = kIfStat;
        cond = std::move(e);
        block = std::move(then);
        elifs = std::move(elfs);
        other = std::move(els);
    }
    //void AddElif(unique_ptr<CondStat> c) {elifs.push_back(std::move(c));}
    std::string toString() override;


    unique_ptr<Expr> cond;
    unique_ptr<Block> block;
    vector<CondStat> elifs;
    unique_ptr<Block> other; //could be empty
};

//for namelist in explist do block end
class ForInStat: public Statement{
public:
    ForInStat(vector<LuaString*>&& ns, unique_ptr<ExprList> es,
              unique_ptr<Block> b){
        type = kForInStat;
        names = std::move(ns);
        explist = std::move(es);
        block = std::move(b);

    }
    std::string toString() override;
    vector<LuaString*> names;
    unique_ptr<ExprList> explist;
    unique_ptr<Block> block;
};
//for Name ‘=’ exp ‘,’ exp [‘,’ exp] do block end
class ForEqStat: public Statement{
public:
    ForEqStat(LuaString* n, unique_ptr<Block> bl,unique_ptr<Expr> a,
            unique_ptr<Expr> b,unique_ptr<Expr> c= nullptr){
        type = kForEqStat;
        name = n;
        block = std::move(bl);
        init = std::move(a);
        last = std::move(b);
        step = std::move(c);
    }
    std::string toString() override;

    LuaString* name;
    unique_ptr<Expr> init;
    unique_ptr<Expr> last;
    unique_ptr<Expr> step;

    unique_ptr<Block> block;
};

class FuncDefStat: public Statement{
public:
    FuncDefStat(unique_ptr<Var> fn, unique_ptr<FuncBody> fb, bool flag){
        type = kFuncDefStat;
        fname = std::move(fn);
        fbody = std::move(fb);
        isMethod = flag;
    }
    std::string toString() override;
    unique_ptr<Var> fname;
    //unique_ptr<FuncName> fname;
    unique_ptr<FuncBody> fbody;
    bool isMethod;
};

class LocalFuncStat: public Statement{
public:
    LocalFuncStat(LuaString* n, unique_ptr<FuncBody> fb){
        type = kLocalFuncStat;
        name = n;
        fbody = std::move(fb);
    }
    std::string toString() override;
    LuaString* name;
    unique_ptr<FuncBody> fbody;
};

class LocalNameStat: public Statement{
public:
    LocalNameStat(){
        type = kLocalNameStat;

    }
    std::string toString() override;
    void AddName(LuaString* n)  {names.push_back(n);}
    std::vector<LuaString*> names;
    unique_ptr<ExprList> exprlist;//could be empty
};

class AssignStat: public Statement{
public:
    AssignStat(){
        type = kAssignStat;
    }

    void AddVar(unique_ptr<Expr> v)  {vars.push_back(std::move(v));}
    std::string toString() override;

    std::vector<unique_ptr<Expr>> vars;
    unique_ptr<ExprList> exprs;
};




#endif //LUUA2_AST_H

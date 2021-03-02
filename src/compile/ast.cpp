//
// Created by gpf on 2020/12/29.
//

#include <fmt/format.h>
#include "ast.h"
#include "parser.h"

namespace {

    std::string uop2str(int uop){
        switch (uop) {
            case OPR_NOT: return "not";
            case OPR_MINUS: return "minus";
            case OPR_BNOT: return "bnot";
            case OPR_LEN: return "len";
            default: return "";
        }
    }

    std::string bop2str(int op) {
        switch (op) {
            case OPR_ADD: return "add";
            case OPR_SUB: return "sub";
            case OPR_MUL: return "mul";
            case OPR_MOD: return "mod";
            case OPR_POW: return "pow";
            case OPR_DIV: return "div";
            case OPR_IDIV: return "idiv";
            case OPR_BAND: return "band";
            case OPR_BOR: return "bor";
            case OPR_BXOR: return "bxor";
            case OPR_SHL: return "shl";
            case OPR_SHR: return "shr";
            case OPR_CONCAT: return "concat";
            case OPR_NE: return "ne";
            case OPR_EQ: return "eq";
            case OPR_LT: return "lt";
            case OPR_LE: return "le";
            case OPR_GT: return "gt";
            case OPR_GE: return "ge";
            case OPR_AND: return "and";
            case OPR_OR: return "or";
            default: return "";
        }
    }


}


std::string ExprList::toString() {
    std::string ret;
    ret.append("ExprList{");
    for(auto& exp: exprList){
        ret.append(exp->toString());
        ret.append(", ");
    }
    //remove last ,
    if(!exprList.empty()){
        ret.pop_back();
        ret.pop_back();
    }
    ret.append("}");
    return ret;
}

std::string BoolExpr::toString() {
    return fmt::format("bool{{{}}}", bval?"true":"false");
}

std::string NumeralExpr::toString() {
    return fmt::format("flt{{{}}}", nval);
}

std::string IntegerExpr::toString() {
    return fmt::format("int{{{}}}", ival);
}

std::string StringExpr::toString() {
    return fmt::format("str{{{}}}", sval->data());
}

std::string ParenExpr::toString() {
    return fmt::format("({})", expr->toString());
}

std::string BinaryOpExpr::toString() {
    auto opstr = bop2str(op);
    // add{int{3}int{4}}
    return fmt::format("{}{{{}{}}}",opstr, left->toString(), right->toString());
}

std::string UnaryOpExpr::toString() {
    auto opstr = uop2str(op);
    // minus{}
    return fmt::format("{}{{{}}}",opstr, exp->toString());
}

std::string VarExpr::toString() {
    return fmt::format("{}",var->toString());
}

std::string SimpleVar::toString() {
    return fmt::format("simvar{{{}}}",name->data());
}
//var ::= prefixexp ‘[’ exp ‘]’
std::string SubscribVar::toString() {
    return fmt::format("subvar{{{}{}}}", prefix->toString(), sub->toString());
}
//prefixexp ‘.’ Name
std::string FieldVar::toString() {
    return fmt::format("fieldvar{{{}{}}}", prefix->toString(), name->data());
}

std::string TableConstructorExpr::toString() {

    std::string ret("fields{");
    for(auto& field: listFields){
        ret.append(field->toString());
        ret.append(",");
    }
    for(auto& field: recFields){
        ret.append(field->toString());
        ret.append(",");
    }
    if(!listFields.empty() || !recFields.empty())
        ret.pop_back();
    ret.push_back('}');
    return ret;
}

std::string SingleField::toString() {
    return fmt::format("sinfield{{{}}}",exp->toString());
}

std::string NameField::toString() {
    return fmt::format("namefield{{{}{}}}",exp->toString(), name->data());
}

std::string ExprField::toString() {
    return fmt::format("exprfield{{{}{}}}",left->toString(), right->toString());
}

//args
std::string ExprArgs::toString() {
    return fmt::format("exprargs{{{}}}", exps?exps->toString():"");
}

std::string TableArgs::toString() {
    return fmt::format("tblarg{{{}}}", tc->toString());
}

std::string StringArgs::toString() {
    return fmt::format("strarg{{{}}}", sarg->data());
}

std::string FunCallExpr::toString() {
    if(methodName){
        return fmt::format("selfcall{{{}{}{}}}", prefix->toString(), methodName->data(), args->toString());
    }
    return fmt::format("funcall{{{}{}}}",prefix->toString(), args->toString());
}

//Statement
std::string Block::toString() {
    std::string ret("block{");
    for(auto &stat: stats){
        ret.append(stat->toString());
        ret.append(",");
    }
    if(!stats.empty())
        ret.pop_back();
    ret.append("}");
    return ret;

}

std::string RetStat::toString() {
    return fmt::format("ret{{{}}}", exps?exps->toString():"");
}

std::string DoStat::toString() {
    return fmt::format("do{{{}}}",block->toString());
}

std::string WhileStat::toString() {
    return fmt::format("while{{{}|{}}}",cond->toString(), block->toString());
}

std::string CondStat::toString() {
    return fmt::format("elif{{{}|{}}}", cond->toString(), block->toString());
}

std::string IfStat::toString() {
    auto ret =  fmt::format("if{{{}|{}",cond->toString(),block->toString());
    //add elfis
    for(auto &elf: elifs){
        ret.append(elf.toString());
    }
    if(other){
        ret.append("|");
        ret.append(other->toString());
    }
    ret.append("}");
    return ret;
}

std::string ForInStat::toString() {
    std::string ret("forin{");
    for(auto name:names){
        ret.append(name->data());
        ret.append(",");
    }
    ret.pop_back();
    ret.append(fmt::format("|{}|{}",explist->toString(), block->toString()));
    ret.append("}");
    return ret;
}
//for Name ‘=’ exp ‘,’ exp [‘,’ exp] do block end
std::string ForEqStat::toString() {
    auto ret = fmt::format("foreq{{{}|{}|{}}}",name->data(), init->toString(), last->toString());
    if(step){
        ret.append("|");
        ret.append(step->toString());
    }
    ret.append("|");
    ret.append(block->toString());
    return ret;
}

std::string FuncDefStat::toString() {
    return fmt::format("funcdef{{{}|{}}}", fname->toString(), fbody->toString());
}

std::string LocalFuncStat::toString() {
    return fmt::format("locfunc{{{}|{}}}",name->data(), fbody->toString());
}

std::string LocalNameStat::toString() {
    std::string ret("locstat{");
    for(auto name: names){
        ret.append(name->data());
        ret.append(",");
    }
    if(!names.empty())
        ret.pop_back();
    if(exprlist){
        ret.append(fmt::format("|{}", exprlist->toString()));
    }
    ret.append("}");
    return ret;
}

std::string AssignStat::toString() {
    std::string ret("assign{");
    for(auto &var: vars){
        ret.append(var->toString());
        ret.append(",");
    }
    if(!vars.empty()) ret.pop_back();
    if(exprs){
        ret.append(fmt::format("|{}", exprs->toString()));
    }
    ret.append("}");
    return ret;
}

std::string FuncBody::toString() {
    std::string ret("funcbody{");
    for(auto name: names){
        ret.append(name->data());
        ret.append(",");
    }
    if(hasVararg)
        ret.append("...");
    if(!hasVararg && !names.empty())
        ret.pop_back();
    ret.append(fmt::format("|{}}}", block->toString()));
    return ret;
}

std::string FunCallStat::toString() {
    return fmt::format("funstat{{{}}}",fcall->toString());
}

std::string FuncName::toString() {
    std::string ret("fname{");
    for(auto name: names){
        ret.append(name->data());
        ret.append(".");
    }
    ret.pop_back();
    if(methodName){
        ret.append(":");
        ret.append(methodName->data());
    }
    ret.append("}");
    return ret;
}


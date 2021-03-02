//
// Created by gpf on 2020/12/14.
//

#include "object/luaString.h"
#include "proto.h"
#include "memory/GC.h"
#include "vm/globalState.h"
#include "vm/opcodes.h"
#include <fmt/core.h>

#define MYK(x)		(-1-(x))


Proto * Proto::CreateProto() {
    auto obj = GlobalState::gc->createObject(LUA_TPROTO, sizeof(Proto));
    auto proto = static_cast<Proto*>(obj);

    //初始化赋值
    proto->nparams_ = 0;
    proto->isVararg_ = 0;
    proto->maxStackSize_ = 0;
    proto->lastlinedefined_ = 0;
    proto->linedefined_ = 0;
    proto->source_ = nullptr;
    proto->consts_ = Array<Value>();
    proto->codes_  = Array<Instruction>();
    proto->lineInfo_ = Array<int>();
    proto->subs_ = Array<Proto*>();
    proto->localVars_ = Array<Proto::LocVar>();
    proto->upvalDescs_ = Array<Proto::Upvaldesc>();

    return proto;
}

void Proto::DestroyProto(Proto *p) {
    p->consts_.Destroy();
    p->codes_.Destroy();
    p->lineInfo_.Destroy();
    p->subs_.Destroy();
    p->localVars_.Destroy();
    p->upvalDescs_.Destroy();

    Allocator::Free(p, sizeof(Proto));
}

std::string Proto::toString() const {
    //header
    std::string ret;
    ret.append(header2string());
    ret.append(codes2string());
    for(int i=0;i<subs_.len();i++){
        ret.append(subs_[i]->toString());
    }

    return ret;
}

void Proto::addCode(int pc, Instruction i) {
    /*
    if(pc>=codes_.len()){
        codes_.resize(pc);
    }
    codes_[pc] = i;
     */
    codes_.push_back(i);
}

int Proto::addConst(Value *val) {
    consts_.push_back(*val);
    return consts_.len()-1;
}

int Proto::addLocalVar(LuaString *varname) {
    //lvar是新生成的localvar
    auto& lvar =  localVars_.push_back();
    lvar.varname = varname;
    return localVars_.len()-1;
}

/*
void Proto::addLineInfo(int pc, Instruction i) {
    if(pc>= lineInfo_.len()){
        lineInfo_.resize(pc);
    }
    lineInfo_[pc] = i;
}
 */



std::string Proto::constant2String(int i) const {
    if(!(i>=0 && i< consts_.len())) return " ";
    auto &value = consts_[i];
    std::string ret;
    switch (value.type) {
        case LUA_TNIL:
            ret.append("nil");
            break;
        case LUA_TBOOLEAN:
            if(value.bval)  ret.append("true");
            else            ret.append("false");
            break;
        case LUA_TNUMFLT:
            ret.append(std::to_string(value.nval));
            break;
        case LUA_TNUMINT:
            ret.append(std::to_string(value.ival));
            break;
        case LUA_TSTRING:
        case LUA_TLNGSTR:
            ret.append("\"");
            ret.append(static_cast<LuaString*>(value.obj)->data());
            ret.append("\"");
            break;
        default:{
            throw std::runtime_error("should not reach here!");
        }
    }

    return ret;
}

const char * Proto::upvalueNameAt(int i) const {
    if(upvalDescs_[i].name)
        return upvalDescs_[i].name->data();
    return "-";
}



/**
 * 打印一条指令
 * @param ins
 */

std::string Proto::code2string(int idx, Instruction ins) const {
    std::string ret;

    auto o = ByteCode::getOpCode(ins);
    int a = ByteCode::getArgA(ins);
    int b = ByteCode::getArgB(ins);
    int c = ByteCode::getArgC(ins);
    int ax = ByteCode::getArgA(ins);
    //int bx = 0;
    int bx = ByteCode::getArgBx(ins);
    int sbx = ByteCode::getArgsBx(ins);
    ret.append(fmt::format("\t{}\t[-]\t", idx+1));
    ret.append(fmt::format("{:9}\t", ByteCode::opnames[o]));
    //if(o == ByteCode::OP_JMP){
    //    int i=1;
    //}
    switch (ByteCode::getOpMode(o)) {
        case ByteCode::iABC:
            ret.append(std::to_string(a));
            if(ByteCode::getBMode(o) != ByteCode::OpArgN){
                ret.append(" ");
                ret.append(std::to_string(ISK(b)?(MYK(INDEXK(b))):b));
            }

            if(ByteCode::getCMode(o) != ByteCode::OpArgN){
                ret.append(" ");
                ret.append(std::to_string(ISK(c)?(MYK(INDEXK(c))):c));
            }
            break;
        case ByteCode::iABx:
            ret.append(std::to_string(a));
            ret.append(" ");
            if(ByteCode::getBMode(o) == ByteCode::OpArgK)
                ret.append(std::to_string(MYK(bx)));
            if(ByteCode::getBMode(o) == ByteCode::OpArgU)
                ret.append(std::to_string(bx));
            break;
        case ByteCode::iAsBx:
            ret.append(fmt::format("{} {}",a,sbx));
            break;
        case ByteCode::iAx:
            ret.append(std::to_string(MYK(ax)));
            break;
    }
    switch (o) {
        case ByteCode::OP_LOADK:
            ret.append("\t; ");
            ret.append(constant2String(bx));
            break;
        case ByteCode::OP_GETUPVAL:
        case ByteCode::OP_SETUPVAL:
            ret.append(fmt::format("\t; {}", upvalueNameAt(b)));
            break;
        case ByteCode::OP_GETTABUP:
            ret.append(fmt::format("\t; {}", upvalueNameAt(b)));
            if(ISK(c)){
                ret.append(" ");
                ret.append(constant2String(INDEXK(c)));
            }
            break;
        case ByteCode::OP_SETTABUP:
            ret.append(fmt::format("\t; {}", upvalueNameAt(a)));
            if(ISK(b)){
                ret.append(" ");
                ret.append(constant2String(INDEXK(b)));
            }
            if(ISK(c)){
                ret.append(" ");
                ret.append(constant2String(INDEXK(c)));
            }
            break;
        case ByteCode::OP_GETTABLE:
        case ByteCode::OP_SELF:
            if(ISK(c)){
                ret.append("\t; ");
                ret.append(constant2String(INDEXK(c)));
            }
            break;
        case ByteCode::OP_SETTABLE:
        case ByteCode::OP_ADD:
        case ByteCode::OP_SUB:
        case ByteCode::OP_MUL:
        case ByteCode::OP_POW:
        case ByteCode::OP_DIV:
        case ByteCode::OP_IDIV:
        case ByteCode::OP_BAND:
        case ByteCode::OP_BOR:
        case ByteCode::OP_BXOR:
        case ByteCode::OP_SHL:
        case ByteCode::OP_SHR:
        case ByteCode::OP_EQ:
        case ByteCode::OP_LT:
        case ByteCode::OP_LE:
            if(ISK(b) || ISK(c)){
                ret.append("\t; ");
                if(ISK(b))  ret.append(constant2String(INDEXK(b)));
                else        ret.append("-");
                if(ISK(c))  ret.append(constant2String(INDEXK(c)));
                else        ret.append("-");
            }
            break;
        case ByteCode::OP_JMP:
        case ByteCode::OP_FORLOOP:
        case ByteCode::OP_FORPREP:
        case ByteCode::OP_TFORLOOP:
            ret.append("\t; to ");
            ret.append(std::to_string(sbx + idx + 2));
            break;
        case ByteCode::OP_CLOSURE:
            ret.append("\t; ");
            ret.append(std::to_string(bx));
            break;
        case ByteCode::OP_SETLIST:
            ret.append("\t; ");
            ret.append(std::to_string(c));
            break;
        default:
            break;
    }
    return ret;
}

//打印所有的指令
std::string Proto::codes2string() const {
    std::string ret;
    for(int i=0;i<codes_.len();i++){
        ret.append(code2string(i, codes_[i]));
        ret.append("\n");
    }
    return ret;
}

std::string Proto::header2string() const {
    std::string ret;
    ret.append("<header>\n");
    ret.append(fmt::format("{}{} params {} slots {} upvalues, ", nparams_, isVararg_?"+":"",
            maxStackSize_, upvalDescs_.len()));
    ret.append(fmt::format("{} locals {} constant {} function\n",
            localVars_.len(), consts_.len(), subs_.len()));

    return ret;
}


void Proto::markSelf() {
    type_ = GC_BLACK;
    if(source_) source_->markSelf();
    for(int i=0;i<codes_.len();i++)
        consts_[i].markSelf();
    for(int i=0;i<subs_.len();i++)
        subs_[i]->markSelf();
    for(int i=0;i<localVars_.len();i++){
        auto name = localVars_[i].varname;
        if(name) name->markSelf();
    }
    for(int i=0;i<upvalDescs_.len();i++){
        auto name = upvalDescs_[i].name;
        if(name) name->markSelf();
    }

}

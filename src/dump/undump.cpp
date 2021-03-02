//
// Created by gpf on 2020/12/14.
//

#include "undump.h"
#include "object/luaString.h"
#include "vm/globalState.h"
#include <fstream>
#include <sstream>
#include <vector>

Undumper::Undumper(std::string fname):fname_(fname) ,idx_(0){
    //读取整个文件到buff中去
    std::ifstream in(fname.data());
    if(!in){
        //error("file not exist!");
        throw std::runtime_error("File is not exist!");
    }
    std::stringstream ss;
    ss<<in.rdbuf();
    buf_ = std::move(ss.str());
    len_ = buf_.size();
}

LuaByte Undumper::readByte() {
    assert(idx_< buf_.size());
    return buf_.at(idx_++);
}

int Undumper::readInt() {
    auto a0 = readByte();
    auto a1 = readByte();
    auto a2 = readByte();
    auto a3 = readByte();
    return (a3 << 24)|(a2<<16)|(a1<<8)|a0;
}

unsigned int Undumper::readUInt() {
    auto a0 = readByte();
    auto a1 = readByte();
    auto a2 = readByte();
    auto a3 = readByte();
    return (a3 << 24)|(a2<<16)|(a1<<8)|a0;
}

LuaInteger Undumper::readInteger() {
    LuaInteger a0 = readInt();
    LuaInteger a1 = readInt();
    return (a1<<32) | a0;
}

//把int型转化为double
LuaNumber Undumper::readNumber() {
    auto a = readInteger();
    return *(reinterpret_cast<LuaNumber*>(&a));
}

LuaString * Undumper::readString() {
    size_t size = readByte();
    if(size == 0xFF)//长字符串
        size = readInteger();
    if(size ==0) //空的字符串
        return nullptr;
    //对于长字符串和短字符串一并处理
    auto chars = readChars(size-1);
    auto obj = LuaString::CreateString(chars.data(), size-1);
    return obj;

}

Array<LuaByte> Undumper::readBytes(size_t len) {
    Array<LuaByte> ret(len, 0);
    for(size_t i=0;i<len;i++){
        ret[i] = readByte();
    }
    return ret;
}

std::string Undumper::readChars(size_t len) {
    std::string ret(len, 0);
    for(size_t i=0;i<len;i++){
        ret[i] = readByte();
    }
    return ret;
}

void Undumper::readCodes(Proto* p) {
    int n = readUInt();
    p->codes_ = Array<Instruction>(n);
    for(int i=0;i<n;i++)
        p->codes_[i] =  readUInt();
}

void Undumper::readConstants(Proto* p) {
    int n = readInt();
    p->consts_ = Array<Value>(n);
    //初始化为null
    for(int i=0;i<n;i++)
        p->consts_[i] = Value();
    for(int i=0;i<n;i++){
        int t = readByte();
        auto& val = p->consts_[i];
        switch (t) {
            case LUA_TNIL: //set nil before
                break;
            case LUA_TBOOLEAN:
                val = Value(readByte());
                break;
            case LUA_TNUMFLT:
                val = Value(readNumber());
                break;
            case LUA_TNUMINT:
                val = Value(readInteger());
                break;
            case LUA_TSTRING:
                val = Value(static_cast<GCObject*>(readString()), LUA_TSTRING);
                break;
            default:
                //should not reach here
                assert(0);
        }
    }

}

void Undumper::readUpvalues(Proto* p) {
    int n = readInt();
    p->upvalDescs_ = Array<Proto::Upvaldesc>(n);

    //开始赋值
    for(int i=0;i<n;i++){
        p->upvalDescs_[i].name = nullptr;
        p->upvalDescs_[i].instack = readByte();
        p->upvalDescs_[i].idx = readByte();
    }
}

void Undumper::readDebug(Proto* p) {
    //read line infors
    int n = readInt();
    p->lineInfo_ = Array<int>(n);
    for(int i=0;i<n;i++)
        p->lineInfo_[i] = readInt();

    //read localvars
    n = readInt();
    p->localVars_ = Array<Proto::LocVar>(n);
    for(int i=0;i<n;i++){
        p->localVars_[i].varname = readString();
        p->localVars_[i].startpc = readInt();
        p->localVars_[i].endpc = readInt();
    }

    //read unpvalue names
    n = readInt();
    for(int i=0;i<n;i++)
        p->upvalDescs_[i].name = readString();

}

void Undumper::readProtos(Proto* p) {
    int n = readInt();
    p->subs_ = Array<Proto*>(n);
    for(int i=0;i<n;i++)
        p->subs_[i] = nullptr;
    for(int i=0;i<n;i++){
        p->subs_[i] = Proto::CreateProto();
        readFunction(p->subs_[i], p->source_);
    }

}


void Undumper::readFunction(Proto* p, LuaString* psource){
    p->source_ = readString();
    if(!p->source_){
        p->source_ = psource;
    }

    p->linedefined_ = readUInt();
    p->lastlinedefined_ = readUInt();
    p->nparams_ = readByte();
    p->isVararg_ = readByte();
    p->maxStackSize_ = readByte();

    //other
    readCodes(p);
    readConstants(p);
    readUpvalues(p);
    readProtos(p);
    readDebug(p);
}

void Undumper::checkHeader() {
    auto assert_eq = [](bool s, const char* msg){
        if(!s){
            fprintf(stderr,"%s\n",msg);
            exit(1);
        }
    };

    assert_eq(readBytes(4) == GlobalState::sig, "not a precompiled chunk");
    assert_eq(readByte() == kLuacVersion, "version mismatch!");
    assert_eq(readByte() == kLuaFormat,"format mismatch!");
    //auto bytes = readBytes(6);
    assert_eq(readBytes(6)==GlobalState::LUAC_DATA,"corrupted!");
    assert_eq(readByte() == CINT_SIZE, "int size mismatch!");
    assert_eq(readByte()==CSIZET_SIZE, "size_t size match");
    assert_eq(readByte()==INSTRUCTION_SIZE, "instuction size mismatch!");
    assert_eq(readByte()==LUA_INTEGER_SIZE, "lua_Integer size mismatch!");
    assert_eq(readByte()==LUA_NUMBER_SIZE, "lua_Number size mismatch!");
    assert_eq(readInteger()==LUAC_INT, "endianness mismatch!");
    assert_eq(readNumber()==LUAC_NUM, "float format mismatch!");


}

//public function
Proto * Undumper::GetProto() {
    Proto* proto = Proto::CreateProto();
    readFunction(proto, nullptr);
    return proto;
}

//static function
Proto * Undumper::Undump(const char *fname) {
    Undumper r(fname);
    r.checkHeader();
    r.readByte();//silly byte
    return r.GetProto();

}
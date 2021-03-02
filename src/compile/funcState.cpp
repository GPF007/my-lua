//
// Created by gpf on 2021/1/11.
//

#include "funcState.h"
#include "vm/opcodes.h"
#include "object/luaTable.h"

std::vector<Vardesc> FuncState::activeVars_;

/**
 * 构造函数 Constructor
 */
FuncState::FuncState(FuncState* prev, Proto* p, BlockCnt* bl) {
    prev_ = prev;
    proto_ = p;
    blcnt_ = bl;
    firstLocal_ = activeVars_.size();
    freereg_ = 0;
    pc_ = 0;
    nconst_ = 0;
    nsub_ = 0;
    nlocalvar_ = 0;
    nups_ = 0;
    nactvar_=0;
    ctable_ = LuaTable::CreateTable();
    enterBlock(bl);
}

void FuncState::checkStack(int n) {
    int newStack = freereg_ + n;
    if(newStack > proto_->maxStackSize()){
        proto_->setMaxStack(newStack);
    }
}

void FuncState::allocRegs(int n) {
    checkStack(n);
    freereg_+=n;
}

/**
 * 得到i处的localvar信息，i是在当前activa var中的索引
 * @param i
 * @return
 */
Proto::LocVar * FuncState::getlocalvar(int i) {
    int idx = activeVars_[firstLocal_ + i].idx;
    return proto_->localVarAt(idx);
}

/**
 * 创建一个局部变量 ，名字是 name
 */
void FuncState::createLocalVar(LuaString *name) {
    proto_->addLocalVar(name);
    //idx 是 新建的localvar所在的位置
    short idx = nlocalvar_;
    nlocalvar_++;
    ASSERT(proto_->nlocal() == nlocalvar_);
    //在活跃的局部变量中加入该变量
    activeVars_.push_back(Vardesc{idx});
}

void FuncState::createLocalVarLiteral(const char *str) {
    createLocalVar(LuaString::CreateString(str));
}

//设置nvars个变量的pc信息
void FuncState::adjustLocalVars(int nvars) {
    nactvar_ +=  nvars;
    for(;nvars;nvars--){
        getlocalvar(nactvar_ - nvars)->startpc = pc_;
    }
}

/**
 * 进入一个新的block ，该函数会讲bl加入到当前blcnt的下一个节点，然后置位当前的blcnt
 * 这里会检查一个可用的下一个寄存器索引应该是actvar的数量
 */
void FuncState::enterBlock(BlockCnt* bl) {
    bl->nactvar = nactvar_;
    bl->previous = blcnt_;
    bl->upval = 0;
    blcnt_ = bl;
    ASSERT(freereg_ == nactvar_);
}

/**
 * 退出当前块
 */
void FuncState::leaveBlock() {
    auto bl = blcnt_;
    if(bl->previous && bl->upval){
        //todo close upvalue
    }
    blcnt_ = bl->previous;
    //删除当前块中的变量
    //因为在新的块中新建局部变量只会改变nactvar_的数量，不会改变block中nactvar的数量
    //所以要移除掉所有新加入的活跃变量， bl的nactvar为进入该块时的变量个数


    while(nactvar_ > bl->nactvar){
        getlocalvar(--nactvar_)->endpc = pc_;
        activeVars_.pop_back();
    }
    ASSERT(bl->nactvar == nactvar_);
    //更新 freereg为下一个nactvar的位置
    freereg_ = nactvar_;
}

/**
 * 修正pc处jump指令的偏移值为offset
 * @param pc
 * @param offset
 * A sBx	pc+=sBx;
 */
void FuncState::fixJump(int pc, int offset) {
    auto codes = proto_->codes();
    //codes[pc] |= static_cast<Instruction>(offset)<<POS_Bx;
    ByteCode::setArgsBx(codes[pc], offset);
}

void FuncState::fixSbx(int pc, int offset) {
    //todo
    auto codes = proto_->codes();
    ByteCode::setArgsBx(codes[pc], offset);

}




/**
 * 创建一个upvalue
 * @param name
 * @param vt
 * @return
 */
int FuncState::newUpvalue(LuaString *name, VarType vt,int idx) {
    LuaString* ts = LuaString::CreateString("mt");
    if(ts == name){
        int a = 1;
    }
    auto& nupval = proto_->addUpvalue();
    nupval.name = name;
    nupval.idx = idx;
    //只有在local情况下instack才是1
    nupval.instack =static_cast<LuaByte>(vt==kLocal);
    return nups_++;
}

/**
 * 在fs中查找名字为name的变量，如果有返回变量的索引，以及类型
 * @param fs
 * @param name
 * @return
 */
FuncState::VarPair FuncState::findVar(FuncState *fs, LuaString *name) {
    if(!fs){
        return {kGloabl,0};//-2说明是全局变量
    }
    //在当前fs中查找，如果有直接返回
    for(int i= fs->nactvar_-1;i>=0;i--){
        auto targetName = fs->getlocalvar(i)->varname;
        if(name == targetName)
            return {kLocal,i};
    }
    //此时说明当前fs中没有变量name，尝试upvalue
    int idx = fs->searchUpvalue(name);
    if(idx < 0 ){
        //递归查找
        auto res = findVar(fs->prev_, name);
        //如果是global说明没有这个变量，直接返回就行了
        if(res.first == kGloabl){
            return res;
        }
        //需要新建一个upvalue
        idx = fs->newUpvalue(name, res.first, res.second);

    }

    return {kUpval, idx};

}

/**
 * 在当前层级查找upvalue为name的元素，如果有返回索引，否则返回-1
 * @param name
 * @return
 */
int FuncState::searchUpvalue(LuaString *name) {
    for(int i=0;i<proto_->nupval();i++){
        auto& upval =proto_->upvalDescAt(i);
        if(name == upval.name)
            return i;
    }
    return -1;
}

/**
 * 查找名为name的var所在的寄存器，如果没有返回-1
 * @param name
 * @return
 */
FuncState::VarPair FuncState::singleVar(LuaString *name) {
    //利用辅助函数递归查找
    auto res = findVar(this, name);
    //todo 创建全局的变量
    if(res.first == kGloabl){
        //此时在env中创建全局的变量哟
        //先得到env的var
        LuaString* envstr = LuaString::CreateString("ENV_");
        auto envar = findVar(this, envstr);
        //env变量应该是local或者upvalue的
        ASSERT(envar.first != kGloabl);

    }
    return res;

}

//新建一个subproto，然后返回
Proto * FuncState::addSubProto() {
    auto subproto = Proto::CreateProto();
    proto_->addProto(subproto);
    return subproto;
}
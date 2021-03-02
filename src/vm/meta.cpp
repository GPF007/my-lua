//
// Created by gpf on 2020/12/17.
//

#include "meta.h"
#include "globalState.h"
#include "object/luaTable.h"


//初始化meta类,赋值state然后初始化name
void Meta::Init() {
    state_ = nullptr;
    static const char *const luaT_eventname[] = {  /* ORDER TM */
            "__index", "__newindex",
            "__gc", "__mode", "__len", "__eq",
            "__add", "__sub", "__mul", "__mod", "__pow",
            "__div", "__idiv",
            "__band", "__bor", "__bxor", "__shl", "__shr",
            "__unm", "__bnot", "__lt", "__le",
            "__concat", "__call"
    };

    //初始names
    for(int i=0;i<TM_N;i++){
        mtnames_[i] = LuaString::CreateString(luaT_eventname[i]);
        //todo fix 让该name 不回收
        
    }
}

/**
 * 尝试调用一个元方法
 * @param f 调用的函数
 * @param a 第一个参数
 * @param b 第二个参数
 * @param idxres 结果所在的idx,栈索引
 * @param hasres 是否有结果
 */
void Meta::callMetaMethod(const Value &f, const Value &a, const Value &b, int idxres, int hasres) {
    int funcidx = state_->top();
    state_->push(f);

    state_->push(a);
    state_->push(b);
    //push 三个值然后准备调用
    if(!hasres){//如果没有结果产生,那么idxres代表第三个参数值
        state_->push(state_->at(idxres));
    }
    state_->call(funcidx,hasres);
    if(hasres){//如果有结果产生的话,结果在栈下面一个位置
        //拷贝到目标位置
        state_->at(idxres) = state_->pop();
    }

}

/**
 * 尝试调用接受两个参数的元方法
 * @param a  第一个参数
 * @param b  第二个参数
 * @param idres  结果应该放置的位置
 * @param event  调用的方法
 */
bool Meta::callBinaryMetaMethod(const Value &a, const Value &b, int idres, TMS event) {
    //首先尝试a对象的event方法是否存在如果存在即调用
    //如果不存在尝试b对象的event方法
    auto tm = getMtFromObj(a,event);
    if(tm->isNil())
        tm = getMtFromObj(b,event);
    if(tm->isNil())
        return false;
    callMetaMethod(*tm, a,b, idres, 1);
    return true;
}

/**
 * 得到obj的一个metamethod指针 如果没有返回nil
 */
const Value * Meta::getMtFromObj(const Value &obj, TMS event) {
    if(!obj.isTable())
        return kNil;
    auto tbl = static_cast<LuaTable*>(obj.obj);
    return getMtFromTable(tbl, event);
}

void Meta::tryCallBinary(const Value &a, const Value &b, int idres, TMS event) {
    if(!callBinaryMetaMethod(a,b,idres, event)){
        state_->runerror("No metamethod {} between two object!",mtnames_[event]->data());
    }
}

//从table中返回一个event的元方法
const Value * Meta::getMtFromTable(LuaTable *tbl, TMS event) {
    auto mttable = tbl->metatable();
    if(!mttable)
        return nullptr;
    auto tmname = mtnames_[event];
    return mttable->getShortString(tmname);

}

//比较元方法,会把结果放在top的位置上
//结果返回比较的结果
bool Meta::callOrderMetaMethod(const Value &a, const Value &b,TMS event) {
    if(!callBinaryMetaMethod(a,b, state_->top(), event)){
        return false;
    }else{
        return !state_->at(state_->top()).isFalse();
    }
}
/**
 * 将state索引为idx的元素推入栈顶,成功返回true
 * @param idx  索引
 * @return
 */
bool Meta::getmetatable(int idx) {
    idx += state_->ci()->base;
    auto &obj = state_->at(idx);
    if(!obj.isTable())
        return false;
    auto tbl = static_cast<LuaTable*>(obj.obj);
    auto mt = tbl->metatable();
    if(mt){
        state_->push(Value(mt, LUA_TTABLE));
        return true;
    }
    return false;
}

/**
 * 设置idx处的值为栈顶的metatable,栈顶下降一个值,成功返回true
 * @param idx
 * @return
 */
bool Meta:: setmetatable(int idx) {
    idx += state_->ci()->base;
    auto& obj = state_->at(idx);
    auto& top  = state_->at(state_->top() - 1);
    if(!obj.isTable()){
        state_->runerror("Only Table could have metatable!");
    }

    auto mt = top.isTable()? static_cast<LuaTable*>(top.obj): nullptr;
    auto tbl = static_cast<LuaTable*>(obj.obj);
    tbl->setmetatable(mt);
    state_->pop();
    return true;

}
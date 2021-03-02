//
// Created by gpf on 2020/12/15.
//

#include "luaState.h"
#include "object/closure.h"
#include "object/proto.h"
#include "utils/debug.h"
#include "memory/GC.h"
#include "vm/globalState.h"
#include "vm/VM.h"
#include "vm/meta.h"

#define EXTRA_STACK   5
#define LUA_MINSTACK	20

#define BASIC_STACK_SIZE        (2*LUA_MINSTACK)
#define LUAI_MAXSTACK		1000000

/**
 * 初始化栈 栈的初始大小为40,然后在0处设置为当前ci的func,top设置为1,
 * lastfree设置为35位置处
 */
void LuaState::initStack() {

    stack_ = (Value*) Allocator::NewVector(nullptr, BASIC_STACK_SIZE, sizeof(Value));
    for(int i=0;i<BASIC_STACK_SIZE;i++)
        stack_[i].setnil();
    stackSize_ = BASIC_STACK_SIZE;
    top_ = 0;
    lastFree_ = stackSize_ - EXTRA_STACK;
    ci_ = baseCi_;
    ci_->next = ci_->previous = nullptr;
    ci_->status = 0;
    ci_->func = top_;
    top_++;
    ci_->top = top_ +  LUA_MINSTACK;

}

/**
 * 清除当前栈
 */
void LuaState::freeStack() {
    if(!stack_)
        return;
    //删除所有的callinfo
    ci_ = baseCi_;
    auto ci = ci_;
    auto next = ci->next;
    ci->next = nullptr;
    while((ci = next)!= nullptr){
        next = ci->next;
        Allocator::Free(ci, sizeof(CallInfo));
    }
    //最后删除栈
    Allocator::Free(stack_, stackSize_*sizeof(Value));
}

/**
 * 清除state l
 * @param l
 */
void LuaState::DestroyLuaState(LuaState *l) {
    //todo 关闭所有闭包
    auto cur = l->openlist_;
    while(cur != nullptr){
        auto next= LuaUpvalue::DestroyUpvalue(cur);
        cur = next;
    }
    l->freeStack();
    //然后删除栈本身
    Allocator::Free(l, sizeof(LuaState));

}

/**
 * 在当前state上面新建一个callinfo,然后返回它
 * 还会更细当前state的callinfo
 * @return 新创建的callinfo
 */
CallInfo * LuaState::ExtendCallInfo() {
    CallInfo* ci = (CallInfo*) Allocator::Alloc(sizeof(CallInfo));
    ci_->next = ci;
    ci->previous = ci_;
    ci->next = nullptr;
    ci_ = ci;
    return ci;
}

/**
 * 增加n个栈位置
 * @param n
 */
void LuaState::growStack(int n) {
    //首先查看当前栈的大小是否已经超过了maxstack
    if(stackSize_ > LUAI_MAXSTACK){
        panic("Exceed max stack!");
    }else{
        //尝试扩容为 min(2*size, needed),检查是否溢出,如果没有溢出那么直接realloc
        int needed = top_ + n + EXTRA_STACK;
        int newsize = 2*stackSize_;
        if(newsize>LUAI_MAXSTACK)
            newsize = LUAI_MAXSTACK;
        if(newsize < needed)
            newsize = needed;
        if(newsize>LUAI_MAXSTACK){
            panic("stack overflow");
        }else{
            reallocStack(newsize);
        }
    }
}

/**
 * 重新分配栈
 * @param n
 */
void LuaState::reallocStack(int n) {
    int lim = stackSize_;
    //重新分配
    Value* oldstack = stack_;
    stack_ = (Value*) Allocator::ReallocVector(stack_, stackSize_, n, sizeof(Value));
    for(;lim < n;lim++)
        stack_[lim].setnil();
    stackSize_ = n;
    lastFree_ =  n - EXTRA_STACK;
    //todo correct stack
    //如果原来的stack上存在开放的upvalue会更新upvalue指针的位置
    auto cur = openlist_;
    while(cur!= nullptr){
        cur->setVal(stack_ + (cur->val() - oldstack));
        cur = cur->next();
    }
}

/**
 * precall这个函数用来新键frame,传入的参数有func所在的stkid以及需要返回的result个数，nresults
 * 首先根据func得到closure，分别处理
 * 1、对于luaclosure，首先计算实际传入的参数，即func到top之间的个数，然后检查栈的大小
 *    然后根据vararg调整参数个数，接着设置新的栈底为base+1(不是全局的base_),即下一个栈开始的位置
 *    最后创建一个新的callinfo，替换为当前的callinfo，更新栈顶等信息然后返回0
 * 2、对于Cclosure，创建一个虚拟的stack，大小为20，然后在上面完成调用，最后返回结果
 *
 *
 */
int LuaState::precall(int func, int nresults) {
    auto& funcobj = stack_[func];
    //auto closure = static_cast<Closure*>(stack_[func].gc);
    //根据closure的类型做不同的处理

    switch (funcobj.type) {
        case LUA_TLCL:{//如果是一个lua function
            int  base;
            auto lclosure = static_cast<LuaClosure*>(funcobj.obj);
            auto proto = lclosure->proto();
            //实际传入的参数
            int nargs = top_ - func -1;
            int framesize = proto->maxStackSize_;

            //检查栈的大小
            checkStack(framesize);
            if(proto->isVararg_){
                base = adjustVararg(proto, nargs);
            }else{
                //是固定的参数,首先补全需要但没有传入的参数为nil
                while(nargs < proto->nparams_){
                    pushnil();
                    nargs++;
                }
                base = func+1;
            }

            //现在进入新的frame,创建一个新的callinfo
            //auto luaCallInfo = new LuaCallInfo(func, base, nresults, proto->codes_);
            auto ci = nextCallInfo();
            ci->nresult = nresults;
            ci->func = func;
            ci->base = base;
            ci->savedpc = proto->codes_.data();
            ci->status = 0;
            //更新当前的栈顶
            top_ = ci->top = base + framesize;
            return 0;
        }
        case LUA_TCCL:{//如果是cclosure
            auto f = static_cast<CClosure*>(funcobj.obj)->func();
            checkStack(LUA_MINSTACK); //c函数需要20个slots
            auto saveBase = ci_->base;
            auto ci = nextCallInfo();
            ci->nresult = nresults;
            ci->func = func;
            ci->top = top_ + LUA_MINSTACK;
            ci->base = func;
            //actual call
            int n = f(this);

            poscall(ci, top_ - n, n);
            return 1;
        }
        default:{//尝试调用call元方法

            auto tm = GlobalState::meta->getMtFromObj(funcobj, Meta::TM_CALL);
            if(!tm || !tm->isFunction()){
                runerror("Expected call metamethod at call object!");
            }
            //讲func及以上的所有参数上移一个单位,空出一个位置,然后讲func的位置赋值tm
            for(int p=top_;p>func;p--)
                stack_[p] = stack_[p-1];
            top_++;
            stack_[func] = *tm;
            //最后precall
            return precall(func, nresults);

        }

    }
    return 1;
}

/**
 *  这个函数用来处理可变参数的情况
 *  此时func到top之间有所有的参数都要尝试传递，即vararg，而该func需要的参数个数为nparams，即nfixargs。
 *  确定的个数。设置fixed为func的下一个位置，即第一个参数的位置，base为新的frame栈底。
 *  下面进行拷贝，讲参数拷贝nparams个到base上去，如果nparams < actual,那么已经传递完了，否则需要添加nil
 */
int LuaState::adjustVararg(Proto *proto, int actual) {
    int base, fixed;
    int nfixargs = proto->nparams_;
    fixed = top_ - actual;
    base = top_;
    int i=0;
    for(;i < nfixargs && i< actual; i++){
        push(stack_[fixed+i]);
        stack_[fixed+i] = Value();
    }
    for(;i<nfixargs;i++){
        pushnil();
    }

    return base;

}

//返回当前callinfo的下一个callinfo，如果为null那么新建一个,同时更新当前的ci
CallInfo * LuaState::nextCallInfo() {

    if(ci_->next){
        ci_ = ci_->next;
    }else{
        //创建一个新的callinfo然后加入到链表中
        ci_ = ExtendCallInfo();
    }

    return ci_;
}


int LuaState::poscall(CallInfo *ci, int firstResult, int nres) {
    int wanted = ci->nresult;
    int res = ci->func;
    ci_ = ci->previous;
    //auto base = ci_? ci_->base:0;
    return moveResults(firstResult, res, nres, wanted);
}

/**
 * 从from开始的nres个value移动到to开始的nwanted个位置,最后要更新top,分情况讨论
 * 1、wanted = 0,即不需要返回值，那么直接退出
 * 2、wanted = 1, 需要1个返回值，只需要赋值1个
 * 3、wanted = -1,此时返回多个值，即有多少个返回多少个
 * 4、其他情况下根据nres 和nwanted的值确定，wanted较多需要补充nil，否则丢弃
 *
 * 返回值为0表示尽可能返回多的，否则为1表示返回固定的nwanted的值
 */
int LuaState::moveResults(int from, int to, int nres, int nwanted) {
    switch (nwanted) {
        case 0: break;
        case 1:{
            //返回一个结果
            if(nres == 0) //如果没有结果设置为nil
                stack_[from] = Value();
            stack_[to] = stack_[from];
            break;
        }
        case -1:{ //-1表示多个返回值
            for(int i=0;i<nres;i++){
                stack_[to+i] = stack_[from+i];
            }
            top_ = to + nres;
            return 0;
        }
        default:{
            int i;
            if(nwanted <= nres){
                //如果需要的比实际的要多
                for(i=0;i<nwanted;i++)
                    stack_[to+i] = stack_[from+i];
            }else{
                //需要的多，那么用nil补充
                for(i=0;i<nres;i++)
                    stack_[to+i] = stack_[from+i];
                for(;i<nwanted;i++)
                    stack_[to+i] = Value();
            }
            break;
        }
    }

    top_ = to + nwanted;
    return 1;
}

/**
 * call一个函数,lua 或则C的,func是调用函数所在栈的idx,nresult是期待的返回值
 * @param func
 * @param nresult
 */
void LuaState::call(int func, int nresult) {
    if(!precall(func, nresult)){
         vm_->Execute();
    }
}

/**
 * 保护的调用一个函数
 * @param func 函数的idx
 * @param nresult 期望的结果个数
 * @return 返回当前的status
 */
int LuaState::pcall(int func, int nresult) {
    int status = LUA_OK;
    auto oldCallInfo = ci_;
    //准备创建新的frame 然后call
    try{
        call(func, nresult);
    } catch (int errcode) {
        status = errcode;
    }

    //如果出错status会返回errcode,同时此时栈顶的值为errmsg
    if(status !=LUA_OK){
        int oldtop = oldCallInfo->top;
        closeUpvalues( stack_ + oldtop);
        //讲top的值赋值给oldtop
        stack_[oldtop] = stack_[top_-1];
        top_ = oldtop + 1;
        ci_ = oldCallInfo;
        //todo shrinkstack
    }
    return status;
}

/**
 * 在当前栈中的openupvalue链表中查找是有upval指向idx处的值
 * @param 指向的idx
 * @return 没有返回nullptr
 */
LuaUpvalue * LuaState::findUpval(Value* val) {
    //if(!openlist_)
    //    return nullptr;

    LuaUpvalue* cur = openlist_;
    while(cur != nullptr){
        if(cur->val() == val){
            return cur;
        }
        cur = cur->next();
    }

    //没有的话创建一个新的upvalue指向val,然后更改list
    auto newUpValue = LuaUpvalue::CreateUpvalue(val, openlist_);
    openlist_ = newUpValue;
    return newUpValue;
}

/**
 * 关闭 level 到top之间的upvalues
 * @param level
 */
void LuaState::closeUpvalues(Value* level) {
    LuaUpvalue* cur = openlist_;

    while(cur!= nullptr && cur->val() >=level){
        //close cur
        //先保存next，因为close后next字段就不存在了
        auto next = cur->next();
        cur->close();
        openlist_ = next;
        cur = openlist_;
    }
}

LuaState * LuaState::CreateLuaState() {
    auto obj = GlobalState::gc->createObject(LUA_TTHREAD, sizeof(LuaState));
    auto state = static_cast<LuaState*>(obj);
    state->stack_ =(Value*) Allocator::Alloc(BASIC_STACK_SIZE * sizeof(Value));
    state->stackSize_ = BASIC_STACK_SIZE;
    state->top_ = 0;
    state->lastFree_ = state->top_ + state->stackSize_ - EXTRA_STACK;

    state->baseCi_ = CallInfo::CreateCallInfo();
    state->ci_ = state->baseCi_;
    state->openlist_ = nullptr;

    return state;
}

CallInfo * CallInfo::CreateCallInfo() {
    auto ci = (CallInfo*)Allocator::Alloc(sizeof(CallInfo));
    ci->func=0;  //funcidx
    ci->top=0;   //top idx
    ci->base=0;
    ci->previous= nullptr;
    ci->next= nullptr;
    ci->nresult=0;
    ci->status=0;
    return ci;
}

//遍历自己
void LuaState::markSelf() {
    makked_ = GC_BLACK;
    for(int i=0;i<top_;i++){
        stack_[i].markSelf();
    }

    //处理upvalue
    LuaUpvalue* cur = openlist_;
    while(cur != nullptr){
        cur->markSelf();
        cur = cur->next();
    }

}
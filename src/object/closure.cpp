//
// Created by gpf on 2020/12/16.
//

#include "closure.h"
#include "memory/GC.h"
#include "vm/globalState.h"
#include "proto.h"


/**
 * 创建一个新的luaclosure,
 * @param n upvalue的数量
 * @return
 */
LuaClosure * LuaClosure::CreateLuaClosure(int n, Proto* p) {
    auto obj = GlobalState::gc->createObject(LUA_TLCL, sizeof(LuaClosure));
    auto lc = static_cast<LuaClosure*>(obj);
    lc->upvals_ = Array<LuaUpvalue*>(n, nullptr);
    lc->proto_ = p;
    return lc;
}

void LuaClosure::DestroyLuaClosure(LuaClosure *cl) {
    cl->upvals_.Destroy();
    Allocator::Free(cl, sizeof(LuaClosure));
}

CClosure * CClosure::CreateCClosure(Cfunction f) {
    auto obj = GlobalState::gc->createObject(LUA_TCCL, sizeof(CClosure));
    auto cc = static_cast<CClosure*>(obj);
    cc->func_ = f;
    return cc;
}

void CClosure::DestroyCClosure(CClosure *c) {
    Allocator::Free(c, sizeof(CClosure));
}

void LuaClosure::markSelf() {
    type_ = GC_BLACK;
    for(int i=0;i<upvals_.len();i++){
        auto upval = upvals_[i];
        if(upval){
            upval->markSelf();
        }
    }
    proto_->markSelf();
}
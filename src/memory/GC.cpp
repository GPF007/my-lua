//
// Created by gpf on 2020/12/8.
//

#include "GC.h"
#include "object/Value.h"

#include "vm/globalState.h"
#include "object/luaTable.h"
#include "object/closure.h"
#include "object/proto.h"
#include "vm/luaState.h"

GC::GC() {
    begin_ = new GCObject;
    cur_ = begin_;
}

//创建一个object，大小为size ,初始状态为未扫描状态的
GCObject * GC::createObject(int tag, size_t size) {
    GCObject* obj = reinterpret_cast<GCObject*>(Allocator::Realloc(nullptr,0, size));
    obj->makked_ = GC_WHITE;
    appendObject(obj);
    return obj;
}

/**
 * 首先是mark阶段
 */
void GC::mark() {
    //准备扫描root，将所有活跃的对象标记为black
    LuaTable* GTable = GlobalState::globalTable;
    LuaState* mainThread = GlobalState::mainState;
    //两个root分别是globaltable 和 mainthread
    GTable->markSelf();
    mainThread->markSelf();

}

void GC::sweep() {
    //第一个节点是空节点
    auto fh = begin_;
    while(fh ->next_ != nullptr){
        auto tmp = fh->next_;
        //如果是黑色说明已经被引用，因此不用删除
        if(tmp->makked_ == GC_BLACK){
            tmp->makked_ = GC_WHITE;
        }else{
            fh->next_ = tmp->next_;
            //删除该对象
            sweepObject(tmp);
        }
        fh = fh->next_;
    }
}

void GC::sweepObject(GCObject *obj) {
    switch (obj->type_) {
        case LUA_TLNGSTR:   LuaString::DestroyLongString(static_cast<LuaString*>(obj)); break;
        case LUA_TTHREAD:    LuaState::DestroyLuaState(static_cast<LuaState*>(obj));     break;
        case LUA_TTABLE:    LuaTable::DestroyTable(static_cast<LuaTable*>(obj));        break;
        case LUA_TLCL:      LuaClosure::DestroyLuaClosure(static_cast<LuaClosure*>(obj));   break;
        case LUA_TCCL:      CClosure::DestroyCClosure(static_cast<CClosure*>(obj));     break;
        case LUA_TPROTO:    Proto::DestroyProto(static_cast<Proto*>(obj));              break;

        default:{
            throw std::runtime_error("Unkwown type for sweep!");
        }
    }
}

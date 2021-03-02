//
// Created by gpf on 2020/12/20.
//

#include "runner.h"
#include "dump/undump.h"
#include "vm/luaState.h"
#include "vm/meta.h"
#include "object/luaTable.h"

Runner::Runner() {
    vm_ = std::make_unique<VM>();
}

void Runner::RunClosure(const char *fname) {
    auto proto = Undumper::Undump(fname);
    auto mainState = LuaState::CreateLuaState();
    auto mainClosure = LuaClosure::CreateLuaClosure(3, proto);
    GlobalState::SetMainthread(mainState);

    //初始化第一个upvalue
    auto registryTable = static_cast<LuaTable*>(GlobalState::registry.obj);
    auto upval = LuaUpvalue::CreateUpvalue(const_cast<Value *>(registryTable->getInt(2)), nullptr);
    mainClosure->setUpvalue(0, upval);

    vm_->SetState(mainState);
    GlobalState::meta->SetState(mainState);

    vm_->CallMainClosure(mainClosure);

}

void Runner::RunClosure(Proto *proto) {
    auto mainState = LuaState::CreateLuaState();
    auto mainClosure = LuaClosure::CreateLuaClosure(3, proto);
    GlobalState::SetMainthread(mainState);

    //初始化第一个upvalue
    auto registryTable = static_cast<LuaTable*>(GlobalState::registry.obj);
    auto upval = LuaUpvalue::CreateUpvalue(const_cast<Value *>(registryTable->getInt(2)), nullptr);
    mainClosure->setUpvalue(0, upval);

    vm_->SetState(mainState);
    GlobalState::meta->SetState(mainState);

    vm_->CallMainClosure(mainClosure);
}

Runner::~Runner() {
}
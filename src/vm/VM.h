//
// Created by gpf on 2020/12/17.
//

#ifndef LUUA2_VM_H
#define LUUA2_VM_H

#include "object/closure.h"

class LuaState;
class Proto;

class VM {

public:
    VM(){}
    void Execute();
    void Start(const char* fname);
    void CallMainClosure(LuaClosure* closure);
    void SetState(LuaState* s);

private:

    LuaState* state_;


private:
    void add(const Value& a, const Value& b, int idres);
    void sub(const Value& a, const Value& b, int idres);
    void mul(const Value& a, const Value& b, int idres);
    void div(const Value& a, const Value& b, int idres);
    void len(const Value& obj, int idres);
    int eq(const Value& left, const Value& right);
    int lt(const Value& left, const Value& right);
    int le(const Value& left, const Value& right);
    void pushclosure(Proto* p, Array<LuaUpvalue*>& prevUpvals, int base, int ra);
    void getTable(const Value& obj, int idxres, const Value& key);
    void setTable(const Value& obj, const Value& key, const Value& value);
    void concate(int total);




};


#endif //LUUA2_VM_H

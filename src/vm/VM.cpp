//
// Created by gpf on 2020/12/17.
//

#include "VM.h"
#include "luaState.h"
#include "object/proto.h"
#include "object/luaTable.h"
#include "opcodes.h"
#include "globalState.h"
#include "meta.h"
#include <math.h>
#include <fmt/core.h>

#define KMASK  (1<<8)

#define LFIELDS_PER_FLUSH 50
#define MAXTAGLOOP 2000


namespace {
    template<typename... Args>
    void debug(Args... args){
#ifdef DEBUG
        fmt::print(args...);
        fmt::print("\n");
#endif
    }

};


void VM::Execute() {
    auto ci = state_->ci_;
    ci->status |= CIST_FRESH;
    LuaClosure* luaClosure;

    newframe:
    luaClosure = static_cast<LuaClosure*>(state_->at(ci->func).obj);
    auto proto = luaClosure->proto();
    auto& constants = proto->consts();//constants is an array reference
    int base = ci->base;
    LuaState* state = state_;

    //A函数返回当前ins的A所在的idx
    auto A = [base](Instruction ins)->int {
        return base +  ByteCode::getArgA(ins);
    };

    auto B = [base](Instruction ins)->int {
        return base + ByteCode::getArgB(ins);
    };

    auto C = [base](Instruction ins)->int {
        return  base +  ByteCode::getArgC(ins);
    };

    auto RA = [base,state](Instruction ins)->Value& {
        return state->at(base + ByteCode::getArgA(ins));
    };

    auto RB = [base,state](Instruction ins)->Value& {
        return state->at(base + ByteCode::getArgB(ins));
    };


    auto RKB =[base, constants, state](Instruction ins)->Value&{
        int idx = ByteCode::getArgB(ins);
        return (idx & KMASK)?constants[idx & ~KMASK]:state->at(base + idx);
    };

    auto RKC =[base, constants, state](Instruction ins)->Value&{
        int idx = ByteCode::getArgC(ins);
        return (idx & KMASK)?constants[idx & ~KMASK]:state->at(base + idx);
    };

    auto Kst= [constants](int idx) -> Value&{
        return constants[idx];
    };

    auto R = [base, state](int idx) ->Value&{
        return state->at(base + idx);
    };

    auto RK = [](int idx) ->int{
        return (idx & KMASK)?idx & ~KMASK:idx;
    };

    int i=0;
    //interpreter
    for(;;){
        ++i;
        auto ins = ci->fetch();
        switch (ByteCode::getOpCode(ins)) {
            default:{
                state_->runerror("Unkwnon opcode!");
                break;
            }
            case ByteCode::OP_MOVE:{
                //R(A) = R(B) 把idx为b的值复制到A处 解引用得到了Value，可以直接赋值
                debug("[{}] MOVE {} <= {}",i, ByteCode::getArgA(ins), ByteCode::getArgB(ins));
                RA(ins) = RB(ins);
                break;
            }
            case ByteCode::OP_LOADK:{
                //R(A) := Kst(Bx) 从常亮数组加载一个常亮赋值给 RA的值)
                debug("[{}] LOADK stack[{}] <= Const[{}]",i, ByteCode::getArgA(ins), ByteCode::getArgB(ins));
                RA(ins) = Kst(ByteCode::getArgBx(ins));
                break;
            }
            case ByteCode::OP_LOADKX:{
                //R(A) := Kst(extra arg) 此时下一条指令一定是iAx指令，用后面的Ax来代替索引
                debug("[{}] LOADKX",i);
                auto nextIns = ci->fetch();
                RA(ins) = Kst(ByteCode::getAx(nextIns));
                break;
            }
            case ByteCode::OP_LOADBOOL:{
                //R(A) := (Bool)B; if (C) pc++ （注意，这里是b，r(b)）
                //首先给RA赋值，然后根据C的值来判断是否要pc++
                debug("[{}] LOADBOOL {} {} {}",i, ByteCode::getArgA(ins), ByteCode::getArgB(ins), ByteCode::getArgC(ins));
                RA(ins) = Value(static_cast<LuaByte>(ByteCode::getArgB(ins)));
                if(ByteCode::getArgC(ins))
                    ci->incpc();
                break;
            }
            case ByteCode::OP_LOADNIL:{
                //R(A), R(A+1), ..., R(A+B) := nil
                //从R(A)开始到R(A+B) 设置为nil
                int a = ByteCode::getArgA(ins);
                debug("[{}] LOADNIL {} ~ {}",i, a, a+ByteCode::getArgB(ins));
                for(int i=0;i<=ByteCode::getArgB(ins); i++)
                    R(a+i) = *kNil;
                break;
            }

            case ByteCode::OP_GETUPVAL:{
                //R(A) := UpValue[B]
                //讲upvalu[b]赋值给 ra ,这个upvalue是从closure中得到的
                debug("[{}] GETUPVAL R[{}]  = Upvalue[{}]",i, ByteCode::getArgA(ins), ByteCode::getArgB(ins));
                RA(ins) = *luaClosure->getUpvalue(ByteCode::getArgB(ins))->val();

                break;
            }


            case ByteCode::OP_GETTABUP:{
                //R(A) := UpValue[B][RK(C)]
                //upvalue[b]的值是一个表，然后从表中的到rkc的值
                //首先访问upvalue得到一个Value& 然后得到它的gc部分，在转化为表
                debug("[{}] GETTABUP R[{}]  = Upvalue[{}][{}]",i, ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                        RK(ByteCode::getArgC(ins)));


                getTable(*luaClosure->getUpvalue(ByteCode::getArgB(ins))->val(), A(ins), RKC(ins));

                break;
            }
            case ByteCode::OP_GETTABLE:{
                //R(A) := R(B)[RK(C)]
                //R(B)处的value是一个表,然后访问表赋值给a
                debug("[{}] GETTABLE R[{}]  = R({})[{}]",i, ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                      RK(ByteCode::getArgC(ins)));
                getTable(RB(ins), A(ins), RKC(ins));
                break;
            }

            case ByteCode::OP_SETUPVAL:{
                //UpValue[B] := R(A)
                //讲R(A)赋值给UPVALUE[B] ,这个upvalue是从closure中得到的
                debug("[{}] SETUPVAL Upvalue[{}] = R({})",i, ByteCode::getArgB(ins),ByteCode::getArgA(ins));
                auto upval = luaClosure->getUpvalue(ByteCode::getArgB(ins));
                upval->setVal(RA(ins));
                break;
            }
            case ByteCode::OP_SETTABUP:{
                //UpValue[A][RK(B)] := RK(C)
                debug("[{}] SETTABUP Upvalue[{}][{}] = RK({})",i, ByteCode::getArgB(ins),ByteCode::getArgA(ins),
                        RK(ByteCode::getArgC(ins)));

                setTable(*luaClosure->getUpvalue(ByteCode::getArgA(ins))->val(), RKB(ins), RKC(ins));

                break;
            }
            case ByteCode::OP_SETTABLE:{
                //R(A)[RK(B)] := RK(C)
                //R(A)是一个表，插入一个对
                debug("[{}] SETABLE R({})[{}] = RK({})",i, ByteCode::getArgA(ins),RK(ByteCode::getArgB(ins)),
                      RK(ByteCode::getArgC(ins)));
                setTable(RA(ins), RKB(ins), RKC(ins));
                //auto tbl = static_cast<Table*>(RA(ins).gc);
                //tbl->Set(RKB(ins), RKC(ins));
                break;
            }


            case ByteCode::OP_NEWTABLE:{
                //R(A) := {} (size = B,C)
                //创建一个表，两部分大小分别是BC，然后存入A
                debug("[{}] NEWTABLE  R({}) <= Size({}, {})", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                        ByteCode::getArgC(ins));
                int b = ByteCode::getArgB(ins);
                int c = ByteCode::getArgC(ins);
                auto tbl = LuaTable::CreateTable();
                if(b!=0 || c!=0)
                    tbl->resize(b,c);
                RA(ins) = Value(tbl, LUA_TTABLE);
                break;
            }

            case ByteCode::OP_SELF:{
                //R(A+1) := R(B); R(A) := R(B)[RK(C)]
                //用来支持面向对象的指令,R(B)一定是一个表，RK(C)一定是个string类型的
                debug("[{}] SELF {}, {}, {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                      ByteCode::getArgC(ins));
                R(ByteCode::getArgA(ins)+1) = RB(ins);

                //auto tbl = static_cast<LuaTable*>(R(B(ins)).obj);
                auto& key = RKC(ins);
                ASSERT(key.type == LUA_TSTRING);
                //用gettable
                getTable(RB(ins), A(ins),RKC(ins));
                break;
            }

                //下面是一些算数指令
            case ByteCode::OP_ADD:{
                //R(A) := RK(B) + RK(C)
                debug("[{}] ADD {}, {}, {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                      ByteCode::getArgC(ins));
                add(RKB(ins), RKC(ins), A(ins));
                break;
            }
            case ByteCode::OP_SUB:{
                //R(A) := RK(B) + RK(C)
                //RA(ins) = RKB(ins) - RKC(ins);
                debug("[{}] SUB {}, {}, {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                      ByteCode::getArgC(ins));
                sub(RKB(ins), RKC(ins), A(ins));
                break;
            }
            case ByteCode::OP_MUL:{
                //R(A) := RK(B) + RK(C)
                debug("[{}] MUL {}, {}, {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                      ByteCode::getArgC(ins));
                mul(RKB(ins), RKC(ins), A(ins));
                break;
            }
            case ByteCode::OP_DIV:{
                //R(A) := RK(B) + RK(C)
                debug("[{}] DIV {}, {}, {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                      ByteCode::getArgC(ins));
                div(RKB(ins), RKC(ins), A(ins));
                break;
            }

                //一些位运算的指令，只能用于integer类型的
            case ByteCode::OP_BAND:{
                //R(A) := RK(B) & RK(C)
                //RA(ins) = Value(RKB(ins).ival & RKC(ins).ival);
                debug("[{}] BAND {}, {}, {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                      ByteCode::getArgC(ins));
                div(RKB(ins), RKC(ins), A(ins));
                break;
            }
            case ByteCode::OP_BOR:{
                //R(A) := RK(B) | RK(C)
                debug("[{}] BOR {}, {}, {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                      ByteCode::getArgC(ins));
                RA(ins) = Value(RKB(ins).ival | RKC(ins).ival);
                break;
            }
            case ByteCode::OP_BXOR:{
                //R(A) := RK(B) ^ RK(C)
                debug("[{}] BXOR {}, {}, {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                      ByteCode::getArgC(ins));
                RA(ins) = Value(RKB(ins).ival ^ RKC(ins).ival);
                break;
            }
            case ByteCode::OP_SHL:{
                //R(A) := RK(B) << RK(C)
                debug("[{}] SHL {}, {}, {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                      ByteCode::getArgC(ins));
                RA(ins) = Value(RKB(ins).ival << RKC(ins).ival);
                break;
            }
            case ByteCode::OP_SHR:{
                //R(A) := RK(B) >> RK(C)
                debug("[{}] SHR {}, {}, {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                      ByteCode::getArgC(ins));
                RA(ins) = Value(RKB(ins).ival >> RKC(ins).ival);
                break;
            }
            case ByteCode::OP_MOD:{
                //暂时不考虑double的
                //R(A) := RK(B) % RK(C)
                debug("[{}] MOD {}, {}, {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                      ByteCode::getArgC(ins));
                RA(ins) = Value(RKB(ins).ival % RKC(ins).ival);
                break;
            }
            case ByteCode::OP_IDIV:{
                //R(A) := RK(B) // RK(C) floor divition
                debug("[{}] IDIV {}, {}, {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                      ByteCode::getArgC(ins));
                RA(ins) = Value(RKB(ins).ival / RKC(ins).ival);
                break;
            }
            case ByteCode::OP_POW:{
                //R(A) := RK(B) ^ RK(C)
                debug("[{}] POW {}, {}, {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                      ByteCode::getArgC(ins));
                RA(ins) = Value((LuaNumber)pow(RKB(ins).nval, RKC(ins).nval));
                break;
            }
            case ByteCode::OP_UNM:{
                //R(A) := -R(B)
                //RA(ins) = RB(ins).uminus();
                break;
            }
            case ByteCode::OP_BNOT:{
                //R(A) := ~R(B)
                debug("[{}] BNOT R({}) <= ~ R({})", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins));
                RA(ins)= Value(~  RB(ins).ival);
                break;
            }
            case ByteCode::OP_NOT:{
                //R(A) := not R(B)
                //(ttisnil(o) || (ttisboolean(o) && bvalue(o) == 0))
                debug("[{}] NOT R({}) <= ! R({})", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins));
                auto& val = RB(ins);
                RA(ins) = Value(val.isFalse());
                break;
            }
            case ByteCode::OP_LEN:{
                //R(A) := length of R(B)
                debug("[{}] LEN R({}) <= len(R({}))", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins));
                len(RB(ins), A(ins));
                break;
            }

                //一些跳转指令
            case ByteCode::OP_JMP:{
                //pc+=sBx; if (A) close all upvalues >= R(A - 1)
                //首先设置pc的值，然后根据A决定是否关闭upvalues
                debug("[{}] JMP {}  to {}", i,ByteCode::getArgA(ins), ByteCode::getArgsBx(ins)+ByteCode::getArgA(ins));
                ci->savedpc += ByteCode::getArgsBx(ins);
                auto a = ByteCode::getArgA(ins);
                if(a!=0)
                    state->closeUpvalues(&R(a+base -1));
                break;
            }
            case ByteCode::OP_EQ:{
                //if ((RK(B) == RK(C)) ~= A) then pc++
                //首先得到RK(B)和RK(C)的值，然后判断你是否相等得到一个bool值，
                //将该bool值与A比较,如果不等则pc++，即会跳过下一条指令，
                //因为下一条指令一定是跳转指令
                debug("[{}] EQ (RK({}) == RK({})~= {} )?", i,ByteCode::getArgB(ins), ByteCode::getArgC(ins),
                        ByteCode::getArgA(ins));
                auto& rb = RKB(ins);
                auto& rc = RKC(ins);


                if(eq(rb,rc) != ByteCode::getArgA(ins))
                    ci->savedpc++;
                break;
            }
            case ByteCode::OP_LT:{
                //if ((RK(B) <  RK(C)) ~= A) then pc++
                //类似与上面的
                debug("[{}] LT (RK({}) < RK({})~= {} )?", i,ByteCode::getArgB(ins), ByteCode::getArgC(ins),
                      ByteCode::getArgA(ins));
                auto& rb = RKB(ins);
                auto& rc = RKC(ins);
                //int tmpb = ByteCode::getArgB(ins);
                //int tmpc = ByteCode::getArgC(ins);

                if(lt(rb , rc) != ByteCode::getArgA(ins))
                    ci->savedpc++;
                break;
            }
            case ByteCode::OP_LE:{
                //if ((RK(B) <=  RK(C)) ~= A) then pc++
                //类似与上面的
                debug("[{}] LE (RK({}) <= RK({})~= {} )?", i,ByteCode::getArgB(ins), ByteCode::getArgC(ins),
                      ByteCode::getArgA(ins));
                auto& rb = RKB(ins);
                auto& rc = RKC(ins);
                if(le(rb, rc) != ByteCode::getArgA(ins))
                    ci->savedpc++;
                break;
            }
            case ByteCode::OP_TEST:{
                //if not (R(A) <=> C) then pc++
                //首先判断RA和c是否有相同的bool，如果没有那么跳转
                debug("[{}] TEST (R({}) <=> {})?", i,ByteCode::getArgA(ins), ByteCode::getArgC(ins));
                bool boola = !(RA(ins).isFalse());
                bool boolc = ByteCode::getArgC(ins);
                if(boola != boolc)
                    ci->savedpc++;
                break;
            }
            case ByteCode::OP_TESTSET:{
                //if (R(B) <=> C) then R(A) := R(B) else pc++
                //首先判断rb和c是否有相同的bool值，如果有那么进行赋值，否则pc++
                debug("[{}] TESTTEST (R({}) <=> {}) A={}", i,ByteCode::getArgB(ins), ByteCode::getArgC(ins),
                        ByteCode::getArgA(ins));
                //int b = ByteCode::getArgB(ins);
                //auto t1 = RB(ins).isFalse();
                //auto t2 = ByteCode::getArgC(ins)!=0;
                if(!(RB(ins).isFalse()) == (ByteCode::getArgC(ins)!=0))
                    RA(ins) = RB(ins);
                else
                    ci->savedpc++;
                break;
            }

            case ByteCode::OP_CONCAT:{
                //R(A) := R(B).. ... ..R(C)
                debug("[{}] CONCAT {} {} {}", i,ByteCode::getArgA(ins),ByteCode::getArgB(ins),
                        ByteCode::getArgC(ins));
                int b = ByteCode::getArgB(ins);
                int c = ByteCode::getArgC(ins);
                state_->top_ = base + c +1;// 更新top
                concate(c-b+1);

                //讲栈顶的位置(rb)赋值给ra
                RA(ins) = state_->at(base + b);
                //回复top
                state_->top_ = ci->top;
                break;
            }


                //call
            case ByteCode::OP_CALL:{
                //R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1))
                //因为参数的范围是R(A+1) ~ R(A+B-1) 那么下一个可用的位置就是R(A+B)
                // 如果b>0说明传入参数的个数是固定的，那么需要预留b个位置，更新全局栈顶
                //auto tmpclosure = static_cast<LuaClosure*>(state_->at(4).obj);
                int b = ByteCode::getArgB(ins);
                int nresults = ByteCode::getArgC(ins) -1;
                auto raidx = ByteCode::getArgA(ins) + base;
                if(b!=0)
                    state_->top_ = raidx + b;
                debug("[{}] CALL {} {} {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                        ByteCode::getArgC(ins));
                //返回0说明是luacall
                bool isLuaCall = state_->precall(raidx, nresults) == 0;
                if(isLuaCall){
                    ci = state_->ci_;
                    goto newframe;
                }else{//c call
                    //此时已经调用完了，结果就在站上面
                    if(nresults >=0 ){ //base 和 top
                        state_->top_ = ci->top;
                        base = ci->base;
                    }
                }
                break;
            }
            case ByteCode::OP_RETURN:{
                //return R(A), ... ,R(A+B-2)
                // b-1 个结果
                debug("[{}] RETURN R({}) ~ R({})", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins)+
                      ByteCode::getArgC(ins)-2);
                int b = ByteCode::getArgB(ins);

                //如果该closure存在subproto， 那么关闭当前栈上面的所有闭包，因为它的子函数可能引用了当前栈的函数，必须要关闭他们
                if(luaClosure->proto()->nsubs())
                    state_->closeUpvalues(&R(0));

                auto raidx = A(ins); //base + Bytecode::getarga(ins)
                //如果b！=0说明返回b-1个参数，否则返回所有
                int nres = (b!=0)?b-1: static_cast<int>(state_->top_ - raidx);
                b = state_->poscall(ci, raidx, nres);
                //如果是第一次调用的话直接return，否则返回到上一个栈中执行哟

                if(ci->status & CIST_FRESH)
                    return;
                else{
                    //update
                    ci = state_->ci_;
                    if(b)//如果b不为0表示固定结果的res，要更新到前一个栈顶
                        state_->top_ = ci->top;
                    goto newframe;
                }
            }

            case ByteCode::OP_VARARG:{
                //R(A), R(A+1), ..., R(A+B-2) = vararg
                //A为起始寄存器，B代表个数,用call来实现
                debug("[{}] VARARG {} {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins));
                int b = ByteCode::getArgB(ins)-1;
                int n = base - ci->func - luaClosure->proto()->nparams() - 1;
                if(n<0) //argument 比 parameter少
                    n=0;
                if(b<0) {//如果 b<0说明接受所有的参数
                    b = n;
                    state_->checkStack(n);
                    state_->top_ = ByteCode::getArgA(ins) + base + n;
                }
                //如果b不是小于0的或则传递b个参数
                int j;
                int ra = ByteCode::getArgA(ins) + base;
                for(j=0;j<b && j< n;j++){

                    state_->at(ra+j) = state_->at(base - n + j);
                }

                for(;j<b;j++)
                    state_->at(ra+j)=*kNil;
                break;
            }

            case ByteCode::OP_TAILCALL:{
                //return R(A)(R(A+1), ... ,R(A+B-1))
                debug("[{}] TAILCALL {} {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins));
                int b = ByteCode::getArgB(ins);
                int raidx = ByteCode::getArgA(ins) + base;
                if(b!=0)//如果b不为0，那么新的top为参数的后一个位置
                    state_->top_ = ByteCode::getArgA(ins) + base + b;

                if(state_->precall(raidx, -1)){
                    //如果是cclosure，直接返回并更新update base
                    base = ci->base;
                }else{//lua closure
                    //cur是被调用函数的callinfo，prev是前一个函数的callinfo
                    auto cur = state_->ci_;
                    auto prev = cur->previous;
                    auto curFunc = cur->func;
                    auto prevFunc = prev->func;
                    auto lim = cur->base + static_cast<LuaClosure*>(state_->at(curFunc).obj)->proto()->nparams();
                    if(luaClosure->proto()->nsubs())
                        state_->closeUpvalues(&R(base));
                    int aux;
                    for(aux = 0; curFunc+aux<lim; aux++)
                        state_->at(prevFunc + aux) = state_->at(curFunc + aux);
                    //更新前一个栈的base和top
                    prev->base = prevFunc + (cur->base - curFunc);
                    prev->top  = state->top_ = prevFunc + (state_->top_ - curFunc);
                    prev->savedpc = cur->savedpc;
                    prev->status |= CIST_TAIL;
                    ci = state->ci_ = prev;
                    goto newframe;
                }
                break;
            }

            case ByteCode::OP_FORLOOP:{


                //R(A)+=R(A+2);
                // if R(A) <?= R(A+1) then { pc+=sBx; R(A+3)=R(A)}
                //首先迭代一次r(a)，如果r(a)的<=r(a+1),那么说明还在循环中，那么跳转到sbx处，同时
                //讲r(a)拷贝给用户变量 r(a+3)
                // <?表示如果步长为整数为:<= ,负数是 >=
                debug("[{}] FORLOOP {}", i,ByteCode::getArgA(ins));
                auto& iter = RA(ins);
                if(iter.isInteger()){//integer loop
                    auto step = R(ByteCode::getArgA(ins) + 2).ival;
                    auto idx = RA(ins).ival + step;
                    auto limit = R(ByteCode::getArgA(ins) + 1).ival;
                    bool flag = (step<0)?(limit<=idx):(idx<=limit);
                    if(flag){//跳转到循环开始处
                        ci->savedpc += ByteCode::getArgsBx(ins);
                        //ra = newidx
                        RA(ins).ival = idx;
                        R(ByteCode::getArgA(ins)+3) = Value(idx);
                    }
                }else{//浮点数的循环
                    auto step = R(ByteCode::getArgA(ins) + 2).nval;
                    auto idx = RA(ins).nval + step;
                    auto limit = R(ByteCode::getArgA(ins)+1).nval;
                    bool flag = (step<0)?(limit<=idx):(idx<=limit);
                    if(flag){//跳转到循环开始处
                        ci->savedpc += ByteCode::getArgsBx(ins);
                        //ra = newidx
                        RA(ins).nval = idx;
                        R(ByteCode::getArgA(ins)+3) = Value(idx);
                    }
                }
                break;
            }

            case ByteCode::OP_FORPREP:{
                //R(A)-=R(A+2); pc+=sBx
                //初始化循环的初始值，并跳转到内部去执行
                debug("[{}] FORLOOP {} {}", i,ByteCode::getArgA(ins), ByteCode::getArgsBx(ins));
                auto a = ByteCode::getArgA(ins);
                auto& init = R(a);
                auto& limit = R(a+1);
                auto& step = R(a+2);
                //如果都是integer类型的

                if(init.isInteger() && limit.isInteger() && step.isInteger()){
                    init.ival -= step.ival;
                }else{
                    //panic("not integer!");
                    //尝试转化为浮点类型
                    auto finit = init.convertToNumber();
                    if(finit.isNil())
                        state->runerror("'for' init must be a number");
                    auto flimit = limit.convertToNumber();
                    if(flimit.isNil())
                        state->runerror("'for' limit must be a number");
                    auto fstep = step.convertToNumber();
                    if(fstep.isNil())
                        state->runerror("'for' step must be a number");
                    //这里会把init转化为float类型的
                    init = Value(finit.nval - finit.nval);
                }
                ci->savedpc += ByteCode::getArgsBx(ins);
                break;
            }

            case ByteCode::OP_CLOSURE:{
                //R(A) := closure(KPROTO[Bx])
                //创建一个closure加入到stack
                debug("[{}] CLOSURE R({}) <= Protos[{}]", i,ByteCode::getArgA(ins), ByteCode::getArgBx(ins));
                auto p = luaClosure->proto()->subProtoAt(ByteCode::getArgBx(ins));

                pushclosure(p, luaClosure->upvals(), base, ByteCode::getArgA(ins) + base);
                break;
            }

            case ByteCode::OP_SETLIST:{
                //R(A)[(C-1)*FPF+i] := R(A+i), 1 <= i <= B
                //R(A)是一个表，
                //当表构造器的最后一个元素是函数调用或者是vararg表达式的时候，b=0，表示收集所有元素s
                debug("[{}] SETLIST {} {} {}", i,ByteCode::getArgA(ins), ByteCode::getArgB(ins),
                        ByteCode::getArgC(ins));
                auto n = ByteCode::getArgB(ins);
                auto c = ByteCode::getArgC(ins);
                if(n==0)
                    n = state_->top_ - (ByteCode::getArgA(ins) + base) -1;
                LuaInteger last = ((c-1)* LFIELDS_PER_FLUSH) + n;
                LuaInteger begin = ((c-1)* LFIELDS_PER_FLUSH);
                auto a = ByteCode::getArgA(ins);
                auto tbl = static_cast<LuaTable*>(RA(ins).obj);

                if(last > tbl->sizearray())
                    tbl->resize(last, tbl->sizenode());

                for(int i=1;i<=n;i++){
                    tbl->setInt(++begin, &R(a+i));
                }
                break;
            }

            case ByteCode::OP_TFORCALL:{
                //R(A+3), ... ,R(A+2+C) := R(A)(R(A+1), R(A+2));
                //这个指令会调用一次通用的next函数
                // R(A)为函数，R(A+1)为需要迭代的表，R(A+2)为键
                // R(A+3), ... R(a+2+c) =  next(t,k)
                int ra = A(ins);
                int cb = ra + 3; //结果存放的位置
                state_->at(cb +2) = state_->at(ra+2);
                state_->at(cb +1) = state_->at(ra+1);
                state_->at(cb ) = state_->at(ra);
                state_->top_ = cb + 3;
                state_->call(cb, ByteCode::getArgC(ins));
                //回复top
                state_->top_ = ci->top;
                break;
            }

            case ByteCode::OP_TFORLOOP:{
                // if R(A+1) ~= nil then { R(A)=R(A+1); pc += sBx }
                //判断R(A+1)的值是否为nil，如果不是继续下次循环，需要更新循环的迭代值R(A),同时跳转到循环开始处
                //此时的R(A+1)是上面那条指令的R(A+3)
                auto& nextKey = state_->at(A(ins) + 1);
                if(!nextKey.isNil()){//继续循环
                    RA(ins) = nextKey;
                    ci->savedpc += ByteCode::getArgsBx(ins);
                }
                break;
            }

        }


        //break;

    }

}


/**
 * 计算两个value相加的结果，赋值给res
 * @param a
 * @param b
 * @param res
 */

void VM::add(const Value& a, const Value& b, int idres) {
    auto& res = state_->at(idres);
    if(a.isInteger() && b.isInteger()){
        res = Value(a.ival + b.ival);
    }else{
        //convertonumber 只会有两种结果 nil 和 number
        Value na = a.convertToNumber();
        Value nb = b.convertToNumber();

        if(na.isFloat() && nb.isFloat()){
            res = Value(na.nval + nb.nval);
        }else{
            GlobalState::meta->tryCallBinary(a,b, idres, Meta::TM_ADD);
        }
    }
}

void VM::sub(const Value &a, const Value &b, int idres) {
    auto& res = state_->at(idres);
    if(a.isInteger() && b.isInteger()){
        res = Value(a.ival - b.ival);
    }else{
        //convertonumber 只会有两种结果 nil 和 number
        Value na = a.convertToNumber();
        Value nb = b.convertToNumber();

        if(na.isFloat() && nb.isFloat()){
            res = Value(na.nval - nb.nval);
        }else{
            GlobalState::meta->tryCallBinary(a,b, idres, Meta::TM_SUB);
        }
    }
}

void VM::mul(const Value &a, const Value &b, int idres) {
    auto& res = state_->at(idres);
    if(a.isInteger() && b.isInteger()){
        res = Value(a.ival * b.ival);
    }else{
        //convertonumber 只会有两种结果 nil 和 number
        Value na = a.convertToNumber();
        Value nb = b.convertToNumber();

        if(na.isFloat() && nb.isFloat()){
            res = Value(na.nval / nb.nval);
        }else{
            GlobalState::meta->tryCallBinary(a,b, idres, Meta::TM_MUL);
        }
    }
}

void VM::div(const Value &a, const Value &b, int idres) {
    auto& res = state_->at(idres);
    if(a.isInteger() && b.isInteger()){
        res = Value(a.ival / b.ival);
    }else{
        //convertonumber 只会有两种结果 nil 和 number
        Value na = a.convertToNumber();
        Value nb = b.convertToNumber();

        if(na.isFloat() && nb.isFloat()){
            res = Value(na.nval / nb.nval);
        }else{
            GlobalState::meta->tryCallBinary(a,b, idres, Meta::TM_DIV);
        }
    }
}

//尝试比较两个value,相等返回结果 成功返回1 失败返回0
int VM::eq(const Value &left, const Value &right) {
    //判读如果类型不同，首先判断是否都是number的子类型，即int和float，如果是那么尝试转化为整数比较
    if(left.type != right.type){
        if(left.basicType() != right.basicType() || left.basicType() != LUA_TNUMBER)
            return 0;
        else{
            auto i1 = left.convertToInteger();
            auto i2 = right.convertToInteger();
            return i1.isInteger() && i2.isInteger() && i1.ival == i2.ival;
        }
    }

    //此时value的type是相同的
    switch (left.type) {
        case LUA_TNIL:          return true;
        case LUA_TNUMINT:       return left.ival == right.ival;
        case LUA_TNUMFLT:       return left.nval == right.nval;
        case LUA_TBOOLEAN:      return left.bval == right.bval;
        case LUA_TSHRSTR:       return left.obj == right.obj;   //如果是短字符串直接比较所指向的对象是否一样
        case LUA_TLNGSTR:       return LuaString::equalLongString(static_cast<LuaString*>(left.obj),
                static_cast<LuaString*>(right.obj));
        case LUA_TTABLE:{
            //fall through
            if(left.obj == right.obj)//如果是同一个表的话
                return true;
            auto tbl1 = static_cast<LuaTable*>(left.obj);
            auto tbl2 = static_cast<LuaTable*>(left.obj);
            const Value* tm= nullptr;
            tm = GlobalState::meta->getMtFromTable(tbl1, Meta::TM_EQ);
            if(tm)
                tm = GlobalState::meta->getMtFromTable(tbl2, Meta::TM_EQ);;
            //如果tm不是nil那么说明有元方法
            if(tm){
                GlobalState::meta->callMetaMethod(*tm,left, right, state_->top_, 1);
                return !state_->at(state_->top_).isFalse();
            }

            //否则 fall thorough
        }
        default:
            return left.obj == right.obj;
    }

}

//比较left < right 是否成立，成立返回1 否则返回0
int VM::lt(const Value &left, const Value &right) {
    if(left.isNumber() && right.isNumber()){
        //如果两者都是number的子类型，那么比较
        auto i1 = left.convertToInteger();
        auto i2 = right.convertToInteger();
        return i1.isInteger() && i2.isInteger() && i1.ival < i2.ival;
    }else if(left.isString() && right.isString()){
        return *static_cast<LuaString*>(left.obj) < *static_cast<LuaString*>(right.obj);
    }

    auto res = GlobalState::meta->callOrderMetaMethod(left,right, Meta::TM_LT);
    return res?1:0;

}

//比较left <= right 是否成立，成立返回1 否则返回0
int VM::le(const Value &left, const Value &right) {
    if(left.isNumber() && right.isNumber()){
        //如果两者都是number的子类型，那么比较
        auto i1 = left.convertToInteger();
        auto i2 = right.convertToInteger();
        return i1.isInteger() && i2.isInteger() && i1.ival <= i2.ival;
    }else if(left.isString() && right.isString()){
        return *static_cast<LuaString*>(left.obj) <= *static_cast<LuaString*>(right.obj);
    }

    auto res = GlobalState::meta->callOrderMetaMethod(left,right, Meta::TM_LE);
    return res?1:0;

}

/**
 * @param obj 表
 * @param idxres 结果应该放在的索引, A
 * @param key   查找的key, RK(C)
 */
void VM::getTable(const Value& obj, int idxres, const Value &key) {
    //这个obj必须是table类型的
    if(!obj.isTable()){
        state_->runerror("Expected a table for gettable operand!");
    }

    auto tbl = static_cast<LuaTable*>(obj.obj);
    auto val = tbl->Get(key);
    //如果在表中找到了对应的value那么直接设置值为找到的val,然后退出
    if(!val->isNil()){
        state_->at(idxres) =  *val;
        return;
    }

    //元方法
    const Value* tm = nullptr;
    const Value* t = &obj; //t是当前表
    //否则尝试元表
    //这是一个递归的过程,如果表的元表是一个表,那么把元表设为当前表,继续查找
    for(int loop=0;loop<MAXTAGLOOP;loop++){
        if(!t->isTable()){
            state_->at(idxres) = *kNil;
            return;
        }
        auto tbl = static_cast<LuaTable*>(t->obj);
        auto val = tbl->Get(key);
        if(!val->isNil()){
            state_->at(idxres) = *val;
            return;
        }
        //尝试元表
        tm = GlobalState::meta->getMtFromTable(tbl, Meta::TM_INDEX);
        if(!tm){ //如果没有元方法
            state_->at(idxres) = *kNil;
            return;
        }
        //如果index_对应的是一个函数,那么直接调用该函数,第一个参数是表,第二个参数是key,结果放在idxres位置
        if(tm->isFunction()){
            GlobalState::meta->callMetaMethod(*tm, *t, key, idxres, 1);
            return;
        }
        //讲t替换为元表
        t = tm;
    }

    state_->runerror("Exceed max metatable loop!");

}

/**
 *
 * @param tblidx  表所在的索引
 * @param key
 * @param value
 */
void VM::setTable(const Value& obj, const Value &key, const Value &value) {
    //尝试fastset,
    //这个obj必须是table类型的
    if(!obj.isTable()){
        state_->runerror("Expected a table for settable operand!");
    }
    auto tbl = static_cast<LuaTable*>(obj.obj);
    tbl->Set(key,value);



}

/**
 * 得到obj的len,
 * @param obj  求长度的对象
 * @param idres  结果应该放置 的位置
 */
void VM::len(const Value &obj, int idres) {
    auto &res = state_->at(idres);
    switch (obj.type) {
        case LUA_TTABLE:{
            auto tbl = static_cast<LuaTable*>(obj.obj);
            const Value* tm = GlobalState::meta->getMtFromTable(tbl, Meta::TM_LEN);
            if(tm){//如果有len的元方法尝试调用该元方法
                GlobalState::meta->callMetaMethod(*tm, obj, obj, idres, 1);
            }else{
                res = Value(static_cast<LuaInteger>(tbl->Size()));
            }
            return;
        }
        case LUA_TLNGSTR:{
            auto str = static_cast<LuaString*>(obj.obj);
            res = Value(static_cast<LuaInteger>(str->lnglen()));
            return;
        }
        case LUA_TSHRSTR:{
            auto str = static_cast<LuaString*>(obj.obj);
            res = Value(static_cast<LuaInteger>(str->shrlen()));
            return;
        }

        default:{
            //
            auto tm =  GlobalState::meta->getMtFromObj(obj, Meta::TM_LEN);
            if(tm->isNil()){
                state_->runerror("Get length of object withoud len method!");
            }
            GlobalState::meta->callMetaMethod(*tm, obj, obj, idres, 1);
        }
    }
}

/**
 * 创建一个LuaClosure，其proto为p，推入栈顶。
 * encup是一个upvalue 的数组，表示当前的closure的upvalue列表
 */
void VM::pushclosure(Proto *p, Array<LuaUpvalue*>& prevUpvals, int base, int ra) {

    int nup = p->nupval();
    auto newClosure = LuaClosure::CreateLuaClosure(nup, p);
    state_->at(ra) = Value(newClosure, LUA_TLCL);
    //更新 upvalue
    for(int i=0;i<nup;i++){
        auto& upvalDesc = p->upvalDescAt(i);
        if(upvalDesc.instack){
            //如果该upvalue在当前的stack中,需要更新当前的upvalue[i] = 栈中的值
            auto upval = state_->findUpval(&state_->at(base + upvalDesc.idx));
            newClosure->setUpvalue(i, upval);
        }else{
            //不在当前的栈中，当前的closure已经捕获了，在encup里面
            newClosure->setUpvalue(i, prevUpvals[upvalDesc.idx]);
        }
    }

}

void VM::SetState(LuaState *s) {
    state_ = s;
    state_->vm_ = this;
}

void VM::CallMainClosure(LuaClosure *closure) {
    state_->push(Value(static_cast<GCObject*>(closure), LUA_TLCL));
    state_->top_ = closure->proto()->maxStackSize() + 1;
    int status = state_->pcall(0, 0);
    if(status != LUA_OK){//此时有错误,栈顶的值是一个string,输出string然后退出
        auto obj = state_->at(--state_->top_).obj;
        auto strobj = static_cast<LuaString*>(obj);
        printf("%s\n",strobj->data());
        exit(EXIT_FAILURE);
    }
}

/**
 * 连接total个value, 从 top_- total 到 top_-1
 * 每次从栈顶取出两个元素进行concate操作然后放回到栈顶,然后栈顶会减一
 * @param total
 */
void VM::concate(int total) {
    ASSERT(total>=2);

    do{
        int top = state_->top_;
        auto &a = state_->at(top-2);
        auto &b = state_->at(top-1);
        auto stra = a.convertToString();
        auto strb = b.convertToString();
        //如果两者有一个不能转换为字符串,那么就要尝试元表
        if(stra.isNil() || strb.isNil()){
            GlobalState::meta->tryCallBinary(a,b, top-2, Meta::TM_CONCAT);
        }else{//连接两个字符串
            auto sa = static_cast<LuaString*>(stra.obj);
            auto sb = static_cast<LuaString*>(strb.obj);
            std::string buf(sa->data());
            buf += sb->data();
            auto newstr = LuaString::CreateString(buf.data(), buf.size());
            state_->at(top-2) = Value(static_cast<GCObject*>(newstr), newstr->type());
        }

        //iterator
        total -= 1;
        state_->top_ -= 1;
    }while(total>1);
    //直到total=1截止

}
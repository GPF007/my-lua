//
// Created by gpf on 2021/1/11.
//

#include "compiler.h"
#include "parser.h"
#include "object/proto.h"
#include "object/luaString.h"

namespace{
    //创建一条abc的指令
    bool isMulti(int type){
        return type == kFuncCallExpr || type == kEllipsisExpr;
    }
    bool isVar(int type){
        return type == kVar || type == kSimpleVar || type == kFieldVar || type == kSubscribVar;
    }

};

Compiler::Compiler() {
    env_ = LuaString::CreateString("ENV_");
    //ctable_ = LuaTable::CreateTable();
}

int Compiler::code(Instruction i) {
    auto f = fs_->proto_;
    f->addCode(fs_->pc_, i);
    //no line infos
    return fs_->pc_++;
}

int Compiler::codeABC(ByteCode::OpCode op, int a, int b, int c) {
    return code(ByteCode::createABC(op, a,b,c));
}

int Compiler::codeABx(ByteCode::OpCode op, int a, unsigned int bx) {
    //auto ins = ByteCode::createABx(op, a, bx);
    //auto tbx = ByteCode::getArgBx(ins);
    return code(ByteCode::createABx(op, a, bx));
}

//pc+=sBx; if (A) close all upvalues >= R(A - 1)
int Compiler::codeJmp(int a, int offset) {
    return codeAsBx(ByteCode::OP_JMP, a, offset);
}
//R(A) := (Bool)B; if (C) pc++
int Compiler::codeLoadBool(int a, int b, int c) {
    return 0;
}
//return R(A), ... ,R(A+B-2)
int Compiler::codeRet(int start, int nret) {
    return codeABC(ByteCode::OP_RETURN, start, nret+1, 0);
}



/**
 * 生成一条loadk指令，寄存器在reg， 常亮索引是k
 * 注意这里没有处理k的idx大于18bit的情况，此时应该生成一条Ax指令
 * @param reg
 * @param k
 * @return
 */
int Compiler::codek(int reg, int k) {

    return codeABx(ByteCode::OP_LOADK, reg, k);
}

/**
 * 生成一条set_list指令
 * @param base 表的寄存器索引
 * @param nelems 第一个要存储的数的位置
 * @param tostore 存储的个数
 */
void Compiler::codeSetList(int base, int nelems, int tostore) {
    int c = (nelems -1) / LFIELDS_PER_FLUSH +1;
    int b = (tostore == -1)?0:tostore;
    codeABC(ByteCode::OP_SETLIST, base, b,c);
}

/**
 * 在常亮数组中尝试加入一个value，如果已经存在了那么直接返回其idx，
 * 否则加入
 * @param value
 * @return
 */
int Compiler::addConst(Value *value) {
    auto pos = fs_->ctable_->Get(value);
    int idx;
    if(pos->isNil()){
        //如果table中没有该value，那么插入一个，并且value是 它在索引数组中的索引
        auto f = fs_->proto_;
        idx = f->addConst(value);
        auto key = value;
        auto val = Value(static_cast<LuaInteger>(idx));
        fs_->ctable_->Set(*key, val);
    }else{
        //找到了是value的值
        ASSERT(pos->isInteger());
        idx = pos->ival;
    }
    return idx;

}

int Compiler::addStringConst(LuaString *s) {
    auto tmpval = Value(s, s->type());
    return addConst(&tmpval);
}

//todo change type to user data to avoid colision
//返回一个常量所在const数组中的索引
int Compiler::addIntConst(LuaInteger ival) {
    auto tmpval = Value(ival);
    return addConst(&tmpval);
}

int Compiler::addFloatConst(LuaNumber nval) {
    auto tmpval = Value(nval);
    return addConst(&tmpval);
}

int Compiler::addBoolConst(bool bval) {
    auto tmpval = Value(static_cast<LuaByte>(bval));
    return addConst(&tmpval);
}

//因为nil不能够作为key，所以使用表本身作为key
int Compiler::addNilConst() {
    auto tmpkey = Value(fs_->ctable_, LUA_TTABLE);
    return addConst(&tmpkey);
}


/**
 * 设置expr 所在pc处的return指令或者vararg的指令的参数为nresult
 * @param nresult
 */
void Compiler::fixReturns(Expr* expr, int nresult) {
    int type = expr->type;
    ASSERT(type == kFuncCallExpr || type == kEllipsisExpr);
    int pc = type==kFuncCallExpr?(static_cast<FunCallExpr*>(expr)->pc):
             (static_cast<EllipsisExpr*>(expr)->pc);

    auto& ins = fs_->proto_->codes()[pc];
    if(type == kFuncCallExpr){
        ByteCode::setArgC(ins, nresult+1);
    }else{
        //vararg
        ByteCode::setArgB(ins, nresult+1);
        //vararg 从新的freereg开始
        ByteCode::setArgA(ins, fs_->freereg_);
        fs_->allocRegs(1);
    }
}






/**
 * 具体的方法
 */
void Compiler::genLocalNameStat(LocalNameStat* node) {
    //首先创建变量
    for(auto &name: node->names){
        fs_->createLocalVar(name);
    }
    int nvars = node->names.size();
    int nexps = 0;
    //然后生成表达式的代码
    if(node->exprlist){
        //每个表达式会占据一个reg，即freereg会增加一个位置
        for(auto &expr: node->exprlist->exprList){
             genExprToNextReg(expr.get());
        }
        nexps = node->exprlist->size();
    }
    //根据赋值两端的大小调整，多的表达式舍弃，然后少的变量赋值为nil
    //现在只考虑左右相等的情况

    int extra = nvars - nexps;
    if(nexps>0 && isMulti(node->exprlist->exprList.back()->type)){
        //如果最后一个表达式是call或者vararg的话
        auto lastExpr = node->exprlist->exprList.back().get();
        extra++;
        fixReturns(lastExpr, extra);
        //如果多余的话需要多分配寄存器
        if(extra>1)
            fs_->allocRegs(extra-1);
    }else if(extra >0){
        //没有multiret比较简单，只需要多退少补就行了
        int start = fs_->freereg_;
        fs_->allocRegs(extra);
        codeABC(ByteCode::OP_LOADNIL, start, extra-1, 0);
    }

    if(nexps > nvars){
        fs_->freeRegs(nexps - nvars);
    }

    fs_->nactvar_= static_cast<LuaByte>(fs_->nactvar_ + nvars);
    //更新startpc信息
    while(nvars){
        fs_->getlocalvar(fs_->nactvar_ - nvars)->startpc = fs_->pc_;
        nvars--;
    }
    //最后要更新一些freereg，因为一个local变量对应一个reg
    fs_->freereg_ = fs_->nactvar_;

}



/**
 * 如果expr是常量，那么返回所在常量表的索引,否则返回-1
 * @param expr 表达式
 * @return
 */
int Compiler::getConstIdx(Expr *expr) {
    switch (expr->type) {
        case kIntegerExpr:  return addIntConst(static_cast<IntegerExpr*>(expr)->ival);
        case kNumeralExpr:  return addFloatConst(static_cast<NumeralExpr*>(expr)->nval);
        case kStringExpr:   return addStringConst(static_cast<StringExpr*>(expr)->sval);
        case kBoolExpr:     return addBoolConst(static_cast<BoolExpr*>(expr)->bval);
    }
    return -1;
}


/**
 * 编译一个表达式到target寄存器，
 * @param reg
 * @return
 */
void Compiler::genExprToReg(Expr *expr, int target) {
    auto idx  = getConstIdx(expr);
    if(idx>=0){
        codek(target, idx);
    }else if(expr->type == kVarExpr){
        getVarReg(static_cast<VarExpr*>(expr)->var.get(), target);
    }else if(expr->type == kRegExpr){
        codeABC(ByteCode::OP_MOVE, target, static_cast<RegExpr*>(expr)->reg,0);
    }else{
        genOtherExpr(expr, target);
    }

}

/**
 *
 * @param expr
 * @return
 */
Compiler::ExprIdx Compiler::genExprToAnyReg(Expr *expr) {
    auto idx  = getConstIdx(expr);
    //如果是常熟类型返回索引
    if(idx>=0)  return {idx, kConst};
    //var类型返回var所在的变量的reg
    if(expr->type == kVarExpr){
        return getVarReg(static_cast<VarExpr*>(expr)->var.get(),-1);
    }else if(expr->type == kRegExpr){
        //对于虚拟的表达式直接返回reg就行了
        return {static_cast<RegExpr*>(expr)->reg, kReg};
    }
    //对于其他的都要生成tmpreg
    fs_->allocRegs(1);
    auto tmpreg = fs_->freereg_-1;
    genOtherExpr(expr, tmpreg);

    return {tmpreg, kNewReg};


}

void Compiler::genOtherExpr(Expr *expr, int target) {
    switch (expr->type) {
        case kVarExpr:{
            getVarReg(static_cast<VarExpr*>(expr)->var.get(), target);
            break;
        }
        case kUnaryExpr:    genUnaryExpr(static_cast<UnaryOpExpr*>(expr), target); break;
        case kBinaryExpr:   genBinaryExpr(static_cast<BinaryOpExpr*>(expr), target); break;
        case kTableConstructorExpr:     genTableConstructorExpr(static_cast<TableConstructorExpr*>(expr), target); break;
        case kFuncCallExpr:         genFuncCallExpr(static_cast<FunCallExpr*>(expr), target);   break;
        case kParenExpr:            genExprToReg(static_cast<ParenExpr*>(expr)->expr.get(), target);        break;
        case kEllipsisExpr:{
            auto tmpExpr = static_cast<EllipsisExpr*>(expr);
            //(A), R(A+1), ..., R(A+B-2) = vararg
            tmpExpr->pc = codeABC(ByteCode::OP_VARARG, 0, 0,0);
            break;
        }
        case kNilExpr:{
            codeABC(ByteCode::OP_LOADNIL, target, 0, 0);
            break;
        }
        case kFuncBodyExpr:       genFuncBody(static_cast<FuncBodyExpr*>(expr)->fbody.get(), target);  break;
        default:
            throw std::runtime_error("Unknow expr kind");
    }
}




void Compiler::genExprToNextReg(Expr *expr) {
    auto reg = fs_->freereg_;
    fs_->allocRegs(1);
    genExprToReg(expr, reg);
}





void Compiler::genTableConstructorExpr(TableConstructorExpr *expr, int target) {
    int reg = target;
    //首先统计数组部分的长度
    int narr =  expr->listFields.size();
    int nhash = expr->recFields.size();
    //int nfields = narr + nhash;

    //生成一条newtable指令
    auto tblreg = reg;
    //todo 用浮点数表示大小
    codeABC(ByteCode::OP_NEWTABLE, tblreg, expr->varargField?narr-1:narr, nhash);

    //处理数组成员
    int na = 0;
    int tostore = 0;
    for(int i=0;i<narr;i++){
        auto field = expr->listFields[i].get();
        tostore++;
        na++;
        genExprToNextReg(static_cast<SingleField*>(field)->exp.get());
        if(i == narr-1 && tostore != 0){ //如果是最后一个表达式那么不管是否满足 LFIELDS_PER_FLUASH必须有set_list指令
            if(expr->varargField){
                //这里要释放...field的寄存器
                fs_->freeRegs(1);
                fixReturns(static_cast<SingleField*>(field)->exp.get(),  -1);
                codeSetList(tblreg, na, -1);
            }else{
                codeSetList(tblreg, na, tostore);
            }
        }else if(tostore == LFIELDS_PER_FLUSH){
            codeSetList(tblreg, na, tostore);
            tostore = 0;
        }
    }

    fs_->freereg_ = tblreg+1;

    //处理表成员
    for(int i=0;i<nhash;i++){
        auto field = expr->recFields[i].get();
        ASSERT(field->type == kExprField);
        auto exprField = static_cast<ExprField*>(field);
        //OP_SETTABLE   R(A)[RK(B)] := RK(C)
        auto b = genExprToAnyReg(exprField->left.get());
        auto c =genExprToAnyReg(exprField->right.get());
        auto bidx = b.kind==kConst?RKMASK(b.idx):b.idx;
        auto cidx = c.kind==kConst?RKMASK(c.idx):c.idx;
        codeABC(ByteCode::OP_SETTABLE, tblreg, bidx, cidx);
        freeExp(b);
        freeExp(c);
    }

    ASSERT(fs_->freereg_ == tblreg+1);

}




/**
 * 在targe处编译一个一元表达式的结果
 * @param expr 一元表达式
 * @param target  目标寄存器
 * 首先 编译子表达式
 */
void Compiler::genUnaryExpr(UnaryOpExpr *unexp, int target) {
    //UnaryExpr需要保证必须有一个中间寄存器
    //首先申请一个临时寄存器
    //auto tmpreg = fs_->freereg_++;
    //genExprToReg(unexp->exp.get(), tmpreg);
    auto exprIdx = genExprToAnyReg(unexp->exp.get());
    if(exprIdx.kind == kConst){
        //一条move指令
        auto tmpreg = fs_->freereg_;
        fs_->allocRegs(1);
        codeABC(ByteCode::OP_MOVE, tmpreg, exprIdx.idx, 0);
        exprIdx.idx = tmpreg;
        exprIdx.kind = kNewReg;
    }
    freeExp(exprIdx);
    int tmpreg = exprIdx.idx;
    auto reg = target;
    //然后根据不同的一元操作进行编译
    switch (unexp->op) {
        case OPR_NOT:       codeABC(ByteCode::OP_NOT, reg, tmpreg, 0);  break;
        case OPR_BNOT:      codeABC(ByteCode::OP_BNOT, reg, tmpreg, 0);  break;
        case OPR_MINUS:     codeABC(ByteCode::OP_UNM, reg, tmpreg, 0);  break;
        case OPR_LEN:       codeABC(ByteCode::OP_LEN, reg, tmpreg, 0);  break;
    }
}



void Compiler::genBinaryExpr(BinaryOpExpr *expr, int target) {
    int reg;

    if(expr->op == OPR_AND || expr->op == OPR_OR){

        reg = target; //todo
        //对于and 和 or指令需要特殊处理，因为需要处理跳转
        //首先编译左边的表达式
        auto b = genExprToAnyReg(expr->left.get());
        freeExp(b);
        //生成testset指令
        //if (R(B) <=> C) then R(A) := R(B) else pc++
        //如果是and的话，如果左边的值为0那么就不用计算右边的0直接赋值给最后的reg
        //如果是or，左边的是1 也不用计算后面的0
        if(expr->op == OPR_AND)
            codeABC(ByteCode::OP_TESTSET, reg, b.idx,0);
        else
            codeABC(ByteCode::OP_TESTSET, reg, b.idx,1);
        //空的跳转指令后面修正
        auto pjump = codeJmp(0,0);
        b = genExprToAnyReg(expr->right.get());
        freeExp(b);

        //一条move指令然后修正jump的跳转值
        codeABC(ByteCode::OP_MOVE, reg, b.idx,0);
        fs_->fixJump(pjump, fs_->pc_ -1  - pjump);

    }else if(expr->op == OPR_CONCAT){
        //R(A) := R(B).. ... ..R(C)
        //对于concate指令需要特殊处理哟
        auto left = expr->left.get();
        auto right = expr->right.get();
        if(right->type == kBinaryExpr && static_cast<BinaryOpExpr*>(right)->op == OPR_CONCAT){
            //如果右边的是concat类型，说明已经有concate指令了，那么bu需要生成新的指令
            auto rightConcateExpr = static_cast<BinaryOpExpr*>(right);
            genExprToReg(left, target);
            genExprToNextReg(rightConcateExpr);
            expr->pc = rightConcateExpr->pc;
            auto& ins = fs_->proto_->codes()[expr->pc];
            ByteCode::setArgA(ins, target);
            ByteCode::setArgB(ins, target);
            fs_->freeRegs(1);
        }else{
            //new ins
            auto c = fs_->freereg_;
            fs_->allocRegs(1);
            genExprToReg(left, target);
            genExprToReg(right, c);
            expr->pc =  codeABC(ByteCode::OP_CONCAT, target, target, c);
            fs_->freeRegs(1);
        }

    }else{

        //简单的二元指令
        //concate

        //首先申请两个临时寄存器然后编译
        auto b = genExprToAnyReg(expr->left.get());
        auto c = genExprToAnyReg(expr->right.get());
        //如果是const索引需要加入rkmask
        int t1 =  b.kind == kConst? RKMASK(b.idx) : b.idx;
        int t2 =  c.kind == kConst? RKMASK(c.idx) : c.idx;

        freeExp(b);
        freeExp(c);
        reg = target;

        //R(A) := RK(B) + RK(C)
        switch (expr->op) {
            case OPR_ADD:       codeABC(ByteCode::OP_ADD, reg, t1,t2);  break;
            case OPR_SUB:       codeABC(ByteCode::OP_SUB, reg, t1,t2);  break;
            case OPR_MUL:       codeABC(ByteCode::OP_MUL, reg, t1,t2);  break;
            case OPR_DIV:       codeABC(ByteCode::OP_DIV, reg, t1,t2);  break;
            case OPR_IDIV:      codeABC(ByteCode::OP_IDIV, reg, t1,t2);  break;
            case OPR_MOD:       codeABC(ByteCode::OP_MOD, reg, t1,t2);  break;
            case OPR_BAND:      codeABC(ByteCode::OP_BAND, reg, t1,t2);  break;
            case OPR_BOR:       codeABC(ByteCode::OP_BOR, reg, t1,t2);  break;
            case OPR_BXOR:      codeABC(ByteCode::OP_BXOR, reg, t1,t2);  break;
            case OPR_SHL:       codeABC(ByteCode::OP_SHL, reg, t1,t2);  break;
            case OPR_SHR:       codeABC(ByteCode::OP_SHR, reg, t1,t2);  break;
            default:{
                //对于其他的 EQ, NE等条件比较操作需要生成跳转指令
                //A B C	if ((RK(B) == RK(C)) ~= A) then pc++
                switch (expr->op) {
                    //将 > >= 和!=转化为相应的指令，只需要改变A的值即可
                    case OPR_EQ:    codeABC(ByteCode::OP_EQ, 1, t1, t2);    break;
                    case OPR_NE:    codeABC(ByteCode::OP_EQ, 0, t1, t2);    break;
                    case OPR_LE:    codeABC(ByteCode::OP_LE, 1, t1, t2);    break;
                    case OPR_GE:    codeABC(ByteCode::OP_LE, 0, t1, t2);    break;
                    case OPR_LT:    codeABC(ByteCode::OP_LT, 1, t1, t2);    break;
                    case OPR_GT:    codeABC(ByteCode::OP_LT, 0, t1, t2);    break;
                }
                //跳转指令，如果条件成立的话则不会跳转，如果条件不成立则会执行下一条jump指令，跳过代码块
                codeJmp(0, 1);
                //R(A) := (Bool)B; if (C) pc++
                //执行第一个loadbool会加载0到reg
                codeABC(ByteCode::OP_LOADBOOL, reg,0,1);
                codeABC(ByteCode::OP_LOADBOOL, reg, 1,0);
                break;
            }

        }
    }


}


/**
 * 得到var所在的寄存器，如果target>0的话那么将var编译到target, 否则返回该变量的类型
 * @param var
 * @param reg
 */
Compiler::ExprIdx Compiler::getVarReg(Var *var, int target) {
    switch (var->type) {
        case kSimpleVar:{
            auto name = static_cast<SimpleVar*>(var)->name;
            auto res = fs_->singleVar(name);
            switch (res.first) {
                case FuncState::kLocal:{
                    //对于local直接用move指令即可
                    if(target>=0)
                        codeABC(ByteCode::OP_MOVE, target, res.second,0);
                    return {res.second, kReg};
                }
                case FuncState::kUpval:{
                    //R(A) := UpValue[B], 一条getupvalu指令
                    int reg = target;
                    if(reg<0){
                        reg = fs_->freereg_;
                        fs_->allocRegs(1);
                    }
                    codeABC(ByteCode::OP_GETUPVAL, reg, res.second, 0);
                    return {reg, kNewReg};
                }
                default:{
                    //此时在env中创建全局的变量哟
                    //先得到env的var
                    int reg = target;
                    if(reg<0){
                        reg = fs_->freereg_;
                        fs_->allocRegs(1);
                    }
                    auto envar = fs_->singleVar(env_);
                    //env变量应该是local或者upvalue的
                    ASSERT(envar.first != FuncState::kGloabl);
                    auto varIdx = addStringConst(name);
                    //R(A) := UpValue[B][RK(C)]
                    //全局变量是env表中的一个值
                    auto op = envar.first == FuncState::kLocal?ByteCode::OP_GETTABLE:ByteCode::OP_GETTABUP;
                    codeABC(op, reg, envar.second, RKMASK(varIdx));
                    return {reg, kNewReg};
                }
            }
        }
        case kFieldVar:{
            //prefixexp ‘.’ Name
            //首先在tmp处申请一个寄存器
            int reg = target;
            if(reg<0){
                reg = fs_->freereg_;
                fs_->allocRegs(1);
            }
            auto fieldVar = static_cast<FieldVar*>(var);
            auto tblReg = genExprToAnyReg(fieldVar->prefix.get());
            //用一条gettable指令?
            //R(A) := R(B)[RK(C)]
            //这里c一定是常量的索引，所以要用mask
            auto c = addStringConst(fieldVar->name);
            codeABC(ByteCode::OP_GETTABLE, reg, tblReg.idx,  RKMASK(c));
            freeExp(tblReg);
            //field var 一定占据了一个寄存器
            return {reg, kNewReg};
        }
        case kSubscribVar:{
            int reg = target;
            if(reg<0){
                reg = fs_->freereg_;
                fs_->allocRegs(1);
            }
            auto subVar = static_cast<SubscribVar*>(var);

            auto tblReg = genExprToAnyReg(subVar->prefix.get());

            auto keyReg = genExprToAnyReg(subVar->sub.get());
            int c = keyReg.kind == kConst?RKMASK(keyReg.idx):keyReg.idx;
            codeABC(ByteCode::OP_GETTABLE, reg, tblReg.idx, c);
            freeExp(tblReg);
            freeExp(keyReg);
            return {reg, kNewReg};

        }
    }

    throw std::runtime_error("Unknown var type!");
}



//functioncall ::=  prefixexp args | prefixexp ‘:’ Name args
void Compiler::genFuncCallExpr(FunCallExpr *expr, int target) {
    int oldFree = fs_->freereg_;
    //现在target处编译函数的名称，如 add
    genExprToReg(expr->prefix.get(), target);
    //如果该函数变量是ｌｏｃａｌ的，需要移动到新的寄存器中

    auto reg = fs_->freereg_-1;
    //处理self
    if(expr->methodName){
        // foo:a(...)
        auto key = expr->methodName;
        auto c = addStringConst(key);
        //R(A+1) := R(B); R(A) := R(B)[RK(C)]
        ASSERT(reg + 1 == fs_->freereg_);
        //此时需要多申请一个freereg放置第一个self参数
        fs_->allocRegs(1);
        codeABC(ByteCode::OP_SELF,reg, reg, RKMASK(c));
    }
    //然后依次编译每个参数
    auto& exprs = expr->args->exprList;
    int nargs = exprs.size();
    bool lastIsMulti = false;
    //首先编译前面 n-1个参数
    for(int i=0;i<nargs;i++){
        auto exp = exprs[i].get();
        genExprToNextReg(exp);
        if(i== nargs-1 && isMulti(exp->type)){
            lastIsMulti = true;
        }
    }



    //如果是multi说明传入的参数是栈上所有的参数，即最后一个参数为call或者vararg
    if(lastIsMulti){
        auto lastExpr = exprs.back().get();
        fixReturns(lastExpr, -1);
        //fixReturns(lastPC_, kFuncCallExpr, -1);
        nargs = -1;
    }

    //回复 freereg
    fs_->freereg_ = oldFree;

    //最后的编译指令
    //R(A), ... ,R(A+C-2) := R(A)(R(A+1), ... ,R(A+B-1))
    //默认返回1个值，所以c的值为2
    expr->pc = codeABC(ByteCode::OP_CALL, reg, nargs+1, 2);

}


//只有一个expr
void Compiler::genFuncCallStat(FunCallStat *stat) {
    auto tmpreg = fs_->freereg_;
    fs_->allocRegs(1);
    auto callExpr = static_cast<FunCallExpr*>(stat->fcall.get());
    genFuncCallExpr(callExpr, tmpreg);
    ByteCode::setArgC(fs_->proto_->codes()[callExpr->pc], 1);
    fs_->freeRegs(1);
}


/* Statements */

// name and funcbody
void Compiler::genLocalFuncStat(LocalFuncStat *stat) {
    //首先创建一个局部变量
    fs_->createLocalVar(stat->name);
    fs_->adjustLocalVars(1);
    auto varIdx =  fs_->freereg_;
    fs_->allocRegs(1);
    genFuncBody(stat->fbody.get(), varIdx);
    //这里申请了一个变量的寄存器，因此不用释放就行了
    //fs_->freereg_--;

}

//cond and block
void Compiler::genWhileStat(WhileStat *stat) {
    auto beforePC = fs_->pc_;
    genExprToNextReg(stat->cond.get());
    auto r = fs_->freereg_-1;
    //首先编译表达式条件
    fs_->freereg_--;
    //if not (R(A) <=> C) then pc++
    //TEST指令,如果得出的条件结果为0，则跳转一条指令，直接结束循环
    codeABC(ByteCode::OP_TEST, r, 0,0);
    auto endPC = codeJmp(0,0);
    //进入循环体
    //BlockCnt blockCnt;
    //fs_->enterBlock(&blockCnt);
    genBlock(stat->block.get());
    //跳转到循环的开始
    codeJmp(0, beforePC - fs_->pc_ - 1);
    //fs_->leaveBlock();
    //最后修正条件为假的跳转到循环结束的指令
    fs_->fixJump(endPC, fs_->pc_ - endPC-1);
}

void Compiler::genDoStat(DoStat *stat) {
    //BlockCnt blockCnt;
    //fs_->enterBlock(&blockCnt);
    genBlock(stat->block.get());
    //fs_->leaveBlock();
}

void Compiler::genIfStat(IfStat *stat) {
    //ncond表示条件表达式的数量
    //还要包括else，因此共有 nconds +1 个block需要跳转到结尾
    vector<int> pcJumpToEnds;
    //如果有elif或者有other则说明需要在if后面加入一条jumk指令

    //首先是if exp
    auto r = fs_->freereg_;
    genExprToNextReg(stat->cond.get());
    fs_->freereg_--;

    codeABC(ByteCode::OP_TEST, r,0,0);
    auto pcJumpToNext = codeJmp(0,0);
    //enterscope
    //BlockCnt blockCnt;
    //fs_->enterBlock(&blockCnt);
    genBlock(stat->block.get());
    //fs_->leaveBlock();
    //一条跳转到最后的指令
    bool hasJump = stat->elifs.size()>0 || stat->other;
    if(hasJump){
        pcJumpToEnds.push_back(codeJmp(0,0));
    }


    //然后处理elseif
    for(auto& elif: stat->elifs){
        //修正上一条条件测试失败的跳转
        fs_->fixJump(pcJumpToNext, fs_->pc_ - pcJumpToNext);
        auto r = fs_->freereg_;

        genExprToNextReg(elif.cond.get());
        fs_->freereg_--;
        codeABC(ByteCode::OP_TEST, r,0,0);
        pcJumpToNext = codeJmp(0, 0);
        pcJumpToEnds.push_back(codeJmp(0,0));
        genBlock(stat->block.get());
    }

    //最后是else的

    fs_->fixJump(pcJumpToNext, fs_->pc_ - pcJumpToNext-1);
    if(stat->other){
        //fs_->fixJump(pcJumpToNext, fs_->pc_ - pcJumpToNext);
        //enterscope
        genBlock(stat->block.get());
    }



    //ASSERT(i == nconds);
    //最后修正所有的jumptoends
    //日如果只有1个说明只有if块，那么if块结束后就是该语句的结尾，因此不用加上jump
    for(auto pc: pcJumpToEnds){
        fs_->fixJump(pc, fs_->pc_ - pc-1);
    }

}

/**
 * 一般for语句， for i=1,2,3 (init, last, step)
 * @param stat
 */
void Compiler::genForEqStat(ForEqStat *stat) {
    BlockCnt blockCnt;
    fs_->enterBlock(&blockCnt);
    int base = fs_->freereg_;
    //首先生成3个内部变量和name的变量
    fs_->createLocalVarLiteral("(for index)");
    fs_->createLocalVarLiteral("(for limit)");
    fs_->createLocalVarLiteral("(for step)");
    fs_->createLocalVar(stat->name);
    //initvalue
    genExprToNextReg(stat->init.get());
    genExprToNextReg(stat->last.get());
    //step 默认是 1
    if(stat->step){
        genExprToNextReg(stat->step.get());
    }else{
        codek(fs_->freereg_, addIntConst(1));
        fs_->allocRegs(1);
    }
    //forbody
    fs_->adjustLocalVars(3);
    auto prep = codeAsBx(ByteCode::OP_FORPREP, base, -1);
    fs_->adjustLocalVars(1);
    fs_->allocRegs(1);
    //在newblock已经生成了一个变量
    for(auto &stat: stat->block->stats){
        genStatement(stat.get());
    }
    fs_->leaveBlock();
    //for loop
    auto endfor = codeAsBx(ByteCode::OP_FORLOOP, base, -1);

    //最后修正prep 和 endfor
    fs_->fixSbx(prep,endfor - prep -1);
    fs_->fixSbx(endfor, prep - endfor);

    //for 语句结束后， freereg的值应该是base
    //最后要删除三个变量
    //fs_->leaveBlock();


    fs_->freereg_ = base;
}

/**
 * 编译一个函数体，即在当前fs中加入一个subfs进行编译，
 * @param fbody
 * @param reg 表示OP_CLOSURE 的目标寄存器
 */
void Compiler::genFuncBody(FuncBody* fbody, int reg, bool isMethod) {
    //因为subidx是加入新proto之前的长度，因此新的proto会占据该位置，len+=1
    int subIdx = fs_->proto_->nsubs();
    auto subProto = fs_->addSubProto();
    BlockCnt blockCnt;
    //创建nfs时构造函数中会调用enterblock
    auto nfs = new FuncState(fs_, subProto, &blockCnt);
    fs_ = nfs;
    //处理 param
    //首先方法需要插入"self"param
    if(isMethod){
        fs_->createLocalVarLiteral("self");
        fs_->adjustLocalVars(1);
    }
    for(auto name: fbody->names){
        fs_->createLocalVar(name);
    }
    int nparam =fbody->names.size();
    if(fbody->hasVararg){
        subProto->setVararg();
    }
    fs_->adjustLocalVars(nparam);
    //分配寄存器
    fs_->allocRegs(fs_->nactvar_);
    fs_->proto_->setNparam(fs_->nactvar_);

    //开始处理函数体
    for(auto &stat: fbody->block->stats){
        genStatement(stat.get());
    }


    //结束生成一条return指令
    codeRet(0,0);
    //最后离开block
    fs_->leaveBlock();
    fs_ = fs_->prev_;
    delete nfs;
    //在父fs中生成一条closure指令
    codeABx(ByteCode::OP_CLOSURE, reg,  subIdx);

}


void Compiler::genFuncDefStat(FuncDefStat *stat) {
    //然后编译函数体
    auto freg = fs_->freereg_;
    fs_->allocRegs(1);
    genFuncBody(stat->fbody.get(), freg, stat->isMethod);
    //调用一次store var函数
    //因为这里新申请了一个reg所以，类型是newreg
    auto fexpr = std::make_unique<RegExpr>(freg);
    storeVar(stat->fname.get(), fexpr.get());

    fs_->freeRegs(1);

}



void Compiler::storeVar(Var* var, Expr *exp, int src) {
    switch (var->type) {
        case kSimpleVar:{
            auto res = fs_->singleVar(static_cast<SimpleVar*>(var)->name);
            switch (res.first) {
                case FuncState::kLocal:{

                    genExprToReg(exp, res.second);
                    break;
                }
                case FuncState::kUpval:{
                    //UpValue[B] := R(A)
                    auto exprIdx = genExprToAnyReg(exp);
                    codeABC(ByteCode::OP_SETUPVAL, exprIdx.idx, res.second,0);
                    freeExp(exprIdx);
                    break;
                }
                case FuncState::kGloabl:{
                    //如果var是global的话，那么属于ENV_表
                    //UpValue[A][RK(B)] := RK(C)
                    //此时在env中创建全局的变量哟
                    //先得到env的var
                    auto envar = fs_->singleVar(env_);
                    //env变量应该是local或者upvalue的
                    ASSERT(envar.first != FuncState::kGloabl);
                    //在env表中创建一个变量，根据是否是upvalue调用setupvalue还是setuptable
                    //varIdx是常量表的索引，因此需要加上RKMASK
                    auto exprIdx = genExprToAnyReg(exp);
                    freeExp(exprIdx);
                    auto varIdx = addStringConst(static_cast<SimpleVar*>(var)->name);
                    auto op = envar.first == FuncState::kLocal?ByteCode::OP_SETTABLE:ByteCode::OP_SETTABUP;
                    codeABC(op, envar.second, RKMASK(varIdx), exprIdx.idx);
                    break;
                }
            }
            break;
        }

        case kFieldVar:{
            //a.b.c
            //申请一个表所占用的寄存器
            auto fieldVar = static_cast<FieldVar*>(var);
            auto prefix = fieldVar->prefix.get();
            ASSERT(prefix->type == kVarExpr);
            //首先得到要存储的变量所在的寄存器
            auto prefixVar = static_cast<VarExpr*>(prefix);
            auto varRegIdx = getVarReg(prefixVar->var.get(), -1);
            auto tblReg = varRegIdx.idx;
            auto b = addStringConst(fieldVar->name);
            auto exprIdx = genExprToAnyReg(exp);
            int c = exprIdx.idx;
            //R(A)[RK(B)] := RK(C)
            //如果ｃ是常量表索引，需要mask
            if(exprIdx.kind == kConst){
                c = RKMASK(c);
            }
            codeABC(ByteCode::OP_SETTABLE, tblReg, RKMASK(b), c);
            freeExp(varRegIdx);
            freeExp(exprIdx);
            break;
        }

        case kSubscribVar:{
            //prefix ['exp']
            auto subVar = static_cast<SubscribVar*>(var);
            auto prefix = subVar->prefix.get();
            ASSERT(prefix->type == kVarExpr);
            auto prefixVar = static_cast<VarExpr*>(prefix);
            auto varRegIdx = getVarReg(prefixVar->var.get(), -1);
            auto bReg = genExprToAnyReg(subVar->sub.get());
            auto expRegIdx = genExprToAnyReg(exp);
            //这里分类讨论，如果 索引表达式是常数类型则b需要RKMASK，否则需要申请一个寄存器哟
            //R(A)[RK(B)] := RK(C)
            int b = bReg.kind == kConst?RKMASK(bReg.idx):bReg.idx;
            int c = expRegIdx.kind == kConst?RKMASK(expRegIdx.idx):expRegIdx.idx;
            codeABC(ByteCode::OP_SETTABLE, varRegIdx.idx, b, c);
            freeExp(bReg);
            freeExp(expRegIdx);
            freeExp(varRegIdx);
            break;
        }

    }
}

/**
 * 编译赋值表达式
 * vars  = exprs
 * @param stat
 */
void Compiler::genAssignStat(AssignStat *stat) {
    //先编译表达式
    int nvars = stat->vars.size();
    int nexps = stat->exprs->size();
    //这里做了适量的简化，只允许var的数量比nexps多的情况
    ASSERT(nvars >= nexps);

    //按照常规方法处理n个表达式
    for(int i=0;i<nexps;i++){
        auto exp = stat->exprs->exprList.at(i).get();
        auto varExp = stat->vars.at(i).get();
        //保证=左边必须是var类型的exp
        ASSERT(varExp->type == kVarExpr);
        auto var = static_cast<VarExpr*>(varExp)->var.get();
        storeVar(var, exp);
        //auto expReg = genExpr(exp);
        //storeVar(var, expReg);
        //freeExp(expReg);//一定要释放
    }


    //然后处理最后一个表达式
    auto lastExpr = stat->exprs->exprList.back().get();
    if(isMulti(lastExpr->type) && nvars > nexps){
        //如果最后一个表达式是multi且var的数量比exp多
        int extra = nvars - nexps;
        extra++;
        //fixReturns(lastPC_, lastExpr->type, extra);
        fixReturns(lastExpr, extra);
        //给变量赋值, freereg此时是函数的第一个返回值的位置，第一个已经赋值完了
        for(int i=1;i<extra;i++){
            auto varExp = stat->vars.at(i+nexps-1).get();
            //保证=左边必须是var类型的exp
            ASSERT(varExp->type == kVarExpr);
            auto var = static_cast<VarExpr*>(varExp)->var.get();
            //首先尝试constexpr
            auto e = std::make_unique<RegExpr>(fs_->freereg_+i);
            storeVar(var, e.get());
        }
    }
}

/**
 * 编译return 语句
 * @param stat
 * return explist
 */
void Compiler::genRetStat(RetStat *stat) {
    int first=0;
    int nret = stat->exps->size();
    //todo 暂时不考虑　multiret
    int oldFree = fs_->freereg_;

    for(auto &expr: stat->exps->exprList){
        genExprToNextReg(expr.get());
    }
    if(stat->exps->exprList.empty()){
        first = nret = 0;
    }else if(stat->exps->exprList.back().get()->type == kFuncCallExpr){
        auto lastExpr = stat->exps->exprList.back().get();

        //那么最后一个函数应该返回多个值
        fixReturns(lastExpr, -1);
        //然后用tail call来表示一下
        if(nret == 1){
            int pc = static_cast<FunCallExpr*>(lastExpr)->pc;
            auto& ins = fs_->proto_->codes()[pc];
            ByteCode::setOpCode(ins, ByteCode::OP_TAILCALL);
        }
        first = fs_->nactvar_;
        nret = -1;
    }else{
        //oldfree是第一个expression position
        if(nret == 1){
            first = oldFree;
        }else{
            //返回所有活跃的变量
            first = fs_->nactvar_;
            //todo
            //返回的数量应该是　first 到最新freereg之间的所有寄存器
            //ASSERT(nret == fs_->freereg_  - first);
        }
    }
    fs_->freereg_ = oldFree;
    codeRet(first,nret);

}

void Compiler::genStatement(Statement *stat) {
    switch (stat->type) {
        case kAssignStat:   genAssignStat(static_cast<AssignStat*>(stat));          break;
        case kIfStat:       genIfStat(static_cast<IfStat*>(stat));                  break;
        case kWhileStat:    genWhileStat(static_cast<WhileStat*>(stat));            break;
        case kDoStat:       genDoStat(static_cast<DoStat*>(stat));                  break;
        case kFuncDefStat:  genFuncDefStat(static_cast<FuncDefStat*>(stat));        break;
        case kRetStat:      genRetStat(static_cast<RetStat*>(stat));                break;
        case kFuncCallStat: genFuncCallStat(static_cast<FunCallStat*>(stat));       break;
        case kLocalNameStat: genLocalNameStat(static_cast<LocalNameStat*>(stat));   break;
        case kLocalFuncStat:    genLocalFuncStat(static_cast<LocalFuncStat*>(stat)); break;
        case kForEqStat:    genForEqStat(static_cast<ForEqStat*>(stat));            break;
        case kEmptyStat:                                                            break;

        default:{
            ASSERT(false);
        }
    }




}

/**
 * 编译block块
 * @param block
 */
void Compiler::genBlock(Block *block) {
    BlockCnt bl;
    fs_->enterBlock(&bl);
    for(auto &stat: block->stats){
        genStatement(stat.get());
    }
    fs_->leaveBlock();
}

//编译一个块
Proto * Compiler::compile() {
    BlockCnt bl;
    auto proto = Proto::CreateProto();
    fs_ = new FuncState(nullptr, proto, & bl);
    proto->setVararg();
    fs_->newUpvalue(env_, FuncState::kLocal, 0);
    //这时候已经进入新的block了，所以直接编译语句列表即可
    for(auto &stat: block_->stats){
        genStatement(stat.get());
    }
    codeRet(0,0);
    fs_->leaveBlock();
    delete fs_;

    return proto;

}

Proto * Compiler::compile(const char *fname) {
    auto parser = std::make_unique<Parser>(fname);
    block_ = parser->Parse();

    //开始编译
    BlockCnt bl;
    auto proto = Proto::CreateProto();
    fs_ = new FuncState(nullptr, proto, & bl);
    proto->setVararg();
    fs_->newUpvalue(env_, FuncState::kLocal, 0);
    //这时候已经进入新的block了，所以直接编译语句列表即可
    for(auto &stat: block_->stats){
        genStatement(stat.get());
    }
    codeRet(0,0);
    fs_->leaveBlock();
    delete fs_;

    return proto;

}


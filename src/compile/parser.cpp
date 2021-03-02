//
// Created by gpf on 2020/12/29.
//

#include "parser.h"
#include <fmt/core.h>

/*
namespace{
    //创建一条abc的指令
    bool isMulti(int type){
        return type == kFuncCallExpr || type == kEllipsisExpr;
    }
    bool isVar(int type){
        return type == kVar || type == kSimpleVar || type == kFieldVar || type == kSubscribVar;
    }

};
*/



namespace {
    const struct {
        LuaByte left;  /* left priority for each binary operator */
        LuaByte right; /* right priority */
    } priority[] = {  /* ORDER OPR */
            {10, 10}, {10, 10},           /* '+' '-' */
            {11, 11}, {11, 11},           /* '*' '%' */
            {14, 13},                  /* '^' (right associative) */
            {11, 11}, {11, 11},           /* '/' '//' */
            {6, 6}, {4, 4}, {5, 5},   /* '&' '|' '~' */
            {7, 7}, {7, 7},           /* '<<' '>>' */
            {9, 8},                   /* '..' (right associative) */
            {3, 3}, {3, 3}, {3, 3},   /* ==, <, <= */
            {3, 3}, {3, 3}, {3, 3},   /* ~=, >, >= */
            {2, 2}, {1, 1}            /* and, or */
    };


};
#define UNARY_PRIORITY	12  /* priority for unary operators */


void Parser::reset(std::string content) {
    lexer = std::make_unique<Lexer>(content, 0);
    token_ = lexer->GetToken();
    nextToken_ = std::make_unique<Token>(Token::TK_EOS);
}

Parser::Parser(const char *fname) {
    lexer = std::make_unique<Lexer>(fname);
    token_ = lexer->GetToken();
    nextToken_ = std::make_unique<Token>(Token::TK_EOS);
}

Parser::Parser(std::string content, int i) {
    lexer = std::make_unique<Lexer>(content, i);
    token_ = lexer->GetToken();
    nextToken_ = std::make_unique<Token>(Token::TK_EOS);
}

void Parser::syntaxError(const char *msg) {
    lexer->error(msg, (token_)?token_->type():0);
}

void Parser::errorExpected(int tokenType) {
    auto msg = fmt::format("{} expected", Token::type2string(tokenType));
    syntaxError(msg.data());
}

void Parser::semError(const char *msg) {
    token_ = nullptr;
    syntaxError(msg);
}

//期待一个类型为c的token ，如果不是返回错误
void Parser::check(int c) {
    if(token_->type() !=c )
        errorExpected(c);
}

void Parser::checknext(int c) {
    check(c);
    next();
}

/**
 * 检查当前的token是否是类型c，如果是next返回true，否则返回false
 * @param c
 * @return
 */
bool Parser::testnext(int c) {
    if(token_->type() == c){
        next();
        return true;
    }
    return false;
}

/**
 * 这个函数用来检查是否有闭合的token
 * @param what 期待的下一token的类型
 * @param who 原来的左半边token （ 【等
 * @param where 所在的行数
 */
void Parser::checkMatch(int what, int who, int where) {
    if(!testnext(what)){
        if(where == lexer->lineno())
            errorExpected(what);
        else{
            auto msg = fmt::format("{} expected (to close {} at line {})",
                    Token::type2string(what), Token::type2string(who), where);
            syntaxError(msg.data());
        }
    }
}

/**
 * 判断当前token是否是一个块的结束，如果是返回true】
 * 当token是 else, elseif,end, eos表示结束
 * @return
 */
bool Parser::blockFollow() {
    switch (token_->type()) {
        case Token::TK_ELSE: case Token::TK_ELSEIF:
        case Token::TK_END: case Token::TK_EOS:
            return true;
    }
    return false;
}

/**
 * 得到当前token 的sval， 其必须是name类型的，然后进行next操作
 * @return
 */
LuaString * Parser::getName() {
    check(Token::TK_NAME);
    auto name = token_->sval();
    next();
    return name;
}

/**
 * 读取下一个token
 */
void Parser::next() {
    lexer->setlastline(lexer->lineno());
    if(nextToken_->type() != Token::TK_EOS){
        token_ = std::move(nextToken_);
        nextToken_ = std::make_unique<Token>(Token::TK_EOS);
    }else{
        token_ = lexer->GetToken();
    }
}
/**
 * 查看下一个token的类型
 */
int Parser::lookahead() {
    ASSERT(nextToken_->type() == Token::TK_EOS);
    nextToken_ = lexer->GetToken();
    return nextToken_->type();
}

UnOpr Parser::getunopr(int op) {
    switch (op) {
        case Token::TK_NOT: return OPR_NOT;
        case '-': return OPR_MINUS;
        case '~': return OPR_BNOT;
        case '#': return OPR_LEN;
        default: return OPR_NOUNOPR;
    }
}

BinOpr Parser::getbinopr(int op) {
    switch (op) {
        case '+': return OPR_ADD;
        case '-': return OPR_SUB;
        case '*': return OPR_MUL;
        case '%': return OPR_MOD;
        case '^': return OPR_POW;
        case '/': return OPR_DIV;
        case Token::TK_IDIV: return OPR_IDIV;
        case '&': return OPR_BAND;
        case '|': return OPR_BOR;
        case '~': return OPR_BXOR;
        case Token::TK_SHL: return OPR_SHL;
        case Token::TK_SHR: return OPR_SHR;
        case Token::TK_CONCAT: return OPR_CONCAT;
        case Token::TK_NE: return OPR_NE;
        case Token::TK_EQ: return OPR_EQ;
        case '<': return OPR_LT;
        case Token::TK_LE: return OPR_LE;
        case '>': return OPR_GT;
        case Token::TK_GE: return OPR_GE;
        case Token::TK_AND: return OPR_AND;
        case Token::TK_OR: return OPR_OR;
        default: return OPR_NOBINOPR;
    }
}

ExprP Parser::parseExpr() {
    auto ret = parseSubExpr(0);
    auto tmp = std::move(ret.first);
    return tmp;
}

/** Parse Expressions **/
std::pair<ExprP, int> Parser::parseSubExpr(int curPriority) {
    auto uop = getunopr(token_->type());
    ExprP exp;
    if(uop!= OPR_NOUNOPR){
        //如果得到了一元操作符
        next();
        auto subexpr = parseSubExpr(UNARY_PRIORITY).first;
        exp = std::make_unique<UnaryOpExpr>(std::move(subexpr), (int)uop);
    }else{
        exp = parseSimpleExpr();
    }

    //此时已经得到了expr然后查看是否有binary 的操作符
    int bop = getbinopr(token_->type());
    while(bop != OPR_NOBINOPR && priority[bop].left > curPriority){
        //如果bop的优先级大于当前的优先级的话，那么尝试parse右边的
        next();
        auto rexp = parseSubExpr(priority[bop].right);
        exp = std::make_unique<BinaryOpExpr>(std::move(exp), std::move(rexp.first), bop);
        bop = rexp.second;
    }

    return std::make_pair(std::move(exp),bop);
}

/**
 * simpleexp -> FLT | INT | STRING | NIL | TRUE | FALSE | ... |
                  constructor | FUNCTION body | suffixedexp
 *
 * 对于constructor function和suffiedexp parse完成后会自动后移一个token，因此直接return，不用在当前函数进行next操作
 */
ExprP Parser::parseSimpleExpr() {
    //根据当前tokendtype判断
    ExprP exp;
    switch (token_->type()) {
        case Token::TK_FLT:{
            exp = std::make_unique<NumeralExpr>(token_->nval());
            break;
        }
        case Token::TK_INT:{
            exp = std::make_unique<IntegerExpr>(token_->ival());
            break;
        }
        case Token::TK_STRING:{
            exp = std::make_unique<StringExpr>(token_->sval());
            break;
        }
        case Token::TK_NIL:{
            exp = std::make_unique<NilExpr>();
            break;
        }
        case Token::TK_TRUE:{
            exp = std::make_unique<BoolExpr>(true);
            break;
        }
        case Token::TK_FALSE:{
            exp = std::make_unique<BoolExpr>(false);
            break;
        }
        case Token::TK_DOTS:{
            // ...
            exp = std::make_unique<EllipsisExpr>();
            break;
        }
        case '{':{
            //constructor
            exp = parseConstructor();
            return exp;
        }
        case Token::TK_FUNCTION:{
            // ...
            next();
            //todo body
            auto fbody = parseFuncBody();
            exp = std::make_unique<FuncBodyExpr>(std::move(fbody));
            //exp = std::make_unique<EllipsisExpr>();
            return exp;
        }

        default:{
            exp = parseSuffixedExpr();
            return exp;
        }
    }

    next();
    return exp;
}

/**
 * parse后缀表达式
 * @return
 * suffixedexp ->
       primaryexp { '.' NAME | '[' exp ']' | ':' NAME funcargs | funcargs }
   primaryexp后面可能是多个后缀
 */
ExprP Parser::parseSuffixedExpr() {
    auto primary = parsePrimaryExpr();
    //int line = lexer->lineno();
    std::unique_ptr<Var> var;
    for(;;){
        switch (token_->type()) {
            default:{
                return primary;
            }
            case '.':{
                // fieldvar a.b
                next();
                var = std::make_unique<FieldVar>(std::move(primary), token_->sval());
                primary = std::make_unique<VarExpr>(std::move(var));
                next(); //skip name
                break;
            }
            case '[':{
                // '[' exp ']'
                next();
                auto exp = parseExpr();
                checknext(']');
                var = std::make_unique<SubscribVar>(std::move(primary),std::move(exp));
                primary = std::make_unique<VarExpr>(std::move(var));
                break;
            }
            case ':':{
                //':' NAME funcargs
                next();
                ASSERT(token_->type() == Token::TK_NAME);
                auto name = token_->sval();
                next();
                primary = std::make_unique<FunCallExpr>(std::move(primary), parseArgs(), name);

                break;
            }

            case '(': case Token::TK_STRING: case '{': {
                primary = std::make_unique<FunCallExpr>(std::move(primary), parseArgs());
                break;
            }

        }
    }
}
/**
 * primaryexp -> NAME | '(' expr ')'
 * @return
 */
ExprP Parser::parsePrimaryExpr() {
    switch (token_->type()) {
        case Token::TK_NAME:{
            //一个变量，首先创建var，然后创建exp
            std::unique_ptr<Var> var = std::make_unique<SimpleVar>(token_->sval());
            ExprP varexpr = std::make_unique<VarExpr>(std::move(var));
            next();
            return varexpr;
        }
        case '(':{
            int line = lexer->lineno();
            next();// skip (
            auto expr = parseExpr();
            checkMatch(')','(', line);
            return std::make_unique<ParenExpr>(std::move(expr));
        }
        default:{
            syntaxError("unexpected symbol");
        }
    }
    //should not reach here
    return nullptr;
}

/**
 * constructor -> '{' [ field { sep field } [sep] ] '}'
     sep -> ',' | ';'
 * @return
 */
ExprP Parser::parseConstructor() {
    unique_ptr<Field> field;
    auto ret = std::make_unique<TableConstructorExpr>();
    int line = lexer->lineno();
    checknext('{');
    do{
        if(token_->type() == '}') break;
        auto field = parseField();
        if(field->type == kSingleField)
            ret->listFields.push_back(std::move(field));
        else
            ret->recFields.push_back(std::move(field));

    }while(testnext(',') || testnext(';'));
    //检查最后一个listfield是否是...
    if(!ret->listFields.empty()){
        auto laseField = static_cast<SingleField*>(ret->listFields.back().get());
        if(laseField->exp->type == kEllipsisExpr || laseField->exp->type == kFuncCallExpr){
            ret->varargField = true;
        }
    }

    checkMatch('}','{', line);
    return ret;

}

//field -> listfield | recfield
//field ::= ‘[’ exp ‘]’ ‘=’ exp | Name ‘=’ exp | exp
std::unique_ptr<Field> Parser::parseField() {

    auto curType = token_->type();
    if(curType== '['){
        //recfield
        next();//skip [
        auto key = parseExpr();
        checknext(']');
        checknext('=');
        auto val = parseExpr();
        auto tmp_field = std::make_unique<ExprField>(std::move(key), std::move(val));
        return tmp_field;
    }else if(curType== Token::TK_NAME && lookahead() == '='){
        //recfield
        auto name = token_->sval();
        next();
        checknext('=');
        unique_ptr<Expr> keyexpr = std::make_unique<StringExpr>(name);
        return std::make_unique<ExprField>(std::move(keyexpr), parseExpr());
    }else{
        //list field, simgle expr
        return std::make_unique<SingleField>(parseExpr());
    }
}

unique_ptr<ExprList> Parser::parseArgs() {
    auto ret = std::make_unique<ExprList>();
    switch (token_->type()) {
        case '(':{
            // funcargs -> '(' [ explist ] ')'
            next();
            if(token_->type() == ')'){
                next();
                goto done;
            }
            //explist
            ret = parseExprList();
            next(); //skip )
            break;
        }
        case '{':{
            ret->AddExpr(parseConstructor());
            break;
        }
        case Token::TK_STRING:{
            ret->AddExpr(std::make_unique<StringExpr>(token_->sval()));
            next();
            break;

        }
        default:{
            syntaxError("function arguments expected");
        }
    }

    done:
    return ret;
}

unique_ptr<ExprList> Parser::parseExprList() {
    auto exps = std::make_unique<ExprList>();
    exps->AddExpr(parseExpr());
    while(testnext(',')){
        exps->AddExpr(parseExpr());
    }
    return exps;
}

/**
 * Parse statement
 * @return
 */
StatP Parser::parseStat() {
    int line = lexer->lineno();
    StatP stat;
    switch (token_->type()) {
        case ';':{
            stat = std::make_unique<EmptyStat>();
            next();
            break;
        }
        case Token::TK_IF:{
            stat = parseIfStat();
            break;
        }
        case Token::TK_WHILE:{
            stat = parseWhileStat();
            break;
        }
        case Token::TK_DO:{
            next();
            stat = std::make_unique<DoStat>(parseBlock());
            checkMatch(Token::TK_END, Token::TK_DO, line);
            break;
        }
        case Token::TK_FOR:{
            stat = parseForStat();
            break;
        }
        case Token::TK_FUNCTION:{
            next();
            //parse funcbody
            stat = parseFuncDefStat();
            break;
        }
        case Token::TK_LOCAL:{
            next();
            if(testnext(Token::TK_FUNCTION))
                stat = parseLocalFunc();
            else
                stat = parseLocalStat();
            break;
        }
        case Token::TK_RETURN:{
            next();
            stat = parseRetStat();
            break;
        }
        case Token::TK_BREAK:{
            stat = std::make_unique<BreakStat>();
            break;
        }
        default:{
            stat = parseExprStat();
            break;
        }
    }

    return stat;
}

//ifstat -> IF cond THEN block {ELSEIF cond THEN block} [ELSE block] END
StatP Parser::parseIfStat() {
    next();//skip if
    auto cond = parseExpr();
    checknext(Token::TK_THEN);
    auto block = parseBlock();

    vector<CondStat> elifs;
    while(testnext(Token::TK_ELSEIF)){
        auto tcond = parseExpr();
        checknext(Token::TK_THEN);
        auto tblock = parseBlock();
        elifs.emplace_back(std::move(tcond), std::move(tblock));
    }
    //是否有else啊
    BlockP elseBlock;
    if(testnext(Token::TK_ELSE)){
        elseBlock = parseBlock();
    }
    checknext(Token::TK_END);

    auto ret = std::make_unique<IfStat>(std::move(cond), std::move(block),
            std::move(elifs), std::move(elseBlock));

    return ret;

}
/* whilestat -> WHILE cond DO block END */
StatP Parser::parseWhileStat() {
    next();
    auto cond = parseExpr();
    checknext(Token::TK_DO);
    auto block = parseBlock();
    checknext(Token::TK_END);
    return std::make_unique<WhileStat>(std::move(cond), std::move(block));
}

/* forstat -> FOR (fornum | forlist) END */
StatP Parser::parseForStat() {
    next();
    auto name = token_->sval();
    next();
    StatP ret;
    switch (token_->type()) {
        case '=':{
            //for Name ‘=’ exp ‘,’ exp [‘,’ exp] do block end
            next();
            auto init = parseExpr();
            checknext(',');
            auto limit = parseExpr();
            ExprP step;
            if(testnext(','))
                step = parseExpr();
            checknext(Token::TK_DO);
            auto block = parseBlock();
            ret = std::make_unique<ForEqStat>(name, std::move(block),
                    std::move(init),std::move(limit), std::move(step));
            break;
        }
        case ',':{
            //for namelist in explist do block end
            vector<LuaString*> names;
            names.push_back(name);
            while(testnext(',')){
                auto name = getName();
                names.push_back(name);

            }
            checknext(Token::TK_IN);
            auto exprs = parseExprList();
            checknext(Token::TK_DO);
            auto block = parseBlock();
            ret = std::make_unique<ForInStat>(std::move(names), std::move(exprs),
                    std::move(block));
            break;
        }
    }

    checknext(Token::TK_END);
    return ret;
}

// return [explist] [;]
StatP Parser::parseRetStat() {
    if(blockFollow() || token_->type() == ';'){
        return std::make_unique<RetStat>();
    }
    auto exprs = parseExprList();
    auto ret= std::make_unique<RetStat>(std::move(exprs));
    testnext(';'); //maybe ret
    return ret;
}

//block ::= {stat} [retstat]
BlockP Parser::parseBlock() {
    auto block = std::make_unique<Block>();
    while(!blockFollow()){
        //return语句是块最后一条语句哟
        if(token_->type() == Token::TK_RETURN){
            block->AddStat(parseStat());
            return block;
        }
        block->AddStat(parseStat());
    }

    return block;
}
//local namelist [‘=’ explist]
StatP Parser::parseLocalStat() {
    auto ret = std::make_unique<LocalNameStat>();
    do{
        auto name = token_->sval();
        ret->AddName(name);
        next();
    }while(testnext(','));
    if(testnext('=')){
        ret->exprlist = parseExprList();
    }
    return ret;
}
/* stat -> func | assignment */
StatP Parser::parseExprStat() {
    auto expr = parseSuffixedExpr();
    if(token_->type() == '=' || token_->type() == ','){
        //assign statemetn
        auto ret = std::make_unique<AssignStat>();
        if(expr->type != kVarExpr){
            syntaxError("Expected left value in assignment left");
        }
        ret->AddVar( std::move(expr));
        while(testnext(',')){
            expr = parseSuffixedExpr();
            if(expr->type != kVarExpr){
                syntaxError("Expected left value in assignment left");
            }
            ret->AddVar( std::move(expr));
        }
        if(testnext('='))
            ret->exprs = parseExprList();

        return ret;
    }
    //else is func call
    return std::make_unique<FunCallStat>(std::move(expr));
}
//local function Name funcbody
StatP Parser::parseLocalFunc() {
    //already skip function
    auto name = getName();
    auto ret = std::make_unique<LocalFuncStat>(name, parseFuncBody());
    return ret;
}

//funcbody ::= ‘(’ [parlist] ‘)’ block end
//parlist ::= namelist [‘,’ ‘...’] | ‘...’
unique_ptr<FuncBody> Parser::parseFuncBody() {
    auto ret = std::make_unique<FuncBody>();
    checknext('(');
    if(testnext(')'))
        goto paramdone;

    if(testnext(Token::TK_DOTS)){
        ret->hasVararg = true;
        checknext(')');
        goto paramdone;
    }else{
        //name list
        auto name = token_->sval();
        next();
        ret->AddParam(name);
        while(testnext(',')){
            if(token_->type() == Token::TK_DOTS){
                ret->hasVararg = true;
                next();
                break;
            }
            ret->AddParam(token_->sval());
            next();
        }
        checknext(')');
    }

    paramdone:
    //此时已经parse param完了
    ret->block = parseBlock();
    checknext(Token::TK_END);

    return ret;
}

//funcname ::= Name {‘.’ Name} [‘:’ Name]
unique_ptr<FuncName> Parser::parseFuncName() {
    auto fname = std::make_unique<FuncName>();
    auto name=  getName();
    fname->AddName(name);
    while(testnext('.')){
        fname->AddName(getName());
    }
    if(testnext(':')){
        fname->methodName = getName();
    }
    return fname;
}

/**
 * 编译函数定义语句
 * @return
 */

StatP Parser::parseFuncDefStat() {
    //首先讲函数名列表转化为var
    auto name = getName();
    bool isMethod = false;
    unique_ptr<Var> var = std::make_unique<SimpleVar>(name);
    ExprP exp = std::make_unique<VarExpr>(std::move(var));
    while(testnext('.')){
        var = std::make_unique<FieldVar>(std::move(exp), getName());
        exp = std::make_unique<VarExpr>(std::move(var));
    }
    if(testnext(':')){
        var = std::make_unique<FieldVar>(std::move(exp), getName());
        exp = std::make_unique<VarExpr>(std::move(var));
        isMethod = true;
    }

    var = std::move(static_cast<VarExpr*>(exp.get())->var);
    return std::make_unique<FuncDefStat>(std::move(var), parseFuncBody(), isMethod);

}

/**
 * 返回一个block，其中包含一个statement的列表
 * @return
 */
BlockP Parser::Parse() {
    auto block = parseBlock();
    check(Token::TK_EOS);
    return block;
}

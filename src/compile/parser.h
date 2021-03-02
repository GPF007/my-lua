//
// Created by gpf on 2020/12/29.
//

#ifndef LUUA2_PARSER_H
#define LUUA2_PARSER_H

#include "lexer.h"
#include "ast.h"

class Parser {
public:
    Parser(const char* fname);
    Parser(std::string content, int i);

    ExprP parseExpr();
    StatP parseStat();
    BlockP Parse();
    void reset(std::string content);

private:
    std::unique_ptr<Lexer> lexer;
    TokenPtr token_;
    TokenPtr nextToken_;


private:
    //error methods
    void errorExpected(int tokenType);
    void syntaxError(const char* msg);
    void semError(const char* msg);

    //check method
    void check(int c);
    void checknext(int c);
    void checkMatch(int what, int who, int where);
    bool testnext(int c);
    bool blockFollow();
    //next method
    void next();
    int lookahead();
    LuaString* getName();



    //parse methods
    //parse expressions
    //ExprP parseExpr();
    //ExprP parseSubExpr(int curPriority);
    std::pair<ExprP, int> parseSubExpr(int curPriority);
    unique_ptr<ExprList> parseExprList();
    ExprP parseSimpleExpr();
    ExprP parseSuffixedExpr();
    ExprP parseConstructor();
    ExprP parsePrimaryExpr();
    unique_ptr<ExprList> parseArgs();

    std::unique_ptr<Field> parseField();

    //statements
    BlockP parseBlock();
    StatP parseIfStat();
    StatP parseWhileStat();
    StatP parseForStat();
    StatP parseRetStat();
    StatP parseLocalStat();
    StatP parseExprStat();
    StatP parseLocalFunc();
    StatP parseFuncDefStat();
    unique_ptr<FuncBody> parseFuncBody();
    unique_ptr<FuncName> parseFuncName();


    //static function
    //helper method
    static UnOpr getunopr(int op);
    static BinOpr getbinopr(int op);

    
};


#endif //LUUA2_PARSER_H

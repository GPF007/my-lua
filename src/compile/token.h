//
// Created by gpf on 2020/12/24.
//

#ifndef LUUA2_TOKEN_H
#define LUUA2_TOKEN_H
#include "utils/typeAlias.h"
#include "object/luaString.h"

#define FIRST_RESERVED	257
#define NUM_RESERVED (int)(TK_WHILE-FIRST_RESERVED+1)

class Token {
    friend class Lexer;
public:

    enum TOKEN_TYPE {
        /* terminal symbols denoted by reserved words */
        TK_AND = FIRST_RESERVED, TK_BREAK,
        TK_DO, TK_ELSE, TK_ELSEIF, TK_END, TK_FALSE, TK_FOR, TK_FUNCTION,
        TK_GOTO, TK_IF, TK_IN, TK_LOCAL, TK_NIL, TK_NOT, TK_OR, TK_REPEAT,
        TK_RETURN, TK_THEN, TK_TRUE, TK_UNTIL, TK_WHILE,
        /* other terminal symbols */
        TK_IDIV, TK_CONCAT, TK_DOTS, TK_EQ, TK_GE, TK_LE, TK_NE,
        TK_SHL, TK_SHR,
        TK_DBCOLON, TK_EOS,
        TK_FLT, TK_INT, TK_NAME, TK_STRING
    };

public:
    Token(LuaInteger ival):type_(TK_INT),ival_(ival){}
    Token(LuaNumber nval):type_(TK_FLT),nval_(nval){}
    Token(LuaString* sval, TOKEN_TYPE ty):type_(ty),sval_(sval){}
    Token(TOKEN_TYPE ty):type_(ty){}
    Token(char ch):type_(ch){}

    std::string toString()  const;
    static std::string type2string(int tokenType);
    static void InitToken();

    //getters
    int type()      const   {return type_;}
    LuaNumber nval() const   {return nval_;}
    LuaInteger ival()   const {return ival_;}
    LuaString* sval()   const {return sval_;}


private:
    int type_;
    union {
        LuaNumber nval_;
        LuaInteger ival_;
        LuaString* sval_;
    };

};


#endif //LUUA2_TOKEN_H

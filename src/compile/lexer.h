//
// Created by gpf on 2020/12/24.
//

#ifndef LUUA2_LEXER_H
#define LUUA2_LEXER_H

#include "token.h"
#include <memory>

using TokenPtr = std::unique_ptr<Token>;

class Lexer {
public:
    enum LEXER_STATUS{
        LEXER_OK,
        LEXER_ERROR,
    };



public:

    Lexer(const char* fname);
    Lexer(std::string contant,int i);
    TokenPtr GetToken();
    Token* GetNextToken();
    void syntaxError(const char* msg,int tokenType){
        error(msg, tokenType);
    }
    void error(const char* msg, int tokenType);

    //getter
    int lineno()    const   {return lineno_;}
    int lastline()  const   {return lastline_;}
    void setlastline(int l)      {lastline_ = l;}


private:
    int ch_; //current char
    int lineno_;
    int lastline_;
    std::string buffer_;
    //TokenPtr curToken_;
    //TokenPtr nextToken_; //next token
    int status_; //表示是否有错误出现
    std::string errmsg_;

    //File part
    std::string source_;
    std::string content_;//文件所有的内容读取到当前字符串中
    int len_;
    int idx_;


private:
    //debug method
    std::string token2string(int tokenType);

    //some lexer method
    void next()  {ch_ = content_[idx_++];}
    void saveAndNext()  {buffer_.push_back(ch_); next();}
    bool isNewLine()        {return ch_ == '\n' || ch_ == '\r';}
    void incrLineNumber();
    bool curIs(int c);
    bool curIs2(const char* cset);
    int skipSeparator();

    void checkError(){
        if(status_ != LEXER_OK){
            fprintf(stderr,"%s",errmsg_.data());
            exit(EXIT_FAILURE);
        }
    }


    //一些read函数
    TokenPtr readNumeral();
    TokenPtr readLongString(int sep, bool isComment);
    TokenPtr readString(int del);

    LuaString* newString(const char* str, size_t l);

    TokenPtr lex();
};


#endif //LUUA2_LEXER_H

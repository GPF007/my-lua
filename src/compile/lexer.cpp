//
// Created by gpf on 2020/12/24.
//

#include "lexer.h"
#include <fmt/core.h>
#include <fstream>
#include <sstream>

namespace{
  bool isIdent(int ch){
      return isalnum(ch) || ch =='_';
  }
};

/**
 * 构造函数，输入一个文件名
 * @param fname
 */
Lexer::Lexer(const char *fname) {
    //首先读取文件内容
    source_ = std::string(fname);
    std::ifstream in(fname);
    if(!in){
        //error("file not exist!");
        throw std::runtime_error("File is not exist!");
    }
    std::stringstream ss;
    ss<<in.rdbuf();
    content_ = std::move(ss.str());
    //在内容后面加入一个-1
    content_.push_back(-1);
    len_ = content_.size();

    //然后初始化一些变量
    idx_ = 0;
    status_ = LEXER_OK;
    lineno_ = 0;
    lastline_ = 0;
    //curToken_ = std::make_unique<Token>(Token::TK_EOS);
    //nextToken_ = std::make_unique<Token>(Token::TK_EOS);
    next();

}

Lexer::Lexer(std::string contant,int i) {

    content_ = contant;
    source_ = "silly test";
    //在内容后面加入一个-1
    content_.push_back(-1);
    len_ = content_.size();

    //然后初始化一些变量
    idx_ = 0;
    status_ = LEXER_OK;
    lineno_ = 0;
    lastline_ = 0;
    //curToken_ = std::make_unique<Token>(Token::TK_EOS);
    //nextToken_ = std::make_unique<Token>(Token::TK_EOS);
    next();

}


void Lexer::error(const char *msg, int tokenType) {
    errmsg_ = fmt::format("{}{}{}",msg,source_, lineno_);
    if(tokenType){
        errmsg_.append(fmt::format("{} near {}",msg, token2string(tokenType)));
    }
    errmsg_.append("\n");
    status_ = LEXER_ERROR;
}

std::string Lexer::token2string(int tokenType) {
    //如果是有内容的四种类型，那么返回对应的内容，否则返回buffer
    switch (tokenType) {
        case Token::TK_NAME:
        case Token::TK_STRING:
        case Token::TK_INT:
        case Token::TK_FLT:{
            return buffer_;
        }
        default:{
            return Token::type2string(tokenType);
        }
    }
}
/*
void Lexer::syntaxError(Token* tok, const char *msg) {
    error(msg, tok->type_);
}
 */

/**
 * 创建一个LuaString对象
 * @param str 字符串内容
 * @param l 字符串长度
 * @return
 */
 //todo 用一个集合消除重复的字符串
LuaString * Lexer::newString(const char *str, size_t l) {
    auto strobj = LuaString::CreateString(str, l);
    return strobj;
}

/**
 *增长linenumber并且跳过所有的换行符
 */
void Lexer::incrLineNumber() {
    int old = ch_;
    ASSERT(isNewLine()); //当前的字符必须是 \r 或者 \n
    next();
    //\n\r 或者\r\n 即前后两个符号不一样
    if(isNewLine() && ch_ != old)
        next();
    ++lineno_;
}
/**
 * 检查当前字符是否是c，如果是返回true，然后读取下一个字符，否则返回false
 * @param c
 * @return
 */
bool Lexer::curIs(int c) {
    if(ch_ == c){
        next();
        return true;
    }
    return false;
}

/**
 * 检查当前字符是cset中的字符，如果是返回true，然后读取下一个字符，否则返回false
 * @param c
 * @return
 */
bool Lexer::curIs2(const char* cset) {
    ASSERT(cset[2]=='\0');
    if(ch_ == cset[0] || ch_ == cset[1]){
        next();
        return true;
    }
    return false;
}

/**
 * 读取一个数值类型的token然后返回
 * @return
 */
TokenPtr Lexer::readNumeral() {
    buffer_.clear();
    int first = ch_;
    buffer_.push_back(ch_);
    next();
    //检查是否是16进制
    const char* expo="Ee";
    bool isHex= false;
    bool isFloat = false;
    if(first =='0' && curIs2("xX")){
        isHex = true;
        expo="Pp";
    }

    //开始读取数字
    for(;;){
        //首先跳过指数符号和正负号
        if(curIs2(expo)){
            buffer_.push_back('e');
            isFloat = true;
            if(ch_ == '+' || ch_ == '-'){
                saveAndNext();
            }
        }
        if(isxdigit(ch_) || ch_ == '.'){
            if(ch_=='.') isFloat = true;
            buffer_.push_back(ch_);
            next();
        } else
            break;
    }

    //尝试转化为浮点数或者整数
    if(isFloat){
        LuaNumber num = std::stod(buffer_);
        return std::make_unique<Token>(num);
    }else{
        LuaInteger num;
        if(isHex)
            num = std::stoll(buffer_, nullptr, 16);
        else
            num = std::stoll(buffer_);
        return std::make_unique<Token>(num);
    }
}

/**
 * 跳过[=*[ 或者]=*] ，返回=的数量
 * @return count of =
 */
int Lexer::skipSeparator() {
    int count = 0;
    int delim = ch_;
    ASSERT(delim=='[' || delim==']');
    next();//? why save
    while(ch_ =='='){
        count++;
        next();
    }
    //如果没有闭合的][返回负数
    return ch_== delim?count:(-count-1);
}

/**
 * 返回一个长字符串对象的token
 * 长字符串对象以 [[ 开始 以 ]] 结束
 * @return
 */
TokenPtr Lexer::readLongString(int sep, bool isComment) {
    buffer_.clear();
    int line = lineno_;
    //buffer_.push_back(ch_);
    next();//跳过 第二个 [
    if(isNewLine())
        incrLineNumber();
    //开始读取字符串内容
    //跳出循环的条件是 遇到结尾的 ]]
    for(;;){
        checkError();
        switch (ch_) {
            case EOF:{
                //在读字符串的时候出现了EOF说明发生了错误
                auto msg = fmt::format("unfinished long string (starting at line {})", line);
                error(msg.data(), Token::TK_EOS);
                break;
            }
            case ']':{
                if(skipSeparator() == sep){
                    //saveAndNext();
                    next();
                    goto endloop;
                }
                break;
            }
            case '\n':
            case '\r':{
                buffer_.push_back('\n');
                incrLineNumber();
                break;
            }
            default:{
                saveAndNext();
                break;
            }
        }
    }

    endloop:
    if(isComment){
        return nullptr;
    }
    auto strobj = LuaString::CreateString(buffer_.data(), buffer_.size());
    return std::make_unique<Token>(strobj, Token::TK_STRING);

}

/**
 * 读取一个字符串对象
 * @param del 分隔符
 * @return
 */
TokenPtr Lexer::readString(int del) {
    buffer_.clear();
    //buffer_.push_back(ch_); //保存分隔符
    next();
    while(ch_ != del){
        checkError();
        switch (ch_) {
            case EOF:{
                error("unfinished string", Token::TK_EOS);
                break;
            }
            //短字符串不允许出现换行符
            case '\r': case '\n':{
                error("unfinihsed string", Token::TK_STRING);
                break;
            }
            //转义字符
            case '\\':{
                int c;
                //先保存下来\\字符，
                buffer_.push_back(ch_);
                next();
                switch (ch_) {
                    case 'a': c = '\a'; goto read_save;
                    case 'b': c = '\b'; goto read_save;
                    case 'f': c = '\f'; goto read_save;
                    case 'n': c = '\n'; goto read_save;
                    case 'r': c = '\r'; goto read_save;
                    case 't': c = '\t'; goto read_save;
                    case 'v': c = '\v'; goto read_save;
                    case '\n': case '\r':
                        incrLineNumber(); c= '\n'; goto only_save;
                    case '\\':
                    case '\"':
                    case '\'':
                        c = ch_; goto read_save;
                    case EOF:
                        goto done;
                    default:{
                        error("Unknown escaped char",0);
                        goto done;
                    }
                }

                read_save:
                    next();
                only_save:
                    buffer_.pop_back();
                    buffer_.push_back(c);
                done:
                break;
            }
            default:{
                //save and next
                buffer_.push_back(ch_); //保存分隔符
                next();
                break;
            }
        }
    }

    //跳过分隔符
    //saveAndNext();
    next();
    auto strobj = LuaString::CreateString(buffer_.data(), buffer_.size());
    return std::make_unique<Token>(strobj, Token::TK_STRING);
}


/**
 *主要的lexer函数，返回一个token
 * @return
 */

TokenPtr  Lexer::lex() {
    buffer_.clear();
    for(;;){
        checkError();

        switch (ch_) {
        case '\n': case '\r':{
            incrLineNumber();
            break;
        }
        case ' ': case '\f': case '\t': case '\v':{
            next();
            break;
        }
        case '-':{ //是否是单行注释或者是多行注释
            //注意这里如果是注释的话那么不会返回一个token，而是进行下一循环读取下一token
            next();
            if(ch_ != '-')
                return std::make_unique<Token>('-');
            next();
            //此时是两个--,可能是单行注释或者多行注释
            if(ch_ == '['){
                int sep = skipSeparator();
                if(sep >=0){
                    readLongString(sep, true);
                    break;

                }
            }
            //直到遇到换行符号
            while(!isNewLine() && ch_ !=EOF)
                next();
            break;
        }

        case '=':{
            next();
            if(curIs('='))
                return std::make_unique<Token>(Token::TK_EQ);
            return std::make_unique<Token>('=');
        }

        case '[':{
            int sep = skipSeparator();
            if(sep>=0){
                return readLongString(sep, false);
            }
            return std::make_unique<Token>('[');

        }

        case '<':{
            next();
            if(curIs('='))
                return std::make_unique<Token>(Token::TK_LE);
            else if(curIs('<'))
                return std::make_unique<Token>(Token::TK_SHL);
            return std::make_unique<Token>('<');
        }

        case '>':{
            next();
            if(curIs('='))
                return std::make_unique<Token>(Token::TK_GE);
            else if(curIs('>'))
                return std::make_unique<Token>(Token::TK_SHR);
            return std::make_unique<Token>('>');
        }

        case '/':{
            next();
            if(curIs('/'))
                return std::make_unique<Token>(Token::TK_IDIV);
            return std::make_unique<Token>('/');
        }
        case ':':{
            next();
            if(curIs(':'))
                return std::make_unique<Token>(Token::TK_DBCOLON);
            return std::make_unique<Token>(':');
        }
        case '~':{
            next();
            if(curIs('='))
                return std::make_unique<Token>(Token::TK_NE);
            return std::make_unique<Token>('~');
        }
        //短字符串
        case '"':
        case '\'':{
            return readString(ch_);
        }
        case '.':{
            saveAndNext();
            if(curIs('.')){
                if(curIs('.'))
                    return std::make_unique<Token>(Token::TK_DOTS);
                else
                    return std::make_unique<Token>(Token::TK_CONCAT);
            }else if(!isdigit(ch_))
                return std::make_unique<Token>('.');
            return readNumeral();
        }
        case '0'...'9':{
            return readNumeral();
        }
        case EOF:{
            return std::make_unique<Token>(Token::TK_EOS);
        }
        default: {
            if(isalpha(ch_) || ch_ =='_'){
                buffer_.clear();
                //如果当前字符是字母，那么说明是identifier或者关键字
                do{
                    saveAndNext();
                }while(isIdent(ch_));
                auto strobj = newString(buffer_.data(), buffer_.size());
                if(strobj->isReserved()){
                    return std::make_unique<Token>(static_cast<Token::TOKEN_TYPE>(strobj->extra() -1 + FIRST_RESERVED));
                }else{
                    return std::make_unique<Token>(strobj, Token::TK_NAME);
                }
            }else{//single token
                char c = ch_;
                next();
                return std::make_unique<Token>(c);
            }
        }
        }

    }
}


TokenPtr  Lexer::GetToken() {
    lastline_ = lineno_;
    return lex();

}




//
// Created by gpf on 2020/12/24.
//

#include "token.h"
#include <fmt/core.h>
static const char *const tokenNames[] = {
        "and", "break", "do", "else", "elseif",
        "end", "false", "for", "function", "goto", "if",
        "in", "local", "nil", "not", "or", "repeat",
        "return", "then", "true", "until", "while",
        "//", "..", "...", "==", ">=", "<=", "~=",
        "<<", ">>", "::", "<eof>",
        "<number>", "<integer>", "<name>", "<string>"
};

std::string Token::toString() const {
    if(type_ < FIRST_RESERVED){ //token是单个字符 如 + -, 等
        //此时type就是它的内容
        return fmt::format("'{}'", static_cast<char>(type_));
    }else{
        //const char*
        auto str = tokenNames[type_ - FIRST_RESERVED];
        //如果<TK_EOS说明是关键字，打印时加上单引号，否则直接输出s
        if(type_ < TK_EOS){
            return fmt::format("'{}'",str);
        }else{
            std::string res(str);
            res.append(": ");
            switch (type_) {
                case TK_STRING:
                case TK_NAME:   res.append(sval_->data());  break;
                case TK_INT:    res.append(std::to_string(ival_));  break;
                case TK_FLT:    res.append(std::to_string(nval_));  break;
            }
            return res;
        }
    }
}

/**
 * 根据传入的tokentype返回一个字符串表示
 * @param tokenType
 * @return
 */
std::string Token::type2string(int tokenType){
    if(tokenType < FIRST_RESERVED){ //token是单个字符 如 + -, 等
        //此时type就是它的内容
        return fmt::format("{}", char(tokenType));
    }else{
        //const char*
        auto str = tokenNames[tokenType - FIRST_RESERVED];
        //如果<TK_EOS说明是关键字，打印时加上单引号，否则直接输出s
        if(tokenType < TK_EOS){
            return fmt::format("'{}'",str);
        }else{
            return std::string(str);
        }
    }
}

/**
 * 将token字符串加入到字符串表中去，同时将保留字的LuaString对象中的extra字段
 * 设置为对应的标签
 */
void Token::InitToken() {
    for(int i=0;i<NUM_RESERVED;i++){
        auto strobj = LuaString::CreateString(tokenNames[i]);
        //extra表示它的标签
        strobj->setExtra(i+1);
    }
}


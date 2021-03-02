//
// Created by gpf on 2020/12/26.
//

#include <gtest/gtest.h>
#include "vm/globalState.h"
#include "compile/lexer.h"
#include <fmt/core.h>

TEST(add, simpleadd){
    EXPECT_EQ(1+2, 3);
}

TEST(lexer, keyword){
    std::string data = R"(
    and break do else elseif
        end false for function goto if
        in local nil not or repeat
        return then true until while
        // .. ... == >= <= ~=
        << >> ::
)";
    Lexer lexer(data, 0);
    int i=0;
    for(;;){
        auto tok = lexer.GetToken();
        if(tok->type() == Token::TK_EOS)
            break;
        //第一个关键字的type是从First_RESERVED开始的
        //fmt::print("token: {}\n", tok->toString());
        EXPECT_EQ(tok->type(), i+ FIRST_RESERVED);
        i++;
    }
}


TEST(lexer, identifier){
    std::string data = R"(
    foo ident myclass2 num2string HHHhh
)";
    Lexer lexer(data, 0);
#define EXPECT_IDENT(str)   \
    tok = lexer.GetToken();     \
    EXPECT_EQ(tok->type(), Token::TK_NAME);    \
    EXPECT_STREQ(tok->sval()->data(), str);


    TokenPtr tok;
    EXPECT_IDENT("foo");
    EXPECT_IDENT("ident");
    EXPECT_IDENT("myclass2");
    EXPECT_IDENT("num2string");
    EXPECT_IDENT("HHHhh");
}

TEST(lexer, integer){
    std::string data = R"(
        0 123458 0x80 115
    )";
    Lexer lexer(data, 0);

    auto expect_integer = [&lexer](LuaInteger val){
        auto tok = lexer.GetToken();
        EXPECT_EQ(tok->type(), Token::TK_INT);
        EXPECT_EQ(tok->ival(), val);
    };
    expect_integer(0);
    expect_integer(123458);
    expect_integer(128);
    expect_integer(115);
}


TEST(lexer, number){
    std::string data = R"(
        0.0001 3.1415926 1e-5 12.0
    )";
    Lexer lexer(data, 0);

    auto expect_number = [&lexer](LuaNumber val){
        auto tok = lexer.GetToken();
        EXPECT_EQ(tok->type(), Token::TK_FLT);
        EXPECT_EQ(tok->nval(), val);
    };
    expect_number(0.0001);
    expect_number(3.1415926);
    expect_number(0.00001);
    expect_number(12.0);
}

TEST(lexer, string){
    std::string data = R"(
        "hello" 'world'
        [[ hello \n \t ]]
        [[ line one
line two ]]
    )";
    Lexer lexer(data, 0);

    auto expect_string = [&lexer](const char* str){
        auto tok = lexer.GetToken();
        EXPECT_EQ(tok->type(), Token::TK_STRING);
        EXPECT_STREQ(tok->sval()->data(), str);
    };
    expect_string("hello");
    expect_string("world");
    expect_string(" hello \\n \\t ");
    expect_string(" line one\nline two ");
}


int main(int argc, char **argv)
{
    // 分析gtest程序的命令行参数
    GlobalState::InitAll();
    testing::InitGoogleTest(&argc, argv);



    // 调用RUN_ALL_TESTS()运行所有测试用例
    // main函数返回RUN_ALL_TESTS()的运行结果
    return RUN_ALL_TESTS();
}

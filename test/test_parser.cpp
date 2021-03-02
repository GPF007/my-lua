//
// Created by gpf on 2020/12/26.
//

#include <gtest/gtest.h>
#include "vm/globalState.h"
#include "compile/parser.h"
#include <fmt/core.h>

#define EXPECT_PARSE(source, result)        \
        parser.reset(source);              \
        EXPECT_STREQ(parser.parseExpr()->toString().data(), result);    \

#define EXPECT_PARSE_STAT(source, result)        \
        parser.reset(source);              \
        EXPECT_STREQ(parser.parseStat()->toString().data(), result);    \


TEST(add, simpleadd){
    EXPECT_EQ(1+2, 3);
}

TEST(parser, sillyparse){
    Parser parser("3+4*5+2/123",0);
    auto expr = parser.parseExpr();
    //fmt::print("{}\n",expr->toString());

}

TEST(parser, binaryop){
    Parser parser("2+3",0);
    EXPECT_STREQ(parser.parseExpr()->toString().data(), "add{int{2}int{3}}");
    EXPECT_PARSE("2+3","add{int{2}int{3}}")
    EXPECT_PARSE("false or true and false", "or{bool{false}and{bool{true}bool{false}}}")

}

TEST(parser, suffixexp){
    Parser parser("2+3",0);
    EXPECT_PARSE("a+b","add{simvar{a}simvar{b}}")
    EXPECT_PARSE("a.b.c","fieldvar{fieldvar{simvar{a}b}c}")
    EXPECT_PARSE("a[3+4]","subvar{simvar{a}add{int{3}int{4}}}")
    EXPECT_PARSE("a[b].c","fieldvar{subvar{simvar{a}simvar{b}}c}")

    //table constructor
    EXPECT_PARSE("{}","fields{}")
    EXPECT_PARSE("{1,2,3,a,b, a.b}","fields{sinfield{int{1}},sinfield{int{2}},sinfield{int{3}},sinfield{simvar{a}},sinfield{simvar{b}},sinfield{fieldvar{simvar{a}b}}}");

    EXPECT_PARSE("{[\"a\"]=3, [\"b\"]=5, c=7}","fields{exprfield{str{a}int{3}},exprfield{str{b}int{5}},exprfield{str{c}int{7}}}");
}

TEST(parser, funcall){
    Parser parser("2+3",0);
    EXPECT_PARSE("add(1,3, a+b)","funcall{simvar{add}ExprList{int{1}, int{3}, add{simvar{a}simvar{b}}}}");
    EXPECT_PARSE("foo()","funcall{simvar{foo}ExprList{}}");
    EXPECT_PARSE("foo{1}","funcall{simvar{foo}ExprList{fields{sinfield{int{1}}}}}")
    EXPECT_PARSE("foo \"hello\"","funcall{simvar{foo}ExprList{str{hello}}}")
    EXPECT_PARSE("A:foo (4)","selfcall{simvar{A}fooExprList{int{4}}}")
}


TEST(parser, statement){
    Parser parser("dd",0);
    EXPECT_PARSE_STAT(";","empty{}")
    EXPECT_PARSE_STAT("break","break{}")

    //Expr stat
    EXPECT_PARSE_STAT("a,b=1,2","assign{simvar{a},simvar{b}|ExprList{int{1}, int{2}}}")
    EXPECT_PARSE_STAT("foo(1,2,c,...)",
            "funstat{funcall{simvar{foo}ExprList{int{1}, int{2}, simvar{c}, ...}}}");

    //Do Stat
    EXPECT_PARSE_STAT("do a = b+c add(3,4) end",
            "do{block{assign{simvar{a}|ExprList{add{simvar{b}simvar{c}}}},funstat{funcall{simvar{add}ExprList{int{3}, int{4}}}}}}");

    //While Stat
    EXPECT_PARSE_STAT("while i<100 do i=i+1 end",
                      "while{lt{simvar{i}int{100}}|block{assign{simvar{i}|ExprList{add{simvar{i}int{1}}}}}}");

    //If Stat
    EXPECT_PARSE_STAT("if a<10 then print(a) end",
            "if{lt{simvar{a}int{10}}|block{funstat{funcall{simvar{print}ExprList{simvar{a}}}}}}");
    EXPECT_PARSE_STAT("if a<0 then a = -a elseif a>0 then a = a else a = 0 end",
                      "if{lt{simvar{a}int{0}}|block{assign{simvar{a}|ExprList{minus{simvar{a}}}}}elif{gt{simvar{a}int{0}}|block{assign{simvar{a}|ExprList{simvar{a}}}}}|block{assign{simvar{a}|ExprList{int{0}}}}}");

    //For Stat
    //for eq
    EXPECT_PARSE_STAT("for i=1,10 do print(i*2) end ","foreq{i|int{1}|int{10}}|block{funstat{funcall{simvar{print}ExprList{mul{simvar{i}int{2}}}}}}")
    EXPECT_PARSE_STAT("for i=1,10,2 do print(i/10) end ","foreq{i|int{1}|int{10}}|int{2}|block{funstat{funcall{simvar{print}ExprList{div{simvar{i}int{10}}}}}}")
    //for in
    EXPECT_PARSE_STAT("for k,v in map do add(k,v) end","forin{k,v|ExprList{simvar{map}}|block{funstat{funcall{simvar{add}ExprList{simvar{k}, simvar{v}}}}}}")

    //Function Def
    EXPECT_PARSE_STAT("function add(a,b) return a+b end","funcdef{simvar{add}|funcbody{a,b|block{ret{ExprList{add{simvar{a}simvar{b}}}}}}}")
    EXPECT_PARSE_STAT("function add() return 0 end","funcdef{simvar{add}|funcbody{|block{ret{ExprList{int{0}}}}}}")
    EXPECT_PARSE_STAT("function add(...) return 0 end","funcdef{simvar{add}|funcbody{...|block{ret{ExprList{int{0}}}}}}")
    EXPECT_PARSE_STAT("function add(a,...) return 0 end","funcdef{simvar{add}|funcbody{a,...|block{ret{ExprList{int{0}}}}}}")
    EXPECT_PARSE_STAT("function arith.add() return 0 end","funcdef{fieldvar{simvar{arith}add}|funcbody{|block{ret{ExprList{int{0}}}}}}")

    //Local stat
    EXPECT_PARSE_STAT("local function add(a,b) return a+b end","locfunc{add|funcbody{a,b|block{ret{ExprList{add{simvar{a}simvar{b}}}}}}}")
    EXPECT_PARSE_STAT("local a,b =1,0","locstat{a,b|ExprList{int{1}, int{0}}}")
    EXPECT_PARSE_STAT("local a,b","locstat{a,b}")
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

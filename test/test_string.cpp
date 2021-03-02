//
// Created by gpf on 2020/12/9.
//

#include <gtest/gtest.h>
#include "vm/globalState.h"
#include "object/stringTable.h"
#include "object/luaString.h"

TEST(add, simpleadd){
    EXPECT_EQ(1+2, 3);
}
/*
TEST(LuaString, CreateString){
    auto& strtable = GlobalState::stringTable;
    const char* str1 = "hello world";
    auto ls1 = LuaString::CreateString(str1);
    EXPECT_STREQ(ls1->data(), str1);
    EXPECT_EQ(ls1->shrlen(), strlen(str1));
    EXPECT_EQ(strtable.nuse(), 1);

    const char* str2 = "asdasdasdasd";
    auto ls2 = LuaString::CreateString(str2);
    EXPECT_STREQ(ls2->data(), str2);
    EXPECT_EQ(ls2->shrlen(), strlen(str2));

    //测试字符表长度应该为2 nused
    EXPECT_EQ(strtable.nuse(), 2);

    LuaString::CreateString("hello world");
    EXPECT_EQ(strtable.nuse(), 2);

    //测试长字符串
    const char* str3 = "1234567890 1234567890 1234567890 1234567890 2";
    auto ls3 = LuaString::CreateString(str3);
    EXPECT_STREQ(ls3->data(), str3);
    EXPECT_EQ(ls3->lnglen(), strlen(str3));
    EXPECT_EQ(strtable.nuse(), 2);

}
 */

//测试字符串表的扩容
TEST(LuaString, EqualString){
    auto& strtable = GlobalState::stringTable;
    const char* str1 = "str1";
    const char* str2 = "str2";
    const char* str3 = "str3";
    const char* str4 = "str4";
    const char* str5 = "str5";
    auto ls1 = LuaString::CreateString(str1);
    auto ls2 = LuaString::CreateString(str2);
    auto ls3 = LuaString::CreateString(str3);
    auto ls4 = LuaString::CreateString(str4);

    EXPECT_EQ(strtable.nuse(), 4);
    EXPECT_EQ(strtable.size(), 4);

    auto ls5 = LuaString::CreateString(str5);
    EXPECT_EQ(strtable.nuse(), 5);
    EXPECT_EQ(strtable.size(), 8);
    EXPECT_STREQ(ls1->data(), "str1");
    printf("Used memory:0x%x\n", Allocator::usedMemory);

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


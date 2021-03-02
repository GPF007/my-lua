//
// Created by gpf on 2020/12/10.
//

#include <gtest/gtest.h>
#include "vm/globalState.h"
#include "object/stringTable.h"
#include "object/luaString.h"
#include "object/luaTable.h"

TEST(add, simpleadd){
    EXPECT_EQ(1+2, 3);
}



//测试字符串表的扩容
TEST(LuaTable, CreateTable){
    auto tbl = LuaTable::CreateTable();
    tbl->resize(6,10);
    EXPECT_EQ(tbl->sizearray(), 6);
    EXPECT_EQ(tbl->lsizenode(), 4);
    EXPECT_EQ(tbl->sizenode(), 16);

    //test get
    auto key1 = Value(1);
    auto val1 = tbl->Get(&key1);
    EXPECT_EQ(val1->type, LUA_TNIL);
    //插入5个数
    for(int i=1;i<=6;i++){
        auto val = Value(i);
        tbl->setInt(i, &val);
    }

    for(int i=1;i<=6;i++){
        auto key = Value(i);
        EXPECT_EQ(tbl->getInt(i)->ival, i);
        EXPECT_EQ(tbl->Get(&key)->ival, i);
    }

    auto str1 = LuaString::CreateString("str1");
    auto key2 = Value();
    key2.type = LUA_TSHRSTR;
    key2.obj = str1;
    auto val2 = tbl->Get(&key2);
    EXPECT_EQ(val2->type, LUA_TNIL);
    //插入 5个字符串
    std::string tmp="str";
    for(int i=0;i<5;i++){
        auto str = tmp + std::to_string(i);
        auto strobj = LuaString::CreateString(str.data());
        auto key = Value();
        key.type = LUA_TSHRSTR;
        key.obj = strobj;
        *tbl->Set(&key) = Value(i);
    }
    //检测五个字符串已经插入成功
    for(int i=0;i<5;i++){
        auto str = tmp + std::to_string(i);
        auto strobj = LuaString::CreateString(str.data());
        auto key = Value();
        key.type = LUA_TSHRSTR;
        key.obj = strobj;
        EXPECT_EQ(tbl->getShortString(strobj)->ival, i);
        EXPECT_EQ(tbl->Get(&key)->ival, i);
    }

    //插入一个长字符串
    auto longstr = LuaString::CreateString("1234567890112345678901123456789011234567890112345678901");
    auto longkey = Value();
    longkey.type = LUA_TLNGSTR;
    longkey.obj = longstr;
    EXPECT_EQ(tbl->Get(&longkey)->type, LUA_TNIL );
    *tbl->Set(&longkey) = Value(12138);
    EXPECT_EQ(tbl->Get(&longkey)->ival, 12138);

    LuaTable::DestroyTable(tbl);

    printf("Used memory:0x%lx\n", Allocator::usedMemory);

}


TEST(LuaTable, rehash){
    //rehash table
    auto tbl = LuaTable::CreateTable();
    tbl->resize(2,0);
    EXPECT_EQ(tbl->sizearray(), 2);
    EXPECT_EQ(tbl->lsizenode(), 0);
    EXPECT_EQ(tbl->sizenode(), 1);

    //test get
    auto key1 = Value(1);
    auto val1 = tbl->Get(&key1);
    EXPECT_EQ(val1->type, LUA_TNIL);
    //插入10个数
    for(int i=1;i<=10;i++){
        auto val = Value(i);
        tbl->setInt(i, &val);
    }

    for(int i=1;i<=10;i++){
        auto key = Value(i);
        EXPECT_EQ(tbl->getInt(i)->ival, i);
        EXPECT_EQ(tbl->Get(&key)->ival, i);
    }


    EXPECT_EQ(tbl->sizearray(), 16);
    EXPECT_EQ(tbl->lsizenode(), 0);
    EXPECT_EQ(tbl->sizenode(), 1);

    //test rehash hash part
    std::string tmp="str";
    for(int i=0;i<20;i++){
        auto str = tmp + std::to_string(i);
        auto strobj = LuaString::CreateString(str.data());
        auto key = Value();
        key.type = LUA_TSHRSTR;
        key.obj = strobj;
        *tbl->Set(&key) = Value(i);
    }
    //检测五个字符串已经插入成功
    for(int i=0;i<20;i++){
        auto str = tmp + std::to_string(i);
        auto strobj = LuaString::CreateString(str.data());
        auto key = Value();
        key.type = LUA_TSHRSTR;
        key.obj = strobj;
        EXPECT_EQ(tbl->getShortString(strobj)->ival, i);
        EXPECT_EQ(tbl->Get(&key)->ival, i);
    }

    EXPECT_EQ(tbl->sizearray(), 16);
    EXPECT_EQ(tbl->lsizenode(), 5);
    EXPECT_EQ(tbl->sizenode(), 32);


}

//test next 迭代器
TEST(LuaTable, next){
    //rehash table
    auto tbl = LuaTable::CreateTable();
    tbl->resize(2,0);
    EXPECT_EQ(tbl->sizearray(), 2);
    EXPECT_EQ(tbl->lsizenode(), 0);
    EXPECT_EQ(tbl->sizenode(), 1);

    //插入10个数
    for(int i=1;i<=10;i++){
        auto val = Value(i);
        tbl->setInt(i, &val);
    }
    //插入10个字符串
    std::string tmp="str";
    for(int i=0;i<10;i++){
        auto str = tmp + std::to_string(i);
        auto strobj = LuaString::CreateString(str.data());
        auto key = Value(strobj, strobj->type());
        Value val;
        val.type = LUA_TSHRSTR;
        val.obj = strobj;
        *tbl->Set(&key) = val;
    }

    Value key;
    Value val;
    int i=0;
    while(tbl->Next(&key, &val)){
        printf("%d:\t",i++);
        if(val.isInteger()){
            printf("Integer: %lld\n",val.ival);
        }else if(val.isShortString()){
            printf("ShortString: %s\n", static_cast<LuaString*>(val.obj)->data());
        }else{
            printf("Unkonwn");
        }
        //if(i>=10)
        //    break;
    }



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


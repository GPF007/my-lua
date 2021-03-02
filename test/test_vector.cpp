//
// Created by gpf on 2020/12/14.
//


#include <gtest/gtest.h>
#include "vm/globalState.h"
#include "object/stringTable.h"
#include "object/luaString.h"


#include "utils/Array.h"
#include "utils/Vector.h"

TEST(add, simpleadd){
    EXPECT_EQ(1+2, 3);
}

TEST(Array, arraytest){
    Array<int> intArr(10, 1);
    for(int i=0;i<10;i++){
        EXPECT_EQ(intArr[i], 1);
    }

    Array<LuaString*> strArr(10, LuaString::CreateString("HELLO"));
    for(int i=0;i<10;i++){
        EXPECT_STREQ("HELLO", strArr[i]->data());
    }

    auto strArr2 = strArr;
    for(int i=0;i<10;i++){
        EXPECT_STREQ("HELLO", strArr2[i]->data());
    }

}

TEST(Vector, vectortest){
    Vector<int> intVec(4,1);
    for(int i=0;i<4;i++){
        EXPECT_EQ(1, intVec[i]);
    }
    EXPECT_EQ(intVec.size(), 4);
    EXPECT_EQ(intVec.cap(), 8);

    //test append
    for(int i=0;i<10;i++){
        intVec.push_back(1);
    }
    EXPECT_EQ(intVec.size(), 14);
    EXPECT_EQ(intVec.cap(), 16);
    for(int i=0;i<intVec.size();i++){
        EXPECT_EQ(intVec[i],1);
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


# 查找 GTest 库
find_package(GTest REQUIRED)
# GTest 的头文件
include_directories(${GTEST_INCLUDE_DIRS})




add_executable(test_string test_string.cpp  ${src_list})
add_executable(test_table test_table.cpp  ${src_list})
add_executable(test_vector test_vector.cpp  ${src_list})
add_executable(test_lexer test_lexer.cpp  ${src_list})
add_executable(test_parser test_parser.cpp  ${src_list})



#add_executable(test_compile test_compile.cpp  ${source})

#设置输出目录
#set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#undump 程序，用来读取chunk
#add_executable(undump show_chunk.cpp ${source})
#target_link_libraries(undump libfmt.a)


# 链接测试库

target_link_libraries( test_string
        ${GTEST_BOTH_LIBRARIES}
        pthread libfmt.a)

target_link_libraries( test_table
        ${GTEST_BOTH_LIBRARIES}
        pthread libfmt.a)

target_link_libraries( test_vector
        ${GTEST_BOTH_LIBRARIES}
        pthread libfmt.a)
target_link_libraries( test_lexer
        ${GTEST_BOTH_LIBRARIES}
        pthread libfmt.a)

target_link_libraries( test_parser
        ${GTEST_BOTH_LIBRARIES}
        pthread libfmt.a)



# 添加到测试
gtest_discover_tests(test_string)
gtest_discover_tests(test_table)
gtest_discover_tests(test_lexer)
gtest_discover_tests(test_parser)
#gtest_discover_tests(test_compile)
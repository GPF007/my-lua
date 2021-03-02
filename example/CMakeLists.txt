


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
add_executable(show_chunk show_chunk.cpp  ${src_list})
add_executable(tokenlizer tokenlizer.cpp  ${src_list})
add_executable(compile compile.cpp  ${src_list})
add_executable(exevm exevm.cpp  ${src_list})
add_executable(run run.cpp  ${src_list})



# 链接库


target_link_libraries( show_chunk libfmt.a)
target_link_libraries( tokenlizer libfmt.a)
target_link_libraries( compile libfmt.a)
target_link_libraries( run libfmt.a)

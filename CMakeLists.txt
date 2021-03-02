cmake_minimum_required(VERSION 3.16)
project(luua2)

set(CMAKE_CXX_STANDARD 14)
add_definitions(-Wall)
include_directories(${PROJECT_SOURCE_DIR}/src)
enable_testing()


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#开启debug参数
#add_definitions(-DDEBUG)

aux_source_directory(${PROJECT_SOURCE_DIR}/src/memory source_memory)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/object source_object)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/utils source_utils)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/vm source_vm)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/dump source_dump)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/lib source_lib)
aux_source_directory(${PROJECT_SOURCE_DIR}/src/compile source_compile)

set(src_list ${source_memory} ${source_object} ${source_utils} ${source_vm} ${source_dump} ${source_lib} ${source_compile})
add_executable(luua2 main.cpp  ${src_list})
#add_executable(luua2 main.cpp  ${source_vm} ${source_memory} ${source_utils} ${source_object} ${source_dump}  ${source_lib} ${source_compile})

target_link_libraries(luua2 libfmt.a)

add_subdirectory(test)
add_subdirectory(example)
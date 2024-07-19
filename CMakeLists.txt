# CMake 最低版本号要求
cmake_minimum_required (VERSION 3.10.0)
set(CMAKE_EXE_LINKER_FLAGS "-static")

# 项目信息
project(MySFS)
#include
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
#源代码
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src SRC_LIST) 
# 指定生成目标
ADD_EXECUTABLE(MySFS ${SRC_LIST})
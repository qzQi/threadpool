cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_STANDARD 17)
# set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
    message("Setting default build type to Release")
endif()

project(my_project_name VERSION 0.0.1 LANGUAGES C CXX)

# 设置exe文件到一块build/bin 没有这一行的话生成的是在build下面各自的dir里面
# eg：src里面的还是在build/src里
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
# 设置库文件输出路径
# Old library location variable.,现在使用的是 output directory
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)


# 向工程添加多个特定的头文件搜索路径。 相当于我们使用g++时候的-I参数
include_directories(${PROJECT_SOURCE_DIR}/include)

# # 设置链接库的寻找路径，相当于g++的 -L参数指定寻找库文件路径
# link_directories(${PROJECT_SOURCE_DIR}/lib)



add_subdirectory(./src)

add_subdirectory(./test)

add_subdirectory(./example)

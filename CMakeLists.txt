cmake_minimum_required(VERSION 2.6)

project(fileserver CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "debug")
endif()

if(NOT MUDUO_PATH)
    set(MUDUO_PATH "/home/ubuntu/cpp/build/release-install-cpp11")
endif()

set(CXX_FLAGS
    -g
    # -DVALGRIND
    # -DMUDUO_STD_STRING
    # -Wall
    # -Wextra
    # -m32
    # -Werror
    # -Wconversion
    # -Wno-unused-parameter
    # -Wold-style-cast
    # -Woverloaded-virtual
    # -Wpointer-arith
    # -Wshadow
    # -Wwrite-strings
    # -march=native
    # -Werror=shadow
    # -MMD
    # -std=c++0x
    -rdynamic
    -std=c++11
)
# message("${CXX_FLAGS}")
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

set(CMAKE_CXX_COMPILLER "g++")
set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -finline-limit=1000 -DNDEBUG")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin) #project_binary_dir是执行cmake的目录
set(LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)

#指定头文件dir和库文件的path(-Ldir -llibrary -Idir)
# find_path, find_library find_file  || CMAKE_library_path CMAKE_include_path
# find_package: 找到include_dirs和libraries两个 => 使用CMAKE_MODULE_PATH和XXX_DIR
# targer_include_directory target_link_libraries
# include_directory link_libraries

find_path(Muduo_INCLUDE_DIR muduo ${MUDUO_PATH}/include)#没找到 不会设置
find_path(Muduo_LIBRARY_DIR libmuduo_net.a ${MUDUO_PATH}/lib) #找到库的目录
# message(STATUS ${Muduo_LIBRARY_DIR}) #变量缓存问题 可以删除binatry_dir的东西, 从而达到重新编译的目的
set(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} ${Muduo_LIBRARY_DIR})
find_library(muduo_base muduo_base)
find_library(muduo_net muduo_net) #找到库 后面需要链接
find_library(muduo_http muduo_http) # 递归查找目录
message(STATUS ${muduo_http})
message(STATUS ${muduo_net})
include_directories(${Muduo_INCLUDE_DIR}) #找到并设置 include目录

find_package(Boost REQUIRED) #需要找到包的lib和include, lib后面链接 include需要include, target_xxx
include_directories(${Boost_INCLUDE_DIRS}) #加入include 目录

include_directories(${PROJECT_SOURCE_DIR}) # 头文件的搜索目录\
aux_source_directory(. MAIN) # 搜索当前目录下的所有.cpp文件
message(STATUS ${MAIN})

include_directories(${PROJECT_SOURCE_DIR}/base)
aux_source_directory(./base BASE)
message(STATUS ${BASE})

add_executable(main.o ${MAIN} ${BASE})



# target_link_libraries(server1 ${muduo_net})
# target_link_libraries(server1 ${muduo_base})
# target_link_libraries(server1 ${muduo_http}) #和父目录是不同的库文件 .a文件(静态) .so(动态)
target_link_libraries(main.o pthread rt) # muduo依赖需要的动态库 静态库依赖的动态库 就是 自己的动态库 需要手动指定; 因为静态库的lib在链接的时候, 不会查找需要的ddl, 需要在后面手动导入; 动态库在链接会自动查找ddl;
# # 对外暴露 动态库, 这样不会需要库的依赖
# # ldd

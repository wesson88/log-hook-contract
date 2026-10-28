cmake_minimum_required(VERSION 3.22)

project(loghookcontract VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(LhcCompileOptions)

# 契约只有头文件：INTERFACE 目标。消费方以 FetchContent 钉 tag 引入后链接 lhc::lhc。
add_library(lhc INTERFACE)
add_library(lhc::lhc ALIAS lhc)
target_include_directories(lhc INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")

# 作为子项目被消费方引入时不构建本仓测试
if(PROJECT_IS_TOP_LEVEL)
  option(LHC_BUILD_TESTS "Build log-hook-contract tests" ON)
else()
  option(LHC_BUILD_TESTS "Build log-hook-contract tests" OFF)
endif()

if(LHC_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

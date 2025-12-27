cmake_minimum_required(VERSION 3.12...3.26)

# ======================================
# 🚨 最高优先级：解决核心问题的4个关键全局配置（必须在project前）
# ======================================
# 1. 根治CUDA20：分离CXX/CUDA标准，禁用扩展语法
set(CMAKE_CXX_EXTENSIONS OFF CACHE BOOL "")
set(CMAKE_CUDA_EXTENSIONS OFF CACHE BOOL "")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

# 2. 🚨 强制锁定CUDA13.1编译器路径（解决多版本冲突，核心中的核心）
set(CMAKE_CUDA_COMPILER "/usr/local/cuda-13.1/bin/nvcc" CACHE PATH "" FORCE)
# 3. 🚨 强制指定CUDA架构sm_86，禁用CMake默认的sm_52检测（解决架构报错）
set(CMAKE_CUDA_ARCHITECTURES 86 CACHE STRING "" FORCE)
# 4. 指定CUDA宿主编译器为GCC12（兼容CUDA13.1，避免版本匹配警告）
set(CMAKE_CUDA_HOST_COMPILER "/usr/bin/gcc-12" CACHE PATH "" FORCE)

# 默认编译模式为Release
get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT is_multi_config AND NOT (CMAKE_BUILD_TYPE OR DEFINED ENV{CMAKE_BUILD_TYPE}))
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ======================================
# 项目初始化（CXX+CUDA，保留CUDA编译能力）
# ======================================
project(tinyrenderer LANGUAGES CXX CUDA)

# ======================================
# 编译器参数配置（兼容GCC12，屏蔽冗余警告）
# ======================================
# C++编译参数（仅.cpp生效）
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|Intel")
  add_compile_options(-Wall)
endif()

# CUDA编译参数（仅.cu生效，关键：allow-unsupported-compiler兼容GCC12）
set(CMAKE_CUDA_FLAGS "${CMAKE_CUDA_FLAGS} -allow-unsupported-compiler -w")

# ======================================
# 依赖查找（OpenMP + SDL2，保留原有功能）
# ======================================
find_package(OpenMP COMPONENTS CXX)
find_package(SDL2 REQUIRED)

# ======================================
# 源码列表（保留所有文件，model.cu正常编译）
# ======================================
set(SOURCES 
    src/main.cpp
    src/realtime.cpp
    src/basic/tgaimage.cpp 
    src/pipeline/model.cu
    src/pipeline/camera.cpp
    src/pipeline/rasterization.cpp
    src/pipeline/shader/BlinnPhong.cpp
    src/pipeline/shader/random.cpp
    src/tools/filereader.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# ======================================
# 目标链接（精简无冲突，自动链接CUDA核心库）
# ======================================
# 链接OpenMP
if(OpenMP_CXX_FOUND)
    target_link_libraries(${PROJECT_NAME} PRIVATE OpenMP::OpenMP_CXX)
endif()
# 链接SDL2
target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2)

# ======================================
# 兜底保障：强制.cu文件用CUDA编译，确保核函数生效
# ======================================
set_source_files_properties(src/pipeline/model.cu PROPERTIES LANGUAGE CUDA)

file(GENERATE OUTPUT .gitignore CONTENT "*")

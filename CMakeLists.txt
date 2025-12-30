cmake_minimum_required(VERSION 3.12...3.26)

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT is_multi_config AND NOT (CMAKE_BUILD_TYPE OR DEFINED ENV{CMAKE_BUILD_TYPE}))
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Release default")
endif()

project(tinyrenderer LANGUAGES CXX CUDA)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)

option(iwyu "Run include-what-you-use")
if(iwyu)
  find_program(IWYU_EXE NAMES include-what-you-use REQUIRED)
  set(CMAKE_CXX_INCLUDE_WHAT_YOU_USE ${IWYU_EXE})
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU|Intel")
  add_compile_options(-Wall)
endif()

find_package(OpenMP COMPONENTS CXX)
find_package(SDL2 REQUIRED)

set(SOURCES
  src/main.cpp
  src/realtime.cpp
  src/basic/tgaimage.cpp
  src/pipeline/model.cpp
  src/pipeline/camera.cpp
  src/pipeline/rasterization.cpp
  src/pipeline/shader/BlinnPhong.cpp
  src/pipeline/shader/random.cpp
  src/tools/filereader.cpp
  src/pipeline/kernel.cu
  src/pipeline/model.cu
  src/pipeline/rasterization.cu
  src/pipeline/upload.cu
  src/pipeline/clear.cu
  src/pipeline/shader/shader.cu
)

add_executable(${PROJECT_NAME} ${SOURCES})

# CUDA 13.1 路径（只用这一套）
set(CUDA_LOCAL_ROOT "/usr/local/cuda-13.1")

# 强制 include 优先使用 CUDA 13.1
target_include_directories(${PROJECT_NAME} BEFORE PRIVATE
  ${CUDA_LOCAL_ROOT}/include
  ${CUDA_LOCAL_ROOT}/targets/x86_64-linux/include
)

# 手工找 cudart
find_library(CUDART_LIB cudart HINTS
  ${CUDA_LOCAL_ROOT}/lib64
  ${CUDA_LOCAL_ROOT}/targets/x86_64-linux/lib
  ${CUDA_LOCAL_ROOT}/targets/x86_64-linux/lib/stubs
  REQUIRED
)

set_target_properties(${PROJECT_NAME} PROPERTIES
  CUDA_SEPARABLE_COMPILATION ON
)

target_link_libraries(${PROJECT_NAME} PRIVATE
  $<$<BOOL:${OpenMP_CXX_FOUND}>:OpenMP::OpenMP_CXX>
  SDL2::SDL2
  ${CUDART_LIB}
)

file(GENERATE OUTPUT .gitignore CONTENT "*")

cmake_minimum_required(VERSION 3.25)
project(nexus VERSION 1.0.0 LANGUAGES CXX)

# ═══════════════════════════════════════════════════════════════════
# Nexus 神经系统 — CMake 构建系统
# ═══════════════════════════════════════════════════════════════════

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ── 选项 ──
option(NEXUS_BUILD_TESTS "构建测试" ON)
option(NEXUS_CUDA_SUPPORT "启用 CUDA 支持" ON)
option(NEXUS_BUILD_EXAMPLES "构建示例" OFF)

# ── 输出目录 ──
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# ── 编译器选项 ──
if(MSVC)
  add_compile_options(/W4 /utf-8 /permissive- /Zc:__cplusplus)
  add_compile_options(/wd4100)  # 'identifier' : unreferenced formal parameter
  add_compile_options(/wd4189)  # local variable is initialized but not referenced
  add_link_options(/LTCG)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_definitions(NEXUS_NDEBUG)
endif()

# ── CUDA ──
if(NEXUS_CUDA_SUPPORT)
  find_package(CUDAToolkit REQUIRED)
  message(STATUS "CUDA Toolkit: ${CUDAToolkit_VERSION} at ${CUDAToolkit_DIR}")
  add_compile_definitions(NEXUS_CUDA_ENABLED)
endif()

# ── 第三方依赖 (vcpkg) ──
find_package(fmt CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

message(STATUS "fmt: ${fmt_VERSION}")
message(STATUS "spdlog: ${spdlog_VERSION}")
message(STATUS "nlohmann_json: ${nlohmann_json_VERSION}")

# ═══════════════════════════════════════════════════════════════════
# 共享 IPC 库
# ═══════════════════════════════════════════════════════════════════
file(GLOB_RECURSE NEXUS_IPC_SRC "src/ipc/*.cpp")
file(GLOB_RECURSE NEXUS_IPC_HDR "include/nexus/ipc/*.h")

add_library(nexus_ipc STATIC ${NEXUS_IPC_SRC} ${NEXUS_IPC_HDR})
target_include_directories(nexus_ipc PUBLIC include)
target_link_libraries(nexus_ipc PUBLIC
  fmt::fmt
  nlohmann_json::nlohmann_json
  spdlog::spdlog
)

# ═══════════════════════════════════════════════════════════════════
# 共享类型库 (header-only)
# ═══════════════════════════════════════════════════════════════════
add_library(nexus_types INTERFACE)
target_include_directories(nexus_types INTERFACE include)
target_link_libraries(nexus_types INTERFACE nexus_ipc)

# ═══════════════════════════════════════════════════════════════════
# 共享工具库
# ═══════════════════════════════════════════════════════════════════
file(GLOB_RECURSE NEXUS_UTILS_SRC "src/utils/*.cpp")
file(GLOB_RECURSE NEXUS_UTILS_HDR "include/nexus/utils/*.h")

if(NEXUS_UTILS_SRC)
  add_library(nexus_utils STATIC ${NEXUS_UTILS_SRC} ${NEXUS_UTILS_HDR})
  target_include_directories(nexus_utils PUBLIC include)
  target_link_libraries(nexus_utils PUBLIC nexus_ipc nexus_types)
endif()

# ═══════════════════════════════════════════════════════════════════
# 组件 EXE
# ═══════════════════════════════════════════════════════════════════
add_subdirectory(src/env_checker)
add_subdirectory(src/core)
add_subdirectory(src/algo)
add_subdirectory(src/daemon)
add_subdirectory(src/psyche)
add_subdirectory(src/bridge)
add_subdirectory(src/coordinator)

# ═══════════════════════════════════════════════════════════════════
# 测试
# ═══════════════════════════════════════════════════════════════════
if(NEXUS_BUILD_TESTS)
  find_package(GTest CONFIG REQUIRED)
  enable_testing()
  add_subdirectory(tests)
endif()

# ═══════════════════════════════════════════════════════════════════
# 示例
# ═══════════════════════════════════════════════════════════════════
if(NEXUS_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

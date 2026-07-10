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
option(NEXUS_USE_VCPKG "使用 vcpkg 依赖管理" OFF)

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
  find_package(CUDAToolkit QUIET)
  if(CUDAToolkit_FOUND)
    message(STATUS "CUDA Toolkit: ${CUDAToolkit_VERSION} at ${CUDAToolkit_DIR}")
    add_compile_definitions(NEXUS_CUDA_ENABLED)
  else()
    message(WARNING "CUDA Toolkit not found — GPU 功能将不可用")
  endif()
endif()

# ── 依赖管理 ──
#   模式 A: NEXUS_USE_VCPKG=ON  → vcpkg 管理 (fmt, spdlog, nlohmann-json, GTest)
#   模式 B: NEXUS_USE_VCPKG=OFF → third_party 内嵌版本

if(NEXUS_USE_VCPKG)
  find_package(fmt CONFIG REQUIRED)
  find_package(spdlog CONFIG REQUIRED)
  find_package(nlohmann_json CONFIG REQUIRED)
  message(STATUS "fmt: ${fmt_VERSION}")
  message(STATUS "spdlog: ${spdlog_VERSION}")
  message(STATUS "nlohmann_json: ${nlohmann_json_VERSION}")
else()
  message(STATUS "使用 third_party/ 内嵌依赖 (无 vcpkg 时自动降级)")
endif()

# ── third_party 头文件路径（始终添加，vcpkg 模式会覆盖） ──
set(NEXUS_THIRD_PARTY_DIR "${CMAKE_SOURCE_DIR}/third_party")

# ═══════════════════════════════════════════════════════════════════
# 共享 IPC 库
# ═══════════════════════════════════════════════════════════════════
if(WIN32)
  file(GLOB_RECURSE NEXUS_IPC_SRC "src/ipc/state_file.cpp" "src/ipc/mmap_ringbuf.cpp")
else()
  file(GLOB_RECURSE NEXUS_IPC_SRC "src/ipc/*_posix.cpp")
endif()
file(GLOB_RECURSE NEXUS_IPC_HDR "include/nexus/ipc/*.h")

add_library(nexus_ipc STATIC ${NEXUS_IPC_SRC} ${NEXUS_IPC_HDR})
target_include_directories(nexus_ipc PUBLIC
  include
  "${NEXUS_THIRD_PARTY_DIR}")

if(NEXUS_USE_VCPKG)
  target_link_libraries(nexus_ipc PUBLIC
    fmt::fmt
    nlohmann_json::nlohmann_json
    spdlog::spdlog)
endif()

if(CUDAToolkit_FOUND)
  target_link_libraries(nexus_ipc PUBLIC CUDA::cuda_driver)
endif()

# ═══════════════════════════════════════════════════════════════════
# 共享类型库 (header-only)
# ═══════════════════════════════════════════════════════════════════
add_library(nexus_types INTERFACE)
target_include_directories(nexus_types INTERFACE
  include
  "${NEXUS_THIRD_PARTY_DIR}")
target_link_libraries(nexus_types INTERFACE nexus_ipc)

# ═══════════════════════════════════════════════════════════════════
# 共享工具库
# ═══════════════════════════════════════════════════════════════════
file(GLOB_RECURSE NEXUS_UTILS_SRC "src/utils/*.cpp")
file(GLOB_RECURSE NEXUS_UTILS_HDR "include/nexus/utils/*.h")

if(NEXUS_UTILS_SRC)
  add_library(nexus_utils STATIC ${NEXUS_UTILS_SRC} ${NEXUS_UTILS_HDR})
  target_include_directories(nexus_utils PUBLIC
    include
    "${NEXUS_THIRD_PARTY_DIR}")
  target_link_libraries(nexus_utils PUBLIC nexus_ipc nexus_types)
endif()

# ═══════════════════════════════════════════════════════════════════
# 组件 EXE
# ═══════════════════════════════════════════════════════════════════
if(WIN32)
  # Windows: 完整构建所有组件
  add_subdirectory(src/env_checker)
  add_subdirectory(src/core)
  add_subdirectory(src/algo)
  add_subdirectory(src/daemon)
  add_subdirectory(src/psyche)
  add_subdirectory(src/bridge)
  add_subdirectory(src/coordinator)
  add_subdirectory(src/quantum)
  add_subdirectory(src/logic)
  add_subdirectory(src/holographic)
else()
  # Linux: 仅构建跨平台组件
  add_subdirectory(src/env_checker)
  add_subdirectory(src/algo)
endif()

# ═══════════════════════════════════════════════════════════════════
# 测试
# ═══════════════════════════════════════════════════════════════════
if(NEXUS_BUILD_TESTS)
  # GTest via FetchContent (vcpkg 模式用 vcpkg 的 GTest)
  if(NOT NEXUS_USE_VCPKG)
    include(FetchContent)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG v1.15.2
    )
    # 关闭 GTest 自身的测试和安装
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    message(STATUS "GTest: ${googletest_SOURCE_DIR} (via FetchContent)")
  endif()

  if(NEXUS_USE_VCPKG)
    find_package(GTest CONFIG QUIET)
    if(GTest_FOUND)
      enable_testing()
      add_subdirectory(tests EXCLUDE_FROM_ALL)
    else()
      message(WARNING "GTest not found — 跳过测试构建")
    endif()
  else()
    enable_testing()
    add_subdirectory(tests EXCLUDE_FROM_ALL)
  endif()
endif()

# ═══════════════════════════════════════════════════════════════════
# 示例
# ═══════════════════════════════════════════════════════════════════
if(NEXUS_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# ═══════════════════════════════════════════════════════════════════
# CPack — Linux 包生成 (.deb / .rpm)
# ═══════════════════════════════════════════════════════════════════
# 用法: cpack --config build/CPackConfig.cmake -G DEB
#       cpack --config build/CPackConfig.cmake -G RPM
# ═══════════════════════════════════════════════════════════════════
if(UNIX AND NOT APPLE)
  set(CPACK_GENERATOR "DEB;RPM")
  set(CPACK_PACKAGE_NAME "nexus")
  set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
  set(CPACK_PACKAGE_VENDOR "CherryClaw")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Nexus AGI Nervous System")
  set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/hy7pc8gfmf-dotcom/nexus")
  set(CPACK_PACKAGE_CONTACT "nexus@cherryclaw.dev")
  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
  set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

  # Debian 特定
  set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.35)")
  set(CPACK_DEBIAN_PACKAGE_SECTION "devel")
  set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")

  # RPM 特定
  set(CPACK_RPM_PACKAGE_REQUIRES "glibc >= 2.35")
  set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")

  # Linux 只构建了 env_checker + algo
  install(TARGETS
    env_checker algo
    RUNTIME DESTINATION bin
  )

  install(FILES
    "${CMAKE_SOURCE_DIR}/README.md"
    "${CMAKE_SOURCE_DIR}/LICENSE"
    "${CMAKE_SOURCE_DIR}/NOTICE"
    "${CMAKE_SOURCE_DIR}/CHANGELOG.md"
    DESTINATION share/doc/nexus
  )

  include(CPack)
endif()

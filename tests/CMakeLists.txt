# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 CherryClaw & Contributors

# ═══════════════════════════════════════════════════════════════════
# Nexus 测试套件
# ═══════════════════════════════════════════════════════════════════

add_executable(nexus_tests
  test_env/env_checker_test.cpp
  test_core/state_file_test.cpp
  test_core/mmap_ringbuf_test.cpp
  test_daemon/state_writer_test.cpp
  test_psyche/psi_reasoner_test.cpp
)

target_link_libraries(nexus_tests PRIVATE
  nexus_ipc
  nexus_types
  psyche_lib
)

# GTest 目标名: FetchContent → gtest/gtest_main, find_package → GTest::gtest
if(TARGET GTest::gtest)
  target_link_libraries(nexus_tests PRIVATE GTest::gtest GTest::gtest_main)
else()
  target_link_libraries(nexus_tests PRIVATE gtest gtest_main)
endif()

target_include_directories(nexus_tests PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/../include"
  "${CMAKE_CURRENT_LIST_DIR}/../third_party"
)

# vcpkg 模式下额外链接 fmt/spdlog
if(NEXUS_USE_VCPKG)
  target_link_libraries(nexus_tests PRIVATE
    fmt::fmt
    spdlog::spdlog
  )
endif()

# 注册 CTest
add_test(NAME nexus_tests COMMAND nexus_tests)

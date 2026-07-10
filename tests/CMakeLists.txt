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
)

target_link_libraries(nexus_tests PRIVATE
  nexus_ipc
  nexus_types
  GTest::gtest
  GTest::gtest_main
)

target_include_directories(nexus_tests PRIVATE
  "${CMAKE_SOURCE_DIR}/include"
  "${CMAKE_SOURCE_DIR}/third_party"
)

# vcpkg 模式下额外链接 fmt/spdlog
if(NEXUS_USE_VCPKG)
  target_link_libraries(nexus_tests PRIVATE
    fmt::fmt
    spdlog::spdlog
  )
endif()

include(GoogleTest)
gtest_discover_tests(nexus_tests)

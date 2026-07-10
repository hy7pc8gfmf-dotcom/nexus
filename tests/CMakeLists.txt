# SPDX-License-Identifier: Apache-2.0
# Copyright 2026 CherryClaw & Contributors

# ═══════════════════════════════════════════════════════════════════
# Nexus 测试套件
# ═══════════════════════════════════════════════════════════════════

set(NEXUS_TEST_SRC
  test_env/env_checker_test.cpp
  test_core/state_file_test.cpp
  test_core/mmap_ringbuf_test.cpp
  test_core/quantum_test.cpp
  test_core/meta_axiom_test.cpp
  test_core/integration_test.cpp
  test_daemon/state_writer_test.cpp
  test_psyche/psi_reasoner_test.cpp
  test_psyche/observer_test.cpp
  ${CMAKE_SOURCE_DIR}/src/core/meta_axiom_engine.cpp
  ${CMAKE_SOURCE_DIR}/src/core/coq_kernel.cpp
)

# thermal_governor 使用 Windows API, 仅 Windows 构建
if(WIN32)
  list(APPEND NEXUS_TEST_SRC
    test_core/thermal_test.cpp
    ${CMAKE_SOURCE_DIR}/src/core/thermal_governor.cpp
  )
  if(MSVC)
    set_source_files_properties(
      ${CMAKE_SOURCE_DIR}/src/core/thermal_governor.cpp
      PROPERTIES COMPILE_FLAGS "/Zc:strictStrings-"
    )
  endif()
endif()

add_executable(nexus_tests ${NEXUS_TEST_SRC})

target_link_libraries(nexus_tests PRIVATE
  nexus_ipc
  nexus_types
  psyche_lib
  quantum_lib
  bridge_lib
)

# GTest 目标名: FetchContent → gtest/gtest_main, find_package → GTest::gtest
if(TARGET GTest::gtest)
  target_link_libraries(nexus_tests PRIVATE GTest::gtest GTest::gtest_main)
else()
  target_link_libraries(nexus_tests PRIVATE gtest gtest_main)
endif()

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

# 注册 CTest
add_test(NAME nexus_tests COMMAND nexus_tests)

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
  fmt::fmt
  spdlog::spdlog
)

target_include_directories(nexus_tests PRIVATE "${CMAKE_SOURCE_DIR}/include")

include(GoogleTest)
gtest_discover_tests(nexus_tests)

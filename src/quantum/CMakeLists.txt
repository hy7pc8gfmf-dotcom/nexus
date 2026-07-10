# 量子层 — 静态库 (供测试复用)
add_library(quantum_lib STATIC
  quantum_core.cpp)

target_link_libraries(quantum_lib PUBLIC nexus_ipc nexus_types)
target_include_directories(quantum_lib PUBLIC "${PROJECT_SOURCE_DIR}/include")
if(NEXUS_USE_VCPKG)
  target_link_libraries(quantum_lib PUBLIC fmt::fmt nlohmann_json::nlohmann_json spdlog::spdlog)
endif()

# 量子层 — main EXE
add_executable(quantum
  main.cpp)

target_link_libraries(quantum PRIVATE quantum_lib)
install(TARGETS quantum RUNTIME DESTINATION bin)

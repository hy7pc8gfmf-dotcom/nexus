# 桥接层 — 静态库 (seed_bank 可被 psyche 复用)
add_library(bridge_lib STATIC
  seed_bank.cpp)

target_link_libraries(bridge_lib PUBLIC nexus_ipc nexus_types)
target_include_directories(bridge_lib PUBLIC "${PROJECT_SOURCE_DIR}/include")
if(NEXUS_USE_VCPKG)
  target_link_libraries(bridge_lib PUBLIC fmt::fmt nlohmann_json::nlohmann_json spdlog::spdlog)
endif()

# 桥接层 — main EXE (mcp_client 需 Windows API)
if(WIN32)
  add_executable(bridge
    main.cpp
    mcp_client.cpp
    seed_channel.cpp
    global_memory.cpp
    seed_cultivator.cpp
    willchain.cpp
    capability.cpp
    sinan.cpp)

  target_link_libraries(bridge PRIVATE bridge_lib)
  install(TARGETS bridge RUNTIME DESTINATION bin)
endif()
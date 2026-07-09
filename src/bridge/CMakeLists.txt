add_executable(bridge
  main.cpp
  mcp_client.cpp
  seed_channel.cpp
  global_memory.cpp)

target_link_libraries(bridge PRIVATE nexus_ipc nexus_types)
if(NEXUS_USE_VCPKG)
  target_link_libraries(bridge PRIVATE fmt::fmt nlohmann_json::nlohmann_json spdlog::spdlog)
endif()
install(TARGETS bridge RUNTIME DESTINATION bin)

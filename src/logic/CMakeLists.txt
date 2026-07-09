add_executable(logic
  main.cpp
  logic_core.cpp)

target_link_libraries(logic PRIVATE nexus_ipc nexus_types)
if(NEXUS_USE_VCPKG)
  target_link_libraries(logic PRIVATE fmt::fmt nlohmann_json::nlohmann_json spdlog::spdlog)
endif()
install(TARGETS logic RUNTIME DESTINATION bin)

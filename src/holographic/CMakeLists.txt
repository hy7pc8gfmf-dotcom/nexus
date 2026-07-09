add_executable(holographic
  main.cpp
  holographic_core.cpp)

target_link_libraries(holographic PRIVATE nexus_ipc nexus_types)
if(NEXUS_USE_VCPKG)
  target_link_libraries(holographic PRIVATE fmt::fmt nlohmann_json::nlohmann_json spdlog::spdlog)
endif()
install(TARGETS holographic RUNTIME DESTINATION bin)

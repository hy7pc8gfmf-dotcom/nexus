add_executable(quantum
  main.cpp
  quantum_core.cpp)

target_link_libraries(quantum PRIVATE nexus_ipc nexus_types)
if(NEXUS_USE_VCPKG)
  target_link_libraries(quantum PRIVATE fmt::fmt nlohmann_json::nlohmann_json spdlog::spdlog)
endif()
install(TARGETS quantum RUNTIME DESTINATION bin)

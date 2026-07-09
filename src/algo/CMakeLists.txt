add_executable(algo
  main.cpp
  engine_registry.cpp
  audit.cpp
  shell_registry.cpp
  interlock.cpp
  recursor.cpp)

target_link_libraries(algo PRIVATE nexus_ipc nexus_types)
if(NEXUS_USE_VCPKG)
  target_link_libraries(algo PRIVATE fmt::fmt nlohmann_json::nlohmann_json spdlog::spdlog)
endif()
install(TARGETS algo RUNTIME DESTINATION bin)

add_subdirectory(subprocess)

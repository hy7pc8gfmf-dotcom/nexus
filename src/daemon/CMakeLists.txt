add_executable(daemon main.cpp)
target_link_libraries(daemon PRIVATE
  nexus_ipc
  nexus_types)
if(NEXUS_USE_VCPKG)
  target_link_libraries(daemon PRIVATE
    fmt::fmt
    nlohmann_json::nlohmann_json
    spdlog::spdlog)
endif()
install(TARGETS daemon RUNTIME DESTINATION bin)

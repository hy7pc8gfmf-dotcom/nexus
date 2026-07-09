add_executable(coordinator main.cpp)
target_link_libraries(coordinator PRIVATE
  nexus_ipc
  nexus_types)
if(NEXUS_USE_VCPKG)
  target_link_libraries(coordinator PRIVATE
    fmt::fmt
    spdlog::spdlog)
endif()
install(TARGETS coordinator RUNTIME DESTINATION bin)

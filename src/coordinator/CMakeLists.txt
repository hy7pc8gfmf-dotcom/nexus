add_executable(coordinator main.cpp)
target_link_libraries(coordinator PRIVATE
  nexus_ipc
  nexus_types
  fmt::fmt
  nlohmann_json::nlohmann_json
  spdlog::spdlog
)
install(TARGETS coordinator RUNTIME DESTINATION bin)

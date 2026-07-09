add_executable(bridge main.cpp)
target_link_libraries(bridge PRIVATE
  nexus_ipc
  nexus_types
  fmt::fmt
  nlohmann_json::nlohmann_json
  spdlog::spdlog
)
install(TARGETS bridge RUNTIME DESTINATION bin)

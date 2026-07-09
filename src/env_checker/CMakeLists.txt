add_executable(env_checker main.cpp)
target_link_libraries(env_checker PRIVATE
  nexus_ipc
  nexus_types
  fmt::fmt
  nlohmann_json::nlohmann_json
  spdlog::spdlog
)
install(TARGETS env_checker RUNTIME DESTINATION bin)

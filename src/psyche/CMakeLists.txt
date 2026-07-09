add_executable(psyche main.cpp)
target_link_libraries(psyche PRIVATE
  nexus_ipc
  nexus_types
  fmt::fmt
  nlohmann_json::nlohmann_json
  spdlog::spdlog
)
install(TARGETS psyche RUNTIME DESTINATION bin)

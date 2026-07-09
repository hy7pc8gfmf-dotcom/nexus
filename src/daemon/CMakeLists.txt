add_executable(daemon main.cpp)
target_link_libraries(daemon PRIVATE
  nexus_ipc
  nexus_types
  fmt::fmt
  nlohmann_json::nlohmann_json
  spdlog::spdlog
)
install(TARGETS daemon RUNTIME DESTINATION bin)

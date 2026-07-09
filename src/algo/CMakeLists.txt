add_executable(algo main.cpp)
target_link_libraries(algo PRIVATE
  nexus_ipc
  nexus_types
  fmt::fmt
  nlohmann_json::nlohmann_json
  spdlog::spdlog
)
install(TARGETS algo RUNTIME DESTINATION bin)

# 隔离子进程
add_subdirectory(subprocess)

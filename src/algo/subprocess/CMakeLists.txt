add_executable(algo_engine main.cpp)
target_link_libraries(algo_engine PRIVATE
  nexus_ipc
  fmt::fmt
  spdlog::spdlog
)
install(TARGETS algo_engine RUNTIME DESTINATION bin)

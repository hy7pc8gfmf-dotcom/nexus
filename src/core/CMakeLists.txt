add_executable(core main.cpp)
target_link_libraries(core PRIVATE
  nexus_ipc
  nexus_types
  fmt::fmt
  nlohmann_json::nlohmann_json
  spdlog::spdlog
)
if(NEXUS_CUDA_SUPPORT)
  target_link_libraries(core PRIVATE CUDA::cuda_driver CUDA::cudart)
  target_compile_definitions(core PRIVATE NEXUS_CUDA_ENABLED)
endif()
install(TARGETS core RUNTIME DESTINATION bin)

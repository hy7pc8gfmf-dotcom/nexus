add_executable(env_checker main.cpp)
target_link_libraries(env_checker PRIVATE
  nexus_ipc
  nexus_types)
if(NEXUS_USE_VCPKG)
  target_link_libraries(env_checker PRIVATE
    fmt::fmt
    nlohmann_json::nlohmann_json
    spdlog::spdlog)
endif()
if(CUDAToolkit_FOUND)
  target_link_libraries(env_checker PRIVATE CUDA::cuda_driver CUDA::cudart)
  target_compile_definitions(env_checker PRIVATE NEXUS_CUDA_ENABLED)
endif()
install(TARGETS env_checker RUNTIME DESTINATION bin)

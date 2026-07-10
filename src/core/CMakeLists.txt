# ── llama.cpp (FetchContent, CPU-only) ──
# GPU 推理通过 CUDA Runtime API 直接管理 VRAM，llama.cpp 作为 CPU fallback
include(FetchContent)
set(LLAMA_CUDA OFF CACHE BOOL "Disable CUDA in llama.cpp" FORCE)
set(LLAMA_CURL OFF CACHE BOOL "Disable curl" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Static build" FORCE)
set(LLAMA_BUILD_TESTS OFF CACHE BOOL "Disable tests" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "Disable examples" FORCE)
FetchContent_Declare(llama
  GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git
  GIT_TAG master
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(llama)

# ── core.exe ──
add_executable(core
  main.cpp
  vram_manager.cpp
  expert_loader.cpp
  scheduler.cpp
  block_swapper.cpp
  meta_kernel.cpp
  experience.cpp
  native_bridge.cpp
  gpu_token.cpp
  coq_kernel.cpp
  meta_axiom_engine.cpp
  thermal_governor.cpp)

target_link_libraries(core PRIVATE
  nexus_ipc
  nexus_types
  PRIVATE llama)

if(NEXUS_USE_VCPKG)
  target_link_libraries(core PRIVATE fmt::fmt nlohmann_json::nlohmann_json)
endif()

if(CUDAToolkit_FOUND)
  target_link_libraries(core PRIVATE CUDA::cuda_driver CUDA::cudart)
  target_compile_definitions(core PRIVATE NEXUS_CUDA_ENABLED)
endif()

install(TARGETS core RUNTIME DESTINATION bin)

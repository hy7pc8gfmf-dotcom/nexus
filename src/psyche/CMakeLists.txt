# Ψ 层 — 静态库 (psi_reasoner 可被测试复用)
add_library(psyche_lib STATIC
  psi_reasoner.cpp)

target_link_libraries(psyche_lib PUBLIC nexus_ipc nexus_types)
target_include_directories(psyche_lib PUBLIC "${PROJECT_SOURCE_DIR}/include")
if(NEXUS_USE_VCPKG)
  target_link_libraries(psyche_lib PUBLIC fmt::fmt nlohmann_json::nlohmann_json spdlog::spdlog)
endif()

# Ψ 层 — main EXE
add_executable(psyche
  main.cpp
  navigator.cpp
  emergence.cpp
  convergence.cpp
  sophia_core.cpp
  self_evolve.cpp
  observer.cpp
  algo_quantum.cpp
  scoreboard.cpp
  meta_axiom.cpp
  holographic.cpp)

target_link_libraries(psyche PRIVATE psyche_lib)
install(TARGETS psyche RUNTIME DESTINATION bin)

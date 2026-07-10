# Ψ 层 — 静态库 (跨平台, 供测试复用)
add_library(psyche_lib STATIC
  psi_reasoner.cpp
  observer.cpp
  semantic_port.cpp
  psi_guard.cpp
  grid_reasoner.cpp
  concept_pathway.cpp
  psi_balancer.cpp
  flow_bridge.cpp
  semantic_cluster.cpp
  self_evolve_trigger.cpp)

target_link_libraries(psyche_lib PUBLIC nexus_ipc nexus_types)
target_include_directories(psyche_lib PUBLIC "${PROJECT_SOURCE_DIR}/include")
if(NEXUS_USE_VCPKG)
  target_link_libraries(psyche_lib PUBLIC fmt::fmt nlohmann_json::nlohmann_json spdlog::spdlog)
endif()

# Ψ 层 — main EXE (仅 Windows, 依赖 bridge_lib)
if(WIN32)
  add_executable(psyche
    main.cpp
    navigator.cpp
    emergence.cpp
    convergence.cpp
    sophia_core.cpp
    self_evolve.cpp
    algo_quantum.cpp
    scoreboard.cpp
    meta_axiom.cpp
    holographic.cpp)

  target_link_libraries(psyche PRIVATE psyche_lib bridge_lib)
  install(TARGETS psyche RUNTIME DESTINATION bin)
endif()

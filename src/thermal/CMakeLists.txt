# 热敏治理库
if(WIN32)
  add_library(thermal_lib STATIC ${CMAKE_CURRENT_LIST_DIR}/../core/thermal_governor.cpp)
  target_link_libraries(thermal_lib PUBLIC nexus_ipc nexus_types)
  target_include_directories(thermal_lib PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../../include)
endif()

if(BUILD_UIOHOOK)
  add_subdirectory(uiohook EXCLUDE_FROM_ALL)
endif()

if(LINUX)
  add_subdirectory(qtmpris)
endif()

add_library(miniaudio)
target_include_directories(miniaudio
                           INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/miniaudio)

qt_add_executable(YuRadio main.cpp)

target_compile_features(YuRadio PUBLIC cxx_std_20)
set_target_properties(
  YuRadio
  PROPERTIES CXX_SCAN_FOR_MODULES OFF
             CXX_STANDARD_REQUIRED ON
             CXX_EXTENSIONS OFF)

add_subdirectory(YuRadioContents)

if(ANDROID)
  include(FetchContent)
  FetchContent_Declare(
    android_ffmpeg_openssl
    DOWNLOAD_EXTRACT_TIMESTAMP true
    URL "https://github.com/yunag/YuRadio-deps/releases/download/2.0.0/android-ffmpeg-openssl-${CMAKE_ANDROID_ARCH_ABI}.zip"
  )
  FetchContent_MakeAvailable(android_ffmpeg_openssl)

  if(NOT OPENSSL_ROOT_DIR)
    set(OPENSSL_ROOT_DIR ${android_ffmpeg_openssl_SOURCE_DIR}/openssl)
  endif()

  if(NOT FFMPEG_ROOT_DIR)
    set(FFMPEG_ROOT_DIR ${android_ffmpeg_openssl_SOURCE_DIR}/ffmpeg)
  endif()

  # Need to set CMAKE_FIND_ROOT_PATH when cross-compiling
  list(APPEND CMAKE_FIND_ROOT_PATH ${FFMPEG_ROOT_DIR})

  list(APPEND android_extra_libs "${OPENSSL_ROOT_DIR}/lib/libcrypto34.so"
       "${OPENSSL_ROOT_DIR}/lib/libssl34.so")
  set_target_properties(YuRadio PROPERTIES QT_ANDROID_EXTRA_LIBS
                                           "${android_extra_libs}")
  add_subdirectory(android)
endif()

if(LINUX)
  add_subdirectory(linux)
endif()

add_subdirectory(network)
add_subdirectory(audio)

if(SPOTIFY_CLIENTID
   AND SPOTIFY_SECRET
   AND SPOTIFY_CLIENTID_HASH MATCHES "^(0|[a-fA-F0-9]+)$"
   AND SPOTIFY_SECRET_HASH MATCHES "^(0|[a-fA-F0-9]+)$")
  message("Spotify integration - OK")
else()
  set(SPOTIFY_SECRET_HASH 0)
  set(SPOTIFY_CLIENTID_HASH 0)
endif()

configure_file(applicationconfig.h.in
               "${CMAKE_BINARY_DIR}/config/applicationconfig.h" @ONLY)

target_include_directories(
  YuRadio PUBLIC "$<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/config>")

qt_add_qml_module(
  YuRadio
  URI
  Main
  QML_FILES
  Main.qml
  ErrorPage.qml
  NO_PLUGIN
  SOURCES
  application.h
  application.cpp
  algorithm.h
  algorithm.cpp
  mediaplayerservice.h
  mediaplayerservice.cpp
  audiostreamrecorder.h
  audiostreamrecorder.cpp
  downloadspeedmeasurer.h
  downloadspeedmeasurer.cpp
  radioplayer.h
  radioplayer.cpp
  musicinfomodel.h
  musicinfomodel.cpp
  musicinfoproviderbackend.h
  musicinfoproviderbackend.cpp
  itunesbackend.h
  itunesbackend.cpp
  storage.h
  storage.cpp
  spotifybackend.h
  spotifybackend.cpp
  imageloader.h
  imageloader.cpp
  platformradiocontroller.cpp
  platformradiocontroller.h
  languagetranslator.h
  languagetranslator.cpp
  globalkeylistener.h
  globalkeylistener.cpp
  sqlquerymodel.h
  sqlquerymodel.cpp
  radiostation.h
  radiostation.cpp
  radiostationmodel.h
  radiostationmodel.cpp
  logging.h
  logging.cpp
  obf.h
  obf.cpp
  memoryliterals.h
  ffmpegradiocontroller.h
  ffmpegradiocontroller.cpp
  DEPENDENCIES
  network)

if(OFF) # TODO add option
  target_sources(
    YuRadio PRIVATE qtmediaradiocontroller.cpp qtmediaradiocontroller.h
                    radioinforeaderproxyserver.h radioinforeaderproxyserver.cpp)
endif()

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1. If
# you are developing for iOS or macOS you should consider setting an explicit,
# fixed bundle identifier manually though.
set_target_properties(
  YuRadio
  PROPERTIES # MACOSX_BUNDLE_GUI_IDENTIFIER com.example.YuRadio
             MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
             MACOSX_BUNDLE_SHORT_VERSION_STRING
             ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
             MACOSX_BUNDLE TRUE
             WIN32_EXECUTABLE TRUE
             QT_ANDROID_PACKAGE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/android
             QT_ANDROID_TARGET_SDK_VERSION 34
             QT_ANDROID_SDK_BUILD_TOOLS_REVISION 34.0.0)

target_link_libraries(
  YuRadio
  PRIVATE FFmpegPlayer
          YuRadioNetworkplugin
          YuRadioContentsplugin
          Qt6::Core
          Qt6::Gui
          Qt6::Qml
          Qt6::Quick
          Qt6::QuickControls2
          Qt6::Sql
          Qt6::Location
          Qt6::NetworkAuth
          Qt6::Svg
          "$<$<NOT:$<PLATFORM_ID:Android>>:Qt6::Widgets>"
          "$<TARGET_NAME_IF_EXISTS:uiohook>"
          "$<TARGET_NAME_IF_EXISTS:HotReloader>")

target_set_warnings(YuRadio)

# OpenSSL support for android

target_sources(
  YuRadio
  PRIVATE androidkeyboard.h
          androidkeyboard.cpp
          androidmediaplayerservice.h
          androidmediaplayerservice.cpp
          androidmediasessionimageprovider.h
          androidmediasessionimageprovider.cpp
          androidstatusbar.h
          androidstatusbar.cpp
          notificationclient.cpp
          notificationclient.h
          virtualkeyboardlistener.h
          virtualkeyboardlistener.cpp)
target_include_directories(YuRadio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

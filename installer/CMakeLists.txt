cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "VitaSDK toolchain")
  else()
    message(FATAL_ERROR "Set VITASDK before configuring this project")
  endif()
endif()

include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
project(caffeine_installer LANGUAGES C)

if(NOT DEFINED CAFFEINE_SUPRX OR NOT EXISTS "${CAFFEINE_SUPRX}")
  message(FATAL_ERROR "Set CAFFEINE_SUPRX to the compiled caffeine.suprx path")
endif()

set(VITA_APP_NAME "Caffeine Installer")
set(VITA_TITLEID "CFNE00001")
set(VITA_VERSION "01.00")
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Os")

add_executable(${PROJECT_NAME}
  src/main.c
  "$ENV{VITASDK}/share/gcc-arm-vita-eabi/samples/common/debugScreen.c"
)
target_include_directories(${PROJECT_NAME} PRIVATE
  "$ENV{VITASDK}/share/gcc-arm-vita-eabi/samples/common"
)
target_link_libraries(${PROJECT_NAME}
  SceCtrl_stub
  SceDisplay_stub
  SceIofilemgr_stub
  SceLibKernel_stub
  ScePower_stub
)

vita_create_self(${PROJECT_NAME}.self ${PROJECT_NAME} UNSAFE)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/payload")
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/sce_sys")
file(DOWNLOAD
  "https://github.com/Ibrahim778/QuickMenuReborn/releases/download/V2.5/QuickMenuReborn.suprx"
  "${CMAKE_BINARY_DIR}/payload/QuickMenuReborn.suprx"
  EXPECTED_HASH SHA256=7B1FDD0C97A8E8C848F1DC186F15D28B1704242BD68DB5028888D75B0FDC2A78
  TLS_VERIFY ON
)
file(DOWNLOAD
  "https://github.com/Ibrahim778/QuickMenuReborn/releases/download/V2.5/qmr_plugin.rco"
  "${CMAKE_BINARY_DIR}/payload/qmr_plugin.rco"
  EXPECTED_HASH SHA256=0E1C2AC10FC8FD6D9C6B9B60EA690F5B03BF038B3E74DCB9358A4EBCA02AAE61
  TLS_VERIFY ON
)
configure_file("${CMAKE_SOURCE_DIR}/sce_sys/icon0.png"
  "${CMAKE_BINARY_DIR}/sce_sys/icon0.png" COPYONLY)
configure_file("${CMAKE_SOURCE_DIR}/../assets/caffeine.png"
  "${CMAKE_BINARY_DIR}/payload/caffeine.png" COPYONLY)
configure_file("${CAFFEINE_SUPRX}"
  "${CMAKE_BINARY_DIR}/payload/caffeine.suprx" COPYONLY)
vita_create_vpk(CaffeineForVita.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
  VERSION ${VITA_VERSION}
  NAME ${VITA_APP_NAME}
  FILE "${CMAKE_BINARY_DIR}/sce_sys/icon0.png" sce_sys/icon0.png
  FILE "${CMAKE_BINARY_DIR}/payload/caffeine.suprx" payload/caffeine.suprx
  FILE "${CMAKE_BINARY_DIR}/payload/caffeine.png" payload/caffeine.png
  FILE "${CMAKE_BINARY_DIR}/payload/QuickMenuReborn.suprx" payload/QuickMenuReborn.suprx
  FILE "${CMAKE_BINARY_DIR}/payload/qmr_plugin.rco" payload/qmr_plugin.rco
)

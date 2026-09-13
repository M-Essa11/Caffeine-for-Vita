cmake_minimum_required(VERSION 3.16)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "VitaSDK toolchain")
  else()
    message(FATAL_ERROR "Set VITASDK before configuring this project")
  endif()
endif()

include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
project(caffeine VERSION 0.2.2 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Os -nostdlib")

set(QMR_STUB_DIR "${CMAKE_BINARY_DIR}/qmr_stubs")
file(MAKE_DIRECTORY "${QMR_STUB_DIR}")
execute_process(
  COMMAND vita-libs-gen -c "${CMAKE_SOURCE_DIR}/vendor/QuickMenuReborn.yml" "${QMR_STUB_DIR}"
  RESULT_VARIABLE QMR_STUB_RESULT
)
if(NOT QMR_STUB_RESULT EQUAL 0)
  message(FATAL_ERROR "Could not generate QuickMenuReborn stubs")
endif()
add_subdirectory("${QMR_STUB_DIR}" "${CMAKE_BINARY_DIR}/qmr_stubs_build")

add_executable(${PROJECT_NAME}.elf src/main.c)
target_include_directories(${PROJECT_NAME}.elf PRIVATE include)
target_link_libraries(${PROJECT_NAME}.elf
  QuickMenuReborn_stub_weak
  SceLibKernel_stub_weak
  SceKernelThreadMgr_stub_weak
  SceProcessmgr_stub_weak
  SceIofilemgr_stub_weak
)

# vita-elf-create does not quote its CONFIG argument, so keep the generated
# metadata in the (space-free) Linux build directory.
configure_file(caffeine.yml "${CMAKE_BINARY_DIR}/caffeine.yml" COPYONLY)

vita_create_self(${PROJECT_NAME}.suprx ${PROJECT_NAME}.elf
  UNSAFE
  CONFIG "${CMAKE_BINARY_DIR}/caffeine.yml"
)

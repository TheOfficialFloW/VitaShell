cmake_minimum_required(VERSION 2.8)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  if(DEFINED ENV{VITASDK})
    set(CMAKE_TOOLCHAIN_FILE "$ENV{VITASDK}/share/vita.toolchain.cmake" CACHE PATH "toolchain file")
  else()
    message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
  endif()
endif()

project(user)
include("${VITASDK}/share/vita.cmake" REQUIRED)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,-q -Wall -O3 -nostdlib")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-rtti -fno-exceptions")

add_executable(user
  main.c
)

target_link_libraries(user
  SceLibKernel_stub
  SceIofilemgr_stub
  VitaShellKernel2_stub
)

vita_create_self(user.suprx user CONFIG exports.yml UNSAFE)

vita_create_stubs(stubs user ${CMAKE_SOURCE_DIR}/exports.yml)

install(DIRECTORY ${CMAKE_BINARY_DIR}/stubs/
  DESTINATION lib
  FILES_MATCHING PATTERN "*.a"
)

install(FILES vitashell_user.h
  DESTINATION include
)

add_custom_target(copy
  COMMAND cp user.suprx ../../../resources/user.suprx
  DEPENDS user.suprx
)
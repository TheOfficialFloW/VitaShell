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

include_directories(
  ../kernel
)

add_executable(user
  main.c
)

add_dependencies(user vitashell_kernel_stubs)

target_link_libraries(user
  ${CMAKE_CURRENT_BINARY_DIR}/../kernel/libVitaShellKernel2_stub.a
  SceLibKernel_stub
  SceIofilemgr_stub
)

vita_create_self(user.suprx user CONFIG exports.yml UNSAFE)

vita_create_stubs(vitashell_user_stubs user exports.yml)

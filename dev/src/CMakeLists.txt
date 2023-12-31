cmake_minimum_required(VERSION 3.2)
project(land15)
set(LOCAL_DIR ${CMAKE_CURRENT_LIST_DIR})

# ------------------------------------------------------------------------------

groupSourceList(
  SRC_COMMON
  common 
  "deleter_ptr.h;random.h"
  "random.cc")

groupSourceList(
  SRC_GFX
  gfx 
  "core.h;gfx.h;image.h"
  "gfx.cc;image.cc")

groupSourceList(
  SRC_SDL
  sdl 
  "cleanup.h"
  "cleanup.cc")

# ------------------------------------------------------------------------------

add_executable(land15)
target_link_libraries(land15 gflags glm glog SDL3-static stb)
target_include_directories(land15 PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_sources(land15 PRIVATE
  main.cc
  ${SRC_COMMON}
  ${SRC_GFX}
  ${SRC_SDL})

add_custom_command(TARGET land15 POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_BINARY_DIR}/../res
                       $<TARGET_FILE_DIR:land15>/res)
add_custom_command(TARGET land15 POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy_directory
                       ${CMAKE_BINARY_DIR}/../res 
                       $<TARGET_FILE_DIR:land15>/../res)
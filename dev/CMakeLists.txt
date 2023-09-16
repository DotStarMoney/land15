cmake_minimum_required(VERSION 3.25)
project(land15)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT land15)
set(LOCAL_DIR ${CMAKE_CURRENT_LIST_DIR})

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# For MSVC; gives a nice project layout.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
 
if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast /arch:AVX2 /bigobj")   
else()
  message(FATAL_ERROR "Only MSVC is supported for now.")
endif()
 
define_property(
  TARGET
  PROPERTY FOLDER
  INHERITED
  BRIEF_DOCS "Set the folder name."
  FULL_DOCS  "Use to organize targets in an IDE."
)

# Set up external deps below...

include(FetchContent)

FetchContent_Declare(
  abseil
  GIT_REPOSITORY https://github.com/abseil/abseil-cpp
  GIT_TAG        master
)

FetchContent_Declare(
  gflags
  GIT_REPOSITORY https://github.com/gflags/gflags
  GIT_TAG        master
)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm
  GIT_TAG        master
)

FetchContent_Declare(
  glog
  GIT_REPOSITORY https://github.com/google/glog
  GIT_TAG        master
)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest
  GIT_TAG        main
)

FetchContent_Declare(
  sdl
  GIT_REPOSITORY https://github.com/libsdl-org/SDL
  GIT_TAG        main
)

FetchContent_Declare(
  stb
  GIT_REPOSITORY https://github.com/nothings/stb
  GIT_TAG        master
)

# Grab gflags and googletest first since other targets depend on them.

option(BUILD_gflags_LIB ON)
FetchContent_MakeAvailable(gflags googletest)

set(gflags_DIR "${gflags_BINARY_DIR}")
set(GTest_DIR "${googletest_BINARY_DIR}")

# Now grab the rest of the targets.

# We don't care about the included project tests.
option(BUILD_TESTING OFF)
set(BUILD_SHARED_LIBS OFF)

set(ABSL_PROPAGATE_CXX_STD ON)

# Major hack to get around deficiencies with monobuild-ing glog.
set(WITH_GFLAGS OFF)
set(gflags_NAMESPACE gflags) 

# No shared libraries thanks.
set(SDL_STATIC ON)
set(SDL_SHARED OFF)

FetchContent_MakeAvailable(abseil gflags glm glog sdl stb)

enable_testing()
option(BUILD_TESTING ON)

# STB has no CMakeLists.txt, so we'll make a fake dep.

add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ${stb_SOURCE_DIR})

# Convenience function for adding MSVC friendly headers and sources:

function(groupSourceList OUT_GROUP_ALL NAMESPACE GROUP_HEADERS GROUP_SOURCES)
  list(TRANSFORM GROUP_HEADERS PREPEND "${NAMESPACE}/")
  list(TRANSFORM GROUP_SOURCES PREPEND "${NAMESPACE}/")
  source_group("Header Files\\${NAMESPACE}" FILES ${GROUP_HEADERS})
  source_group("Source Files\\${NAMESPACE}" FILES ${GROUP_SOURCES})
  set(${OUT_GROUP_ALL} ${GROUP_HEADERS} ${GROUP_SOURCES} PARENT_SCOPE)
endfunction()

add_subdirectory(${LOCAL_DIR}/src)

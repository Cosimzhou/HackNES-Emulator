cmake_minimum_required(VERSION 3.1)
project(HackerNES)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING
      "Choose the type of build, options are: Debug Release."
      FORCE)
endif()

set(BUILD_STATIC FALSE CACHE STRING "Set this to link external libraries statically")

if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -g")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif()

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG}  -std=c++11")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -std=c++11")
endif()

# Add directory containing FindSFML.cmake to module path
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/Modules/;${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}")

# Add sources
file(GLOB SOURCES
    "${PROJECT_SOURCE_DIR}/src/core/*.cc"
    "${PROJECT_SOURCE_DIR}/src/core/devices/*.cc"
    "${PROJECT_SOURCE_DIR}/src/mapper/*.cc"
)

# Copy keybindings.conf
file(COPY keybindings.conf DESTINATION .)
file(MAKE_DIRECTORY record/tmp record/save record/pics record/snds)

# Specify include Directory
include_directories("${PROJECT_SOURCE_DIR}/include")

# Set static if BUILD_STATIC is set
if(BUILD_STATIC)
  set(SFML_STATIC_LIBRARIES TRUE)
  # Link libgcc and libstc++ statically as well
  if(CMAKE_COMPILER_IS_GNUCXX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")
  endif()
endif()

find_package(glog REQUIRED)
if ("${GLOG_LIBRARIES}" STREQUAL "")
	set(GLOG_LIBRARIES glog)
endif()

find_package(gflags REQUIRED)

# Find SFML
if(SFML_OS_WINDOWS AND SFML_COMPILER_MSVC)
  find_package(SFML 2 COMPONENTS main audio graphics window system REQUIRED)
else()
  find_package(SFML 2 COMPONENTS audio graphics window system REQUIRED)
endif()

if(SFML_FOUND)
  include_directories(${SFML_INCLUDE_DIR})
else()
  set(SFML_ROOT "" CACHE PATH "SFML top-level directory")
  message("\nSFML directory not found. Set SFML_ROOT to SFML's top-level path (containing \"include\" and \"lib\" directories).")
  message("Make sure the SFML libraries with the same configuration (Release/Debug, Static/Dynamic) exist.\n")
endif()

set(CLI_DEPENDENCIES ${GLOG_LIBRARIES} ${GFLAGS_LIBRARIES})
set(HN_DEPENDENCIES ${SFML_LIBRARIES} ${SFML_DEPENDENCIES} ${CLI_DEPENDENCIES})

add_executable(etest "${PROJECT_SOURCE_DIR}/src/etest.cpp")
target_link_libraries(etest ${HN_DEPENDENCIES})

add_executable(patternv ${SOURCES} "${PROJECT_SOURCE_DIR}/src/pv-main.cc")
target_link_libraries(patternv ${HN_DEPENDENCIES})

add_executable(HackerNES ${SOURCES} "${PROJECT_SOURCE_DIR}/src/main.cc")
target_link_libraries(HackerNES ${HN_DEPENDENCIES})


set_property(TARGET HackerNES PROPERTY CXX_STANDARD 11)
set_property(TARGET HackerNES PROPERTY CXX_STANDARD_REQUIRED ON)

target_link_libraries(HackerNES)

#install(DIRECTORY DESTINATION ${directory})

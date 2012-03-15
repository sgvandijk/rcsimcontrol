cmake_minimum_required (VERSION 2.6)
project (RCSimControl)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

find_package(Threads REQUIRED)
find_package(Boost REQUIRED COMPONENTS thread system signals)
find_package(LibConfig REQUIRED)

include_directories(${Boost_INCLUDE_DIRS} ${LIBCONFIG_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})

add_subdirectory(scserver)
add_subdirectory(scclient)
add_subdirectory(servers)

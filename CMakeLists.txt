cmake_minimum_required (VERSION 2.6)
project (RCSimControl)

find_package(Boost REQUIRED COMPONENTS thread system signals)

include_directories(${Boost_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIRS})

add_subdirectory(scserver)
add_subdirectory(scclient)
add_subdirectory(servers)

cmake_minimum_required (VERSION 2.6)
project (RCSimControl)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/)

set(BUILD_SHARED_LIBS ON CACHE BOOL "Choose whether to build a shared library")
set(BUILD_CLIENT ON CACHE BOOL "Choose whether to build the SCClient")
set(BUILD_SERVERS ON CACHE BOOL "Choose whether to build example servers")

find_package(Threads REQUIRED)
find_package(Boost 1.47 REQUIRED COMPONENTS thread system signals)
find_package(LibConfig REQUIRED)

include_directories(${Boost_INCLUDE_DIRS} ${LIBCONFIG_INCLUDE_DIR})
link_directories(${Boost_LIBRARY_DIRS})

add_library(rcsimcontrol
lib/Path/init.cc
lib/Path/path.cc
lib/RunDef/rundef.cc
lib/Parser/parse.cc
lib/Parser/parser0.cc
lib/Parser/generate.cc
lib/Parser/initialize.cc
lib/Parser/static.cc
lib/AST/find.cc
lib/AST/finddeep.cc
lib/AST/findall.cc
lib/AST/findalldeep.cc
lib/AST/selectall.cc
lib/AST/select.cc
lib/AST/findparentofdeep.cc
lib/Comm/comm.cc
lib/Comm/AgentDataComm/agentdatacomm.cc
lib/Predicate/destroy.cc
lib/Predicate/copy.cc
lib/Predicate/generate.cc
lib/Predicate/operator0.cc
lib/Predicate/toString.cc
lib/PortableParser/static.cc
lib/Process/process.cc
scserver/RCMComm/rcmcomm.cc
scserver/SCCComm/scccomm.cc
scserver/SCServer/scserver.cc
)

install(TARGETS rcsimcontrol DESTINATION lib)

IF(BUILD_CLIENT)
  add_subdirectory(scclient)
ENDIF(BUILD_CLIENT)

IF(BUILD_SERVERS)
  add_subdirectory(servers)
ENDIF(BUILD_SERVERS)

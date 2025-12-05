cmake_minimum_required(VERSION 3.20)

project(microswim LANGUAGES C CXX)

set(EXPORT_COMPILE_COMMANDS
    "Export compile definitions (for debugging purposes)" OFF)
if(EXPORT_COMPILE_COMMANDS)
  set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

option(CBOR "Support CBOR messages" ON)
option(JSON "Support JSON message" OFF)
option(BUILD_BENCHMARKS "Build the benchmarks" OFF)
option(BUILD_EXAMPLES "Build the examples" OFF)
option(BUILD_TESTS "Build the tests" OFF)

if(CBOR)
  set_directory_properties(PROPERTIES COMPILE_DEFINITIONS MICROSWIM_CBOR=1)
endif()

if(JSON)
  set_directory_properties(PROPERTIES COMPILE_DEFINITIONS MICROSWIM_JSON=1)
endif()

if(BUILD_BENCHMARKS)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/benchmarks)
endif()

if(BUILD_EXAMPLES)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/examples)
endif()

if(BUILD_TESTS)
  add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/tests)
endif()

if(BUILD_LIBRARY)
  set(SOURCES
      ${PROJECT_SOURCE_DIR}/src/microswim.c
      ${PROJECT_SOURCE_DIR}/src/member.c
      ${PROJECT_SOURCE_DIR}/src/message.c
      ${PROJECT_SOURCE_DIR}/src/ping.c
      ${PROJECT_SOURCE_DIR}/src/ping_req.c
      ${PROJECT_SOURCE_DIR}/src/update.c
      ${PROJECT_SOURCE_DIR}/src/event.c)

  if(CBOR)
    set(SOURCES ${SOURCES} ${PROJECT_SOURCE_DIR}/src/encode_cbor.c
                ${PROJECT_SOURCE_DIR}/src/decode_cbor.c)
  elseif(JSON)
    set(SOURCES ${SOURCES} ${PROJECT_SOURCE_DIR}/src/encode_json.c
                ${PROJECT_SOURCE_DIR}/src/decode_json.c)
  endif()

  add_library(microswim ${SOURCES})

  target_include_directories(microswim
                             PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

  if(CBOR)
    target_link_libraries(microswim PUBLIC cbor uuid)
  endif()
endif()

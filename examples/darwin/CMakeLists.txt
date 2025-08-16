cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 11)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_executable(
  darwin
  main.c
  ${PROJECT_SOURCE_DIR}/src/microswim.c
  ${PROJECT_SOURCE_DIR}/src/member.c
  ${PROJECT_SOURCE_DIR}/src/message.c
  ${PROJECT_SOURCE_DIR}/src/ping.c
  ${PROJECT_SOURCE_DIR}/src/ping_req.c
  ${PROJECT_SOURCE_DIR}/src/update.c
  ${PROJECT_SOURCE_DIR}/src/utils.c)

target_include_directories(darwin PUBLIC ${PROJECT_BINARY_DIR}
                                         ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(darwin PUBLIC cbor)

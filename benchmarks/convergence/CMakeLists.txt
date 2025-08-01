cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 11)

add_executable(
  convergence
  main.c
  ${PROJECT_SOURCE_DIR}/src/microswim.c
  ${PROJECT_SOURCE_DIR}/src/member.c
  ${PROJECT_SOURCE_DIR}/src/message.c
  ${PROJECT_SOURCE_DIR}/src/ping.c
  ${PROJECT_SOURCE_DIR}/src/ping_req.c
  ${PROJECT_SOURCE_DIR}/src/update.c
  ${PROJECT_SOURCE_DIR}/src/utils.c)

target_include_directories(convergence PUBLIC ${PROJECT_BINARY_DIR}
                                              ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(convergence PUBLIC cbor uuid hiredis)

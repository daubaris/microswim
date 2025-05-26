cmake_minimum_required(VERSION 3.20)

project(tests)

add_subdirectory(Unity)

set(SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/member.c
    ${CMAKE_SOURCE_DIR}/src/message.c
    ${CMAKE_SOURCE_DIR}/src/microswim.c
    ${CMAKE_SOURCE_DIR}/src/update.c
    ${CMAKE_SOURCE_DIR}/src/ping.c
    ${CMAKE_SOURCE_DIR}/src/ping_req.c
    ${CMAKE_SOURCE_DIR}/src/utils.c)
set(TEST_FILES test_member.c test_message.c test_ping.c test_ping_req.c
               test_update.c test_indices.c)

foreach(tfile ${TEST_FILES})
  get_filename_component(test_name ${tfile} NAME_WE)

  add_executable(${test_name} ${tfile} ${SRC_FILES})

  target_link_libraries(${test_name} PRIVATE unity cbor)

  target_include_directories(
    ${test_name} PUBLIC ${CMAKE_SOURCE_DIR}/include
                        ${PROJECT_SOURCE_DIR}/Unity/src)
endforeach()

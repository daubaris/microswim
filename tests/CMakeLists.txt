cmake_minimum_required(VERSION 3.20)

project(tests)

add_subdirectory(Unity)

set(TEST_FILES test_member.c test_message.c test_ping.c test_ping_req.c
               test_update.c)

foreach(tfile ${TEST_FILES})
  get_filename_component(test_name ${tfile} NAME_WE)

  add_executable(${test_name} ${tfile})

  target_link_libraries(${test_name} PRIVATE unity)

  target_include_directories(
    ${test_name} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
                        ${PROJECT_SOURCE_DIR}/Unity/src)
endforeach()

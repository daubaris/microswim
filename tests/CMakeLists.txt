cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 11)

set(UNITY_INCLUDE_DOUBLE
    ON
    CACHE BOOL "" FORCE)

include(FetchContent)
FetchContent_Declare(unity
                     GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git)
FetchContent_MakeAvailable(unity)

if(CUSTOM_CONFIGURATION)
  add_compile_definitions(CUSTOM_CONFIGURATION=1)
endif()

set(SOURCES
    test_main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils.c
    test_helpers.c
    test_microswim.c
    test_member.c
    test_message.c
    test_ping.c
    test_ping_req.c
    test_update.c
    test_m_event.c
    test_ipso.c
    ${PROJECT_SOURCE_DIR}/src/microswim.c
    ${PROJECT_SOURCE_DIR}/src/member.c
    ${PROJECT_SOURCE_DIR}/src/message.c
    ${PROJECT_SOURCE_DIR}/src/ping.c
    ${PROJECT_SOURCE_DIR}/src/ping_req.c
    ${PROJECT_SOURCE_DIR}/src/update.c
    ${PROJECT_SOURCE_DIR}/src/m_event.c
    ${PROJECT_SOURCE_DIR}/src/ipso.c)

if(CBOR)
  set(SOURCES
      ${SOURCES} test_encode_decode_cbor.c
      ${PROJECT_SOURCE_DIR}/src/encode_cbor.c
      ${PROJECT_SOURCE_DIR}/src/decode_cbor.c)
endif()

if(JSON)
  set(SOURCES ${SOURCES} ${PROJECT_SOURCE_DIR}/src/encode_json.c
              ${PROJECT_SOURCE_DIR}/src/decode_json.c)
endif()

add_executable(microswim_tests ${SOURCES})

target_include_directories(microswim_tests PUBLIC ${PROJECT_SOURCE_DIR}/include
                                                  ${PROJECT_BINARY_DIR})

if(CUSTOM_CONFIGURATION)
  target_include_directories(microswim_tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()

target_link_libraries(microswim_tests PUBLIC unity)

if(CBOR)
  target_link_libraries(microswim_tests PUBLIC cbor)
endif()

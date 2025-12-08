cmake_minimum_required(VERSION 3.20)

set(CMAKE_C_STANDARD 11)

if(CUSTOM_CONFIGURATION)
  add_compile_definitions(CUSTOM_CONFIGURATION=1)
endif()

set(SOURCES
    main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/utils.c
    ${PROJECT_SOURCE_DIR}/src/microswim.c
    ${PROJECT_SOURCE_DIR}/src/member.c
    ${PROJECT_SOURCE_DIR}/src/message.c
    ${PROJECT_SOURCE_DIR}/src/ping.c
    ${PROJECT_SOURCE_DIR}/src/ping_req.c
    ${PROJECT_SOURCE_DIR}/src/update.c)

if(CBOR)
  set(SOURCES ${SOURCES} ${PROJECT_SOURCE_DIR}/src/encode_cbor.c
              ${PROJECT_SOURCE_DIR}/src/decode_cbor.c)
elseif(JSON)
  set(SOURCES ${SOURCES} ${PROJECT_SOURCE_DIR}/src/encode_json.c
              ${PROJECT_SOURCE_DIR}/src/decode_json.c)
endif()

add_executable(convergence ${SOURCES})

target_include_directories(convergence PUBLIC ${PROJECT_BINARY_DIR}
                                              ${PROJECT_SOURCE_DIR}/include)

if(CUSTOM_CONFIGURATION)
  target_include_directories(convergence PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()

target_link_libraries(convergence PUBLIC cbor hiredis uuid)

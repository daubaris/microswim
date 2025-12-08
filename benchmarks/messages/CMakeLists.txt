cmake_minimum_required(VERSION 3.20)

find_package(benchmark REQUIRED)

set(CMAKE_CXX_STANDARD 17)

if(CUSTOM_CONFIGURATION)
  add_compile_definitions(CUSTOM_CONFIGURATION=1)
endif()

set(SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/utils.c
    ${PROJECT_SOURCE_DIR}/src/microswim.c
    ${PROJECT_SOURCE_DIR}/src/member.c
    ${PROJECT_SOURCE_DIR}/src/message.c
    ${PROJECT_SOURCE_DIR}/src/ping.c
    ${PROJECT_SOURCE_DIR}/src/ping_req.c
    ${PROJECT_SOURCE_DIR}/src/update.c)

set(SOURCES_JSON ${SOURCES} ${PROJECT_SOURCE_DIR}/src/encode_json.c
                 ${PROJECT_SOURCE_DIR}/src/decode_json.c)
set(SOURCES_CBOR ${SOURCES} ${PROJECT_SOURCE_DIR}/src/encode_cbor.c
                 ${PROJECT_SOURCE_DIR}/src/decode_cbor.c)

add_executable(messages_json main_json.cc ${SOURCES_JSON})
add_executable(messages_cbor main_cbor.cc ${SOURCES_CBOR})

target_compile_definitions(messages_json PRIVATE MICROSWIM_JSON=1)
target_compile_definitions(messages_cbor PRIVATE MICROSWIM_CBOR=1)

target_include_directories(messages_json PUBLIC ${PROJECT_BINARY_DIR}
                                                ${PROJECT_SOURCE_DIR}/include)
target_include_directories(messages_cbor PUBLIC ${PROJECT_BINARY_DIR}
                                                ${PROJECT_SOURCE_DIR}/include)
if(CUSTOM_CONFIGURATION)
  target_include_directories(messages_json PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_include_directories(messages_cbor PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
endif()

target_link_libraries(messages_json PUBLIC uuid benchmark::benchmark)
target_link_libraries(messages_cbor PUBLIC cbor uuid benchmark::benchmark)

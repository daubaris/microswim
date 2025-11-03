cmake_minimum_required(VERSION 3.20)

find_package(benchmark REQUIRED)

set(CMAKE_CXX_STANDARD 17)

set(SOURCES
    ${PROJECT_SOURCE_DIR}/src/microswim.c
    ${PROJECT_SOURCE_DIR}/src/member.c
    ${PROJECT_SOURCE_DIR}/src/message.c
    ${PROJECT_SOURCE_DIR}/src/ping.c
    ${PROJECT_SOURCE_DIR}/src/ping_req.c
    ${PROJECT_SOURCE_DIR}/src/update.c
    ${CMAKE_CURRENT_LIST_DIR}/utils.c)

add_executable(messages_json main_json.cc ${SOURCES})
add_executable(messages_cbor main_cbor.cc ${SOURCES})

target_compile_definitions(messages_json PRIVATE MICROSWIM_JSON=1)
target_compile_definitions(messages_cbor PRIVATE MICROSWIM_CBOR=1)

target_include_directories(messages_json PUBLIC ${PROJECT_BINARY_DIR}
                                                ${PROJECT_SOURCE_DIR}/include)
target_include_directories(messages_cbor PUBLIC ${PROJECT_BINARY_DIR}
                                                ${PROJECT_SOURCE_DIR}/include)

target_link_libraries(messages_json PUBLIC cbor uuid benchmark::benchmark)
target_link_libraries(messages_cbor PUBLIC cbor uuid benchmark::benchmark)

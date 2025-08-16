#include "message.h"
#include <benchmark/benchmark.h>
#include <string.h>

static const char* JSON_STRING =
    "{\"message\": 2, \"uuid\": \"1FA96E9D-AACE-4CA7-83B9-8083A75C7EAB\", \"uri\": "
    "\"127.0.0.1:8000\", \"updates\": [";

static const char* JSON_SINGLE_UPDATE =
    "{\"uuid\": "
    "\"5F70CD24-A1CB-466C-B871-3DF9D64A48AB\", \"uri\": \"127.0.0.1:5000\", "
    "\"status\": 0, \"incarnation\": 0}";

static void BENCHMARK_microswim_json_decoding(benchmark::State& state) {
    microswim_message_t message;
    char buffer[BUFFER_SIZE] = { 0 };
    strncpy(buffer, JSON_STRING, strlen(JSON_STRING));
    for (int i = 0; i < state.range(0); i++) {
        if (state.range(0) > 0) {
            strncat(buffer, JSON_SINGLE_UPDATE, strlen(JSON_SINGLE_UPDATE));
            if (i < state.range(0) - 1) {
                strncat(buffer, ",", 1);
            }
        }
    }

    strncat(buffer, "]}", 2);

    for (auto _ : state) {
        microswim_json_decode_message(&message, buffer, strlen(buffer));
    }
}

static void BENCHMARK_microswim_json_encoding(benchmark::State& state) {
    microswim_message_t message;
    char buffer[BUFFER_SIZE] = { 0 };
    strncpy(buffer, JSON_STRING, strlen(JSON_STRING));
    for (int i = 0; i < state.range(0); i++) {
        if (state.range(0) > 0) {
            strncat(buffer, JSON_SINGLE_UPDATE, strlen(JSON_SINGLE_UPDATE));
            if (i < state.range(0) - 1) {
                strncat(buffer, ",", 1);
            }
        }
    }

    strncat(buffer, "]}", 2);

    microswim_json_decode_message(&message, buffer, strlen(buffer));

    for (auto _ : state) {
        size_t len = microswim_json_encode_message(&message, (unsigned char*)buffer, BUFFER_SIZE);
    }
}

BENCHMARK(BENCHMARK_microswim_json_encoding)
    ->Args({ 1 })
    ->Args({ 2 })
    ->Args({ 3 })
    ->Args({ 4 })
    ->Args({ 5 })
    ->Args({ 6 })
    ->Args({ 7 })
    ->Args({ 8 })
    ->Args({ 9 });

BENCHMARK(BENCHMARK_microswim_json_decoding)
    ->Args({ 1 })
    ->Args({ 2 })
    ->Args({ 3 })
    ->Args({ 4 })
    ->Args({ 5 })
    ->Args({ 6 })
    ->Args({ 7 })
    ->Args({ 8 })
    ->Args({ 9 });

BENCHMARK_MAIN();

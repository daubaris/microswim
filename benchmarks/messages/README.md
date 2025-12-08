# messages

Build the benchmarks from the root directory (`microswim`):

```bash
cmake -DBUILD_EXAMPLES=0 -DBUILD_BENCHMARKS=1 -DCBOR=0 -DJSON=1 -DCUSTOM_CONFIGURATION=1 -DBUILD_LIBRARY=0 -DCMAKE_BUILD_TYPE=Release -B build -S .
```

Run the benchmark from the root directory:
```bash
# JSON benchmark
./build/benchmarks/messages/messages_json --benchmark_format=csv > results/messages/json.csv

# CBOR benchmark
./build/benchmarks/messages/messages_cbor --benchmark_format=csv > results/messages/cbor.csv
```



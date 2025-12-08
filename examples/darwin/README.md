# darwin

Build the examples from the root directory (`microswim`):

```bash
cmake -DBUILD_EXAMPLES=1 -DBUILD_BENCHMARKS=0 -DCBOR=1 -DJSON=0 -DCUSTOM_CONFIGURATION=1 -DBUILD_LIBRARY=0 -DCMAKE_BUILD_TYPE=Release -B build -S .
```

Enable CBOR or JSON encoding by using `-DCBOR=1` or `-DJSON=1`, respectively. However, do not enable both at the same time.

To run, open at least two terminals and launch the program with, for example:

```bash
# Terminal 1.
./build/examples/darwin/darwin 127.0.0.1 8000 127.0.0.1 8001

# Terminal 2.
./build/examples/darwin/darwin 127.0.0.1 8001 127.0.0.1 8000
```

You should see output, such as:
```
[DEBUG] ms->ping_count: 0
[DEBUG] ms->ping_req_count: 0
[DEBUG] ms->update_count: 2
[DEBUG] ms->member_count: 2
[DEBUG] ms->confirmed_count: 0
[DEBUG] ms->indices: [0 1]
[DEBUG] MESSAGE: ACK MESSAGE, FROM: 932C14BC-B447-4DAD-BB60-AB4A24A7AB23, STATUS: 0, INCARNATION: 0, URI: 8000
[DEBUG] UPDATES:
[DEBUG]         D3121467-63A8-438A-9174-40D801343986: STATUS: 0, INCARNATION: 0
[DEBUG]         932C14BC-B447-4DAD-BB60-AB4A24A7AB23: STATUS: 0, INCARNATION: 0
[DEBUG] Updated member's UUID
[DEBUG] MESSAGE: PING MESSAGE, FROM: 932C14BC-B447-4DAD-BB60-AB4A24A7AB23, STATUS: 0, INCARNATION: 0, URI: 8000
[DEBUG] UPDATES:
[DEBUG]         D3121467-63A8-438A-9174-40D801343986: STATUS: 0, INCARNATION: 0
[DEBUG]         932C14BC-B447-4DAD-BB60-AB4A24A7AB23: STATUS: 0, INCARNATION: 0
[DEBUG] Member: 932C14BC-B447-4DAD-BB60-AB4A24A7AB23 was marked alive
...
```

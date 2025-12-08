# µswim

A gossip protocol implementation/library suitable for resource constrained devices.

# Usage

There are multiple examples in the `examples` folder that showcase the general structure and usage of the library.

# Dependencies

Before using, make sure to install the required dependencies: [`benchmark`](https://github.com/google/benchmark) for message related benchmarks (encoding, decoding, message size); [`libcbor`](https://github.com/PJK/libcbor) for CBOR encoding support; [`hiredis`](https://github.com/redis/hiredis) for convergence measurements; `libuuid` via `apt-get install uuid-dev`; and python dependencies `pip install -r requirements.txt`.

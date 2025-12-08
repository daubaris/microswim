# convergence

Build the docker containers from the root directory (`microswim`):

```bash
docker build -t microswim-builder -f Dockerfile.builder .
docker build -t microswim -f Dockerfile.runtime .
```

Launch the experiment using:
```bash
python3 scripts/spawn.py --header benchmarks/convergence/configuration.h
```



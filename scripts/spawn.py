import docker
import time
import pathlib
import argparse
import redis
import re
from itertools import product
from concurrent.futures import ThreadPoolExecutor, as_completed


def command(self, remote):
    return f"./build/benchmarks/convergence/convergence 127.0.0.1 {self} 127.0.0.1 {remote}"


def spawn_redis_container(client):
    print("[+] Spawning Redis container...")
    node = client.api.create_container(
        "redis:latest",
        detach=True,
        host_config=client.api.create_host_config(port_bindings={6379: 6379}),
    )
    node = client.containers.get(node["Id"])
    node.start()
    print(f"[+] Redis container started: {node.id[:12]}")
    return node


def wait_for_redis_entries(redis_client, expected_count, poll_interval=1, timeout=480):
    print(f"[+] Waiting for {expected_count} entries in Redis...")
    start_time = time.time()
    while time.time() - start_time < timeout:
        count = len(redis_client.keys("*"))
        print(f"    -> Found {count}/{expected_count} keys in Redis...")
        if count >= expected_count:
            print("[+] All entries detected!")
            return True
        time.sleep(poll_interval)
    print("[!] Timeout waiting for Redis entries.")
    return False


def dump_redis_results_fast(redis_client, members, output_file, max_workers=16):
    print(f"[+] Dumping {members} Redis results (parallel) to {output_file} ...")

    def get_value(i):
        key = f"result:{i}"
        val = redis_client.get(key)
        return f"{key},{val or ''}\n"

    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        results = list(executor.map(get_value, range(8000, 8000 + members)))

    with open(output_file, "w") as f:
        f.writelines(results)

    print(f"[+] Results written to {output_file}")


def spawn_container(client, self, remote):
    path = pathlib.Path().resolve()
    node = client.api.create_container(
        "microswim",
        command=command(self, remote),
        detach=True,
        volumes=["/tmp", "/microswim/build"],
        host_config=client.api.create_host_config(
            network_mode="host",
            binds={
                f"{path}/results": {"bind": "/tmp", "mode": "rw"},
                f"{path}/build": {"bind": "/microswim/build", "mode": "ro"},
            },
        ),
        working_dir="/microswim",
    )
    node = client.containers.get(node["Id"])
    node.start()
    print(f"[+] Started microswim container: {node.id[:12]}")
    return node


def spawn_containers_parallel(client, start_ip, members, max_workers=8):
    print(f"[+] Spawning {members} microswim containers in parallel...")
    containers = []
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = []
        futures.append(executor.submit(spawn_container, client, start_ip, start_ip + 1))
        for i in range(start_ip, start_ip + members - 1):
            x, y = i, i + 1
            futures.append(executor.submit(spawn_container, client, y, x))
        for future in as_completed(futures):
            containers.append(future.result())
    return containers


def stop_and_remove_container(container, remove_volumes=False, force=False):
    """Helper to stop and remove a single container."""
    try:
        container.stop(timeout=2)
    except Exception as e:
        if not force:
            raise e
    # Remove the container
    container.remove(v=remove_volumes, force=force)


def stop_containers_fast(containers, max_workers=8, remove_volumes=False, force=False):
    print(f"[+] Stopping {len(containers)} containers quickly...")
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = {
            executor.submit(stop_and_remove_container, c, remove_volumes, force): c
            for c in containers
        }

        for future in as_completed(futures):
            c = futures[future]
            try:
                future.result()
                print(f"    -> Removed {c.id[:12]}")
            except Exception as e:
                print(f"    [!] Failed to stop/remove {c.id[:12]}: {e}")


def update_header_file(header_path, members, members_in_update, fanout):
    print(f"[+] Updating header: {header_path}")
    with open(header_path, "r") as f:
        content = f.read()

    content = re.sub(
        r"#define\s+MAXIMUM_MEMBERS\s+\d+",
        f"#define MAXIMUM_MEMBERS {members}",
        content,
    )
    content = re.sub(
        r"#define\s+MAXIMUM_MEMBERS_IN_AN_UPDATE\s+\d+",
        f"#define MAXIMUM_MEMBERS_IN_AN_UPDATE {members_in_update}",
        content,
    )
    content = re.sub(
        r"#define\s+GOSSIP_FANOUT\s+\d+", f"#define GOSSIP_FANOUT {fanout}", content
    )

    with open(header_path, "w") as f:
        f.write(content)

    print(
        f"    -> Set MAXIMUM_MEMBERS={members}, MAXIMUM_MEMBERS_IN_AN_UPDATE={members_in_update}, GOSSIP_FANOUT={fanout}"
    )


def spawn_build_container(client, source_dir):
    """Run a long-lived build container with the project mounted."""
    print("[+] Spawning microswim builder container...")
    node = client.api.create_container(
        "microswim-builder",
        command="sleep infinity",
        tty=True,
        detach=True,
        volumes=["/src"],
        host_config=client.api.create_host_config(
            binds={f"{source_dir}": {"bind": "/src", "mode": "rw"}}
        ),
        working_dir="/src",
    )
    node = client.containers.get(node["Id"])
    node.start()
    print(f"[+] Builder container started: {node.id[:12]}")
    return node


def build_inside_container(container):
    """Rebuild microswim binary inside the builder container."""
    print("[+] Building microswim binary inside container...")
    build_cmd = (
        "cmake -DBUILD_TESTS=0 -DBUILD_BENCHMARKS=1 "
        "-DBUILD_EXAMPLES=0 -DCUSTOM_CONFIGURATION=1 -DCBOR=1 -DJSON=0 -B build -S . && "
        "cmake --build build"
    )
    exit_code, output = container.exec_run(f"bash -lc '{build_cmd}'", demux=True)
    stdout = output[0].decode() if output[0] else ""
    stderr = output[1].decode() if output[1] else ""
    if exit_code != 0:
        print("[!] Build failed!\n", stderr)
        raise RuntimeError("Build failed inside container.")
    print("[+] Build completed successfully inside container.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--header", help="Path to microswim_custom_configuration.h", required=True
    )
    args = parser.parse_args()

    client = docker.client.from_env()
    source_dir = str(pathlib.Path().resolve())

    # Start persistent containers
    builder_container = spawn_build_container(client, source_dir)
    redis_container = spawn_redis_container(client)
    time.sleep(2)

    redis_client = redis.Redis(host="127.0.0.1", port=6379, decode_responses=True)

    members_list = [8, 16, 32, 64, 128]
    update_list = [2, 3, 4]
    fanout_list = [2, 3, 4]

    try:
        for iteration in range(1, 11):
            for members, members_in_update, fanout in product(
                members_list, update_list, fanout_list
            ):
                print("=" * 70)
                print(
                    f"[+] Running experiment: members={members}, updates={members_in_update}, fanout={fanout}, iteration={iteration}"
                )
                print("=" * 70)

                update_header_file(args.header, members, members_in_update, fanout)
                build_inside_container(builder_container)
                containers = spawn_containers_parallel(client, 8000, members)

                if wait_for_redis_entries(redis_client, members):
                    dump_redis_results_fast(
                        redis_client,
                        members,
                        f"results/results_{members}_{members_in_update}_{fanout}_{iteration}.csv",
                    )
                else:
                    print("[!] Not all containers wrote to Redis in time.")

                stop_containers_fast(containers)
                redis_client.flushall()
                print("[+] Redis flushed for next experiment.\n")

    finally:
        print("[+] Cleaning up builder and Redis containers...")
        try:
            builder_container.stop()
        except Exception:
            pass
        try:
            redis_container.stop()
        except Exception:
            pass
        print("[+] All experiments complete.")

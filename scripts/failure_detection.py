import argparse
import csv
import os
import pathlib
import re
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from itertools import product

import docker
import redis

BASE_PORT = 8000


def command(self_port, remote_port, drop_pct):
    return (
        f"./build/benchmarks/failure_detection/failure_detection "
        f"127.0.0.1 {self_port} 127.0.0.1 {remote_port} {drop_pct}"
    )


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


def spawn_container(client, self_port, remote_port, drop_pct):
    path = pathlib.Path().resolve()
    node = client.api.create_container(
        "microswim",
        command=command(self_port, remote_port, drop_pct),
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
    print(f"[+] Started microswim container (port {self_port}): {node.id[:12]}")
    return node


def spawn_containers_parallel(client, members, drop_pct, max_workers=8):
    print(
        f"[+] Spawning {members} microswim containers (drop={drop_pct}%) in parallel..."
    )
    containers = {}
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = {}
        f = executor.submit(spawn_container, client, BASE_PORT, BASE_PORT + 1, drop_pct)
        futures[f] = BASE_PORT
        for i in range(BASE_PORT, BASE_PORT + members - 1):
            f = executor.submit(spawn_container, client, i + 1, i, drop_pct)
            futures[f] = i + 1
        for future in as_completed(futures):
            port = futures[future]
            containers[port] = future.result()
    return containers


def stop_and_remove_container(container, force=False):
    try:
        container.stop(timeout=2)
    except Exception as e:
        if not force:
            raise e
    container.remove(force=force)


def stop_containers_fast(containers, max_workers=8, force=False):
    print(f"[+] Stopping {len(containers)} containers...")
    with ThreadPoolExecutor(max_workers=max_workers) as executor:
        futures = {
            executor.submit(stop_and_remove_container, c, force): cid
            for cid, c in containers.items()
        }
        for future in as_completed(futures):
            cid = futures[future]
            try:
                future.result()
                print(f"    -> Removed container for port {cid}")
            except Exception as e:
                print(f"    [!] Failed to stop/remove container for port {cid}: {e}")


def spawn_build_container(client, source_dir):
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
    print("[+] Building failure_detection binary inside container...")
    build_cmd = (
        "cmake -DBUILD_TESTS=0 -DBUILD_BENCHMARKS=1 "
        "-DBUILD_EXAMPLES=0 -DCUSTOM_CONFIGURATION=1 -DCBOR=1 -DJSON=0 -B build -S . && "
        "cmake --build build --target failure_detection"
    )
    exit_code, output = container.exec_run(f"bash -lc '{build_cmd}'", demux=True)
    stdout = output[0].decode() if output[0] else ""
    stderr = output[1].decode() if output[1] else ""
    if exit_code != 0:
        print("[!] Build failed!\n", stderr)
        raise RuntimeError("Build failed inside container.")
    print("[+] Build completed successfully.")


def update_header_file(header_path, members):
    print(f"[+] Patching {header_path} with MAXIMUM_MEMBERS={members}")
    with open(header_path, "r") as f:
        content = f.read()
    content = re.sub(
        r"#define\s+MAXIMUM_MEMBERS\s+\d+",
        f"#define MAXIMUM_MEMBERS {members}",
        content,
    )
    with open(header_path, "w") as f:
        f.write(content)


def wait_for_convergence(redis_client, members, timeout=300, poll_interval=2):
    """Wait until all N convergence:{port} keys appear in Redis."""
    expected_ports = set(range(BASE_PORT, BASE_PORT + members))
    print(f"[+] Waiting for convergence of {members} nodes (timeout={timeout}s)...")
    start = time.time()
    while time.time() - start < timeout:
        keys = redis_client.keys("convergence:*")
        ports_done = {int(k.split(":")[1]) for k in keys}
        print(f"    -> {len(ports_done)}/{members} nodes converged...")
        if expected_ports <= ports_done:
            print("[+] All nodes converged!")
            return True
        time.sleep(poll_interval)
    print("[!] Timeout waiting for convergence.")
    return False


def collect_events(redis_client, members):
    """Return dict: port -> list of (target_port, state, ts_ms)."""
    events = {}
    for port in range(BASE_PORT, BASE_PORT + members):
        key = f"event:{port}"
        raw = redis_client.lrange(key, 0, -1)
        parsed = []
        for entry in raw:
            m = re.match(r"target=(\d+):state=(\d+):ts=(\d+)", entry)
            if m:
                parsed.append((int(m.group(1)), int(m.group(2)), int(m.group(3))))
        events[port] = parsed
    return events


def compute_metrics(events, killed_port, kill_time_ms, detection_window_ms):
    fp_suspect_count = 0
    fp_confirmed_count = 0
    fn_count = 0
    detection_times = []
    per_node_fp_suspect = []
    per_node_fp_confirmed = []

    deadline = kill_time_ms + detection_window_ms

    for observer_port, evts in events.items():
        if observer_port == killed_port:
            continue

        detected = False
        node_fp_suspect = 0
        node_fp_confirmed = 0

        for target_port, state, ts in evts:
            is_fp = target_port != killed_port or ts < kill_time_ms
            if is_fp:
                if state == 1:
                    fp_suspect_count += 1
                    node_fp_suspect += 1
                elif state == 2:
                    fp_confirmed_count += 1
                    node_fp_confirmed += 1

            if (
                target_port == killed_port
                and state == 2
                and kill_time_ms <= ts <= deadline
            ):
                if not detected:
                    detection_times.append(ts - kill_time_ms)
                    detected = True

        if not detected:
            fn_count += 1

        per_node_fp_suspect.append(node_fp_suspect)
        per_node_fp_confirmed.append(node_fp_confirmed)

    n_observers = len(per_node_fp_suspect)
    fp_suspect_per_node = sum(per_node_fp_suspect) / n_observers if n_observers else 0
    fp_confirmed_per_node = (
        sum(per_node_fp_confirmed) / n_observers if n_observers else 0
    )

    return (
        fp_suspect_count,
        fp_confirmed_count,
        fn_count,
        detection_times,
        fp_suspect_per_node,
        fp_confirmed_per_node,
    )


RESULTS_DIR = pathlib.Path("results/failure_detection")
SUMMARY_PATH = RESULTS_DIR / "summary.csv"

SUMMARY_HEADER = [
    "members",
    "drop_pct",
    "iteration",
    "fp_suspect_count",
    "fp_confirmed_count",
    "fn_count",
    "fp_suspect_per_node",
    "fp_confirmed_per_node",
    "detection_mean_ms",
    "detection_median_ms",
    "detection_min_ms",
    "detection_max_ms",
    "detection_count",
]


def save_events_csv(events, killed_port, kill_time_ms, members, drop_pct, iteration):
    path = RESULTS_DIR / f"events_{members}_{drop_pct}_{iteration}.csv"
    with open(path, "w", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(["observer_port", "target_port", "state", "ts_ms"])
        for observer_port, evts in events.items():
            for target_port, state, ts in evts:
                writer.writerow([observer_port, target_port, state, ts])
    print(f"[+] Events written to {path}")


def append_summary(row):
    write_header = not SUMMARY_PATH.exists()
    with open(SUMMARY_PATH, "a", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=SUMMARY_HEADER)
        if write_header:
            writer.writeheader()
        writer.writerow(row)
    print(f"[+] Summary appended to {SUMMARY_PATH}")


def median(values):
    if not values:
        return 0
    s = sorted(values)
    n = len(s)
    mid = n // 2
    return s[mid] if n % 2 else (s[mid - 1] + s[mid]) / 2


def run_experiment(
    client,
    builder_container,
    redis_client,
    members,
    drop_pct,
    iteration,
    observation_duration,
    detection_window,
):
    print("=" * 70)
    print(f"[+] Experiment: members={members}, drop={drop_pct}%, iteration={iteration}")
    print("=" * 70)

    containers = spawn_containers_parallel(client, members, drop_pct)

    try:
        if not wait_for_convergence(redis_client, members):
            print("[!] Skipping experiment — convergence not reached.")
            return

        time.sleep(observation_duration)

        killed_port = BASE_PORT + members - 1
        victim = containers.pop(killed_port)

        try:
            victim.stop(timeout=2)
            victim.remove(force=True)
            kill_time_ms = int(time.time() * 1000)
            redis_client.set("kill_time", kill_time_ms)
            redis_client.set("killed_port", killed_port)

            print(
                f"[+] Killing victim node at port {killed_port} (ts={kill_time_ms}ms)..."
            )
        except Exception as e:
            print(f"    [!] Error stopping victim: {e}")

        print(f"[+] Detection window: {detection_window}s ...")
        time.sleep(detection_window)

        events = collect_events(redis_client, members)
        fp_suspect, fp_confirmed, fn, det_times, fp_suspect_pn, fp_confirmed_pn = (
            compute_metrics(events, killed_port, kill_time_ms, detection_window * 1000)
        )

        det_mean = sum(det_times) / len(det_times) if det_times else 0
        det_median = median(det_times)
        det_min = min(det_times) if det_times else 0
        det_max = max(det_times) if det_times else 0

        print(
            f"[+] Results: FP_suspect={fp_suspect}, FP_confirmed={fp_confirmed}, FN={fn}, "
            f"detection_time(mean={det_mean:.0f}ms, median={det_median:.0f}ms, "
            f"min={det_min}ms, max={det_max}ms, n={len(det_times)})"
        )

        save_events_csv(events, killed_port, kill_time_ms, members, drop_pct, iteration)
        append_summary(
            {
                "members": members,
                "drop_pct": drop_pct,
                "iteration": iteration,
                "fp_suspect_count": fp_suspect,
                "fp_confirmed_count": fp_confirmed,
                "fn_count": fn,
                "fp_suspect_per_node": f"{fp_suspect_pn:.4f}",
                "fp_confirmed_per_node": f"{fp_confirmed_pn:.4f}",
                "detection_mean_ms": f"{det_mean:.1f}",
                "detection_median_ms": f"{det_median:.1f}",
                "detection_min_ms": det_min,
                "detection_max_ms": det_max,
                "detection_count": len(det_times),
            }
        )

    finally:
        stop_containers_fast(containers, force=True)
        redis_client.flushall()
        print("[+] Redis flushed.\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Failure detection accuracy benchmark for µswim"
    )
    parser.add_argument(
        "--header",
        help="Path to failure_detection/configuration.h",
        default="benchmarks/failure_detection/configuration.h",
    )
    parser.add_argument(
        "--members",
        nargs="+",
        type=int,
        default=[8, 16, 32, 64],
        metavar="N",
        help="Network sizes to test (default: 8 16 32 64)",
    )
    parser.add_argument(
        "--drop-rates",
        nargs="+",
        type=int,
        default=[0, 10],
        metavar="PCT",
        help="Packet drop rates in %% (default: 0 5 10 20)",
    )
    parser.add_argument(
        "--iterations",
        type=int,
        default=3,
        help="Number of repetitions per (members, drop-rate) combination (default: 3)",
    )
    parser.add_argument(
        "--observation-duration",
        type=int,
        default=60,
        metavar="SECS",
        help="Stable window after convergence before killing the victim (default: 30)",
    )
    parser.add_argument(
        "--detection-window",
        type=int,
        default=60,
        metavar="SECS",
        help="Time to wait for detection after killing victim (default: 30)",
    )
    args = parser.parse_args()

    RESULTS_DIR.mkdir(parents=True, exist_ok=True)

    client = docker.client.from_env()
    source_dir = str(pathlib.Path().resolve())

    builder_container = spawn_build_container(client, source_dir)
    redis_container = spawn_redis_container(client)
    time.sleep(2)

    redis_client = redis.Redis(host="127.0.0.1", port=6379, decode_responses=True)

    try:
        prev_members = None
        for members, drop_pct, iteration in product(
            args.members, args.drop_rates, range(1, args.iterations + 1)
        ):
            if members != prev_members:
                update_header_file(args.header, members)
                build_inside_container(builder_container)
                prev_members = members

            run_experiment(
                client,
                builder_container,
                redis_client,
                members,
                drop_pct,
                iteration,
                args.observation_duration,
                args.detection_window,
            )

    finally:
        print("[+] Cleaning up builder and Redis containers...")
        try:
            builder_container.stop()
            builder_container.remove(force=True)
        except Exception:
            pass
        try:
            redis_container.stop()
            redis_container.remove(force=True)
        except Exception:
            pass
        print("[+] All experiments complete.")

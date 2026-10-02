# Routing performance benchmark

Regression benchmark for the Manager's message routing, to run before and after changes that touch the routing or display paths.

`run_benchmark.sh` starts the Manager headless and runs `loadtest.py` against it:

- the idle CPU usage, with no nodes connected,
- for each message rate: messages sent and received, loss, and one-way latency percentiles from a `FLIGHT_MODEL` node streaming `HIGHRES_IMU` to `INSTRUMENTS` nodes,
- Manager CPU usage sampled once per second during the run,
- with `-p`, the inclusive time of the hot paths from a `perf` profile.

## Requirements

- Python dialect generated with `scripts/update_mavlink.py` (`dist/all_marsh.py`)
- `perf` and `top` (Linux)
- no other process using UDP port 24400

## Usage

Build in Release, otherwise the numbers are dominated by unoptimized code:

```bash
cmake -B build-release -DCMAKE_BUILD_TYPE=Release
cmake --build build-release -j
scripts/profiling/run_benchmark.sh -b build-release/marsh-mgr -p
```

Options: `-r` rates in Hz (comma separated), `-d` seconds per rate, `-l` number of listeners, `-o` output directory (default `build/profiling`).

`loadtest.py` can also be run on its own against any Manager instance, see `loadtest.py --help`.

Latencies are measured on loopback with all processes on one machine, so they show the time spent inside the Manager plus the operating system's UDP overhead.

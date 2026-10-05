#! /bin/bash
# Regression benchmark for MARSH Manager routing performance.
#
# Starts the manager headless (offscreen platform), measures idle CPU usage, then runs
# loadtest.py while sampling the manager CPU usage once per second.
# With -p also records a perf profile under load and prints the inclusive time of the hot paths.
#
# Usage: run_benchmark.sh [-b binary] [-r rates] [-d duration] [-l listeners] [-o outdir] [-p]
# Build in Release for meaningful numbers, e.g.:
#   cmake -B build-release -DCMAKE_BUILD_TYPE=Release && cmake --build build-release -j
#   scripts/profiling/run_benchmark.sh -b build-release/marsh-mgr

set -e

SCRIPT_DIR=$(readlink -f "$(dirname "$0")")
REPO_ROOT=$(readlink -f "$SCRIPT_DIR/../..")

BINARY="$REPO_ROOT/build/marsh-mgr"
RATES="100,500,1000,2000,4000"
DURATION=5
LISTENERS=1
OUT_DIR="$REPO_ROOT/build/profiling"
PERF=0

while getopts "b:r:d:l:o:ph" opt; do
    case $opt in
        b) BINARY=$(readlink -f "$OPTARG") ;;
        r) RATES=$OPTARG ;;
        d) DURATION=$OPTARG ;;
        l) LISTENERS=$OPTARG ;;
        o) OUT_DIR=$OPTARG ;;
        p) PERF=1 ;;
        *) sed -n '2,12p' "$0" | sed 's/^# \?//'; exit 1 ;;
    esac
done

if [ ! -x "$BINARY" ]; then
    echo "Manager binary not found: $BINARY"
    exit 1
fi
if ss -uln 2>/dev/null | grep -q ':24400 '; then
    echo "Port 24400 is already in use, close other manager instances first"
    exit 1
fi
mkdir -p "$OUT_DIR"

QT_QPA_PLATFORM=offscreen "$BINARY" > "$OUT_DIR/manager.log" 2>&1 &
MANAGER=$!
trap 'status=$?; kill $MANAGER 2> /dev/null; wait $MANAGER 2> /dev/null || true; exit $status' EXIT
sleep 3

echo "== $(git -C "$REPO_ROOT" describe --tags --always --dirty) $(basename "$BINARY"), listeners: $LISTENERS"

# perf stat reports CPU time used by the process over the interval
IDLE_MS=$(perf stat -x, -e task-clock -p $MANAGER -- sleep 5 2>&1 | awk -F, '/task-clock/ {print $1}')
echo "idle CPU (no nodes): $(awk -v ms="$IDLE_MS" 'BEGIN {printf "%.1f", ms / 5000 * 100}') %"

python3 "$SCRIPT_DIR/loadtest.py" -r "$RATES" -d "$DURATION" -l "$LISTENERS" > "$OUT_DIR/loadtest.txt" &
LOADTEST=$!

top -b -d 1 -p $MANAGER | awk -v pid=$MANAGER '$1 == pid {print $9; fflush()}' > "$OUT_DIR/cpu.txt" &
TOP=$!

if [ $PERF == 1 ]; then
    # profile the middle of the second rate step
    sleep $(awk -v d="$DURATION" 'BEGIN {print 2 + d + 1 + d / 2}')
    perf record -F 999 -g -p $MANAGER -o "$OUT_DIR/perf.data" -- sleep 2 > /dev/null 2>&1
fi

wait $LOADTEST
kill $TOP

cat "$OUT_DIR/loadtest.txt"
echo "CPU samples (%, 1 s): $(tr ',\n' '. ' < "$OUT_DIR/cpu.txt")"

if [ $PERF == 1 ]; then
    echo "== inclusive time of hot paths (perf, second rate step)"
    perf report -i "$OUT_DIR/perf.data" --children --sort symbol --stdio -g none 2> /dev/null \
        | grep -E "Router::readPendingDatagrams|Router::receiveMessage|ClientNode::(send|receive)Message|NetworkDisplay::|Logger::writeMessage|QStandardItem::setData|QQuickText::setText|renderSceneGraph|mavlink_parse_char|QUdpSocket::(writeDatagram|receiveDatagram)" \
        | sed -E 's/ +/ /g; s/ - - *$//' | cut -c1-120 | head -20
fi

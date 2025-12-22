#!/bin/bash

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_ROOT/build"
BIN_DIR="$BUILD_DIR/bin"

SERVER_BIN="$BIN_DIR/tcp-server-example"
STRESSOR_BIN="$BIN_DIR/tcp-client-stressor"

SERVER_LOG="/tmp/ak24_tcp_server.log"
CLIENT_LOG="/tmp/ak24_tcp_client.log"

NUM_CHUNKS="${1:-100}"
CHUNK_SIZE="${2:-4096}"
PARALLELISM="${3:-10}"
PORT="${4:-9999}"

cleanup() {
    if [ -n "$SERVER_PID" ] && kill -0 "$SERVER_PID" 2>/dev/null; then
        kill -TERM "$SERVER_PID" 2>/dev/null || true
        wait "$SERVER_PID" 2>/dev/null || true
    fi
}

trap cleanup EXIT

if [ ! -x "$SERVER_BIN" ]; then
    echo "ERROR: Server binary not found: $SERVER_BIN"
    echo "Run 'make' from project root first."
    exit 1
fi

if [ ! -x "$STRESSOR_BIN" ]; then
    echo "ERROR: Stressor binary not found: $STRESSOR_BIN"
    echo "Run 'make' from project root first."
    exit 1
fi

rm -f "$SERVER_LOG" "$CLIENT_LOG"

echo "=== AK24 TCP Stress Test ==="
echo "Config: $PARALLELISM clients, $NUM_CHUNKS chunks x $CHUNK_SIZE bytes"
TOTAL_BYTES=$((NUM_CHUNKS * CHUNK_SIZE * PARALLELISM))
TOTAL_MB=$(echo "scale=2; $TOTAL_BYTES / 1024 / 1024" | bc)
echo "Total data: $TOTAL_BYTES bytes ($TOTAL_MB MB)"
echo ""
echo "Server log: $SERVER_LOG"
echo "Client log: $CLIENT_LOG"
echo ""

cd "$BUILD_DIR"

echo "Starting server on port $PORT..."
"$SERVER_BIN" > "$SERVER_LOG" 2>&1 &
SERVER_PID=$!

sleep 1

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "ERROR: Server failed to start"
    cat "$SERVER_LOG"
    exit 1
fi

echo "Server running (PID: $SERVER_PID)"
echo ""
echo "Starting stress test..."
echo ""

"$STRESSOR_BIN" 127.0.0.1 "$PORT" "$NUM_CHUNKS" "$CHUNK_SIZE" "$PARALLELISM" 2>&1 | tee "$CLIENT_LOG"

echo ""
echo "Stopping server..."
kill -TERM "$SERVER_PID" 2>/dev/null || true
wait "$SERVER_PID" 2>/dev/null || true
SERVER_PID=""

sleep 1

echo ""
echo "=== Hash Comparison ==="
echo ""

SERVER_HASHES=$(grep -o 'hash=0x[0-9A-Fa-f]*' "$SERVER_LOG" | sort | uniq -c | sort -rn)
CLIENT_HASHES=$(grep -o 'hash=0x[0-9A-Fa-f]*' "$CLIENT_LOG" | sort | uniq -c | sort -rn)

echo "Server hashes:"
echo "$SERVER_HASHES"
echo ""
echo "Client hashes:"
echo "$CLIENT_HASHES"
echo ""

SERVER_COUNT=$(grep -c '\[ID:' "$SERVER_LOG" || echo "0")
CLIENT_COUNT=$(grep -c '\[ID:' "$CLIENT_LOG" || echo "0")

echo "Server connections: $SERVER_COUNT"
echo "Client completions: $CLIENT_COUNT"
echo ""

if [ "$SERVER_COUNT" -eq "$PARALLELISM" ] && [ "$CLIENT_COUNT" -eq "$PARALLELISM" ]; then
    SERVER_UNIQUE=$(echo "$SERVER_HASHES" | wc -l | tr -d ' ')
    CLIENT_UNIQUE=$(echo "$CLIENT_HASHES" | wc -l | tr -d ' ')

    if [ "$SERVER_UNIQUE" -eq 1 ] && [ "$CLIENT_UNIQUE" -eq 1 ]; then
        SERVER_HASH=$(echo "$SERVER_HASHES" | awk '{print $2}')
        CLIENT_HASH=$(echo "$CLIENT_HASHES" | awk '{print $2}')

        if [ "$SERVER_HASH" = "$CLIENT_HASH" ]; then
            echo "PASS: All $PARALLELISM connections completed with matching hash $SERVER_HASH"
            exit 0
        else
            echo "FAIL: Hash mismatch - server=$SERVER_HASH client=$CLIENT_HASH"
            exit 1
        fi
    else
        echo "FAIL: Non-uniform hashes detected"
        exit 1
    fi
else
    echo "FAIL: Connection count mismatch (expected $PARALLELISM)"
    exit 1
fi

#!/bin/bash

# 종료 시 백그라운드 프로세스들을 정리하기 위한 배열
PIDS=()

cleanup() {
    echo ""
    echo "Stopping all REST test servers..."
    for pid in "${PIDS[@]}"; do
        if kill -0 "$pid" 2>/dev/null; then
            kill "$pid" 2>/dev/null
        fi
    done
    wait 2>/dev/null
    echo "All servers have been stopped."
    exit 0
}

# Ctrl+C (SIGINT) 및 종료 신호(SIGTERM) 수신 시 cleanup 함수 호출
trap cleanup SIGINT SIGTERM EXIT

echo "Starting all REST test servers (Ports: 20011 ~ 20019)..."

python3 get_server.py &
PIDS+=($!)

python3 post_server.py &
PIDS+=($!)

python3 put_server.py &
PIDS+=($!)

python3 patch_server.py &
PIDS+=($!)

python3 delete_server.py &
PIDS+=($!)

python3 head_server.py &
PIDS+=($!)

python3 options_server.py &
PIDS+=($!)

python3 trace_server.py &
PIDS+=($!)

python3 connect_server.py &
PIDS+=($!)

echo "All 9 servers are running in the background."
echo "Press [Ctrl + C] to terminate all servers."

# 모든 자식 프로세스가 종료될 때까지 대기
wait


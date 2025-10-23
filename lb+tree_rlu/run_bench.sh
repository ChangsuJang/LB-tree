#!/bin/bash

# 첫 번째 인자를 set_number로 사용
set_number=$1

# set_number가 전달되지 않으면 오류 메시지 출력
if [ -z "$set_number" ]; then
    echo "Error: Please provide a set_number as an argument (e.g., ./run_bench.sh 0)"
    exit 1
fi

# trace 디렉토리 생성 (없으면 생성)
TRACE_DIR="./trace"
mkdir -p "$TRACE_DIR"

# 파일명 설정
FLAMEGRAPH_SVG_PATH="$TRACE_DIR/flamegraph.svg"
OUT_PERF_PATH="$TRACE_DIR/out.perf"
OUT_FOLDED_PATH="$TRACE_DIR/out.folded"
PERF_DATA_PATH="$TRACE_DIR/perf.data"  # perf 데이터 파일 경로

# 이미 존재하는 flamegraph.svg 파일 제거
if [ -f "$FLAMEGRAPH_SVG_PATH" ]; then
    rm "$FLAMEGRAPH_SVG_PATH"
fi

# 이미 존재하는 perf.data 파일 제거
if [ -f "$PERF_DATA_PATH" ]; then
    echo "Removing existing perf.data..."
    rm "$PERF_DATA_PATH"
fi

# Valgrind + perf record + Python 벤치마크 실행
echo "Running benchmark with Valgrind and Perf, passing set_number: $set_number..."

# Valgrind + perf record + Python 실행
sudo valgrind --leak-check=full --track-origins=yes perf record -g python3 run_bench.py "$set_number"

# Flamegraph 생성 과정
FLAMEGRAPH_SCRIPT_PATH="Flamegraph"

# perf 데이터를 스크립트로 분석하여 folded 파일 생성
echo "Processing perf data..."
sudo perf script > "$OUT_PERF_PATH"

# Flamegraph 생성
echo "Generating Flamegraph..."
sudo $FLAMEGRAPH_SCRIPT_PATH/stackcollapse-perf.pl "$OUT_PERF_PATH" > "$OUT_FOLDED_PATH"
sudo $FLAMEGRAPH_SCRIPT_PATH/flamegraph.pl "$OUT_FOLDED_PATH" > "$FLAMEGRAPH_SVG_PATH"

# 성공적으로 생성된 경우 알림
if [ -f "$FLAMEGRAPH_SVG_PATH" ]; then
    echo "Flamegraph successfully created at $FLAMEGRAPH_SVG_PATH"
else
    echo "Flamegraph creation failed."
fi

# 벤치마크 종료 후, 빌드된 오브젝트 파일과 perf.data 삭제
echo "Cleaning up after benchmark..."

# make clean을 실행하여 이전 빌드 파일 삭제
make clean

rm -f perf.data
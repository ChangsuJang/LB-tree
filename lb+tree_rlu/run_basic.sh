#!/bin/bash

# 변수 설정
exec_mode="rlu"
bench_set=0
log_option=0

# gdb를 통해 실행
if [ "$use_gdb" = true ]; then
    echo "Starting gdb for debugging..."
    sudo gdb --args ./bench-"$exec_mode" --bench_set ./bench_set/"$bench_set".txt --log_option "$log_option"
else
    # gdb 없이 그냥 실행
    sudo ./bench-"$exec_mode" --bench_set ./bench_set/"$bench_set".txt --log_option "$log_option"
fi

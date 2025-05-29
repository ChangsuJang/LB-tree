import subprocess
import sys
import os
import pandas as pd
import re

def extract_bench_params(file_path):
    """Extracts execution mode and option from the specified file."""
    exec_mode = None
    bench_set = None
    log_option = None
    
    try:
        with open(file_path, 'r') as file:
            for line in file:
                key_value = line.strip().split(':')
                
                if len(key_value) == 2:
                    key = key_value[0].strip()
                    value = key_value[1].strip()
                    
                    if key == "EXEC MODE":
                        exec_mode = value
                    elif key == "BENCH_SET":
                        bench_set = value
                    elif key == "LOG_OPTION":
                        log_option = value

    except FileNotFoundError:
        print(f"File not found: {file_path}")
    except Exception as e:
        print(f"An error occurred while reading the file: {e}")
    
    return exec_mode, bench_set, log_option

def run_make():
    """Runs the 'make' command to compile the project."""
    make_command = ['make']
    
    try:
        result = subprocess.run(make_command, capture_output=True, text=True, check=True)
        print("Make output:")
        print(result.stdout)
        return True
    except subprocess.CalledProcessError as e:
        print("Make failed with errors:")
        print(e.stderr)
        return False

def run_bench(exec_mode, bench_set, log_option):
    """Runs the benchmark based on the extracted execution mode and option."""
    trace_dir = './trace'
    if not os.path.exists(trace_dir):
        os.makedirs(trace_dir)

    trace_log_path = os.path.join(trace_dir, 'trace.log')

    bench_command = [
        "strace", "-o", trace_log_path,
        f'./bench-{exec_mode}',
        '--bench_set', f'./bench_set/{bench_set}.txt',
        '--log_option', f'{log_option}'
    ]
    
    print(f"Running command: {' '.join(bench_command)}")
    
    try:
        result = subprocess.run(
            bench_command,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=True
        )
        print("Benchmark command executed successfully.")
        print("Output:\n", result.stdout)
        print(f"Benchmark option has been configured by [./bench_set/{bench_set}.txt] file")
        print(f"1. Execution Mode: {exec_mode}")
        print(f"2. Bench Set: {bench_set}")
        print(f"3. Log Option: {log_option}")
        return result.stdout
    except subprocess.CalledProcessError as e:
        print("Benchmark execution failed with the following errors:")
        print("Standard Output:\n", e.stdout)
        print("Standard Error:\n", e.stderr)

def extract_section(log, start_marker, end_marker=None):
    pattern = rf"{re.escape(start_marker)}(.*?){re.escape(end_marker) if end_marker else '={3,}'}"
    match = re.search(pattern, log, re.DOTALL)
    return match.group(1).strip() if match else ""

def parse_key_value_block(text):
    lines = text.strip().splitlines()
    data = {}
    for line in lines:
        if ':' in line:
            key, val = line.split(':', 1)
            data[key.strip()] = val.strip()
    return data

def create_benchmark_directory(base_dir='./benchmark_result'):
    """Creates the benchmark_result directory if it doesn't exist."""
    if not os.path.exists(base_dir):
        os.makedirs(base_dir)
    return base_dir

def create_subdirectory(directory, base_name):
    """Create a subdirectory numbered with base_name."""
    subdirectory = os.path.join(directory, base_name)
    if not os.path.exists(subdirectory):
        os.makedirs(subdirectory)
    return subdirectory

def get_next_available_filename(directory, base_name, subdirectory):
    """Finds the next available filename in the format `base_name_number.csv` inside the subdirectory."""
    i = 0
    while True:
        filename = os.path.join(subdirectory, f"{base_name}_{i}.csv")
        if not os.path.exists(filename):
            return filename
        i += 1

def parse_and_save_csv(log_text):
    # 메타 설정 저장
    setting_text = extract_section(log_text, "==========      SETTING TEST        ==========")
    meta_data = re.split(r'-{5,}.*?-{5,}', setting_text)
    meta_titles = re.findall(r'-{5,}\s*(.*?)\s*-{5,}', setting_text)
    meta_dict = {title.strip(): parse_key_value_block(data) for title, data in zip(meta_titles, meta_data[1:])}
    meta_df = pd.DataFrame(meta_dict)

    # benchmark_result 폴더 생성
    benchmark_dir = create_benchmark_directory()

    # 0번 서브디렉토리 생성
    subdirectory = create_subdirectory(benchmark_dir, "0")

    # 메타 설정 파일 생성
    meta_filename = get_next_available_filename(benchmark_dir, "meta_settings", subdirectory)
    meta_df.to_csv(meta_filename, encoding="utf-8-sig")

    # 측정 결과 저장
    meas_text = extract_section(log_text, "---------- TOTAL THREAD ----------", "----------      SET BY RUNNING-TIME")
    meas_dict = parse_key_value_block(meas_text)
    meas_tail = extract_section(log_text, "----------      SET BY RUNNING-TIME")
    meas_dict.update(parse_key_value_block(meas_tail))
    meas_df = pd.DataFrame.from_dict(meas_dict, orient='index', columns=["Measurement"])

    # 측정 결과 파일 생성
    meas_filename = get_next_available_filename(benchmark_dir, "meta_measurements", subdirectory)
    meas_df.to_csv(meas_filename, encoding="utf-8-sig")

    print("✅ CSV 파일로 결과 저장 완료")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        set_number = sys.argv[1]
    else:
        set_number = 0

    file_path = f'./bench_option/{set_number}.txt'
    exec_mode, bench_set, log_option = extract_bench_params(file_path)

    if exec_mode is None or bench_set is None:
        print("Failed to extract execution mode or bench set from the parameter file.")
    else:
        if run_make():
            log_output = run_bench(exec_mode, bench_set, log_option)
            parse_and_save_csv(log_output)

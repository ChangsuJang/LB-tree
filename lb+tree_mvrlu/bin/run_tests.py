# SPDX-FileCopyrightText: Copyright (c) 2018-2019 Virginia Tech
# SPDX-License-Identifier: Apache-2.0

import os
import sys
import time
import argparse
import subprocess
from typing import Dict, List

IS_NUMA = 1
IS_2_SOCKET = 0
IS_PERF = 0
TIMEOUT_SEC = 300

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

PERF_FILE = "__perf_output.file"
TMP_OUTPUT_FILENAME = '___temp.file'
W_OUTPUT_FILENAME = '__w_check.txt'

CMD_PREFIX_PERF = "perf stat -d -o %s" % (PERF_FILE,)

LBTREE_SCRIPT = os.path.join(SCRIPT_DIR, "plot_scripts", "lbtree")

CMD_PREFIX_LIBS = 'export LD_PRELOAD=\\"/usr/lib64/libtcmalloc_minimal.so.4\\"'
#CMD_PREFIX_LIBS = 'export LD_PRELOAD=\\"$LD_PRELOAD\\"'

CMD_NUMA_BIND_TO_CPU_0 = 'numactl --cpunodebind=0 '
CMD_NUMA_BIND_TO_CPU_1 = 'numactl --cpunodebind=1 '
CMD_NUMA_BIND_TO_CPU_0_1 = 'numactl --cpunodebind=0,1 '
CMD_NUMA_PREFIX_8  = 'taskset -c 0-7 '
CMD_NUMA_PREFIX_10 = 'taskset -c 0-9 '
CMD_NUMA_PREFIX_12 = 'taskset -c 0-11 '
CMD_NUMA_PREFIX_14 = 'taskset -c 0-13 '
CMD_NUMA_PREFIX_15 = 'taskset -c 0-14 '
CMD_NUMA_PREFIX_16 = 'taskset -c 0-15 '
CMD_NUMA_PREFIX_30 = 'taskset -c 0-29 '
CMD_NUMA_PREFIX_45 = 'taskset -c 0-44 '
CMD_NUMA_PREFIX_60 = 'taskset -c 0-59 '
CMD_NUMA_PREFIX_75 = 'taskset -c 0-74 '
CMD_NUMA_PREFIX_90 = 'taskset -c 0-89 '
CMD_NUMA_PREFIX_105= 'taskset -c 0-104 '
CMD_NUMA_PREFIX_120= 'taskset -c 0-119 '


BENCH_CMD_BASE = {
'zipf': 'zipf'
}

LBTREE_CMD_BASE = {
'rlu': 'bench-lbtree-rlu',
'rlu_ordo': 'bench-lbtree-rlu-ordo',
'mvrlu': 'bench-lbtree-mvrlu-gclk',
'mvrlu_ordo': 'bench-lbtree-mvrlu-ordo',
}

LBTREE_CMD_PARAMS = (
    '--num_threads %d '
	'--zipf_dist_val %f '
    '--seed %d '
    '--duration %d '
    '--key_range %d '
    '--initial_size %d '
    '--add_ratio %d '
    '--remove_ratio %d '
    '--search_ratio %d '
    '--scan_ratio %d '
    '--inner_degree %d '
    '--leaf_degree %d '
    '--split_threshold_ratio %d '
    '--merge_threshold_ratio %d '
	'--distribution_ratio %d '
	'--rlu_max_ws %f '
).strip()


result_keys = [
	'#ops          :',
	'#update ops   :',
#	't_writer_writebacks =',
#	't_writeback_q_iters =',
#	'a_writeback_q_iters =',
#	't_pure_readers =',
#	't_steals =',
#	't_sync_requests =',
#	't_sync_and_writeback =',
]

perf_result_keys = [
	'instructions              #',
	'branches                  #',
	'branch-misses             #',
	'L1-dcache-loads           #',
	'L1-dcache-load-misses     #',
]


def cmd_numa_prefix(threads_num: int) -> str:
	if IS_2_SOCKET:
		print(f'cmd_numa_prefix: BIND_CPU num_threads = {threads_num}')
		if threads_num <= 36:
			return CMD_NUMA_BIND_TO_CPU_1
		else:
			return CMD_NUMA_BIND_TO_CPU_0_1

	print(f'cmd_numa_prefix: num_threads = {threads_num}')

	if threads_num <= 15 :
		return CMD_NUMA_PREFIX_15
	if threads_num <= 30 :
		return CMD_NUMA_PREFIX_30
	if threads_num <= 45 :
		return CMD_NUMA_PREFIX_45
	if threads_num <= 60 :
		return CMD_NUMA_PREFIX_60
	if threads_num <= 75 :
		return CMD_NUMA_PREFIX_75
	if threads_num <= 90 :
		return CMD_NUMA_PREFIX_90
	if threads_num <= 105 :
		return CMD_NUMA_PREFIX_105
	if threads_num <= 120 :
		return CMD_NUMA_PREFIX_120

	raise ValueError(f'Unsupported num_threads={threads_num}')


def extract_data(output_data: str, key_str: str) -> float:

	data = output_data.split(key_str, 1)[1].split()[0].strip()

	if 'nan' in data:
		return 0.0

	if key_str in ('L1-dcache-load-misses #', 'branch-misses #'):
		data = data.strip('%')

	return float(data)


def extract_keys(output_data: str) -> Dict[str, float]:
	d: Dict[str, float] = {}

	for key in result_keys:
		d[key] = extract_data(output_data, key)

	if IS_PERF:
		for key in perf_result_keys:
			d[key] = extract_data(output_data, key)

	return d

def print_keys(dict_keys: Dict[str, float]) -> None:

	print('=================================')

	for key in result_keys:
		print(f'{key} {dict_keys.get(key, 0.0):.2f}')

	if IS_PERF:
		for key in perf_result_keys:
			print(f'{key} {dict_keys.get(key, 0.0):.2f}')


def run_test(runs_per_test: int, cmd: str) -> Dict[str, float]:

	total: Dict[str, float] = {key: 0.0 for key in result_keys}

	if IS_PERF:
		for key in perf_result_keys:
			total[key] = 0.0

	i = 0

	while i < runs_per_test:
		print(f'run {i}')

		if IS_PERF and os.path.exists(PERF_FILE):
			try: 
				os.unlink(PERF_FILE)
			except OSError:
				pass

		with open(W_OUTPUT_FILENAME, 'a', encoding = 'utf-8', errors='ignore') as wf:
			wf.write(f'\n=== BEFORE RUN {i} ===\n')
		subprocess.run(['bash', '-lc', f'w >> {W_OUTPUT_FILENAME}'], check = False)

		full_cmd = f'timeout {TIMEOUT_SEC} bash -lc "{cmd}; echo $? > done"'
		print(full_cmd)

		with open(TMP_OUTPUT_FILENAME, 'w', encoding= 'utf-8', errors='ignore') as outf:
			subprocess.run(full_cmd, shell=True, stdout=outf, stderr=subprocess.STDOUT, check=False)
		
		with open(W_OUTPUT_FILENAME, 'a', encoding='utf-8', errors='ignore') as wf:
			wf.write(f'\n=== AFTER RUN ===\n')
		subprocess.run(['bash', '-lc', f'w >> {W_OUTPUT_FILENAME}'], check = False)

		if not os.path.exists('done'):
			print('no done file (timeout/kill) - retrying...')
			time.sleep(5)
			continue
		
		with open ('done', 'r', encoding='utf-8', errors='ignore') as df:
			rv = df.read().strip()
		try:
			rc = int(rv)
		except ValueError:
			rc = -1
		finally:
			try:
				os.unlink('done')
			except OSError:
				pass
		
		if rc == 124:
			print('timeout (124) - retyring this run')
			time.sleep(5)
			continue

		if rc == -11:
			print('got -11 (SEGV) - retyring this run')
			time.sleep(5)
			continue

		with open(TMP_OUTPUT_FILENAME, 'r', encoding='utf-8', errors='ignore') as f:
			output_data = f.read()
		try:
			os.unlink(TMP_OUTPUT_FILENAME)
		except OSError:
			pass

		if IS_PERF and os.path.exists(PERF_FILE):
			with open(PERF_FILE, 'r', encoding='utf-8', errors='ignore') as pf:
				output_data += pf.read()
			try:
				os.unlink(PERF_FILE)
			except OSError:
				pass
		
		print("------------------------------------")
		print(output_data)
		print("------------------------------------")

		# dict_keys = extract_keys(output_data)
		# print_keys(dict_keys)
# 
		# for key, value in dict_keys.items():
			# total[key] = total.get(key, 0.0) + value
# 
		# i += 1
# 
		for key in list(total.keys()):
			total[key] = total[key] / max(runs_per_test, 1)
			return total



def print_run_results(f_out, rlu_max_ws: int, update_rate: int, num_threads: int, dict_keys: Dict[str, float]) -> None:

	f_out.write(f'\n {rlu_max_ws:.2f} {update_rate:.2f} {num_threads:.2f}')

	for key in result_keys:
		f_out.write(f' {dict_keys.get(key, 0.0):.2f}')
	if IS_PERF:
		for key in perf_result_keys:
			f_out.write(f' {dict_keys.get(key, 0.0):.2f}')
	f_out.flush()

def execute_lbtree(
	bench_type: str,
	alg_type: str,
	index_type: str,
	num_threads_list: List[int],
	runs_per_test: int,
	zipf_dist_val: float,
	seed: int,
	duration: int,
	key_range: int,
	initial_size: int,
	add_ratio: int,
	remove_ratio: int,
	search_ratio: int,
	scan_ratio: int,
	inner_degree: int,
	leaf_degree: int,
	split_threshold_ratio: int,
	merge_threshold_ratio: int,
	distribution_ratio: int,
	rlu_max_ws: float,
	output_filename) -> None:
		
		if alg_type in ('mvrlu', 'mvrlu_ordo'):
			key_name = '                        n_aborts ='
		else:
			key_name = 't_aborts               ='
		
		if key_name not in result_keys:
			result_keys.append(key_name)

		with open(W_OUTPUT_FILENAME, 'w', encoding='utf-8') as f_w:
			f_w.write('')

		update_rate = add_ratio + remove_ratio

		if index_type == "lbtree":
			base = os.path.join(SCRIPT_DIR, BENCH_CMD_BASE[bench_type], LBTREE_CMD_BASE[alg_type])
	
		with open(output_filename, 'w', encoding='utf-8') as f_out:

			for num_threads in num_threads_list:
					params = (LBTREE_CMD_PARAMS % (
						num_threads,
						zipf_dist_val,
						seed,
						duration,
						key_range,
						initial_size,
						add_ratio,
						remove_ratio,
						search_ratio,
						scan_ratio,
						inner_degree,
						leaf_degree,
						split_threshold_ratio,
						merge_threshold_ratio,
						distribution_ratio,
						rlu_max_ws
					))

					cmd = (base + ' ' + params).strip()
			
					print('-------------------------------')
					print(f'[{num_threads}] {cmd}')

					numa_prefix = cmd_numa_prefix(num_threads)

					if numa_prefix:
						cmd = numa_prefix + cmd
					if IS_PERF:
						cmd = CMD_PREFIX_PERF + cmd


					dict_keys = run_test(runs_per_test, cmd)
					print('complete')
					print_run_results(f_out, rlu_max_ws, update_rate, num_threads, dict_keys)

			f_out.write('\n')
			f_out.flush()
			
			try:
				result_keys.remove(key_name)
			except ValueError:
				pass

			print(f'DONE: written output to {output_filename}')



if '__main__' == __name__:

	parser = argparse.ArgumentParser(description='Index Benchmark Runner (py3) — long-only flags')

	parser.add_argument('--index_type_list', default=['lbtree'], help='Index type (label only)')
	parser.add_argument('--alg_type_list', default=['mvrlu_ordo'], help='Algorithm type (rlu|rlu_ordo|mvrlu|mvrlu_ordo)')

	parser.add_argument('--num_threads_list', type=int, nargs='+', default=[1], help='List of thread counts')

	# run controls
	parser.add_argument('--runs_per_test', type=int, default=1, help='Runs per test')
	parser.add_argument('--duration', type=int, default=1000, help='Duration (ms)')
	parser.add_argument('--seed', type=int, default=1, help='Seed')

	# workload mix
	parser.add_argument('--zipf_dist_val', type=float, default=0.0, help='Zipf distribution value (float)')
	parser.add_argument('--key_range', dest='key_range', type=int, default=100000, help='Key range')
	parser.add_argument('--initial_size', type=int, default=500, help='Initial size of index')
	parser.add_argument('--add_ratio', type=int, default=10)
	parser.add_argument('--remove_ratio', type=int, default=10)
	parser.add_argument('--search_ratio', type=int, default=70)
	parser.add_argument('--scan_ratio', type=int, default=10)
	parser.add_argument('--inner_degree', type=int, default=10)
	parser.add_argument('--leaf_degree', type=int, default=10)
	parser.add_argument('--split_threshold_ratio', type=int, default=75)
	parser.add_argument('--merge_threshold_ratio', type=int, default=25)
	parser.add_argument('--distribution_ratio', type=int, default=50)
	parser.add_argument('--rlu_max_ws', type=int, default=1)


	# output layout
	parser.add_argument('--outdir', default='results', help='Root output directory')
	parser.add_argument('--output_filename', default='__result.txt', help='Output file name')


	# optional graph
	parser.add_argument('--generate_graph', type=int, default=0, help='Generate graph (1|0)')


	opts = parser.parse_args()


	# validate
	if not opts.num_threads_list:
		print('Number of threads required (use --num_threads_list N [N ...])')
		parser.print_help(); sys.exit(1)


	# compute update rate for dir naming
	update_rate = int(opts.add_ratio) + int(opts.remove_ratio)


	for index_type in opts.index_type_list:
		for alg_type in opts.alg_type_list:
			result_dir = os.path.join(SCRIPT_DIR, opts.outdir, 'zipf', str(index_type), str(alg_type), f'u{update_rate}')
			os.makedirs(result_dir, exist_ok=True)

			try:
				if str(opts.index_type).lower() == 'lbtree':
					cmd = 'cp %s/* %s' % (LBTREE_SCRIPT, result_dir)
				print(cmd)
				subprocess.check_output(cmd, shell=True)
			except Exception:
				pass

			output_path = os.path.join(result_dir, opts.output_filename)

			execute_lbtree(
				'zipf',
				index_type = index_type,
				alg_type = alg_type,
				num_threads_list = opts.num_threads_list,
				runs_per_test=opts.runs_per_test,
				zipf_dist_val = opts.zipf_dist_val,
				seed = opts.seed,
				duration = opts.duration,
				key_range = opts.key_range,
				initial_size = opts.initial_size,
				add_ratio = opts.add_ratio,
				remove_ratio = opts.remove_ratio,
				search_ratio = opts.search_ratio,
				scan_ratio = opts.scan_ratio,
				inner_degree = opts.inner_degree,
				leaf_degree = opts.leaf_degree,
				split_threshold_ratio = opts.split_threshold_ratio,
				merge_threshold_ratio = opts.merge_threshold_ratio,
				distribution_ratio = opts.distribution_ratio,
				rlu_max_ws = opts.rlu_max_ws,
				output_filename = output_path
			)


			# if opts.generate_graph == 1:
			# 	cmd = 'cd %s && sh extr_txs_tree.sh && cd -' % result_dir
			# 	print(cmd)

			# try:
				# subprocess.check_output(cmd, shell=True)
			# except subprocess.CalledProcessError as e:
				# print('Graph generation failed:', e)

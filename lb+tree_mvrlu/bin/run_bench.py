#!/usr/bin/env python
# -*- coding: utf-8 -*-

# SPDX-FileCopyrightText: Copyright (c) 2018-2019 Virginia Tech
# SPDX-License-Identifier: Apache-2.0

import shutil
import json
import os
import argparse


import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt

from run_tests import execute_lbtree

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

LBTREE_SCRIPT = os.path.join(SCRIPT_DIR, "plot_scripts", "lbtree")

if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Index Benchmark Runner (Py3)')
    parser.add_argument("-c", "--config", default="config.json", help="config file")
    parser.add_argument("-p", "--perf", type = int, choices=[0,1], default=0,  help="enable perf mode (0=off, 1=on)")
    args = parser.parse_args()


#Create result directory
    config_path = os.path.join(SCRIPT_DIR, args.config)

#Make benches
    with open(config_path, 'r', encoding='utf-8') as json_data_file:
        bench_list = json.load(json_data_file)

    for bench_data in bench_list:
        update_rate = int(bench_list[bench_data][0]["add_ratio"]) + int(bench_list[bench_data][0]["remove_ratio"])
        index_type_list = bench_list[bench_data][0]["index_type"]
        alg_type_list = bench_list[bench_data][0]["alg_type"]

        for index_type in index_type_list:
            for alg_type in alg_type_list:
                result_dir = os.path.join(SCRIPT_DIR, 'results', bench_data, index_type, alg_type, f'u{update_rate}')

                if os.path.exists(result_dir):
                    shutil.rmtree(result_dir)

                os.makedirs(result_dir, exist_ok=True)
                
                if args.perf == 1:
                    perf_dir = os.path.join(result_dir, 'perf')
                    os.makedirs(perf_dir, exist_ok=True)
                else:
                    perf_dir = None

                if index_type == "lbtree":
                    execute_lbtree(
                        bench_data, alg_type, index_type,                       bench_list[bench_data][0]["num_threads"],
                        bench_list[bench_data][0]["runs_per_test"],             bench_list[bench_data][0]["zipf_dist_val"],
                        bench_list[bench_data][0]["seed"],                      bench_list[bench_data][0]["duration"],
                        bench_list[bench_data][0]["key_range"],                 bench_list[bench_data][0]["initial_size"],
                        bench_list[bench_data][0]["add_ratio"],                 bench_list[bench_data][0]["remove_ratio"],
                        bench_list[bench_data][0]["search_ratio"],              bench_list[bench_data][0]["scan_ratio"],
                        bench_list[bench_data][0]["inner_degree"],              bench_list[bench_data][0]["leaf_degree"],
                        bench_list[bench_data][0]["split_threshold_ratio"],     bench_list[bench_data][0]["merge_threshold_ratio"],
                        bench_list[bench_data][0]["distribution_ratio"],        bench_list[bench_data][0]["rlu_max_ws"],
                        result_dir, perf_dir
                    )
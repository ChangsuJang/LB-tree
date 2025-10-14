#!/usr/bin/env python
# -*- coding: utf-8 -*-

# SPDX-FileCopyrightText: Copyright (c) 2018-2019 Virginia Tech
# SPDX-License-Identifier: Apache-2.0

import json
import sys
import os
import subprocess
import argparse


import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt

from run_tests import execute_lbtree

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))

LBTREE_SCRIPT = os.path.join(SCRIPT_DIR, "plot_scripts", "lbtree")
HLIST_SCRIPT = os.path.join(SCRIPT_DIR, "plot_scripts", "hlist")
RESULTS_DIR  = os.path.join(SCRIPT_DIR, "..", "results")

def parseresults(log_file, plot_data, alg_type, duration, data_file):
    with open(log_file, 'r', encoding='utf-8', errors='ignore') as f:
        fp = f.readlines()

    i = 0
    plot_data[alg_type] = {}
    plot_data[alg_type]['tot_ops'] = []
    plot_data[alg_type]['abrt_ratio'] = []
    data_file.write("#" + alg_type + ": thr  ops/us\n")

    for line in fp:
        if i <= 1:
            i += 1
            continue
        w = line.split()
        if not w:
            break

        thd = (w[2])
        tot_ops = w[3]
        plot_data[alg_type]['tot_ops'].append(float(tot_ops) / duration / 1000.0)
        abrts = (w[5])
        plot_data[alg_type]['abrt_ratio'].append(float(abrts) / (float(abrts) + float(tot_ops)))
        data_file.write(f"{thd} {float(tot_ops) / duration / 1000.0}\n")
        #print thd
        #print tot_ops
        
def plotgraph(plot_data, num_threads, update_rate, index_type, initial_size, graph_type, final_dir):
    fig = plt.figure()
    title = f"{index_type}_{graph_type}_u{update_rate}_i{initial_size}"
    fig.suptitle(title)
    ax = fig.add_subplot(111)

    for key in plot_data:
        ax.plot(num_threads, plot_data[key][graph_type], marker='o', linestyle='-', label=key)

    ax.set_xlabel('num-threads')
    if graph_type == 'tot_ops':
        ax.set_ylabel('Ops/us')
    else:
        ax.set_ylabel('Abort Ratio')

    ax.legend(loc='upper left')

    os.makedirs(final_dir, exist_ok=True)
    #plt.show()
    fig.savefig(os.path.join(final_dir, title + '.png'))
    plt.close(fig)


if __name__ == '__main__':

    parser = argparse.ArgumentParser(description='Index Benchmark Runner (Py3)')
    parser.add_argument("-d", "--dest", default="temp", help="destination folder")
    parser.add_argument("-c", "--config", default="config.json", help="config file")
    parser.add_argument("-p", "--plot", action="store_true", help="plot only")
    args = parser.parse_args()


#Create result directory
    config_path = os.path.join(SCRIPT_DIR, args.config)
    result_dir = os.path.join(RESULTS_DIR, args.dest)
    os.makedirs(result_dir, exist_ok=True)


#Make benches
    #if opts.plot == False:
    #    status = subprocess.check_output('make clean -C ..; make -C ..', shell=True)

    #Read config files

    with open(config_path, 'r', encoding='utf-8') as json_data_file:
        bench_type = json.load(json_data_file)

    for bench_data in bench_type:
        update_rate = int(bench_type[bench_data][0]["add_ratio"]) + int(bench_type[bench_data][0]["remove_ratio"])
        result_dir = os.path.join()
    
        for update_rate in data[test][0]["update-rate"]:
            final_dir = result_dir + test + "/u" + str(update_rate) + "/";

            try:
                os.stat(final_dir)
            except:
                os.makedirs(final_dir)

            plot_data = {}
            data_file = open(final_dir + "plot_data.dat", "w+")

            for alg_type in data[test][0]["alg-type"]:

                for index_type in data[test][0]["index-type"]:
                    out_file = final_dir + "__" + alg_type + "_" + index_type + "_" + str(data[test][0]["initial-size"]) + "_u" + str(update_rate) + ".txt"

                    if opts.plot == False:

                        if index_type == "lbtree":
                            execute_lbtree( alg_type, data[test][0]["runs-per-test"], data[test][0]["duration"], data[test][0]["distribution"], data[test][0]["inner-degree"], data[test][0]["initial-size"], data[test][0]["leaf-degree"], \
                                            data[test][0]["num-threads"], data[test][0]["merge-threshold"], data[test][0]["range"], data[test][0]["rlu-max-ws"], data[test][0]["seed"], data[test][0]["split-threshold"], update_rate, data[test][0]["zipf-dist-val"], out_file)
# 
                parseresults(out_file, plot_data, alg_type, \
                        data[test][0]["duration"], data_file)
                
            plotgraph(plot_data, data[test][0]["num-threads"], update_rate, data[test][0]["index-type"], data[test][0]["initial-size"], 'tot_ops', final_dir)
            plotgraph(plot_data, data[test][0]["num-threads"], update_rate, data[test][0]["index-type"], data[test][0]["initial-size"], 'abrt_ratio', final_dir)
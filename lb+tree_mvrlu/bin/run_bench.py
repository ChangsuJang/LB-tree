#!/usr/bin/env python
# -*- coding: utf-8 -*-

# SPDX-FileCopyrightText: Copyright (c) 2018-2019 Virginia Tech
# SPDX-License-Identifier: Apache-2.0

import json
import sys
from run_tests import execute_lbtree, execute_hash
import optparse
import os
import subprocess
import matplotlib as mpl
mpl.use('Agg')
import matplotlib.pyplot as plt

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
os.chdir(SCRIPT_DIR)  # 상대경로(예: config.json)가 항상 bin 기준으로 동작

def parseresults(log_file, plot_data, alg_type, duration, data_file):
    fp = open(log_file).readlines()
    i = 0
    plot_data[alg_type] = {}
    plot_data[alg_type]['tot_ops'] = []
    plot_data[alg_type]['abrt_ratio'] = []
    data_file.write("#"+alg_type+": thr  ops/us"+"\n")

    for line in fp:
        if i <= 1:
            i += 1
            continue
        w = line.split()
        if not w:
            break
        
        thd = (w[2])
        tot_ops = w[3]
        plot_data[alg_type]['tot_ops'].append(float(tot_ops)/duration/1000)
        abrts = (w[5])
        plot_data[alg_type]['abrt_ratio'].append(float(abrts)/(float(abrts)+float(tot_ops)))
        data_file.write(str(thd) + " " + str(float(tot_ops)/duration/1000) + "\n")
        #print thd
        #print tot_ops
        
def plotgraph(plot_data, num_threads, update_rate, index_type, initial_size, graph_type, final_dir):

    fig = plt.figure()
    title = index_type + '_' + graph_type + '_u' + str(update_rate) + '_i' + str(initial_size)
    fig.suptitle(title)
    ax = fig.add_subplot(111)
    for keys in plot_data:
        ax.plot(num_threads, plot_data[keys][graph_type], marker='o', linestyle='-', label = keys )
    ax.set_xlabel('num-threads')
    if graph_type == 'tot_ops':
        ax.set_ylabel('Ops/us')
    else:
        ax.set_ylabel('Abort Ratio')
    ax.legend(loc = 'upper left')
    #plt.show()
    fig.savefig(final_dir+title+'.png')


parser = optparse.OptionParser()
parser.add_option("-d", "--dest", default = "temp",
        help = "destination folder")
parser.add_option("-c", "--config", default = "config.json",
        help = "config file")
parser.add_option("-p", "--plot", default = False,
        help = "plot only")


(opts, args) = parser.parse_args()

#Create result directory
result_dir = "./results/" + opts.dest + "/"
try:
    os.stat(result_dir)
except:
    os.makedirs(result_dir)


#Make benches
#if opts.plot == False:
#    status = subprocess.check_output('make clean -C ..; make -C ..', shell=True)

#Read config files
with open(opts.config) as json_data_file:
    data = json.load(json_data_file)

for test in data:
    if data[test][0]["index-type"] == "llist":
        if data[test][0]["buckets"] != 1:
            sys.exit("Buckets should be 1\n");
    
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
                    else:
                        if index_type == "llist":
                            if data[test][0]["buckets"] != 1:
                                sys.exit("Buckets should be 1\n");
                        execute_hash(   alg_type, data[test][0]["runs-per-test"], data[test][0]["buckets"], data[test][0]["duration"], data[test][0]["initial-size"], data[test][0]["num-threads"], data[test][0]["range"], data[test][0]["rlu-max-ws"], \
                                        data[test][0]["seed"], update_rate, data[test][0]["zipf-dist-val"], out_file)


        #     parseresults(out_file, plot_data, alg_type, \
        #             data[test][0]["duration"], data_file)
        #     
        # plotgraph(plot_data, data[test][0]["num-threads"], update_rate, data[test][0]["index-type"], data[test][0]["initial-size"], 'tot_ops', final_dir)
        # plotgraph(plot_data, data[test][0]["num-threads"], update_rate, data[test][0]["index-type"], data[test][0]["initial-size"], 'abrt_ratio', final_dir)
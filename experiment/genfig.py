# BSD 2-Clause License

# Copyright (c) 2022, Kasper Skott

# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:

# 1. Redistributions of source code must retain the above copyright notice, this
#    list of conditions and the following disclaimer.

# 2. Redistributions in binary form must reproduce the above copyright notice,
#    this list of conditions and the following disclaimer in the documentation
#    and/or other materials provided with the distribution.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os
import os.path
import csv
import matplotlib.pyplot as plt
import numpy as np

class benchdata:
    def __init__(self, data: list) -> None:
        self.runs        = data[0]
        self.elapsed     = data[1]
        self.mean_dev    = data[2]
        self.elapsed_avg = sum(self.elapsed) / len(self.elapsed)
        self.elapsed_max = max(self.elapsed)
        self.elapsed_min = min(self.elapsed)
        self.std         = np.std(self.elapsed)
        pass

    def calc_speedup_over_bitmask(self, bitmask_eq):
        self.relativeBitmask = (bitmask_eq.elapsed_avg / self.elapsed_avg) - 1.0

def get_csv_files(dir: str) -> "tuple[str, str, str, str]":
    csv_files = [(dir + "br/" + f) for f in os.listdir(dir + "/br/") if f.endswith(".csv")]
    br = csv_files[0]

    csv_files = [(dir + "bs/" + f) for f in os.listdir(dir + "/bs/") if f.endswith(".csv")]
    bs = csv_files[0]

    csv_files = [(dir + "ur/" + f) for f in os.listdir(dir + "/ur/") if f.endswith(".csv")]
    ur = csv_files[0]

    csv_files = [(dir + "us/" + f) for f in os.listdir(dir + "/us/") if f.endswith(".csv")]
    us = csv_files[0]
    return br, bs, ur, us

def read_csv_data(filePath: str) -> list:
    data = list()
    file = open(filePath, "tr")
    reader = csv.reader(file, delimiter=',')

    data.append([])
    data.append([])
    data.append([])

    for i, row in enumerate(reader):
        if i == 0:
            continue
        data[0].append(int(row[0]))
        data[1].append(float(row[1]))
        data[2].append(float(row[2]))

    file.close()
    return data

# Takes in data from either register or stack benchmarks, both union and bitmasking.
def gen_elapsed_by_arch(filePath, O0_bitm, O0_union, O1_bitm, O1_union, O3_bitm, O3_union):
    arch_cat     = ['-O0', '-O1', '-O3']
    bitmask_min  = [O0_bitm.elapsed_min, O1_bitm.elapsed_min, O3_bitm.elapsed_min]
    bitmask_avg  = [O0_bitm.elapsed_avg, O1_bitm.elapsed_avg, O3_bitm.elapsed_avg]
    union_avg    = [O0_union.elapsed_avg, O1_union.elapsed_avg, O3_union.elapsed_avg]
    union_min    = [O0_union.elapsed_min, O1_union.elapsed_min, O3_union.elapsed_min]

    x = np.arange(len(arch_cat))
    fig, ax = plt.subplots(figsize=(9,5))
    ax.bar(x - 0.1, bitmask_avg, 0.2, color=(0.80, 0.80, 0.80))
    ax.bar(x - 0.1, bitmask_min, 0.2, label='Bitmasking', color=(0.50, 0.50, 0.50))
    ax.bar(x + 0.1, union_avg, 0.2, color=(0.70, 0.70, 0.70))
    ax.bar(x + 0.1, union_min, 0.2, label='Union', color=(0.30, 0.30, 0.30))
    ax.set_ylabel('Elapsed Time (ms)')
    ax.set_ylim(bottom=O3_union.elapsed_avg - 50.0, top=O0_union.elapsed_avg + 50.0)
    ax.set_xticks(x, arch_cat)
    ax.yaxis.set_minor_locator(plt.MultipleLocator(12.5))
    ax.grid()
    ax.legend()
    plt.savefig(filePath)

# Takes in data from register and stack benchmarks, using union decoding.
def gen_speedup(filePath, O0_reg, O0_stack, O1_reg, O1_stack, O3_reg, O3_stack):
    arch_cat      = ['-O0', '-O1', '-O3']
    reg_speedup   = [O0_reg.relativeBitmask, O1_reg.relativeBitmask, O3_reg.relativeBitmask]
    stack_speedup = [O0_stack.relativeBitmask, O1_stack.relativeBitmask, O3_stack.relativeBitmask]

    x = np.arange(len(arch_cat))
    fig, ax = plt.subplots(figsize=(7,5))
    ax.bar(x - 0.1, reg_speedup, 0.2, label='Register', color=(0.50, 0.50, 0.50))
    ax.bar(x + 0.1, stack_speedup, 0.2, label='Stack', color=(0.30, 0.30, 0.30))
    ax.set_ylabel('Speedup')
    ax.set_ylim(bottom=-0.04, top=0.04)
    ax.set_xticks(x, arch_cat)
    ax.yaxis.set_major_locator(plt.MultipleLocator(0.01))
    ax.yaxis.set_minor_locator(plt.MultipleLocator(0.005))
    ax.grid()
    ax.legend()
    plt.savefig(filePath)

def generate_all():
    
    O0_bit_reg, O0_bit_stack, \
    O0_union_reg, O0_union_stack = get_csv_files("./data/GCC/O0/")

    O1_bit_reg, O1_bit_stack, \
    O1_union_reg, O1_union_stack = get_csv_files("./data/GCC/O1/")

    O3_bit_reg, O3_bit_stack, \
    O3_union_reg, O3_union_stack = get_csv_files("./data/GCC/O3/")

    # Read and analyze data for each benchmark run.
    O0_bit_reg_data     = benchdata(read_csv_data(O0_bit_reg))
    O0_bit_stack_data   = benchdata(read_csv_data(O0_bit_stack))
    O0_union_reg_data   = benchdata(read_csv_data(O0_union_reg))
    O0_union_stack_data = benchdata(read_csv_data(O0_union_stack))

    O1_bit_reg_data     = benchdata(read_csv_data(O1_bit_reg))
    O1_bit_stack_data   = benchdata(read_csv_data(O1_bit_stack))
    O1_union_reg_data   = benchdata(read_csv_data(O1_union_reg))
    O1_union_stack_data = benchdata(read_csv_data(O1_union_stack))

    O3_bit_reg_data     = benchdata(read_csv_data(O3_bit_reg))
    O3_bit_stack_data   = benchdata(read_csv_data(O3_bit_stack))
    O3_union_reg_data   = benchdata(read_csv_data(O3_union_reg))
    O3_union_stack_data = benchdata(read_csv_data(O3_union_stack))

    O0_union_reg_data.calc_speedup_over_bitmask(O0_bit_reg_data)
    O0_union_stack_data.calc_speedup_over_bitmask(O0_bit_stack_data)
    O1_union_reg_data.calc_speedup_over_bitmask(O1_bit_reg_data)
    O1_union_stack_data.calc_speedup_over_bitmask(O1_bit_stack_data)
    O3_union_reg_data.calc_speedup_over_bitmask(O3_bit_reg_data)
    O3_union_stack_data.calc_speedup_over_bitmask(O3_bit_stack_data)

    gen_elapsed_by_arch("out/register_elapsed.png", \
                        O0_bit_reg_data, O0_union_reg_data, \
                        O1_bit_reg_data, O1_union_reg_data, \
                        O3_bit_reg_data, O3_union_reg_data)

    gen_elapsed_by_arch("out/stack_elapsed.png", \
                        O0_bit_stack_data, O0_union_stack_data, \
                        O1_bit_stack_data, O1_union_stack_data, \
                        O3_bit_stack_data, O3_union_stack_data)

    gen_speedup("out/union_speedup.png", \
                O0_union_reg_data, O0_union_stack_data, \
                O1_union_reg_data, O1_union_stack_data, \
                O3_union_reg_data, O3_union_stack_data)

    return

if __name__ == "__main__":
    generate_all()
    exit()

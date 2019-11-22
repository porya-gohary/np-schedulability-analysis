#!/usr/bin/env python3

import argparse

import csv
import re

import random


from math import ceil, floor, log10

from collections import defaultdict

import fractions

US_TO_NS = 1000
MS_TO_US = 1000

def next_power_of_10(x):
    return 10**(ceil(log10(x)))

def ms2us(x):
    return int(ceil(MS_TO_US * x))

def lcm(a,b):
    return abs(a * b) // fractions.gcd(a,b) if a and b else 0

def hyperperiod(periods):
    h = 1
    for p in periods:
        h = lcm(h, p)
    return h


    return (periods, deadlines, nodes)


def task_format_csv(j):
    return "{0:1},{1:1},{2:1},{3:1}\n".format(*j)

def job_format_csv(j):
    return "{0:1},{1:1},{2:1},{3:1},{4:1},{5:1},{6:1}".format(*j)

def write_tasks_jobs(fname, tasks, jobs):
    f = open(fname, 'w')
    for j in tasks:
        f.write(task_format_csv(tasks[j]))

    for j in jobs:
        f.write(job_format_csv(jobs[j]))
        prec= str(jobs[j][7])
        if prec != '':
            prec= "," + prec
        f.write(str(prec) + "\n")
    f.close()

def parse_args():
    parser = argparse.ArgumentParser(
        description="Generate random tasks")

    parser.add_argument('-t', '--tasks', dest='tasks', default=3,
                        action='store',
                        help='how many random tasks should be created')

    parser.add_argument('-j', '--jobs', dest='jobs', default=3,
                        action='store',
                        help='maximum number of vertex in each tasks')

    parser.add_argument('-max', '--maximum', dest='max', default=100,
                        action='store',
                        help='maximum number')

    parser.add_argument('-p', '--precedence', dest='prec', default=0,
                        action='store',
                        help='maximum precedence per job')

    parser.add_argument('--save', default=None, dest='save',
                        required=True,
                        help='name to store the random dag')

    return parser.parse_args()

def prio_table(params):
    p = [(params[tid], tid) for tid in params]
    p.sort()
    lut = {}
    for i, (_, tid) in enumerate(p):
        lut[tid] = i + 1
    return lut


def generate_tasks(n_tasks,maxim):
    all_tasks = {}
    for i in range(n_tasks):
       period=random.randrange(2,maxim)
       deadline=random.randrange(period,maxim)
       all_tasks[i] = ('T',i+1,period,deadline)

    return all_tasks

def generate_jobs(n_tasks, n_jobs, m_pre, all_tasks):
    all_jobs = {}
    for i in range(n_tasks):
        length=random.randrange(1,n_jobs)
        for j in range(length):
            task_id=all_tasks[i][1]
            period=all_tasks[i][2]
            deadline=all_tasks[i][3]
            job_id=j+1
            r_min=random.randrange(0,int(period/4)+1)
            r_max=r_min + random.randrange(0,int(period/4)+1)
            bcet=random.randrange(0,int(deadline/4)+1)
            wcet=bcet + random.randrange(0,int(deadline/4)+1)
            #generate precedence

            str_pre = ""
            if m_pre >= 1 and length > 1:
                if m_pre > length:
                    m_pre = length;

                pre = random.randrange(0, m_pre)
                for p in range(pre):
                    str_pre = str_pre + str(random.randrange(1,length)) + ","
                str_pre = str(random.randrange(1,length))

            all_jobs["T" + str(task_id) + "J" + str(job_id)] = ('V',task_id,job_id,r_min,r_max,bcet,wcet,str_pre)

    return all_jobs;

def main():
    opts = parse_args()

    #random.seed(1)

    all_tasks = generate_tasks(int(opts.tasks), int(opts.max))
    all_jobs = generate_jobs(int(opts.tasks), int(opts.jobs), int(opts.prec), all_tasks)

    #print (all_tasks)
    #print(all_jobs)

    write_tasks_jobs(opts.save, all_tasks, all_jobs)

if __name__ == '__main__':
    main()

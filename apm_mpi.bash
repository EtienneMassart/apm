#!/bin/bash -l

#SBATCH -n 3
#SBATCH -N 3

mpirun apm 0 dna/small_chrY.fa AGAA ACCAGTGTGTACAC AGAA TGTA ACCAGT
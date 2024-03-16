#!/bin/bash -l

#SBATCH -n 8
#SBATCH -N 1

mpirun apm 0 dna/small_chrY.fa AGAA ACCAGTGTGTACAC AGAA TGTA ACCAGT
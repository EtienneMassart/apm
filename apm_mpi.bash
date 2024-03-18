#!/bin/bash -l

#SBATCH -n 8
#SBATCH -N 1

# mpirun apm 0 dna/big_chr.fa AGAA ACCAGTGTGTACAC AGAA TGTA ACCAGT
mpirun apm 0 dna/medium_chr.fa AGAA ACCAGTGTGTACAC AGAA TGTA ACCAGT

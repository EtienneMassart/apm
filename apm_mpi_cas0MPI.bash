#!/bin/bash -l

#SBATCH -n 1
#SBATCH -N 1

mpirun apm 0 dna/big_chr.fa GTAGGGTTGCACGAGACAGAACTAGCTTAGGCACTATTTGTCCAAATACA

# Passe de 26 à 18 secondes 

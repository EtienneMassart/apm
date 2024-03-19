#ifndef AUX_H
#define AUX_H

void openmp_sequence(int nb_patterns, char ** pattern, int n_bytes, int approx_factor, char * buf, int * n_matches);

void openmp_patterns(int nb_patterns, char ** pattern, int n_bytes, int approx_factor, char * buf, int * n_matches);

void openmp_seq_simple(char * pattern, int n_bytes, int approx_factor, char * buf, int * n_matches, int i);

#endif
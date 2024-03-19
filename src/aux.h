#ifndef AUX_H
#define AUX_H

void cas1_OpenMP(int nb_patterns, char ** pattern, int n_bytes, int approx_factor, char * buf, int * n_matches);

void cas2_OpenMP(int nb_patterns, char ** pattern, int n_bytes, int approx_factor, char * buf, int * n_matches);

void cas1_OpenMP_aux(int début, int fin, char * pattern, int n_bytes, int approx_factor, char * buf, int * n_matches, int i);

#endif
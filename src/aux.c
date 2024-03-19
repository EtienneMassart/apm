#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include "aux.h"
#include "apm.h"
#include "apm_cuda.h"


void cas0_OpenMP(int nb_patterns, char ** pattern, int n_bytes, int approx_factor, char * buf, int * n_matches){
    for (int i = 0; i < nb_patterns; i++) {
        // printf("Processing with OpenMP thread %d\n", omp_get_thread_num());
        int size_pattern = strlen(pattern[i]);
        int *column;

        /* Initialize the number of matches to 0 */
        n_matches[i] = 0;

        column = (int *)malloc((size_pattern + 1) * sizeof(int));
        // if (column == NULL) {
        //     fprintf(stderr,
        //             "Error: unable to allocate memory for column (%ldB)\n",
        //             (size_pattern + 1) * sizeof(int));
        //     return 1;
        // }

        /* Traverse the input data up to the end of the file */
        for (int j = 0; j < n_bytes; j++) {
            int distance = 0;
            int size;

#if APM_DEBUG
            if (j % 100 == 0) {
                printf("Procesing byte %d (out of %d)\n", j, n_bytes);
            }
#endif

            size = size_pattern;
            if (n_bytes - j < size_pattern) {
                size = n_bytes - j;
            }

            distance = levenshtein(pattern[i], &buf[j], size, column);

            if (distance <= approx_factor) {
                n_matches[i]++;
            }
        }

        free(column);
    }
}

void cas1_OpenMP(int nb_patterns, char ** pattern, int n_bytes, int approx_factor, char * buf, int * n_matches){
    for (int i = 0; i < nb_patterns; i++) {
        // printf("Processing with OpenMP thread %d\n", omp_get_thread_num());
        int size_pattern = strlen(pattern[i]);
        int *column;

        /* Initialize the number of matches to 0 */
        n_matches[i] = 0;

        /* Traverse the input data up to the end of the file */
# pragma omp parallel for private(column) reduction(+:n_matches[i])
        for (int j = 0; j < n_bytes; j++) {
            // on est obligé d'allouer l'espace pour la column plus tard pour la rendre privée
            column = (int *)malloc((size_pattern + 1) * sizeof(int));
            int distance = 0;
            int size;

#if APM_DEBUG
            if (j % 100 == 0) {
                printf("Procesing byte %d (out of %d)\n", j, n_bytes);
            }
#endif

            size = size_pattern;
            if (n_bytes - j < size_pattern) {
                size = n_bytes - j;
            }

            distance = levenshtein(pattern[i], &buf[j], size, column);

            if (distance <= approx_factor) {
                n_matches[i]++;
            }
            free(column);
        }

    }
}

void cas1_OpenMP_aux(int début, int fin, char * pattern, int n_bytes, int approx_factor, char * buf, int * n_matches, int i){
    int size_pattern = strlen(pattern);
    int *column;

    printf("patern en ce moment = %s\n", pattern);
    printf("int i = %d\n", i);     

    /* Initialize the number of matches to 0 */
    n_matches[i] = 0;        
# pragma omp parallel for private(column) reduction(+:n_matches[i])
    for (int j = début; j < fin ; j++) {
        // on est obligé d'allouer l'espace pour la column plus tard pour la rendre privée
        column = (int *)malloc((size_pattern + 1) * sizeof(int));
        int distance = 0;
        int size;

#if APM_DEBUG
        if (j % 100 == 0) {
            printf("Procesing byte %d (out of %d)\n", j, n_bytes);
        }
#endif

        size = size_pattern;
        if (n_bytes - j < size_pattern) {
            size = n_bytes - j;
        }

        distance = levenshtein(pattern, &buf[j], size, column);

        if (distance <= approx_factor) {
            n_matches[i]++;
        }
        free(column);
    }
    printf("n_matches[i] = %d\n", n_matches[i]);
}

void cas_rien_aux(int début, int fin, char * pattern, int n_bytes, int approx_factor, char * buf, int * n_matches, int i){
    int size_pattern = strlen(pattern);
    int *column;

    /* Initialize the number of matches to 0 */
    n_matches[i] = 0;        
    for (int j = début; j < fin ; j++) {
        // on est obligé d'allouer l'espace pour la column plus tard pour la rendre privée
        column = (int *)malloc((size_pattern + 1) * sizeof(int));
        int distance = 0;
        int size;

#if APM_DEBUG
        if (j % 100 == 0) {
            printf("Procesing byte %d (out of %d)\n", j, n_bytes);
        }
#endif

        size = size_pattern;
        if (n_bytes - j < size_pattern) {
            size = n_bytes - j;
        }

        distance = levenshtein(pattern, &buf[j], size, column);

        if (distance <= approx_factor) {
            n_matches[i]++;
        }
        free(column);
    }
}

void cas2_OpenMP(int nb_patterns, char ** pattern, int n_bytes, int approx_factor, char * buf, int * n_matches){
    #pragma omp parallel for
    for (int i = 0; i < nb_patterns; i++) {
        // printf("Processing with OpenMP thread %d\n", omp_get_thread_num());
        int size_pattern = strlen(pattern[i]);
        int *column;

        /* Initialize the number of matches to 0 */
        n_matches[i] = 0;

        column = (int *)malloc((size_pattern + 1) * sizeof(int));

        /* Traverse the input data up to the end of the file */
        for (int j = 0; j < n_bytes; j++) {
            int distance = 0;
            int size;

#if APM_DEBUG
            if (j % 100 == 0) {
                printf("Procesing byte %d (out of %d)\n", j, n_bytes);
            }
#endif

            size = size_pattern;
            if (n_bytes - j < size_pattern) {
                size = n_bytes - j;
            }

            distance = levenshtein(pattern[i], &buf[j], size, column);

            if (distance <= approx_factor) {
                n_matches[i]++;
            }
        }

        free(column);
    }
}
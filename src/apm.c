/**
 * APPROXIMATE PATTERN MATCHING
 *
 * INF560
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <mpi.h>
#include <assert.h>

#define APM_DEBUG 0
#define ETIENNE_DEBUG 0

char *read_input_file(char *filename, int *size) {
    char *buf;
    off_t fsize;
    int fd = 0;
    int n_bytes = 1;

    /* Open the text file */
    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        fprintf(stderr, "Unable to open the text file <%s>\n", filename);
        return NULL;
    }

    /* Get the number of characters in the textfile */
    fsize = lseek(fd, 0, SEEK_END);
    if (fsize == -1) {
        fprintf(stderr, "Unable to lseek to the end\n");
        return NULL;
    }

#if APM_DEBUG
    printf("File length: %ld\n", fsize);
#endif

    /* Go back to the beginning of the input file */
    if (lseek(fd, 0, SEEK_SET) == -1) {
        fprintf(stderr, "Unable to lseek to start\n");
        return NULL;
    }

    /* Allocate data to copy the target text */
    buf = (char *)malloc(fsize * sizeof(char));
    if (buf == NULL) {
        fprintf(stderr, "Unable to allocate %ld byte(s) for main array\n",
                fsize);
        return NULL;
    }

    n_bytes = read(fd, buf, fsize);
    if (n_bytes != fsize) {
        fprintf(
            stderr,
            "Unable to copy %ld byte(s) from text file (%d byte(s) copied)\n",
            fsize, n_bytes);
        return NULL;
    }

#if APM_DEBUG
    printf("Number of read bytes: %d\n", n_bytes);
#endif

    *size = n_bytes;

    close(fd);

    return buf;
}

int compare(const void *a, const void *b) {
    int la = strlen(*(const char **)a);
    int lb = strlen(*(const char **)b);
    if (la < lb) {
        return 1;
    } else if (la > lb) {
        return -1;
    } else {
        return 0;
    }
}



#define MIN3(a, b, c) \
    ((a) < (b) ? ((a) < (c) ? (a) : (c)) : ((b) < (c) ? (b) : (c)))

int levenshtein(char *s1, char *s2, int len, int *column) {
    unsigned int x, y, lastdiag, olddiag;

    for (y = 1; y <= len; y++) {
        column[y] = y;
    }
    for (x = 1; x <= len; x++) {
        column[0] = x;
        lastdiag = x - 1;
        for (y = 1; y <= len; y++) {
            olddiag = column[y];
            column[y] = MIN3(column[y] + 1, column[y - 1] + 1,
                             lastdiag + (s1[y - 1] == s2[x - 1] ? 0 : 1));
            lastdiag = olddiag;
        }
    }
    return (column[len]);
}

int main(int argc, char **argv) {
    char **pattern;
    char *filename;
    int *pattern_len_squared; // tableau contenant la taille au carré de chaque patern
    int approx_factor = 0;
    int nb_patterns = 0;
    int sum_len_squared = 0; // somme des tailles au carré des paterns
    int i, j;
    char *buf;
    struct timeval t1, t2;
    double duration;
    int n_bytes;
    int *n_matches;
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* Check number of arguments */
    if (argc < 4) {
        printf(
            "Usage: %s approximation_factor "
            "dna_database pattern1 pattern2 ...\n",
            argv[0]);
        return 1;
    }

    /* Get the distance factor */
    approx_factor = atoi(argv[1]);

    /* Grab the filename containing the target text */
    filename = argv[2];

    /* Get the number of patterns that the user wants to search for */
    nb_patterns = argc - 3;

    /* Fill the pattern array */
    pattern = (char **)malloc(nb_patterns * sizeof(char *));
    if (pattern == NULL) {
        fprintf(stderr, "Unable to allocate array of pattern of size %d\n",
                nb_patterns);
        return 1;
    }


    pattern_len_squared = (int *)malloc(nb_patterns * sizeof(int));

    /* Grab the patterns */
    for (i = 0; i < nb_patterns; i++) {
        int l;

        l = strlen(argv[i + 3]);
        if (l <= 0) {
            fprintf(stderr, "Error while parsing argument %d\n", i + 3);
            return 1;
        }
        
        pattern[i] = (char *)malloc((l + 1) * sizeof(char));
        if (pattern[i] == NULL) {
            fprintf(stderr, "Unable to allocate string of size %d\n", l);
            return 1;
        }

        strncpy(pattern[i], argv[i + 3], (l + 1));
    }

    /* Sort patterns by length in decreasing order*/
    
    qsort(pattern, nb_patterns, sizeof(char *), compare);

    /* Compute the sum of the length squared of each pattern */
    for (i = 0; i < nb_patterns; i++) {
        pattern_len_squared[i] = strlen(pattern[i]) * strlen(pattern[i]);
        sum_len_squared += pattern_len_squared[i];
    }

    int sls_by_process = sum_len_squared / size;

    if (rank == 0) {
        printf(
            "Approximate Pattern Mathing: "
            "looking for %d pattern(s) in file %s w/ distance of %d\n",
            nb_patterns, filename, approx_factor);
    }

    buf = read_input_file(filename, &n_bytes);
    if (buf == NULL) {
        return 1;
    }

    /* Allocate the array of matches */
    n_matches = (int *)malloc(nb_patterns * sizeof(int));
    if (n_matches == NULL) {
        fprintf(stderr, "Error: unable to allocate memory for %ldB\n",
                nb_patterns * sizeof(int));
        return 1;
    }

    /*****
     * BEGIN MAIN LOOP
     ******/

    /* Timer start */
    gettimeofday(&t1, NULL);


    int * distribution = (int *)malloc(nb_patterns * sizeof(int)); // contient les numéros des paterns
    int * sendcounts = (int *)malloc(size * sizeof(int)); // contient le nombre de paterns envoyé à chaque process et sert aussi à calculer le décalage et recvcount = sendcounts[rank]
    int * displs = (int *)malloc(size * sizeof(int));

    /* Répartition du nombre de patterns entre les process 
    Peut être amélioré en prenant en compte la taille des paterns */

    int count_squared_len = 0;
    displs[0] = 0;
    j = 0; // j est le numéro du process

    int max_receive = 0;

    for (i = 0; i < nb_patterns; i++) {
        distribution[i] = i;
        assert (j < size), "N'a pas réparti tous les patterns entre les process\n";
        count_squared_len += pattern_len_squared[i];

        if (count_squared_len > sls_by_process * (j + 1) || i == nb_patterns - 1){
            sendcounts[j] = i + 1  - displs[j];
            if (sendcounts[j] > max_receive) {
                max_receive = sendcounts[j];
            }
            if (j < size - 1) {
                displs[j+1] = i+1;
            }
            j++;
        }
        
        while (count_squared_len > sls_by_process * (j + 1) && j < size - 1) {

            sendcounts[j] = 0;
            displs[j+1] = i+1;
            j++;
        }
    }

    int * reception = (int *)malloc(max_receive * sizeof(int));

    if (rank == 0) {
        for (i = 0; i < size; i++) {
            printf("sendcounts[%d] = %d\n", i, sendcounts[i]);
            printf("displs[%d] = %d\n", i, displs[i]);
        }
    }


    MPI_Scatterv(distribution, sendcounts, displs, MPI_INT, reception, sendcounts[rank], MPI_INT, 0, MPI_COMM_WORLD);
#if ETIENNE_DEBUG
    for (i = 0; i < sendcounts[rank]; i++) {
        printf("tableau[%d] envoyé au rank %d = %d\n", i, rank, reception[i]);
    }
#endif
    /* Check each pattern one by one */
    for (i = 0; i < sendcounts[rank]; i++) {
        int size_pattern = strlen(pattern[reception[i]]);
        int *column;

        /* Initialize the number of matches to 0 */
        n_matches[i] = 0;

        column = (int *)malloc((size_pattern + 1) * sizeof(int));
        if (column == NULL) {
            fprintf(stderr,
                    "Error: unable to allocate memory for column (%ldB)\n",
                    (size_pattern + 1) * sizeof(int));
            return 1;
        }

        /* Traverse the input data up to the end of the file */
        for (j = 0; j < n_bytes; j++) {
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

            distance = levenshtein(pattern[reception[i]], &buf[j], size, column);

            if (distance <= approx_factor) {
                n_matches[i]++;
            }
        }

        free(column);
    }

    /* Timer stop */
    gettimeofday(&t2, NULL);

    duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);

    printf("APM done in %lf s on device %d\n", duration, rank);

    /*****
     * END MAIN LOOP
     ******/

    for (i = 0; i < sendcounts[rank]; i++) {
        printf("Number of matches for pattern <%s>: %d\n", pattern[reception[i]],
               n_matches[i]);
    }

    MPI_Finalize();

    return 0;
}

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

#define APM_DEBUG 0

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
    printf("File length: %lld\n", fsize);
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
    int approx_factor = 0;
    int nb_patterns = 0;
    int i, j;
    char *buf;
    struct timeval t1, t2;
    double duration;
    int n_bytes;
    int *n_matches;
    int rank, size;
    int ppp; // pattern per process

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

    printf(
        "Approximate Pattern Mathing: "
        "looking for %d pattern(s) in file %s w/ distance of %d\n",
        nb_patterns, filename, approx_factor);

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

    /* Distribution des paterns entre les process */
    if (nb_patterns % size != 0) {
        ppp = (nb_patterns / size) + 1;
    }
    else {
        ppp = nb_patterns / size;
    }

    int * distribution = (int *)malloc(size * ppp * sizeof(int)); // contient les numéros des paterns
    int * reception = (int *)malloc(ppp * sizeof(int));
    int * sendcounts = (int *)malloc(size * sizeof(int)); // contient le nombre de paterns envoyé à chaque process et sert aussi à calculer le décalage et recvcount = sendcounts[rank]
    int * displs = (int *)malloc(size * sizeof(int));

    /* Répartition du nombre de patterns entre les process 
    Peut être amélioré en prenant en compte la taille des paterns */
    i = 0;
    if (nb_patterns % size != 0) {
        while (i < nb_patterns % size ) {
            sendcounts[i] = ppp;
            i++;
        }
        while (i < size) {
            sendcounts[i] = ppp - 1;
            i++;
        }
    }
    else {
        while (i < size) {
            sendcounts[i] = ppp;
            i++;
        }
    }

    if (rank == 0) {
        for (i = 0; i<nb_patterns; i++) {
            distribution[i] = i;
        }
        displs[0] = 0; 
        for (i = 1; i < size; i++) {
            displs[i] = displs [i-1] + sendcounts[i-1];
        }
    }
    MPI_Scatterv(distribution, sendcounts, displs, MPI_INT, reception, sendcounts[rank], MPI_INT, 0, MPI_COMM_WORLD);
#if APM_DEBUG
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

    printf("APM done in %lf s\n", duration);

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

/**
 * APPROXIMATE PATTERN MATCHING
 *
 * INF560
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <mpi.h>
#include "aux.h"
#include "apm_cuda.h"
#include <assert.h>

#define APM_DEBUG 0

char *read_input_file(char *filename, int *size)
{
    char *buf;
    off_t fsize;
    int fd = 0;
    int n_bytes = 1;

    /* Open the text file */
    fd = open(filename, O_RDONLY);
    if (fd == -1)
    {
        fprintf(stderr, "Unable to open the text file <%s>\n", filename);
        return NULL;
    }

    /* Get the number of characters in the textfile */
    fsize = lseek(fd, 0, SEEK_END);
    if (fsize == -1)
    {
        fprintf(stderr, "Unable to lseek to the end\n");
        return NULL;
    }

#if APM_DEBUG
    printf("File length: %ld\n", fsize);
#endif

    /* Go back to the beginning of the input file */
    if (lseek(fd, 0, SEEK_SET) == -1)
    {
        fprintf(stderr, "Unable to lseek to start\n");
        return NULL;
    }

    /* Allocate data to copy the target text */
    buf = (char *)malloc(fsize * sizeof(char));
    if (buf == NULL)
    {
        fprintf(stderr, "Unable to allocate %ld byte(s) for main array\n",
                fsize);
        return NULL;
    }

    n_bytes = read(fd, buf, fsize);
    if (n_bytes != fsize)
    {
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

int levenshtein(char *s1, char *s2, int len, int *column)
{
    unsigned int x, y, lastdiag, olddiag;

    for (y = 1; y <= len; y++)
    {
        column[y] = y;
    }
    for (x = 1; x <= len; x++)
    {
        column[0] = x;
        lastdiag = x - 1;
        for (y = 1; y <= len; y++)
        {
            olddiag = column[y];
            column[y] = MIN3(column[y] + 1, column[y - 1] + 1,
                             lastdiag + (s1[y - 1] == s2[x - 1] ? 0 : 1));
            lastdiag = olddiag;
        }
    }
    return (column[len]);
}

/*
Cas quand il n'y a qu'un seul ordinateur donc qu'on n'utilise pas de MPI
*/
int case0(char **argv, char **pattern, char *buf, int n_bytes, int nb_patterns, int approx_factor, int *n_matches, int size, int rank)
{
    int i;
    struct timeval t1, t2;

    /* Grab the patterns */
    for (i = 0; i < nb_patterns; i++)
    {
        int l;

        l = strlen(argv[i + 3]);
        if (l <= 0)
        {
            fprintf(stderr, "Error while parsing argument %d\n", i + 3);
            return 1;
        }

        pattern[i] = (char *)malloc((l + 1) * sizeof(char));
        if (pattern[i] == NULL)
        {
            fprintf(stderr, "Unable to allocate string of size %d\n", l);
            return 1;
        }

        strncpy(pattern[i], argv[i + 3], (l + 1));
    }

    /*****
     * BEGIN MAIN LOOP
     ******/

    /* Timer start */
    gettimeofday(&t1, NULL);

    /* Check each pattern one by one */

    if (nb_patterns == 1)
    {
        printf(" 1 pattern\n");
        cas1_OpenMP(nb_patterns, pattern, n_bytes, approx_factor, buf, n_matches);
    }
    else
    {
        printf(" plusieurs patterns\n");
        cas2_OpenMP(nb_patterns, pattern, n_bytes, approx_factor, buf, n_matches);
    }

    /* Timer stop */
    gettimeofday(&t2, NULL);

    double duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);

    printf("APM done in %lf s\n", duration);

    /*****
     * END MAIN LOOP
     ******/

    for (i = 0; i < nb_patterns; i++)
    {
        printf("Number of matches for pattern <%s>: %d\n", pattern[i],
               n_matches[i]);
    }

    return 0;
}

/*
Cas quand on a plusieurs ordinateur et un long texte par rapport au nombre de patern (en gros très peu de paterns)
*/
int case1(char **argv, char **pattern, char *buf, int n_bytes, int nb_patterns, int approx_factor, int *n_matches, int size, int rank)
{
    int *distribution = (int *)malloc(n_bytes * sizeof(int)); // contient les numéros des paterns
    int *sendcounts = (int *)malloc(size * sizeof(int));      // contient le nombre de paterns envoyé à chaque process et sert aussi à calculer le décalage et recvcount = sendcounts[rank]
    int *displs = (int *)malloc(size * sizeof(int));

    int i;
    struct timeval t1, t2;

    /* Grab the patterns */
    for (i = 0; i < nb_patterns; i++)
    {
        int l;

        l = strlen(argv[i + 3]);
        if (l <= 0)
        {
            fprintf(stderr, "Error while parsing argument %d\n", i + 3);
            return 1;
        }

        pattern[i] = (char *)malloc((l + 1) * sizeof(char));
        if (pattern[i] == NULL)
        {
            fprintf(stderr, "Unable to allocate string of size %d\n", l);
            return 1;
        }

        strncpy(pattern[i], argv[i + 3], (l + 1));
    }

    /*****
     * BEGIN MAIN LOOP
     ******/

    /* Timer start */
    gettimeofday(&t1, NULL);

    for (i = 0; i < n_bytes; i++)
    {
        distribution[i] = i;
    }

    for (i = 0; i < n_bytes % size; i++)
    {
        sendcounts[i] = n_bytes / size + 1;
        displs[i] = i * (n_bytes / size + 1);
    }
    for (i = n_bytes % size; i < size; i++)
    {
        sendcounts[i] = n_bytes / size;
        displs[i] = i * (n_bytes / size) + n_bytes % size;
    }

    int *reception = (int *)malloc((n_bytes / size + 1) * sizeof(int));

    MPI_Scatterv(distribution, sendcounts, displs, MPI_INT, reception, sendcounts[rank], MPI_INT, 0, MPI_COMM_WORLD);

    // for every pattern in the pattern array look for matches in starting at position reception[i]
    for (i = 0; i < nb_patterns; i++)
    {
        int size_pattern = strlen(pattern[i]);
        int *column;

        /* Initialize the number of matches to 0 */
        n_matches[i] = 0;

/* Traverse the input data up to the end of the file */
#pragma omp parallel for private(column) reduction(+ : n_matches[i])
        for (int j = 0; j < sendcounts[rank]; j++)
        {
            column = (int *)malloc((size_pattern + 1) * sizeof(int));
            int distance = 0;
            int size;

            size = size_pattern;
            if (n_bytes - j < size_pattern)
            {
                size = n_bytes - j;
            }

            distance = levenshtein(pattern[i], &buf[reception[j]], size, column);

            if (distance <= approx_factor)
            {
                n_matches[i]++;
            }
            free(column);
        }
    }

    int *total_matches = (int *)malloc(nb_patterns * sizeof(int));

    // Perform the MPI_Allreduce operation to sum up all n_matches across all ranks
    MPI_Reduce(n_matches, total_matches, nb_patterns, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    /* Timer stop */
    gettimeofday(&t2, NULL);

    /*****
     * END MAIN LOOP
     ******/

    if (rank == 0)
    {
        double duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
        printf("APM done in %lf s\n", duration);
        for (i = 0; i < nb_patterns; i++)
        {
            printf("Number of matches for pattern <%s>: %d\n", pattern[i],
                   total_matches[i]);
        }
    }
    return 0;
}

/*
Cas où il y a plusieurs ordinateurs et plusieurs paterns
*/
int case2(char **argv, char **pattern, char *buf, int n_bytes, int nb_patterns, int approx_factor, int *n_matches, int size, int rank)
{

    int *distribution = (int *)malloc(nb_patterns * sizeof(int)); // contient les numéros des paterns
    int *sendcounts = (int *)malloc(size * sizeof(int));          // contient le nombre de paterns envoyé à chaque process et sert aussi à calculer le décalage et recvcount = sendcounts[rank]
    int *displs = (int *)malloc(size * sizeof(int));

    int i, j;
    struct timeval t1, t2;
    int *pattern_len_squared; // tableau contenant la taille au carré de chaque patern
    int sum_len_squared = 0;  // somme des tailles au carré des paterns
    pattern_len_squared = (int *)malloc(nb_patterns * sizeof(int));

    /* Grab the patterns */
    for (i = 0; i < nb_patterns; i++)
    {
        int l;

        l = strlen(argv[i + 3]);
        if (l <= 0)
        {
            fprintf(stderr, "Error while parsing argument %d\n", i + 3);
            return 1;
        }

        // Remplissage du tableau pattern_size_squared et calcul de la somme des tailles au carré des paterns
        pattern_len_squared[i] = l * l;
        sum_len_squared += pattern_len_squared[i];

        pattern[i] = (char *)malloc((l + 1) * sizeof(char));
        if (pattern[i] == NULL)
        {
            fprintf(stderr, "Unable to allocate string of size %d\n", l);
            return 1;
        }

        strncpy(pattern[i], argv[i + 3], (l + 1));
    }

    int sls_by_process = sum_len_squared / size;

    /*****
     * BEGIN MAIN LOOP
     ******/

    /* Timer start */
    gettimeofday(&t1, NULL);

    /* Répartition des patterns entre les process en prenant en compte leur taille */

    int count_squared_len = 0;
    displs[0] = 0;
    j = 0; // j est le numéro du process

    int max_receive = 0;

    for (i = 0; i < nb_patterns; i++)
    {
        distribution[i] = i;
        assert(j < size), "N'a pas réparti tous les patterns entre les process\n";
        count_squared_len += pattern_len_squared[i];

        if (count_squared_len >= sls_by_process * (j + 1) || i == nb_patterns - 1)
        {
            sendcounts[j] = i + 1 - displs[j];
            if (sendcounts[j] > max_receive)
            {
                max_receive = sendcounts[j];
            }
            if (j < size - 1)
            {
                displs[j + 1] = i + 1;
            }
            j++;
        }

        while (count_squared_len >= sls_by_process * (j + 1) && j < size)
        {
            sendcounts[j] = 0;
            if (j < size - 1)
            {
                displs[j + 1] = i + 1;
            }
            j++;
        }
    }

    int *reception = (int *)malloc(max_receive * sizeof(int));

    MPI_Scatterv(distribution, sendcounts, displs, MPI_INT, reception, sendcounts[rank], MPI_INT, 0, MPI_COMM_WORLD);
    /* Check each pattern one by one */
    for (i = 0; i < sendcounts[rank]; i++)
    {
        int size_pattern = strlen(pattern[reception[i]]);
        int *column;

        /* Initialize the number of matches to 0 */
        n_matches[i] = 0;

        column = (int *)malloc((size_pattern + 1) * sizeof(int));
        if (column == NULL)
        {
            fprintf(stderr,
                    "Error: unable to allocate memory for column (%ldB)\n",
                    (size_pattern + 1) * sizeof(int));
            return 1;
        }

        /* Traverse the input data up to the end of the file */
        cas1_OpenMP_aux(0, n_bytes, pattern[reception[i]], n_bytes, approx_factor, buf, n_matches, reception[i]);

        free(column);
    }

    /* Timer stop */
    gettimeofday(&t2, NULL);

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == 0)
    {
        double duration = (t2.tv_sec - t1.tv_sec) + ((t2.tv_usec - t1.tv_usec) / 1e6);
        printf("APM done in %lf s\n", duration);
    }

    /*****
     * END MAIN LOOP
     ******/

    for (i = 0; i < sendcounts[rank]; i++)
    {
        printf("Rank number %d : Number of matches for pattern <%s>: %d\n", rank, pattern[reception[i]],
               n_matches[reception[i]]);
    }

    return 0;
}

int main(int argc, char **argv)
{
    char **pattern;
    char *filename;
    int approx_factor = 0;
    int nb_patterns = 0;
    char *buf;
    int n_bytes;
    int *n_matches;
    int rank, size;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    /* Check number of arguments */
    if (argc < 4)
    {
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

    if (rank == 0)
    {
        printf(
            "Approximate Pattern Matching: "
            "looking for %d pattern(s) in file %s w/ distance of %d\n",
            nb_patterns, filename, approx_factor);
    }

    buf = read_input_file(filename, &n_bytes);
    if (buf == NULL)
    {
        return 1;
    }

    /* Allocate the array of matches */
    n_matches = (int *)malloc(nb_patterns * sizeof(int));
    if (n_matches == NULL)
    {
        fprintf(stderr, "Error: unable to allocate memory for %ldB\n",
                nb_patterns * sizeof(int));
        return 1;
    }

    /* Fill the pattern array */
    pattern = (char **)malloc(nb_patterns * sizeof(char *));
    if (pattern == NULL)
    {
        fprintf(stderr, "Unable to allocate array of pattern of size %d\n",
                nb_patterns);
        return 1;
    }

    if (size == 1)
    {
        if (rank == 0)
        {
            printf("Cas 0\n");
        }
        case0(argv, pattern, buf, n_bytes, nb_patterns, approx_factor, n_matches, size, rank);
    }
    else if (size < nb_patterns && n_bytes < 1000000)
    {
        if (rank == 0)
        {
            printf("Cas 2\n");
        }
        case2(argv, pattern, buf, n_bytes, nb_patterns, approx_factor, n_matches, size, rank);
    }
    else
    {
        if (rank == 0)
        {
            printf("Cas 1\n");
        }
        case1(argv, pattern, buf, n_bytes, nb_patterns, approx_factor, n_matches, size, rank);
    }

    MPI_Finalize();

    return 0;
}

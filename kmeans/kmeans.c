/* =============================================================================
 *
 * kmeans.c
 *
 * =============================================================================
 *
 * Description:
 *
 * Takes as input a file:
 *   ascii  file: containing 1 data point per line
 *   binary file: first int is the number of objects
 *                2nd int is the no. of features of each object
 *
 * This example performs a fuzzy c-means clustering on the data. Fuzzy clustering
 * is performed using min to max clusters and the clustering that gets the best
 * score according to a compactness and separation criterion are returned.
 *
 *
 * Author:
 *
 * Wei-keng Liao
 * ECE Department Northwestern University
 * email: wkliao@ece.northwestern.edu
 *
 *
 * Edited by:
 *
 * Jay Pisharath
 * Northwestern University
 *
 * Chi Cao Minh
 * Stanford University
 *
 * =============================================================================
 *
 * For the license of bayes/sort.h and bayes/sort.c, please see the header
 * of the files.
 *
 * ------------------------------------------------------------------------
 *
 * For the license of kmeans, please see kmeans/LICENSE.kmeans
 *
 * ------------------------------------------------------------------------
 *
 * For the license of ssca2, please see ssca2/COPYRIGHT
 *
 * ------------------------------------------------------------------------
 *
 * For the license of lib/mt19937ar.c and lib/mt19937ar.h, please see the
 * header of the files.
 *
 * ------------------------------------------------------------------------
 *
 * For the license of lib/rbtree.h and lib/rbtree.c, please see
 * lib/LEGALNOTICE.rbtree and lib/LICENSE.rbtree
 *
 * ------------------------------------------------------------------------
 *
 * Unless otherwise noted, the following license applies to STAMP files:
 *
 * Copyright (c) 2007, Stanford University
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *
 *     * Neither the name of Stanford University nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 * =============================================================================
 */


#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "cluster.h"
#include "common.h"
#include "thread.h"
#include "tm.h"
#include "util.h"

#define MAX_LINE_LENGTH 1000000 /* max input is 400000 one digit input + spaces */

extern double global_time;

HTM_STATS(global_tsx_status);

/* =============================================================================
 * usage
 * =============================================================================
 */
void
usage (char* argv0)
{
    char* help =
        "Usage: %s [switches] -i filename\n"
        "       -i filename:     file containing data to be clustered\n"
        "       -b               input file is in binary format\n"
        "       -m max_clusters: maximum number of clusters allowed\n"
        "       -n min_clusters: minimum number of clusters allowed\n"
        "       -z             : don't zscore transform data\n"
        "       -t threshold   : threshold value\n"
        "       -p nproc       : number of threads\n";
    fprintf(stderr, help, argv0);
    exit(-1);
}


/* =============================================================================
 * main
 * =============================================================================
 */
MAIN(argc, argv)
{
    int     max_nclusters = 13;
    int     min_nclusters = 4;
    char*   filename = 0;
    float*  buf;
    float** attributes;
    float** cluster_centres = NULL;
    int     i;
    int     j;
    int     best_nclusters;
    int*    cluster_assign;
    int     numAttributes;
    int     numObjects;
    int     use_zscore_transform = 1;
    char*   line;
    int     isBinaryFile = 0;
    int     nloops;
    int     nthreads;
    float   threshold = 0.001;
    int     opt;

    GOTO_REAL();

    line = (char*)malloc(MAX_LINE_LENGTH); /* reserve memory line */
    assert(line);

    nthreads = 1;
    while ((opt = getopt(argc,(char**)argv,"p:i:m:n:t:bz")) != EOF) {
        switch (opt) {
            case 'i': filename = optarg;
                      break;
            case 'b': isBinaryFile = 1;
                      break;
            case 't': threshold = atof(optarg);
                      break;
            case 'm': max_nclusters = atoi(optarg);
                      break;
            case 'n': min_nclusters = atoi(optarg);
                      break;
            case 'z': use_zscore_transform = 0;
                      break;
            case 'p': nthreads = atoi(optarg);
                      break;
            case '?': usage((char*)argv[0]);
                      break;
            default: usage((char*)argv[0]);
                      break;
        }
    }

    if (filename == 0) {
        usage((char*)argv[0]);
    }

    if (max_nclusters < min_nclusters) {
        fprintf(stderr, "Error: max_clusters must be >= min_clusters\n");
        usage((char*)argv[0]);
    }

    SIM_GET_NUM_CPU(nthreads);

    numAttributes = 0;
    numObjects = 0;

    /*
     * From the input file, get the numAttributes and numObjects
     */
    if (isBinaryFile) {
        ssize_t ret;
        int infile;
        if ((infile = open(filename, O_RDONLY, "0600")) == -1) {
            fprintf(stderr, "Error: no such file (%s)\n", filename);
            exit(1);
        }
        ret = read(infile, &numObjects, sizeof(int));
        assert(ret != -1);
        ret = read(infile, &numAttributes, sizeof(int));
        assert(ret != -1);

        /* Allocate space for attributes[] and read attributes of all objects */
        buf = (float*)malloc(numObjects * numAttributes * sizeof(float));
        assert(buf);
        attributes = (float**)malloc(numObjects * sizeof(float*));
        assert(attributes);
        attributes[0] = (float*)malloc(numObjects * numAttributes * sizeof(float));
        assert(attributes[0]);
        for (i = 1; i < numObjects; i++) {
            attributes[i] = attributes[i-1] + numAttributes;
        }
        ret = read(infile, buf, (numObjects * numAttributes * sizeof(float)));
        assert(ret != -1);
        close(infile);
    } else {
        FILE *infile;
        if ((infile = fopen(filename, "r")) == NULL) {
            fprintf(stderr, "Error: no such file (%s)\n", filename);
            exit(1);
        }
        while (fgets(line, MAX_LINE_LENGTH, infile) != NULL) {
            if (strtok(line, " \t\n") != 0) {
                numObjects++;
            }
        }
        rewind(infile);
        while (fgets(line, MAX_LINE_LENGTH, infile) != NULL) {
            if (strtok(line, " \t\n") != 0) {
                /* Ignore the id (first attribute): numAttributes = 1; */
                while (strtok(NULL, " ,\t\n") != NULL) {
                    numAttributes++;
                }
                break;
            }
        }

        /* Allocate space for attributes[] and read attributes of all objects */
        buf = (float*)malloc(numObjects * numAttributes * sizeof(float));
        assert(buf);
        attributes = (float**)malloc(numObjects * sizeof(float*));
        assert(attributes);
        attributes[0] = (float*)malloc(numObjects * numAttributes * sizeof(float));
        assert(attributes[0]);
        for (i = 1; i < numObjects; i++) {
            attributes[i] = attributes[i-1] + numAttributes;
        }
        rewind(infile);
        i = 0;
        while (fgets(line, MAX_LINE_LENGTH, infile) != NULL) {
            if (strtok(line, " \t\n") == NULL) {
                continue;
            }
            for (j = 0; j < numAttributes; j++) {
                buf[i] = atof(strtok(NULL, " ,\t\n"));
                i++;
            }
        }
        fclose(infile);
    }

    free(line);

    TM_STARTUP(nthreads);
    thread_startup(nthreads);

    /*
     * The core of the clustering
     */
    cluster_assign = (int*)malloc(numObjects * sizeof(int));
    assert(cluster_assign);

    nloops = 1;

    for (i = 0; i < nloops; i++) {
        /*
         * Since zscore transform may perform in cluster() which modifies the
         * contents of attributes[][], we need to re-store the originals
         */
        memcpy(attributes[0], buf, (numObjects * numAttributes * sizeof(float)));

        cluster_centres = NULL;
        cluster_exec(nthreads,
                     numObjects,
                     numAttributes,
                     attributes,           /* [numObjects][numAttributes] */
                     use_zscore_transform, /* 0 or 1 */
                     min_nclusters,        /* pre-define range from min to max */
                     max_nclusters,
                     threshold,
                     &best_nclusters,      /* return: number between min and max */
                     &cluster_centres,     /* return: [best_nclusters][numAttributes] */
                     cluster_assign);      /* return: [numObjects] cluster id for each object */

    }

#ifdef GNUPLOT_OUTPUT
    {
        FILE** fptr;
        char outFileName[1024];
        fptr = (FILE**)malloc(best_nclusters * sizeof(FILE*));
        for (i = 0; i < best_nclusters; i++) {
            sprintf(outFileName, "group.%d", i);
            fptr[i] = fopen(outFileName, "w");
        }
        for (i = 0; i < numObjects; i++) {
            fprintf(fptr[cluster_assign[i]],
                    "%6.4f %6.4f\n",
                    attributes[i][0],
                    attributes[i][1]);
        }
        for (i = 0; i < best_nclusters; i++) {
            fclose(fptr[i]);
        }
        free(fptr);
    }
#endif /* GNUPLOT_OUTPUT */

#ifdef OUTPUT_TO_FILE
    {
        /* Output: the coordinates of the cluster centres */
        FILE* cluster_centre_file;
        FILE* clustering_file;
        char outFileName[1024];

        sprintf(outFileName, "%s-m%dn%d.cluster_centres", filename, max_nclusters, min_nclusters);
        cluster_centre_file = fopen(outFileName, "w");
        for (i = 0; i < best_nclusters; i++) {
            fprintf(cluster_centre_file, "%d ", i);
            for (j = 0; j < numAttributes; j++) {
                fprintf(cluster_centre_file, "%f ", cluster_centres[i][j]);
            }
            fprintf(cluster_centre_file, "\n");
        }
        fclose(cluster_centre_file);

        /* Output: the closest cluster centre to each of the data points */
        sprintf(outFileName, "%s-m%dn%d.cluster_assign", filename, max_nclusters, min_nclusters);
        clustering_file = fopen(outFileName, "w");
        for (i = 0; i < numObjects; i++) {
            fprintf(clustering_file, "%d %d\n", i, cluster_assign[i]);
        }
        fclose(clustering_file);
    }
#endif /* OUTPUT TO_FILE */

#ifdef OUTPUT_TO_STDOUT
    {
        /* Output: the coordinates of the cluster centres */
        for (i = 0; i < best_nclusters; i++) {
            printf("%d ", i);
            for (j = 0; j < numAttributes; j++) {
                printf("%f ", cluster_centres[i][j]);
            }
            printf("\n");
        }
    }
#endif /* OUTPUT TO_STDOUT */

#ifdef OUTPUT_VERIFY
    #include <libgen.h>

    bool_t success = TRUE;
    {
        char outFileName[1024];
        FILE *cluster_centre_result;
        char dname[1024], *dir;
        strncpy(dname, filename, sizeof(dname));
        dname[sizeof(dname) - 1] = '\0';
        dir = dirname(dname);
        char bname[1024], *base;
        strncpy(bname, filename, sizeof(bname));
        bname[sizeof(bname) - 1] = '\0';
        base = basename(bname);

        char *line = NULL;
        size_t sz = 0;

        /* Check results */
        sprintf(outFileName, "%s/../outputs/%s-m%dn%d.cluster_centres", dir, base, max_nclusters, min_nclusters);
        if ((cluster_centre_result = fopen(outFileName, "r"))) {
            for (int i = 0; i < best_nclusters; i++) {
                ssize_t lz = getline(&line, &sz, cluster_centre_result);
                if (lz <= 0)
                    success = FALSE;
                assert(lz);

                char *tok = strtok(line, " ");
                int ri = tok ? atoi(tok) : 0;
                success = success ? tok && ri == i : success;
                assert(success);
                for (int j = 0; j < numAttributes; j++) {
                    tok = strtok(NULL, " ");
                    float rj = tok ? atof(tok) : 0.0;
                    success = success ? tok && fabsf(rj - cluster_centres[i][j]) <= (max_nclusters + min_nclusters) * 0.5 * 10 * threshold : success;
# ifdef TM_DEBUG
                    if (!success)
                        printf("actual: %f, computed: %f, threshold: %f\n", cluster_centres[i][j], rj, (max_nclusters + min_nclusters) * 0.5 * threshold);
# endif
                    assert(success);
                }
            }
            fclose(cluster_centre_result);
        }

        if (line)
            free(line);
    }
    assert(success);
#endif /* OUTPUT_VERIFY */

    printf("Cluster time = %f\n", global_time);

    HTM_STATS_PRINT(global_tsx_status);

    free(cluster_assign);
    free(attributes[0]);
    free(attributes);
    free(cluster_centres[0]);
    free(cluster_centres);
    free(buf);

    TM_SHUTDOWN();

    GOTO_SIM();

    thread_shutdown();

    MAIN_RETURN(!success);
}


/* =============================================================================
 *
 * End of kmeans.c
 *
 * =============================================================================
 */

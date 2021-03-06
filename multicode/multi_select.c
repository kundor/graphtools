/*
 * Main developer: Nico Van Cleemput
 * 
 * Copyright (C) 2013 Nico Van Cleemput.
 * Licensed under the GNU GPL, read the file LICENSE.txt for details.
 */

/* This program reads graphs in multicode format from standard in and
 * writes those that were selected to standard out in multicode format.
 * 
 * Compile with:
 *     
 *     cc -o multi_select -O4  multi_select.c \
 *     shared/multicode_base.c shared/multicode_input.c shared/multicode_output.c
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "shared/multicode_base.h"
#include "shared/multicode_input.h"
#include "shared/multicode_output.h"

//====================== USAGE =======================

void help(char *name) {
    fprintf(stderr, "The program %s filters out the selected graphs.\n\n", name);
    fprintf(stderr, "Usage\n=====\n");
    fprintf(stderr, " %s [options]\n\n", name);
    fprintf(stderr, "\nThis program can handle graphs up to %d vertices. Recompile if you need larger\n", MAXN);
    fprintf(stderr, "graphs.\n\n");
    fprintf(stderr, "Valid options\n=============\n");
    fprintf(stderr, "    -m, --modulo r:m\n");
    fprintf(stderr, "       Split the input into m parts and only output part r (0<=r<m).\n");
    fprintf(stderr, "    -h, --help\n");
    fprintf(stderr, "       Print this help and return.\n");
}

void usage(char *name) {
    fprintf(stderr, "Usage: %s [options] g1 g2\n", name);
    fprintf(stderr, "       %s -m r:m\n", name);
    fprintf(stderr, "For more information type: %s -h \n\n", name);
}

/*
 * 
 */
int main(int argc, char** argv) {
    
    GRAPH graph;
    ADJACENCY adj;
    
    unsigned long long int graphsRead = 0;
    int graphsFiltered = 0;
    
    boolean moduloEnabled = FALSE;
    int moduloRest;
    int moduloMod;

    /*=========== commandline parsing ===========*/

    int c;
    char *name = argv[0];
    static struct option long_options[] = {
        {"modulo", required_argument, NULL, 'm'},
        {"help", no_argument, NULL, 'h'}
    };
    int option_index = 0;

    while ((c = getopt_long(argc, argv, "hm:", long_options, &option_index)) != -1) {
        switch (c) {
            case 'm':
                moduloEnabled = TRUE;
                if(sscanf(optarg, "%d:%d", &moduloRest, &moduloMod)!=2){
                    fprintf(stderr, "Error while reading modulo -- exiting.\n");
                    usage(name);
                    return EXIT_FAILURE;
                }
                break;
            case 'h':
                help(name);
                return EXIT_SUCCESS;
            case '?':
                usage(name);
                return EXIT_FAILURE;
            default:
                fprintf(stderr, "Illegal option %c.\n", c);
                usage(name);
                return EXIT_FAILURE;
        }
    }
        
    if (argc - optind == 0 && !moduloEnabled) {
        usage(name);
        return EXIT_FAILURE;
    } else if(argc - optind > 0 && moduloEnabled){
        usage(name);
        return EXIT_FAILURE;
    }
    
    int i;
    int selectedGraphs[argc - optind];
    for (i = 0; i < argc - optind; i++){
        selectedGraphs[i] = atoi(argv[i + optind]);
    }
    

    
    unsigned short code[MAXCODELENGTH];
    int length;
    while (readMultiCode(code, &length, stdin)) {
        graphsRead++;
        
        if(moduloEnabled){
            if(graphsRead % moduloMod == moduloRest){
                decodeMultiCode(code, length, graph, adj);
                graphsFiltered++;
                writeMultiCode(graph, adj, stdout);
            }
        } else if (graphsFiltered < argc - optind && (graphsRead == selectedGraphs[graphsFiltered])) {
            decodeMultiCode(code, length, graph, adj);
            graphsFiltered++;
            writeMultiCode(graph, adj, stdout);
        }
    }
    
    fprintf(stderr, "Read %llu graph%s.\n", graphsRead, graphsRead==1 ? "" : "s");
    fprintf(stderr, "Filtered %d graph%s.\n", graphsFiltered, graphsFiltered==1 ? "" : "s");

    return (EXIT_SUCCESS);
}


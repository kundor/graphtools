/*
 * Main developer: Nico Van Cleemput
 * 
 * Copyright (C) 2013 Nico Van Cleemput.
 * Licensed under the GNU GPL, read the file LICENSE.txt for details.
 */

/* This program reads graphs in multicode format from standard in and
 * writes the corona of those graphs to standard out in multicode format.   
 * The corona of a graph is constructed by adding a vertex for each vertex
 * and connecting these two vertices.
 * 
 * Compile with:
 *     
 *     cc -o multi_corona -O4  multi_corona.c shared/multicode_base.c \
 *     shared/multicode_input.c shared/multicode_output.c
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "shared/multicode_base.h"
#include "shared/multicode_input.h"
#include "shared/multicode_output.h"


void constructCorona(GRAPH graph, ADJACENCY adj, GRAPH coronaGraph, ADJACENCY coronaAdj){
    int i, j;
    
    int order = graph[0][0];
    
    prepareGraph(coronaGraph, coronaAdj, order*2);
    
    for(i=1; i<=order; i++){
        for(j=0; j<adj[i]; j++){
            if(i < graph[i][j]){
                addEdge(coronaGraph, coronaAdj, i, graph[i][j]);
            }
        }
        addEdge(coronaGraph, coronaAdj, i, i + order);
    }
}

//====================== USAGE =======================

void help(char *name) {
    fprintf(stderr, "The program %s constructs the corona of graphs in multicode format.\n\n", name);
    fprintf(stderr, "Usage\n=====\n");
    fprintf(stderr, " %s [options]\n\n", name);
    fprintf(stderr, "\nThis program can handle graphs up to %d vertices. Recompile if you need larger\n", MAXN);
    fprintf(stderr, "graphs.\n\n");
    fprintf(stderr, "Valid options\n=============\n");
    fprintf(stderr, "    -h, --help\n");
    fprintf(stderr, "       Print this help and return.\n");
}

void usage(char *name) {
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "For more information type: %s -h \n\n", name);
}

/*
 * 
 */
int main(int argc, char** argv) {
    int i;
    
    GRAPH graph;
    ADJACENCY adj;
    GRAPH coronaGraph;
    ADJACENCY coronaAdj;

    /*=========== commandline parsing ===========*/

    int c;
    char *name = argv[0];
    static struct option long_options[] = {
        {"help", no_argument, NULL, 'h'}
    };
    int option_index = 0;

    while ((c = getopt_long(argc, argv, "h", long_options, &option_index)) != -1) {
        switch (c) {
            case 0:
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
    
    int connectionCount = argc - optind;
    
    int connections[connectionCount][2];
    
    for (i = 0; i < connectionCount; i++){
        if(sscanf(argv[optind + i], "%d,%d", connections[i], connections[i]+1)!=2){
            fprintf(stderr, "Error while reading edges to be added.\n", c);
            usage(name);
            return EXIT_FAILURE;
        }
    }
    
    unsigned short code[MAXCODELENGTH];
    int length;
    while (readMultiCode(code, &length, stdin)) {
        decodeMultiCode(code, length, graph, adj);
        
        constructCorona(graph, adj, coronaGraph, coronaAdj);
        
        writeMultiCode(coronaGraph, coronaAdj, stdout);
    }

    return (EXIT_SUCCESS);
}


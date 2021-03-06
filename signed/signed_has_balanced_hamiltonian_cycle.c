/*
 * Main developer: Nico Van Cleemput
 * 
 * Copyright (C) 2014 Nico Van Cleemput.
 * Licensed under the GNU GPL, read the file LICENSE.txt for details.
 */

/* This program reads graphs in signed_code format and computes whether
 * they have a balanced hamiltonian cycle.
 * 
 * 
 * Compile with:
 *     
 *     cc -o signed_has_balanced_hamiltonian_cycle -O4  signed_has_balanced_hamiltonian_cycle.c \
 *           shared/signed_base.c shared/signed_input.c shared/signed_output.c
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include "shared/signed_base.h"
#include "shared/signed_input.h"
#include "shared/signed_output.h"

#define MAX_EDGE_COUNT 64

int edgeCounter = 0;

boolean handleSimpleCycle(int negativeEdgesInCycle){
    if(negativeEdgesInCycle%2==0){
        //cycle is balanced
        return TRUE;
    }
    return FALSE;
}

boolean checkSimpleCycles_impl(GRAPH graph, ADJACENCY adj, int order, int firstVertex, int secondVertex,
        int currentVertex,
        unsigned long long int verticesInCycle, unsigned long long int edgesInCycle, int negativeEdgesInCycle, int length){
    int i;
    for(i=0; i<adj[currentVertex]; i++){
        EDGE *e = graph[currentVertex][i];
        int neighbour = (e->smallest == currentVertex) ? e->largest : e->smallest;
        if((1ULL << e->index) & (edgesInCycle)){
            //edge already in cycle
            continue;
        } else if((neighbour != firstVertex) && ((1ULL << neighbour) & (verticesInCycle))){
            //vertex already in cycle (and not first vertex)
            continue;
        } else if(neighbour < firstVertex){
            //cycle not in canonical form
            continue;
        } else if(neighbour == firstVertex){
            //we have returned to the first vertex
            if(currentVertex < secondVertex){
                //cycle not in canonical form
                continue;
            }
            if(length < order){
                //if it is not a hamiltonian cycle
                continue;
            }
            if(e->isNegative){
                if(handleSimpleCycle(negativeEdgesInCycle + 1)){
                    return TRUE;
                }
            } else {
                if(handleSimpleCycle(negativeEdgesInCycle)){
                    return TRUE;
                }
            }
        } else {
            //we continue the cycle
            if(e->isNegative){
                if(checkSimpleCycles_impl(graph, adj, order, firstVertex, secondVertex, neighbour,
                    verticesInCycle | (1ULL<<neighbour), edgesInCycle | (1ULL<<(e->index)),
                        negativeEdgesInCycle+1, length + 1)){
                    return TRUE;
                }
            } else {
                if(checkSimpleCycles_impl(graph, adj, order, firstVertex, secondVertex, neighbour,
                    verticesInCycle | (1ULL<<neighbour), edgesInCycle | (1ULL<<(e->index)),
                        negativeEdgesInCycle, length + 1)){
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

/* Checks the simple cycles and returns TRUE if the graph has a balanced hamiltonian cycle.
 */
boolean hasBalancedHamiltonianCycle(GRAPH graph, ADJACENCY adj, int order){
    int v, i;
    for(i=0; i<edgeCounter; i++){
        edges[i].index = i;
    }
    
    for(v = 1; v < order; v++){ //intentionally skip v==order!
        for(i=0; i<adj[v]; i++){
            EDGE *e = graph[v][i];
            int neighbour = e->largest;
            if(neighbour == v){
                //cycle not in canonical form: we have seen this cycle already
                continue;
            } else {
                //start a cycle
                unsigned long long int verticesInCycle = (1ULL<<v) | (1ULL<<neighbour); //add vertex
                unsigned long long int edgesInCycle = (1ULL<<(e->index)); //add edge
                int negativeEdgesInCycle = (e->isNegative) ? 1 : 0;
                if(checkSimpleCycles_impl(graph, adj, order, v, neighbour, neighbour,
                        verticesInCycle, edgesInCycle, negativeEdgesInCycle, 2)){
                    return TRUE;
                }
            }
        }
    }
    return FALSE;
}

//====================== USAGE =======================

void help(char *name) {
    fprintf(stderr, "The program %s determines whether signed graphs in signed_code\n", name);
    fprintf(stderr, "format have a balanced hamiltonian cycle.\n\n");
    fprintf(stderr, "Usage\n=====\n");
    fprintf(stderr, " %s [options] k\n\n", name);
    fprintf(stderr, "\nThis program can handle graphs up to %d vertices. Recompile if you need larger\n", MAXN);
    fprintf(stderr, "graphs and give a larger value for MAXN.\n\n");
    fprintf(stderr, "Valid options\n=============\n");
    fprintf(stderr, "    -f, --filter\n");
    fprintf(stderr, "       Filter graphs that have a k-flow.\n");
    fprintf(stderr, "    -i, --invert\n");
    fprintf(stderr, "       Invert the filter.\n");
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
    
    GRAPH graph;
    ADJACENCY adj;
    
    boolean doFiltering = FALSE;
    boolean invert = FALSE;

    int graphCount = 0;
    int graphsFiltered = 0;

    /*=========== commandline parsing ===========*/

    int c;
    char *name = argv[0];
    static struct option long_options[] = {
        {"invert", no_argument, NULL, 'i'},
        {"filter", no_argument, NULL, 'f'},
        {"help", no_argument, NULL, 'h'}
    };
    int option_index = 0;

    while ((c = getopt_long(argc, argv, "hfi", long_options, &option_index)) != -1) {
        switch (c) {
            case 'i':
                invert = TRUE;
                break;
            case 'f':
                doFiltering = TRUE;
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
    
    unsigned short code[MAXCODELENGTH];
    int length;
    while (readSignedCode(code, &length, stdin)) {
        int order;
        decodeSignedCode(code, length, graph, adj, &order);
        graphCount++;
        if(edgeCounter > MAX_EDGE_COUNT){
            fprintf(stderr, "Current version only supports graphs with up to %d edges.\n", MAX_EDGE_COUNT);
            fprintf(stderr, "Graph %d has %d edges. Exiting!\n", graphCount, edgeCounter);
            exit(EXIT_FAILURE);
        }
        
        boolean value = hasBalancedHamiltonianCycle(graph, adj, order);
        if(doFiltering){
            if(invert && !value){
                graphsFiltered++;
                writeSignedCode(graph, adj, order, stdout);
            } else if(!invert && value){
                graphsFiltered++;
                writeSignedCode(graph, adj, order, stdout);
            }
        } else {
            if(value){
                fprintf(stdout, "Graph %d has a balanced hamiltonian cycle.\n", graphCount);
            } else {
                fprintf(stdout, "Graph %d does not have a balanced hamiltonian cycle.\n", graphCount);
            }
        }
    }
    
    fprintf(stderr, "Read %d graph%s.\n", graphCount, graphCount==1 ? "" : "s");
    if(doFiltering){
        fprintf(stderr, "Filtered %d graph%s that %s%s balanced hamiltonian cycle.\n", 
                graphsFiltered, graphsFiltered==1 ? "" : "s",
                graphsFiltered==1 ? "has" : "have",
                invert ? " no" : "a");
    }

    return (EXIT_SUCCESS);
}


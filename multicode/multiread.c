/* Usage: cat codes | multiread 
 * or:    cat codes | multiread graphNumber
 */

#include<stdio.h>
#include<stdlib.h>
#include<limits.h>

#define EMPTY USHRT_MAX

#define MAXN 4000
#define MAXVALENCE 100

typedef unsigned short ENTRYTYPE;
typedef ENTRYTYPE GRAPH[MAXN + 1][MAXVALENCE + 1];
typedef ENTRYTYPE ADJACENCY[MAXN + 1];

int maxvalence, codeLength;
ENTRYTYPE vertexCount;

void writeGraph_impl(GRAPH g, int columns, char *numberFormat, char *headerSeparator, char* emptyCell) {
    int x, y, lowerBound, upperBound;
    fprintf(stdout, "\n\n ");

    fprintf(stdout, numberFormat, g[0][0]);

    for (x = 1; (x <= g[0][0])&&(x <= columns); x++) {
        fprintf(stdout, numberFormat, x);
    }
    fprintf(stdout, "|\n");

    fprintf(stdout, " ");

    for (x = 0; (x <= g[0][0])&&(x <= columns); x++) {
        fprintf(stdout, "|%s", headerSeparator);
    }
    fprintf(stdout, "|\n");

    for (x = 0; x < maxvalence; x++) {
        fprintf(stdout, " |%s", emptyCell);
        for (y = 1; (y <= g[0][0])&&(y <= columns); y++) {
            if (g[y][x] == EMPTY) {
                fprintf(stdout, "|%s", emptyCell);
            } else {
                fprintf(stdout, numberFormat, g[y][x]);
            }
        }
        fprintf(stdout, "|\n");
    }

    lowerBound = columns + 1;
    upperBound = 2*columns;

    while (g[0][0] >= lowerBound) {
        fprintf(stdout, "\n");

        fprintf(stdout, "  %s", emptyCell);

        for (x = lowerBound; (x <= g[0][0])&&(x <= upperBound); x++) {
            fprintf(stdout, numberFormat, x);
        }
        fprintf(stdout, "|\n");

        fprintf(stdout, "  %s", emptyCell);

        for (x = lowerBound; (x <= g[0][0])&&(x <= upperBound); x++) {
            fprintf(stdout, "|%s", headerSeparator);
        }
        fprintf(stdout, "|\n");

        for (y = 0; y < maxvalence; y++) {
            fprintf(stdout, "  %s", emptyCell);
            for (x = lowerBound; (x <= g[0][0])&&(x <= upperBound); x++) {
                if (g[x][y] == EMPTY) {
                    fprintf(stdout, "|%s", emptyCell);
                } else {
                    fprintf(stdout, numberFormat, g[x][y]);
                }
            }
            fprintf(stdout, "|\n");
        }
        lowerBound += columns;
        upperBound += columns;
    }
}

void writeGraph2Digits(GRAPH g) {
    writeGraph_impl(g, 24, "|%2d", "==", "  ");
}

void writeGraph3Digits(GRAPH g) {
    writeGraph_impl(g, 18, "|%3d", "===", "   ");
}

void writeGraph4Digits(GRAPH g) {
    writeGraph_impl(g, 14, "|%4d", "====", "    ");
}

void writeGraph(GRAPH g) {
    if (g[0][0] < 100) {
        writeGraph2Digits(g);
    } else if (g[0][0] < 1000) {
        writeGraph3Digits(g);
    } else /*if (g[0][0] < 10000)*/ {
        writeGraph4Digits(g);
    }

}

/* This method adds the edge (v,w) to graph. This assumes that adj contains
 * the current degree of the vertices v and w. This degrees are then updated.
 */
void addEdge(GRAPH graph, ADJACENCY adj, int v, int w) {
    graph[v][adj[v]] = w;
    graph[w][adj[w]] = v;
    adj[v]++;
    adj[w]++;
}

void decode(ENTRYTYPE *code, GRAPH graph, ADJACENCY adj, int codelength) {
    int i, j;
    ENTRYTYPE vertexCount;

    graph[0][0] = vertexCount = code[0];

    for (i = 1; i <= vertexCount; i++) {
        adj[i] = 0;
        for (j = 0; j <= MAXVALENCE; j++) {
            graph[i][j] = EMPTY;
        }
    }
    for (j = 1; j <= MAXVALENCE; j++) {
        graph[0][j] = 0;
    }

    j = 1;

    for (i = 1; i < codelength; i++) {
        if (code[i] == 0) {
            j++;
        } else {
            addEdge(graph, adj, j, (int) code[i]);
        }
        if ((adj[code[i]] > MAXVALENCE) || (adj[j] > MAXVALENCE)) {
            fprintf(stderr, "MAXVALENCE too small (%d)!\n", MAXVALENCE);
            exit(0);
        }
    }
}

main(int argc, char *argv[]) {
    GRAPH graph;
    ADJACENCY adj;
    int graphCount, i, zeroCount;
    ENTRYTYPE code[MAXN * MAXVALENCE + MAXN];
    unsigned char dummy;

    int filterGraph = 0;

    if (argc >= 2) {
        sscanf(argv[1], "%d", &filterGraph);
    }

    graphCount = 0;


    for (; fread(&dummy, sizeof (unsigned char), 1, stdin);) {
        if (dummy != 0) {
            vertexCount = code[0] = dummy;
            zeroCount = 0;
            codeLength = 1;
            while (zeroCount < vertexCount - 1) {
                code[codeLength] = getc(stdin);
                if (code[codeLength] == 0) {
                    zeroCount++;
                }
                codeLength++;
            }
        } else {
            if (fread(code, sizeof (ENTRYTYPE), 1, stdin)!=1) {
                fprintf(stderr, "Error while reading graph -- exiting!\n");
                exit(EXIT_FAILURE);
            }
            vertexCount = code[0];
            zeroCount = 0;
            codeLength = 1;
            while (zeroCount < vertexCount - 1) {
                if (fread(code + codeLength, sizeof (ENTRYTYPE), 1, stdin)!=1) {
                    fprintf(stderr, "Error while reading graph -- exiting!\n");
                    exit(EXIT_FAILURE);
                }
                if (code[codeLength] == 0) {
                    zeroCount++;
                }
                codeLength++;
            }
        }


        graphCount++;
        if ((!filterGraph) || (filterGraph == graphCount)) {
            decode(code, graph, adj, codeLength);
            for (i = 1, maxvalence = 0; i <= vertexCount; i++) {
                if (adj[i] > maxvalence) {
                    maxvalence = adj[i];
                }
            }
            printf("\n\n\n Graph Nr: %d \n\n", graphCount);
            writeGraph(graph);
        }
    }



    return (0);

}



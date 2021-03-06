/*
 * Main developer: Nico Van Cleemput
 * 
 * Copyright (C) 2014 Nico Van Cleemput.
 * Licensed under the GNU GPL, read the file LICENSE.txt for details.
 */

/* This program reads 2 plane graphs from standard in and glues the specified
 * face in the first graph to the specified face in the second graph.
 * 
 * 
 * Compile with:
 *     
 *     cc -o fill_face_pl -O4 fill_face_pl.c
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <time.h>


#define MAXN 400            /* the maximum number of vertices */
#define MAXE (6*MAXN-12)    /* the maximum number of oriented edges */
#define MAXF (2*MAXN-4)      /* the maximum number of faces */
#define MAXVAL (MAXN-1)  /* the maximum degree of a vertex */
#define MAXCODELENGTH (MAXN+MAXE+3)

#define INFI (MAXN + 1)

#define FALSE 0
#define TRUE  1

typedef int boolean;

typedef struct e /* The data type used for edges */ {
    int start; /* vertex where the edge starts */
    int end; /* vertex where the edge ends */
    int rightface; /* face on the right side of the edge
                          note: only valid if make_dual() called */
    struct e *prev; /* previous edge in clockwise direction */
    struct e *next; /* next edge in clockwise direction */
    struct e *inverse; /* the edge that is inverse to this one */
    int mark, index; /* two ints for temporary use;
                          Only access mark via the MARK macros. */

    int left_facesize; /* size of the face in prev-direction of the edge.
        		  Only used for -p option. */
} EDGE;

EDGE *firstedge[MAXN]; /* pointer to arbitrary edge out of vertex i. */
int degree[MAXN];

EDGE edges[MAXE];

EDGE *firstedge2[MAXN]; /* pointer to arbitrary edge out of vertex i. */
int degree2[MAXN];

EDGE edges2[MAXE];

static int markvalue = 30000;
#define RESETMARKS {int mki; if ((markvalue += 2) > 30000) \
       { markvalue = 2; for (mki=0;mki<MAXE;++mki) edges[mki].mark=0;}}
#define MARK(e) (e)->mark = markvalue
#define MARKLO(e) (e)->mark = markvalue
#define MARKHI(e) (e)->mark = markvalue+1
#define UNMARK(e) (e)->mark = markvalue-1
#define ISMARKED(e) ((e)->mark >= markvalue)
#define ISMARKEDLO(e) ((e)->mark == markvalue)
#define ISMARKEDHI(e) ((e)->mark > markvalue)

int nv;
int ne;
int nv2;
int ne2;

FILE *firstFile = NULL;
FILE *secondFile = NULL;

void printGraph2(){
    int i;
    EDGE *e, *elast;
    
    for(i=0; i<nv2; i++){
        fprintf(stderr, "%d) ", i+1);
        e = elast = firstedge2[i];
        do {
            fprintf(stderr, "%d (%p) ", e->end + 1, e);
            e = e->next;
        } while (e != elast);
        fprintf(stderr, "\n");
    }
}

//////////////////////////////////////////////////////////////////////////////

//=============== Writing planarcode of graph ===========================

void writePlanarCodeChar(){
    int i;
    EDGE *e, *elast;
    
    //write the number of vertices
    fputc(nv, stdout);
    
    for(i=0; i<nv; i++){
        e = elast = firstedge[i];
        do {
            fputc(e->end + 1, stdout);
            e = e->next;
        } while (e != elast);
        fputc(0, stdout);
    }
}

void writeShort(unsigned short value){
    if (fwrite(&value, sizeof (unsigned short), 1, stdout) != 1) {
        fprintf(stderr, "fwrite() failed -- exiting!\n");
        exit(-1);
    }
}

void writePlanarCodeShort(){
    int i;
    EDGE *e, *elast;
    
    //write the number of vertices
    fputc(0, stdout);
    writeShort(nv);
    
    
    for(i=0; i<nv; i++){
        e = elast = firstedge[i];
        do {
            writeShort(e->end + 1);
            e = e->next;
        } while (e != elast);
        writeShort(0);
    }
}

void writePlanarCode(){
    static int first = TRUE;
    
    if(first){
        first = FALSE;
        
        fprintf(stdout, ">>planar_code<<");
    }
    
    if (nv + 1 <= 255) {
        writePlanarCodeChar();
    } else if (nv + 1 <= 65535) {
        writePlanarCodeShort();
    } else {
        fprintf(stderr, "Graphs of that size are currently not supported -- exiting!\n");
        exit(-1);
    }
    
}

//=============== Reading and decoding planarcode ===========================

EDGE *findEdge(int from, int to) {
    EDGE *e, *elast;

    e = elast = firstedge[from];
    do {
        if (e->end == to) {
            return e;
        }
        e = e->next;
    } while (e != elast);
    fprintf(stderr, "error while looking for edge from %d to %d.\n", from, to);
    exit(0);
}

void decodePlanarCode(unsigned short* code) {
    /* complexity of method to determine inverse isn't that good, but will have to satisfy for now
     */
    int i, j, codePosition;
    int edgeCounter = 0;
    EDGE *inverse;

    nv = code[0];
    codePosition = 1;

    for (i = 0; i < nv; i++) {
        degree[i] = 0;
        firstedge[i] = edges + edgeCounter;
        edges[edgeCounter].start = i;
        edges[edgeCounter].end = code[codePosition] - 1;
        edges[edgeCounter].next = edges + edgeCounter + 1;
        if (code[codePosition] - 1 < i) {
            inverse = findEdge(code[codePosition] - 1, i);
            edges[edgeCounter].inverse = inverse;
            inverse->inverse = edges + edgeCounter;
        } else {
            edges[edgeCounter].inverse = NULL;
        }
        edgeCounter++;
        codePosition++;
        for (j = 1; code[codePosition]; j++, codePosition++) {
            if (j == MAXVAL) {
                fprintf(stderr, "MAXVAL too small: %d\n", MAXVAL);
                exit(0);
            }
            edges[edgeCounter].start = i;
            edges[edgeCounter].end = code[codePosition] - 1;
            edges[edgeCounter].prev = edges + edgeCounter - 1;
            edges[edgeCounter].next = edges + edgeCounter + 1;
            if (code[codePosition] - 1 < i) {
                inverse = findEdge(code[codePosition] - 1, i);
                edges[edgeCounter].inverse = inverse;
                inverse->inverse = edges + edgeCounter;
            } else {
                edges[edgeCounter].inverse = NULL;
            }
            edgeCounter++;
        }
        firstedge[i]->prev = edges + edgeCounter - 1;
        edges[edgeCounter - 1].next = firstedge[i];
        degree[i] = j;

        codePosition++; /* read the closing 0 */
    }

    ne = edgeCounter;
}

EDGE *findEdge2(int from, int to) {
    EDGE *e, *elast;

    e = elast = firstedge2[from];
    do {
        if (e->end == to) {
            return e;
        }
        e = e->next;
    } while (e != elast);
    fprintf(stderr, "error while looking for edge from %d to %d.\n", from, to);
    exit(0);
}

void decodePlanarCode2(unsigned short* code) {
    /* complexity of method to determine inverse isn't that good, but will have to satisfy for now
     */
    int i, j, codePosition;
    int edgeCounter = 0;
    EDGE *inverse;

    nv2 = code[0];
    codePosition = 1;

    for (i = 0; i < nv2; i++) {
        degree2[i] = 0;
        firstedge2[i] = edges2 + edgeCounter;
        edges2[edgeCounter].start = i;
        edges2[edgeCounter].end = code[codePosition] - 1;
        edges2[edgeCounter].next = edges2 + edgeCounter + 1;
        if (code[codePosition] - 1 < i) {
            inverse = findEdge2(code[codePosition] - 1, i);
            edges2[edgeCounter].inverse = inverse;
            inverse->inverse = edges2 + edgeCounter;
        } else {
            edges2[edgeCounter].inverse = NULL;
        }
        edgeCounter++;
        codePosition++;
        for (j = 1; code[codePosition]; j++, codePosition++) {
            if (j == MAXVAL) {
                fprintf(stderr, "MAXVAL too small: %d\n", MAXVAL);
                exit(0);
            }
            edges2[edgeCounter].start = i;
            edges2[edgeCounter].end = code[codePosition] - 1;
            edges2[edgeCounter].prev = edges2 + edgeCounter - 1;
            edges2[edgeCounter].next = edges2 + edgeCounter + 1;
            if (code[codePosition] - 1 < i) {
                inverse = findEdge2(code[codePosition] - 1, i);
                edges2[edgeCounter].inverse = inverse;
                inverse->inverse = edges2 + edgeCounter;
            } else {
                edges2[edgeCounter].inverse = NULL;
            }
            edgeCounter++;
        }
        firstedge2[i]->prev = edges2 + edgeCounter - 1;
        edges2[edgeCounter - 1].next = firstedge2[i];
        degree2[i] = j;

        codePosition++; /* read the closing 0 */
    }

    ne2 = edgeCounter;
}

/**
 * 
 * @param code
 * @param length
 * @param file
 * @return returns 1 if a code was read and 0 otherwise. Exits in case of error.
 */
int readPlanarCode(unsigned short code[], int *length, FILE *file) {
    static int first = 1;
    unsigned char c;
    char testheader[20];
    int bufferSize, zeroCounter;
    
    int readCount;


    if (first) {
        first = 0;

        if (fread(&testheader, sizeof (unsigned char), 13, file) != 13) {
            fprintf(stderr, "can't read header ((1)file too small)-- exiting\n");
            exit(1);
        }
        testheader[13] = 0;
        if (strcmp(testheader, ">>planar_code") == 0) {

        } else {
            fprintf(stderr, "No planarcode header detected -- exiting!\n");
            exit(1);
        }
        //read reminder of header (either empty or le/be specification)
        if (fread(&c, sizeof (unsigned char), 1, file) == 0) {
            return FALSE;
        }
        while (c!='<'){
            if (fread(&c, sizeof (unsigned char), 1, file) == 0) {
                return FALSE;
            }
        }
        //read one more character
        if (fread(&c, sizeof (unsigned char), 1, file) == 0) {
            return FALSE;
        }
    }

    /* possibly removing interior headers -- only done for planarcode */
    if (fread(&c, sizeof (unsigned char), 1, file) == 0) {
        //nothing left in file
        return (0);
    }

    if (c == '>') {
        // could be a header, or maybe just a 62 (which is also possible for unsigned char
        code[0] = c;
        bufferSize = 1;
        zeroCounter = 0;
        code[1] = (unsigned short) getc(file);
        if (code[1] == 0) zeroCounter++;
        code[2] = (unsigned short) getc(file);
        if (code[2] == 0) zeroCounter++;
        bufferSize = 3;
        // 3 characters were read and stored in buffer
        if ((code[1] == '>') && (code[2] == 'p')) /*we are sure that we're dealing with a header*/ {
            while ((c = getc(file)) != '<');
            /* read 2 more characters: */
            c = getc(file);
            if (c != '<') {
                fprintf(stderr, "Problems with header -- single '<'\n");
                exit(1);
            }
            if (!fread(&c, sizeof (unsigned char), 1, file)) {
                //nothing left in file
                return (0);
            }
            bufferSize = 1;
            zeroCounter = 0;
        }
    } else {
        //no header present
        bufferSize = 1;
        zeroCounter = 0;
    }

    if (c != 0) /* unsigned chars would be sufficient */ {
        code[0] = c;
        if (code[0] > MAXN) {
            fprintf(stderr, "Constant N too small %d > %d \n", code[0], MAXN);
            exit(1);
        }
        while (zeroCounter < code[0]) {
            code[bufferSize] = (unsigned short) getc(file);
            if (code[bufferSize] == 0) zeroCounter++;
            bufferSize++;
        }
    } else {
        readCount = fread(code, sizeof (unsigned short), 1, file);
        if(!readCount){
            fprintf(stderr, "Unexpected EOF.\n");
            exit(1);
        }
        if (code[0] > MAXN) {
            fprintf(stderr, "Constant N too small %d > %d \n", code[0], MAXN);
            exit(1);
        }
        bufferSize = 1;
        zeroCounter = 0;
        while (zeroCounter < code[0]) {
            readCount = fread(code + bufferSize, sizeof (unsigned short), 1, file);
            if(!readCount){
                fprintf(stderr, "Unexpected EOF.\n");
                exit(1);
            }
            if (code[bufferSize] == 0) zeroCounter++;
            bufferSize++;
        }
    }

    *length = bufferSize;
    return (1);


}

//====================== USAGE =======================

void help(char *name) {
    fprintf(stderr, "The program %s subdivides the face of one plane graph with another graph.\n\n", name);
    fprintf(stderr, "Usage\n=====\n");
    fprintf(stderr, " %s [options] u1,v1 u2,v2\n\n", name);
    fprintf(stderr, "This glues the face to the right of edge u1,v1 in the first graph to the face to\n");
    fprintf(stderr, "the left of u2,v2 in the second graph.\n");
    fprintf(stderr, "\nThis program can handle graphs up to %d vertices. Recompile if you need larger\n", MAXN);
    fprintf(stderr, "graphs.\n\n");
    fprintf(stderr, "Valid options\n=============\n");
    fprintf(stderr, "    -1 file\n");
    fprintf(stderr, "       Read the first graph from the specified file instead of stdin.\n");
    fprintf(stderr, "    -2 file\n");
    fprintf(stderr, "       Read the second graph from the specified file instead of stdin.\n");
    fprintf(stderr, "    -h, --help\n");
    fprintf(stderr, "       Print this help and return.\n");
}

void usage(char *name) {
    fprintf(stderr, "Usage: %s [options]\n", name);
    fprintf(stderr, "For more information type: %s -h \n\n", name);
}

int main(int argc, char *argv[]) {

    /*=========== commandline parsing ===========*/

    int c;
    char *name = argv[0];
    static struct option long_options[] = {
        {"help", no_argument, NULL, 'h'}
    };
    int option_index = 0;

    while ((c = getopt_long(argc, argv, "h1:2:", long_options, &option_index)) != -1) {
        switch (c) {
            case '1':
                fprintf(stderr, "Reading first graph from file %s.\n", optarg);
                firstFile = fopen(optarg, "r");
                break;
            case '2':
                fprintf(stderr, "Reading second graph from file %s.\n", optarg);
                secondFile = fopen(optarg, "r");
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
    
    if(argc - optind < 2){
        usage(name);
        return EXIT_FAILURE;
    }
    
    int from1, to1, from2, to2;
    if(sscanf(argv[optind + 0], "%d,%d", &from1, &to1)!=2){
        fprintf(stderr, "Error while reading edge 1.\n");
        usage(name);
        return EXIT_FAILURE;
    }
    if(sscanf(argv[optind + 1], "%d,%d", &from2, &to2)!=2){
        fprintf(stderr, "Error while reading edge 2.\n");
        usage(name);
        return EXIT_FAILURE;
    }
    
    from1--;
    to1--;
    from2--;
    to2--;

    /*=========== read planar graphs ===========*/

    unsigned short code[MAXCODELENGTH];
    int length;
    if (readPlanarCode(code, &length, firstFile == NULL ? stdin : firstFile)) {
        decodePlanarCode(code);
    } else {
        fprintf(stderr, "Could not read first graph -- exiting!\n");
        return EXIT_FAILURE;
    }
    if (readPlanarCode(code, &length, secondFile == NULL ? stdin : secondFile)) {
        decodePlanarCode2(code);
    } else {
        fprintf(stderr, "Could not read second graph -- exiting!\n");
        return EXIT_FAILURE;
    }
    
    //close files
    if(firstFile!=NULL){
        fclose(firstFile);
    }
    if(secondFile!=NULL){
        fclose(secondFile);
    }
    
    EDGE* e1 = findEdge(from1, to1);
    EDGE* e2 = findEdge2(from2, to2);
    
    //check faces have same size
    EDGE* e, *elast;
    int size1 = 0;
    e = elast = e1;
    do {
        size1++;
        e = e->inverse->prev;
    } while(e != elast);
    
    int size2 = 0;
    e = elast = e2;
    do {
        size2++;
        e = e->inverse->next;
    } while(e != elast);
    
    if(size1!=size2){
        fprintf(stderr, "Faces do not have the same size -- exiting!\n");
        return EXIT_FAILURE;
    }
    
    //check that resulting graph doesn't have to many vertices
    if(nv + nv2 - size1 > MAXN){
        fprintf(stderr, "Resulting graph has too many vertices -- exiting!\n");
        return EXIT_FAILURE;
    }
    
    //relabel vertices second graph
    int labels[nv2];
    int inverseLabels[nv + nv2 - size1];
    int i;
    for(i = 0; i < nv2; i++){
        labels[i] = -1;
    }
    
    EDGE* e_2 = e2;
    e = elast = e1;
    do {
        labels[e_2->end] = e->end;
        e = e->inverse->prev;
        e_2 = e_2->inverse->next;
    } while(e != elast);
    
    int currentLabel = nv;
    for(i = 0; i < nv2; i++){
        if(labels[i]<0){
            labels[i] = currentLabel++;
        }
    }
    
    if(nv + nv2 - size1 != currentLabel){
        fprintf(stderr, "Error while relabeling vertices -- exiting!\n");
        return EXIT_FAILURE;
    }
    
    for(i = 0; i < nv2; i++){
        inverseLabels[labels[i]] = i;
    }
    
    for(i = 0; i < nv2; i++){
        e = elast = firstedge2[i];
        do {
            e->start = labels[i];
            e->end = labels[e->end];
            e = e->next;
        } while(e != elast);
    }
    
    //subdivide face
    e = e1;
    int endVertex = e->start;
    e_2 = e2;
    do {
        EDGE* currentEdgeOuter = e;
        EDGE* currentEdgeOuterNext = e->next;
        EDGE* currentEdgeInner = e_2;
        EDGE* currentEdgeInnerPrev = e_2->prev;
        
        if(currentEdgeInner->next == currentEdgeInnerPrev){
            //do nothing
        } else {
            currentEdgeOuter->next = currentEdgeInner->next;
            currentEdgeInner->next->prev = currentEdgeOuter;
            currentEdgeOuterNext->prev = currentEdgeInnerPrev->prev;
            currentEdgeInnerPrev->prev->next = currentEdgeOuterNext;
            degree[currentEdgeOuter->start] += 
                    degree2[inverseLabels[currentEdgeOuter->start]] - 2;
        }
        
        e = e->inverse->prev;
        e_2 = e_2->inverse->next;
    } while(e->start != endVertex);
    /*can't use elast above since the face is removed while we go around*/
    
    for(i = nv; i < nv + nv2 - size1; i++){
        degree[i] = degree2[inverseLabels[i]];
        firstedge[i] = firstedge2[inverseLabels[i]];
    }
    
    nv += nv2 - size1;
    
    //write resulting graph
    writePlanarCode();
    
    //print relabeling in graph 2
    fprintf(stderr, "The vertex labels in the first graph were preserved.\n");
    fprintf(stderr, "The vertex labels in the second graph were changed as follows:\n");
    for(i = 0; i < nv2; i++){
        fprintf(stderr, "%d -> %d\n", i + 1, labels[i] + 1);
    }
}

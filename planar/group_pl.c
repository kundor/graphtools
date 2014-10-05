/*
 * Main developer: Nico Van Cleemput
 * 
 * Copyright (C) 2014 Ghent University.
 * Licensed under the GNU GPL, read the file LICENSE.txt for details.
 */

/* This program reads planar graphs from standard in and
 * determines the symmetry group for each graph.   
 * 
 * 
 * Compile with:
 *     
 *     cc -o group_pl -O4 group_pl.c
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <ctype.h>

#define UNKNOWN 0
#define Cn__    1
#define Cnh__   2
#define Cnv__   3
#define S2n__   4
#define Dn__    5
#define Dnh__   6
#define Dnd__   7
#define T__     8
#define Td__    9
#define Th__   10
#define O__    11
#define Oh__   12
#define I__    13
#define Ih__   14


#ifndef MAXN
#define MAXN 1000            /* the maximum number of vertices */
#endif
#define MAXE (6*MAXN-12)    /* the maximum number of oriented edges */
#define MAXF (2*MAXN-4)      /* the maximum number of faces */
#define MAXVAL (MAXN-1)  /* the maximum degree of a vertex */
#define MAXCODELENGTH (MAXN+MAXE+3)

#define INFI (MAXN + 1)

typedef int boolean;

#undef FALSE
#undef TRUE
#define FALSE 0
#define TRUE  1

typedef struct e /* The data type used for edges */ {
    int start;       /* vertex where the edge starts */
    int end;         /* vertex where the edge ends */
    int rightface;   /* face on the right side of the edge
                        note: only valid if make_dual() called */
    struct e *prev;  /* previous edge in clockwise direction */
    struct e *next;  /* next edge in clockwise direction */
    struct e *inverse; /* the edge that is inverse to this one */
    int mark, index; /* two ints for temporary use;
                          Only access mark via the MARK macros. */
} EDGE;

EDGE *firstedge[MAXN]; /* pointer to arbitrary edge out of vertex i. */
int degree[MAXN];

EDGE *facestart[MAXF]; /* pointer to arbitrary edge of face i. */
int faceSize[MAXF]; /* pointer to arbitrary edge of face i. */

EDGE edges[MAXE];

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
int nf;

int automorphisms[2*MAXE][MAXN]; //there are at most 2e automorphisms (e = #arcs)
boolean isOrientationReversingAutomorphism[2*MAXE];
int automorphismsCount;
int mirrorAutomorphismsCount;

int numberOfGraphs = 0;
int reportsWritten = 0;

//////////////////////////////////////////////////////////////////////////////

void printGroupName(FILE *f, int groupId, int groupParameter){
    if(groupId==UNKNOWN){
        fprintf(f, "UNKNOWN");
    } else if(groupId==Cn__){
        fprintf(f, "C%d", groupParameter);
    } else if(groupId==Cnh__){
        fprintf(f, "C%dh", groupParameter);
    } else if(groupId==Cnv__){
        fprintf(f, "C%dv", groupParameter);
    } else if(groupId==S2n__){
        fprintf(f, "S%d", 2*groupParameter);
    } else if(groupId==Dn__){
        fprintf(f, "D%d", groupParameter);
    } else if(groupId==Dnh__){
        fprintf(f, "D%dh", groupParameter);
    } else if(groupId==Dnd__){
        fprintf(f, "D%dd", groupParameter);
    } else if(groupId==T__){
        fprintf(f, "T");
    } else if(groupId==Td__){
        fprintf(f, "Td");
    } else if(groupId==Th__){
        fprintf(f, "Th");
    } else if(groupId==O__){
        fprintf(f, "O");
    } else if(groupId==Oh__){
        fprintf(f, "Oh");
    } else if(groupId==I__){
        fprintf(f, "I");
    } else if(groupId==Ih__){
        fprintf(f, "Ih");
    } else {
        fprintf(stderr, "Illegal group id: %d -- exiting!\n", groupId);
        exit(EXIT_FAILURE);
    }
}

//////////////////////////////////////////////////////////////////////////////

int certificate[MAXE+MAXN];
int canonicalLabelling[MAXN];
int reverseCanonicalLabelling[MAXN];
EDGE *canonicalFirstedge[MAXN];
int alternateCertificate[MAXE+MAXN];
int alternateLabelling[MAXN];
EDGE *alternateFirstedge[MAXN];
int queue[MAXN];

int constructCertificate(EDGE *eStart){
    int i;
    for(i=0; i<MAXN; i++){
        canonicalLabelling[i] = MAXN;
    }
    EDGE *e, *elast;
    int head = 1;
    int tail = 0;
    int vertexCounter = 1;
    int position = 0;
    queue[0] = eStart->start;
    canonicalFirstedge[eStart->start] = eStart;
    canonicalLabelling[eStart->start] = 0;
    while(head>tail){
        int currentVertex = queue[tail++];
        e = elast = canonicalFirstedge[currentVertex];
        do {
            if(canonicalLabelling[e->end]==MAXN){
                queue[head++] = e->end;
                canonicalLabelling[e->end] = vertexCounter++;
                canonicalFirstedge[e->end] = e->inverse;
            }
            certificate[position++] = canonicalLabelling[e->end];
            e = e->next;
        } while (e!=elast);
        certificate[position++] = MAXN;
    }
    return position;
}

void constructAlternateCertificate(EDGE *eStart){
    int i;
    for(i=0; i<MAXN; i++){
        alternateLabelling[i] = MAXN;
    }
    EDGE *e, *elast;
    int head = 1;
    int tail = 0;
    int vertexCounter = 1;
    int alternateCertificatePosition = 0;
    queue[0] = eStart->start;
    alternateFirstedge[eStart->start] = eStart;
    alternateLabelling[eStart->start] = 0;
    while(head>tail){
        int currentVertex = queue[tail++];
        e = elast = alternateFirstedge[currentVertex];
        do {
            if(alternateLabelling[e->end]==MAXN){
                queue[head++] = e->end;
                alternateLabelling[e->end] = vertexCounter++;
                alternateFirstedge[e->end] = e->inverse;
            }
            alternateCertificate[alternateCertificatePosition++] = alternateLabelling[e->end];
            e = e->next;
        } while (e!=elast);
        alternateCertificate[alternateCertificatePosition++] = MAXN;
    }
}

void constructAlternateCertificateOrientationReversing(EDGE *eStart){
    int i;
    for(i=0; i<MAXN; i++){
        alternateLabelling[i] = MAXN;
    }
    EDGE *e, *elast;
    int head = 1;
    int tail = 0;
    int vertexCounter = 1;
    int alternateCertificatePosition = 0;
    queue[0] = eStart->start;
    alternateFirstedge[eStart->start] = eStart;
    alternateLabelling[eStart->start] = 0;
    while(head>tail){
        int currentVertex = queue[tail++];
        e = elast = alternateFirstedge[currentVertex];
        do {
            if(alternateLabelling[e->end]==MAXN){
                queue[head++] = e->end;
                alternateLabelling[e->end] = vertexCounter++;
                alternateFirstedge[e->end] = e->inverse;
            }
            alternateCertificate[alternateCertificatePosition++] = alternateLabelling[e->end];
            e = e->prev;
        } while (e!=elast);
        alternateCertificate[alternateCertificatePosition++] = MAXN;
    }
}

void determineAutomorphisms(){
    int i, j;
    
    //identity
    for(i = 0; i < nv; i++){
        automorphisms[0][i] = i;
    }
    isOrientationReversingAutomorphism[0] = FALSE;
    
    automorphismsCount = 1;
    mirrorAutomorphismsCount = 0;
    
    //construct certificate
    int pos = 0;
    
    pos = constructCertificate(firstedge[0]);
    for(i = 0; i < nv; i++){
        reverseCanonicalLabelling[canonicalLabelling[i]] = i;
    }
    
    //construct alternate certificates
    EDGE *ebase = firstedge[0];
    
    for(i=0; i<nv; i++){
        if(degree[i]==degree[0]){
            EDGE *e, *elast;

            e = elast = firstedge[i];
            do {
                if(e!=ebase){
                    constructAlternateCertificate(e);
                    if(memcmp(certificate, alternateCertificate, sizeof(int)*pos) == 0) {
                        //store automorphism
                        for(j = 0; j < nv; j++){
                            automorphisms[automorphismsCount][j] = reverseCanonicalLabelling[alternateLabelling[j]];
                        }
                        isOrientationReversingAutomorphism[automorphismsCount] = FALSE;
                        automorphismsCount++;
                    }
                }
                constructAlternateCertificateOrientationReversing(e);
                if(memcmp(certificate, alternateCertificate, sizeof(int)*pos) == 0) {
                    //store automorphism
                    for(j = 0; j < nv; j++){
                        automorphisms[automorphismsCount][j] = reverseCanonicalLabelling[alternateLabelling[j]];
                    }
                    isOrientationReversingAutomorphism[automorphismsCount] = TRUE;
                    automorphismsCount++;
                    mirrorAutomorphismsCount++;
                }
                e = e->next;
            } while (e!=elast);
        }
    }
}

int identifyRotationalSymmetryThroughVertex(int v){
    EDGE *base, *image;
    int deg, i, length;
    
    deg = degree[v];
    base = image = firstedge[v];
    i = 0;
    
    length = constructCertificate(base);
    
    while(i < deg/2){
        i++;
        image = image->next;
        constructAlternateCertificate(image);
        if(memcmp(certificate, alternateCertificate, sizeof(int)*length) == 0){
            return deg/i;
        }
    }
    
    return 1;
}

int identifyRotationalSymmetryThroughFace(int f){
    EDGE *base, *image;
    int deg, i, length;
    
    deg = faceSize[f];
    base = image = facestart[f];
    i = 0;
    
    length = constructCertificate(base);
    
    while(i < deg/2){
        i++;
        image = image->next->inverse;
        constructAlternateCertificate(image);
        if(memcmp(certificate, alternateCertificate, sizeof(int)*length) == 0){
            return deg/i;
        }
    }
    
    return 1;
}

boolean hasRotationalSymmetryThroughEdge(EDGE *e){
    int length = constructCertificate(e);
    constructAlternateCertificate(e->inverse);
    return memcmp(certificate, alternateCertificate, sizeof(int)*length) == 0;
}

/* Returns TRUE if the graph contains a mirror symmetry fixating the given vertex.
 * This method assumes that the automorphism group is already determined.
 */
boolean hasOrientationReversingSymmetryStabilisingGivenVertex(int v){
    int i;
    
    for(i = 0; i < automorphismsCount; i++){
        if(isOrientationReversingAutomorphism[i] 
                && automorphisms[i][v] == v){
            return TRUE;
        }
    }
    
    return FALSE;
}

/* Returns TRUE if the graph contains a mirror symmetry fixating the given face.
 * This method assumes that the automorphism group is already determined.
 */
boolean hasOrientationReversingSymmetryStabilisingGivenFace(int f){
    int i, from, to;
    EDGE *e;
    
    for(i = 0; i < automorphismsCount; i++){
        if(isOrientationReversingAutomorphism[i]){
            e = facestart[f];
            from = automorphisms[i][e->start];
            to = automorphisms[i][e->end];
            e = firstedge[from];
            while(e->end != to) {
                e = e->next;
            }
            if(e->inverse->rightface == f){
                return TRUE;
            }
        }
    }
    
    return FALSE;
}

/* Returns TRUE if the graph contains a mirror symmetry fixating the given edge.
 * This method assumes that the automorphism group is already determined.
 */
boolean hasOrientationReversingSymmetryStabilisingGivenEdge(int e){
    int i, from, to;
    EDGE *edge = edges+e;
    
    for(i = 0; i < automorphismsCount; i++){
        if(isOrientationReversingAutomorphism[i]){
            from = automorphisms[i][edge->start];
            to = automorphisms[i][edge->end];
            if((edge->start == from && edge->end == to) ||
                    (edge->start == to && edge->end == from)){
                return TRUE;
            }
        }
    }
    
    return FALSE;
}

boolean hasOrientationReversingSymmetryWithFixPoint(){
    int i, j;
    
    //first check if any vertices are fixed
    for(i = 0; i < automorphismsCount; i++){
        if(isOrientationReversingAutomorphism[i]){
            for(j = 0; j < nv; j++){
                if(automorphisms[i][j] == j){
                    return TRUE;
                }
            }
        }
    }
    
    //next we check if any edge is fixed as a set
    //we don't need to check that a directed edge is fixed
    //because in that case also the vertices are fixed
    for(i = 0; i < automorphismsCount; i++){
        if(isOrientationReversingAutomorphism[i]){
            for(j = 0; j < ne; j++){
                if(j < edges[j].inverse->index){
                    int start = edges[j].start;
                    int end = edges[j].end;
                    if(automorphisms[i][start] == end &&
                            automorphisms[i][end] == start){
                        return TRUE;
                    }
                }
            }
        }
    }
    
    //we don't need to check if there is a fixpoint in a face
    //because in that case at least either a vertex or an edge
    //is fixed
    return FALSE;
}

boolean isOrientationReversingSymmetryWithFixPoint(int i){
    int j;
    
    if(!isOrientationReversingAutomorphism[i]){
        return FALSE;
    }
        
    //first check if any vertices are fixed
    for(j = 0; j < nv; j++){
        if(automorphisms[i][j] == j){
            return TRUE;
        }
    }
    
    //next we check if any edge is fixed as a set
    //we don't need to check that a directed edge is fixed
    //because in that case also the vertices are fixed
    for(j = 0; j < ne; j++){
        if(j < edges[j].inverse->index){
            int start = edges[j].start;
            int end = edges[j].end;
            if(automorphisms[i][start] == end &&
                    automorphisms[i][end] == start){
                return TRUE;
            }
        }
    }
    
    //we don't need to check if there is a fixpoint in a face
    //because in that case at least either a vertex or an edge
    //is fixed
    return FALSE;
}

int countOrientationReversingSymmetriesWithFixPoints(){
    int i, count;
    
    count = 0;
    
    //first check if any vertices are fixed
    for(i = 0; i < automorphismsCount; i++){
        if(isOrientationReversingSymmetryWithFixPoint(i)){
            count++;
        }
    }
    
    return count;
}

void determineAutomorphismGroupInfiniteFamilies(int *groupId,
        int *groupParameter, int rotDegree, int center, boolean centerIsVertex,
        boolean centerIsEdge){
    if(mirrorAutomorphismsCount==0){
        if(automorphismsCount==rotDegree){
            *groupId = Cn__;
        } else if(automorphismsCount==2*rotDegree){
            *groupId = Dn__;
        } else {
            fprintf(stderr, "Illegal order for chiral axial symmetry group containing a %d-fold rotation: %d -- exiting!\n", rotDegree, automorphismsCount);
            exit(EXIT_FAILURE);
        }
        *groupParameter = rotDegree;
    } else if(automorphismsCount == 4*rotDegree){
        int orientationReversingSymmetriesWithFixPoints = 
                      countOrientationReversingSymmetriesWithFixPoints();
        if(orientationReversingSymmetriesWithFixPoints == rotDegree){
            *groupId = Dnd__;
            *groupParameter = rotDegree;
        } else if(orientationReversingSymmetriesWithFixPoints == rotDegree + 1){
            *groupId = Dnh__;
            *groupParameter = rotDegree;
        } else {
            fprintf(stderr, "Illegal number of orientation reversing automorphisms with fixpoints for chiral axial symmetry group containing a %d-fold rotation that has order %d: %d -- exiting!\n", rotDegree, automorphismsCount, orientationReversingSymmetriesWithFixPoints);
            exit(EXIT_FAILURE);
        }
    } else if(automorphismsCount == 2*rotDegree){
        if(centerIsVertex){
            if(hasOrientationReversingSymmetryStabilisingGivenVertex(center)){
                *groupId = Cnv__;
                *groupParameter = rotDegree;
                return;
            }
        } else if(centerIsEdge){
            if(hasOrientationReversingSymmetryStabilisingGivenEdge(center)){
                *groupId = Cnv__;
                *groupParameter = rotDegree;
                return;
            }
        } else {
            if(hasOrientationReversingSymmetryStabilisingGivenFace(center)){
                *groupId = Cnv__;
                *groupParameter = rotDegree;
                return;
            }
        }
        //if the rotational center is not fixed by any orientation reversing symmetry,
        //we check if there is any fix point
        if(hasOrientationReversingSymmetryWithFixPoint()){
            *groupId = Cnh__;
            *groupParameter = rotDegree;
        } else {
            *groupId = S2n__;
            *groupParameter = rotDegree;
        }
    } else {
        fprintf(stderr, "Illegal order for achiral axial symmetry group containing a %d-fold rotation: %d -- exiting!\n", rotDegree, automorphismsCount);
        exit(EXIT_FAILURE);
    }
}

void determineAutomorphismGroupOther(int *groupId, int *groupParameter){
    //most of these groups can be identified based on the size of the group
    //or the size of the group combined with the chirality
    //the only exceptions are Td and Th. These have the same size and the same
    //chirality.
    if(automorphismsCount==120){
        *groupId = Ih__;
    } else if(automorphismsCount==60){
        *groupId = I__;
    } else if(automorphismsCount==48){
        *groupId = Oh__;
    } else if(automorphismsCount==24){
        if(mirrorAutomorphismsCount==0){
            *groupId = O__;
        } else {
            int orientationReversingSymmetriesWithFixPoints = 
                      countOrientationReversingSymmetriesWithFixPoints();
            if(orientationReversingSymmetriesWithFixPoints == 6){
                *groupId = Td__;
            } else if(orientationReversingSymmetriesWithFixPoints == 3){
                *groupId = Th__;
            } else {
                fprintf(stderr, "Illegal number of orientation reversing automorphisms with fixpoints for achiral non-axial symmetry group with order 24: %d -- exiting!\n", orientationReversingSymmetriesWithFixPoints);
                exit(EXIT_FAILURE);
            }
        }
    } else if(automorphismsCount==12){
        *groupId = T__;
    } else {
        fprintf(stderr, "Illegal order for a non-axial symmetry group -- exiting!\n");
        exit(EXIT_FAILURE);
    }
}

void determineAutomorphismGroup(int *groupId, int *groupParameter){
    int i, j;
    int maxRotation, maxRotationCenter;
    boolean maxRotationCenterIsVertex, maxRotationCenterIsEdge;
    maxRotation = -1;
    
    //we start by determining all automorphisms
    determineAutomorphisms();
    
    //if the group is chiral and has an odd order or an order less than 4
    //then it is a cyclic group. (since D1 is equal to C2)
    if(mirrorAutomorphismsCount == 0 && 
            (automorphismsCount < 4 || automorphismsCount%2 == 1)){
        *groupId = Cn__;
        *groupParameter = automorphismsCount;
        return;
    } else if(mirrorAutomorphismsCount == 1 && automorphismsCount == 2){
        *groupId = Cnh__;
        *groupParameter = 1;
        return;
    }
    
    //next we will count the number of rotational axis
    //each axis gets counted twice
    
    //first we set the counts to 0
    int foldCount[6];
    for(i = 0; i < 6; i++){
        foldCount[i] = 0;
    }
    
    //first we look for rotational axis through a vertex
    for(i = 0; i < nv; i++){
        int rotDegree = identifyRotationalSymmetryThroughVertex(i);
        
        //if the order of the rotation is larger than 5, then we known that it 
        //has to be one of the axial symmetry groups
        if(rotDegree>5){
            determineAutomorphismGroupInfiniteFamilies(
                    groupId, groupParameter, rotDegree, i, TRUE, FALSE);
            return;
        }
        //otherwise we just count the axis
        foldCount[rotDegree]++;
        
        //store the maxRotationCenter
        if(rotDegree > maxRotation){
            maxRotation = rotDegree;
            maxRotationCenter = i;
            maxRotationCenterIsEdge = FALSE;
            maxRotationCenterIsVertex = TRUE;
        }
        
        //if there is more than one 3-fold rotational axis, then it can't be
        //one of the axial symmetry groups
        if(foldCount[3] > 2){
            determineAutomorphismGroupOther(groupId, groupParameter);
            return;
        }
        
        //if there is more than one type of n-fold rotational axis with n at
        //least 3, then it can't be one of the axial symmetry groups
        int rotTypeCount = 0;
        for(j = 3; j < 6; j++){
            if(foldCount[j]){
                rotTypeCount++;
            }
        }
        if(rotTypeCount>1){
            determineAutomorphismGroupOther(groupId, groupParameter);
            return;
        }
    }
    
    for(i = 0; i < nf; i++){
        int rotDegree = identifyRotationalSymmetryThroughFace(i);
        
        //if the order of the rotation is larger than 5, then we known that it 
        //has to be one of the axial symmetry groups
        if(rotDegree>5){
            determineAutomorphismGroupInfiniteFamilies(
                    groupId, groupParameter, rotDegree, i, FALSE, FALSE);
            return;
        }
        foldCount[rotDegree]++;
        
        //store the maxRotationCenter
        if(rotDegree > maxRotation){
            maxRotation = rotDegree;
            maxRotationCenter = i;
            maxRotationCenterIsEdge = FALSE;
            maxRotationCenterIsVertex = FALSE;
        }
        
        //if there is more than one 3-fold rotational axis, then it can't be
        //one of the axial symmetry groups
        if(foldCount[3] > 2){
            determineAutomorphismGroupOther(groupId, groupParameter);
            return;
        }
        
        //if there is more than one type of n-fold rotational axis with n at
        //least 3, then it can't be one of the axial symmetry groups
        int rotTypeCount = 0;
        for(j = 3; j < 6; j++){
            if(foldCount[j]){
                rotTypeCount++;
            }
        }
        if(rotTypeCount>1){
            determineAutomorphismGroupOther(groupId, groupParameter);
            return;
        }
    }
    
    if(foldCount[3]<=2){
        //first we also check all edges, so that we also have all 2-fold rotations
        for(i = 0; i < ne; i++){
            if(i < edges[i].inverse->index 
                    && hasRotationalSymmetryThroughEdge(edges+i)){
                foldCount[2]++;
                
                //store the maxRotationCenter
                if(2 > maxRotation){
                    maxRotation = 2;
                    maxRotationCenter = i;
                    maxRotationCenterIsEdge = TRUE;
                    maxRotationCenterIsVertex = FALSE;
                }
            }
        }
        determineAutomorphismGroupInfiniteFamilies(
                groupId, groupParameter, maxRotation, maxRotationCenter,
                maxRotationCenterIsVertex, maxRotationCenterIsEdge);
    } else {
        //shouldn't happen
        determineAutomorphismGroupOther(groupId, groupParameter);
    }
}

//////////////////////////////////////////////////////////////////////////////


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

/* Store in the rightface field of each edge the number of the face on
   the right hand side of that edge.  Faces are numbered 0,1,....  Also
   store in facestart[i] an example of an edge in the clockwise orientation
   of the face boundary, and the size of the face in facesize[i], for each i.
   Returns the number of faces. */
void makeDual() {
    register int i, sz;
    register EDGE *e, *ex, *ef, *efx;

    RESETMARKS;

    nf = 0;
    for (i = 0; i < nv; ++i) {

        e = ex = firstedge[i];
        do {
            if (!ISMARKEDLO(e)) {
                facestart[nf] = ef = efx = e;
                sz = 0;
                do {
                    ef->rightface = nf;
                    MARKLO(ef);
                    ef = ef->inverse->prev;
                    ++sz;
                } while (ef != efx);
                faceSize[nf] = sz;
                ++nf;
            }
            e = e->next;
        } while (e != ex);
    }
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
        edges[edgeCounter].index = edgeCounter;
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
            edges[edgeCounter].index = edgeCounter;
            edgeCounter++;
        }
        firstedge[i]->prev = edges + edgeCounter - 1;
        edges[edgeCounter - 1].next = firstedge[i];
        degree[i] = j;

        codePosition++; /* read the closing 0 */
    }

    ne = edgeCounter;

    makeDual();

    // nv - ne/2 + nf = 2
}

/**
 * 
 * @param code
 * @param laenge
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

//================== PARSE GROUP NAME ================

int parseGroupParameter(char* input, int* groupParameter, boolean *anyParameterAllowed){
    if (input[0] == '*'){
        *anyParameterAllowed = TRUE;
        return 1;
    } else if (isdigit(input[0])){
        int length = 1;
        while(isdigit(input[length])) length++;
        *groupParameter = atoi(input);
        *anyParameterAllowed = FALSE;
        return length;
    } else {
        fprintf(stderr, "Illegal group parameter: %s -- exiting!\n", input);
        exit(EXIT_FAILURE);
    }
}

void parseGroup(char* input, int* groupId, int* groupParameter, boolean* anyParameterAllowed){
    if (input[0] == 'T'){
        if (input[1] == '\0') {
            *groupId = T__;
        } else if (input[2] != '\0') {
            fprintf(stderr, "Illegal group name: %s -- exiting!\n", input);
            exit(EXIT_FAILURE);
        } else if (input[1] == 'd') {
            *groupId = Td__;
        } else if (input[1] == 'h') {
            *groupId = Th__;
        } else {
            fprintf(stderr, "Illegal group name: %s -- exiting!\n", input);
            exit(EXIT_FAILURE);
        }
    } else if (input[0] == 'O') {
        if (input[1] == '\0') {
            *groupId = O__;
        } else if (input[2] != '\0') {
            fprintf(stderr, "Illegal group name: %s -- exiting!\n", input);
            exit(EXIT_FAILURE);
        } else if (input[1] == 'h') {
            *groupId = Oh__;
        } else {
            fprintf(stderr, "Illegal group name: %s -- exiting!\n", input);
            exit(EXIT_FAILURE);
        }
    } else if (input[0] == 'I') {
        if (input[1] == '\0') {
            *groupId = I__;
        } else if (input[2] != '\0') {
            fprintf(stderr, "Illegal group name: %s -- exiting!\n", input);
            exit(EXIT_FAILURE);
        } else if (input[1] == 'h') {
            *groupId = Ih__;
        } else {
            fprintf(stderr, "Illegal group name: %s -- exiting!\n", input);
            exit(EXIT_FAILURE);
        }
    } else if (input[0] == 'C') {
        int length = parseGroupParameter(input + 1, groupParameter, anyParameterAllowed);
        if (!(*anyParameterAllowed) && (*groupParameter <= 0)) {
            fprintf(stderr, "Illegal group parameter: %d -- exiting!\n", *groupParameter);
            exit(EXIT_FAILURE);
        } else if (input[length + 1] == '\0') {
            *groupId = Cn__;
        } else if (input[length + 1] == 'h' && input[length + 2] == '\0') {
            *groupId = Cnh__;
        } else if (input[length + 1] == 'v' && input[length + 2] == '\0') {
            *groupId = Cnv__;
        } else {
            fprintf(stderr, "Illegal group name: %s -- exiting!\n", input);
            exit(EXIT_FAILURE);
        }
    } else if (input[0] == 'S') {
       int length = parseGroupParameter(input + 1, groupParameter, anyParameterAllowed);
       if (input[length + 1] != '\0') {
            fprintf(stderr, "Illegal group name: %s -- exiting!\n", input);
            exit(EXIT_FAILURE);
       } else if (!(*anyParameterAllowed) && 
               (*groupParameter <= 0 || (*groupParameter)%2 == 1)) {
            fprintf(stderr, "Illegal group parameter: %d -- exiting!\n", *groupParameter);
            exit(EXIT_FAILURE);
       } else {
           *groupId = S2n__;
       }
    } else if (input[0] == 'D') {
        int length = parseGroupParameter(input + 1, groupParameter, anyParameterAllowed);
        if (!(*anyParameterAllowed) && (*groupParameter <= 0)) {
            fprintf(stderr, "Illegal group parameter: %d -- exiting!\n", *groupParameter);
            exit(EXIT_FAILURE);
        } else if (input[length + 1] == '\0') {
            *groupId = Dn__;
        } else if (input[length + 1] == 'h' && input[length + 2] == '\0') {
            *groupId = Dnh__;
        } else if (input[length + 1] == 'd' && input[length + 2] == '\0') {
            *groupId = Dnd__;
        } else {
            fprintf(stderr, "Illegal group name: %s -- exiting!\n", input);
            exit(EXIT_FAILURE);
        }
    } else {
        fprintf(stderr, "Illegal group name: %s -- exiting!\n", input);
        exit(EXIT_FAILURE);
    }
}

//====================== USAGE =======================

void help(char *name) {
    fprintf(stderr, "The program %s generates a summary of planar graphs.\n\n", name);
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

int main(int argc, char *argv[]) {

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

    /*=========== read planar graphs ===========*/

    unsigned short code[MAXCODELENGTH];
    int length;
    while (readPlanarCode(code, &length, stdin)) {
        decodePlanarCode(code);
        numberOfGraphs++;
        int groupId = UNKNOWN;
        int groupParameter = 0;
        determineAutomorphismGroup(&groupId, &groupParameter);
        fprintf(stderr, "Graph %d has group ", numberOfGraphs);
        printGroupName(stderr, groupId, groupParameter);
        fprintf(stderr, "\n");
    }

}
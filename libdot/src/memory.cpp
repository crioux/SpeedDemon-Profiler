/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/
#include "geometry.h"
#include "mem.h"
/* #include <vmalloc.h> */
#include <stdlib.h>
#include <stdio.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

void
freeinit(Freelist *fl, int size)
{
  
    fl -> head = NULL;
    fl -> nodesize = size;
    if (fl->blocklist != NULL) {
        Freeblock  *bp, *np;

        bp = fl->blocklist;
        while (bp != NULL) {
            np = bp->next;
            free (bp);
            bp = np;
        }
    }
    fl -> blocklist = NULL;
}

void *
myalloc(unsigned int n)
{
    void *t;
  
    if ((t=malloc(n)) == NULL) {    
      fprintf(stderr, "Insufficient memory\n");
      exit(1);
    }
    return(t);
}

void *
getfree(Freelist *fl)
{
    int i; 
    Freenode *t;
    Freeblock *mem;

    if(fl->head == NULL) {
        int  size = fl->nodesize;
        char *cp;
        mem = (Freeblock *)(myalloc(sizeof(Freeblock*) + sqrt_nsites * size));
        cp = ((char*)mem) + sizeof(Freeblock*);
        for(i=0; i<sqrt_nsites; i++) {
            makefree(cp + i*size, fl);
        }
        mem->next = fl->blocklist;
        fl->blocklist = mem;
    }
    t = fl -> head;
    fl -> head = t -> nextfree;
    return((void *)t);
}

void
makefree(void *curr, Freelist *fl)
{
    ((Freenode*)curr) -> nextfree = fl -> head;
    fl -> head = (Freenode*)curr;
}

#ifdef HEAP
void *getheap(Heap *hp, int sz)
{
    char      *mem;
    Heapblock *blp;
    int       size;

    if (hp->left < sz) {
        if (sz > hp->size) size = sz;
        else size = hp->size;
        blp = myalloc(sizeof(Heapblock*) + size);
        hp->head = ((char*)blp) + sizeof(Heapblock*);
        hp->left = size;
        blp->next = hp->blocklist;
        hp->blocklist = blp;
    }
    
    mem = hp->head;
    hp->head = mem+sz;
    hp->left -= sz;
    return((void *)mem);
}

void heapinit(Heap *hp, int size)
{
    hp -> head = NULL;
    hp -> size = size;
    hp -> left = 0;
    if (hp->blocklist != NULL) {
        Heapblock  *bp, *np;

        bp = hp->blocklist;
        while (bp != NULL) {
            np = bp->next;
            free (bp);
            bp = np;
        }
    }
    hp -> blocklist = NULL;
}

#endif

/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/


#ifndef MEMORY_H
#define MEMORY_H

#ifndef NULL
#define NULL 0
#endif

  /* Support for freelists */

typedef struct Freenode {
    struct Freenode    *nextfree;
} Freenode;

typedef struct freeblock {
    struct freeblock   *next;
} Freeblock;

typedef struct Freelist{
    struct Freenode    *head;          /* List of free nodes */
    Freeblock          *blocklist;     /* List of malloced blocks */
    int                nodesize;       /* Size of node */
} Freelist;

extern void *getfree(Freelist *);
extern void *myalloc(unsigned int);
extern void freeinit(Freelist *, int);
extern void makefree(void *,Freelist *);

#ifdef HEAP
  /* Support for heap */
typedef struct heapblock {
    struct heapblock   *next;
} Heapblock;

typedef struct {
    char*              head;           /* Available memory */
    Heapblock          *blocklist;     /* List of malloced blocks */
    int                size;           /* Size of blocks */
    int                left;           /* Bytes left */
} Heap;

extern void *getheap(Heap *, int);
extern void heapinit(Heap *, int);
#endif

#endif


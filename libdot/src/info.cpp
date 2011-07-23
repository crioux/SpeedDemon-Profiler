/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/
#include <stdio.h>
#include "neato.h"
#include "mem.h"
#include "info.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

Info_t*    nodeInfo;          /* Array of node info */
static Freelist pfl;

void
infoinit ()
{
    freeinit(&pfl, sizeof (PtItem));
}

#ifdef OLD
static int
lessThan (Point* o, PtItem* p, PtItem* q)
{
    double x0 = ((double)(p->p.x)) - ((double)(o->x));
    double y0 = ((double)(p->p.y)) - ((double)(o->y));
    double x1;
    double y1;
    double a, b;

    if (q == NULL) return 1;

    x1 = ((double)(q->p.x)) - ((double)(o->x));
    y1 = ((double)(q->p.y)) - ((double)(o->y));
    if (x0 >= 0.0) {
        if (x1 < 0.0) return 1;
        else if (x0 > 0.0) {
            if (x1 > 0.0) {
                a = y1/x1;
                b = y0/x0;
                return (a > b);
            }
            else return (y1 >= 0.0);
        }
        else {
            if (x1 > 0.0) return (y0 <= 0.0);
            else return (y0 < y1);
        }
    }
    else {
        if (x1 >= 0.0) return 0;
        else {
            a = y1/x1;
            b = y0/x0;
            return (a > b);
        }
    }
}
#endif

/* compare:
 * returns -1 if p < q
 *          0 if p = q
 *          1 if p > q
 * if q if NULL, returns -1
 * Ordering is by angle from -pi/2 to 3pi/4.
 * For equal angles (which should not happen in our context)
 * ordering is by closeness to origin.
 */
static int
compare (Point* o, PtItem* p, PtItem* q)
{
    double x0;
    double y0;
    double x1;
    double y1;
    double a, b;

    if (q == NULL) return -1;
    if ((p->p.x == q->p.x) && (p->p.y == q->p.y)) return 0;

    x0 = ((double)(p->p.x)) - ((double)(o->x));
    y0 = ((double)(p->p.y)) - ((double)(o->y));
    x1 = ((double)(q->p.x)) - ((double)(o->x));
    y1 = ((double)(q->p.y)) - ((double)(o->y));
    if (x0 >= 0.0) {
      if (x1 < 0.0) return -1;
      else if (x0 > 0.0) {
        if (x1 > 0.0) {
          a = y1/x1;
          b = y0/x0;
          if (b < a) return -1;
          else if (b > a) return 1;
          else if (x0 < x1) return -1;
          else return 1;
        }
        else {  /* x1 == 0.0 */
          if (y1 > 0.0) return -1;
          else return 1;
        }
      }
      else {        /* x0 == 0.0 */
        if (x1 > 0.0) {
          if (y0 <= 0.0) return -1;
          else return 1;
        }
        else {    /* x1 == 0.0 */
          if (y0 < y1) {
            if (y1 <= 0.0) return 1;
            else return -1;
          }
          else {
            if (y0 <= 0.0) return -1;
            else return 1;
          }
        }
      }
    }
    else {
      if (x1 >= 0.0) return 1;
      else {
          a = y1/x1;
          b = y0/x0;
          if (b < a) return -1;
          else if (b > a) return 1;
          else if (x0 > x1) return -1;
          else return 1;
      }
    }
}

static void
printV (PtItem *vp)
{
    if (vp == NULL) {
       fprintf (stderr, "<empty>\n");
       return;
    }

    while (vp != NULL) {
       fprintf (stderr, "(%.16f,%.16f)\n", vp->p.x, vp->p.y);
       vp = vp->next;
    }
}

static void
error (Info_t* ip, Site* s, float x, float y)
{
    fprintf (stderr, "Unsorted vertex list for site %d (%.16f,%.16f), pt (%f,%f)\n",
        s->sitenbr, s->coord.x, s->coord.y, x, y);
    printV (ip->verts);
}

#ifdef OLD
static int
sorted (Point* origin, PtItem* vp)
{
    PtItem*  next;

    if (vp == NULL) return 1;
    next = vp->next;

    while(next != NULL) {
      if (lessThan(origin,vp,next) || 
           ((next->p.x == vp->p.x) && (next->p.y == vp->p.y))) {
         vp = next;
         next = next->next;
      }
      else {
          fprintf (stderr, "(%.16f,%.16f) > (%.16f,%.16f)\n",
            vp->p.x, vp->p.y, next->p.x, next->p.y);
          return 0;
      }
    }

    return 1;
    
}

void 
addVertex (Site* s, float x, float y)
{
    Info_t*     ip;
    PtItem*   p;
    PtItem*   curr;
    PtItem*   prev;
    Point*    origin = &(s->coord);
    PtItem    tmp;

/* fprintf (stderr, "addVertex (%d, %f, %f)\n", s->sitenbr, x, y); */
    ip = nodeInfo + (s->sitenbr);
    curr = ip->verts;

    tmp.p.x = x;
    tmp.p.y = y;


    if (lessThan(origin, &tmp, curr)) {
        p = (PtItem *) getfree(&pfl);
        p->p.x = x;
        p->p.y = y;
        p->next = curr;
        ip->verts = p;
        return;
    }

    prev = curr;
    curr = curr->next;
    while (!lessThan (origin, &tmp, curr)) {
        prev = curr;
        curr = curr->next;
    }
    if ((x == prev->p.x) && (y == prev->p.y)) return;
    p = (PtItem *) getfree(&pfl);
    p->p.x = x;
    p->p.y = y;
    prev->next = p;
    p->next = curr;

    if (!sorted(origin,ip->verts)) 
      error (ip,s,x,y);
}
#endif

static int
sorted (Point* origin, PtItem* vp)
{
    PtItem*  next;

    if (vp == NULL) return 1;
    next = vp->next;

    while(next != NULL) {
      if (compare(origin,vp,next) <= 0) {
         vp = next;
         next = next->next;
      }
      else {
          fprintf (stderr, "(%.16f,%.16f) > (%.16f,%.16f)\n",
            vp->p.x, vp->p.y, next->p.x, next->p.y);
          return 0;
      }
    }

    return 1;
    
}

void 
addVertex (Site* s, float x, float y)
{
    Info_t*   ip;
    PtItem*   p;
    PtItem*   curr;
    PtItem*   prev;
    Point*    origin = &(s->coord);
    PtItem    tmp;
    int       cmp;

/* fprintf (stderr, "addVertex (%d, %f, %f)\n", s->sitenbr, x, y); */
    ip = nodeInfo + (s->sitenbr);
    curr = ip->verts;

    tmp.p.x = x;
    tmp.p.y = y;

    cmp = compare(origin, &tmp, curr);
    if (cmp == 0) return;
    else if (cmp < 0) {
        p = (PtItem *) getfree(&pfl);
        p->p.x = x;
        p->p.y = y;
        p->next = curr;
        ip->verts = p;
        return;
    }

    prev = curr;
    curr = curr->next;
    while ((cmp = compare(origin, &tmp, curr)) > 0) {
        prev = curr;
        curr = curr->next;
    }
    if (cmp == 0) return;
    p = (PtItem *) getfree(&pfl);
    p->p.x = x;
    p->p.y = y;
    prev->next = p;
    p->next = curr;

    /* This test should be unnecessary */
      /* if (!sorted(origin,ip->verts))  */
        /* error (ip,s,x,y); */

}

/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/
/*  vladimir@cs.ualberta.ca,  9-Dec-1997
 *	merge edges with specified samehead/sametail onto the same port
 */

#include	"dot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define MAXSAME 5               /* max no of same{head,tail} groups on a node */

typedef struct same_t {
  char  *id;                    /* group id */
  elist l;                      /* edges in the group */
  int   n_arr;                  /* number of edges with arrows */
  float arr_len;                /* arrow length of an edge in the group */
} same_t;
static int n_same;              /* number of same_t groups on current node */

static void sameedge (same_t* same, node_t* n, edge_t *e, char *id);
static void sameport (node_t *u, elist *l, float arr_len);

void
dot_sameports (graph_t* g)
/* merge edge ports in G */
{
  node_t *n;
  edge_t *e;
  char   *id;
  same_t same[MAXSAME];
  int    i;

  E_samehead = agfindattr(g->proto->e,"samehead");
  E_sametail = agfindattr(g->proto->e,"sametail");
  if (!(E_samehead || E_sametail)) return;
  for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
    n_same = 0;
    for (e = agfstedge(g,n); e; e = agnxtedge(g,e,n)) {
      if (e->head==n && E_samehead &&
          (id = agxget (e, E_samehead->index))[0])
        sameedge (same, n, e, id);
      else if (e->tail==n && E_sametail &&
               (id = agxget (e, E_sametail->index))[0])
        sameedge (same, n, e, id);
    }
    for (i=0; i<n_same; i++) {
      if (same[i].l.size>1) sameport (n, &same[i].l, same[i].arr_len);
      free_list(same[i].l);
      /* I sure hope I don't need to free the char* id */
    }
  }
}

static void
sameedge (same_t* same, node_t* n, edge_t *e, char *id)
/* register E in the SAME structure of N under ID. Uses static int N_SAME */
{
  int i,sflag,eflag,flag;

  for (i=0; i<n_same; i++)
    if (streq(same[i].id,id)) {
      elist_append(e,same[i].l);
      goto set_arrow;
    }
  if (++n_same > MAXSAME) {
    fprintf(stderr,"too many same{head,tail} groups for node %s\n", n->name);
    return;
  }
  alloc_elist(1,same[i].l);
  elist_fastapp(e,same[i].l);
  same[i].id = id;
  same[i].n_arr = 0;
  same[i].arr_len = 0;
set_arrow:
  arrow_flags (e, &sflag, &eflag);
  if ((flag = e->head==n ? eflag : sflag))
    same[i].arr_len =
      /* only consider arrows if there's exactly one arrow */
      (++same[i].n_arr==1) ? arrow_length(e,flag) : 0;
}

static void
sameport (node_t *u, elist *l, float arr_len)
/* make all edges in L share the same port on U. The port is placed on the
   node boundary and the average angle between the edges. FIXME: this assumes
   naively that the edges are straight lines, which is wrong if they are long.
   In that case something like concentration could be done.

   An arr_port is also computed that's ARR_LEN away from the node boundary.
   It's used for edges that don't themselves have an arrow.
*/
{
  node_t *v;
  edge_t *e, *f;
  int    i;
  float  x=0, y=0, x1, y1, x2, y2, r;
  port_t port, arr_port;
  int    sflag, eflag, ht2;

  /* Compute the direction vector (x,y) of the average direction. We compute
     with direction vectors instead of angles because else we have to first
     bring the angles within PI of each other. av(a,b)!=av(a,b+2*PI) */
  for (i=0; i < l->size; i++) {
    e = l->list[i];
    if (e->head==u) v=e->tail; else v=e->head;
    x1 = v->u.coord.x - u->u.coord.x;
    y1 = v->u.coord.y - u->u.coord.y;
    r = hypot(x1,y1);
    x += x1 / r;
    y += y1 / r;
  }
  r = hypot(x,y);
  x /= r;
  y /= r;

  /* (x1,y1),(x2,y2) is a segment that must cross the node boundary */
  x1 = u->u.coord.x; y1 = u->u.coord.y; /* center of node */
  r  = MAX (u->u.lw+u->u.rw, u->u.ht);  /* far away */
  x2 = x*r + u->u.coord.x; y2 = y*r + u->u.coord.y;
  { /* now move (x1,y1) to the node boundary */
    point curve[4];	/* bezier control points for a straight line */
    curve[0].x = ROUND(x1);			 curve[0].y = ROUND(y1);
    curve[1].x = ROUND((2*x1+x2)/3); curve[1].y = ROUND((2*y1+y2)/3);
    curve[2].x = ROUND((2*x2+x1)/3); curve[2].y = ROUND((2*y2+y1)/3);
    curve[3].x = ROUND(x2);			 curve[3].y = ROUND(y2);

    shape_clip (u, curve, l->list[0]);
    x1 = curve[0].x - u->u.coord.x;
    y1 = curve[0].y - u->u.coord.y;
  }

  /* compute PORT on the boundary */
  port.p.x = ROUND(x1);
  port.p.y = ROUND(y1);
  port.order = (MC_SCALE * (u->u.lw + port.p.x)) / (u->u.lw + u->u.rw);
  port.constrained = FALSE;
  port.defined = TRUE;

  /* compute ARR_PORT at a distance ARR_LEN away from the boundary */
  if ((arr_port.defined = arr_len && TRUE)) {
    arr_port.p.x = ROUND(x1 + x * arr_len);
    arr_port.p.y = ROUND(y1 + y * arr_len);
    arr_port.constrained = FALSE;
    arr_port.order = (MC_SCALE * (u->u.lw + arr_port.p.x)) / (u->u.lw + u->u.rw);
    if (u->graph->u.rank[u->u.rank].ht2 < (ht2 = ABS(arr_port.p.y)))
      /* adjust ht2 so that splines.c uses feasible boxes.
       FIXME: I guess this adds an extra box for all edges in the rank */
      u->graph->u.rank[u->u.rank].ht2 = ht2;
  }

  /* assign one of the ports to every edge */
  for (i=0; i < l->size; i++) {
    e = l->list[i];
    arrow_flags (e, &sflag, &eflag);
#ifndef OLD
    for ( ; e; e=e->u.to_virt) { /* assign to all virt edges of e */
        for (f=e; f;
             f = f->u.edge_type==VIRTUAL &&
                 f->head->u.node_type==VIRTUAL &&
                 f->head->u.out.size==1 ?
                 f->head->u.out.list[0] : NULL)
            {
                if (f->head==u) f->u.head_port = port;
                if (f->tail==u) f->u.tail_port = port;
            }
        for (f=e; f;
             f = f->u.edge_type==VIRTUAL &&
                 f->tail->u.node_type==VIRTUAL &&
                 f->tail->u.in.size==1 ?
                 f->tail->u.in.list[0] : NULL)
            {
                if (f->head==u) f->u.head_port = port;
                if (f->tail==u) f->u.tail_port = port;
            }
    }
#else
    for ( ; e; e=e->u.to_virt) { /* assign to all virt edges of e */
      if (e->head==u) e->u.head_port =
                        arr_port.defined && !eflag ? arr_port : port;
      if (e->tail==u) e->u.tail_port =
                        arr_port.defined && !sflag ? arr_port : port;
    }
#endif
  }

  u->u.has_port = TRUE; /* kinda pointless, because mincross is already done */
}

/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/

#include	"dot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

node_t	*
make_vn_slot(graph_t *g, int r, int pos)
{
	int		i;
	node_t	**v,*n;

	v = g->u.rank[r].v = ALLOC(g->u.rank[r].n+2,g->u.rank[r].v,node_t*);
	for (i = g->u.rank[r].n; i > pos; i--) {
		v[i] = v[i-1];
		v[i]->u.order++;
	}
	n = v[pos] = virtual_node(g);
	n->u.order = pos;	n->u.rank = r;
	v[++(g->u.rank[r].n)] = NULL;
	return v[pos];
}

#define 	HLB 	0		/* hard left bound */
#define		HRB		1		/* hard right bound */
#define		SLB		2		/* soft left bound */
#define		SRB		3		/* soft right bound */

void findlr(node_t *u, node_t *v, int *lp, int *rp)
{
	int		l,r;
	l = u->u.order; r = v->u.order;
	if (l > r) {int t = l; l = r; r = t;}
	*lp = l; *rp = r;
}

void setbounds(node_t *v, int *bounds,int lpos, int rpos)
{
	int		i, l,r,ord;
	edge_t	*f;

	if (v->u.node_type == VIRTUAL) {
		ord = v->u.order;
		if (v->u.in.size == 0) {	/* flat */
			assert(v->u.out.size == 2);
			findlr(v->u.out.list[0]->head,v->u.out.list[1]->head,&l,&r);
				/* the other flat edge could be to the left or right */
			if (r <= lpos) bounds[SLB] = bounds[HLB] = ord;
			else if (l >= rpos) bounds[SRB] = bounds[HRB] = ord;
				/* could be spanning this one */
			else if ((l < lpos) && (r > rpos)) ; /* ignore */
				/* must have intersecting ranges */
			else {
				if ((l < lpos) || ((l == lpos) && (r < rpos)))
					bounds[SLB] = ord;
				if ((r > rpos) || ((r == rpos) && (l > lpos)))
					bounds[SRB] = ord;
			}
		}
		else {						/* forward */
			boolean		onleft,onright;
			onleft = onright = FALSE;
			for (i = 0; (f = v->u.out.list[i]); i++) {
				if (f->head->u.order <= lpos) {onleft = TRUE; continue;}
				if (f->head->u.order >= rpos) {onright = TRUE; continue;}
			}
			if (onleft && (onright == FALSE)) bounds[HLB] = ord + 1;
			if (onright && (onleft == FALSE)) bounds[HRB] = ord - 1;
		}
	}
}

int flat_limits(graph_t* g, edge_t* e)
{
	int			lnode,rnode,r,bounds[4],lpos,rpos,pos;
	node_t		**rank;

	r = e->tail->u.rank - 1;
	rank = g->u.rank[r].v;
	lnode = 0;
	rnode = g->u.rank[r].n - 1;
	bounds[HLB] = bounds[SLB] = lnode - 1;
	bounds[HRB] = bounds[SRB] = rnode + 1;
	findlr(e->tail,e->head,&lpos,&rpos);
	while (lnode <= rnode) {
		setbounds(rank[lnode],bounds,lpos,rpos);
		if (lnode != rnode)
			setbounds(rank[rnode],bounds,lpos,rpos);
		lnode++; rnode--;
		if (bounds[HRB] - bounds[HLB] <= 1) break;
	}
	if (bounds[HLB] <= bounds[HRB])
		pos = (bounds[HLB] + bounds[HRB] + 1) / 2;
	else
		pos = (bounds[SLB] + bounds[SRB] + 1) / 2;
	return pos;
}

void flat_node(edge_t* e)
{
	int		r,place,ypos,h2;
	graph_t	*g;
	node_t	*n,*vn;
	edge_t	*ve;
	pointf	dimen;

	if (e->u.label == NULL) return;
	g = e->tail->graph;
	r = e->tail->u.rank;

	place = flat_limits(g,e);
	n = g->u.rank[r - 1].v[0];
		/* grab ypos before make_vn_slot() */
	if (n) ypos = g->u.rank[r - 1].v[0]->u.coord.y;
	else ypos = g->u.rank[r].v[0]->u.coord.y + g->u.rank[r].ht2 /*+ vn->u.ht/2 */; 
	vn = make_vn_slot(g,r-1,place);	
	dimen = e->u.label->dimen;
	if (g->u.left_to_right) {double f = dimen.x; dimen.x = dimen.y; dimen.y = f;}
	vn->u.ht = POINTS(dimen.y); h2 = vn->u.ht / 2;
	vn->u.lw = vn->u.rw = POINTS(dimen.x)/2;
	vn->u.label = e->u.label;
	vn->u.coord.y = (n) ? ypos : ypos + h2;
	ve = virtual_edge(vn,e->tail,e);	/* was NULL? */
		ve->u.tail_port.p.x = -vn->u.lw;
		ve->u.head_port.p.x = e->tail->u.rw;
		ve->u.edge_type = FLATORDER;
	ve = virtual_edge(vn,e->head,e);
		ve->u.tail_port.p.x = vn->u.rw;
		ve->u.head_port.p.x = e->head->u.lw;
		ve->u.edge_type = FLATORDER;
	if (g->u.rank[r-1].ht2 < h2) g->u.rank[r-1].ht2 = h2;
}

void flat_edges(graph_t* g)
{
	int		i,j,reset = FALSE;
	node_t	*n;
	edge_t	*e;

	if ((g->u.rank[0].flat) || (g->u.n_cluster > 0)) {
		for (i = 0; (n = g->u.rank[0].v[i]); i++) {
			for (j = 0; (e = n->u.flat_in.list[j]); j++) {
				if (e->u.label) {abomination(g); break;}
			}
			if (e) break;
		}
	}
			
	rec_save_vlists(g);
	for (n = g->u.nlist; n; n = n->u.next) {
		if (n->u.flat_out.list) for (i = 0; (e = n->u.flat_out.list[i]); i++) {
			reset = TRUE;
			flat_node(e);
		}
	}
	if (reset) rec_reset_vlists(g);
}

void abomination(graph_t* g)
{
	int		r;
	rank_t	*rptr;

	assert(g->u.minrank == 0);
		/* 3 = one for new rank, one for sentinel, one for off-by-one */
	r = g->u.maxrank + 3;
	rptr = ALLOC(r,g->u.rank,rank_t);
	g->u.rank = rptr + 1;
	for (r = g->u.maxrank; r >= 0; r--)
		g->u.rank[r] = g->u.rank[r-1];
	g->u.rank[r].n = g->u.rank[0].an = 0;
	g->u.rank[r].v = g->u.rank[0].av = N_NEW(2,node_t*);
	g->u.rank[r].flat = NULL;
	g->u.rank[r].ht2 = 1;
	g->u.minrank--;
}

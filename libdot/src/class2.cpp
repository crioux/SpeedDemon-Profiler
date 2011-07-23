/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/

/* classify edges for mincross/nodepos/splines, using given ranks */

#include "dot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif


void class2(graph_t* g)
{
	int		c;
	node_t	*n,*t,*h;
	edge_t	*e,*prev,*opp;

	g->u.nlist = NULL;

	g->u.n_nodes = 0;	/* new */

	mark_clusters(g);
	for (c = 1; c <= g->u.n_cluster; c++)
		build_skeleton(g,g->u.clust[c]);
	for (n = agfstnode(g); n; n = agnxtnode(g,n))
		for (e = agfstout(g,n); e; e = agnxtout(g,e)) {
			if (e->head->u.weight_class <= 2) e->head->u.weight_class++;
			if (e->tail->u.weight_class <= 2) e->tail->u.weight_class++;
	}

	for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
		if ((n->u.clust == NULL) && (n == UF_find(n))) {fast_node(g,n); g->u.n_nodes++;}
		prev = NULL;
		for (e = agfstout(g,n); e; e = agnxtout(g,e)) {

				/* already processed */
			if (e->u.to_virt) continue;

				/* edges involving sub-clusters of g */
			if (is_cluster_edge(e)) {
				/* following is new cluster multi-edge code */
				if (mergeable(prev,e)) {
					if (prev->u.to_virt) { 
						merge_chain(g,e,prev->u.to_virt,FALSE);
						other_edge(e);
					}
					else if (e->tail->u.rank == e->head->u.rank) {
						merge_oneway(e,prev);
						other_edge(e);
					}
					/* else is an intra-cluster edge */
					continue;
				}
				interclrep(g,e);
				prev = e;
				continue;
			}
				/* merge multi-edges */
			if (prev && (e->tail == prev->tail) && (e->head == prev->head)) {
				if (e->tail->u.rank == e->head->u.rank) {
					merge_oneway(e,prev);
					other_edge(e);
					continue;
				}
				if ((e->u.label == NULL) && (prev->u.label == NULL) && ports_eq(e,prev)) {
					if (Concentrate) 
						e->u.edge_type = IGNORED;
					else{
					merge_chain(g,e,prev->u.to_virt,TRUE);
					other_edge(e);
					}
					continue;
				}
				/* parallel edges with different labels fall through here */
			}

				/* self edges */
			if (e->tail == e->head) {
				other_edge(e);
				prev = e;
				continue;
			}

			t = UF_find(e->tail);
			h = UF_find(e->head);

				/* non-leader leaf nodes */
			if ((e->tail != t) || (e->head != h)) {
					/* ### need to merge stuff */
				continue;	
			}


				/* flat edges */
			if (e->tail->u.rank == e->head->u.rank) {
				flat_edge(g,e);
				prev = e;
				continue;
			}

				/* forward edges */
			if (e->head->u.rank > e->tail->u.rank) {
				make_chain(g,e->tail,e->head,e);
				prev = e;
				continue;
			}

				/* backward edges */
			else {
				/*other_edge(e);*/
				if ((opp = agfindedge(g,e->head,e->tail))) {
					/* shadows a forward edge */
					if (opp->u.to_virt == NULL)
						make_chain(g,opp->tail,opp->head,opp);
					if ((e->u.label == NULL) && (opp->u.label == NULL) && ports_eq(e,opp)) {
						if (Concentrate) {
							e->u.edge_type = IGNORED;
							opp->u.conc_opp_flag = TRUE;
						}
						else{	/* see above.  this is getting out of hand */
						other_edge(e);
						merge_chain(g,e,opp->u.to_virt,TRUE);
						}
						continue;
					}
				}
				make_chain(g,e->head,e->tail,e);
				prev = e;
			}
		}
	}
	/* since decompose() is not called on subgraphs */
	if (g != g->root) {
		g->u.comp.list = ALLOC(1,g->u.comp.list,node_t*);
		g->u.comp.list[0] = g->u.nlist;
	}
}

node_t	*
label_vnode(graph_t* g, edge_t* orig)
{
	node_t		*v;
	pointf		dimen;

	dimen = orig->u.label->dimen;
	v = virtual_node(g);
	v->u.label = orig->u.label;
	v->u.lw = v->graph->u.nodesep;
	if (g->u.left_to_right) {
		v->u.ht = POINTS(dimen.x); v->u.rw = POINTS(dimen.y);
	}
	else {
		v->u.ht = POINTS(dimen.y); v->u.rw = POINTS(dimen.x);
	}
	return v;
}

node_t	*
plain_vnode(graph_t* g, edge_t* orig)
{
	node_t		*v;
	orig = orig;
	v = virtual_node(g);
	incr_width(g,v);
	return v;
}

void incr_width(graph_t* g, node_t* v)
{
	int width = g->u.nodesep / 2;
	v->u.lw += width;
	v->u.rw += width;
}

void make_chain(graph_t *g, node_t *from, node_t *to, edge_t *orig)
{
	int		r,label_rank;
	node_t	*u,*v;
	edge_t	*e;

	u = from;
	if (orig->u.label) label_rank = (from->u.rank + to->u.rank) / 2;
	else label_rank = -1;
	assert(orig->u.to_virt == NULL);
	for (r = from->u.rank + 1; r <= to->u.rank; r++) {
		if (r < to->u.rank) {
			if (r == label_rank) v = label_vnode(g,orig);
			else v = plain_vnode(g,orig);
			v->u.rank = r;
		}
		else v = to;
		e = virtual_edge(u,v,orig);
		virtual_weight(e);
		u = v;
	}
	assert(orig->u.to_virt != NULL);
}

void merge_chain(graph_t *g, edge_t	*e, edge_t *f, int flag)
{
	edge_t		*rep;
	int			lastrank = MAX(e->tail->u.rank,e->head->u.rank);

	assert(e->u.to_virt == NULL);
	e->u.to_virt = f;
	rep = f;
	do {
			/* interclust multi-edges are not counted now */
		if (flag) rep->u.count += e->u.count;
		rep->u.xpenalty += e->u.xpenalty;
		rep->u.weight += e->u.weight;
		if (rep->head->u.rank == lastrank) break;
		incr_width(g,rep->head);
		rep = rep->head->u.out.list[0];
	} while (rep);
}

node_t	*
leader_of(graph_t* g, node_t* v)
{
	graph_t	*clust;
	node_t	*rv;

	if (v->u.ranktype != CLUSTER) {
		/*assert(v == UF_find(v));  could be leaf, so comment out */
		rv = UF_find(v);
	}
	else {
		clust = v->u.clust;
		rv = clust->u.rankleader[v->u.rank];
	}
	return rv;
}

void interclrep(graph_t* g, edge_t* e)
{
	node_t		*t,*h;
	edge_t		*ve;

	t = leader_of(g,e->tail);
	h = leader_of(g,e->head);
	if (t->u.rank > h->u.rank) {node_t *t0 = t; t = h; h = t0;}
	if (t->u.clust != h->u.clust) {
		if ((ve = find_fast_edge(t,h))) {
			merge_chain(g,e,ve,TRUE);
			return;
		}
		if (t->u.rank == h->u.rank) return;
		make_chain(g,t,h,e);

		/* mark as cluster edge */
		for (ve = e->u.to_virt; ve && (ve->head->u.rank <= h->u.rank);
			ve = ve->head->u.out.list[0]) ve->u.edge_type = CLUSTER_EDGE;
	}
	/* else ignore intra-cluster edges at this point */
}

int is_cluster_edge(edge_t* e)
{
	return ((e->tail->u.ranktype==CLUSTER) || (e->head->u.ranktype==CLUSTER));
}

int mergeable(edge_t *e, edge_t *f)
{
	if (e && f && (e->tail == f->tail) && (e->head == f->head) &&
		(e->u.label == f->u.label) && ports_eq(e,f))
			return TRUE;
	return FALSE;
}

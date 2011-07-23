/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/
/*
 * Classify edges for rank assignment phase to
 * create temporary edges.
 */

#include "dot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

int nonconstraint_edge(edge_t* e)
{
	char	*constr;

	if (E_constr && (constr = agxget(e,E_constr->index))) {
		if (constr[0] && mapbool(constr) == FALSE) return TRUE;
	}
	return FALSE;
}

void class1(graph_t* g)
{
	node_t	*n,*t,*h;
	edge_t	*e,*rep;

	mark_clusters(g);
	for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
		for (e = agfstout(g,n); e; e = agnxtout(g,e)) {

				/* skip edges already processed */
			if (e->u.to_virt) continue;

				/* skip edges that we want to ignore in this phase */
			if (nonconstraint_edge(e)) continue;

			t = UF_find(e->tail);
			h = UF_find(e->head);

				/* skip self, flat, and intra-cluster edges */
			if (t == h) continue;


				/* inter-cluster edges require special treatment */
			if (t->u.clust || h->u.clust) {
				interclust1(g,e->tail,e->head,e);
				continue;
			}

			if ((rep = find_fast_edge(t,h))) merge_oneway(e,rep);
			else virtual_edge(t,h,e);

#ifdef NOTDEF
			if ((t == e->tail) && (h == e->head)) {
			if (rep = find_fast_edge(t,h)) merge_oneway(e,rep);
			else virtual_edge(t,h,e);
			}
			else {
				f = agfindedge(g,t,h);
				if (f && (f->u.to_virt == NULL)) rep = virtual_edge(t,h,f);
				else rep = find_fast_edge(t,h);
				if (rep) merge_oneway(e,rep);
				else virtual_edge(t,h,e);
			}
#endif
		}
	}
}

void interclust1(graph_t *g, node_t *t, node_t *h, edge_t *e)
{
	node_t		*v,*t0,*h0;
	int			offset,t_len,h_len,t_rank,h_rank;
	edge_t		*rt,*rh;

	if (e->tail->u.clust)
		t_rank = e->tail->u.rank - e->tail->u.clust->u.leader->u.rank;
	else t_rank = 0;
	if (e->head->u.clust)
		h_rank = e->head->u.rank - e->head->u.clust->u.leader->u.rank;
	else h_rank = 0;
	offset = e->u.minlen + t_rank - h_rank;
	if (offset > 0) {t_len = 0; h_len = offset;}
	else {t_len = -offset; h_len = 0;}

	v = virtual_node(g);
	v->u.node_type = SLACKNODE;
	t0 = UF_find(t); h0 = UF_find(h);
	rt = make_aux_edge(v,t0,t_len,CL_BACK*e->u.weight);
	rh = make_aux_edge(v,h0,h_len,e->u.weight);
	rt->u.to_orig = rh->u.to_orig = e;
}

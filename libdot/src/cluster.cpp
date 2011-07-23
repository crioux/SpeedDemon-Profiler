/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/
#include "dot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* delete virtual nodes of a cluster, and install real nodes or sub-clusters */
void expand_cluster(graph_t* subg)
{
	/* build internal structure of the cluster */
	class2(subg);
		subg->u.comp.size = 1;
		subg->u.comp.list[0] = subg->u.nlist;
	allocate_ranks(subg);
	build_ranks(subg,0);
	merge_ranks(subg);

	/* build external structure of the cluster */
	interclexp(subg);
	remove_rankleaders(subg);
}

void mark_clusters(graph_t* g)
{
	int			c;
	node_t		*n,*vn;
	edge_t		*orig,*e;
	graph_t		*clust;

	/* remove sub-clusters below this level */
	for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
		if (n->u.ranktype == CLUSTER) UF_singleton(n);
		n->u.clust = NULL;
	}

	for (c = 1; c <= g->u.n_cluster; c++) {
		clust= g->u.clust[c];
		for (n = agfstnode(clust); n; n = agnxtnode(clust,n)) {
			if (n->u.ranktype != NORMAL) {
				fprintf(stderr,"warning: %s was already in a rankset, ignored in cluster %s\n",n->name,g->name);
				continue;
			}
			UF_setname(n,clust->u.leader);
			n->u.clust = clust;
			n->u.ranktype = CLUSTER;

			/* here we mark the vnodes of edges in the cluster */
			for (orig = agfstout(g,n); orig; orig = agnxtout(g,orig)) {
				if ((e = orig->u.to_virt)) {
					while ((vn = e->head)->u.node_type == VIRTUAL)  {
						vn->u.clust = g;
						e = e->head->u.out.list[0];
						/* trouble if concentrators and clusters are mixed */
					}
				}
			}
		}
	}
}

void build_skeleton(graph_t *g, graph_t *subg)
{
	int			r;
	node_t		*v,*prev,*rl;
	edge_t		*e;

	prev = NULL;
	subg->u.rankleader = N_NEW(subg->u.maxrank + 2,node_t*);
	for (r = subg->u.minrank; r <= subg->u.maxrank; r++) {
		v = subg->u.rankleader[r] = virtual_node(g);
		v->u.rank = r;
		v->u.ranktype = CLUSTER;
		v->u.clust = subg;
		if (prev) {
			e = virtual_edge(prev,v,NULL);
			e->u.xpenalty *= CL_CROSS;
		}
		prev = v;
	}

	/* set the counts on virtual edges of the cluster skeleton */
	for (v = agfstnode(subg); v; v = agnxtnode(subg,v)) {
		rl = subg->u.rankleader[v->u.rank];
		rl->u.UF_size++;
		for (e = agfstout(subg,v); e; e = agnxtout(subg,e)) {
			for (r = e->tail->u.rank; r < e->head->u.rank; r++) {
				rl->u.out.list[0]->u.count++;
			}
		}
	}
	for (r = subg->u.minrank; r <= subg->u.maxrank; r++) {
		rl = subg->u.rankleader[r];
		if (rl->u.UF_size > 1) rl->u.UF_size--;
	}
}

void merge_ranks(graph_t* subg)
{
	int		i,d,r,pos,ipos;
	node_t	*v;
	graph_t	*root;

	root = subg->root;
	if (subg->u.minrank > 0) 
		root->u.rank[subg->u.minrank-1].valid = FALSE;
	for (r = subg->u.minrank; r <= subg->u.maxrank; r++) {
		d = subg->u.rank[r].n;
		ipos = pos = subg->u.rankleader[r]->u.order;
		make_slots(root,r,pos,d);
		for (i = 0; i < subg->u.rank[r].n; i++) {
			v = root->u.rank[r].v[pos] = subg->u.rank[r].v[i];
			v->u.order = pos++;
			v->graph = subg->root;
			delete_fast_node(subg,v);
			fast_node(subg->root,v);
			subg->root->u.n_nodes++;
		}
		subg->u.rank[r].v = root->u.rank[r].v + ipos;
		root->u.rank[r].valid = FALSE;
	}
	if (r < root->u.maxrank) root->u.rank[r].valid = FALSE;
	subg->u.expanded = TRUE;
}

/* make d slots starting at position pos (where 1 already exists) */
void make_slots(graph_t *root, int r, int pos, int d)
{
	int		i;
	node_t	*v,**vlist;
	vlist = root->u.rank[r].v;
	if (d <= 0) {
		for (i = pos - d + 1; i < root->u.rank[r].n; i++) {
			v = vlist[i];
			v->u.order = i + d - 1;
			vlist[v->u.order] = v;
		}
		for (i = root->u.rank[r].n + d - 1; i < root->u.rank[r].n; i++)
			vlist[i] = NULL;
	}
	else {
/*assert(root->u.rank[r].n + d - 1 <= root->u.rank[r].an);*/
		for (i = root->u.rank[r].n - 1; i > pos; i--) {
			v = vlist[i];
			v->u.order = i + d - 1;
			vlist[v->u.order] = v;
		}
		for (i = pos + 1; i < pos + d; i++) vlist[i] = NULL;
	}
	root->u.rank[r].n += d - 1;
}

/* 
 * attach and install edges between clusters.
 * essentially, class2() for interclust edges.
 */
void interclexp(graph_t* subg)
{
	graph_t		*g;
	node_t		*n;
	edge_t		*e,*prev;

	g = subg->root;
	for (n = agfstnode(subg); n; n = agnxtnode(subg,n)) {

		/* N.B. n may be in a sub-cluster of subg */
		prev = NULL;
		for (e = agfstedge(subg->root,n); e; e = agnxtedge(subg->root,e,n)) {
			if (agcontains(subg,e)) continue;

			/* short/flat multi edges */
			if (mergeable(prev,e)) {
					if (e->tail->u.rank == e->head->u.rank) e->u.to_virt = prev;
					else e->u.to_virt = NULL;
					if (prev->u.to_virt == NULL) continue;	/* internal edge */
					merge_chain(subg,e,prev->u.to_virt,FALSE);
					safe_other_edge(e);
					continue;
			}

				/* flat edges */
			if (e->tail->u.rank == e->head->u.rank) {
				if (find_flat_edge(e->tail,e->head) == NULL) {
					flat_edge(g,e); prev = e;
				}
				else prev = NULL;
				continue;
			}

			assert (e->u.to_virt != NULL);

				/* forward edges */
			if (e->head->u.rank > e->tail->u.rank) {
				make_interclust_chain(g,e->tail,e->head,e);
				prev = e;
				continue;
			}

				/* backward edges */
			else {
/*
I think that make_interclust_chain should create call other_edge(e) anyway 
				if (agcontains(subg,e->tail)
					&& agfindedge(subg->root,e->head,e->tail)) other_edge(e);
*/
				make_interclust_chain(g,e->head,e->tail,e);
				prev = e;
			}
		}
	}
}

node_t*
map_interclust_node(node_t* n)
{
	node_t		*rv;

	if ((n->u.clust == NULL) || (n->u.clust->u.expanded)) rv = n;
	else rv = n->u.clust->u.rankleader[n->u.rank];
	return rv;
}

void make_interclust_chain(graph_t *g, node_t *from, node_t *to, edge_t *orig)
{
	int			newtype;
	node_t		*u,*v;

	u = map_interclust_node(from);
	v = map_interclust_node(to);
	if ((u == from) && (v == to)) newtype = VIRTUAL;
	else newtype = CLUSTER_EDGE;
	map_path(u,v,orig,orig->u.to_virt,newtype);
}

node_t	*
clone_vn(graph_t* g, node_t* vn)
{
	node_t	*rv;
	int		r;

	r = vn->u.rank;
	make_slots(g,r,vn->u.order,2);
	rv = virtual_node(g);
	rv->u.lw = vn->u.lw;
	rv->u.rw = vn->u.rw;
	rv->u.rank = vn->u.rank;
	rv->u.order = vn->u.order + 1;
	g->u.rank[r].v[rv->u.order] = rv;
	return rv;
}

void map_path(node_t *from, node_t *to, edge_t *orig, edge_t *ve, int type)
{
	int			r;
	node_t		*u,*v;
	edge_t		*e;

	assert(from->u.rank < to->u.rank);

	if ((ve->tail == from) && (ve->head == to)) return;

	if (ve->u.count > 1)  {
		orig->u.to_virt = NULL;
		if (to->u.rank - from->u.rank == 1) {
			if ((e = find_fast_edge(from,to)) && (ports_eq(orig,e))) {
				merge_oneway(orig,e);
				if ((from->u.node_type == NORMAL)
					&& (to->u.node_type == NORMAL))
						other_edge(orig);
				return;
			}
		}
		u = from;
		for (r = from->u.rank; r < to->u.rank; r++) {
			if (r < to->u.rank - 1) v = clone_vn(from->graph,ve->head);
			else v = to;
			e = virtual_edge(u,v,orig);
			e->u.edge_type = type;
			u = v;
			ve->u.count--;
			ve = ve->head->u.out.list[0];
		}
	}
	else {
		if (to->u.rank - from->u.rank == 1) {
			if ((ve = find_fast_edge(from,to)) && (ports_eq(orig,ve))) {
				/*ve->u.to_orig = orig;*/
				orig->u.to_virt = ve;
				ve->u.edge_type = type;
				ve->u.count++;
				if ((from->u.node_type == NORMAL)
					&& (to->u.node_type == NORMAL))
						other_edge(orig);
			}
			else {
				orig->u.to_virt = NULL;
				ve = virtual_edge(from,to,orig);
				ve->u.edge_type = type;
			}
		}
		if (to->u.rank - from->u.rank > 1) {
			e = ve;
			if (ve->tail != from) {
				orig->u.to_virt = NULL;
				e = orig->u.to_virt = virtual_edge(from,ve->head,orig);
				delete_fast_edge(ve);
			}
			else e = ve;
			while (e->head->u.rank != to->u.rank) e = e->head->u.out.list[0];
			if (e->head != to) {
				ve = e;
				e = virtual_edge(e->tail,to,orig);
				e->u.edge_type = type;
				delete_fast_edge(ve);
			}
		}
	}
}

void install_cluster(graph_t* g, node_t* n, int pass, queue* q)
{
	int			r;
	graph_t		*clust;

	clust = n->u.clust;
	if (clust->u.installed != pass + 1) {
		for (r = clust->u.minrank; r <= clust->u.maxrank; r++)
			install_in_rank(g,clust->u.rankleader[r]);
		for (r = clust->u.minrank; r <= clust->u.maxrank; r++)
			enqueue_neighbors(q,clust->u.rankleader[r],pass);
		clust->u.installed = pass + 1;
	}
}

void remove_rankleaders(graph_t* g)
{
	int			r;
	node_t		*v;
	edge_t		*e;

	for (r = g->u.minrank; r <= g->u.maxrank; r++) {
		v = g->u.rankleader[r];

		/* remove the entire chain */
		while ((e = v->u.out.list[0])) delete_fast_edge(e);
		while ((e = v->u.in.list[0])) delete_fast_edge(e);
		delete_fast_node(g->root,v);
		g->u.rankleader[r] = NULL;
	}
}

void mark_lowclusters(graph_t* g)
{
	node_t	*n;
	graph_t	*clust;
	int		c;

	for (n = agfstnode(g); n; n = agnxtnode(g,n)) n->u.clust = g;
	for (c = 1; c <= g->u.n_cluster; c++) {
		clust = g->u.clust[c];
		mark_lowclusters(clust);
	}
}

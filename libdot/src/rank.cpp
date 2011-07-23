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
 * Rank the nodes of a directed graph, subject to user-defined
 * sets of nodes to be kept on the same, min, or max rank.
 * The temporary acyclic fast graph is constructed and ranked
 * by a network-simplex technique.  Then ranks are propagated
 * to non-leader nodes and temporary edges are deleted.
 * Leaf nodes and top-level clusters are left collapsed, though.
 * Assigns global minrank and maxrank of graph and all clusters.
 *
 * TODO: safety code.  must not be in two clusters at same level.
 *  must not be in same/min/max/rank and a cluster at the same time.
 *  watch out for interactions between leaves and clusters.
 */

#include	"dot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

void dot_rank(graph_t* g)
{
	edgelabel_ranks(g);
	collapse_sets(g);
	/*collapse_leaves(g);*/
	class1(g);
	minmax_edges(g);
	decompose(g,0);
	acyclic(g);
	rank1(g);
	expand_ranksets(g);
	cleanup1(g);
}

/* When there are edge labels, extra ranks are reserved here for the virtual
 * nodes of the labels.  This is done by doubling the input edge lengths.
 * The input rank separation is adjusted to compensate.
 */
void edgelabel_ranks(graph_t* g)
{
	node_t		*n;
	edge_t		*e;

	if (g->u.has_edge_labels) {
		for (n = agfstnode(g); n; n = agnxtnode(g,n))
			for (e = agfstout(g,n); e; e = agnxtout(g,e))
				e->u.minlen *= 2;
		g->u.ranksep = (g->u.ranksep  + 1) / 2;
	}
}

/* Run the network simplex algorithm on each component. */
void rank1(graph_t* g)
{
	int			maxiter = MAXINT;
	int			c;
	char		*s;

	if ((s = agget(g,"nslimit1")))
		maxiter = (int)(atof(s) * agnnodes(g));
	for (c = 0; c < g->u.comp.size; c++) {
		g->u.nlist = g->u.comp.list[c];
		rank(g,TRUE,maxiter);
	}
}

int is_cluster(graph_t* g)
{
	return (strncmp(g->name,"cluster",7) == 0);
}

int rank_set_class(graph_t* g)
{
	static char	*name[] = {"same","min","source","max","sink",NULL};
	static int	_class[] = {SAMERANK,MINRANK,SOURCERANK,MAXRANK,SINKRANK,0};
	int		val;

	if (is_cluster(g)) return CLUSTER;
	val = maptoken(agget(g,"rank"),name,_class);
	g->u.set_type = val;
	return val;
}

/* Execute union commands for "same rank" subgraphs and clusters. */
void collapse_sets(graph_t* g)
{
	int			c;
	graph_t		*mg,*subg;
	node_t		*mn,*n;
	edge_t		*me;

	mg = g->meta_node->graph;
	for (me = agfstout(mg,g->meta_node); me; me = agnxtout(mg,me)) {
		mn = me->head;
		subg = agusergraph(mn);

		c = rank_set_class(subg);
		if (c) {
			if ((c == CLUSTER) && CL_type == LOCAL) collapse_cluster(g,subg);
			else collapse_rankset(g,subg,c);
		}

		/* mark nodes with ordered edges so their leaves are not collapsed */
		if (agget(subg,"ordering"))
			for (n = agfstnode(subg); n; n = agnxtnode(subg,n)) n->u.order = 1;
	}
}

/* Merge the nodes of a min, max, or same rank set. */
void collapse_rankset(graph_t *g, graph_t *subg, int kind)
{
	node_t	*u,*v;

	u = v = agfstnode(subg);
	if (u) {
		u->u.ranktype = kind;
		while ((v = agnxtnode(subg,v))) {
			UF_union(u,v);
			v->u.ranktype = u->u.ranktype;
		}
		switch (kind) {
		case MINRANK: case SOURCERANK:
			if (g->u.minset == NULL) g->u.minset = u;
			else g->u.minset = UF_union(g->u.minset,u);
			break;
		case MAXRANK: case SINKRANK:
			if (g->u.maxset == NULL) g->u.maxset = u;
			else g->u.maxset = UF_union(g->u.maxset,u);
			break;
		}
		switch (kind) {
			case SOURCERANK: g->u.minset->u.ranktype = kind; break;
			case SINKRANK: g->u.maxset->u.ranktype = kind; break;
		}
	}
}

node_t* merge_leaves(graph_t *g, node_t *cur, node_t *_new)
{
	node_t	*rv;

	if (cur == NULL) rv = _new;
	else {
		rv = UF_union(cur,_new);
		rv->u.ht = MAX(cur->u.ht,_new->u.ht);
		rv->u.lw = cur->u.lw + _new->u.lw + g->u.nodesep/2;
		rv->u.rw = cur->u.rw + _new->u.rw + g->u.nodesep/2;
	}
	return rv;
}

void potential_leaf(graph_t* g, edge_t* e, node_t* leaf)
{
	node_t		*par;

	if ((e->u.tail_port.p.x) || (e->u.head_port.p.x)) return;
	if ((e->u.minlen != 1) || (e->tail->u.order > 0)) return;
	par = ((leaf != e->head)? e->head : e->tail);
	leaf->u.ranktype = LEAFSET;
	if (par == e->tail) par->u.outleaf = merge_leaves(g,par->u.outleaf,leaf);
	else par->u.inleaf = merge_leaves(g,par->u.inleaf,leaf);
}

void collapse_leaves(graph_t* g)
{
	node_t		*n;
	edge_t		*e;

	for (n = agfstnode(g); n; n = agnxtnode(g,n)) {

		/* consider n as a potential leaf of some other node. */
		if ((n->u.ranktype != NOCMD) || (n->u.order)) continue;
		if (agfstout(g,n) == NULL) {
			if ((e = agfstin(g,n)) && (agnxtin(g,e) == NULL)) {
				potential_leaf(g,e,n);
				continue;
			}
		}
		if (agfstin(g,n) == NULL) {
			if ((e = agfstout(g,n)) && (agnxtout(g,e) == NULL)) {
				potential_leaf(g,e,n);
				continue;
			}
		}
	}
}

/* To ensure that min and max rank nodes always have the intended rank
 * assignment, reverse any incompatible edges.
 */
void minmax_edges(graph_t* g)
{
	node_t	*n;
	edge_t	*e;
	int		srclen,sinklen;

	srclen = sinklen = 0;
	if ((g->u.maxset == NULL) && (g->u.minset == NULL)) return;
	if ( g->u.minset != NULL ) g->u.minset = UF_find(g->u.minset);
	if ( g->u.maxset != NULL ) g->u.maxset = UF_find(g->u.maxset);

	if ((n = g->u.maxset)) {
		sinklen = (g->u.maxset->u.ranktype == SINKRANK);
		while ((e = n->u.out.list[0])) {
			assert(e->head == UF_find(e->head));
			reverse_edge(e);
		}
	}
	if ((n = g->u.minset)) {
		srclen = (g->u.minset->u.ranktype == SOURCERANK);
		while ((e = n->u.in.list[0])) {
			assert(e->tail == UF_find(e->tail));
			reverse_edge(e);
		}
	}

	for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
		if (n != UF_find(n)) continue;
		if ((n->u.out.size == 0) && g->u.maxset && (n != g->u.maxset))
			virtual_edge(n,g->u.maxset,NULL)->u.minlen = sinklen;
		if ((n->u.in.size == 0) && g->u.minset && (n != g->u.minset))
			virtual_edge(g->u.minset,n,NULL)->u.minlen = srclen;
	}
}

/*
 * A cluster is collapsed in three steps.
 * 1) The nodes of the cluster are ranked locally.
 * 2) The cluster is collapsed into one node on the least rank.
 * 3) In class1(), any inter-cluster edges are converted using
 *    the "virtual node + 2 edges" trick.
 */
void collapse_cluster(graph_t *g, graph_t *subg)
{
	node_induce(g,subg);
	if (agfstnode(subg) == NULL) return;
	make_new_cluster(g,subg);
	if (CL_type == LOCAL) {
		dot_rank(subg);
		cluster_leader(subg);
	}
	else scan_ranks(subg);
}

int
make_new_cluster(graph_t *g, graph_t *subg)
{
	int 	cno;
	cno = ++(g->u.n_cluster);
	g->u.clust = ZALLOC(cno+1,g->u.clust,graph_t*,g->u.n_cluster);
	g->u.clust[cno] = subg;
	do_graph_label(subg);
	return cno;
}

void node_induce(graph_t *par, graph_t *g)
{
	node_t		*n,*nn;
	edge_t		*e;
	int			i;

	/* enforce that a node is in at most one cluster at this level */
	for (n = agfstnode(g); n; n = nn) {
		nn = agnxtnode(g,n);
		if (n->u.ranktype) {agdelete(g,n); continue;}
		for (i = 1; i < par->u.n_cluster; i++)
			if (agcontains(par->u.clust[i],n)) break;
		if (i < par->u.n_cluster) agdelete(g,n);
		n->u.clust = NULL;
	}

	for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
		for (e = agfstout(g->root,n); e; e = agnxtout(g->root,e)) {
			if (agcontains(g,e->head)) aginsert(g,e);
		}
	}
}

void cluster_leader(graph_t* clust)
{
	node_t		*leader,*n;
	int			maxrank = 0;

	/* find number of ranks and select a leader */
	leader = NULL;
	for (n = clust->u.nlist; n; n = n->u.next) {
		if ((n->u.rank == 0) && (n->u.node_type == NORMAL)) leader = n;
		if (maxrank < n->u.rank) maxrank = n->u.rank;
	}
	assert(leader != NULL);
	clust->u.leader = leader;

	for (n = agfstnode(clust); n; n = agnxtnode(clust,n)) {
		assert ((n->u.UF_size <= 1) || (n == leader));
		UF_union(n,leader);
		n->u.ranktype = CLUSTER;
	}
}

/* 
 * Assigns ranks of non-leader nodes.
 * Expands same, min, max rank sets.
 * Leaf sets and clusters remain merged.
 * Sets minrank and maxrank appropriately.
 */
void expand_ranksets(graph_t* g)
{
	int			c;
	node_t		*n,*leader;

	if ((n = agfstnode(g))) {
		g->u.minrank = MAXSHORT;
		g->u.maxrank = -1;
		while (n) {
			leader = UF_find(n);
			/* The following works because n->u.rank == 0 if n is not in a
			 * cluster, and n->u.rank = the local rank offset if n is in
			 * a cluster. */
			if (leader != n) n->u.rank += leader->u.rank;

			if (g->u.maxrank < n->u.rank) g->u.maxrank = n->u.rank;
			if (g->u.minrank > n->u.rank) g->u.minrank = n->u.rank;

			if (n->u.ranktype && (n->u.ranktype != LEAFSET))
					UF_singleton(n);
			n = agnxtnode(g,n);
		}
		if (g == g->root) {
			if (CL_type == LOCAL) {
				for (c = 1; c <= g->u.n_cluster; c++)
					set_minmax(g->u.clust[c]);
			}
			else {
				find_clusters(g);
			}
		}
	}
	else {
		g->u.minrank = g->u.maxrank = 0;
	}
}

void renewlist(elist* L)
{
	int		i;
	for (i = L->size; i >= 0; i--) L->list[i] = NULL;
	L->size = 0;
}

void cleanup1(graph_t* g)
{
	node_t		*n;
	edge_t		*e, *f;
	int			c;

	for (c = 0; c < g->u.comp.size; c++) {
		g->u.nlist = g->u.comp.list[c];
		for (n = g->u.nlist; n; n = n->u.next) {
			renewlist(&n->u.in);
			renewlist(&n->u.out);
			n->u.mark = FALSE;
		}
	}
	for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
		for (e = agfstout(g,n); e; e = agnxtout(g,e)) {
			f = e->u.to_virt;
			if (f && (e == f->u.to_orig)) free(f);
			e->u.to_virt = NULL;
		}
	}
	free(g->u.comp.list);
	g->u.comp.list = NULL;
	g->u.comp.size = 0;
}

void set_minmax(graph_t* g)
{
	int		c;

	g->u.minrank += g->u.leader->u.rank;
	g->u.maxrank += g->u.leader->u.rank;
	for (c = 1; c <= g->u.n_cluster; c++) set_minmax(g->u.clust[c]);
}

void scan_ranks(graph_t* g)
{
	node_t		*n,*leader=NULL;
	g->u.minrank = MAXSHORT;
	g->u.maxrank = -1;
	for (n = agfstnode(g); n; n = agnxtnode(g,n)) {
		if (g->u.maxrank < n->u.rank) g->u.maxrank = n->u.rank;
		if (g->u.minrank > n->u.rank) g->u.minrank = n->u.rank;
		if (leader == NULL) leader = n;
		else { if (n->u.rank < leader->u.rank) leader = n; }
	}
	g->u.leader = leader;
}

void find_clusters(graph_t* g)
{
	graph_t		*mg,*subg;
	node_t		*mn;
	edge_t		*me;

	mg = g->meta_node->graph;
	for (me = agfstout(mg,g->meta_node); me; me = agnxtout(mg,me)) {
		mn = me->head;
		subg = agusergraph(mn);

		if (subg->u.set_type == CLUSTER) collapse_cluster(g,subg);
	}
}

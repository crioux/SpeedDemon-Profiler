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
 * Network Simplex Algorithm for Ranking Nodes of a DAG
 */

#include "dot.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static int init_graph(graph_t *);

#define LENGTH(e)		((e)->head->u.rank - (e)->tail->u.rank)
#define SLACK(e)		(LENGTH(e) - (e)->u.minlen)
#define SEQ(a,b,c)		(((a) <= (b)) && ((b) <= (c)))
#define TREE_EDGE(e)	(e->u.tree_index >= 0)

static graph_t	*G;
static int		N_nodes,N_edges;
static int		Minrank,Maxrank;
static int		S_i;		/* search index for enter_edge */
static int		Search_size;
#define SEARCHSIZE 30
static nlist_t	Tree_node;
static elist	Tree_edge;

void add_tree_edge(edge_t* e)
{
	node_t	*n;
	if (TREE_EDGE(e)) abort();
	e->u.tree_index = Tree_edge.size;
	Tree_edge.list[Tree_edge.size++] = e;
	if (e->tail->u.mark == FALSE) Tree_node.list[Tree_node.size++] = e->tail;
	if (e->head->u.mark == FALSE) Tree_node.list[Tree_node.size++] = e->head;
	n = e->tail;
	n->u.mark = TRUE;
	n->u.tree_out.list[n->u.tree_out.size++] = e;
	n->u.tree_out.list[n->u.tree_out.size] = NULL;
	if (n->u.out.list[n->u.tree_out.size-1] == 0) abort();
	n = e->head;
	n->u.mark = TRUE;
	n->u.tree_in.list[n->u.tree_in.size++] = e;
	n->u.tree_in.list[n->u.tree_in.size] = NULL;
	if (n->u.in.list[n->u.tree_in.size-1] == 0) abort();
}

void exchange_tree_edges(edge_t *e,edge_t *f)
{
	int		i,j;
	node_t	*n;

	f->u.tree_index = e->u.tree_index;
	Tree_edge.list[e->u.tree_index] = f;
	e->u.tree_index = -1;

	n = e->tail;
	i = --(n->u.tree_out.size);
	for (j = 0; j <= i; j++) if (n->u.tree_out.list[j] == e) break;
	n->u.tree_out.list[j] = n->u.tree_out.list[i];
	n->u.tree_out.list[i] = NULL;
	n = e->head;
	i = --(n->u.tree_in.size);
	for (j = 0; j <= i; j++) if (n->u.tree_in.list[j] == e) break;
	n->u.tree_in.list[j] = n->u.tree_in.list[i];
	n->u.tree_in.list[i] = NULL;

	n = f->tail;
	n->u.tree_out.list[n->u.tree_out.size++] = f;
	n->u.tree_out.list[n->u.tree_out.size] = NULL;
	n = f->head;
	n->u.tree_in.list[n->u.tree_in.size++] = f;
	n->u.tree_in.list[n->u.tree_in.size] = NULL;
}

void init_rank(void)
{
	int			i,ctr;
	queue		*Q;
	node_t		*v;
	edge_t		*e;

	Q = new_queue(N_nodes);
	ctr = 0;

	for (v = G->u.nlist; v; v = v->u.next) {
		if (v->u.priority == 0) enqueue(Q,v);
	}

	while ((v = dequeue(Q))) {
		v->u.rank = 0;
		ctr++;
		for (i = 0; (e = v->u.in.list[i]); i++)
			v->u.rank = MAX(v->u.rank,e->tail->u.rank + e->u.minlen);
		for (i = 0; (e = v->u.out.list[i]); i++) {
			if (--(e->head->u.priority) <= 0) enqueue(Q,e->head);
		}
	}
	if (ctr != N_nodes) {
		fprintf(stderr,"trouble in init_rank\n");
		for (v = G->u.nlist; v; v = v->u.next)
			if (v->u.priority)
				fprintf(stderr,"\t%s %d\n",v->name,v->u.priority);
	}
	free_queue(Q);
}

node_t *
incident(edge_t* e)
{
	if (e->tail->u.mark) {
		if (e->head->u.mark == FALSE)
			return e->tail;
	}
	else {
		if (e->head->u.mark)
			return e->head;
	}
	return NULL;
}

edge_t *
leave_edge (void)
{
	edge_t			*f,*rv = NULL;
	int				j,cnt = 0;

	j = S_i;
	while (S_i < Tree_edge.size) {
		if ((f = Tree_edge.list[S_i])->u.cutvalue < 0) {
			if (rv) {
				if (rv->u.cutvalue > f->u.cutvalue) rv = f;
			}
			else rv = Tree_edge.list[S_i];
			if (++cnt >= Search_size) return rv;
		}
		S_i++;
	}
	if (j > 0) {
		S_i = 0;
		while (S_i < j) {
			if ((f = Tree_edge.list[S_i])->u.cutvalue < 0) {
				if (rv) {
					if (rv->u.cutvalue > f->u.cutvalue) rv = f;
				}
				else rv = Tree_edge.list[S_i];
				if (++cnt >= Search_size) return rv;
			}
			S_i++;
		}
	}
	return rv;
}

static edge_t	*Enter;
static int		Low,Lim,Slack;

void dfs_enter_outedge(node_t* v)
{
	int		i,slack;
	edge_t	*e;

	for (i = 0; (e = v->u.out.list[i]); i++) {
		if (TREE_EDGE(e) == FALSE) {
			if (!SEQ(Low,e->head->u.lim,Lim)) {
				slack = SLACK(e);
				if ((slack < Slack) || (Enter == NULL)) {
					Enter = e;
					Slack = slack;
				}
			}
		}
		else if (e->head->u.lim < v->u.lim) dfs_enter_outedge(e->head);
	}
	for (i = 0; (e = v->u.tree_in.list[i]) && (Slack > 0); i++)
		if (e->tail->u.lim < v->u.lim) dfs_enter_outedge(e->tail);
}

void dfs_enter_inedge(node_t* v)
{
	int		i,slack;
	edge_t	*e;

	for (i = 0; (e = v->u.in.list[i]); i++) {
		if (TREE_EDGE(e) == FALSE) {
			if (!SEQ(Low,e->tail->u.lim,Lim)) {
				slack = SLACK(e);
				if ((slack < Slack) || (Enter == NULL)) {
					Enter = e;
					Slack = slack;
				}
			}
		}
		else if (e->tail->u.lim < v->u.lim) dfs_enter_inedge(e->tail);
	}
	for (i = 0; (e = v->u.tree_out.list[i]) && (Slack > 0); i++)
		if (e->head->u.lim < v->u.lim) dfs_enter_inedge(e->head);
}

edge_t *
enter_edge(edge_t* e)
{
	node_t	*v;
	int		outsearch;

	/* v is the down node */
	if (e->tail->u.lim < e->head->u.lim) {v = e->tail; outsearch = FALSE;}
	else {v = e->head;outsearch = TRUE;}
	Enter = NULL;
	Slack = MAXINT;
	Low = v->u.low;
	Lim = v->u.lim;
	if (outsearch) dfs_enter_outedge(v);
	else dfs_enter_inedge(v);
	return Enter;
}

int treesearch(node_t* v)
{
	int		i;
	edge_t	*e;

	for (i = 0; (e = v->u.out.list[i]); i++) {
		if ((e->head->u.mark == FALSE) && (SLACK(e) == 0)) {
			add_tree_edge(e);
			if ((Tree_edge.size == N_nodes-1) || treesearch(e->head)) return TRUE;
		}
	}
	for (i = 0; (e = v->u.in.list[i]); i++) {
		if ((e->tail->u.mark == FALSE) && (SLACK(e) == 0)) {
			add_tree_edge(e);
			if ((Tree_edge.size == N_nodes-1) || treesearch(e->tail)) return TRUE;
		}
	}
	return FALSE;
}

int
tight_tree(void)
{
	int		i;
	node_t	*n;

	for (n = G->u.nlist; n; n = n->u.next) {
		n->u.mark = FALSE;
		n->u.tree_in.list[0] = n->u.tree_out.list[0] = NULL;
		n->u.tree_in.size = n->u.tree_out.size = 0;
	}
	for (i = 0; i < Tree_edge.size; i++) Tree_edge.list[i]->u.tree_index = -1;

	Tree_node.size = Tree_edge.size = 0;
	for (n = G->u.nlist; n && (Tree_edge.size == 0); n = n->u.next) treesearch(n);
	return Tree_node.size;
}

void init_cutvalues(void)
{
	dfs_range(G->u.nlist,NULL,1);
	dfs_cutval(G->u.nlist,NULL);
}

void feasible_tree(void)
{
	int			i,delta;
	node_t		*n;
	edge_t		*e,*f;

	if (N_nodes <= 1) return;
	while (tight_tree() < N_nodes) {
		e = NULL;
		for (n = G->u.nlist; n; n = n->u.next) {
			for (i = 0; (f = n->u.out.list[i]); i++) {
				if ((TREE_EDGE(f) == FALSE) && incident(f) && ((e == NULL)
					|| (SLACK(f) < SLACK(e)))) e = f;
			}
		}
		if (e) {
			delta = SLACK(e);
			if (delta) {
				if (incident(e) == e->head) delta = -delta;
				for (i = 0; i < Tree_node.size; i++)
					Tree_node.list[i]->u.rank += delta;
			}
		}
		else {
#ifdef DEBUG
			fprintf(stderr,"not in tight tree:\n");
			for (n = G->u.nlist; n; n = n->u.next) {
				for (i = 0; i < Tree_node.size; i++)
					if (Tree_node.list[i] == n) break;
				if (i >= Tree_node.size) fprintf(stderr,"\t%s\n",n->name);
			}
#endif
			abort();
		}
	}
	init_cutvalues();
}

/* walk up from v to LCA(v,w), setting new cutvalues. */
node_t *
treeupdate(node_t *v, node_t *w, int cutvalue, int dir)
{
	edge_t	*e;
	int		d;

	while (!SEQ(v->u.low,w->u.lim,v->u.lim)) {
		e = v->u.par;
		if (v == e->tail) d = dir; else d = NOT(dir);
		if (d) e->u.cutvalue += cutvalue; else e->u.cutvalue -= cutvalue;
		if (e->tail->u.lim > e->head->u.lim) v = e->tail; else v = e->head;
	}
	return v;
}

void rerank(node_t* v, int delta)
{
	int		i;
	edge_t	*e;

	v->u.rank -= delta;
	for (i = 0; (e = v->u.tree_out.list[i]); i++) 
		if (e != v->u.par) rerank(e->head,delta);
	for (i = 0; (e = v->u.tree_in.list[i]); i++) 
		if (e != v->u.par) rerank(e->tail,delta);
}

/* e is the tree edge that is leaving and f is the nontree edge that
 * is entering.  compute new cut values, ranks, and exchange e and f.
 */
void update(edge_t *e, edge_t *f)
{
	int		cutvalue,delta;
	node_t	*lca;

	delta = SLACK(f);
	/* "for (v = in nodes in tail side of e) do v->u.rank -= delta;" */
	if (delta > 0) {
		int s;
		s = e->tail->u.tree_in.size + e->tail->u.tree_out.size;
		if (s == 1) rerank(e->tail,delta);
		else {
			s = e->head->u.tree_in.size + e->head->u.tree_out.size;
			if (s == 1) rerank(e->head,-delta);
			else {
				if (e->tail->u.lim < e->head->u.lim) rerank(e->tail,delta);
				else rerank(e->head,-delta);
			}
		}
	}

	cutvalue = e->u.cutvalue;
	lca = treeupdate(f->tail,f->head,cutvalue,1);
	if (treeupdate(f->head,f->tail,cutvalue,0) != lca) abort();
	f->u.cutvalue = -cutvalue;
	e->u.cutvalue = 0;
	exchange_tree_edges(e,f);
	dfs_range(lca,lca->u.par,lca->u.low);
}

void scan_result(void)
{
	node_t	*n;

	Minrank = MAXINT;
	Maxrank = -MAXINT;
	for (n = G->u.nlist; n; n = n->u.next) {
		if (n->u.node_type == NORMAL) {
			Minrank = MIN(Minrank,n->u.rank);
			Maxrank = MAX(Maxrank,n->u.rank);
		}
	}
}

void LR_balance(void)
{
	int		i,delta;
	node_t	*n;
	edge_t	*e,*f;

	for (i = 0; i < Tree_edge.size; i++) {
		e = Tree_edge.list[i];
		if (e->u.cutvalue == 0) {
			f = enter_edge(e);
			if (f == NULL) continue;
			delta = SLACK(f);
			if (delta <= 1) continue;
			if (e->tail->u.lim < e->head->u.lim) rerank(e->tail,delta/2);
			else rerank(e->head,-delta/2);
		}
	}
	for (n = G->u.nlist; n; n = n->u.next) {
		free_list(n->u.tree_in);
		free_list(n->u.tree_out);
		n->u.mark = FALSE;
	}
}

void TB_balance(void)
{
	node_t	*n;
	edge_t	*e;
	int		i,low,high,choice,*nrank;
	int		inweight,outweight;

	scan_result();
	if (Minrank != 0) {
		for (n = G->u.nlist; n; n = n->u.next) n->u.rank -= Minrank;
		Maxrank -= Minrank;
		Minrank = 0;
	}

	/* find nodes that are not tight and move to less populated ranks */
	nrank = N_NEW(Maxrank+1,int);
	for (i = 0; i <= Maxrank; i++) nrank[i] = 0;
	for (n = G->u.nlist; n; n = n->u.next)
		if (n->u.node_type == NORMAL) nrank[n->u.rank]++;
	for (n = G->u.nlist; n; n = n->u.next) {
		if (n->u.node_type != NORMAL) continue;
		inweight = outweight = 0;
		low = 0;
		high = Maxrank;
		for (i = 0; (e = n->u.in.list[i]); i++) {
			inweight += e->u.weight;
			low = MAX(low,e->tail->u.rank + e->u.minlen);
		}
		for (i = 0; (e = n->u.out.list[i]); i++) {
			outweight += e->u.weight;
			high = MIN(high,e->head->u.rank - e->u.minlen);
		}
		if (low < 0) low = 0;	/* vnodes can have ranks < 0 */
		if (inweight == outweight) {
			choice = low;
			for (i = low + 1; i <= high; i++)
				if (nrank[i] < nrank[choice]) choice = i;
			nrank[n->u.rank]--; nrank[choice]++;
			n->u.rank = choice;
		}
		free_list(n->u.tree_in);
		free_list(n->u.tree_out);
		n->u.mark = FALSE;
	}
	free(nrank);
}

static int
init_graph(graph_t* g)
{
	int		i,feasible;
	node_t	*n;
	edge_t	*e;

	G = g;
	N_nodes = N_edges = S_i = 0;
	for (n = g->u.nlist; n; n = n->u.next) {
		n->u.mark = FALSE;
		N_nodes++;
		for (i = 0; (e = n->u.out.list[i]); i++) N_edges++;
	}

	Tree_node.list = ALLOC(N_nodes,Tree_node.list,node_t*);
	Tree_node.size = 0;
	Tree_edge.list = ALLOC(N_nodes,Tree_edge.list,edge_t*);
	Tree_edge.size = 0;

	feasible = TRUE;
	for (n = g->u.nlist; n; n = n->u.next) {
		n->u.priority = 0;
		for (i = 0; (e = n->u.in.list[i]); i++) {
			n->u.priority++;
			e->u.cutvalue = 0;
			e->u.tree_index = -1;
			if (feasible && (e->head->u.rank - e->tail->u.rank < e->u.minlen))
				feasible = FALSE;
		}
		n->u.tree_in.list = N_NEW(i+1,edge_t*);
		n->u.tree_in.size = 0;
		for (i = 0; (e = n->u.out.list[i]); i++);
		n->u.tree_out.list = N_NEW(i+1,edge_t*);
		n->u.tree_out.size = 0;
	}
	return feasible;
}

void rank(graph_t *g, int tb_mode, int maxiter)
{
	int	iter = 0,feasible;
	char	*s,*ns = "network simplex: ";
	edge_t	*e,*f;

	if (Verbose) start_timer();
	feasible = init_graph(g);
	if (!feasible) init_rank();
	if (maxiter <= 0) return;

	if ((s = agget(g,"searchsize"))) Search_size = atoi(s);
	else Search_size = SEARCHSIZE;

	feasible_tree();
	while ((e = leave_edge())) {
		f = enter_edge(e);
		update(e,f);
		iter++;
		if (Verbose && (iter % 100 == 0)) {
			if (iter % 1000 == 100) fputs(ns,stderr);
			fprintf(stderr,"%d ",iter);
			if (iter % 1000 == 0) fputc('\n',stderr);
			if (iter >= maxiter) break;
		}
	}
	if (tb_mode) TB_balance();
	else LR_balance();
	if (Verbose) {
		if (iter >= 100) fputc('\n',stderr);
		fprintf(stderr,"%s%d nodes %d edges %d iter %.2f sec\n",
			ns,N_nodes,N_edges,iter,elapsed_sec());
	}
}

/* set cut value of f, assuming values of edges on one side were already set */
void x_cutval(edge_t* f)
{
	node_t	*v;
	edge_t	*e;
	int		i,sum,dir;

	/* set v to the node on the side of the edge already searched */
	if (f->tail->u.par == f) { v = f->tail; dir = 1; }
	else { v = f->head; dir = -1; }

	sum = 0;
	for (i = 0; (e = v->u.out.list[i]); i++) sum += x_val(e,v,dir);
	for (i = 0; (e = v->u.in.list[i]); i++) sum += x_val(e,v,dir);
	f->u.cutvalue = sum;
}

int x_val(edge_t* e, node_t* v, int dir)
{
	node_t	*other;
	int		d,rv,f;

	if (e->tail == v) other = e->head; else other = e->tail;
	if (!(SEQ(v->u.low,other->u.lim,v->u.lim))) {f = 1; rv = e->u.weight;}
	else {
		f = 0;
		if (TREE_EDGE(e)) rv = e->u.cutvalue;
		else rv = 0;
		rv -= e->u.weight;
	}
	if (dir > 0) {if (e->head == v) d = 1; else d = -1;}
	else {if (e->tail == v) d = 1; else d = -1; }
	if (f) d = -d;
	if (d < 0) rv = -rv;
	return rv;
}

void dfs_cutval(node_t* v, edge_t* par)
{
	int		i;
	edge_t	*e;

	for (i = 0; (e = v->u.tree_out.list[i]); i++)
		if (e != par) dfs_cutval(e->head,e);
	for (i = 0; (e = v->u.tree_in.list[i]); i++)
		if (e != par) dfs_cutval(e->tail,e);
	if (par) x_cutval(par);
}

int dfs_range(node_t* v, edge_t* par, int low)
{
	edge_t	*e;
	int		i,lim;

	lim = low;
	v->u.par = par;
	v->u.low = low;
	for (i = 0; (e = v->u.tree_out.list[i]); i++)
		if (e != par) lim = dfs_range(e->head,e,lim);
	for (i = 0; (e = v->u.tree_in.list[i]); i++)
		if (e != par) lim = dfs_range(e->tail,e,lim);
	v->u.lim = lim;
	return lim + 1;
}

#ifdef DEBUG
void tchk(void)
{
	int		i,n_cnt,e_cnt;
	node_t	*n;
	edge_t	*e;

	n_cnt = 0;
	e_cnt = 0;
	for (n = G->u.nlist; n; n = n->u.next) {
		n_cnt++;
		for (i = 0; e = n->u.tree_out.list[i]; i++) {
			e_cnt++;
			if (SLACK(e) > 0)
				printf("not a tight tree %x",e);
		}
	}
	if ((n_cnt != Tree_node.size)  || (e_cnt != Tree_edge.size))
		printf("something missing\n");
}

void check_cutvalues(void)
{
	node_t	*v;
	edge_t	*e;
	int		i,save;

	for (v = G->u.nlist; v; v = v->u.next) {
		for (i = 0; (e = v->u.tree_out.list[i]); i++) {
			save = e->u.cutvalue;
			x_cutval(e);
			if (save != e->u.cutvalue) abort();
		}
	}
}

int
check_ranks(void)
{
	int		i, cost = 0;
	node_t	*n;
	edge_t	*e;

	for (n = G->u.nlist; n; n = n->u.next) {
		for (i = 0; (e = n->u.out.list[i]); i++) {
			cost += (e->u.weight)*abs(LENGTH(e));
if (e->head->u.rank - e->tail->u.rank - e->u.minlen < 0) abort();
		}
	}
	fprintf(stderr,"rank cost %d\n",cost);
	return cost;
}

void checktree(void)
{
	int		i,n = 0,m = 0;
	node_t	*v;
	edge_t	*e;

	for (v = G->u.nlist; v; v = v->u.next) {
		for (i = 0; (e = v->u.tree_out.list[i]); i++) n++;
		if (i != v->u.tree_out.size) abort();
		for (i = 0; (e = v->u.tree_in.list[i]); i++) m++;
		if (i != v->u.tree_in.size) abort();
	}
	printf("%d %d %d\n",Tree_edge.size,n,m);
}

void check_fast_node(node_t* n)
{
	node_t	*nptr;
	nptr = n->graph->u.nlist;
	while (nptr && nptr != n) nptr = nptr->u.next;
	assert (nptr != NULL);
}

void checkdfs(node_t* n)
{
	int		i;
	edge_t	*e;
	node_t	*w;

	if (n->u.mark) return;
	n->u.mark = TRUE;
	n->u.onstack = TRUE;
	for (i = 0; (e = n->u.out.list[i]); i++) {
		w = e->head;
		if (w->u.onstack)
			fprintf(stderr,"cycle involving %s %s\n",n->name,w->name);
		else {
			if (w->u.mark == FALSE) checkdfs(w);
		}
	}
	n->u.onstack = FALSE;
}

void check_cycles(graph_t* g)
{
	node_t	*n;
	for (n = g->u.nlist; n; n = n->u.next) n->u.mark = n->u.onstack = FALSE;
	for (n = g->u.nlist; n; n = n->u.next) checkdfs(n);
}
#endif /* DEBUG */

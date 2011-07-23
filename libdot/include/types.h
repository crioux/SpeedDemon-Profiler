/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/

#include "pathplan.h"

typedef unsigned char boolean;

typedef int (*qsort_cmpf)(const void*, const void*);
typedef int (*bsearch_cmpf) (const void *, const void *);

typedef struct Agraph_t graph_t;
typedef struct Agnode_t node_t;
typedef struct Agedge_t edge_t;
typedef struct Agsym_t attrsym_t;

typedef struct pointf {
	double		x,y;
} pointf;

typedef struct point {
	int			x,y;
} point;

typedef struct box {
	point		LL,UR;
} box;

typedef struct port_t {			/* internal edge endpoint specification */
	point		p;					/* aiming point */
	float		theta;				/* slope in radians */
	boolean	constrained,defined;
	unsigned char order;			/* for mincross */
} port_t;

typedef struct path {			/* internal specification for an edge spline */
	port_t		start,end;
	point		*ulpp, *urpp, *llpp, *lrpp;	/* tangents of near splines */
	int			nbox;				/* number of subdivisions */
	box			*boxes;				/* rectangular regions of subdivision */
	void		*data;
} path;

typedef struct bezier {
	point		*list;
	int			size;
	int			sflag, eflag;
	point		sp, ep;
} bezier;

typedef struct splines {
	bezier		*list;
	int			size;
} splines;

typedef struct polygon_t {		/* mutable shape information for a node */
	int			regular; 			/* true for symmetric shapes */
	int			peripheries;		/* number of periphery lines */
	int			sides;				/* number of sides */
	float		orientation;		/* orientation of shape (+ve degrees) */
	float		distortion;			/* distortion factor - as in trapezium */
	float		skew;				/* skew factor - as in parallelogram */
	int			option;				/* ROUNDED, DIAGONAL corners, etc. */
	pointf		*vertices;		    /* array of vertex points */
} polygon_t;

typedef struct shape_desc {		/* read-only shape descriptor */
	char		*name;							 /* as read from graph file */
	void		(*initfn)(node_t *);			/* initializes shape from node u.shape_info structure */
	void		(*freefn)(node_t *);			/* frees  shape from node u.shape_info structure */
	port_t		(*portfn)(node_t *, char *);	/* finds aiming point and slope of port */
	int			(*insidefn)(node_t *, pointf, edge_t*);		/* clips incident edge spline */
	int			(*pboxfn)(node_t *, edge_t *, int, box *, int *);		/* finds box path to reach port */
	void		(*codefn)(node_t *);			/* emits graphics code for node */
	polygon_t	*polygon;						/* base polygon info */
} shape_desc;

typedef struct codegen_t {
	void    (*reset)(void);
	void	(*begin_job)(FILE *ofp,graph_t *g, char **lib, char *user, char *info[], point pages);
	void 	(*end_job)(void);
	void	(*begin_graph)(graph_t* g, box bb, point pb);
	void	(*end_graph)(void);
	void	(*begin_page)(point page, double scale, int rot, point offset);
	void	(*end_page)(void);
	void    (*begin_cluster)(graph_t *);
	void	(*end_cluster)(void); 
	void    (*begin_nodes)(void);
	void	(*end_nodes)(void);
	void    (*begin_edges)(void);
	void    (*end_edges)(void);
	void	(*begin_node)(node_t *);
	void	(*end_node)(void);
	void	(*begin_edge)(edge_t *);
	void	(*end_edge)(void);
	void	(*begin_context)(void);
	void	(*end_context)(void);
	void	(*set_font)(char *fontname, double fontsize);
	void	(*textline)(point p, char *str, int width, double fontsz, double align);
	void	(*set_color)(char *name);
	void	(*set_style)(char **s);
	void	(*ellipse)(point p, int rx, int ry, int filled);
	void	(*polygon)(point *A, int n, int filled);
	void	(*beziercurve)(point *A, int n, int arrow_at_start, int arrow_at_end);
	void	(*polyline)(point *A,int n);
	/* void	(*arrowhead)(point p, double theta, double scale, int flag); */
	boolean bezier_has_arrows;
	void	(*user_shape)(char *name, point *A, int sides, int filled);
	void	(*comment)(void* obj, attrsym_t* sym);
	point	(*textsize)(char *str, char *fontname, double fontsz);
} codegen_t;

typedef struct queue {
	node_t	**store,**limit,**head,**tail;
} queue;

typedef struct adjmatrix_t {
	int		nrows,ncols;
	char	*data;
} adjmatrix_t;

typedef struct rank_t {
	int				n;			/* number of nodes in this rank			*/
	node_t			**v;		/* ordered list of nodes in rank		*/
	int				an;			/* globally allocated number of nodes	*/
	node_t			**av;		/* allocated list of nodes in rank		*/
	int 			ht1,ht2;	/* height below/above centerline		*/
	int 			pht1,pht2;	/* as above, but only primitive nodes 	*/
	boolean			candidate;	/* for transpose ()						*/
	boolean			valid;
	int				cache_nc;	/* caches number of crossings			*/
	adjmatrix_t		*flat;
} rank_t;

typedef struct textline_t {
	char			*str;
	short			width;
	char			just;
} textline_t;

typedef struct textlabel_t {
	char			*text,*fontname,*fontcolor;
	float			fontsize;
	pointf			dimen;
	point			p;
	textline_t		*line;
	char			nlines;
} textlabel_t;

typedef enum engine_e {DOT, NEATO} engine_t;

typedef struct layout_t {
	float			quantum,scale,font_scale_adj;
	point			margin, page, size;
	boolean			landscape,centered;
	engine_t		engine;
} layout_t;

/* for "record" shapes */
typedef struct field_t {
	point		size;			/* its dimension */
	box			b;				/* its final placement */
	int			n_flds;	
	textlabel_t	*lp;			/* n_flds == 0 */
	struct field_t	**fld;		/* n_flds > 0 */
	int		LR;					/* if box list is horizontal (left to right) */
	char	*id;				/* user's identifier */
} field_t;

typedef struct hsbcolor_t {
	char			*name;
	unsigned char	h,s,b;
} hsbcolor_t;

typedef struct nlist_t {
      node_t        **list;
      int           size;
} nlist_t;

typedef struct elist {
      edge_t        **list;
      int           size;
} elist;

#define elist_fastapp(item,L) do {L.list[L.size++] = item; L.list[L.size] = NULL;} while(0)
#define elist_append(item,L)  do {L.list = ALLOC(L.size + 2,L.list,edge_t*); L.list[L.size++] = item; L.list[L.size] = NULL;} while(0)
#define alloc_elist(n,L)      do {L.size = 0; L.list = N_NEW(n + 1,edge_t*); } while (0)
#define free_list(L)          do {if (L.list) free(L.list);} while (0)

typedef struct Agraphinfo_t {
		/* to generate code */
	layout_t			*drawing;
    textlabel_t        	*label;         /* if the cluster has a title */
	box					bb;				/* bounding box */
	point				border[4];		/* sizes of margins for graph labels */
	boolean				left_to_right,has_edge_labels;
	int					ht1,ht2;		/* below and above extremal ranks */

#ifndef DOT_ONLY
		/* to place nodes */
	node_t				**neato_nlist;
	int					move;
	double				**dist,**spring,**sum_t,***t;
#endif
#ifndef NEATO_ONLY
		/* to have subgraphs */
	int					n_cluster;
	graph_t				**clust;
	node_t				*nlist;
    rank_t              *rank;
        /* fast graph node list */
    nlist_t             comp;
        /* connected components */
    node_t              *minset,*maxset;                /* set leaders */
    long                n_nodes;
        /* includes virtual */
    short               minrank,maxrank;

        /* various flags */
    boolean             has_flat_edges;
    boolean             showboxes;

    int                 nodesep,ranksep;
    node_t              *ln,*rn;        /* left, right nodes of bounding box */


        /* for clusters */
    node_t              *leader,**rankleader;
    boolean             expanded;
    char                installed;
    char                set_type;
    boolean             exact_ranksep;
#endif

} Agraphinfo_t;

typedef struct Agnodeinfo_t {
	shape_desc			*shape;
	void				*shape_info;
	point				coord;
	float				width,height;
	int					ht,lw,rw;
	textlabel_t			*label;
	char				state;

#ifndef DOT_ONLY
	boolean				pinned;
	short				xsize,ysize;
	int					id,heapindex,hops;
	double				pos[NDIM],dist;
#endif
#ifndef NEATO_ONLY
    boolean             showboxes,has_port;

        /* fast graph */
    char                node_type,mark,onstack;
    char                ranktype,weight_class;
    node_t              *next,*prev;
    elist               in,out,flat_out,flat_in,other;
    graph_t             *clust;

        /* for union-find and collapsing nodes */
    int                 UF_size;
    node_t              *UF_parent;
    node_t              *inleaf,*outleaf;

        /* for placing nodes */
    int                 rank,order;     /* initially, order = 1 for ordered edges */
    int                 mval;
    elist               save_in,save_out;

        /* for network-simplex */
    elist               tree_in,tree_out;
    edge_t              *par;
    int                 low,lim;
    int                 priority;

    double              pad[1];
#endif

} Agnodeinfo_t;

typedef struct Agedgeinfo_t {
	splines				*spl;
	port_t				tail_port,head_port;	/* might be used someday */
	textlabel_t			*label,*head_label,*tail_label;
	char				edge_type;
	edge_t				*to_orig;				/* for dot's shapes.c    */

#ifndef DOT_ONLY
	float				factor;
	float				dist;
	Ppolyline_t			path;
#endif
#ifndef NEATO_ONLY
    boolean             showboxes;
    boolean             conc_opp_flag;
    short               xpenalty;
    int                 weight;
    int                 cutvalue,tree_index;
    short               count,minlen;
	edge_t              *to_virt;
#endif

} Agedgeinfo_t;

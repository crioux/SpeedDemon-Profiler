/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/

typedef void (*nodesizefn_t)(Agnode_t*, boolean);

#include <string>

extern point		add_points(point, point);
extern double		atan2pt(point, point);
extern void		attach_attrs(Agraph_t *);
extern pointf		Bezier(pointf *, int, double, pointf *, pointf *);
extern box		boxof(int, int, int, int);
extern char		*canoncolor(char *, char *);
extern void		cat_libfile(FILE *, char **, char **);
extern int		clust_in_layer(Agraph_t *);
extern int		codegen_bezier_has_arrows(void);
extern int		color2rgb(char *name, double rgb[3]);
extern int		colorcmpf(void*, void*);
extern char		*colorxlate(char *, char *);
extern point		coord(node_t *n);
extern pointf		cvt2ptf(point);
extern point		cvt2pt(pointf);
extern Agnode_t		*dequeue(queue *);
extern void		do_graph_label(graph_t* g);
extern point		dotneato_closest (splines *spl, point p);
extern void		graph_init(Agraph_t *);
extern void     	dotneato_initialize(int, char **, char *);
extern void		dotneato_postprocess(Agraph_t *, nodesizefn_t);
extern void		dotneato_terminate(void);
extern void		dotneato_write(Agraph_t *, std::string *);
extern int		edge_in_CB(Agedge_t *);
extern int		edge_in_layer(Agraph_t *, Agedge_t *);
extern void		enqueue_neighbors(queue *, Agnode_t *, int);
extern void		emit_attachment(textlabel_t *, splines *);
extern void		emit_background(Agraph_t *, point, point);
extern void		emit_clusters(Agraph_t *);
extern void		emit_defaults(graph_t* g);
extern void		emit_edge(Agedge_t *);
extern void		emit_edge(Agedge_t *);
extern void		emit_eof(void);
extern void		emit_graph(Agraph_t *, int sorted);
extern void		emit_header(Agraph_t *);
extern void		emit_label(textlabel_t *, graph_t *);
extern void		emit_layer(int);
extern void		emit_node(node_t *);
extern void		emit_node(node_t *);
extern void		emit_reset(Agraph_t *);
extern void		emit_trailer(void);
extern void		epsf_define(void);
extern void		epsf_gencode(node_t *);
extern void		epsf_init(node_t *);
extern void		epsf_free(node_t *);
extern FILE		*file_select(char *);
extern shape_desc	*find_user_shape(char *);
extern void		free_line(textline_t *);
extern void		free_label(textlabel_t *);
extern void		free_queue(queue *);
extern void		free_ugraph(graph_t *);
extern char		*gd_alternate_fontlist(char *font);
extern point		gd_textsize(char *str, char *fontname, double fontsz);
extern void		getfloats2pt(graph_t* g, char* name, point* result);
extern void		getfloat(graph_t* g, char* name, double* result);
extern void		global_def(char *, Agsym_t *(*fun)(Agraph_t *, char *, char *));
extern void		hsv2rgb(double *, double *, double *, double, double, double);
extern void		init_ugraph(Agraph_t *);
extern int		is_natural_number(char *);
extern pointf		label_size(char *, textlabel_t *, graph_t *);
extern int		late_attr(void*, char *);
extern int		late_bool(void *, Agsym_t *, int);
extern double		late_float(void*, Agsym_t *, double, double);
extern int		late_int(void*, Agsym_t *, int, int);
extern char		*late_nnstring(void*, Agsym_t *, char *);
extern char		*late_string(void*, Agsym_t *, char *);
extern int		layer_index(char *, int);
extern int		layerindex(char *);
extern textlabel_t	*make_label(char *, double, char *, char *, graph_t *);
extern int		mapbool(char *);
extern int		maptoken(char *, char **, int *);
extern void		map_edge(Agedge_t *);
extern point		map_point(point);
extern void		Mcircle_hack(node_t* n);
extern box		mkbox(point, point);
extern void		Mlabel_hack(node_t* n);
extern point		neato_closest (splines *spl, point p);
extern FILE		*next_input_file(void);
extern Agraph_t		*next_input_graph(void);
extern int		node_in_CB(node_t *);
extern int		node_in_layer(Agraph_t *, node_t *);
extern void		osize_label(textlabel_t *, int *, int *, int *, int *);
extern point		pageincr(point);
extern point		pagecode(char);
extern int		parse_layers(char *);
extern char		**parse_style(char* s);
extern void		place_graph_label(Agraph_t *);
extern point		pointof(int, int);
extern void		printptf(FILE *, point);
extern void		rec_attach_bb(Agraph_t *);
extern int		rect_overlap(box, box);
extern void		round_corners(node_t *, point *, int, int);
extern int		same_side(pointf, pointf, pointf, pointf);
extern int		selectedlayer(char *);
extern void		setup_graph(Agraph_t *);
extern void		setup_page(graph_t* g, point page);
extern point		spline_at_y(splines* spl, int y);
extern unsigned char	spline_merge(node_t* n);
extern int		strccnt(char *p, char c);
extern void		translate_bb(Agraph_t *, int);
extern Agnode_t		*UF_find(Agnode_t *);
extern void		UF_remove(Agnode_t *, Agnode_t *);
extern void		UF_setname(Agnode_t *, Agnode_t *);
extern void		UF_singleton(Agnode_t *);
extern Agnode_t		*UF_union(Agnode_t *, Agnode_t *);
extern void		use_library(char *);
extern char		*username(char *);
extern int		validpage(point);
extern void		write_plain(Agraph_t *, FILE*);
extern void*		zmalloc(size_t);
extern void*		zrealloc(void*, size_t, size_t, size_t);

#if defined(_BLD_dot) && defined(_DLL)
#   define extern __EXPORT__
#endif
extern point		sub_points(point, point);
extern int		lang_select(char *);
extern int		round(double);

extern void		toggle(int);
extern int		test_toggle();
extern void		intr(int);
#undef extern

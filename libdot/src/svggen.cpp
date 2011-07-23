/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
 */

/* this is a rough start at a working SVG driver.
 you can get image nodes by setting a node's
	 [shape=webimage,shapefile="http://www.your.site.com/path/image.png"]
 (of course can also be a file: reference if you run locally.
 which causes a warning (FIX that) but the back end turns this into
 a custom shape.  you can also set
	[ URL = "http://some.place.com/whatever" ] 
 to get clickable nodes (and edges for that matter).

 some major areas needing work:
  0. fonts including embedded font support.  is SVG finished in this area?
  1. styles, including dotted/dashed lines, also "style function"
     passthrough in SVG similar to what we have in postscript.
  2. look at what happens in landscape mode, pagination? etc.
  3. allow arbitrary user transforms via graph "style" attribute.
  4. javascript hooks.  particularly, look at this in the context
     of SVG animation for dynadag output (n.b. dynadag only in alpha release)
  5. image node improvement, e.g. set clip path to node boundary, and
     support scaling with fixed aspect ratio (this feature seems to be
     broken in the current Adobe SVG plugin for Windows).  
  6. can we support arbitrary HTML for node contents?
  7. encode abstract graph as interleaved XML in some sort of graph XML dialect?
  8. accessibility features
  9. embed directions about getting plugin, if not present in browser

  Stephen North
  north@research.att.com
*/

#include	"render.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define	NONE	0
#define	NODE	1
#define	EDGE	2
#define	CLST	3

/* SVG font modifiers */
#define REGULAR 0
#define BOLD	1
#define ITALIC	2

/* SVG patterns */
#define P_SOLID	0
#define P_NONE  15
#define P_DOTTED 4	/* i wasn't sure about this */
#define P_DASHED 11 /* or this */

/* SVG bold line constant */
#define WIDTH_NORMAL 1
#define WIDTH_BOLD 3

#ifdef TESTFAILED
/* sodipodi doesn't understand style sheets */
#define DEFAULT_STYLE\
"<style type='text/css'>\n" \
"<![CDATA[\n" \
".node ellipse {fill:none;filter:URL(#MyFilter)}\n" \
".node polygon {fill:none;filter:URL(#MyFilter)}\n" \
".cluster polygon {fill:none;filter:URL(#MyFilter)}\n" \
".edge path {fill:none;stroke:black;stroke-width:1;}\n" \
"text {font-family:Times;stroke:black;stroke-width:.4;font-size:12px}\n" \
"]]>\n" \
"</style>"

#define DEFAULT_FILTER \
"<filter id=\"MyFilter\" x=\"-20%\" y=\"-20%\" width=\"160%\" height=\"160%\">\n" \
"<feGaussianBlur in=\"SourceAlpha\" stdDeviation=\"4\" result=\"blur\"/>\n" \
"<feOffset in=\"blur\" dx=\"4\" dy=\"4\" result=\"offsetBlur\"/>\n" \
"<feSpecularLighting in=\"blur\" surfaceScale=\"5\" specularConstant=\"1\"\n" \
"specularExponent=\"10\" style=\"lighting-color:white\" result=\"specOut\">\n" \
"<fePointLight x=\"-5000\" y=\"-10000\" z=\"20000\"/>\n" \
"</feSpecularLighting>\n" \
"<feComposite in=\"specOut\" in2=\"SourceAlpha\" operator=\"in\" result=\"specOut\"/>\n" \
"<feComposite in=\"SourceGraphic\" in2=\"specOut\" operator=\"arithmetic\"\n" \
"k1=\"0\" k2=\"1\" k3=\"1\" k4=\"0\" result=\"litPaint\"/>\n" \
"<feMerge>\n" \
"<feMergeNode in=\"offsetBlur\"/>\n" \
"<feMergeNode in=\"litPaint\"/>\n" \
"</feMerge>\n" \
"</filter>"
#endif

/* SVG dash array */
static char * sdarray = "5,2";
/* SVG dot array */
static char * sdotarray = "1,5";

static	FILE	*Outfile;
static	int		Obj,N_pages;
/* static 	point	Pages; */
static	double	Scale;
static	pointf	Offset;
static	int		Rot;
static	box		PB;
static	graph_t		*G;
static int		onetime = TRUE;

static node_t		*Curnode;
static char		*CurURL;

typedef struct context_t {
	char 	*color,*fontfam,fontopt,font_was_set;
	char	pen,fill,penwidth,style_was_set;
	float	fontsz;
} context_t;

#define MAXNEST 4
static context_t cstk[MAXNEST];
static int SP;

static void svg_reset(void)
{
	onetime = TRUE;
}


static void init_svg(void)
{
	SP = 0;
	cstk[0].color = 0;		/* SVG color */
	cstk[0].fontfam = "Times";	/* font family name */
	cstk[0].fontopt = REGULAR;	/* modifier: REGULAR, BOLD or ITALIC */
	cstk[0].pen = P_SOLID;		/* pen pattern style, default is solid */
	cstk[0].fill = P_NONE;
	cstk[0].penwidth = WIDTH_NORMAL;
}

/* can't hack a transform directly in SVG preamble, alas */
static point
svgpt(point p)
{
	point	rv;

	if (Rot == 0) {
		rv.x = p.x * Scale + Offset.x;
		rv.y = PB.UR.y - PB.LL.y - p.y * Scale + Offset.y;
	} else {
		rv.x = PB.UR.x - PB.LL.x - p.y * Scale + Offset.x;
		rv.y = p.x * Scale + Offset.y;
	}
	return rv;
}

static void
svgptarray(point* A, int n)
{
	int		i;
	point	p;
	char	*c;

	c = "M";
	for (i = 0; i < n; i++) {
		p.x = A[i].x; p.y = A[i].y;
		p = svgpt(p);
		fprintf(Outfile,"%s%d %d",c,p.x,p.y);
		c = "L";
	}
}

static void
svgbzptarray(point* A, int n)
{
	int		i;
	point	p;
	char	*c;

	p.x = A[0].x; p.y = A[0].y;
	p = svgpt(p);
	fprintf(Outfile,"M%d %d",p.x,p.y);
	c = "C";
	for (i = 1; i < n; i++) {
		p.x = A[i].x; p.y = A[i].y;
		p = svgpt(p);
		fprintf(Outfile,"%s%d %d",c,p.x,p.y);
		c = " ";
	}
}

static void svg_font(context_t* cp)
{
/*
	char	*fw,*fa;

	fw = fa = "Regular";
	switch (cp->fontopt) {
		case BOLD: fw = "Bold"; break;
		case ITALIC: fa = "Italic"; break;
	}
*/
	fprintf(Outfile,"style=\"font-family:%s;font-size:%.2f\"",
		cp->fontfam,cp->fontsz);
	/* ,cp->fontfam,cp->fontsz,fw,fa); */
}

static void svg_grstyle(context_t* cp, int filled)
{
	static char *known_colors[] = {"aqua", "black",
	  "blue", "fuchsia", "gray", "green", "lime",
	  "maroon", "navy", "olive", "purple", "red",
	  "silver", "teal", "white", "yellow", 0};
	char buf[1024], *color, **known, *p;
	double	rgb[3];
	/* int	i; */

	color = canoncolor(cp->color,buf);
	for (known = known_colors; *known; known++)
		if (streq(color,*known)) break;
	if (*known == 0) {
		color2rgb(color,rgb);
		color = buf;
		sprintf(color,"#%2x%2x%2x",
			ROUND(rgb[0]*255),
			ROUND(rgb[1]*255),
			ROUND(rgb[2]*255));
		/* i forget how to do this in sprintf */
		/* maybe we should use rgb() functional notation anyway */
		for (p = color; *p; p++) if (*p == ' ') *p = '0';
	}
	fprintf(Outfile," style=\"");
	if (filled) {
		fprintf(Outfile,"fill:%s",color);
	} else {
#ifdef TESTFAILED
		fprintf(Outfile,"stroke:%s",color);
#else
		fprintf(Outfile,"fill:none;stroke:%s",color);
#endif
		if (cp->penwidth!=WIDTH_NORMAL)
			fprintf(Outfile,";stroke:%d",cp->penwidth);
	}
	if( cp->pen == P_DASHED ) {
		fprintf(Outfile,";stroke-dasharray:%s", sdarray);
	} else if( cp->pen == P_DOTTED) {
		fprintf(Outfile,";stroke-dasharray:%s", sdotarray);
	}
	fprintf(Outfile,"\"");
}

static void svg_comment(void* obj, attrsym_t* sym)
{
	char	*str;
	str = late_string(obj,sym,"");
	if (str[0]) fprintf(Outfile,"<!-- %s -->\n",str);
}

static void
svg_begin_job(FILE *ofp, graph_t *g, char **lib, char *user, char *info[], point pages)
{
	Outfile = ofp;
	/* Pages = pages; */
	N_pages = pages.x * pages.y;
#ifdef NONE_OF_THIS_REALLY_WORKS
	fprintf(Outfile,"<?xml version=\"1.0\" standalone=\"no\"?>\n");
	fprintf(Outfile,"<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 20000303 Stylable//EN\"\n");
	fprintf(Outfile," \"http://www/w3.org/TR/2000/03/WD-SVG-20000303/DTD/svg-20000303-stylable.dtd\">\n");
#endif
	fprintf(Outfile,"<!-- Generated by %s version %s (%s)\n", info[0],info[1],info[2]);
	fprintf(Outfile,"     For: %s\n",user);
	fprintf(Outfile,"     Title: %s\n",g->name);
	fprintf(Outfile,"     Pages: %d\n-->\n",N_pages);
}

static  void
svg_end_job(void)
{
}

static void
svg_begin_graph(graph_t* g, box bb, point pb)
{
	G = g;
	PB = bb;
	Offset.x = PB.LL.x + 1;
	Offset.y = PB.LL.y + 1;
	if (onetime) {
#if 0
fprintf(stderr,"LL %d %d UR %d %d\n", PB.LL.x,PB.LL.y,PB.UR.x, PB.UR.y);
#endif
		init_svg();
		svg_comment(g,agfindattr(g,"comment"));
		onetime = FALSE;
	}
}

static void
svg_end_graph(void)
{
}

static void
svg_begin_page(point page, double scale, int rot, point offset)
{
	/* int		page_number; */
	point	sz;

	Scale = scale;
	Rot = rot;
	/* page_number =  page.x + page.y * Pages.x + 1; */
	sz = sub_points(PB.UR,PB.LL);
	fprintf(Outfile,"<svg width=\"%dpx\" height=\"%dpx\"",
		PB.UR.x + PB.LL.x + 2, PB.UR.y + PB.LL.y + 2);
#ifdef TICKLE_ADOBE_ILLUSTRATOR_PLUGIN_BUG_THIS_DOESNT_WORK_EITHER
	fprintf(Outfile," xmlns='http://www.w3.org/Graphics/SVG/svg-19990412.dtd'");
#endif

	/* namespace of svg */
	fprintf(Outfile," xmlns='http://www.w3.org/2000/svg'");
	fprintf(Outfile,">\n");
#ifdef TESTFAILED
	/* sodipodi doesn't understand style sheets */
	fprintf(Outfile,"%s\n",DEFAULT_STYLE);
	fprintf(Outfile,"<def>\n%s\n</def>\n",DEFAULT_FILTER);
#endif
	/* its really just a page of the graph, but its still a graph,
	* and it is the entire graph if we're not currently paging */
	fprintf(Outfile,"<g id=\"%s\" class=\"graph\" >\n",G->name);
}

static  void
svg_end_page(void)
{
	fprintf(Outfile,"</g></svg>\n");
}

static  void
svg_begin_cluster(graph_t* g)
{
	fprintf(Outfile,"<g id=\"%s\" class=\"cluster\" >\n",g->name);
	Obj = CLST;
}

static  void
svg_end_cluster (void)
{
	fprintf(Outfile,"</g>\n");
	Obj = NONE;
}

static void
svg_begin_nodes(void)        
{
	Obj = NONE;
}
    
static void
svg_end_nodes(void)  
{
	Obj = NONE;
}

static void 
svg_begin_edges(void)        
{
	Obj = NONE;
}            

static void
svg_end_edges(void)  
{            
	Obj = NONE;
}            

static void beginURL(void *obj)
{
	char	*s, *url, *esc;
	char 	*s0, *s1, *s2, *s3, *s4;

	if ((s = agget(obj, "URL")) && strlen(s)) {
		CurURL = s;
		s0 = url = strdup(s);
		s1 = s2 = s3 = s4 = "";
		if (esc = strstr(url,NODENAME_ESC)) {
			*esc = 0;
			s4 = esc + strlen(NODENAME_ESC);

			switch(Obj) {
			case NODE:
				s2 = ((Agnode_t*)obj)->name;
				break;
			case CLST:
				s2 = ((Agraph_t*)obj)->name;
				break;
			case EDGE:
				s1 = ((Agedge_t*)obj)->tail->name;
				s2 = "->";
				s3 = ((Agedge_t*)obj)->head->name;
				break;
			}
		}

		fprintf(Outfile,"<a xlink:href=\"%s%s%s%s%s\"",s0,s1,s2,s3,s4);
#ifdef TESTFAILED
		/* amaya doesn't understand this */
	       	fprintf(Outfile," xlink:title=\"%s%s%s%s\"",s0,s1,s2);
	       	fprintf(Outfile," xmlns:xlink=\"http://www.w3.org/1999/xlink\"");
		fprintf(Outfile," xlink:type=\"simple\"");
		fprintf(Outfile," target=\"description\"");
#endif
		fprintf(Outfile,">\n");
	} else CurURL = 0;
}

static void endURL()
{
	if (CurURL) fprintf(Outfile,"</a>");
	CurURL = 0;
}

static  void
svg_begin_node(node_t* n)
{
	Obj = NODE;
	Curnode = n;
#if 0
	fprintf(Outfile,"<!-- %s -->\n",n->name);
	svg_comment(n,N_comment);
#endif
	fprintf(Outfile,"<g id=\"%s\" class=\"node\" >\n",n->name);
	beginURL(n);
}

static  void
svg_end_node (void)
{
	endURL();
	fprintf(Outfile,"</g>\n");
	Obj = NONE;
}

static  void
svg_begin_edge (edge_t* e)
{
	Obj = EDGE;
#if 0
	fprintf(Outfile,"<!-- %s->%s -->\n",e->tail->name,e->head->name);
	svg_comment(e,E_comment);
#endif
	fprintf(Outfile,"<g id=\"%s->%s\" class=\"edge\" >\n",e->tail->name,e->head->name);
	beginURL(e);
}

static  void
svg_end_edge (void)
{
	endURL();
	fprintf(Outfile,"</g>\n");
	Obj = NONE;
}

static  void
svg_begin_context(void)
{
	assert(SP + 1 < MAXNEST);
	cstk[SP+1] = cstk[SP];
	SP++;
}

static  void 
svg_end_context(void)
{
	int			psp = SP - 1;
	assert(SP > 0);
	/*free(cstk[psp].fontfam);*/
	SP = psp;
}

static void 
svg_set_font(char* name, double size)
{
	char	*p,*q,buf[SMALLBUF];
	context_t	*cp;

	cp = &(cstk[SP]);
	cp->font_was_set = TRUE;
	cp->fontsz = Scale * size;
	p = strdup(name);
	if ((q = strchr(p,'-'))) {
		*q++ = 0;
		canoncolor(q,buf);
		if (streq(buf,"italic")) cp->fontopt = ITALIC;
		else if (streq(q,"bold")) cp->fontopt = BOLD;
	}
	cp->fontfam = p;
}

static  void
svg_set_color(char* name)
{
	cstk[SP].color = name;
}

static  void
svg_set_style(char** s)
{
	char		*line, *p;
	context_t	*cp;

	cp = &(cstk[SP]);
	while ((p = line = *s++)) {
		if (streq(line,"solid")) { /* no-op */ }
		else if (streq(line,"dashed")) cp->pen = P_DASHED;
		else if (streq(line,"dotted")) cp->pen = P_DOTTED;
		else if (streq(line,"invis")) cp->pen = P_NONE;
		else if (streq(line,"bold")) cp->penwidth = WIDTH_BOLD;
		else if (streq(line, "setlinewidth")) {
			while (*p) p++; p++; 
			cp->penwidth = atol(p);
		}
		else if (streq(line,"filled")) cp->fill = P_SOLID;
		else if (streq(line,"unfilled")) { /* no-op */ }
		else {
            fprintf(stderr, "svg_set_style: unsupported style %s - ignoring\n",
                line); 
        }
		cp->style_was_set = TRUE;
	}
	/* if (cp->style_was_set) svg_style(cp); */
}

static char *
svg_string(char *s, char *auxbuf)
{
	char			*p = auxbuf,esc;
	while (*s) {
		esc = 0;
		switch (*s) {
			case '\t':	esc = 't'; break;
			case '>': case '\'': case '`': case '\\': esc = *s; break;
		}
		if (esc) {*p++ = '\\'; *p++ = esc;}
		else *p++ = *s;
		s++;
	}
	*p = '\0';
	return auxbuf;
}

static void svg_textline(point  p, char *str, int width, double fontsz, double align)
{
	char	buf[BUFSIZ];
	point	mp;
    /* int x; */
    /* int y; */
    /* int sub_type;        justification */
    char *string;

	if( cstk[SP].pen == P_NONE ) {
		/* its invisible, don't draw */
		return;
	}
	if (align == -.5) mp.x = p.x - width/2.0; /* normal - center on p*/
	else if (align < 0.0) mp.x = p.x - width; /* right justify - left of p */
	else mp.x = p.x;			/* left justify - right of p */
#if 1
	mp.y = p.y - fontsz/3;
#else
	mp.y = p.y;
#endif
	mp = svgpt(mp);

	/* x = mp.x; */
	/* y = mp.y; */

/* FIXME
	if (align == -.5) sub_type = 1;
	else {if (align < 0) sub_type = 2; else sub_type = 0;}
*/
/* FIXME
 * do text rotations
 */

	string = svg_string(str,buf);

	fprintf(Outfile,"<text x=\"%d\" y=\"%d\" ",mp.x,mp.y);
	svg_font(&cstk[SP]);
	fprintf(Outfile,">%s</text>\n",string);
}

static void svg_ellipse(point p, int rx, int ry, int filled)
{
	point	mp;

	if( cstk[SP].pen == P_NONE ) {
		/* its invisible, don't draw */
		return;
	}
	mp.x = p.x;
	mp.y = p.y;
	mp = svgpt(mp);
	fprintf(Outfile,"<ellipse cx=\"%d\" cy=\"%d\"",
		mp.x,mp.y);
    if (Rot) {int t; t = rx; rx = ry; ry = t;}
	mp.x = Scale * rx;
	mp.y = Scale * ry;
	fprintf(Outfile," rx=\"%d\" ry=\"%d\"",mp.x,mp.y);
	svg_grstyle(&cstk[SP], filled);
	fprintf(Outfile,"/>\n");
}

static void svg_bezier(point* A, int n, int arrow_at_start, int arrow_at_end)
{
	if( cstk[SP].pen == P_NONE ) {
		/* its invisible, don't draw */
		return;
	}
	fprintf(Outfile,"<g");
	svg_grstyle(&cstk[SP],0);
	fprintf(Outfile,"><path d=\"");
	svgbzptarray(A,n);
	fprintf(Outfile,"\"/></g>\n");
}

static void svg_polygon(point *A, int n, int filled)
{
	int	i;
	point	p;

	if( cstk[SP].pen == P_NONE ) {
		/* its invisible, don't draw */
		return;
	}
	fprintf(Outfile,"<polygon");
       	svg_grstyle(&cstk[SP],filled);
	fprintf(Outfile," points=\"");
	for (i = 0; i < n; i++) {
		p = svgpt(A[i]);
		fprintf(Outfile,"%d,%d ",p.x,p.y);
	}
	/* because Adobe SVG is broken */
	p = svgpt(A[0]);
	fprintf(Outfile,"%d,%d",p.x,p.y);
	fprintf(Outfile,"\"/>\n");
}

static void svg_polyline(point* A, int n)
{
	int	i;
	point	p;

	if( cstk[SP].pen == P_NONE ) {
		/* its invisible, don't draw */
		return;
	}
	fprintf(Outfile,"<polyline");
       	svg_grstyle(&cstk[SP],0);
	fprintf(Outfile," points=\"");
	for (i = 0; i < n; i++) {
		p = svgpt(A[i]);
		fprintf(Outfile,"%d,%d ",p.x,p.y);
	}
	fprintf(Outfile,"\"/>\n");
}

static void svg_user_shape(char *name, point *A, int n, int filled)
{
	/* int	i; */
	point	p;
	point	sz;
	char	*imagefile;

	if( cstk[SP].pen == P_NONE ) {
		/* its invisible, don't draw */
		return;
	}
	imagefile = agget(Curnode,"shapefile");
	if (imagefile == 0) {
		svg_polygon(A, n, filled);
	       	return;
	}
	p = Curnode->u.coord;
	p.x -= Curnode->u.lw;
	p.y += Curnode->u.ht/2;
	p = svgpt(p);
	sz.x = ROUND(Scale*(Curnode->u.lw+Curnode->u.rw));
	sz.y = ROUND(Scale*Curnode->u.ht);
	fprintf(Outfile,"<image xlink:href=\"%s\" width=\"%dpx\" height=\"%dpx\" preserveAspectRatio=\"xMidYMid meet\" x=\"%d\" y=\"%d\" \n",
		imagefile,sz.x,sz.y,p.x,p.y);
	
#ifdef NOTDEF
	fprintf(Outfile,"<clipPath <polygon points=\"");
		for (i = 0; i < n; i++) {
			p = svgpt(A[i]);
			fprintf(Outfile,"%d,%d ",p.x,p.y);
		}
		/* because Adobe SVG is broken */
		p = svgpt(A[0]);
		fprintf(Outfile,"%d,%d ",p.x,p.y);
		fprintf(Outfile,"\"/> />\n");
#endif
	fprintf(Outfile," />\n");
}

codegen_t	SVG_CodeGen = {
	svg_reset,
	svg_begin_job, svg_end_job,
	svg_begin_graph, svg_end_graph,
	svg_begin_page, svg_end_page,
	svg_begin_cluster, svg_end_cluster,
	svg_begin_nodes, svg_end_nodes,
	svg_begin_edges, svg_end_edges,
	svg_begin_node, svg_end_node,
	svg_begin_edge, svg_end_edge,
	svg_begin_context, svg_end_context,
	svg_set_font, svg_textline,
	svg_set_color, svg_set_style,
	svg_ellipse, svg_polygon,
	svg_bezier, svg_polyline,
    0/* svg_arrowhead */, svg_user_shape,
	0, 0
};

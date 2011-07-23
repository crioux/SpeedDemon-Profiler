/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/

#include	"render.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define	NONE	0
#define	NODE	1
#define	EDGE	2
#define	CLST	3

#define PIC_COORDS_PER_LINE (16)		/* to avoid stdio BUF overflow */

static	FILE	*Outfile;
/* static	int		Obj; */
static	box		BB;
static  int		BezierSubdivision = 10;
static  int	Rot;
static  int		onetime = TRUE;
static	double	Scale;
static	double	Fontscale;

/* static char	**U_lib,*User,*Vers; */
static const char	*EscComment = ".\\\" ";	/* troff comment */

typedef struct grcontext_t {
	char	*color,*font;
	double	size;
} grcontext_t;

#define STACKSIZE 8
static grcontext_t S[STACKSIZE];
static int SP = 0;

static char		picgen_msghdr[] = "dot picgen: ";
static void unsupported(char* s)
{ fprintf(stderr,"%s%s unsupported\n",picgen_msghdr,s); }
static void warn(char* s)
{ fprintf(stderr,"%s%s warning\n",picgen_msghdr,s); }
#undef DEBUG
static void debug(char *s)
{
#ifdef	DEBUG
fprintf(stderr, "%s%s\n",picgen_msghdr,s);
fflush(stderr);
fflush(Outfile);
#endif
}

#define LINESPACING	*1.2

#undef	MAX
#ifndef	MAX
# define	MAX(a,b)	(((a)>(b))? (a) : (b))
#endif
#undef	MIN
#ifndef	MIN
# define	MIN(a,b)	(((a)<(b))? (a) : (b))
#endif

/* There are a couple of ways to generate output: 
    1. generate for whatever size is given by the bounding box
       - the drawing at its "natural" size might not fit on a physical page
         ~ dot size specification can be used to scale the drawing
         ~ and it's not difficult for user to scale the pic output to fit (multiply 4 (3 distinct) numbers on 3 lines by a scale factor)
       - some troff implementations may clip large graphs
         ~ handle by scaling to manageable size
       - give explicit width and height as parameters to .PS
       - pic scale variable is reset to 1.0
       - fonts are printed as size specified by caller, modified by user scaling
    2. scale to fit on a physical page
       - requires an assumption of page size (GNU pic assumes 8.5x11.0 inches)
         ~ any assumption is bound to be wrong more often than right
       - requires separate scaling of font point sizes since pic's scale variable doesn't affect text
         ~ possible, as above
       - likewise for line thickness
       - GNU pic does this (except for fonts) if .PS is used without explicit width or height; DWB pic does not
         ~ pic variants likely to cause trouble
  The first approach is used here.
*/

/* troff font mapping */
typedef struct {
	char	trname[3],*psname;
} fontinfo;

static fontinfo fonttab[] = {
	{"AB",  "AvantGarde-Demi"},
	{"AI",  "AvantGarde-BookOblique"},
	{"AR",  "AvantGarde-Book"},
	{"AX",  "AvantGarde-DemiOblique"},
	{"B ",  "Times-Bold"},
	{"BI",  "Times-BoldItalic"},
	{"CB",  "Courier-Bold"},
	{"CO",  "Courier"},
	{"CX",  "Courier-BoldOblique"},
	{"H ",  "Helvetica"},
	{"HB",  "Helvetica-Bold"},
	{"HI",  "Helvetica-Oblique"},
	{"HX",  "Helvetica-BoldOblique"},
	{"Hb",  "Helvetica-Narrow-Bold"},
	{"Hi",  "Helvetica-Narrow-Oblique"},
	{"Hr",  "Helvetica-Narrow"},
	{"Hx",  "Helvetica-Narrow-BoldOblique"},
	{"I ",  "Times-Italic"},
	{"KB",  "Bookman-Demi"},
	{"KI",  "Bookman-LightItalic"},
	{"KR",  "Bookman-Light"},
	{"KX",  "Bookman-DemiItalic"},
	{"NB",  "NewCenturySchlbk-Bold"},
	{"NI",  "NewCenturySchlbk-Italic"},
	{"NR",  "NewCenturySchlbk-Roman"},
	{"NX",  "NewCenturySchlbk-BoldItalic"},
	{"PA",  "Palatino-Roman"},
	{"PB",  "Palatino-Bold"},
	{"PI",  "Palatino-Italic"},
	{"PX",  "Palatino-BoldItalic"},
	{"R ",  "Times-Roman"},
	{"S ",  "Symbol"},
	{"ZD",  "ZapfDingbats"},
	{"\000\000", (char*)0}
};

static char *picfontname(char* psname)
{
	char	*rv;
	fontinfo *p;

	debug("picfontname");
	for (p = fonttab; p->psname; p++)
		if (streq(p->psname,psname)) break;
	if (p->psname) rv = p->trname;
	else {
		fprintf(stderr,"%s%s is not a troff font\n",picgen_msghdr,psname);
		/* try base font names, e.g. Helvetica-Outline-Oblique -> Helvetica-Outline -> Helvetica */
		if ((rv = strrchr(psname, '-'))) {
			*rv = '\0';	/* psname is not specified as const ... */
			rv = picfontname(psname);
		} else
			rv = "R";
	}
	return rv;
}

static char *pic_fcoord(char* buf, pointf pf)
{
	debug("pic_fcoord");
	sprintf(buf,"(%.5lf,%.5lf)",Scale*pf.x,Scale*pf.y);
	return buf;
}

static char *pic_coord(char* buf, point p)
{
	debug("pic_coord");
	return pic_fcoord(buf,cvt2ptf(p));
}

static void pic_reset(void)
{
	debug("pic_reset");
	onetime = TRUE;
}

static void pic_begin_job(FILE *ofp, graph_t *g, char **lib, char *user,
char *info[], point pages)
{
	debug("pic_begin_job");
	Outfile = ofp;
	/* U_lib = lib; */
	if (onetime && (pages.x * pages.y > 1)) {
		unsupported("pagination");
		onetime = FALSE;
	}
        fprintf(Outfile,"%s Creator: %s version %s (%s)\n",
                EscComment, info[0], info[1],info[2]);
        fprintf(Outfile,"%s For: %s\n",EscComment, user);
        fprintf(Outfile,"%s Title: %s\n",EscComment, g->name);
}

static void pic_end_job(void) { debug("pic_end_job"); }

static void pic_begin_graph(graph_t* g, box bb, point pb)
{
	debug("pic_begin_graph");
	/*g = g;*/
	BB = bb;

	fprintf(Outfile,"%s save point size and font\n.nr .S \\n(.s\n.nr DF \\n(.f\n", EscComment);
}

static void pic_end_graph(void)
{
	debug("pic_end_graph");
	fprintf(Outfile,"%s restore point size and font\n.ps \\n(.S\n.ft \\n(DF\n", EscComment);
}

static void pic_begin_page(point page, double scale, int rot, point offset)
{
	double	height, width;

	debug("pic_begin_page");
	if (onetime && rot && (rot != 90)) {
		unsupported("rotation");
		onetime = FALSE;
	}
	Rot = rot;
	height = PS2INCH((double)(BB.UR.y)-(double)(BB.LL.y));
	width = PS2INCH((double)(BB.UR.x)-(double)(BB.LL.x));
	Scale = scale;
	if (Rot == 90) { double temp = width; width = height; height = temp; }
	fprintf(Outfile,".PS %.5lf %.5lf\n", width, height);
	EscComment = "#";	/* pic comment */
	fprintf(Outfile,"%s to change drawing size, multiply the width and height on the .PS line above and the number on the two lines below (rounded to the nearest integer) by a scale factor\n", EscComment);
	if (width > 0.0) {
		Fontscale = log10(width);
		Fontscale += 3.0 - (int)Fontscale;	/* between 3.0 and 4.0 */
	} else
		Fontscale = 3.0;
	Fontscale = pow(10.0, Fontscale);	/* a power of 10 times width, between 1000 and 10000 */
	fprintf(Outfile,".nr SF %.0lf\nscalethickness = %.0lf\n", Fontscale, Fontscale);
	fprintf(Outfile,"%s don't change anything below this line in this drawing\n", EscComment);
	fprintf(Outfile,"scale=1.0 %s required for boxwid, etc. comparisons\n", EscComment);
	fprintf(Outfile,"%s dashwid is 0.1 in 10th Edition, 0.05 in DWB 2 and in gpic\n", EscComment);
	fprintf(Outfile,"%s fillval is 0.3 in 10th Edition (fill 0 means black), 0.5 in gpic (fill 0 means white), undefined in DWB 2\n", EscComment);
	fprintf(Outfile,"%s arrowhead is undefined in DWB 2.0, initially 1 in gpic, 2 in 10th Edition\n", EscComment);
	fprintf(Outfile,"%s fill has no meaning in DWB 2, gpic can use fill or filled, 10th Edition uses fill only\n", EscComment);
	fprintf(Outfile,"%s boxwid is 0.75 in DWB 2 and 10th Edition(? check!), ever so slightly (approx. 1e-16) less in gpic [ditto for ellipsewid]\n", EscComment);
	fprintf(Outfile,"%s DWB 2 doesn't use fill and doesn't define fillval\n", EscComment);
	fprintf(Outfile,"%s reset works in gpic and 10th edition, but isn't defined in DWB 2\n", EscComment);
	fprintf(Outfile,"%s if boxwid or dashwid have been changed before .PS, the following may fail\n", EscComment);
	fprintf(Outfile,"if boxwid == 0.75 && dashwid == 0.05 then X fillval = 1; define fill Y Y; define solid Y Y; define reset Y scale=1.0 Y X\n");
	fprintf(Outfile,"reset %s set to known state\n", EscComment);
	fprintf(Outfile,"%s GNU pic vs. 10th Edition d\\(e'tente\n", EscComment);
	fprintf(Outfile,"if fillval == 0.5 then X define setfillval Y fillval = 1 - Y; define bold Y thickness 2 Y; define solid Y Y X else Z define setfillval Y fillval = Y; define bold Y Y; define filled Y fill Y Z\n");
	fprintf(Outfile,"%s arrowhead has no meaning in DWB 2.0, arrowhead = 7 makes filled arrowheads in gpic and in 10th Edition\n", EscComment);
	fprintf(Outfile,"arrowhead = 7\n");
	fprintf(Outfile,"%s GNU pic supports a boxrad variable to draw boxes with rounded corners; DWB and 10th Ed. do not\n", EscComment);
	fprintf(Outfile,"boxrad = 0\n");
	fprintf(Outfile,"%s GNU pic supports a linethick variable to set line thickness; DWB and 10th Ed. do not\n", EscComment);
	fprintf(Outfile,"linethick = 0; oldlinethick = linethick\n");
	fprintf(Outfile,"%s .PS w/o args causes GNU pic to scale drawing to fit 8.5x11 paper; DWB does not\n", EscComment);
	fprintf(Outfile,"%s maxpsht and maxpswid have no meaning in DWB 2.0, set page boundaries in gpic and in 10th Edition\n", EscComment);
	fprintf(Outfile,"%s maxpsht and maxpswid are predefined to 11.0 and 8.5 in gpic\n", EscComment);
	fprintf(Outfile,"maxpsht = %f\nmaxpswid = %f\n", height, width);
	fprintf(Outfile,"Dot: [\n");
	fprintf(Outfile,"define attrs0 %% %%; define unfilled %% %%; define rounded %% %%; define diagonals %% %%\n");
}

static void pic_end_page(void)
{
	debug("pic_end_page");
	fprintf(Outfile,"]\n.PE\n");
	EscComment = ".\\\" ";	/* troff comment */
	assert(SP == 0);
}

static void pic_begin_cluster(graph_t* g)
{
	debug("pic_begin_cluster");
	/* Obj = CLST; */
}

static void pic_end_cluster(void)
{
	debug("pic_end_cluster");
	/* Obj = NONE; */
}

static void
pic_begin_nodes(void)        
{
	debug("pic_begin_nodes");
	/* Obj = NONE; */
}
    
static void
pic_end_nodes(void)  
{
	debug("pic_end_nodes");
	/* Obj = NONE; */
}

static void 
pic_begin_edges(void)        
{
	debug("pic_begin_edges");
	/* Obj = NONE; */
}            

static void
pic_end_edges(void)  
{            
	debug("pic_end_edges");
	/* Obj = NONE; */
}            

static void pic_begin_node(node_t* n)
{
	debug("pic_begin_node");
	/* Obj = NODE; */
	fprintf(Outfile,"%s\t%s\n",EscComment,n->name);
}

static void pic_end_node (void)
{
	debug("pic_end_node");
	/* Obj = NONE; */
}

static void pic_begin_edge (edge_t* e)
{
	debug("pic_begin_edge");
	/* Obj = EDGE; */
	fprintf(Outfile,"%s\t%s -> %s\n",EscComment,e->tail->name,e->head->name);
}

static void pic_end_edge (void)
{
	debug("pic_end_edge");
	/* Obj = NONE; */
}

static void pic_begin_context(void)
{
	debug("pic_begin_context");
	fprintf(Outfile,"{\n");
	if (SP == STACKSIZE - 1) warn("stk ovfl");
	else {
		SP++; S[SP] = S[SP-1];
		fprintf(Outfile,"define attrs%d %% %%\n", SP); /* ensure plain (no attributes) style at start of context */
	}
}

static void pic_end_context(void)
{
	debug("pic_end_context");
	if (SP == 0) warn("stk undfl");
	else {
		SP--;
		fprintf(Outfile,"}\n");	/* end context group */
		/* restore correct font and size for context */
		if (S[SP+1].font && (!(S[SP].font) || strcmp(S[SP+1].font,S[SP].font)))
			fprintf(Outfile, ".ft %s\n", picfontname(S[SP].font));
		if (S[SP+1].size != S[SP].size) {
			int	sz;

			if ((sz = (int)(S[SP].size * Scale)) < 1)
				sz = 1;
			fprintf(Outfile,".ps %d*\\n(SFu/%.0lfu\n",sz, Fontscale);
		}
	fprintf(Outfile,"linethick = oldlinethick\n");
	}
}

static void pic_set_font(char* name, double size)
{
	debug("pic_set_font");
	if (name && (!(S[SP].font) || strcmp(S[SP].font,name))) {
		S[SP].font = name;
		fprintf(Outfile,".ft %s\n",picfontname(name));
	}
	if (size != S[SP].size) {
		int	sz;

		S[SP].size = size;
		if ((sz = (int)(size * Scale)) < 1)
			sz = 1;
		fprintf(Outfile,".ps %d*\\n(SFu/%.0lfu\n",sz, Fontscale);
	}
}

static void pic_textline(point p, char *str, int width, double fontsz, double align)
{
	pointf	pf;
	char	flag = 0;

	debug("pic_textline");
	pf = cvt2ptf(p);
	pf.y -= fontsz / (5.0 * POINTS_PER_INCH);
	if (!(S[SP].size)) {	/* size was never set in this or hierarchically higher context */
		pic_set_font(S[SP].font, fontsz);	/* primarily to output font and/or size directives */
		for (flag=SP; ((S[flag].size=fontsz),flag); flag--)	/* set size in contexts */
			;	/* flag is zero again at loop termination */
	}
	if (fontsz != S[SP].size) {	/* size already set in context, but different from request; start new context */
		flag = 1;
		pic_begin_context();
		pic_set_font(S[SP-1].font, fontsz);
	}
	fprintf(Outfile, "\"");
	for (;*str; str++)  {
		if (*str == '\015')	/* GACK, PTUI! Fire up the teletype, boys; somebody's sending an old-fashioned mechanical "carriage return" control character. */
			continue;	/* skip it */
		(void) putc(*str, Outfile);
		if (*str == '\\')	/* double backslashes */
			(void) putc(*str, Outfile);
	}
	fprintf(Outfile,"\" at (%.5lf,%.5lf);\n",Scale*pf.x,Scale*pf.y);
	if (flag)
		pic_end_context();
}

/***** Arrowheads now centralized in emit.c
#ifdef NOTDEF
static void pic_arrowhead(point p, double angle, double scale,int flag)
{
	pointf	pf0,pf1;
	double	len,theta;

	debug("pic_arrowhead");
	theta = RADIANS(angle);
	len = PS2INCH(scale * ARROW_LENGTH);
	pf0 = cvt2ptf(p);
	pf1.x = pf0.x + len * cos(theta);
	pf1.y = pf0.y + len * sin(theta);
	fprintf(Outfile,"line -> attrs%d from (%.5lf,%.5lf) to (%.5lf,%.5lf);\n",
		SP,Scale*pf1.x,Scale*pf1.y,Scale*pf0.x,Scale*pf0.y);
}
#endif
**********/

static void pic_set_color(char* name)
{
	char	buf[SMALLBUF];
	double	gray;

	debug("pic_set_color");
	S[SP].color = name;
	(void) colorxlate(S[SP].color,buf);
	sscanf(buf, "%*f %*f %lf", &gray);
	fprintf(Outfile,"setfillval %f\n", gray);
}

static void pic_set_style(char** s) {
	const char	*line,*p;
	char	skip = 0;
	char	buf[BUFSIZ];

	debug("pic_set_style");
	buf[0]='\0';
	fprintf(Outfile, "define attrs%d %%", SP);
	while ((p = line = *s++)) {
		while (*p) p++; p++;
		while (*p) {
			if (!strcmp(line, "setlinewidth")) {	/* a hack to handle the user-defined (PS) style spec in proc3d.dot */
				long n = atol(p);

				sprintf(buf,"oldlinethick = linethick;linethick = %ld * scalethickness / %.0lf\n", n, Fontscale / Scale);
				skip = 1;
			} else
				fprintf(Outfile," %s",p);
			while (*p) p++; p++;
		}
		if (!skip)
			fprintf(Outfile," %s", line);
		skip = 0;
	}
	fprintf(Outfile," %%\n");
	fprintf(Outfile,"%s", buf);
}

static void pic_ellipse(point p, int rx, int ry, int filled)
{
	pointf	pf;

	debug("pic_ellipse");
	pf = cvt2ptf(p);
	fprintf(Outfile,"ellipse attrs%d %swid %.5lf ht %.5lf at (%.5lf,%.5lf);\n",
		SP, filled? "fill ": "",
		Scale*PS2INCH(2*rx), Scale*PS2INCH(2*ry), Scale*pf.x, Scale*pf.y);
}

static void point_list_out(point* A, int n, int close)
{
	int		j;
	char	buf[SMALLBUF];

	debug("pic_point_list_out");
	for (j = 0; j < n; j ++)
		fprintf(Outfile,"P%d: %s\n", j, pic_coord(buf,A[j]));
	for (j = 0; j+1 < n; j ++)
		fprintf(Outfile,"move to P%d; line attrs%d to P%d\n", j, SP, j+1);
	if (close)
		fprintf(Outfile,"move to P%d; line attrs%d to P0\n", n-1, SP);
}

static void pic_polygon(point *A, int n, int filled)
{
	debug("pic_polygon");
	/* test for special case: rectangle oriented with page */
	if ((n==4) && (
	((A[0].x == A[1].x) && (A[0].y == A[3].y) && (A[1].y == A[2].y) && (A[2].x == A[3].x))
	||
	((A[0].y == A[1].y) && (A[0].x == A[3].x) && (A[1].x == A[2].x) && (A[2].y == A[3].y))
	)) {
		pointf	pf1, pf2;

		pf1 = cvt2ptf(A[0]);	/* opposite */
		pf2 = cvt2ptf(A[2]);	/* corners  */
		if (filled) {
			char	buf[SMALLBUF];
			double	gray;

			(void) colorxlate(S[SP].color,buf);
			sscanf(buf, "%*f %*f %lf", &gray);
			fprintf(Outfile,"setfillval %f\n", gray);
		}
		fprintf(Outfile,"box attrs%d %swid %.5lf ht %.5lf at (%.5lf,%.5lf);\n",
			SP, filled ? "fill " : "",
			Scale*fabs(pf1.x-pf2.x), Scale*fabs(pf1.y-pf2.y),	/* width, height */
			Scale*(pf1.x+pf2.x)/2.0, Scale*(pf1.y+pf2.y)/2.0);	/* center coordinates */
		return;
	}
	if (onetime && filled) {
		unsupported("shape fill");
		onetime = FALSE;
	}
	point_list_out(A,n,TRUE);
}

static void pic_polyline(point* A, int n)
{
	debug("pic_polyline");
	point_list_out(A,n,FALSE);
}

static void pic_user_shape(char *name, point *A, int sides, int filled)
{
	debug("pic_user_shape");
	/* it's not at all clear what xxx_user_shape is supposed to do; in most xxxgen.c files it emits a message or draws a polygon */
	/* this defines the shape as a macro and then invokes the macro */
	fprintf(Outfile, "define %s {\n", name);
	pic_polygon(A, sides, filled);
	fprintf(Outfile, "}\n%s\n", name);
}

static void pic_bezier(point *A, int n, int arrow_at_start, int arrow_at_end)
{
	pointf	V[4],p;
	int		i,j,m,step;
	char	buf[SMALLBUF];

	debug("pic_bezier");
	if (arrow_at_start || arrow_at_end)
		warn("not supposed to be making arrows here!");
	V[3] = cvt2ptf(A[0]);	/* initial cond */
	for (i=m=0; i+3 < n; i += 3) {
		V[0] = V[3];
		for (j = 1; j <= 3; j++)
			V[j] = cvt2ptf(A[i+j]);
		p = Bezier(V, 3, 0.0, NULL, NULL);
		if (!i)
			fprintf(Outfile,"P0: %s\n", pic_fcoord(buf,p));
		for (step = 1; step <= BezierSubdivision; step++) {
			p = Bezier(V, 3, (float)step/BezierSubdivision, NULL, NULL);
			++m;
			fprintf(Outfile,"P%d: %s\n", m, pic_fcoord(buf,p));
		}
	}
	for (i=0; i+2 <= m; i+=2)	/* DWB 2 pic suffers from severe roundoff errors if too many steps are plotted at once */
		fprintf(Outfile,"move to P%d; line attrs%d to P%d then to P%d\n", i, SP, i+1, i+2); /* use line, as splines can't be dotted or dashed */
}

static void
pic_comment(void* obj, attrsym_t* sym)
{
	char	*str;

	debug("pic_comment");
	str = late_string(obj,sym,"");
	if (str[0]) fprintf(Outfile,"'\\\" %s\n",str);
}

codegen_t	PIC_CodeGen = {
	pic_reset,
	pic_begin_job, pic_end_job,
	pic_begin_graph, pic_end_graph,
	pic_begin_page, pic_end_page,
	pic_begin_cluster, pic_end_cluster,
	pic_begin_nodes, pic_end_nodes,
	pic_begin_edges, pic_end_edges,
	pic_begin_node, pic_end_node,
	pic_begin_edge, pic_end_edge,
	pic_begin_context, pic_end_context,
	pic_set_font, pic_textline,
	pic_set_color, pic_set_style,
	pic_ellipse, pic_polygon,
	pic_bezier, pic_polyline,
	0 /* pic_arrowhead */, pic_user_shape,
	pic_comment, 0 /* textsize */
};

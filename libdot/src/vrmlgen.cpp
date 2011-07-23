/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/
#include		"render.h"
#include		"gd.h"
#include		"utils.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

extern char *get_ttf_fontpath(char *fontreq, int warn);

#ifdef HAVE_LIBPNG

#ifndef MAXFLOAT
#define MAXFLOAT 10000000.
#endif

#define		NONE		0
#define		NODE		1
#define		EDGE		2
#define		CLST		3

#define SCALE (GD_RESOLUTION/72.0)

#define BEZIERSUBDIVISION 10

/* font modifiers */
#define REGULAR 0
#define BOLD		1
#define ITALIC		2

/* patterns */
#define P_SOLID		0
#define P_NONE  15
#define P_DOTTED 4				/* i wasn't sure about this */
#define P_DASHED 11				/* or this */

/* bold line constant */
#define WIDTH_NORMAL 1
#define WIDTH_BOLD 3

typedef struct {
	unsigned char r, g, b;
} Color;


static FILE	*Outfile;
static int	  Obj;
/* static int	 N_pages; */
/* static point	Pages; */
static double   Scale = 1.0;
static int	  Rot;
static box	  PB;
/* static int	  onetime = TRUE; */
static int		Saw_skycolor;

static gdImagePtr PNGimage;
static FILE		*PNGfile;
static node_t	*Curnode;
static edge_t	*Curedge;

typedef struct context_t {
	char			*color, *fontfam, fontopt, font_was_set;
	char			color_ix;
	double			r,g,b;
	char			pen, fill, penwidth;
	float		   fontsz;
} context_t;

#define MAXNEST 4
static context_t cstk[MAXNEST];
static int	  SP;

static double dist2(pointf p, point q)
{
	return ((p.x - q.x) * (p.x - q.x) + (p.y - q.y) * (p.y - q.y));
}


static char *nodeURL(node_t *n, char *buf)
{
	sprintf(buf,"node%d.png",n->id);
	return buf;
}

/* gdirname:
 * Returns directory pathname prefix
 * Code adapted from dgk
 */
static char*
gdirname (char* pathname)
{
    char* last;

    /* go to end of path */
    for(last=pathname; *last; last++);
    /* back over trailing '/' */
    while(last>pathname && *--last=='/');
    /* back over non-slash chars */
    for(;last>pathname && *last!='/';last--);
    if(last==pathname)
    {
        /* all '/' or "" */
        if(*pathname!='/')
            *last = '.';
        /* preserve // */
        else if(pathname[1]=='/')
            last++;
    }
    else
    {
        /* back over trailing '/' */
        for(;*last=='/' && last > pathname; last--);
        /* preserve // */
        if(last==pathname && *pathname=='/' && pathname[1]=='/')
            last++;
    }
    last++;
    *last = '\0';

    return pathname;
}

static char *nodefilename(node_t *n, char *buf)
{
	static char *dir;
	static char disposable[1024];
	char junkbuf[1024];

	if (dir == 0) {
		if (Output_file_name)
			dir = gdirname(strcpy(disposable,Output_file_name));
		else dir = ".";
	}
	sprintf(buf,"%s/%s", dir, nodeURL(n,junkbuf));
	return buf;
}

static FILE *nodefile(node_t *n)
{
	FILE	*rv;
	char	buf[1024];

	rv = fopen(nodefilename(n,buf),"w");
	return rv;
}

static void vrml_reset(void)
{
	/* onetime = TRUE; */
}

static void init_png(gdImagePtr im)
{
	int transparent;

	if ((transparent = gdImageGetTransparent(im)) == -1) {
		transparent = gdImageColorResolve(im, 255, 255, 254);
		gdImageColorTransparent(im, transparent);
	}
}

static pointf pngpt(pointf p)
{
	pointf		  tmp, rv;
	tmp.x = p.x * Scale;
	tmp.y = Scale * p.y;
	if (Rot == 0) {
		rv.x = tmp.x;
		rv.y = PB.UR.y - PB.LL.y - tmp.y;
	} else {
		rv.x = PB.UR.x - PB.LL.x - tmp.y;
		rv.y = PB.UR.y - PB.LL.y - tmp.x;
	}
	return rv;
}

static void vrml_font(context_t* cp)
{
/* FIX
	char		   *fw, *fa;

	fw = fa = "Regular";
	switch (cp->fontopt) {
	case BOLD:
		fw = "Bold";
		break;
	case ITALIC:
		fa = "Italic";
		break;
	}
*/
}

/* warmed over VRML code starts here */

static void vrml_begin_job(FILE *ofp, graph_t *g, char **lib, char *user,
char *info[], point pages)
{
	Outfile = ofp;

	fprintf(Outfile, "#VRML V2.0 utf8\n");
}

static void vrml_end_job(void)
{
}

static void vrml_begin_graph(graph_t* g, box bb, point pb)
{
	g = g;
	PB.LL.x = bb.LL.x * SCALE;
	PB.LL.y = bb.LL.y * SCALE;
	PB.UR.x = bb.UR.x * SCALE;
	PB.UR.y = bb.UR.y * SCALE;

	Saw_skycolor = FALSE;
	fprintf(Outfile, "Group { children [\n");
	fprintf(Outfile,"  Viewpoint {position %.3lf %.3lf 10}\n",
		.0278*(bb.UR.x+bb.LL.x)/2.0, .0278*(bb.UR.y+bb.LL.y)/2.0);
	fprintf(Outfile,"  Transform {\n");
	fprintf(Outfile,"    scale %.3lf %.3lf %.3lf\n",
		.0278 , .0278 , .0278 );
	fprintf(Outfile,"    children [\n");

	SP = 0;
	cstk[0].color = "white";
	cstk[0].fontfam = "times";		/* font family name */
	cstk[0].fontopt = REGULAR;		/* modifier: REGULAR, BOLD or ITALIC */
	cstk[0].pen = P_SOLID;		/* pen pattern style, default is solid */
	cstk[0].fill = P_NONE;
	cstk[0].penwidth = WIDTH_NORMAL;
}

static void vrml_end_graph(void)
{
	if (!Saw_skycolor) 
		fprintf(Outfile," Background { skyColor 1 1 1 }\n");
	fprintf(Outfile,"  ] }\n");
	fprintf(Outfile, "] }\n");
}

static void vrml_begin_page(point page, double scale, int rot, point offset)
{
	Scale = scale * SCALE;
	Rot = rot;
}

static void vrml_end_page(void)
{
}

static void
vrml_begin_cluster(graph_t* g)
{
	Obj = CLST;
}

static void
vrml_end_cluster(void)
{
	Obj = NONE;
}

static void
vrml_begin_nodes(void)        
{
	Obj = NONE;
}
    
static void
vrml_end_nodes(void)  
{
	Obj = NONE;
}

static void 
vrml_begin_edges(void)        
{
	Obj = NONE;
}            

static void
vrml_end_edges(void)  
{            
	Obj = NONE;
}            

static void
vrml_begin_node(node_t* n)
{
	int		width, height;
	Obj = NODE;

	PNGfile = nodefile(n);
	width = n->u.lw + n->u.rw;
	height = n->u.ht;
	PNGimage = gdImageCreate(width, height);
	init_png(PNGimage);
	Curnode = n;
}

static void
vrml_end_node(void)
{
	Obj = NONE;

	gdImagePng(PNGimage, PNGfile);
#ifdef SOMETHINGBAD
	gdImageDestroy(PNGimage);
#endif
	PNGimage = 0;
	fclose(PNGfile);
}

static  void
vrml_begin_edge(edge_t* e)
{
	Obj = EDGE;
	Curedge = e;
	fprintf(Outfile," Group { children [\n");
}

static  void
vrml_end_edge(void)
{
	fprintf(Outfile,"] }\n");
	Obj = NONE;
}

static  void
vrml_begin_context(void)
{
	assert(SP + 1 < MAXNEST);
	cstk[SP + 1] = cstk[SP];
	SP++;
}

static  void
vrml_end_context(void)
{
	int			 psp = SP - 1;
	assert(SP > 0);
	if (cstk[SP].font_was_set)
		vrml_font(&(cstk[psp]));
	/* free(cstk[psp].fontfam); */
	SP = psp;
}

static  void
vrml_set_font(char* name, double size)
{
	char		   *p, *q, buf[SMALLBUF];
	context_t	  *cp;

	cp = &(cstk[SP]);
	cp->font_was_set = TRUE;
	cp->fontsz = size;
	p = strdup(name);
	if ((q = strchr(p, '-'))) {
		*q++ = 0;
		canoncolor(q, buf);
		if (streq(buf, "italic"))
			cp->fontopt = ITALIC;
		else if (streq(q, "bold"))
			cp->fontopt = BOLD;
	}
	cp->fontfam = p;
	vrml_font(&cstk[SP]);
}

/***** Arrowheads now centralized in emit.c
static  void
vrml_arrowhead(point p, double theta, double scale, int flag) {}
******/

static void vrml_set_color(char* name)
{
	double r,g,b;
	double h,s,v;
	int	R,G,B, color;
	char   result[SMALLBUF];
 
	cstk[SP].color = name;

	colorxlate(name,result);
	if ((sscanf(result,"%lf %lf %lf", &h, &s, &v)) != 3) {
	  fprintf(stderr, "Unknown color %s; using black\n", name);
	  h = s = v = 0.0;
	}
	hsv2rgb(&r,&g,&b,h,s,v);
	cstk[SP].r = r;
	cstk[SP].g = g;
	cstk[SP].b = b;

	if (Obj == NODE) {
		R = (int)(r*255);
		G = (int)(g*255);
		B = (int)(b*255);

		color=gdImageColorResolve(PNGimage,R,G,B);
		cstk[SP].color_ix = color;
	}
}

static  void
vrml_set_style(char** s)
{
	char		   *line;
	context_t	  *cp;

	cp = &(cstk[SP]);
	while ((line = *s++)) {
		if (streq(line, "solid")) {		/* no-op */
		} else if (streq(line, "dashed"))
			cp->pen = P_DASHED;
 		else if (streq(line, "dotted"))
 			cp->pen = P_DOTTED;
 		else if (streq(line, "bold"))
 			cp->penwidth = WIDTH_BOLD;
		else if (streq(line, "invis"))
			cp->pen = P_NONE;
		else if (streq(line, "filled"))
			cp->fill = P_SOLID;
		else if (streq(line, "unfilled")) {		/* no-op */
		} else {
			fprintf(stderr, "vrml_set_style: unsupported style %s - ignoring\n",
					line);
		}
	}
}

static char	*
vrml_string(char *s, char *auxbuf)
{
	char		   *p = auxbuf;
#if 0
	char		 esc;
#endif
	while (*s) {
#if 0
		esc = 0;
		switch (*s) {
		case '\t':
			esc = 't';
			break;
		case '>':
		case '\'':
		case '`':
		case '\\':
			esc = *s;
			break;
		}
		if (esc) {
			*p++ = '\\';
			*p++ = esc;
		} else
#endif
			*p++ = *s;
		s++;
	}
	*p = '\0';
	return auxbuf;
}

static void
vrml_textline(point p, char *str, int width, double fontsz, double align)
{
	char		buf[BUFSIZ];
	char		color;
	char		*fontlist;
	pointf		mp;
	int		brect[8];
	char		*error;

	if (Obj != NODE) return;

	fontlist = gd_alternate_fontlist(cstk[SP].fontfam);

	/* set p relative to the origin */
	p.x -= Curnode->u.coord.x;
	p.y -= Curnode->u.coord.y;

	/* set mp.x to left side of label */
	if (align == -.5)
		mp.x = p.x - width/2.0; /* normal - center on p*/
	else if (align < 0.0)
		mp.x = p.x - width;	/* right justify - left of p */
	else
		mp.x = p.x;		/* left justify - right of p */

	/* set mp.x relative to PNG canvas */
	mp.x = mp.x + Curnode->u.lw;

	/* set mp.y relative to PNG canvas */
	mp.y = Curnode->u.ht/2 - p.y + fontsz/2.0;

	color= cstk[SP].color_ix;
	error= gdImageStringFT(PNGimage, brect, color, fontlist, 
		fontsz, (Rot? 90.0 : 0.0) * PI / 180.0,
		ROUND(mp.x), ROUND(mp.y), vrml_string(str,buf));
}

static point vrml_textsize(char *str, char *fontname, double fontsz)
{
	char		*fontlist,buf[BUFSIZ];
	point		rv;
	int			brect[8];

	fontlist = gd_alternate_fontlist(fontname);
	if (fontlist && *str) {
/* call gdImageStringFT with invalid color to get brect (gdTransparent is not invalid) */
		gdImageStringFT(NULL, brect, -1, fontlist, 
			ROUND(fontsz), 0, 0, 0, vrml_string(str,buf));
		rv.x = (brect[4] - brect[0]);
		rv.y = (brect[5] - brect[1]);
	}
	else {
		rv.x = rv.y = 0;
	}
	return rv;
}

static double interpolate_zcoord(pointf p1, point fst, double fstz, point snd, double sndz)
{
	double	rv;

	if (fstz == sndz) return fstz;
#ifdef FIX
	if (Curedge->tail->u.rank != Curedge->head->u.rank)
		rv = fstz + (sndz - fstz) * (p1.y - fst.y)/(snd.y - fst.y);
	else
#endif
		rv = fstz + (sndz - fstz) * (p1.x - fst.x)/(snd.x - fst.x);
	return rv;
}

static  void
vrml_bezier(point* A, int n, int arrow_at_start, int arrow_at_end)
{
	pointf		p1, V[4];
	int			i, j, step;
	double		fstz, sndz;
	context_t	  *cp;

	assert(Obj == EDGE);

	cp = &(cstk[SP]);
	if (cp->pen == P_NONE) return;
	fstz = late_float(Curedge->tail, N_z, 0.0, -1000.0);
	sndz = late_float(Curedge->head, N_z, 0.0, -MAXFLOAT);
	fprintf(Outfile,"Shape { geometry Extrusion  {\n");
	fprintf(Outfile," spine [ \n");
	V[3].x = A[0].x; V[3].y = A[0].y;
	for (i = 0; i+3 < n; i += 3) {
		V[0] = V[3];
		for (j = 1; j <= 3; j++) {
			V[j].x  = A[i+j].x; V[j].y = A[i+j].y;
		}
		for (step = 0; step <= BEZIERSUBDIVISION; step++) {
			p1 = Bezier(V, 3, (double)step/BEZIERSUBDIVISION, NULL, NULL);
			fprintf(Outfile," %.3lf %.3lf %.3lf", p1.x, p1.y,
				interpolate_zcoord(p1,A[0],fstz,A[n-1],sndz));
		}
	}
	fprintf(Outfile," ]\n");
	fprintf(Outfile, "  crossSection [ %d %d, %d %d, %d %d, %d %d ]\n",
		(cp->penwidth), (cp->penwidth), -(cp->penwidth), (cp->penwidth),
		-(cp->penwidth), -(cp->penwidth), (cp->penwidth), -(cp->penwidth));
	fprintf(Outfile,"}\n");
	fprintf(Outfile," appearance DEF E%d Appearance {\n",Curedge->id);
	fprintf(Outfile,"   material Material {\n");
	fprintf(Outfile,"   ambientIntensity 0.33\n");
	fprintf(Outfile,"   diffuseColor %.3lf %.3lf %.3lf\n",
		cstk[SP].r, cstk[SP].g, cstk[SP].b);
	fprintf(Outfile,"   }\n");
	fprintf(Outfile," }\n");
	fprintf(Outfile,"}\n");
}

static  void
vrml_polygon(point *A, int n, int filled)
{
	pointf		  p;
	int			 i;
	double		theta, z;
	node_t		*endp;
	/* gdPoint		points[500]; Naughty.  Should allocate n */
	char		somebuf[1024];
	/* pointf		  mp; */

	switch (Obj) {
	case NONE:	 /* GRAPH */
		fprintf(Outfile," Background { skyColor %.3lf %.3lf %.3lf }\n",
			cstk[SP].r, cstk[SP].g, cstk[SP].b);
		Saw_skycolor = TRUE;
		break;

	case NODE:

	if (filled) {
		gdImageFill(PNGimage, 1, 1, cstk[SP].color_ix);

		/* FIXME - horrible hack!!
		 * We happen to know (since the changes that separated
		 * pencolor and fillcolor) that if filling is on
		 * we're going to get called again to do the periphery.
		 * So return now and emit the vrml code then.
		 *
		 * The proper fix is probably to add a flag to the
		 * calling parameters so that shapes.c can indicate
		 * when it is drawing the boundary polygon.
		 * This could also be useful to ismapgen.c and vtxgen.c
		 * or any other renderer that needs a boundary.
		 *
		 * Also then vrmlgen.c could support node images with 
		 * multiple peripheries allowing that other horrible
		 * hack in shapes.c to be removed.
		 */
		return;
	}

	z = late_float(Curnode,N_z,0.0,-MAXFLOAT);

	fprintf(Outfile,"Shape {\n");
	fprintf(Outfile,"  appearance Appearance {\n");
	fprintf(Outfile,"    material Material {\n");
	fprintf(Outfile,"      ambientIntensity 0.33\n");
	fprintf(Outfile,"        diffuseColor 1 1 1\n");
	fprintf(Outfile,"    }\n");
	fprintf(Outfile,"    texture ImageTexture { url \"%s\" }\n",
		nodeURL(Curnode,somebuf));
	fprintf(Outfile,"  }\n");
	fprintf(Outfile,"  geometry Extrusion {\n");
	fprintf(Outfile,"    crossSection [ ");
	for (i = 0; i < n; i++) {
		p.x = A[i].x - Curnode->u.coord.x;
		p.y = A[i].y - Curnode->u.coord.y;
		fprintf(Outfile," %.3lf %.3lf%c", p.x, p.y, (i < n - 1? ',':' '));
	}
	fprintf(Outfile," ]\n");
	fprintf(Outfile, "   spine [ %d %d %.3lf, %d %d %.3lf ]\n",
		Curnode->u.coord.x, Curnode->u.coord.y, z - .01,
		Curnode->u.coord.x, Curnode->u.coord.y, z + .01);
	fprintf(Outfile,"  }\n");
	fprintf(Outfile,"}\n");
	break;

	case EDGE:

		if (cstk[SP].pen == P_NONE) return;
		p.x = p.y = 0.0;
		for (i = 0; i < n; i++) {
			p.x += A[i].x;
			p.y += A[i].y;
		}
		p.x = p.x / n; p.y = p.y / n;

		/* it is bad to know that A[1] is the aiming point, but we do */
		theta = atan2((A[0].y + A[2].y)/2.0 - A[1].y, (A[0].x + A[2].x)/2.0 - A[1].x) + PI / 2.0;


		/* this is gruesome, but how else can we get z coord */
		if (dist2(p,Curedge->tail->u.coord) < dist2(p,Curedge->head->u.coord))
			endp = Curedge->tail;
		else
			endp = Curedge->head;
		z = late_float(endp,N_z,0.0,-MAXFLOAT);

		/* FIXME: arrow vector ought to follow z coord of bezier */
		fprintf(Outfile,"Transform {\n");
		fprintf(Outfile,"  translation %.3lf %.3lf %.3lf\n", p.x, p.y, z);
		fprintf(Outfile,"  children [\n");
		fprintf(Outfile,"    Transform {\n");
		fprintf(Outfile,"      rotation 0 0 1 %.3lf\n", theta);
		fprintf(Outfile,"      children [\n");
		fprintf(Outfile,"        Shape {\n");
		fprintf(Outfile,"          geometry Cone {bottomRadius %.3lf height %.3lf }\n",cstk[SP].penwidth * 2.5,cstk[SP].penwidth * 10.0);
		fprintf(Outfile,"          appearance USE E%d\n",Curedge->id);
		fprintf(Outfile,"        }\n");
		fprintf(Outfile,"      ]\n");
		fprintf(Outfile,"    }\n");
		fprintf(Outfile,"  ]\n");
		fprintf(Outfile,"}\n");
		break;
	default:
		break;
	}
}

static  void
vrml_ellipse(point p, int rx, int ry, int filled)
{
	pointf		  mp;
	double		  z;
	char		somebuf[1024];

	assert(Obj == NODE);

	mp.x = p.x; mp.y = p.y;

		if (filled) {
			gdImageFill(PNGimage, 1, 1, cstk[SP].color_ix);
		}

	z = late_float(Curnode,N_z,0.0,-MAXFLOAT);
	fprintf(Outfile,"Transform {\n");
	fprintf(Outfile,"  translation %.3lf %.3lf %.3lf\n", mp.x, mp.y, z);
	fprintf(Outfile,"  scale %d %d 1\n", rx, ry);
	fprintf(Outfile,"  children [\n");
	fprintf(Outfile,"    Transform {\n");
    fprintf(Outfile,"      rotation 1 0 0   1.57\n");
	fprintf(Outfile,"      children [\n");
	fprintf(Outfile,"        Shape {\n");
	fprintf(Outfile,"          geometry Cylinder { side FALSE }\n");
	fprintf(Outfile,"          appearance Appearance {\n");
	fprintf(Outfile,"            material Material {\n");
	fprintf(Outfile,"              ambientIntensity 0.33\n");
	fprintf(Outfile,"              diffuseColor 1 1 1\n");
	fprintf(Outfile,"            }\n");
	fprintf(Outfile,"            texture ImageTexture { url \"%s\" }\n",
		nodeURL(Curnode,somebuf));
	fprintf(Outfile,"          }\n");
	fprintf(Outfile,"        }\n");
	fprintf(Outfile,"      ]\n");
	fprintf(Outfile,"    }\n");
	fprintf(Outfile,"  ]\n");
	fprintf(Outfile,"}\n");
}

static  void
vrml_polyline(point* A, int n)
{
/*
	pointf		  p, p1;
	int			 i;

	if (cstk[SP].pen != P_NONE) {
		p.x = A[0].x;
		p.y = A[0].y;
		for (i = 1; i < n; i++) {
			p1.x = A[i].x;
			p1.y = A[i].y;
#ifdef NONEOFTHISEITHER
			if (cstk[SP].pen == P_DASHED) {
				gdImageDashedLine(im, ROUND(p.x), ROUND(p.y),
					ROUND(p1.x), ROUND(p1.y), cstk[SP].color_ix);
			} else {
				gdImageLine(im, ROUND(p.x), ROUND(p.y),
					ROUND(p1.x), ROUND(p1.y), cstk[SP].color_ix);
			}
#endif
			p.x = p1.x;
			p.y = p1.y;
		}
	}
*/
}

static  void
vrml_user_shape(char *name, point *A, int n, int filled)
{
	vrml_polygon(A, n, filled);
}

codegen_t	   VRML_CodeGen = {
	vrml_reset,
	vrml_begin_job, vrml_end_job,
	vrml_begin_graph, vrml_end_graph,
	vrml_begin_page, vrml_end_page,
	vrml_begin_cluster, vrml_end_cluster,
	vrml_begin_nodes, vrml_end_nodes,
	vrml_begin_edges, vrml_end_edges,
	vrml_begin_node, vrml_end_node,
	vrml_begin_edge, vrml_end_edge,
	vrml_begin_context, vrml_end_context,
	vrml_set_font, vrml_textline,
	vrml_set_color, vrml_set_style,
	vrml_ellipse, vrml_polygon,
	vrml_bezier, vrml_polyline,
	0/* arrowhead */, vrml_user_shape,
	0 /* comment */, vrml_textsize
};
#endif /* HAVE_LIBPNG */

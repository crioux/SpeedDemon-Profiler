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
#include		<fcntl.h>

#ifdef DMALLOC
#include "dmalloc.h"
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
/* static int	  Obj; */
#if 0
static int	 N_pages;
static point	Pages;
#endif
static double   Scale;
static pointf	Offset;
static double   ArgScale;
static int	  Rot;
static box	  PB;
static int	  onetime = TRUE;

static gdImagePtr im;

typedef struct context_t {
	char	color_ix, *fontfam, fontopt, font_was_set;
	char			pen, fill, penwidth;
	float		   fontsz;
} context_t;

#define MAXNEST 4
static context_t cstk[MAXNEST];
static int	  SP;

static void gd_reset(void)
{
	onetime = TRUE;
}

static void init_gd(void)
{
	int white, transparent, black;

	SP = 0;
	/* must create default background color first... */
	white = gdImageColorResolve(im, 255, 255, 255);
	if ((transparent = gdImageGetTransparent(im)) == -1) {
		/* transparent uses an rgb value very close to white
		   so that formats like GIF that don't support
		   transparency show a white background */
		transparent = gdImageColorResolve(im, 255, 255, 254);
		gdImageColorTransparent(im, transparent);
	}
	/* ...then any other colors that we need */
	black = gdImageColorResolve(im, 0, 0, 0);
	cstk[0].color_ix = black;		/* set pen black*/
	cstk[0].fontfam = "times";		/* font family name */
	cstk[0].fontopt = REGULAR;		/* modifier: REGULAR, BOLD or ITALIC */
	cstk[0].pen = P_SOLID;		/* pen pattern style, default is solid */
	cstk[0].fill = P_NONE;
	cstk[0].penwidth = WIDTH_NORMAL;
}

pointf gdpt(pointf p)
{
	pointf		  rv;

	if (Rot == 0) {
		rv.x = p.x * Scale + Offset.x;
		rv.y = PB.UR.y - PB.LL.y - p.y * Scale + Offset.y;
	} else {
		rv.x = PB.UR.x - PB.LL.x - p.y * Scale + Offset.x;
		rv.y = PB.UR.y - PB.LL.y - p.x * Scale + Offset.y;
	}
	return rv;
}

void gd_font(context_t* cp)
{
/*
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

static void gd_begin_job(FILE *ofp, graph_t *g, char **lib, char *user,
char *info[], point pages)
{
	Outfile = ofp;
	/* Pages = pages; */
	/* N_pages = pages.x * pages.y; */
}

static void gd_end_job(void) { }

static void gd_begin_graph_to_file(graph_t* g, box bb, point pb)
{
	g = g;
	PB.LL.x = bb.LL.x * SCALE;
	PB.LL.y = bb.LL.y * SCALE;
	PB.UR.x = bb.UR.x * SCALE;
	PB.UR.y = bb.UR.y * SCALE;
	Offset.x = PB.LL.x + 1;
	Offset.y = PB.LL.y + 1;
	if (Verbose)
		fprintf(stderr,"dot: allocating a %dK GD image\n",
			(PB.UR.x + PB.LL.x + 2) * (PB.UR.y + PB.LL.y + 2) / 1024);
	im = gdImageCreate((PB.UR.x + PB.LL.x + 2), (PB.UR.y + PB.LL.y + 2));
	if (onetime) {
		init_gd();
		onetime = FALSE;
	}
}

static void gd_begin_graph_to_memory(graph_t* g, box bb, point pb)
{
	g = g;
	PB.LL.x = bb.LL.x * SCALE;
	PB.LL.y = bb.LL.y * SCALE;
	PB.UR.x = bb.UR.x * SCALE;
	PB.UR.y = bb.UR.y * SCALE;
	Offset.x = PB.LL.x + 1;
	Offset.y = PB.LL.y + 1;
	if (Verbose)
		fprintf(stderr,"dot: using existing GD image\n");
	im = *(gdImagePtr *)Output_file;
	if (onetime) {
		init_gd();
		onetime = FALSE;
	}
}

static void gd_end_graph_to_file(void)
{
/*
 * Windows will do \n -> \r\n  translations on stdout unless told otherwise.
 */
#ifdef HAVE_SETMODE
#ifdef O_BINARY
	setmode(fileno(Outfile), O_BINARY);
#endif
#endif

/*
 * Write IM to OUTFILE as a JFIF-formatted JPEG image, using quality
 * JPEG_QUALITY.  If JPEG_QUALITY is in the range 0-100, increasing values
 * represent higher quality but also larger image size.  If JPEG_QUALITY is
 * negative, the IJG JPEG library's default quality is used (which
 * should be near optimal for many applications).  See the IJG JPEG
 * library documentation for more details.  */

#define JPEG_QUALITY -1

	if (Output_lang == GIF) {
//		gdImageGif(im, Outfile);
#ifdef HAVE_LIBPNG
#ifdef HAVE_LIBZ
	} else if (Output_lang == PNG) {
		gdImagePng(im, Outfile);
#endif
#endif
#ifdef HAVE_LIBJPEG
	} else if (Output_lang == JPEG) {
		gdImageJpeg(im, Outfile, JPEG_QUALITY);
#endif
	} else if (Output_lang == GD) {
		gdImageGd(im, Outfile);
/*	} else if (Output_lang == GD2) {
#define GD2_CHUNKSIZE 128
#define GD2_RAW 1
#define GD2_COMPRESSED 2
		gdImageGd2(im, Outfile, GD2_CHUNKSIZE, GD2_COMPRESSED);
 */
	} else if (Output_lang == WBMP) {
        	/* Use black for the foreground color for the B&W wbmp image. */
        	int foreground = gdImageColorResolve(im, 0, 0, 0);
		gdImageWBMP(im, foreground, Outfile);
#ifdef HAVE_LIBXPM
	} else if (Output_lang == XBM) {
		gdImageXbm(im, Outfile);
#endif
	}
	gdImageDestroy(im);
}

static void gd_end_graph_to_memory(void)
{
/* leave image in memory to be handled by Gdtclft output routines */
}

static void
gd_begin_page(point page, double scale, int rot, point offset)
{
#if 0
	int			 page_number;
	point		sz;
#endif

	ArgScale = scale;
	Scale = scale * SCALE;
	Rot = rot;
	/* i guess we would use this if we were displaying page numbers */
#if 0
	page_number = page.x + page.y * Pages.x + 1;
	sz = sub_points(PB.UR, PB.LL);
#endif
}

static void gd_end_page(void) { }

static void
gd_begin_cluster(graph_t* g)
{
	/* Obj = CLST; */
}

static void
gd_end_cluster(void)
{
	/* Obj = NONE; */
}

static void
gd_begin_nodes(void)
{
	/* Obj = NONE; */
}

static void
gd_end_nodes(void)
{
	/* Obj = NONE; */
}

static void
gd_begin_edges(void)
{
	/* Obj = NONE; */
}

static void
gd_end_edges(void)
{
	/* Obj = NONE; */
}

static void
gd_begin_node(node_t* n)
{
	/* Obj = NODE; */
}

static void
gd_end_node(void)
{
	/* Obj = NONE; */
}

static  void
gd_begin_edge(edge_t* e)
{
	/* Obj = EDGE; */
}

static  void
gd_end_edge(void)
{
	/* Obj = NONE; */
}

static  void
gd_begin_context(void)
{
	assert(SP + 1 < MAXNEST);
	cstk[SP + 1] = cstk[SP];
	SP++;
}

static  void
gd_end_context(void)
{
	int			 psp = SP - 1;
	assert(SP > 0);
	if (cstk[SP].font_was_set)
		gd_font(&(cstk[psp]));
	SP = psp;
}

static  void
gd_set_font(char* name, double size)
{
	char		   *p, *q, buf[SMALLBUF];
	context_t	  *cp;

	cp = &(cstk[SP]);
	cp->font_was_set = TRUE;
	cp->fontsz = ArgScale * size;
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
	gd_font(&cstk[SP]);
}

/***** Arrowheads now centralized in emit.c
static  void
gd_arrowhead(point p, double theta, double scale, int flag)
{
	pointf		  p0, p1, p2;
	double		  v;
	gdPoint			points[3];

	if (cstk[SP].pen != P_NONE) {
		p0.x = p.x; p0.y = p.y;
		p0 = gdpt(p0);
		points[0].x = ROUND(p0.x); points[0].y = ROUND(p0.y);
		v = cos(RADIANS(theta+15)) * ARROW_LENGTH; p1.x = v + p.x;
		v = sin(RADIANS(theta+15)) * ARROW_LENGTH; p1.y = v + p.y;
		p1 = gdpt(p1);
		points[1].x = ROUND(p1.x); points[1].y = ROUND(p1.y);
		v = cos(RADIANS(theta-15)) * ARROW_LENGTH; p2.x = v + p.x;
		v = sin(RADIANS(theta-15)) * ARROW_LENGTH; p2.y = v + p.y;
		p2 = gdpt(p2);
		points[2].x = ROUND(p2.x); points[2].y = ROUND(p2.y);
		gdImageFilledPolygon(im, points, 3, cstk[SP].color_ix);
	}
}
******/

static void gd_set_color(char* name)
{
	double r,g,b;
	double h,s,v;
	int	R,G,B;
	char   result[SMALLBUF];
 
	if (!(strcmp(name,"transparent"))) {
	    	/* the colorxlate stuff seems to do some rounding...
		 * we need the exact rgb value for transparent.
		 */
		R = 255; G = 255; B = 254;
	} 
	else {
		colorxlate(name,result);
		if ((sscanf(result,"%lf %lf %lf", &h, &s, &v)) != 3) {
	  		fprintf(stderr, "Unknown color %s; using black\n", name);
	  		h = s = v = 0.0;
		}
		hsv2rgb(&r,&g,&b,h,s,v);
		R = (int)(r*255);
		G = (int)(g*255);
		B = (int)(b*255);
	}
	cstk[SP].color_ix = gdImageColorResolve(im,R,G,B);
}

static  void
gd_set_style(char** s)
{
	char		*line,*p;
	context_t	*cp;

	cp = &(cstk[SP]);
	while ((p = line = *s++)) {
		if (streq(line, "solid")) { /* no-op */ }
		else if (streq(line, "dashed"))
			cp->pen = P_DASHED;
                else if (streq(line, "dotted"))
 			cp->pen = P_DOTTED;
		else if (streq(line, "invis"))
			cp->pen = P_NONE;
 		else if (streq(line, "bold"))
 			cp->penwidth = WIDTH_BOLD;
		else if (streq(line, "setlinewidth")) {
			while (*p) p++; p++;
			cp->penwidth = atol(p) / SCALE;
		}
		else if (streq(line, "filled"))
			cp->fill = P_SOLID;
		else if (streq(line, "unfilled")) { /* no-op */ }
		else fprintf(stderr,
			"gd_set_style: unsupported style %s - ignoring\n",
			line);
	}
}

char	*
gd_string(char *s, char *auxbuf)
{
	char		   *p = auxbuf;
#if 0
	char		    esc;
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

/* sometimes fonts are stored under a different name */
char *
gd_alternate_fontlist(char *font) 
{
	char *fontlist;

	fontlist = font;
	if (stricmp(font,"Times-Roman")==0)
		fontlist = "Times-Roman TimesNewRoman Times times";
	else if (stricmp(font,"Times")==0)
		fontlist = "Times times Times-Roman TimesNewRoman";
	else if (stricmp(font,"TimesNewRoman")==0)
		fontlist = "TimesNewRoman Times-Roman Times times";
	else if (stricmp(font,"Helvetica")==0)
		fontlist = "Helvetica arial";
	else if (stricmp(font,"Arial")==0)
		fontlist = "Arial arial";
	else if (stricmp(font,"Courier")==0)
		fontlist = "Courier cour";
	return fontlist;
}

void gd_missingfont(char *fontreq)
{
	static char     *lastmissing = 0;
	static int      n_errors = 0;
	char		*p;

	if (n_errors >= 20) return;
	if ((lastmissing == 0) || (strcmp(lastmissing,fontreq))) {
		if (!(p=getenv("GDFONTPATH"))) p = DEFAULT_FONTPATH;
		fprintf(stderr, "can't find font %s in %s\n",fontreq,p);
		if (lastmissing) free(lastmissing);
		lastmissing = strdup(fontreq);
		n_errors++;
		if (n_errors >= 20) fprintf(stderr,"(font errors suppressed)\n");
	}
}


static  void
gd_textline(point p, char *str, int width, double fontsz, double align)
{
	char		buf[BUFSIZ];
	char		*fontlist, *err;
	pointf		mp;
	int		brect[8];
	extern          gdFontPtr       gdFontSmall;

	if (cstk[SP].pen == P_NONE) return;
	fontsz *= ArgScale;
	fontlist = gd_alternate_fontlist(cstk[SP].fontfam); 
	gd_string(str,buf);
	if (align == -.5)
		mp.x = p.x - width/2.0; /* normal - center on p*/
	else if (align < 0.0)
		mp.x = p.x - width;	/* right justify - left of p */
	else
		mp.x = p.x;		/* left justify - right of p */
	mp.y = p.y - fontsz/3.0;
	mp = gdpt(mp);
	err = gdImageStringFT(im, brect, cstk[SP].color_ix, fontlist, 
		fontsz, (Rot? 90.0 : 0.0) * PI / 180.0,
		ROUND(mp.x), ROUND(mp.y), gd_string(str,buf));
	if (err) {
		/* revert to builtin fonts */
		if (align == -.5)
			mp.x = p.x - width/2.0; /* normal - center on p*/
		else if (align < 0.0)
			mp.x = p.x - width;	/* right justify - left of p */
		else
			mp.x = p.x;	/* left justify - right of p */
		mp.y = p.y + 2.0;
		mp = gdpt(mp);
	    	gd_missingfont (cstk[SP].fontfam);
		gdImageString(im, gdFontSmall,
			ROUND(mp.x), ROUND(mp.y),
			(unsigned char *)gd_string(str,buf),
			cstk[SP].color_ix);
	}
}

point gd_textsize(char *str, char *fontname, double fontsz)
{
	char		*fontlist,buf[BUFSIZ],*err;
	point		rv;
	int		brect[8];

	fontlist = gd_alternate_fontlist(fontname);
	
	rv.x = rv.y = 0;
	if (fontlist && *str) {
		/* call gdImageStringFT with null *im to get brect */
		err = gdImageStringFT(NULL, brect, -1, fontlist, 
			fontsz, 0, 0, 0, gd_string(str,buf));
		if (!err) {
			rv.x = (brect[4] - brect[0]);
/*			rv.y = (brect[5] - brect[1]); */
			rv.y = (brect[5] - 0       ); /* ignore descenders */
		}
	}
	else { rv.x = rv.y = 0; }
	rv.x /= SCALE; rv.y /= SCALE;
	return rv;
}

static  void
gd_bezier(point* A, int n, int arrow_at_start, int arrow_at_end)
{
	pointf		p0, p1, V[4];
	int		i, j, step;
	int		style[20]; 
	int		pen, width;
	gdImagePtr	brush = NULL;

	if (cstk[SP].pen != P_NONE) {
		if (cstk[SP].pen == P_DASHED) {
			for (i = 0; i < 10; i++)
				style[i] = cstk[SP].color_ix;
			for (; i < 20; i++)
				style[i] = gdTransparent;
			gdImageSetStyle(im, style, 20);
			pen = gdStyled;
		} else if (cstk[SP].pen == P_DOTTED) {
			for (i = 0; i < 2; i++)
				style[i] = cstk[SP].color_ix;
			for (; i < 12; i++)
				style[i] = gdTransparent;
			gdImageSetStyle(im, style, 12);
			pen = gdStyled;
		} else {
			pen = cstk[SP].color_ix;
		}
                if (cstk[SP].penwidth != WIDTH_NORMAL) {
			width=cstk[SP].penwidth;
                        brush = gdImageCreate(width,width);
                        gdImagePaletteCopy(brush, im);
                        gdImageFilledRectangle(brush,
                           0,0,width-1, width-1, cstk[SP].color_ix);
                        gdImageSetBrush(im, brush);
			if (pen == gdStyled) pen = gdStyledBrushed;      
			else pen = gdBrushed;      
		}
		V[3].x = A[0].x; V[3].y = A[0].y;
		for (i = 0; i+3 < n; i += 3) {
			V[0] = V[3];
			for (j = 1; j <= 3; j++) {
				V[j].x  = A[i+j].x; V[j].y = A[i+j].y;
			}
			p0 = gdpt(V[0]); 
			for (step = 1; step <= BEZIERSUBDIVISION; step++) {
				p1 = gdpt(Bezier(V, 3, (double)step/BEZIERSUBDIVISION, NULL, NULL));
				gdImageLine(im, ROUND(p0.x), ROUND(p0.y),
					ROUND(p1.x), ROUND(p1.y), pen);
				p0 = p1;
			}
		}
		if (brush)
			gdImageDestroy(brush);
	}
}

static  void
gd_polygon(point *A, int n, int filled)
{
	pointf		p;
	int		i;
	gdPoint		*points;
	int		style[20];
	int		pen, width;
	gdImagePtr	brush = NULL;

	if (cstk[SP].pen != P_NONE) {
		if (cstk[SP].pen == P_DASHED) {
			for (i = 0; i < 10; i++)
				style[i] = cstk[SP].color_ix;
			for (; i < 20; i++)
				style[i] = gdTransparent;
			gdImageSetStyle(im, style, 20);
			pen = gdStyled;
		} else if (cstk[SP].pen == P_DOTTED) {
			for (i = 0; i < 2; i++)
				style[i] = cstk[SP].color_ix;
			for (; i < 12; i++)
				style[i] = gdTransparent;
			gdImageSetStyle(im, style, 12);
			pen = gdStyled;
		} else {
			pen = cstk[SP].color_ix;
		}
                if (cstk[SP].penwidth != WIDTH_NORMAL) {
			width=cstk[SP].penwidth;
                        brush = gdImageCreate(width,width);
                        gdImagePaletteCopy(brush, im);
                        gdImageFilledRectangle(brush,
                           0,0,width-1, width-1, cstk[SP].color_ix);
                        gdImageSetBrush(im, brush);
			if (pen == gdStyled) pen = gdStyledBrushed;      
			else pen = gdBrushed;      
		}
		points = (gdPoint *)malloc(n*sizeof(gdPoint));
		for (i = 0; i < n; i++) {
			p.x = A[i].x; p.y = A[i].y;
			p = gdpt(p);
			points[i].x = ROUND(p.x); points[i].y = ROUND(p.y);
		}
		if (filled)
			gdImageFilledPolygon(im, points, n, cstk[SP].color_ix);
		else 
			gdImagePolygon(im, points, n, pen);
		free(points);
		if (brush)
			gdImageDestroy(brush);
	}
}

static  void
gd_ellipse(point p, int rx, int ry, int filled)
{
	pointf		mp;
	int		i;
	int		style[40];  /* need 2* size for arcs, I don't know why */
	int		pen, width;
	gdImagePtr	brush = NULL;

	if (cstk[SP].pen != P_NONE) {
		if (cstk[SP].pen == P_DASHED) {
			for (i = 0; i < 20; i++)
				style[i] = cstk[SP].color_ix;
			for (; i < 40; i++)
				style[i] = gdTransparent;
			gdImageSetStyle(im, style, 40);
			pen = gdStyled;
		} else if (cstk[SP].pen == P_DOTTED) {
			for (i = 0; i < 2; i++)
				style[i] = cstk[SP].color_ix;
			for (; i < 24; i++)
				style[i] = gdTransparent;
			gdImageSetStyle(im, style, 24);
			pen = gdStyled;
		} else {
			pen = cstk[SP].color_ix;
		}
                if (cstk[SP].penwidth != WIDTH_NORMAL) {
			width = cstk[SP].penwidth;
                        brush = gdImageCreate(width,width);
                        gdImagePaletteCopy(brush, im);
                        gdImageFilledRectangle(brush,
                           0,0,width-1, width-1, cstk[SP].color_ix);
                        gdImageSetBrush(im, brush);
			if (pen == gdStyled) pen = gdStyledBrushed;      
			else pen = gdBrushed;      
		}
		if (Rot) {int t; t = rx; rx = ry; ry = t;}
		mp.x = p.x; mp.y = p.y;
		mp = gdpt(mp);
		if (filled) {
			gdImageArc(im, ROUND(mp.x), ROUND(mp.y),
				ROUND(Scale*(rx + rx)), ROUND(Scale*(ry + ry)),
				0, 360, cstk[SP].color_ix);
			gdImageFillToBorder(im, ROUND(mp.x), ROUND(mp.y),
				cstk[SP].color_ix, cstk[SP].color_ix);
		} else {
			gdImageArc(im, ROUND(mp.x), ROUND(mp.y),
				ROUND(Scale*(rx + rx)), ROUND(Scale*(ry + ry)),
				0, 360, pen);
		}
		if (brush)
			gdImageDestroy(brush);
	}
}

static  void
gd_polyline(point* A, int n)
{
	pointf		p, p1;
	int		i;
	int		style[20];
	int		pen, width;
	gdImagePtr	brush = NULL;

	if (cstk[SP].pen != P_NONE) {
		if (cstk[SP].pen == P_DASHED) {
			for (i = 0; i < 10; i++)
				style[i] = cstk[SP].color_ix;
			for (; i < 20; i++)
				style[i] = gdTransparent;
			gdImageSetStyle(im, style, 20);
			pen = gdStyled;
		} else if (cstk[SP].pen == P_DOTTED) {
			for (i = 0; i < 2; i++)
				style[i] = cstk[SP].color_ix;
			for (; i < 12; i++)
				style[i] = gdTransparent;
			gdImageSetStyle(im, style, 12);
			pen = gdStyled;
		} else {
			pen = cstk[SP].color_ix;
		}
                if (cstk[SP].penwidth != WIDTH_NORMAL) {
			width = cstk[SP].penwidth;
                        brush = gdImageCreate(width,width);
                        gdImagePaletteCopy(brush, im);
                        gdImageFilledRectangle(brush,
                           0,0,width-1,width-1,cstk[SP].color_ix);
                        gdImageSetBrush(im, brush);
			if (pen == gdStyled) pen = gdStyledBrushed;      
			else pen = gdBrushed;      
		}
		p.x = A[0].x;
		p.y = A[0].y;
		p = gdpt(p);
		for (i = 1; i < n; i++) {
			p1.x = A[i].x;
			p1.y = A[i].y;
			p1 = gdpt(p1);
			gdImageLine(im, ROUND(p.x), ROUND(p.y),
				ROUND(p1.x), ROUND(p1.y), pen);
			p.x = p1.x;
			p.y = p1.y;
		}
		if (brush)
			gdImageDestroy(brush);
	}
}

static  void
gd_user_shape(char *name, point *A, int n, int filled)
{
	static boolean  onetime = TRUE;
	if (onetime) {
		fprintf(stderr, "custom shapes not available with this driver\n");
		onetime = FALSE;
	}
	gd_polygon(A, n, filled);
}
codegen_t	GD_CodeGen = {
	gd_reset,
	gd_begin_job, 
	gd_end_job,
	gd_begin_graph_to_file, 
	gd_end_graph_to_file, 
	gd_begin_page, 
	gd_end_page,
	gd_begin_cluster, 
	gd_end_cluster,
	gd_begin_nodes, 
	gd_end_nodes,
	gd_begin_edges, 
	gd_end_edges,
	gd_begin_node, 
	gd_end_node,
	gd_begin_edge, 
	gd_end_edge,
	gd_begin_context, 
	gd_end_context,
	gd_set_font, 
	gd_textline,
	gd_set_color, 
	gd_set_style,
	gd_ellipse, 
	gd_polygon,
	gd_bezier, 
	gd_polyline,
	0/* gd_arrowhead */, 
	gd_user_shape,
	0 /* gd comment */, 
	gd_textsize
};
codegen_t	memGD_CodeGen = {		/* see tcldot */
	gd_reset,
	gd_begin_job, gd_end_job,
	gd_begin_graph_to_memory, gd_end_graph_to_memory,
	gd_begin_page, gd_end_page,
	gd_begin_cluster, gd_end_cluster,
	gd_begin_nodes, gd_end_nodes,
	gd_begin_edges, gd_end_edges,
	gd_begin_node, gd_end_node,
	gd_begin_edge, gd_end_edge,
	gd_begin_context, gd_end_context,
	gd_set_font, gd_textline,
	gd_set_color, gd_set_style,
	gd_ellipse, gd_polygon,
	gd_bezier, gd_polyline,
	0/* gd_arrowhead */, gd_user_shape,
	0 /* gd comment */, gd_textsize
};


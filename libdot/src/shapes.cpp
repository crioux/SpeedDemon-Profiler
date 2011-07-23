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
 * every shape has these functions:
 *
 * void		SHAPE_init(node_t *n)
 *			initialize the shape (usually at least its size).
 * port_t	SHAPE_port(node_t *n, char *portname)
 *			return the aiming point and slope (if constrained)
 *			of a port.
 * int		SHAPE_inside(node_t *n, pointf p, edge_t *e);
 *			test if point is inside the node shape which is
 *			assumed convex.
 *			the point is relative to the node center.  the edge
 *			is passed in case the port affects spline clipping.
 * void		SHAPE_code(node_t *n)
 *			generate graphics code for a node.
 * int		SHAPE_path(node_t *n, edge_t *e, int pt, box path[], int *nbox)
 *			create a path for the port of e that touches n,
 *			return side
 *
 * some shapes, polygons in particular, use additional shape control data *
 *
 */

 /* this needs an overhaul */

#include	"render.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

#define FILLED 	(1 << 0)
#define ROUNDED (1 << 1)
#define DIAGONALS (1 << 2)
#define AUXLABELS (1 << 3)
#define RBCONST 12
#define RBCURVE .5

#ifndef HAVE_SINCOS
void sincos(double x,double *s,double *c) { *s = sin(x); *c = cos(x); }
#endif

/* functions that deal with labels */

static double  cwFontRatio;
static double romanFontwidth[] = {
    0.25,   0.33,   0.41,   0.50,   0.50,   0.83,
    0.78,   0.33,   0.33,   0.33,   0.50,   0.56,
    0.25,   0.33,   0.25,   0.28,   0.50,   0.50,
    0.50,   0.50,   0.50,   0.50,   0.50,   0.50,
    0.50,   0.50,   0.28,   0.28,   0.56,   0.56,
    0.56,   0.44,   0.92,   0.72,   0.67,   0.67,
    0.72,   0.61,   0.56,   0.72,   0.72,   0.33,
    0.39,   0.72,   0.61,   0.89,   0.72,   0.72,
    0.56,   0.72,   0.67,   0.56,   0.61,   0.72,
    0.72,   0.94,   0.72,   0.72,   0.61,   0.33,
    0.28,   0.33,   0.47,   0.50,   0.33,   0.44,
    0.50,   0.44,   0.50,   0.44,   0.33,   0.50,
    0.50,   0.28,   0.28,   0.50,   0.28,   0.78,
    0.50,   0.50,   0.50,   0.50,   0.33,   0.39,
    0.28,   0.50,   0.50,   0.72,   0.50,   0.50,
    0.44,   0.48,   0.20,   0.48,   0.54,   0.0
};

static double stickFontwidth[] = {
  0.7249,  0.2835,  0.4272,  0.8301,  0.7897,  0.8099, /* " !"#$%" */
  0.8301,  0.3037,  0.2531,  0.2835,  0.6439,  0.7897, /* "&'()*+" */
  0.3037,  0.7897,  0.2835,  0.4657,  0.7532,  0.7532, /* ",-./01" */
  0.7492,  0.7492,  0.7492,  0.7492,  0.7492,  0.7492, /* "234567" */
  0.7492,  0.7492,  0.2835,  0.3037,  0.7694,  0.7694, /* "89:;<=" */
  0.7694,  0.5568,  0.8403,  0.6803,  0.6985,  0.8058, /* ">?@ABC" */
  0.7350,  0.6985,  0.5912,  0.8241,  0.7704,  0.3219, /* "DEFGHI" */
  0.4839,  0.6625,  0.5376,  0.9132,  0.7880,  0.8058, /* "JKLMNO" */
  0.6090,  0.7704,  0.6807,  0.6631,  0.6449,  0.6985, /* "PQRSTU" */
  0.6094,  0.8960,  0.6449,  0.6449,  0.6449,  0.3240, /* "VWXYZ[" */
  0.5568,  0.3543,  0.4292,  1.0832,  0.3240,  0.5568, /* "\]^_`a" */
  0.6074,  0.5710,  0.6074,  0.5872,  0.4292,  0.6074, /* "bcdefg" */
  0.5507,  0.2673,  0.3037,  0.5366,  0.2673,  0.7998, /* "hijklm" */
  0.5507,  0.6074,  0.6074,  0.6074,  0.4151,  0.5264, /* "nopqrs" */
  0.4151,  0.5366,  0.5163,  0.7289,  0.5467,  0.5163, /* "tuvwxy" */
  0.5366,  0.3442,  0.3240,  0.3564,  0.8018,  0.0     /* "z{|}~"  */
};
static double *Fontwidth;
#define FONTWIDTH(c) (isprint(c)?Fontwidth[c-32]:0.0)

static port_t	Center = { {0,0}, -1, 0, 0, 0};

static point
flip_pt(point p)
{ int		t = p.x; p.x = -p.y; p.y = t; return p; }

static point
invflip_pt(point p)
{ int		t = p.x; p.x = p.y; p.y = -t; return p; }

static pointf
flip_ptf(pointf p)
{ double	t = p.x; p.x = -p.y; p.y = t; return p; }

static double
actualwidth(char *str, textlabel_t *lp, graph_t *g)
{
	char 	*p;
	double	rw;		/* raw width */
	double	sw;		/* scaled by lp->fontsize */
	double	aw;		/* actual width according to codegen, if available */
	double	adj, adjust;	/* compensate for miscalculation of raster font sizes */
	int		UFTesc;
	char	joe[2];

	if ((str == 0) || (*str == '\0')) return 0.0;

	/* have UFT-8 escape? */
	UFTesc = 0;
	for (p = str; (p = strchr(p,'&')); p++) {
		if (sscanf(p,"&#%*[0123456789]%c",joe) == 1)
			if (joe[0] == ';') { UFTesc = 1; break; }
	}

	for (rw = 0.0, p = str; *p; p++)
		rw += (cwFontRatio? cwFontRatio : FONTWIDTH(*p));	 /* raw width */
	sw = rw * lp->fontsize;	/* scaled  width */

	/* check if raster font is wider, overrides estimate */
	if (CodeGen && CodeGen->textsize && str) {
		adj = g->root->u.drawing->font_scale_adj;
		do {
		  aw = (CodeGen->textsize(str ,lp->fontname,adj*lp->fontsize).x);
		  if (aw == 0) break;
		  adjust = sw / aw;
		  if (adj > adjust)
			adj = adjust;
		  if (UFTesc) {	/* can shrink, also */
			sw = aw;
		  }
		  /* retry */
		  sw = (CodeGen->textsize(str ,lp->fontname,adj*lp->fontsize).x);
		} while (aw > sw);
		g->root->u.drawing->font_scale_adj = (float)adj;
	}
	return sw;
}

static void
storeline(textlabel_t *lp, char *line, char terminator,graph_t *g)
{
	double	width;

	lp->line = ALLOC(lp->nlines+2,lp->line,textline_t);
	lp->line[lp->nlines].str = line;
	lp->line[lp->nlines].width = width = actualwidth(line,lp,g);
	lp->line[lp->nlines].just = terminator;
	lp->nlines++;
	if (width > 0) {
		width += lp->fontsize;	/* margins */
		width = PS2INCH(width);
		if (lp->dimen.x < width) lp->dimen.x = width;
	}
	lp->dimen.y = PS2INCH(lp->nlines*(lp->fontsize + 2));
}

/* compiles <str> into a label <lp> and returns its bounding box size.
 * also adjusts layout font_scale_adj as a side effect, if the graphics
 * driver has a textsize function.
 */
pointf label_size(char *str, textlabel_t *lp, graph_t *g)
{
	char		c,*p,*line,*lineptr;

	line = lineptr = NULL;
	if (strnicmp(lp->fontname,"Courier",7) == 0) {
		cwFontRatio = 0.65; Fontwidth = 0;
	}
	else if (strnicmp(lp->fontname,"StickCW",7) == 0) {
		cwFontRatio = 0.7; Fontwidth = 0;
	}
	else if (strnicmp(lp->fontname,"Stick",5) == 0) {
		cwFontRatio = 0.0; Fontwidth = stickFontwidth;
	}
	else {
		cwFontRatio = 0.0; Fontwidth = romanFontwidth;
	}

	p = str;
	line = lineptr = (char*) malloc(strlen(p) + 1);
	*line = 0;
	while ((c = *p++)) {
		if (c == '\\') {
			switch (*p) {
				case 'n': case 'l': case 'r':
					*lineptr++ = '\0';
					storeline(lp,line,*p,g);
					line = lineptr;
					break;
				default:
					*lineptr++ = *p;
			}
			if (*p) p++;
		}
		else *lineptr++ = c;
	}

	if (line != lineptr) {
		*lineptr++ = '\0';
		storeline(lp,line,'n',g);
	}
	return lp->dimen;
}

static void
unrecognized(node_t* n, char* p)
{
	fprintf(stderr,"warning: node %s, port %s unrecognized\n",n->name,p);
}

#define GAP (PS2INCH(4.))

void	poly_init(node_t *);
void	poly_free(node_t *);
port_t	poly_port(node_t*, char *);
int		poly_inside(node_t*, pointf, edge_t *);
void	poly_gencode(node_t*);

void	record_init(node_t *);
void	record_free(node_t *);
port_t	record_port(node_t*, char *);
int		record_inside(node_t*, pointf, edge_t *);
int		record_path(node_t* n, edge_t* e, int pt, box rv[], int* kptr);
void	record_gencode(node_t*);

void	epsf_init(node_t *);
void	epsf_free(node_t *);
int		epsf_inside(node_t*, pointf, edge_t *);
void	epsf_gencode(node_t*);

/* polygon descriptions.  "polygon" with 0 sides takes all user control */

/*				      regul perip sides orien disto skew */
static polygon_t p_polygon	  = { FALSE,  1,    0,    0.,   0.,   0. };
/* builtin polygon descriptions */
static polygon_t p_ellipse	  = { FALSE,  1,    1,    0.,   0.,   0. };
static polygon_t p_circle	  = { TRUE,   1,    1,    0.,   0.,   0. };
static polygon_t p_egg		  = { FALSE,  1,    1,    0.,   -.3f,  0. };
static polygon_t p_triangle	  = { FALSE,  1,    3,    0.,   0.,   0. };
static polygon_t p_box		  = { FALSE,  1,    4,    0.,   0.,   0. };
static polygon_t p_plaintext	  = { FALSE,  0,    4,    0.,   0.,   0. };
static polygon_t p_diamond	  = { FALSE,  1,    4,    45.,  0.,   0. };
static polygon_t p_trapezium	  = { FALSE,  1,    4,    0.,   -.4f,  0. };
static polygon_t p_parallelogram  = { FALSE,  1,    4,    0.,   0.,   .6f };
static polygon_t p_house	  = { FALSE,  1,    5,    0.,   -.64f, 0. };
static polygon_t p_hexagon	  = { FALSE,  1,    6,    0.,   0.,   0. };
static polygon_t p_octagon	  = { FALSE,  1,    8,    0.,   0.,   0. };
/* redundant and undocumented builtin polygons */
static polygon_t p_doublecircle   = { TRUE,   2,    1,    0.,   0.,   0. };
static polygon_t p_invtriangle	  = { FALSE,  1,    3,    180., 0.,   0. };
static polygon_t p_invtrapezium   = { FALSE,  1,    4,    180., -.4f,  0. };
static polygon_t p_invhouse	  = { FALSE,  1,    5,    180., -.64f, 0. };
static polygon_t p_doubleoctagon  = { FALSE,  2,    8,    0.,   0.,   0. };
static polygon_t p_tripleoctagon  = { FALSE,  3,    8,    0.,   0.,   0. };
static polygon_t p_Mdiamond  = { FALSE,  1,    4,    45.,  0.,  0. ,DIAGONALS|AUXLABELS};
static polygon_t p_Msquare	  = { TRUE,  1,    4,    0.,  0.,  0. ,DIAGONALS};
static polygon_t p_Mcircle	  = { TRUE,   1,    1,    0.,   0.,   0.,DIAGONALS|AUXLABELS};

static shape_desc Shapes[]  = {	/* first entry is default for no such shape */
{"box"		,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_box		},
{"polygon"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_polygon	},
{"ellipse"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_ellipse	},
{"circle"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_circle	},
{"egg"		,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_egg		},
{"triangle"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_triangle	},
{"plaintext"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_plaintext	},
{"diamond"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_diamond	},
{"trapezium"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_trapezium	},
{"parallelogram",poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_parallelogram	},
{"house"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_house		},
{"hexagon"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_hexagon	},
{"octagon"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_octagon	},
/* redundant and undocumented builtin polygons */
{"doublecircle"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_doublecircle	},
{"doubleoctagon",poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_doubleoctagon	},
{"tripleoctagon",poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_tripleoctagon	},
{"invtriangle"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_invtriangle	},
{"invtrapezium"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_invtrapezium	},
{"invhouse"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_invhouse	},
{"Mdiamond"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_Mdiamond	},
{"Msquare"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_Msquare	},
{"Mcircle"	,poly_init,poly_free,poly_port,poly_inside,NULL,poly_gencode,&p_Mcircle	},
/*  *** shapes other than polygons  *** */
{"record"	,record_init,record_free,record_port,record_inside,record_path,record_gencode,NULL},
{"Mrecord"	,record_init,record_free,record_port,record_inside,record_path,record_gencode,NULL},
//{"epsf"		,epsf_init,epsf_free,poly_port,epsf_inside,NULL,epsf_gencode,NULL},
{NULL		,NULL,NULL,NULL,NULL,NULL,NULL,NULL}
};

static double quant(double val, double q)
{
	int	i;
	i = val / q;
	if (i * q + .00001 < val) i++;
	return i * q;
}

void poly_init(node_t* n)
{
	pointf	dimen;
	pointf	P,Q,R;
	pointf	*vertices;
	double  temp,alpha,beta,gamma,delta,xb,yb;
	double	orientation,distortion,skew;
	double	sectorangle, sidelength, skewdist, gdistortion, gskew;
	double	angle, sinx, cosx, xmax, ymax, scalex, scaley;
	int     regular,peripheries,sides;
	int	i,j,outp;
	polygon_t *poly=NEW(polygon_t);

	regular = n->u.shape->polygon->regular;
	peripheries = n->u.shape->polygon->peripheries;
	sides = n->u.shape->polygon->sides;
	orientation = n->u.shape->polygon->orientation;
	skew = n->u.shape->polygon->skew;
	distortion = n->u.shape->polygon->distortion;

	regular |= mapbool(agget(n,"regular"));
	peripheries = late_int(n,N_peripheries,peripheries,0);
	orientation += late_float(n,N_orientation,0.0,-360.0);
	if (sides==0) { /* not for builtins */
		skew = late_float(n,N_skew,0.0,-100.0);
		sides = late_int(n,N_sides,4,0);
		distortion = late_float(n,N_distortion,0.0,-100.0);
	}

	/* get label dimensions */
	dimen = n->u.label->dimen;

	/* adjust height of label so that text is centered */
    /* 	if (dimen.y > PS2INCH(8.)) dimen.y += PS2INCH(8.); */
    /* vladimir: centering happens in emit_label */

	if (mapbool(late_string(n,N_fixed,"false"))) {
		if ((n->u.width < dimen.x) || (n->u.height < dimen.y))
			fprintf(stderr,"dot: warning, node '%s' size too small for label\n",
				n->name);
		dimen.x = dimen.y = 0;
	}

	/* quantization */
	if ((temp = n->graph->u.drawing->quantum) > 0.0) {
		dimen.x = quant(dimen.x,temp);
		dimen.y = quant(dimen.y,temp);
	}

	/* make square if necessary */
	if (regular) {
		/* make x and y dimensions equal */
		n->u.width = n->u.height = MIN(n->u.width,n->u.height);
		xb = yb = MAX(dimen.x,dimen.y);
	} else {
		xb = dimen.x; yb = dimen.y;
	}


	/* I don't know how to distort or skew ellipses in postscript */
	/* Convert request to a polygon with a large number of sides */
	if ((sides<=2) && ((distortion!=0.) || (skew!=0.))) {
		sides = 120;
	}

	/* adjust bounding box so that label fits in inner ellipse */
	/* this will change the symmetry of the bounding box */
	/* adjust for inner to outer diameter of polygon */
	if (sides>2) { /* except ellipses */
		temp = cos(PI/sides);
		xb /= temp; yb /= temp;
	}

	if (  (sides!=4)
	   || ((ROUND(orientation)%90)!=0)
	   || (distortion!=0.)
	   || (skew!=0.) ) {
		if (yb>xb) temp = xb * (sqrt(2.) - 1.);
		else temp = yb * (sqrt(2.) - 1.);
		xb += temp; yb += temp;
        }
	xb=MAX(n->u.width,xb); yb=MAX(n->u.height,yb);
	outp=peripheries;
	if (peripheries<1) outp=1;
	if (sides<3) { /* ellipses */
		sides=1;
		vertices=N_NEW(outp,pointf);
		P.x=xb/2.; P.y=yb/2.;
		vertices[0] = P;
		if (peripheries>1) {
			for (j=1; j<peripheries; j++) {
				P.x += GAP; P.y += GAP;
				vertices[j] = P;
			}
			xb=2.*P.x; yb=2.*P.y;
		}
	} else {
		vertices=N_NEW(outp*sides,pointf);
		sectorangle = 2.*PI/sides;
		sidelength = sin(sectorangle/2.);
		skewdist = hypot(fabs(distortion)+fabs(skew),1.);
		gdistortion = distortion*sqrt(2.)/cos(sectorangle/2.);
		gskew = skew/2.;
		angle = (sectorangle-PI)/2.;
		sincos(angle,&sinx,&cosx);
		R.x = .5*cosx; R.y = .5*sinx;
		xmax=ymax=0.;
		angle += (PI-sectorangle)/2.;
		for (i=0; i<sides; i++) {
	
			/*next regular vertex*/
			angle += sectorangle;
			sincos(angle,&sinx,&cosx);
			R.x += sidelength*cosx; R.y += sidelength*sinx;
	
			/*distort and skew*/
			P.x = R.x*(skewdist+R.y*gdistortion)+R.y*gskew;
			P.y = R.y;
	
			/*orient P.x,P.y*/
			alpha = RADIANS(orientation)+atan2(P.y,P.x);
			sincos(alpha,&sinx,&cosx);
			P.x = P.y = hypot(P.x,P.y);
			P.x *= cosx; P.y *= sinx;

			/*scale for label*/
			P.x *= xb; P.y *= yb;
	
			/*find max for bounding box*/
			xmax = MAX(fabs(P.x),xmax); ymax = MAX(fabs(P.y),ymax);
	
			/* store result in array of points */
			vertices[i] = P;
		}

		/* apply minimum dimensions */
		xmax *=2.; ymax *=2.;
		xb=MAX(n->u.width,xmax); yb=MAX(n->u.height,ymax);
		scalex=xb/xmax; scaley=yb/ymax;
	
		for (i=0; i<sides; i++) {
			P = vertices[i];
			P.x *= scalex; P.y *= scaley;
			vertices[i] = P;
		}
	
		if (peripheries>1) {
			Q = vertices[(sides-1)];
			R = vertices[0];
			beta = atan2(R.y-Q.y,R.x-Q.x);
			for (i=0; i<sides; i++) {

				/*for each vertex find the bisector*/
				P = Q; Q = R; R = vertices[(i+1)%sides];
				alpha = beta; beta = atan2(R.y-Q.y,R.x-Q.x);
				gamma = (alpha+PI-beta)/2.;

				/*find distance along bisector to*/
				/*intersection of next periphery*/
				temp = GAP/sin(gamma);

				/*convert this distance to x and y*/
				delta = alpha-gamma;
				sincos(delta,&sinx,&cosx);
				sinx *= temp; cosx *= temp;

				/*save the vertices of all the*/
				/*peripheries at this base vertex*/
				for (j=1; j<peripheries; j++) {
					Q.x += cosx; Q.y += sinx;
					vertices[i+j*sides] = Q;
				}
			}
			for (i=0; i<sides; i++) {
				P = vertices[i+(peripheries-1)*sides];
				xb = MAX(2.*fabs(P.x),xb);
				yb = MAX(2.*fabs(P.y),yb);
			}
		}
	}
	poly->regular = regular;
	poly->peripheries = peripheries;
	poly->sides = sides;
	poly->orientation = orientation;
	poly->skew = skew;
	poly->distortion = distortion;
	poly->vertices = vertices;

	n->u.width = xb;
	n->u.height = yb;
	n->u.shape_info = (void*) poly;
}

void poly_free(node_t *n)
{
	polygon_t* p = (polygon_t *)n->u.shape_info;

	if (p) {
		free(p->vertices);
		free(p);
	}
}

int poly_inside(node_t* n, pointf p, edge_t* e)
{
	static polygon_t *poly;
	static int	last,outp,sides;
	static node_t	*lastn;
	static pointf	O;
	static pointf	*vertex;
	static double	xsize,ysize,scalex,scaley,box_URx,box_URy;

	int		i,i1,j,s;
	pointf		P,Q,R;

	e = e;
	P = (n->graph->u.left_to_right? flip_ptf(p) : p);
	if (n != lastn) {
		poly = (polygon_t*) n->u.shape_info;
		vertex = poly->vertices;
		sides = poly->sides;
		lastn = n;

		/* get point and node size adjusted for rankdir=LR */
		if (n->graph->u.left_to_right) {
			ysize = n->u.lw + n->u.rw; xsize = n->u.ht;
		}
		else {
			xsize = n->u.lw + n->u.rw; ysize = n->u.ht;
		}

        	/* scale */
		if (xsize == 0.0) xsize = 1.0;
		if (ysize == 0.0) ysize = 1.0;
		scalex = n->u.width/xsize; scaley = n->u.height/ysize;
		box_URx = n->u.width/2.0; box_URy = n->u.height/2.0;

		/* index to outer-periphery */
		outp=(poly->peripheries-1)*sides;
		if (outp<0) outp=0;
	}

        /* scale */
	P.x *= scalex; P.y *= scaley;

	/* inside bounding box? */
	if ((fabs(P.x)>box_URx) || (fabs(P.y)>box_URy)) return FALSE;

        /* ellipses */
	if (sides<=2) return (hypot(P.x/box_URx,P.y/box_URy)<1.);

	/* use fast test in case we are converging on a segment */
	i = last % sides; /*in case last left over from larger polygon*/
	i1 = (i + 1) % sides;
	Q = vertex[i+outp]; R = vertex[i1+outp];
	if ( !(same_side(P,O,Q,R))) return FALSE;
	if (  (s=same_side(P,Q,R,O)) && (same_side(P,R,O,Q))) return TRUE;
	for (j = 1; j < sides; j++) {
		if (s) {
			i = i1; i1 = (i + 1) % sides;
		} else {
			i1 = i; i = (i + sides - 1) % sides;
		} 
		if ( !(same_side(P,O,vertex[i+outp],vertex[i1+outp]))) {
			last = i;
			return FALSE;
		}
	}
	last = i;  /* in case next edge is to same side */
	return TRUE;
}

int same_side(pointf p0, pointf p1, pointf L0, pointf L1)
{
	int s0,s1;
	double a,b,c;

	/* a x + b y = c */
	a = -(L1.y - L0.y);
	b = (L1.x - L0.x);
	c = a * L0.x + b * L0.y;

	s0 = (a*p0.x + b*p0.y - c >= 0);
	s1 = (a*p1.x + b*p1.y - c >= 0);
	return (s0 == s1);
}


port_t
poly_port(node_t* n, char* pname)
{
	static char *points_of_compass[] =
		{"n","ne","e","se","s","sw","w","nw",NULL};
static struct {char x,y;} a[] = {{0,1},{1,1},{1,0},{1,-1},{0,-1},{-1,-1},{-1,0},{-1,1}};

	int		i,ht,wd;
	port_t	rv;
	char	*p;


	if (*pname) pname++; /* skip over delim */
	for (i = 0; (p = points_of_compass[i]); i++)
		if (streq(p,pname)) break;

	if (p == NULL) {
		if (pname[0]) unrecognized(n,pname);
		rv = Center;
	}
	else {
		ht = n->u.ht / 2; 
		wd = n->u.lw;
		rv.p.x = a[i].x * wd;
		rv.p.y = a[i].y * ht;
		rv.order = (MC_SCALE * (n->u.lw + rv.p.x)) / (n->u.lw + n->u.rw);
		rv.constrained = FALSE;
		rv.defined = TRUE;
	}
	return rv;
}

static void pencolor(node_t *n)
{
	char *color;

	color = late_nnstring(n,N_color,"");
	if (color[0]) CodeGen->set_color(color);
}

static void fillcolor(node_t *n)
{
	const char *color;
	
	color = late_nnstring(n,N_fillcolor,"");
	if (! color[0]) {
		/* for backward compatibilty, default fill is same as pen */
		color = late_nnstring(n,N_color,"");
		if (! color[0])
			color = (Output_lang == MIF ? "black" : DEFAULT_FILL);
	}
	CodeGen->set_color((char*)color);
}

static int stylenode(node_t* n)
{
	int		i;
	char		*style,**pstyle;
	int		istyle = 0;
	polygon_t	*poly;

	style = late_nnstring(n,N_style,"");
	if (style[0]) {
		pstyle = parse_style(style);
		CodeGen->set_style(pstyle);
		for (i = 0; pstyle[i]; i++) {
			if (strcmp(pstyle[i],"filled") == 0) {istyle |= FILLED; continue;}
			if (strcmp(pstyle[i],"rounded") == 0) {istyle |= ROUNDED; continue;}
			if (strcmp(pstyle[i],"diagonals") == 0) {istyle |= DIAGONALS; continue;}
		}
	}
	if ((poly = n->u.shape->polygon)) istyle |= poly->option;
	return istyle;
}


/* generic polygon gencode routine */
void poly_gencode(node_t* n)
{
	polygon_t		*poly;
	double			xsize, ysize;
	int			i,j,peripheries,sides,style;
	pointf			P,*vertices;
	static point	*A;
	static int		A_size;
	
	poly = (polygon_t*) n->u.shape_info;
	vertices = poly->vertices;
	sides = poly->sides;
	peripheries = poly->peripheries;
	if (A_size < sides) {A_size = sides + 5; A = ALLOC(A_size,A,point);}

	CodeGen->begin_node(n);
	CodeGen->begin_context();
/* prescale by 16.0 to help rounding trick below */
	xsize = ((n->u.lw + n->u.rw) / n->u.width) * 16.0;
	ysize = ((n->u.ht) / n->u.height) * 16.0;

	/* this is bad, but it's because of how the VRML driver works */
#ifdef HAVE_LIBPNG
	if ((CodeGen == &VRML_CodeGen) && (peripheries == 0)) {
		peripheries = 1;
	}
#endif

	style = stylenode(n); 
	for (j = 0; j < peripheries; j++) {
		for (i = 0; i < sides; i++) {
			P = vertices[i+j*sides];
/* simple rounding produces random results around .5 
 * this trick should clip off the random part. 
 * (note xsize/ysize prescaled by 16.0 above) */
			A[i].x = ROUND(P.x * xsize) / 16;
			A[i].y = ROUND(P.y * ysize) / 16;
			if (sides > 2) {A[i].x += n->u.coord.x; A[i].y += n->u.coord.y;}
		}
		if (!j) {
			/* innermost periphery only */
			/* draw filled shape before outline */
			if (style & FILLED) {
				fillcolor(n); /* set fill color */
				if (find_user_shape(n->u.shape->name))
					CodeGen->user_shape(n->u.shape->name,A,sides,1);
				else if (sides <= 2) {
					CodeGen->ellipse(n->u.coord,A[0].x,A[0].y,1);
					if (style & DIAGONALS) Mcircle_hack(n);
				}
				else if (style & (ROUNDED | DIAGONALS)) round_corners(n,A,sides,style);
				else CodeGen->polygon(A,sides,1);
			}
			pencolor(n); /* emit pen color */
		}
		if (find_user_shape(n->u.shape->name))
			CodeGen->user_shape(n->u.shape->name,A,sides,0);
		else if (sides <= 2) {
			CodeGen->ellipse(n->u.coord,A[0].x,A[0].y,0);
			if (style & DIAGONALS) Mcircle_hack(n);
		}
		else if (style & (ROUNDED | DIAGONALS)) round_corners(n,A,sides,style);
		else CodeGen->polygon(A,sides,0);
	}

	if (style & AUXLABELS) Mlabel_hack(n);
    	n->u.label->p = n->u.coord;
	emit_label(n->u.label,n->graph);
	CodeGen->end_context();
	CodeGen->end_node();
}

static void
hack1(node_t* n, char* str, int k)
{
	point		p;
	int			width;
	double		fontsize;
	fontsize = n->u.label->fontsize*.8;

	p.x = n->u.coord.x;
	p.y = n->u.coord.y + k * (n->u.ht - n->u.label->fontsize - 2) / 2.0 ;
	width = round(strlen(str) * (double)(n->u.label->fontsize));
	CodeGen->begin_context();
	CodeGen->set_font(n->u.label->fontname,fontsize);
	CodeGen->textline(p,str,width,fontsize,-.5);
	CodeGen->end_context();
}

void Mlabel_hack(node_t* n)
{
	char		*str;
	if ((str = agget(n,"toplabel"))) hack1(n,str,1);
	if ((str = agget(n,"bottomlabel"))) hack1(n,str,-1);
}

void Mcircle_hack(node_t* n)
{
	float	x,y;
	point	A[2],p;

	y = .7500f;
	x = .6614f;	/* x^2 + y^2 = 1.0 */
	p.y = y * n->u.ht / 2.0;
	p.x = n->u.rw * x;	/* assume node is symmetric */

	A[0] = add_points(p,n->u.coord);
	A[1].y = A[0].y; A[1].x = A[0].x - 2*p.x;
	CodeGen->polyline(A,2);
	A[0].y -= 2*p.y; A[1].y = A[0].y;
	CodeGen->polyline(A,2);
}


/* the "record" shape is a rudimentary table formatter */

#define HASTEXT 1
#define HASPORT 2
#define HASTABLE 4
#define INTEXT 8
#define INPORT 16

#define ISCTRL(c) ((c) == '{' || (c) == '}' || (c) == '|' || (c) == '<' || (c) == '>')

static char *reclblp;

static field_t	*
parse_reclbl(node_t *n, int LR, int flag, char* text)
{
	field_t	*fp, *rv = NEW(field_t);
	char *tsp, *psp, *hstsp, *hspsp, *sp;
	char port[SMALLBUF];
	int maxf, cnt, mode, wflag, ishardspace, fi;

	fp = NULL;
	for (maxf = 1, cnt = 0, sp = reclblp; *sp; sp++) {
		if (*sp == '\\') {
			sp++;
			if (*sp && (*sp == '{' || *sp == '}' || *sp == '|'))
				continue;
		}
		if (*sp == '{')
			cnt++;
		else if (*sp == '}')
			cnt--;
		else if (*sp == '|' && cnt == 0)
			maxf++;
		if (cnt < 0)
			break;
	}
	/*maxf = strccnt(reclblp, '|') + 1;*/
	rv->fld = N_NEW (maxf, field_t*);
	rv->LR = LR;
	mode = 0;
	fi = 0;
	hstsp = tsp = text, hspsp = psp = &port[0];
	wflag = TRUE;
	ishardspace = FALSE;
	while (wflag) {
		switch (*reclblp) {
		case '<':
			if (mode & (HASTABLE | HASPORT))
				return NULL;
			mode |= (HASPORT | INPORT);
			reclblp++;
			break;
		case '>':
			if (!(mode & INPORT))
				return NULL;
			mode &= ~INPORT;
			reclblp++;
			break;
		case '{':
			reclblp++;
			if (mode != 0 || !*reclblp)
				return NULL;
			mode = HASTABLE;
			if (!(rv->fld[fi++] = parse_reclbl (n, NOT (LR) , FALSE, text)))
				return NULL;
			break;
		case '}':
		case '|':
		case '\000':
			if ((!*reclblp && !flag) || (mode & INPORT))
				return NULL;
			if (!(mode & HASTABLE))
				fp = rv->fld[fi++] = NEW (field_t);
			if (mode & HASPORT) {
				if (psp > &port[0] + 1 &&
						psp - 1 != hspsp &&
						*(psp - 1) == ' ')
					psp--;
				*psp = '\000';
				fp->id = strdup (&port[0]);
				hspsp = psp = &port[0];
			}
			if (!(mode & (HASTEXT | HASTABLE)))
				mode |= HASTEXT, *tsp++ = ' ';
			if (mode & HASTEXT) {
				if (tsp > text + 1 &&
						tsp - 1 != hstsp &&
						*(tsp - 1) == ' ')
					tsp--;
				*tsp = '\000';
				fp->lp = make_label (strdup (text), n->u.label->fontsize, n->u.label->fontname, n->u.label->fontcolor,n->graph);
				fp->LR = TRUE;
				hstsp = tsp = text;
			}
			if (*reclblp) {
				if (*reclblp == '}') {
					reclblp++;
					rv->n_flds = fi;
					return rv;
				}
				mode = 0;
				reclblp++;
			} else
				wflag = FALSE;
			break;
		case '\\':
			if (*(reclblp + 1))
				if (ISCTRL (*(reclblp + 1)))
					reclblp++;
				else if (*(reclblp + 1) == ' ')
					ishardspace = TRUE, reclblp++;
			/* falling through ... */
		default:
			if ((mode & HASTABLE) && *reclblp != ' ')
				return NULL;
			if (!(mode & (INTEXT | INPORT)) && *reclblp != ' ')
				mode |= (INTEXT | HASTEXT);
			if (mode & INTEXT) {
				if (!(*reclblp == ' ' && !ishardspace &&
						*(tsp - 1) == ' '))
					*tsp++ = *reclblp;
				if (ishardspace)
					hstsp = tsp - 1;
			} else if (mode & INPORT) {
				if (!(*reclblp == ' ' && !ishardspace &&
						(psp == &port[0] ||
						*(psp - 1) == ' ')))
					*psp++ = *reclblp;
				if (ishardspace)
					hspsp = psp - 1;
			}
			reclblp++;
			break;
		}
	}
	rv->n_flds = fi;
	return rv;
}

point
size_reclbl(node_t* n, field_t* f)
{
	int		i;
	point	d,d0;

	if (f->lp) d = cvt2pt(f->lp->dimen);
	else {
		d.x = d.y = 0.0;
		for (i = 0; i < f->n_flds; i++) {
			d0 = size_reclbl(n,f->fld[i]);
			if (f->LR) { d.x += d0.x; d.y = MAX(d.y,d0.y); }
			else { d.y += d0.y; d.x = MAX(d.x,d0.x); }
		}
	}
	f->size = d;
	return d;
}

void
resize_reclbl(field_t* f, point sz)
{
	int			i,amt;
	double		inc;
	point		d,newsz;
	field_t		*sf;

	/* adjust field */
	d.x = sz.x - f->size.x; d.y = sz.y - f->size.y;
	f->size = sz;

	/* adjust children */
	if (f->n_flds) {
		if (f->LR) inc = (double)d.x/f->n_flds;
		else inc = (double)d.y/f->n_flds;
		for (i = 0; i < f->n_flds; i++) {
			sf = f->fld[i];
			amt = ((int)((i+1)*inc)) - ((int)(i*inc));
			if (f->LR) newsz = pointof(sf->size.x+amt,sz.y);
			else newsz = pointof(sz.x,sf->size.y+amt);
			resize_reclbl(sf,newsz);
		}
	}
}

void
pos_reclbl(field_t* f, point ul)
{
	int		i;

	f->b.LL = pointof(ul.x,ul.y-f->size.y);
	f->b.UR = pointof(ul.x+f->size.x,ul.y);
	for (i = 0; i < f->n_flds; i++) {
		pos_reclbl(f->fld[i],ul);
		if (f->LR) ul.x = ul.x + f->fld[i]->size.x;
		else ul.y = ul.y - f->fld[i]->size.y;
	}
}

/* syntax of labels: foo|bar|baz or foo|(recursive|label)|baz */
void
record_init(node_t* n)
{
	field_t	*info;
	point	ul,sz;
    int     len;
    char*   textbuf;   /* temp buffer for storing labels */

	reclblp = n->u.label->text;
    len = strlen (reclblp);
    textbuf = N_NEW(len+1,char);
	if (!(info = parse_reclbl(n,NOT(n->graph->u.left_to_right), TRUE, textbuf))) {
		fprintf (stderr, "bad label format %s\n", n->u.label->text);
		reclblp = "\\N";
		info = parse_reclbl(n,NOT(n->graph->u.left_to_right), TRUE, textbuf);
	}
    free (textbuf);
	size_reclbl(n,info);
	sz.x = POINTS(n->u.width);  sz.y = POINTS(n->u.height);
	sz.x = MAX(info->size.x,sz.x); sz.y = MAX(info->size.y,sz.y);
	resize_reclbl(info,sz);
	ul = pointof(-sz.x/2,sz.y/2);
	pos_reclbl(info,ul);
	n->u.width = PS2INCH(info->size.x);
	n->u.height = PS2INCH(info->size.y);
	n->u.shape_info = (void*) info;
}

void
record_free(node_t* n)
{
	field_t* p = (field_t *)n->u.shape_info;

	free(p);
}

field_t	*
map_rec_port(field_t* f, char* str)
{
	field_t		*rv;
	int		sub;

	if (f->id && (strcmp(f->id,str) == 0)) rv = f;
	else {
		rv = NULL;
		for (sub = 0; sub < f->n_flds; sub++)
			if ((rv = map_rec_port(f->fld[sub],str))) break;
	}
	return rv;
}

port_t
record_port(node_t* n, char* pname)
{
	field_t	*f;
	box		b;
	port_t	rv;

	if (pname[0] != ':') return Center;		/*could be '\000' */
	if ((f = map_rec_port((field_t*) n->u.shape_info,pname+1)) == NULL) {
		unrecognized(n,pname);
		return Center;
	}

	b = f->b;
	rv.p = pointof((b.LL.x+b.UR.x)/2,(b.LL.y+b.UR.y)/2);
	if (n->graph->u.left_to_right) rv.p = invflip_pt(rv.p);
	rv.order = (MC_SCALE * (n->u.lw + rv.p.x)) / (n->u.lw + n->u.rw);
	rv.constrained = FALSE;
	rv.defined = TRUE;
	return rv;
}

int
record_inside(node_t* n, pointf p, edge_t* e)
{

	pointf			LL,UR;
	edge_t			*f;
	field_t			*fld0;
	char			*pname;
	static edge_t	*last_e;
	static node_t	*last_n;
	static field_t	*fld;

	if (n->graph->u.left_to_right) p = flip_ptf(p);
	for (f = e; f->u.edge_type != NORMAL; f = f->u.to_orig);
	e = f;
	if ((e != last_e) || (n != last_n)) {
		last_e = e; last_n = n;
		pname = agget(e,(char*)(n == f->head ? "headport" : "tailport"));
		fld = map_rec_port((field_t*)n->u.shape_info,pname+1);
	}

	if (fld == NULL) {
		fld0 = (field_t*) n->u.shape_info;
		UR.x = fld0->size.x / 2.0; LL.x = -UR.x;
		UR.y = fld0->size.y / 2.0; LL.y = -UR.y;
	}
	else {
		LL.x = fld->b.LL.x; LL.y = fld->b.LL.y;
		UR.x = fld->b.UR.x; UR.y = fld->b.UR.y;
	}
	return (BETWEEN(LL.x,p.x,UR.x) && BETWEEN(LL.y,p.y,UR.y));
}

static box flip_rec_box(box b, point p)
{
	box	rv;
		/* flip box */
	rv.UR.x = b.UR.y; rv.UR.y = b.UR.x;
	rv.LL.x = b.LL.y; rv.LL.y = b.LL.x;
		/* move box */
	rv.LL.x  += p.x; rv.LL.y += p.y;
	rv.UR.x  += p.x; rv.UR.y += p.y;
	return rv;
}


int
record_path(node_t* n, edge_t* e, int pt, box rv[], int* kptr)
{
	int			i,side,ls,rs;
	point		p;
	field_t		*info;

	if (pt == 1) p = e->u.tail_port.p;
	else p = e->u.head_port.p;
	info = (field_t*) n->u.shape_info;

	for (i = 0; i < info->n_flds; i++) {
		if (n->graph->u.left_to_right == FALSE)
			{ ls = info->fld[i]->b.LL.x; rs = info->fld[i]->b.UR.x; }
		else
			{ ls = info->fld[i]->b.LL.y; rs = info->fld[i]->b.UR.y; }
		if (BETWEEN(ls,p.x,rs)) {
			/* FIXME: I don't understand this code */
			if (n->graph->u.left_to_right) {
				rv[0] = flip_rec_box(info->fld[i]->b,n->u.coord);
			}
			else {
				rv[0].LL.x = n->u.coord.x + ls;
				rv[0].LL.y = n->u.coord.y - n->u.ht/2;
				rv[0].UR.x = n->u.coord.x + rs;
			}
#if 0
			s0 = (rv[0].UR.x - rv[0].LL.x)/6;
			s0 = MIN(s0,n->graph->u.nodesep);
			s1 = MIN(p.x - rv[0].LL.x,rv[0].UR.x - p.x)/2;
			sep = MIN(s0,s1);
			rv[0].LL.x += sep;
			rv[0].UR.x -= sep;
#endif
			rv[0].UR.y = n->u.coord.y + n->u.ht/2;
			*kptr = 1;
			break;
		}
	}
	if (pt == 1) side = BOTTOM; else side = TOP;
	return side;
}

void
gen_fields(node_t* n, field_t* f)
{
	int			i;
	double		cx,cy;
	point		A[2];

	if (f->lp) {
		cx = (f->b.LL.x + f->b.UR.x)/2.0 + n->u.coord.x;
		cy = (f->b.LL.y + f->b.UR.y)/2.0 + n->u.coord.y;
		f->lp->p =  pointof((int)cx,(int)cy);
		emit_label(f->lp,n->graph);
	}

	/* yes it is ridiculous that black is hardwired here, the same way
	 * it is wired into psgen.c ... outline color should be adjustable */
    /* for reasons not presently remembered, we used to say
                CodeGen->set_color("black");
        right here */

	for (i = 0; i < f->n_flds; i++) {
		if (i > 0) {
			if (f->LR) {
				A[0] = f->fld[i]->b.LL;
				A[1].x = A[0].x;
				A[1].y = f->fld[i]->b.UR.y;
			}
			else {
				A[1] = f->fld[i]->b.UR;
				A[0].x = f->fld[i]->b.LL.x;
				A[0].y = A[1].y;
			}
			A[0] = add_points(A[0],n->u.coord);
			A[1] = add_points(A[1],n->u.coord);
			CodeGen->polyline(A,2);
		}
		gen_fields(n,f->fld[i]);
	}
}

void
record_gencode(node_t* n)
{
	point	A[4];
	int		i,style;
	field_t	*f;

	f = (field_t*) n->u.shape_info;
	A[0] = f->b.LL;
	A[2] = f->b.UR;
	A[1].x = A[2].x; A[1].y = A[0].y;
	A[3].x = A[0].x; A[3].y = A[2].y;
	for (i = 0; i < 4; i++) A[i] = add_points(A[i],n->u.coord);
	CodeGen->begin_node(n);
	CodeGen->begin_context();
	style = stylenode(n);
	pencolor(n);
	if (streq(n->u.shape->name,"Mrecord")) style |= ROUNDED;
	if (style & (ROUNDED | DIAGONALS)) round_corners(n,A,4,ROUNDED);
	else CodeGen->polygon(A,4,style&FILLED);
	gen_fields(n,f);
	CodeGen->end_context();
	CodeGen->end_node();
}

static shape_desc **UserShape;
static int	N_UserShape;

shape_desc *
find_user_shape(char* name)
{
	int		i;
	if (UserShape) {
		for (i = 0; i < N_UserShape; i++) {
			if (streq(UserShape[i]->name,name)) return UserShape[i];
		}
	}
	return NULL;
}

shape_desc *
user_shape(char* name)
{
	int			i;
	shape_desc	*p;

	if ((p = find_user_shape(name))) return p;
	i = N_UserShape++;
	UserShape = ALLOC(N_UserShape,UserShape,shape_desc*);
	p = UserShape[i] = NEW(shape_desc);
	*p = Shapes[0];
	p->name = name;
	if (Lib == NULL) fprintf(stderr, "warning: using %s for unknown shape %s\n", Shapes[0].name,p->name);
	return p;
}

shape_desc *
bind_shape(char* name)
{
	shape_desc	*ptr,*rv= NULL;

	for (ptr = Shapes; ptr->name; ptr++)
		if (!strcmp(ptr->name,name)) {rv = ptr; break;}
	if (rv == NULL) rv = user_shape(name);
	return rv;
}

static point
interpolate(double t, point p0, point p1)
{
	point	rv;
	rv.x = p0.x + t * (p1.x - p0.x);
	rv.y = p0.y + t * (p1.y - p0.y);
	return rv;
}

void
round_corners(node_t *nn, point *A, int n, int style)
{
	point	*B,C[2],p0,p1;
	double	d,dx,dy,t;
	int		i,seg,mode;

	if (style & DIAGONALS) mode = DIAGONALS;
	else mode = ROUNDED;
	B = N_NEW(4*n+4,point);
	i = 0;
	for (seg = 0; seg < n; seg++) {
		p0 = A[seg];
		if (seg < n - 1) p1 = A[seg+1];
		else p1 = A[0];
		dx = p1.x - p0.x;
		dy = p1.y - p0.y;
		d = sqrt(dx*dx + dy*dy);
		/*t = ((mode == ROUNDED)? RBCONST / d : .5);*/
		t = RBCONST / d;
		if (mode != ROUNDED) B[i++] = p0;
		if (mode == ROUNDED) B[i++] = interpolate(RBCURVE*t,p0,p1);
		B[i++] = interpolate(t,p0,p1);
		B[i++] = interpolate(1.0 - t,p0,p1);
		if (mode == ROUNDED) B[i++] = interpolate(1.0 - RBCURVE*t,p0,p1);
	}
	B[i++] = B[0];
	B[i++] = B[1];
	B[i++] = B[2];

	if (mode == ROUNDED) {
		for (seg = 0; seg < n; seg++) {
			CodeGen->polyline(B + 4*seg+1,2);
			CodeGen->beziercurve(B + 4*seg+2,4,FALSE,FALSE);
		}
	}
	else {	/* diagonals are weird.  rewrite someday. */
		if (style & FILLED) {
			fillcolor(nn);
			CodeGen->polygon(A,n,style&FILLED);
		}
		pencolor(nn);
		CodeGen->polygon(A,n,0);
		for (seg = 0; seg < n; seg++) {
#ifdef NOTDEF
			C[0] = B[3 * seg]; C[1] = B[3 * seg + 3];
			CodeGen->polyline(C,2);
#endif
			C[0] = B[3 * seg + 2]; C[1] = B[3 * seg + 4];
			CodeGen->polyline(C,2);
		}
	}
	free(B);
}

void
draw_user_shape(node_t *n, point *A, int sides, int filled)
{
}

int
epsf_inside(node_t* n, pointf p, edge_t* e)
{
	pointf	P;
	double	x2;

	P = (n->graph->u.left_to_right? flip_ptf(p) : p);
	x2 = n->u.ht / 2;
	return ((P.x >= -x2) && (P.x <= x2) && (P.y >= -n->u.lw) && (P.y <= n->u.rw));
}

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
#ifndef MSWIN32
#include	<unistd.h>
#endif

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/* local funcs */
static double dist(pointf, pointf);

void *zmalloc(size_t nbytes)
{
	char	*rv = (char *)malloc(nbytes);
	if (nbytes == 0) return 0;
	if (rv == NULL) {fprintf(stderr,"out of memory\n"); abort();}
	memset(rv,0,nbytes);
	return rv;
}

void *zrealloc(void *ptr, size_t size, size_t elt, size_t osize)
{
	void	*p = realloc(ptr,size*elt);
	if (osize < size) memset((char*)p+(osize*elt),'\0',(size-osize)*elt);
	return p;
}

/*
 *  a queue of nodes
 */
queue *
new_queue(int sz)
{
	queue		*q = NEW(queue);

	if (sz <= 1) sz = 2;
	q->head = q->tail = q->store = N_NEW(sz,node_t*);
	q->limit = q->store + sz;
	return q;
}

void
free_queue(queue* q)
{
	free(q->store);
	free(q);
}

void
enqueue(queue* q, node_t* n)
{
	*(q->tail++) = n;
	if (q->tail >= q->limit) q->tail = q->store;
}

node_t *
dequeue(queue* q)
{
	node_t	*n;
	if (q->head == q->tail) n = NULL;
	else {
		n = *(q->head++);
		if (q->head >= q->limit) q->head = q->store;
	}
	return n;
}

/* returns index of an attribute if bound, else -1 */
int
late_attr(void* obj, char* name)
{
	attrsym_t	*a;
	if ((a = agfindattr(obj,name)) != 0) return a->index;
	else return -1;
}

int
late_int(void *obj, attrsym_t *attr, int def, int low)
{
	char	*p;
	int		rv;
	if (attr == NULL) return def;
	p = agxget(obj,attr->index);
	if (p[0]  == '\0') return def;
	if ((rv = atoi(p)) < low) rv = low;
	return rv;
}

double
late_float(void *obj,attrsym_t *attr, double def, double low)
{
	char		*p;
	double rv;
	if (attr == NULL) return def;
	p = agxget(obj,attr->index);
	if (p[0]  == '\0') return def;
	if ((rv = atof(p)) < low) rv = low;
	return rv;
}

char *
late_string(void* obj, attrsym_t* attr, char* def)
{
	if (attr == NULL) return def;
	return agxget(obj,attr->index);
}

char *
late_nnstring(void* obj, attrsym_t* attr, char* def)
{
	char	*rv = late_string(obj,attr,def);
	if (rv[0] == '\0') rv = def;
	return rv;
}

int
late_bool(void *obj, attrsym_t *attr, int def)
{
	if (attr == NULL) return def;
	return mapbool(agxget(obj,attr->index));
}

/* counts occurences of 'c' in string 'p' */
int
strccnt(char *p, char c)
{
	int		rv = 0;
	while (*p) if (*p++ == c) rv++;
	return 	rv;
}

/* union-find */
node_t	*
UF_find(node_t* n)
{
	while (n->u.UF_parent && (n->u.UF_parent != n)) {
		if (n->u.UF_parent->u.UF_parent) n->u.UF_parent = n->u.UF_parent->u.UF_parent;
		n = n->u.UF_parent;
	}
	return n;
}

node_t	*
UF_union(node_t *u, node_t *v)
{
	if (u == v) return u;
	if (u->u.UF_parent == NULL) {u->u.UF_parent = u; u->u.UF_size = 1;}
	else u = UF_find(u);
	if (v->u.UF_parent == NULL) {v->u.UF_parent = v; v->u.UF_size = 1;}
	else v = UF_find(v);
	if (u->id > v->id) { u->u.UF_parent = v; v->u.UF_size += u->u.UF_size;}
	else {v->u.UF_parent = u; u->u.UF_size += v->u.UF_size; v = u;}
	return v;
}

void
UF_remove(node_t *u, node_t *v)
{
	assert(u->u.UF_size == 1);
	u->u.UF_parent = u;
	v->u.UF_size -= u->u.UF_size;
}

void
UF_singleton(node_t* u)
{
	u->u.UF_size = 1;
	u->u.UF_parent = NULL;
	u->u.ranktype = NORMAL;
}

void
UF_setname(node_t *u, node_t *v)
{
	assert(u == UF_find(u));
	u->u.UF_parent = v;
	v->u.UF_size += u->u.UF_size;
}

boolean spline_merge(node_t* n)
{
	return ((n->u.node_type == VIRTUAL) && ((n->u.in.size > 1) || (n->u.out.size > 1)));
}

point coord(node_t *n)
{
     pointf      pf;
     pf.x = n->u.pos[0];
     pf.y = n->u.pos[1];
     return cvt2pt(pf);
}

point pointof(int x, int y)
{
	point rv;
	rv.x = x, rv.y = y;
	return rv;
}

point
cvt2pt(pointf p)
{
	point	rv;
	rv.x = POINTS(p.x);
	rv.y = POINTS(p.y);
	return rv;
}

pointf
cvt2ptf(point p)
{
	pointf	rv;
	rv.x = PS2INCH(p.x);
	rv.y = PS2INCH(p.y);
	return rv;
}

box boxof (int llx, int lly, int urx, int ury)
{
	box b;

	b.LL.x = llx, b.LL.y = lly;
	b.UR.x = urx, b.UR.y = ury;
	return b;
}

box mkbox(point p0, point p1)
{
	box		rv;

	if (p0.x < p1.x)	{	rv.LL.x = p0.x; rv.UR.x = p1.x; }
	else				{	rv.LL.x = p1.x; rv.UR.x = p0.x; }
	if (p0.y < p1.y)	{	rv.LL.y = p0.y; rv.UR.y = p1.y; }
	else				{	rv.LL.y = p1.y; rv.UR.y = p0.y; }
	return rv;
}

point add_points(point p0, point p1)
{
	p0.x += p1.x;
	p0.y += p1.y;
	return p0;
}

point sub_points(point p0, point p1)
{
	p0.x -= p1.x;
	p0.y -= p1.y;
	return p0;
}

double atan2pt(point p1, point p0)
{
	return atan2((double)(p1.y - p0.y),(double)(p1.x - p0.x));
}

/* from Glassner's Graphics Gems */
#define W_DEGREE 5

/*
 *  Bezier : 
 *	Evaluate a Bezier curve at a particular parameter value
 *      Fill in control points for resulting sub-curves if "Left" and
 *	"Right" are non-null.
 * 
 */
pointf Bezier (pointf *V, int degree, double t, pointf* Left, pointf* Right)
{
	int i, j;		/* Index variables	*/
	pointf Vtemp[W_DEGREE + 1][W_DEGREE + 1];

	/* Copy control points	*/
	for (j =0; j <= degree; j++) {
		Vtemp[0][j] = V[j];
	}

	/* Triangle computation	*/
	for (i = 1; i <= degree; i++) {	
		for (j =0 ; j <= degree - i; j++) {
	    	Vtemp[i][j].x =
	      		(1.0 - t) * Vtemp[i-1][j].x + t * Vtemp[i-1][j+1].x;
	    	Vtemp[i][j].y =
	      		(1.0 - t) * Vtemp[i-1][j].y + t * Vtemp[i-1][j+1].y;
		}
	}
	
	if (Left != NULL)
		for (j = 0; j <= degree; j++)
	    		Left[j] = Vtemp[j][0];
	if (Right != NULL)
		for (j = 0; j <= degree; j++)
	    		Right[j] = Vtemp[degree-j][j];

	return (Vtemp[degree][0]);
}

textlabel_t	*make_label(char *str, double fontsize, char *fontname, char *fontcolor, graph_t *g)
{
	textlabel_t	*rv = NEW(textlabel_t);
	rv->text = str;
	rv->fontname = fontname;
	rv->fontcolor = fontcolor;
	rv->fontsize = (float)fontsize;
	label_size(str,rv,g);
	return rv;
}

void free_label(textlabel_t* p)
{
	if (p) {
		if (p->nlines != '\0')
			free(p->line[0].str);
		free(p->line);
		free(p);
	}
}

#ifdef DEBUG
edge_t	* debug_getedge(graph_t *g, char *s0, char *s1)
{
	node_t	*n0,*n1;
	n0 = agfindnode(g,s0);
	n1 = agfindnode(g,s1);
	if (n0 && n1) return agfindedge(g,n0,n1);
	else return NULL;
}
#endif

#ifndef MSWIN32
#include	<pwd.h>
#endif

char * username(char* buf)
{
	char	*user = NULL;
#ifndef MSWIN32
	struct passwd	*p;
	p = (struct passwd *) getpwuid(getuid());
#ifdef SVR4
	if (p) {sprintf(buf,"(%s) %s",p->pw_name,p->pw_comment); user = buf;}
#else
	if (p) {sprintf(buf,"(%s) %s",p->pw_name,p->pw_gecos); user = buf;}
#endif
#endif
	if (user == NULL) user = "Bill Gates";
	return user;
}

void cat_libfile(FILE *ofp, char **arglib, char **stdlib)
{
	FILE	*fp;
	char	*p,**s,buf[BUFSIZ];
	int		i,use_stdlib = TRUE;

	if (arglib) for (i = 0; (p = arglib[i]) != 0; i++)
		if (*p == '\0') use_stdlib = FALSE;
	if (use_stdlib) for (s = stdlib; *s; s++) {fputs(*s,ofp); fputc('\n',ofp);}
	if (arglib) for (i = 0; (p = arglib[i]) != 0; i++) {
		if (p[0] && ((fp = fopen(p,"r")) != 0)) {
			while (fgets(buf,sizeof(buf),fp)) fputs(buf,ofp);
		}
		else fprintf(stderr,"warning: can't open library file %s\n", p);
	}
}

int
rect_overlap(box b0, box b1)
{
	if ((b0.UR.x < b1.LL.x) || (b1.UR.x < b0.LL.x) 
		|| (b0.UR.y < b1.LL.y) || (b1.UR.y < b0.LL.y)) return FALSE;
	return TRUE;
}

int round(double f)
{
	int		rv;
	if (f > 0) rv = (int)(f + .5) ;
	else rv = (int)(f - .5);
	return rv;
}

int
maptoken(char *p,char **name, int *val)
{
	int		i;
	char	*q;

	for (i = 0; (q = name[i]) != 0; i++)
		if (p && streq(p,q)) break;
	return val[i];
}

int
mapbool(char* p)
{
	if (p == NULL) return FALSE;
	if (!stricmp(p,"false")) return FALSE;
	if (!stricmp(p,"true")) return TRUE;
	return atoi(p);
}

static double dist(pointf  p,pointf  q)
{
    double  d0,d1;
    d0 = p.x - q.x;
    d1 = p.y - q.y;
    return sqrt(d0*d0 + d1*d1);
}

point dotneato_closest(splines* spl, point p)
{
	int		i, j, k, besti, bestj;
	double	bestdist, d, dlow, dhigh;
	double low, high, t;
	pointf c[4], pt2, pt;
	point rv;
	bezier bz;

	besti = bestj = -1;
	bestdist = 1e+38;
	pt.x = p.x; pt.y = p.y;
	for (i = 0; i < spl->size; i++) {
		bz = spl->list[i];
		for (j = 0; j < bz.size; j++) {
			pointf b;

			b.x = bz.list[j].x; b.y = bz.list[j].y;
			d = dist(b,pt);
			if ((bestj == -1) || (d < bestdist)) {
				besti = i;
				bestj = j;
				bestdist = d;
			}
		}
	}

	bz = spl->list[besti];
	j = bestj/3; if (j >= spl->size) j--;
	for (k = 0; k < 4; k++) {
		c[k].x = bz.list[j + k].x;
		c[k].y = bz.list[j + k].y;
	}
	low = 0.0; high = 1.0;
	dlow = dist(c[0],pt);
	dhigh = dist(c[3],pt);
	do {
		t = (low + high) / 2.0;
		pt2 = Bezier (c, 3, t, NULL, NULL);
		if (fabs(dlow - dhigh) < 1.0) break;
		if (low == high) break;
		if (dlow < dhigh) {high = t; dhigh = dist(pt2,pt);}
		else {low = t; dlow = dist(pt2,pt); }
	} while (1);
	rv.x = (int)pt2.x;
	rv.y = (int)pt2.y;
	return rv;
}

point spline_at_y(splines *spl, int y)
{
	int i,j;
	double low, high, d, t;
	pointf c[4], pt2;
	point pt;
	static bezier bz;
	static splines *mem = NULL;

	if (mem != spl) {
		mem = spl;
		for (i = 0; i < spl->size; i++) {
			bz = spl->list[i];
			if (BETWEEN (bz.list[bz.size-1].y, y, bz.list[0].y))
				break;
		}
	}
	if (y > bz.list[0].y)
		pt = bz.list[0];
	else if (y < bz.list[bz.size-1].y)
		pt = bz.list[bz.size - 1];
	else {
		for (i = 0; i < bz.size; i += 3) {
			for (j = 0; j < 3; j++) {
				if ((bz.list[i+j].y <= y) && (y <= bz.list[i+j+1].y))
					break;
				if ((bz.list[i+j].y >= y) && (y >= bz.list[i+j+1].y))
					break;
			}
			if (j < 3)
				break;
		}
		assert (i < bz.size);
		for (j = 0; j < 4; j++) {
			c[j].x = bz.list[i + j].x;
			c[j].y = bz.list[i + j].y;
			/* make the spline be monotonic in Y, awful but it works for now */
			if ((j > 0) && (c[j].y > c[j - 1].y))
				c[j].y = c[j - 1].y;
		}
		low = 0.0; high = 1.0;
		do {
			t = (low + high) / 2.0;
			pt2 = Bezier (c, 3, t, NULL, NULL);
			d = pt2.y - y;
			if (ABS(d) <= 1)
				break;
			if (d < 0)
				high = t;
			else
				low = t;
		} while (1);
		pt.x = (int)pt2.x;
		pt.y = (int)pt2.y;
	}
	pt.y = y;
	return pt;
}

point neato_closest (splines *spl, point p)
{
/* this is a stub so that we can share a common emit.c between dot and neato */

	return spline_at_y(spl, p.y);
}

static int       Tflag;
void toggle(int s)
{
        Tflag = !Tflag;
#ifndef MSWIN32
        signal (SIGUSR1, toggle);
#endif
}

int test_toggle()
{
	return Tflag;
}

void intr(int s)
{
#if 0    /* FIXME - how to pass args through a signal */
        if (G) dotneato_write(G);
#endif
        dotneato_terminate();
        exit(1);
}

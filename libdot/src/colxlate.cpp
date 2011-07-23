/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/
#include "render.h"

#ifndef NOCOLORNAMES
#include "colortbl.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

char *
canoncolor(char *orig, char *out)
{
	char	c,*p = out;
	while ((c = *orig++)) {
		if (isalnum(c) == FALSE) continue;
		if (isupper(c)) c = tolower(c);
		*out++ = c;
	}
	*out = c;
	return p;
}

int
colorcmpf(void *p0, void *p1)
{
	int		i = (((hsbcolor_t*)p0)->name[0] - ((hsbcolor_t*)p1)->name[0]);
	return (i ? i : strcmp(((hsbcolor_t*)p0)->name,((hsbcolor_t*)p1)->name));
}

int color2rgb(char *str, double rgb[3])
{
	static	hsbcolor_t	*last;
	char				*p,canon[SMALLBUF],buf[BUFSIZ];
	hsbcolor_t			fake;
	double				hsv[3];

	if ((last == NULL)||(last->name[0] != str[0])||(strcmp(last->name,str))) {
		fake.name = canoncolor(str,canon);
		last = (hsbcolor_t*) bsearch((void*)&fake,(void*)color_lib,sizeof(color_lib)/sizeof(hsbcolor_t),sizeof(fake),(bsearch_cmpf)colorcmpf);
	}
	if (last == NULL) {
		if (isdigit(canon[0]) == FALSE) return 0;
		else {
			for (p = buf; (*p = *str++); p++) if (*p == ',') *p = ' ';
			sscanf(buf,"%lf%lf%lf",&hsv[0],&hsv[1],&hsv[2]);
		}
	}
	else {
		hsv[0] = ((double)last->h)/255;
		hsv[1] = ((double)last->s)/255;
		hsv[2] = ((double)last->b)/255;
	}
	hsv2rgb(&rgb[0],&rgb[1],&rgb[2],hsv[0],hsv[1],hsv[2]);
	return 1;
}

char *
colorxlate(char *str, char *buf)
{
	static	hsbcolor_t	*last;
	char				*p,canon[SMALLBUF];
	hsbcolor_t			fake;

	if ((last == NULL)||(last->name[0] != str[0])||(strcmp(last->name,str))) {
		fake.name = canoncolor(str,canon);
		last = (hsbcolor_t*) bsearch((void*)&fake,(void*)color_lib,sizeof(color_lib)/sizeof(hsbcolor_t),sizeof(fake),(bsearch_cmpf)colorcmpf);
	}
	if (last == NULL) {
		if (isdigit(canon[0]) == FALSE) {
			fprintf(stderr,"warning: %s is not a known color\n",str);
			strcpy(buf,str);
		}
		else for (p = buf; (*p = *str++); p++) if (*p == ',') *p = ' ';
	}
	else {
		sprintf(buf,"%.3f %.3f %.3f",((double)last->h)/255,((double)last->s)/255,((double)last->b)/255);
	}
	return buf;
}
#else
char * colorxlate(char *str, char *buf) {return str;}
#endif

void hsv2rgb(double *r, double *g, double *b, double h, double s, double v)
{
	int i;
	double f,p,q,t;

	if (s <= 0.0) {	/* achromatic */
		*r = v;
		*g = v;
		*b = v;
	}
	else {
		if (h >= 1.0) h = 0.0;
		h = 6.0 * h;
		i = (int)h;
		f = h - (double)i;
		p = v * (1 - s);
		q = v * (1 - (s * f));
		t = v * ( 1 - (s * (1 - f)));
		switch (i) {
			case 0: *r = v; *g = t; *b = p; break;
			case 1: *r = q; *g = v; *b = p; break;
			case 2: *r = p; *g = v; *b = t; break;
			case 3: *r = p; *g = q; *b = v; break;
			case 4: *r = t; *g = p; *b = v; break;
			case 5: *r = v; *g = p; *b = q; break;
		}
	}
}

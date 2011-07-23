/********************************************/
/* gd interface to freetype library         */
/*                                          */
/* John Ellson   ellson@lucent.com          */
/********************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "gd.h"
#include "gdhelpers.h"

#ifndef MSWIN32
#include <unistd.h>
#else
#define R_OK 4
#endif

/* number of antialised colors for indexed bitmaps */
#define NUMCOLORS 8

#ifndef HAVE_LIBFREETYPE
char * gdImageStringFT(gdImage *im, int *brect, int fg, char *fontlist,
	double ptsize, double angle, int x, int y, char *string)
{
#ifdef HAVE_LIBTTF
	return gdImageStringTTF(im,brect,fg,fontlist,ptsize,angle,x,y,string);
#else
	return "libgd was not built with FreeType font support\n";
#endif
}
#else

#include "gdcache.h"
#include "freetype/freetype.h"
#include "freetype/ftglyph.h"

/* number of fonts cached before least recently used is replaced */
#define FONTCACHESIZE 6

/* number of antialias color lookups cached */
#define TWEENCOLORCACHESIZE 32

/*
 * Line separation as a factor of font height.  
 *	No space between if LINESPACE = 1.00 
 *	Line separation will be rounded up to next pixel row.
 */
#define LINESPACE 1.05

/*
 * The character (space) used to separate alternate fonts in the
 * fontlist parameter to gdImageStringFT.
 */
#define LISTSEPARATOR " "

/*
 * DEFAULT_FONTPATH and PATHSEPARATOR are host type dependent and
 * are normally set by configure in gvconfig.h.  These are just
 * some last resort values that might match some Un*x system
 * if building this version of gd separate from graphviz.
 */
#ifndef DEFAULT_FONTPATH
#define DEFAULT_FONTPATH "/usr/share/fonts/truetype"
#endif
#ifndef PATHSEPARATOR
#define PATHSEPARATOR ":"
#endif

#ifndef TRUE
#define FALSE 0
#define TRUE !FALSE
#endif

#define MAX(a,b) ((a)>(b)?(a):(b))
#define MIN(a,b) ((a)<(b)?(a):(b))

typedef struct {
	char		*fontlist;	/* key */
	FT_Library	*library;
	FT_Face		face;
	FT_Bool		have_char_map_unicode,
			have_char_map_big5,
			have_char_map_sjis,
			have_char_map_apple_roman;
	gdCache_head_t	*glyphCache;
} font_t;

typedef struct {
	char		*fontlist;	/* key */
	FT_Library	*library;
} fontkey_t;

typedef struct { 
    unsigned char       pixel;		/* key */
    unsigned char       bgcolor;	/* key */
    int			fgcolor;	/* key */ /* -ve means no antialias */
    gdImagePtr          im;		/* key */
    unsigned char       tweencolor;
} tweencolor_t;

typedef struct {
    unsigned char       pixel;		/* key */
    unsigned char       bgcolor;	/* key */
    int			fgcolor;	/* key */ /* -ve means no antialias */
    gdImagePtr          im;		/* key */
} tweencolorkey_t;  

/********************************************************************
 * gdTcl_UtfToUniChar is borrowed from Tcl ...
 */
/*
 * tclUtf.c --
 *
 *	Routines for manipulating UTF-8 strings.
 *
 * Copyright (c) 1997-1998 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tclUtf.c 1.25 98/01/28 18:02:43
 */

/*
 *---------------------------------------------------------------------------
 *
 * gdTcl_UtfToUniChar --
 *
 *	Extract the Tcl_UniChar represented by the UTF-8 string.  Bad
 *	UTF-8 sequences are converted to valid Tcl_UniChars and processing
 *	continues.  Equivalent to Plan 9 chartorune().
 *
 *	The caller must ensure that the source buffer is long enough that
 *	this routine does not run off the end and dereference non-existent
 *	memory looking for trail bytes.  If the source buffer is known to
 *	be '\0' terminated, this cannot happen.  Otherwise, the caller
 *	should call Tcl_UtfCharComplete() before calling this routine to
 *	ensure that enough bytes remain in the string.
 *
 * Results:
 *	*chPtr is filled with the Tcl_UniChar, and the return value is the
 *	number of bytes from the UTF-8 string that were consumed.
 *
 * Side effects:
 *	None.
 *
 *---------------------------------------------------------------------------
 */

#ifdef JISX0208
#include "jisx0208.h"
#endif
 
#define Tcl_UniChar int
#define TCL_UTF_MAX 3
static int
gdTcl_UtfToUniChar(char *str, Tcl_UniChar *chPtr)
/* str is the UTF8 next character pointer */
/* chPtr is the int for the result */
{
	int byte;
    
	/* HTML4.0 entities in decimal form, e.g. &#197; */
	byte = *((unsigned char *) str);
	if (byte == '&') {
		int i, n=0;

		byte = *((unsigned char *) (str+1));
		if (byte == '#') {
			for (i = 2; i < 8; i++) {
				byte = *((unsigned char *) (str+i));
				if (byte >= '0' && byte <= '9') {
					n = (n * 10) + (byte - '0');
				} 
				else
					break;
			}
			if (byte == ';') {
				*chPtr = (Tcl_UniChar) n;
				return ++i;
			}
		}
	}
	 
	/*
	 * Unroll 1 to 3 byte UTF-8 sequences, use loop to handle longer ones.
	 */

	byte = *((unsigned char *) str);
#ifdef JISX0208
	if (0xA1 <= byte && byte <= 0xFE) {
		int jiscode, ku, ten;

		jiscode = 0x100 * (byte & 0x7F) + (str[1] & 0x7F);
		ku = (jiscode >> 8) - 0x20;
		ten = (jiscode % 256) - 0x20;
		if ( (ku < 1 || ku > 92) || (ten < 1 || ten > 94) ) {
			*chPtr = (Tcl_UniChar) byte;
			return 1;
		}

		*chPtr = (Tcl_UniChar) UnicodeTbl[ku - 1][ten - 1];
		return 2;
	} else
#endif /* JISX0208 */
	if (byte < 0xC0) {
		/*
		 * Handles properly formed UTF-8 characters between
		 * 0x01 and 0x7F.  Also treats \0 and naked trail
		 * bytes 0x80 to 0xBF as valid characters representing
		 * themselves.
		 */

		*chPtr = (Tcl_UniChar) byte;
		return 1;
	} else if (byte < 0xE0) {
		if ((str[1] & 0xC0) == 0x80) {
			/*
			 * Two-byte-character lead-byte followed
			 * by a trail-byte.
			 */
	     
			*chPtr = (Tcl_UniChar) (((byte & 0x1F) << 6)
				| (str[1] & 0x3F));
			return 2;
		}
		/*
	 	 * A two-byte-character lead-byte not followed by trail-byte
	 	 * represents itself.
	 	 */
	 	
		*chPtr = (Tcl_UniChar) byte;
		return 1;
    	} else if (byte < 0xF0) {
		if (((str[1] & 0xC0) == 0x80) && ((str[2] & 0xC0) == 0x80)) {
			/*
			 * Three-byte-character lead byte followed by
			 * two trail bytes.
			 */
	
	    		*chPtr = (Tcl_UniChar) (((byte & 0x0F) << 12) 
		    		| ((str[1] & 0x3F) << 6) | (str[2] & 0x3F));
	    		return 3;
		}
		/*
		 * A three-byte-character lead-byte not followed by
		 * two trail-bytes represents itself.
		 */
	
		*chPtr = (Tcl_UniChar) byte;
		return 1;
    	}
#if TCL_UTF_MAX > 3
	else {
		int ch, total, trail;

		total = totalBytes[byte];
		trail = total - 1;
		if (trail > 0) {
			ch = byte & (0x3F >> trail);
			do {
				str++;
				if ((*str & 0xC0) != 0x80) {
					*chPtr = byte;
					return 1;
				}
				ch <<= 6;
				ch |= (*str & 0x3F);
				trail--;
			} while (trail > 0);
			*chPtr = ch;
			return total;
		}
	}
#endif

	*chPtr = (Tcl_UniChar) byte;
	return 1;
}

/********************************************************************/
/* font cache functions                                             */

static int
fontTest ( void *element, void *key )
{
	font_t *a=(font_t *)element;
	fontkey_t *b=(fontkey_t *)key;

	return (strcmp(a->fontlist, b->fontlist) == 0);
}

static void *
fontFetch ( char **error, void *key )
{
	font_t			*a;
	fontkey_t		*b=(fontkey_t *)key;
	int			n;
	int			font_found=0;
	unsigned short		platform, encoding;
	char			*fontsearchpath, *fontpath, *fontlist;
	char			*fullname = NULL;
	char			*name, *path, *dir;
	char			*strtok_ptr;
	FT_Error		err;
	FT_CharMap		found=0;
	FT_CharMap		charmap;

	a = (font_t *)gdMalloc(sizeof(font_t));
	a->fontlist = strdup(b->fontlist);
	a->library = b->library;

	/*
	 * Search the pathlist for any of a list of font names.
	 */
	fontsearchpath = getenv("GDFONTPATH");
	if (! fontsearchpath ) fontsearchpath = DEFAULT_FONTPATH;
	fontlist = strdup(a->fontlist);

	/*
	 * Must use gd_strtok_r else pointer corrupted by strtok in nested loop.
	 */
	for (name = gd_strtok_r(fontlist,LISTSEPARATOR,&strtok_ptr); name;
			name = gd_strtok_r(0,LISTSEPARATOR,&strtok_ptr)) {

		/* make a fresh copy each time - strtok corrupts it. */
        	path = strdup(fontsearchpath);
        	/*
		 * Allocate an oversized buffer that is guaranteed to be
		 * big enough for all paths to be tested.
		 */
		fullname = gdRealloc(fullname,
			strlen(fontsearchpath) + strlen(name) + 6);
		/* if name is an absolute filename then test directly */
		if (*name == '/' || (name[0]!=0 && name[1]==':' && (name[2]=='/' || name[2]=='\\') ) ) {
                	sprintf(fullname,"%s",name);
                	if (access(fullname,R_OK) == 0) {font_found++; break;}
		}
        	for (dir = strtok(path,PATHSEPARATOR); dir;
				dir = strtok(0,PATHSEPARATOR)) {
                	sprintf(fullname,"%s/%s.ttf",dir,name);
                	if (access(fullname,R_OK) == 0) {font_found++; break;}
        	}
		if (font_found) break;
       		gdFree(path);
	}
        gdFree(fontlist);
        if (! font_found) {
                *error = "Could not find/open font";
		return NULL;
        }

	err = FT_New_Face(*b->library, fullname, 0, &a->face);
	if (err) {
		*error = "Could not read font";
		return NULL;
	}
       	gdFree(fullname);
	
/* FIXME - This mapping stuff is imcomplete - where is the spec? */

	a->have_char_map_unicode = 0;
	a->have_char_map_big5 = 0;
	a->have_char_map_sjis = 0;
	a->have_char_map_apple_roman = 0;
	for (n = 0; n < a->face->num_charmaps; n++) {
		charmap = a->face->charmaps[n];
		platform = charmap->platform_id;
		encoding = charmap->encoding_id;
		if ((platform == 3 && encoding == 1)	/* Windows Unicode */
		 || (platform == 3 && encoding == 0)	/* Windows Symbol */
		 || (platform == 2 && encoding == 1)	/* ISO Unicode */
		 || (platform == 0)) {			/* Apple Unicode */
			a->have_char_map_unicode = 1;
			found = charmap;
		} else
		if (platform == 3 && encoding == 4) {	/* Windows Big5 */
			a->have_char_map_big5 = 1;
			found = charmap;
		} else
		if (platform == 3 && encoding == 2) {	/* Windows Sjis */
			a->have_char_map_sjis = 1;
			found = charmap;
		} else
		if ((platform == 1 && encoding == 0)	/* Apple Roman */
		 || (platform == 2 && encoding == 0)) {	/* ISO ASCII */
			a->have_char_map_apple_roman = 1;
			found = charmap;
		}
	}
	if (!found) {
		*error = "Unable to find a CharMap that I can handle";
		return NULL;
	}

	return (void *)a;
}

static void
fontRelease( void *element )
{
	font_t *a=(font_t *)element;

	FT_Done_Face(a->face);
	gdFree(a->fontlist);
	gdFree( (char *)element );
}

/********************************************************************/
/* tweencolor cache functions                                            */

static int
tweenColorTest (void *element, void *key)
{ 
    tweencolor_t *a=(tweencolor_t *)element;
    tweencolorkey_t *b=(tweencolorkey_t *)key;
    
    return (a->pixel == b->pixel    
         && a->bgcolor == b->bgcolor
         && a->fgcolor == b->fgcolor
         && a->im == b->im);
} 

/*
 * Computes a color in im's color table that is part way between
 * the background and foreground colors proportional to the gray
 * pixel value in the range 0-NUMCOLORS. The fg and bg colors must already
 * be in the color table.
 */
static void *
tweenColorFetch (char **error, void *key)
{
	tweencolor_t *a;
	tweencolorkey_t *b=(tweencolorkey_t *)key;
	int pixel, npixel, bg, fg;
	gdImagePtr im;
   
	a = (tweencolor_t *)gdMalloc(sizeof(tweencolor_t));
	pixel = a->pixel = b->pixel;
	bg = a->bgcolor = b->bgcolor;
	fg = a->fgcolor = b->fgcolor;
	im = b->im;

	/* if fg is specified by a negative color idx, then don't antialias */
	if (fg <0) {
		a->tweencolor = -fg;
	} else {
		npixel = NUMCOLORS - pixel;
		a->tweencolor = gdImageColorResolve(im,
			(pixel * im->red[fg] + npixel * im->red[bg]) / NUMCOLORS,
			(pixel * im->green[fg] + npixel * im->green[bg]) / NUMCOLORS,
			(pixel * im->blue[fg] + npixel * im->blue[bg]) / NUMCOLORS);
	}
	return (void *)a;
}   
        
static void
tweenColorRelease(void *element)
{   
    gdFree((char *)element);
}   

/* draw_bitmap - transfers glyph bitmap to GD image */
static char *
gdft_draw_bitmap(gdCache_head_t *tc_cache, gdImage *im, int fg, FT_Bitmap bitmap, int pen_x, int pen_y) {
	char			*pixel;
	int			x, y, row, col, pc;

	tweencolor_t		*tc_elem;
	tweencolorkey_t		tc_key;

	/* copy to gif, mapping colors */
	tc_key.fgcolor = fg;
	tc_key.im = im;
	for (row = 0; row < bitmap.rows; row++) {
		pc = row * bitmap.pitch;
		if(bitmap.pixel_mode==ft_pixel_mode_mono)
			pc *= 8;    /* pc is measured in bits for monochrome images */

		y = pen_y + row;

 		/* clip if out of bounds */
		if (y >= im->sy || y < 0) continue;

		for (col = 0; col < bitmap.width; col++, pc++) {
			if (bitmap.pixel_mode == ft_pixel_mode_grays) {
				/*
				 * Round to NUMCOLORS levels of antialiasing for
				 * index color images since only 256 colors are
				 * available.
				 */
				tc_key.pixel = ((bitmap.buffer[pc] * NUMCOLORS)
					 + bitmap.num_grays/2)
						/ (bitmap.num_grays - 1);
			} else
			if (bitmap.pixel_mode == ft_pixel_mode_mono) {
				tc_key.pixel = ((bitmap.buffer[pc/8]
					<< (pc%8)) & 128) ? NUMCOLORS : 0;
			} else {
				return "Unsupported ft_pixel_mode";
			}

			if (tc_key.pixel > 0) { /* if not background */
				x = pen_x + col;

				/* clip if out of bounds */
				if (x >= im->sx || x < 0) continue;

				/* get pixel location in gd buffer */
				pixel = &im->pixels[y][x];
				if (tc_key.pixel == NUMCOLORS) {
					/* use fg color directly */
					*pixel = abs( fg );
				} else {
					/* find antialised color */
					tc_key.bgcolor = *pixel;
					tc_elem = (tweencolor_t *)gdCacheGet(
						tc_cache, &tc_key);
					*pixel = tc_elem->tweencolor;
				}
			}
		}
	}
	return (char *)NULL;
}

extern int any2eucjp(char *, char *, unsigned int);

/********************************************************************/
/* gdImageStringFT -  render a utf8 string onto a gd image          */ 

char * gdImageStringFT(gdImage *im, int *brect, int fg, char *fontlist,
		double ptsize, double angle, int x, int y, char *string)
{
	FT_F26Dot6	ur_x=0, ur_y=0, ll_x=0, ll_y=0;
	FT_F26Dot6	advance_x, advance_y;
	FT_BBox		bbox;
	FT_Matrix	matrix;
	FT_Vector	pen, delta;
	FT_Face		face;
	FT_Glyph	image;
	FT_GlyphSlot	slot;
	FT_Error	err;
	FT_Bool		use_kerning;
	FT_UInt		glyph_index, previous;
	double		sin_a = sin(angle);
	double		cos_a = cos(angle);
	int		len, i=0, ch;
	int		x1=0, y1=0;
	font_t		*font;
	fontkey_t	fontkey;
	char		*next;
	char		*tmpstr = 0;
	int             render_mode = FT_LOAD_RENDER;

	/***** initialize font library and font cache on first call ******/
	static gdCache_head_t	*fontCache;
	static FT_Library 	library;

	/* 
	 * make a new tweenColorCache on every call 
	 * because caching colormappings between calls
	 * is not safe. If the im-pointer points to a 
	 * brand new image, the cache gives out bogus
	 * colorindexes.          -- 27.06.2001 <krisku@arrak.fi>
	 */
	gdCache_head_t	*tc_cache;

	tc_cache = gdCacheCreate( TWEENCOLORCACHESIZE,
	                  tweenColorTest, tweenColorFetch, tweenColorRelease );

	if (! fontCache) {
		if (FT_Init_FreeType(&library)) {
			gdCacheDelete( tc_cache );
			return "Failure to initialize font library";
		}
		fontCache = gdCacheCreate( FONTCACHESIZE,
			fontTest, fontFetch, fontRelease);
	}
	/*****/

	/* get the font (via font cache) */
	fontkey.fontlist = fontlist;
	fontkey.library = &library;
	font = (font_t *)gdCacheGet(fontCache, &fontkey);
	if (! font) {
        	gdCacheDelete( tc_cache );
		return fontCache->error;
	}
	face = font->face;		/* shortcut */
	slot = face->glyph;  		/* shortcut */

	if (FT_Set_Char_Size(face, 0, (FT_F26Dot6)(ptsize*64),
			GD_RESOLUTION, GD_RESOLUTION)) {
        	gdCacheDelete( tc_cache );
		return "Could not set character size";
	}

	matrix.xx = (FT_Fixed) (cos_a * (1<<16));
	matrix.yx = (FT_Fixed) (sin_a * (1<<16));
	matrix.xy = - matrix.yx;
	matrix.yy = matrix.xx;

	pen.x = pen.y = 0;	   /* running position of rotated string */

	use_kerning = FT_HAS_KERNING(face);
	previous = 0;

	advance_x = advance_y = 0; /* running position (26.6) of right string */

	if(fg<0)
		render_mode |= FT_LOAD_MONOCHROME;

#ifndef JISX0208
	if (font->have_char_map_sjis) {
#endif
		if (tmpstr = (char *)gdMalloc(BUFSIZ)) {
			any2eucjp(tmpstr, string, BUFSIZ);
			next=tmpstr;
		} else {
			next=string;
		}
#ifndef JISX0208
	} else {
		next=string;
	}
#endif
	while (*next) {	  
		ch = *next;

		/* carriage returns */
		if (ch == '\r') {
			advance_x = 0;
			x1 = (advance_x * cos_a - advance_y * sin_a + 32) / 64;
			y1 = (advance_x * sin_a + advance_y * cos_a + 32) / 64;
			pen.x = pen.y = 0;
			previous = 0; /* clear kerning flag */
			next++;
			continue;
		}
		/* newlines */
		if (ch == '\n') {
			advance_y -= face->size->metrics.height * LINESPACE;
			advance_y = (advance_y-32) & -64; /* round to next pixel row */
			x1 = (advance_x * cos_a - advance_y * sin_a + 32) / 64;
			y1 = (advance_x * sin_a + advance_y * cos_a + 32) / 64;
			pen.x = pen.y = 0;
			previous = 0; /* clear kerning flag */
			next++;
			continue;
		}

		if (font->have_char_map_unicode) {
			/* use UTF-8 mapping from ASCII */
			len = gdTcl_UtfToUniChar(next, &ch);
			next += len;
		} else if (font->have_char_map_sjis) {
			unsigned char c;
			int jiscode;
	
			c = *next;
			if ( 0xA1 <= c && c <= 0xFE ) {
				next++;
				jiscode = 0x100 * (c & 0x7F) + ((*next) & 0x7F);
	
				ch = (jiscode >> 8) & 0xFF;
				jiscode &= 0xFF;
	
				if (ch & 1) jiscode += 0x40 - 0x21;
				else        jiscode += 0x9E - 0x21;
	
				if (jiscode >= 0x7F) jiscode++;
				ch = (ch - 0x21) / 2 + 0x81;
				if (ch >= 0xA0) ch += 0x40;
	
				ch = (ch << 8) + jiscode;
			} else {
				ch = c & 0xFF; /* don't extend sign */
			}
			next++;
		} else {
			/*
		 	 * Big 5 mapping:
		 	 * use "JIS-8 half-width katakana" coding from 8-bit characters. Ref:
		 	 * ftp://ftp.ora.com/pub/examples/nutshell/ujip/doc/japan.inf-032092.sjs
		 	 */
			ch = (*next) & 0xFF;	/* don't extend sign */
			next++;
			if (ch >= 161		/* first code of JIS-8 pair */
		 	    && *next) {		/* don't advance past '\0' */
				/* TBB: Fix from Kwok Wah On: & 255 needed */
				ch = (ch * 256) + ((*next) & 255);
				next++;
			}
		}
	
		FT_Set_Transform(face, &matrix, NULL);
	
		/* Convert character code to glyph index */
		glyph_index = FT_Get_Char_Index( face, ch );

		/* retieve kerning distance and move pen position */
		if ( use_kerning && previous && glyph_index ) {
			FT_Get_Kerning( face, previous, glyph_index, 
				ft_kerning_default, &delta );
			pen.x += delta.x >> 6;
		}

		/* load glyph image into the slot (erase previous one) */
		err = FT_Load_Glyph(face, glyph_index, render_mode);
		if (err) {
            		gdCacheDelete( tc_cache );
			return "Problem loading glyph";
		}

		/* if null *im, or invalid color,
			 then assume user just wants brect */
		if (im && fg <= 255 && fg >= -255) {

        		/* now, draw to our target surface */
			gdft_draw_bitmap(tc_cache, im, fg, slot->bitmap,
				x + x1 + pen.x + slot->bitmap_left,
				y - y1 + pen.y - slot->bitmap_top);
		}

       		/* increment pen position */
		pen.x += slot->advance.x >> 6;
		pen.y -= slot->advance.y >> 6;

		/* record current glyph index for kerning */
		previous = glyph_index;
	
		if (brect) { /* only if need brect */
			if (! i++) { /* if first character, init BB corner values */
                                ll_x = slot->metrics.horiBearingX;
                                ll_y = slot->metrics.horiBearingY - slot->metrics.height;
                                ur_x = slot->metrics.horiBearingX + slot->metrics.width;
                                ur_y = slot->metrics.horiBearingY;
                        }
                        else {
                                if (! advance_x) ll_x = MIN(slot->metrics.horiBearingX, ll_x);
                                ll_y = MIN(advance_y + slot->metrics.horiBearingY - slot->metrics.height, ll_y);
                                ur_x = MAX(advance_x + slot->metrics.horiBearingX + slot->metrics.width, ur_x);
                                if (! advance_y) ur_y = MAX(slot->metrics.horiBearingY, ur_y);
                        }
		}

		advance_x += slot->metrics.horiAdvance;
	}

	if (brect) {  /* only if need brect */
		/* rotate bounding rectangle */
		brect[0] = (int)(ll_x * cos_a - ll_y * sin_a);
		brect[1] = (int)(ll_x * sin_a + ll_y * cos_a);
		brect[2] = (int)(ur_x * cos_a - ll_y * sin_a);
		brect[3] = (int)(ur_x * sin_a + ll_y * cos_a);
		brect[4] = (int)(ur_x * cos_a - ur_y * sin_a);
		brect[5] = (int)(ur_x * sin_a + ur_y * cos_a);
		brect[6] = (int)(ll_x * cos_a - ur_y * sin_a);
		brect[7] = (int)(ll_x * sin_a + ur_y * cos_a);
	
		/* scale, round and offset brect */
		i = 0;
		while (i<8) {
			brect[i] = x + (brect[i] + 32) / 64;
			i++;
			brect[i] = y - (brect[i] + 32) / 64;
			i++;
		}
	}

	if ( tmpstr ) gdFree(tmpstr);
	gdCacheDelete( tc_cache );
	return (char *)NULL;
}

#endif /* HAVE_LIBFREETYPE */

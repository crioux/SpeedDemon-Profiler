/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/


#if defined(_BLD_dotneato) && defined(_DLL)
#   define external __EXPORT__
#endif
#if !defined(_BLD_dotneato) && defined(__IMPORT__)
#   define external __IMPORT__
#endif
#ifndef external
#   define external   extern
#endif
#ifndef EXTERN
#define EXTERN extern
#endif

extern		char		*Version;
EXTERN		char		**Files;			/* from command line */
EXTERN		char		**Lib;				/* from command line */
EXTERN		char		*CmdName;
external	int		Output_lang;			/* POSTSCRIPT, DOT, etc. */
external	codegen_t	*CodeGen;
external	FILE		*Output_file;
EXTERN		boolean		Verbose,Reduce;
EXTERN		char		*Output_file_name;
EXTERN		int			Nop;
EXTERN		double		PSinputscale;
extern		double		Epsilon,Nodesep,Nodefactor;
//extern		int			MaxIter;
extern		int			Syntax_errors;
extern		char    	*Info[];            /* from input.c */
EXTERN		int			Show_boxes;         /* emit code for correct box coordinates */
EXTERN		int			CL_type;            /* NONE, LOCAL, GLOBAL */
EXTERN		boolean		Concentrate;        /* if parallel edges should be merged */

/* for neato */
EXTERN		double      Epsilon;    /* defined in input_graph */
EXTERN		double      Nodesep;
EXTERN		double      Nodefactor;
//EXTERN		int         MaxIter;
EXTERN		double      Initial_dist;


/* was external? */
//extern codegen_t /* FIG_CodeGen, */ GD_CodeGen, HPGL_CodeGen, 
//		ISMAP_CodeGen, IMAP_CodeGen, MIF_CodeGen, MP_CodeGen,
//		PIC_CodeGen, PS_CodeGen, SVG_CodeGen, VRML_CodeGen, VTX_CodeGen; 

EXTERN attrsym_t	
		*N_height, *N_width, *N_shape, *N_color, *N_fillcolor,
		*N_fontsize, *N_fontname, *N_fontcolor,
		*N_label, *N_style, *N_showboxes,
		*N_sides,*N_peripheries,*N_orientation,
		*N_skew,*N_distortion,*N_fixed,*N_layer,
		*N_group,*N_comment,*N_vertices,*N_z;

EXTERN attrsym_t	*E_weight, *E_minlen, *E_color,
					*E_fontsize, *E_fontname, *E_fontcolor,
					*E_label, *E_dir, *E_style, *E_decorate,
					*E_showboxes,*E_arrowsz,*E_constr,*E_layer,
					*E_comment;
/* vladimir */
EXTERN attrsym_t	*E_samehead, *E_sametail,
					*E_arrowhead, *E_arrowtail,
					*E_headlabel, *E_taillabel,
					*E_labelfontsize, *E_labelfontname, *E_labelfontcolor,
					*E_labeldistance, *E_labelangle;

/* north */
EXTERN attrsym_t    *E_tailclip, *E_headclip;
#undef external


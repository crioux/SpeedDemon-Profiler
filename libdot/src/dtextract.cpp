/*
    This software may only be used by you under license from AT&T Corp.
    ("AT&T").  A copy of AT&T's Source Code Agreement is available at
    AT&T's Internet website having the URL:
    <http://www.research.att.com/sw/tools/graphviz/license/source.html>
    If you received this software without first entering into a license
    with AT&T, you have an infringing copy of this software and cannot use
    it without violating AT&T's intellectual property rights.
*/
#include	"dthdr.h"

#ifdef DMALLOC
#include "dmalloc.h"
#endif

/*	Extract objects of a dictionary.
**
**	Written by Kiem-Phong Vo (5/25/96).
*/

#if __STD_C
Dtlink_t* dtextract(reg Dt_t* dt)
#else
Dtlink_t* dtextract(dt)
reg Dt_t*	dt;
#endif
{
	reg Dtlink_t	*list, **s, **ends;

	if(dt->data->type&(DT_OSET|DT_OBAG) )
		list = dt->data->here;
	else if(dt->data->type&(DT_SET|DT_BAG))
	{	list = dtflatten(dt);
		for(ends = (s = dt->data->htab) + dt->data->ntab; s < ends; ++s)
			*s = NIL(Dtlink_t*);
	}
	else /*if(dt->data->type&(DT_LIST|DT_STACK|DT_QUEUE))*/
	{	list = dt->data->head;
		dt->data->head = NIL(Dtlink_t*);
	}

	dt->data->type &= ~DT_FLATTEN;
	dt->data->size = 0;
	dt->data->here = NIL(Dtlink_t*);

	return list;
}

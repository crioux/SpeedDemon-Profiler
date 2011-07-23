#include "gvconfig.h"
#ifndef HAVE_STRCASECMP

#include <string.h>
#include <ctype.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

int
strcasecmp(char *s1, char *s2)
{
	while (*s1 != '\0' && tolower(*s1) == tolower(*s2))
	{
		s1++;
		s2++;
	}

	return tolower(*(unsigned char *) s1) - tolower(*(unsigned char *) s2);
}

#endif /* HAVE_STRCASECMP */

#include "gvconfig.h"
#ifndef HAVE_STRNCASECMP

#include <string.h>
#include <ctype.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

int 
strncasecmp(char *s1, char *s2, size_t n)
{
	if (n == 0)
		return 0;

	while (n-- != 0 && tolower(*s1) == tolower(*s2))
		{
		if (n == 0 || *s1 == '\0' || *s2 == '\0')
			break;
		s1++;
		s2++;
	}

	return tolower(*(unsigned char *) s1) - tolower(*(unsigned char *) s2);
}

#endif /* HAVE_STRNCASECMP */

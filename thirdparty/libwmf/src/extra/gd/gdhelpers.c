#include "gd.h"
#include "gdhelpers.h"
#include <stdlib.h>
#include <string.h>
#include <limits.h>

/* TBB: gd_strtok_r is not portable; provide an implementation */

#define SEP_TEST (separators[*((unsigned char *) s)])

char *
gd_strtok_r (char *s, char *sep, char **state)
{
  char separators[256];
  char *result = 0;
  memset (separators, 0, sizeof (separators));
  while (*sep)
    {
      separators[*((unsigned char *) sep)] = 1;
      sep++;
    }
  if (!s)
    {
      /* Pick up where we left off */
      s = *state;
    }
  /* 1. EOS */
  if (!(*s))
    {
      *state = s;
      return 0;
    }
  /* 2. Leading separators, if any */
  if (SEP_TEST)
    {
      do
	{
	  s++;
	}
      while (SEP_TEST);
      /* 2a. EOS after separators only */
      if (!(*s))
	{
	  *state = s;
	  return 0;
	}
    }
  /* 3. A token */
  result = s;
  do
    {
      /* 3a. Token at end of string */
      if (!(*s))
	{
	  *state = s;
	  return result;
	}
      s++;
    }
  while (!SEP_TEST);
  /* 4. Terminate token and skip trailing separators */
  *s = '\0';
  do
    {
      s++;
    }
  while (SEP_TEST);
  /* 5. Return token */
  *state = s;
  return result;
}

void *
gdCalloc (size_t nmemb, size_t size)
{
  return calloc (nmemb, size);
}

void *
gdMalloc (size_t size)
{
  return malloc (size);
}

void *
gdRealloc (void *ptr, size_t size)
{
  return realloc (ptr, size);
}

void
gdFree (void *ptr)
{
  free (ptr);
}

int overflow2(int a, int b)
{
	if(a < 0 || b < 0) {
		fprintf(stderr, "gd warning: one parameter to a memory allocation multiplication is negative, failing operation gracefully\n");
		return 1;
	}
	if(b == 0)
		return 0;
	if(a > INT_MAX / b) {
		fprintf(stderr, "gd warning: product of memory allocation multiplication would exceed INT_MAX, failing operation gracefully\n");
		return 1;
	}
	return 0;
}

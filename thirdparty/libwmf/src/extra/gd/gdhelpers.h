#ifndef GDHELPERS_H 
#define GDHELPERS_H 1

#pragma GCC visibility push(hidden)

/* TBB: strtok_r is not universal; provide an implementation of it. */

extern char *gd_strtok_r(char *s, char *sep, char **state);

/* These functions wrap memory management. gdFree is
	in gd.h, where callers can utilize it to correctly
	free memory allocated by these functions with the
	right version of free(). */
void *gdCalloc(size_t nmemb, size_t size);
void *gdMalloc(size_t size);
void *gdRealloc(void *ptr, size_t size);

int overflow2(int a, int b);

#pragma GCC visibility pop

#endif /* GDHELPERS_H */


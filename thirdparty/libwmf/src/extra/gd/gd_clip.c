#include "gd.h"

extern void* gdMalloc (size_t);
extern void* gdRealloc (void*,size_t);
extern void  gdFree (void*);

int gdImageBoundsSafe(gdImagePtr im, int x, int y)
{	int unsafe = 0;
	int safe = 1;
	int status;
	int i;

	gdClipRectanglePtr r;

	if (y <   0    ) return (unsafe);
	if (y >= im->sy) return (unsafe);
	if (x <   0    ) return (unsafe);
	if (x >= im->sx) return (unsafe);

	if (im->clip == 0) return (safe);
	if (im->clip->count == 0) return (safe);

	status = unsafe;

	for (i = 0; i < im->clip->count; i++)
	{	r = &(im->clip->list[i]);

		if (y < r->y_min) continue;
		if (y > r->y_max) continue;
		if (x < r->x_min) continue;
		if (x > r->x_max) continue;

		status = safe;
		break;
	}

	return (status);
}

void gdClipSetFree(gdImagePtr im)
{	if (im->clip)
	{	gdFree (im->clip->list);
		gdFree (im->clip);
		im->clip = 0;
	}
}

void gdClipSetReset(gdImagePtr im)
{	if (im->clip)
	{	im->clip->count = 0;
	}
}

void gdClipSetAdd(gdImagePtr im,gdClipRectanglePtr rect)
{	gdClipRectanglePtr more;

	if (im->clip == 0)
	{	im->clip = gdMalloc (sizeof (gdClipSet));
		if (im->clip == 0) return;
		im->clip->max = 8;
		im->clip->count = 0;
		im->clip->list = gdMalloc (im->clip->max * sizeof (gdClipRectangle));
		if (im->clip->list == 0)
		{	gdFree (im->clip);
			im->clip = 0;
			return;
		}
	}
	if (im->clip->count == im->clip->max)
	{	more = gdRealloc (im->clip->list,(im->clip->max + 8) * sizeof (gdClipRectangle));
		if (more == 0) return;
		im->clip->max += 8;
                im->clip->list = more;
	}
	im->clip->list[im->clip->count] = (*rect);
	im->clip->count++;
}

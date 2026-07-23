#ifndef GD_CLIP_H
#define GD_CLIP_H 1

#pragma GCC visibility push(hidden)

#ifdef __cplusplus
extern "C" {
#endif

/* ClipRectangle type for use as part of ClipSet. */
typedef struct { /* point must lie in range: x_min <= x <= x_max, y_min <= y <= y_max */
	int x_min, y_min;
	int x_max, y_max;
} gdClipRectangle, *gdClipRectanglePtr;

/* ClipSet type */
typedef struct {
	int max;
	int count;

	gdClipRectangle* list;
} gdClipSet;

#ifdef __cplusplus
}
#endif

#pragma GCC visibility pop

#endif /* GD_CLIP_H */

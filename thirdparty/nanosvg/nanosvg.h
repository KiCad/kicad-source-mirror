/*
 * Copyright (c) 2013-14 Mikko Mononen memon@inside.org
 *
 * This software is provided 'as-is', without any express or implied
 * warranty.  In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software
 * in a product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 * misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 * The SVG parser is based on Anti-Grain Geometry 2.4 SVG example
 * Copyright (C) 2002-2004 Maxim Shemanarev (McSeem) (http://www.antigrain.com/)
 *
 * Arc calculation code based on canvg (https://code.google.com/p/canvg/)
 *
 * Bounding box calculation based on http://blog.hackers-cafe.net/2009/06/how-to-calculate-bezier-curves-bounding.html
 *
 */

#ifndef NANOSVG_H
#define NANOSVG_H


// NanoSVG is a simple stupid single-header-file SVG parse. The output of the parser is a list of cubic bezier shapes.
//
// The library suits well for anything from rendering scalable icons in your editor application to prototyping a game.
//
// NanoSVG supports a wide range of SVG features, but something may be missing, feel free to create a pull request!
//
// The shapes in the SVG images are transformed by the viewBox and converted to specified units.
// That is, you should get the same looking data as your designed in your favorite app.
//
// NanoSVG can return the paths in few different units. For example if you want to render an image, you may choose
// to get the paths in pixels, or if you are feeding the data into a CNC-cutter, you may want to use millimeters.
//
// The units passed to NanoSVG should be one of: 'px', 'pt', 'pc' 'mm', 'cm', or 'in'.
// DPI (dots-per-inch) controls how the unit conversion is done.
//
// If you don't know or care about the units stuff, "px" and 96 should get you going.


/* Example Usage:
	// Load SVG
	NSVGimage* image;
	image = nsvgParseFromFile("test.svg", "px", 96);
	printf("size: %f x %f\n", image->width, image->height);
	// Use...
	for (NSVGshape *shape = image->shapes; shape != NULL; shape = shape->next) {
		for (NSVGpath *path = shape->paths; path != NULL; path = path->next) {
			for (int i = 0; i < path->npts-1; i += 3) {
				float* p = &path->pts[i*2];
				drawCubicBez(p[0],p[1], p[2],p[3], p[4],p[5], p[6],p[7]);
			}
		}
	}
	// Delete
	nsvgDelete(image);
*/

#include <cstdio>

enum NSVGpaintType {
	NSVG_PAINT_NONE = 0,
	NSVG_PAINT_COLOR = 1,
	NSVG_PAINT_LINEAR_GRADIENT = 2,
	NSVG_PAINT_RADIAL_GRADIENT = 3
};

enum NSVGspreadType {
	NSVG_SPREAD_PAD = 0,
	NSVG_SPREAD_REFLECT = 1,
	NSVG_SPREAD_REPEAT = 2
};

enum NSVGlineJoin {
	NSVG_JOIN_MITER = 0,
	NSVG_JOIN_ROUND = 1,
	NSVG_JOIN_BEVEL = 2
};

enum NSVGlineCap {
	NSVG_CAP_BUTT = 0,
	NSVG_CAP_ROUND = 1,
	NSVG_CAP_SQUARE = 2
};

enum NSVGfillRule {
	NSVG_FILLRULE_NONZERO = 0,
	NSVG_FILLRULE_EVENODD = 1
};

enum NSVGflags {
	NSVG_FLAGS_VISIBLE = 0x01
};

typedef struct NSVGgradientStop {
	unsigned int color;
	float offset;
} NSVGgradientStop;

typedef struct NSVGgradient {
	float xform[6];
	char spread;
	float fx, fy;
	int nstops;
	NSVGgradientStop stops[1];
} NSVGgradient;

typedef struct NSVGpaint {
	char type;
	union {
		unsigned int color;
		NSVGgradient* gradient;
	};
} NSVGpaint;

typedef struct NSVGpath
{
	float* pts;					// Cubic bezier points: x0,y0, [cpx1,cpx1,cpx2,cpy2,x1,y1], ...
	int npts;					// Total number of bezier points.
	char closed;				// Flag indicating if shapes should be treated as closed.
	float bounds[4];			// Tight bounding box of the shape [minx,miny,maxx,maxy].
	struct NSVGpath* next;		// Pointer to next path, or NULL if last element.
} NSVGpath;

typedef struct NSVGshape
{
	char id[64];				// Optional 'id' attr of the shape or its group
	NSVGpaint fill;				// Fill paint
	NSVGpaint stroke;			// Stroke paint
	float opacity;				// Opacity of the shape.
	float strokeWidth;			// Stroke width (scaled).
	float strokeDashOffset;		// Stroke dash offset (scaled).
	float strokeDashArray[8];			// Stroke dash array (scaled).
	char strokeDashCount;				// Number of dash values in dash array.
	char strokeLineJoin;		// Stroke join type.
	char strokeLineCap;			// Stroke cap type.
	float miterLimit;			// Miter limit
	char fillRule;				// Fill rule, see NSVGfillRule.
	unsigned char flags;		// Logical or of NSVG_FLAGS_* flags
	float bounds[4];			// Tight bounding box of the shape [minx,miny,maxx,maxy].
	NSVGpath* paths;			// Linked list of paths in the image.
	struct NSVGshape* next;		// Pointer to next shape, or NULL if last element.
} NSVGshape;

typedef struct NSVGimage
{
	float width;				// Width of the image.
	float height;				// Height of the image.
	NSVGshape* shapes;			// Linked list of shapes in the image.
} NSVGimage;

// Parses SVG file from a file, returns SVG image as paths.
// fp will be closed after reading the file
NSVGimage* nsvgParseFromFile( FILE* fp, const char* units, float dpi);

// Parses SVG file from a null terminated string, returns SVG image as paths.
// Important note: changes the string.
NSVGimage* nsvgParse(char* input, const char* units, float dpi);

// Duplicates a path.
NSVGpath* nsvgDuplicatePath(NSVGpath* p);

// Deletes an image.
void nsvgDelete(NSVGimage* image);

#endif // NANOSVG_H

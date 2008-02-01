// PolyLine.h ... definition of CPolyLine class
//
// A polyline contains one or more contours, where each contour
// is defined by a list of corners and side-styles
// There may be multiple contours in a polyline.
// The last contour may be open or closed, any others must be closed.
// All of the corners and side-styles are concatenated into 2 arrays,
// separated by setting the end_contour flag of the last corner of 
// each contour.
//
// When used for copper areas, the first contour is the outer edge 
// of the area, subsequent ones are "holes" in the copper.

#ifndef POLYLINE2KICAD_H
#define POLYLINE2KICAD_H

#define PCBU_PER_MIL 10
#define NM_PER_MIL 10 // 25400


#include "pad_shapes.h"


class CRect {
public:
	int left, right, top, bottom;
};

class CPoint {
public:
	int x, y;
public:
	CPoint(void) { x = y = 0;};
	CPoint(int i, int j) { x = i; y = j;};
};

#endif	// #ifndef POLYLINE2KICAD_H

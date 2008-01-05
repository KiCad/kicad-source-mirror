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
//
// If a CDisplayList pointer is provided, the polyline can draw itself 

#ifndef POLYLINE2KICAD_H
#define POLYLINE2KICAD_H

#define PCBU_PER_MIL 10
#define MAX_LAYERS 32
#define NM_PER_MIL 10 // 25400


#include "pad_shapes.h"


#define LAY_SELECTION  0
#define LAY_TOP_COPPER 0

#define CDC wxDC
class wxDC;

#if 0
class dl_element;
class CDisplayList {
public:
	void Set_visible(void*, int) {};
	int Get_x(void) { return 0;};
	int Get_y(void) { return 0;};
	void StopDragging(void) {};
	void CancelHighLight(void) {};
	void StartDraggingLineVertex(...) {};
	void Add() {};
};
#endif


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

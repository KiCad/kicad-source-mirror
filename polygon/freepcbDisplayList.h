// DisplayList.h : header file for CDisplayList class
//

#ifndef FP_DISPLAY_LIST_H
#define FP_DISPLAY_LIST_H 

// graphics element types
enum 
{
	DL_NONE = 0,
	DL_LINE,		// line segment with round end-caps  
	DL_CIRC,		// filled circle
	DL_HOLLOW_CIRC,	// circle outline
	DL_DONUT,		// annulus
	DL_SQUARE,		// filled square
	DL_RECT,		// filled rectangle
	DL_RRECT,		// filled rounded rectangle
	DL_OVAL,		// filled oval
	DL_OCTAGON,		// filled octagon
	DL_HOLE,		// hole, shown as circle
	DL_HOLLOW_RECT,	// rectangle outline
	DL_RECT_X,		// rectangle outline with X
	DL_POINT,		// shape to highlight a point
	DL_ARC_CW,		// arc with clockwise curve
	DL_ARC_CCW,		// arc with counter-clockwise curve
	DL_X			// X
};

// inflection modes for DS_LINE and DS_LINE_VERTEX
enum
{
	IM_NONE = 0,
	IM_90_45,		 
	IM_45_90,
	IM_90
};


#endif	//	#ifndef FP_DISPLAY_LIST_H

#ifndef BEZIER_CURVES_H
#define BEZIER_CURVES_H

#include <vector>

#include <wx/gdicmn.h>


/**
 * Function Bezier2Poly
 * convert a Bezier curve to a polyline
 * @param c1 - First point to convert.
 * @param c2 - Second point to convert.
 * @param c3 - Third point to convert.
 * @return a std::vector<wxPoint> containing the points of the polyline
 */
std::vector<wxPoint> Bezier2Poly(wxPoint c1, wxPoint c2, wxPoint c3);

std::vector<wxPoint> Bezier2Poly(int x1, int y1, int x2, int y2, int x3, int y3);

/**
 * Function Bezier2Poly
 * convert a Bezier curve to a polyline
 * @return a std::vector<wxPoint> containing the points of the polyline
 * @param c1 - First point to convert.
 * @param c2 - Second point to convert.
 * @param c3 - Third point to convert.
 * @param c4 - Fourth point to convert.
 * @return a std::vector<wxPoint> containing the points of the polyline
 */
std::vector<wxPoint> Bezier2Poly( wxPoint c1, wxPoint c2, wxPoint c3,wxPoint c4 );

std::vector<wxPoint> Bezier2Poly(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);


#endif  // BEZIER_CURVES_H

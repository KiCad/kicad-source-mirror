#ifndef BEZIER_CURVES_H
#define BEZIER_CURVES_H

#include <vector>

std::vector<wxPoint> Bezier2Poly(wxPoint c1, wxPoint c2, wxPoint c3);
std::vector<wxPoint> Bezier2Poly(wxPoint c1, wxPoint c2, wxPoint c3,wxPoint c4);

std::vector<wxPoint> Bezier2Poly(int x1, int y1, int x2, int y2, int x3, int y3);
std::vector<wxPoint> Bezier2Poly(int x1, int y1, int x2, int y2, int x3, int y3, int x4, int y4);


#endif  // BEZIER_CURVES_H

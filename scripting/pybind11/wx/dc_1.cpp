#include <eda_rect.h> // EDA_RECT
#include <gal/color4d.h> // EDA_COLOR_T
#include <gal/color4d.h> // KIGFX::COLOR4D
#include <gr_basic.h> // DrawModeAddHighlight
#include <gr_basic.h> // DrawModeAllowHighContrast
#include <gr_basic.h> // GRArc
#include <gr_basic.h> // GRArc1
#include <gr_basic.h> // GRBezier
#include <gr_basic.h> // GRCircle
#include <gr_basic.h> // GRClosedPoly
#include <gr_basic.h> // GRFilledCircle
#include <gr_basic.h> // GRForceBlackPen
#include <gr_basic.h> // GRLine
#include <gr_basic.h> // GRLineTo
#include <gr_basic.h> // GRMoveTo
#include <gr_basic.h> // GRPoly
#include <gr_basic.h> // GRResetPenAndBrush
#include <gr_basic.h> // GRSetBrush
#include <gr_basic.h> // GRSetColorPen
#include <gr_basic.h> // GR_DRAWMODE
#include <gr_basic.h> // GetGRForceBlackPenState
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/dc.h> // wxDC
#include <wx/dc.h> // wxDCBrushChanger
#include <wx/dc.h> // wxDCClipper
#include <wx/dc.h> // wxDCFontChanger
#include <wx/dc.h> // wxDCImpl
#include <wx/dc.h> // wxDCPenChanger
#include <wx/dc.h> // wxDCTextColourChanger
#include <wx/dc.h> // wxDrawObject
#include <wx/dc.h> // wxFloodFillStyle
#include <wx/dc.h> // wxFontMetrics
#include <wx/dc.h> // wxMappingMode
#include <wx/dc.h> // wxRasterOperationMode
#include <wx/pen.h> // wxPenCap
#include <wx/pen.h> // wxPenJoin
#include <wx/pen.h> // wxPenStyle

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_wx_dc_1(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxDCTextColourChanger file:wx/dc.h line:1385
		pybind11::class_<wxDCTextColourChanger, std::shared_ptr<wxDCTextColourChanger>> cl(M(""), "wxDCTextColourChanger", "");
		cl.def( pybind11::init<class wxDC &>(), pybind11::arg("dc") );

		cl.def( pybind11::init<class wxDC &, const class wxColour &>(), pybind11::arg("dc"), pybind11::arg("col") );

		cl.def("Set", (void (wxDCTextColourChanger::*)(const class wxColour &)) &wxDCTextColourChanger::Set, "C++: wxDCTextColourChanger::Set(const class wxColour &) --> void", pybind11::arg("col"));
	}
	{ // wxDCPenChanger file:wx/dc.h line:1421
		pybind11::class_<wxDCPenChanger, std::shared_ptr<wxDCPenChanger>> cl(M(""), "wxDCPenChanger", "");
		cl.def( pybind11::init<class wxDC &, const class wxPen &>(), pybind11::arg("dc"), pybind11::arg("pen") );

	}
	{ // wxDCBrushChanger file:wx/dc.h line:1448
		pybind11::class_<wxDCBrushChanger, std::shared_ptr<wxDCBrushChanger>> cl(M(""), "wxDCBrushChanger", "");
		cl.def( pybind11::init<class wxDC &, const class wxBrush &>(), pybind11::arg("dc"), pybind11::arg("brush") );

	}
	{ // wxDCClipper file:wx/dc.h line:1475
		pybind11::class_<wxDCClipper, std::shared_ptr<wxDCClipper>> cl(M(""), "wxDCClipper", "");
		cl.def( pybind11::init<class wxDC &, const class wxRegion &>(), pybind11::arg("dc"), pybind11::arg("r") );

		cl.def( pybind11::init<class wxDC &, const class wxRect &>(), pybind11::arg("dc"), pybind11::arg("r") );

		cl.def( pybind11::init<class wxDC &, int, int, int, int>(), pybind11::arg("dc"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h") );

	}
	{ // wxDCFontChanger file:wx/dc.h line:1498
		pybind11::class_<wxDCFontChanger, std::shared_ptr<wxDCFontChanger>> cl(M(""), "wxDCFontChanger", "");
		cl.def( pybind11::init<class wxDC &>(), pybind11::arg("dc") );

		cl.def( pybind11::init<class wxDC &, const class wxFont &>(), pybind11::arg("dc"), pybind11::arg("font") );

		cl.def("Set", (void (wxDCFontChanger::*)(const class wxFont &)) &wxDCFontChanger::Set, "C++: wxDCFontChanger::Set(const class wxFont &) --> void", pybind11::arg("font"));
	}
	// GR_DRAWMODE file:gr_basic.h line:39
	pybind11::enum_<GR_DRAWMODE>(M(""), "GR_DRAWMODE", pybind11::arithmetic(), "Drawmode. Compositing mode plus a flag or two")
		.value("GR_OR", GR_OR)
		.value("GR_XOR", GR_XOR)
		.value("GR_AND", GR_AND)
		.value("GR_NXOR", GR_NXOR)
		.value("GR_INVERT", GR_INVERT)
		.value("GR_ALLOW_HIGHCONTRAST", GR_ALLOW_HIGHCONTRAST)
		.value("GR_COPY", GR_COPY)
		.value("GR_HIGHLIGHT", GR_HIGHLIGHT)
		.value("UNSPECIFIED_DRAWMODE", UNSPECIFIED_DRAWMODE)
		.export_values();

;

	// DrawModeAddHighlight(enum GR_DRAWMODE *) file:gr_basic.h line:51
	M("").def("DrawModeAddHighlight", (void (*)(enum GR_DRAWMODE *)) &DrawModeAddHighlight, "C++: DrawModeAddHighlight(enum GR_DRAWMODE *) --> void", pybind11::arg("mode"));

	// DrawModeAllowHighContrast(enum GR_DRAWMODE *) file:gr_basic.h line:56
	M("").def("DrawModeAllowHighContrast", (void (*)(enum GR_DRAWMODE *)) &DrawModeAllowHighContrast, "C++: DrawModeAllowHighContrast(enum GR_DRAWMODE *) --> void", pybind11::arg("mode"));

	// GRResetPenAndBrush(class wxDC *) file:gr_basic.h line:92
	M("").def("GRResetPenAndBrush", (void (*)(class wxDC *)) &GRResetPenAndBrush, "C++: GRResetPenAndBrush(class wxDC *) --> void", pybind11::arg("DC"));

	// GRSetColorPen(class wxDC *, class KIGFX::COLOR4D, int, enum wxPenStyle) file:gr_basic.h line:93
	M("").def("GRSetColorPen", [](class wxDC * a0, class KIGFX::COLOR4D const & a1) -> void { return GRSetColorPen(a0, a1); }, "", pybind11::arg("DC"), pybind11::arg("Color"));
	M("").def("GRSetColorPen", [](class wxDC * a0, class KIGFX::COLOR4D const & a1, int const & a2) -> void { return GRSetColorPen(a0, a1, a2); }, "", pybind11::arg("DC"), pybind11::arg("Color"), pybind11::arg("width"));
	M("").def("GRSetColorPen", (void (*)(class wxDC *, class KIGFX::COLOR4D, int, enum wxPenStyle)) &GRSetColorPen, "C++: GRSetColorPen(class wxDC *, class KIGFX::COLOR4D, int, enum wxPenStyle) --> void", pybind11::arg("DC"), pybind11::arg("Color"), pybind11::arg("width"), pybind11::arg("stype"));

	// GRSetBrush(class wxDC *, class KIGFX::COLOR4D, bool) file:gr_basic.h line:94
	M("").def("GRSetBrush", [](class wxDC * a0, class KIGFX::COLOR4D const & a1) -> void { return GRSetBrush(a0, a1); }, "", pybind11::arg("DC"), pybind11::arg("Color"));
	M("").def("GRSetBrush", (void (*)(class wxDC *, class KIGFX::COLOR4D, bool)) &GRSetBrush, "C++: GRSetBrush(class wxDC *, class KIGFX::COLOR4D, bool) --> void", pybind11::arg("DC"), pybind11::arg("Color"), pybind11::arg("fill"));

	// GRForceBlackPen(bool) file:gr_basic.h line:99
	M("").def("GRForceBlackPen", (void (*)(bool)) &GRForceBlackPen, "True to force a black pen whenever the asked color.\n\nC++: GRForceBlackPen(bool) --> void", pybind11::arg("flagforce"));

	// GetGRForceBlackPenState() file:gr_basic.h line:104
	M("").def("GetGRForceBlackPenState", (bool (*)()) &GetGRForceBlackPenState, "True if a black pen was forced or false if not forced.\n\nC++: GetGRForceBlackPenState() --> bool");

	// GRLine(class EDA_RECT *, class wxDC *, class wxPoint, class wxPoint, int, class KIGFX::COLOR4D, enum wxPenStyle) file:gr_basic.h line:106
	M("").def("GRLine", [](class EDA_RECT * a0, class wxDC * a1, class wxPoint const & a2, class wxPoint const & a3, int const & a4, class KIGFX::COLOR4D const & a5) -> void { return GRLine(a0, a1, a2, a3, a4, a5); }, "", pybind11::arg("aClipBox"), pybind11::arg("aDC"), pybind11::arg("aStart"), pybind11::arg("aEnd"), pybind11::arg("aWidth"), pybind11::arg("aColor"));
	M("").def("GRLine", (void (*)(class EDA_RECT *, class wxDC *, class wxPoint, class wxPoint, int, class KIGFX::COLOR4D, enum wxPenStyle)) &GRLine, "C++: GRLine(class EDA_RECT *, class wxDC *, class wxPoint, class wxPoint, int, class KIGFX::COLOR4D, enum wxPenStyle) --> void", pybind11::arg("aClipBox"), pybind11::arg("aDC"), pybind11::arg("aStart"), pybind11::arg("aEnd"), pybind11::arg("aWidth"), pybind11::arg("aColor"), pybind11::arg("aStyle"));

	// GRLine(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D, enum wxPenStyle) file:gr_basic.h line:108
	M("").def("GRLine", [](class EDA_RECT * a0, class wxDC * a1, int const & a2, int const & a3, int const & a4, int const & a5, int const & a6, class KIGFX::COLOR4D const & a7) -> void { return GRLine(a0, a1, a2, a3, a4, a5, a6, a7); }, "", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"), pybind11::arg("width"), pybind11::arg("Color"));
	M("").def("GRLine", (void (*)(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D, enum wxPenStyle)) &GRLine, "C++: GRLine(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D, enum wxPenStyle) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"), pybind11::arg("width"), pybind11::arg("Color"), pybind11::arg("aStyle"));

	// GRMoveTo(int, int) file:gr_basic.h line:110
	M("").def("GRMoveTo", (void (*)(int, int)) &GRMoveTo, "C++: GRMoveTo(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));

	// GRLineTo(class EDA_RECT *, class wxDC *, int, int, int, class KIGFX::COLOR4D) file:gr_basic.h line:111
	M("").def("GRLineTo", (void (*)(class EDA_RECT *, class wxDC *, int, int, int, class KIGFX::COLOR4D)) &GRLineTo, "C++: GRLineTo(class EDA_RECT *, class wxDC *, int, int, int, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("width"), pybind11::arg("Color"));

	// GRPoly(class EDA_RECT *, class wxDC *, int, const class wxPoint *, bool, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D) file:gr_basic.h line:113
	M("").def("GRPoly", (void (*)(class EDA_RECT *, class wxDC *, int, const class wxPoint *, bool, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D)) &GRPoly, "C++: GRPoly(class EDA_RECT *, class wxDC *, int, const class wxPoint *, bool, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("n"), pybind11::arg("Points"), pybind11::arg("Fill"), pybind11::arg("width"), pybind11::arg("Color"), pybind11::arg("BgColor"));

	// GRBezier(class EDA_RECT *, class wxDC *, int &, int, class KIGFX::COLOR4D) file:gr_basic.h line:119
	M("").def("GRBezier", (void (*)(class EDA_RECT *, class wxDC *, int &, int, class KIGFX::COLOR4D)) &GRBezier, "Draw cubic (4 points: start control1, control2, end) bezier curve.\n\nC++: GRBezier(class EDA_RECT *, class wxDC *, int &, int, class KIGFX::COLOR4D) --> void", pybind11::arg("aClipBox"), pybind11::arg("aDC"), pybind11::arg("aPoints"), pybind11::arg("aWidth"), pybind11::arg("aColor"));

	// GRClosedPoly(class EDA_RECT *, class wxDC *, int, const class wxPoint *, bool, class KIGFX::COLOR4D, class KIGFX::COLOR4D) file:gr_basic.h line:134
	M("").def("GRClosedPoly", (void (*)(class EDA_RECT *, class wxDC *, int, const class wxPoint *, bool, class KIGFX::COLOR4D, class KIGFX::COLOR4D)) &GRClosedPoly, "Draw a closed polygon onto the drawing context  and optionally fills and/or draws\n a border around it.\n\n \n defines a rectangular boundary outside of which no drawing will occur.\n \n\n the device context into which drawing should occur.\n \n\n the number of points in the array \n \n\n The points to draw.\n \n\n true if polygon is to be filled, else false and only the boundary is drawn.\n \n\n the color of the border.\n \n\n the fill color of the polygon's interior.\n\nC++: GRClosedPoly(class EDA_RECT *, class wxDC *, int, const class wxPoint *, bool, class KIGFX::COLOR4D, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("aDC"), pybind11::arg("aPointCount"), pybind11::arg("aPoints"), pybind11::arg("doFill"), pybind11::arg("aPenColor"), pybind11::arg("aFillColor"));

	// GRClosedPoly(class EDA_RECT *, class wxDC *, int, const class wxPoint *, bool, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D) file:gr_basic.h line:153
	M("").def("GRClosedPoly", (void (*)(class EDA_RECT *, class wxDC *, int, const class wxPoint *, bool, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D)) &GRClosedPoly, "Draw a closed polygon onto the drawing context  and optionally fills and/or draws\n a border around it.\n\n \n defines a rectangular boundary outside of which no drawing will occur.\n \n\n the device context into which drawing should occur.\n \n\n the number of points in the array \n \n\n the points to draw.\n \n\n true if polygon is to be filled, else false and only the boundary is drawn.\n \n\n is the width of the pen to use on the perimeter, can be zero.\n \n\n the color of the border.\n \n\n the fill color of the polygon's interior.\n\nC++: GRClosedPoly(class EDA_RECT *, class wxDC *, int, const class wxPoint *, bool, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("aDC"), pybind11::arg("aPointCount"), pybind11::arg("aPoints"), pybind11::arg("doFill"), pybind11::arg("aPenWidth"), pybind11::arg("aPenColor"), pybind11::arg("aFillColor"));

	// GRCircle(class EDA_RECT *, class wxDC *, int, int, int, class KIGFX::COLOR4D) file:gr_basic.h line:168
	M("").def("GRCircle", (void (*)(class EDA_RECT *, class wxDC *, int, int, int, class KIGFX::COLOR4D)) &GRCircle, "Draw a circle onto the drawing context  centered at the user coordinates (x,y).\n\n \n defines a rectangular boundary outside of which no drawing will occur.\n \n\n the device context into which drawing should occur.\n \n\n The x coordinate in user space of the center of the circle.\n \n\n The y coordinate in user space of the center of the circle.\n \n\n is the radius of the circle.\n \n\n is the color to draw.\n \n\n COLOR4D\n\nC++: GRCircle(class EDA_RECT *, class wxDC *, int, int, int, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("aDC"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("aRadius"), pybind11::arg("aColor"));

	// GRCircle(class EDA_RECT *, class wxDC *, int, int, int, int, class KIGFX::COLOR4D) file:gr_basic.h line:169
	M("").def("GRCircle", (void (*)(class EDA_RECT *, class wxDC *, int, int, int, int, class KIGFX::COLOR4D)) &GRCircle, "C++: GRCircle(class EDA_RECT *, class wxDC *, int, int, int, int, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("r"), pybind11::arg("width"), pybind11::arg("Color"));

	// GRFilledCircle(class EDA_RECT *, class wxDC *, int, int, int, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D) file:gr_basic.h line:170
	M("").def("GRFilledCircle", (void (*)(class EDA_RECT *, class wxDC *, int, int, int, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D)) &GRFilledCircle, "C++: GRFilledCircle(class EDA_RECT *, class wxDC *, int, int, int, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("r"), pybind11::arg("width"), pybind11::arg("Color"), pybind11::arg("BgColor"));

	// GRFilledCircle(class EDA_RECT *, class wxDC *, class wxPoint, int, class KIGFX::COLOR4D) file:gr_basic.h line:172
	M("").def("GRFilledCircle", (void (*)(class EDA_RECT *, class wxDC *, class wxPoint, int, class KIGFX::COLOR4D)) &GRFilledCircle, "C++: GRFilledCircle(class EDA_RECT *, class wxDC *, class wxPoint, int, class KIGFX::COLOR4D) --> void", pybind11::arg("aClipBox"), pybind11::arg("aDC"), pybind11::arg("aPos"), pybind11::arg("aRadius"), pybind11::arg("aColor"));

	// GRCircle(class EDA_RECT *, class wxDC *, class wxPoint, int, int, class KIGFX::COLOR4D) file:gr_basic.h line:173
	M("").def("GRCircle", (void (*)(class EDA_RECT *, class wxDC *, class wxPoint, int, int, class KIGFX::COLOR4D)) &GRCircle, "C++: GRCircle(class EDA_RECT *, class wxDC *, class wxPoint, int, int, class KIGFX::COLOR4D) --> void", pybind11::arg("aClipBox"), pybind11::arg("aDC"), pybind11::arg("aPos"), pybind11::arg("aRadius"), pybind11::arg("aWidth"), pybind11::arg("aColor"));

	// GRArc(class EDA_RECT *, class wxDC *, int, int, double, double, int, class KIGFX::COLOR4D) file:gr_basic.h line:176
	M("").def("GRArc", (void (*)(class EDA_RECT *, class wxDC *, int, int, double, double, int, class KIGFX::COLOR4D)) &GRArc, "C++: GRArc(class EDA_RECT *, class wxDC *, int, int, double, double, int, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("StAngle"), pybind11::arg("EndAngle"), pybind11::arg("r"), pybind11::arg("Color"));

	// GRArc(class EDA_RECT *, class wxDC *, int, int, double, double, int, int, class KIGFX::COLOR4D) file:gr_basic.h line:178
	M("").def("GRArc", (void (*)(class EDA_RECT *, class wxDC *, int, int, double, double, int, int, class KIGFX::COLOR4D)) &GRArc, "C++: GRArc(class EDA_RECT *, class wxDC *, int, int, double, double, int, int, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("StAngle"), pybind11::arg("EndAngle"), pybind11::arg("r"), pybind11::arg("width"), pybind11::arg("Color"));

	// GRArc1(class EDA_RECT *, class wxDC *, int, int, int, int, int, int, class KIGFX::COLOR4D) file:gr_basic.h line:180
	M("").def("GRArc1", (void (*)(class EDA_RECT *, class wxDC *, int, int, int, int, int, int, class KIGFX::COLOR4D)) &GRArc1, "C++: GRArc1(class EDA_RECT *, class wxDC *, int, int, int, int, int, int, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"), pybind11::arg("xc"), pybind11::arg("yc"), pybind11::arg("Color"));

	// GRArc1(class EDA_RECT *, class wxDC *, int, int, int, int, int, int, int, class KIGFX::COLOR4D) file:gr_basic.h line:182
	M("").def("GRArc1", (void (*)(class EDA_RECT *, class wxDC *, int, int, int, int, int, int, int, class KIGFX::COLOR4D)) &GRArc1, "C++: GRArc1(class EDA_RECT *, class wxDC *, int, int, int, int, int, int, int, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"), pybind11::arg("xc"), pybind11::arg("yc"), pybind11::arg("width"), pybind11::arg("Color"));

	// GRArc1(class EDA_RECT *, class wxDC *, class wxPoint, class wxPoint, class wxPoint, int, class KIGFX::COLOR4D) file:gr_basic.h line:184
	M("").def("GRArc1", (void (*)(class EDA_RECT *, class wxDC *, class wxPoint, class wxPoint, class wxPoint, int, class KIGFX::COLOR4D)) &GRArc1, "C++: GRArc1(class EDA_RECT *, class wxDC *, class wxPoint, class wxPoint, class wxPoint, int, class KIGFX::COLOR4D) --> void", pybind11::arg("aClipBox"), pybind11::arg("aDC"), pybind11::arg("aStart"), pybind11::arg("aEnd"), pybind11::arg("aCenter"), pybind11::arg("aWidth"), pybind11::arg("aColor"));

}

#include <eda_rect.h> // EDA_RECT
#include <gal/color4d.h> // EDA_COLOR_T
#include <gal/color4d.h> // KIGFX::COLOR4D
#include <gr_basic.h> // GRCSegm
#include <gr_basic.h> // GRDrawAnchor
#include <gr_basic.h> // GRDrawWrappedText
#include <gr_basic.h> // GRFillCSegm
#include <gr_basic.h> // GRFilledArc
#include <gr_basic.h> // GRFilledRect
#include <gr_basic.h> // GRFilledSegment
#include <gr_basic.h> // GRGetColor
#include <gr_basic.h> // GRLineArray
#include <gr_basic.h> // GRPutPixel
#include <gr_basic.h> // GRRect
#include <gr_basic.h> // GRRectPs
#include <gr_basic.h> // GRSFilledRect
#include <gr_basic.h> // GRSetColor
#include <gr_basic.h> // GRSetDefaultPalette
#include <iterator> // __gnu_cxx::__normal_iterator
#include <layers_id_colors_and_visibility.h> // GAL_LAYER_ID
#include <layers_id_colors_and_visibility.h> // NETNAMES_LAYER_ID
#include <layers_id_colors_and_visibility.h> // PCB_LAYER_ID
#include <memory> // std::allocator
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/dc.h> // wxDC
#include <wx/dc.h> // wxDCImpl
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

void bind_gr_basic(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// GRFilledArc(class EDA_RECT *, class wxDC *, int, int, double, double, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D) file:gr_basic.h line:186
	M("").def("GRFilledArc", (void (*)(class EDA_RECT *, class wxDC *, int, int, double, double, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D)) &GRFilledArc, "C++: GRFilledArc(class EDA_RECT *, class wxDC *, int, int, double, double, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("StAngle"), pybind11::arg("EndAngle"), pybind11::arg("r"), pybind11::arg("Color"), pybind11::arg("BgColor"));

	// GRFilledArc(class EDA_RECT *, class wxDC *, int, int, double, double, int, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D) file:gr_basic.h line:188
	M("").def("GRFilledArc", (void (*)(class EDA_RECT *, class wxDC *, int, int, double, double, int, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D)) &GRFilledArc, "C++: GRFilledArc(class EDA_RECT *, class wxDC *, int, int, double, double, int, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("StAngle"), pybind11::arg("EndAngle"), pybind11::arg("r"), pybind11::arg("width"), pybind11::arg("Color"), pybind11::arg("BgColor"));

	// GRCSegm(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D) file:gr_basic.h line:190
	M("").def("GRCSegm", (void (*)(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D)) &GRCSegm, "C++: GRCSegm(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"), pybind11::arg("width"), pybind11::arg("Color"));

	// GRFillCSegm(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D) file:gr_basic.h line:193
	M("").def("GRFillCSegm", (void (*)(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D)) &GRFillCSegm, "C++: GRFillCSegm(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"), pybind11::arg("width"), pybind11::arg("Color"));

	// GRFilledSegment(class EDA_RECT *, class wxDC *, class wxPoint, class wxPoint, int, class KIGFX::COLOR4D) file:gr_basic.h line:195
	M("").def("GRFilledSegment", (void (*)(class EDA_RECT *, class wxDC *, class wxPoint, class wxPoint, int, class KIGFX::COLOR4D)) &GRFilledSegment, "C++: GRFilledSegment(class EDA_RECT *, class wxDC *, class wxPoint, class wxPoint, int, class KIGFX::COLOR4D) --> void", pybind11::arg("aClipBox"), pybind11::arg("aDC"), pybind11::arg("aStart"), pybind11::arg("aEnd"), pybind11::arg("aWidth"), pybind11::arg("aColor"));

	// GRCSegm(class EDA_RECT *, class wxDC *, int, int, int, int, int, int, class KIGFX::COLOR4D) file:gr_basic.h line:198
	M("").def("GRCSegm", (void (*)(class EDA_RECT *, class wxDC *, int, int, int, int, int, int, class KIGFX::COLOR4D)) &GRCSegm, "C++: GRCSegm(class EDA_RECT *, class wxDC *, int, int, int, int, int, int, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"), pybind11::arg("width"), pybind11::arg("aPenSize"), pybind11::arg("Color"));

	// GRCSegm(class EDA_RECT *, class wxDC *, class wxPoint, class wxPoint, int, class KIGFX::COLOR4D) file:gr_basic.h line:200
	M("").def("GRCSegm", (void (*)(class EDA_RECT *, class wxDC *, class wxPoint, class wxPoint, int, class KIGFX::COLOR4D)) &GRCSegm, "C++: GRCSegm(class EDA_RECT *, class wxDC *, class wxPoint, class wxPoint, int, class KIGFX::COLOR4D) --> void", pybind11::arg("aClipBox"), pybind11::arg("aDC"), pybind11::arg("aStart"), pybind11::arg("aEnd"), pybind11::arg("aWidth"), pybind11::arg("aColor"));

	// GRSetColor(class KIGFX::COLOR4D) file:gr_basic.h line:203
	M("").def("GRSetColor", (void (*)(class KIGFX::COLOR4D)) &GRSetColor, "C++: GRSetColor(class KIGFX::COLOR4D) --> void", pybind11::arg("Color"));

	// GRSetDefaultPalette() file:gr_basic.h line:204
	M("").def("GRSetDefaultPalette", (void (*)()) &GRSetDefaultPalette, "C++: GRSetDefaultPalette() --> void");

	// GRGetColor() file:gr_basic.h line:205
	M("").def("GRGetColor", (class KIGFX::COLOR4D (*)()) &GRGetColor, "C++: GRGetColor() --> class KIGFX::COLOR4D");

	// GRPutPixel(class EDA_RECT *, class wxDC *, int, int, class KIGFX::COLOR4D) file:gr_basic.h line:206
	M("").def("GRPutPixel", (void (*)(class EDA_RECT *, class wxDC *, int, int, class KIGFX::COLOR4D)) &GRPutPixel, "C++: GRPutPixel(class EDA_RECT *, class wxDC *, int, int, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("color"));

	// GRFilledRect(class EDA_RECT *, class wxDC *, int, int, int, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D) file:gr_basic.h line:207
	M("").def("GRFilledRect", (void (*)(class EDA_RECT *, class wxDC *, int, int, int, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D)) &GRFilledRect, "C++: GRFilledRect(class EDA_RECT *, class wxDC *, int, int, int, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"), pybind11::arg("Color"), pybind11::arg("BgColor"));

	// GRFilledRect(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D) file:gr_basic.h line:209
	M("").def("GRFilledRect", (void (*)(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D)) &GRFilledRect, "C++: GRFilledRect(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"), pybind11::arg("width"), pybind11::arg("Color"), pybind11::arg("BgColor"));

	// GRRect(class EDA_RECT *, class wxDC *, int, int, int, int, class KIGFX::COLOR4D) file:gr_basic.h line:211
	M("").def("GRRect", (void (*)(class EDA_RECT *, class wxDC *, int, int, int, int, class KIGFX::COLOR4D)) &GRRect, "C++: GRRect(class EDA_RECT *, class wxDC *, int, int, int, int, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"), pybind11::arg("Color"));

	// GRRect(class EDA_RECT *, class wxDC *, const class EDA_RECT &, int, class KIGFX::COLOR4D) file:gr_basic.h line:212
	M("").def("GRRect", (void (*)(class EDA_RECT *, class wxDC *, const class EDA_RECT &, int, class KIGFX::COLOR4D)) &GRRect, "C++: GRRect(class EDA_RECT *, class wxDC *, const class EDA_RECT &, int, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("aRect"), pybind11::arg("aWidth"), pybind11::arg("Color"));

	// GRRect(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D) file:gr_basic.h line:213
	M("").def("GRRect", (void (*)(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D)) &GRRect, "C++: GRRect(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"), pybind11::arg("width"), pybind11::arg("Color"));

	// GRRectPs(class EDA_RECT *, class wxDC *, const class EDA_RECT &, int, class KIGFX::COLOR4D, enum wxPenStyle) file:gr_basic.h line:215
	M("").def("GRRectPs", [](class EDA_RECT * a0, class wxDC * a1, const class EDA_RECT & a2, int const & a3, class KIGFX::COLOR4D const & a4) -> void { return GRRectPs(a0, a1, a2, a3, a4); }, "", pybind11::arg("aClipBox"), pybind11::arg("aDC"), pybind11::arg("aRect"), pybind11::arg("aWidth"), pybind11::arg("aColor"));
	M("").def("GRRectPs", (void (*)(class EDA_RECT *, class wxDC *, const class EDA_RECT &, int, class KIGFX::COLOR4D, enum wxPenStyle)) &GRRectPs, "C++: GRRectPs(class EDA_RECT *, class wxDC *, const class EDA_RECT &, int, class KIGFX::COLOR4D, enum wxPenStyle) --> void", pybind11::arg("aClipBox"), pybind11::arg("aDC"), pybind11::arg("aRect"), pybind11::arg("aWidth"), pybind11::arg("aColor"), pybind11::arg("aStyle"));

	// GRSFilledRect(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D) file:gr_basic.h line:218
	M("").def("GRSFilledRect", (void (*)(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D)) &GRSFilledRect, "C++: GRSFilledRect(class EDA_RECT *, class wxDC *, int, int, int, int, int, class KIGFX::COLOR4D, class KIGFX::COLOR4D) --> void", pybind11::arg("ClipBox"), pybind11::arg("DC"), pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"), pybind11::arg("width"), pybind11::arg("Color"), pybind11::arg("BgColor"));

	// GRLineArray(class EDA_RECT *, class wxDC *, int &, int, class KIGFX::COLOR4D) file:gr_basic.h line:231
	M("").def("GRLineArray", (void (*)(class EDA_RECT *, class wxDC *, int &, int, class KIGFX::COLOR4D)) &GRLineArray, "Draw an array of lines (not a polygon).\n\n \n the clip box.\n \n\n the device context into which drawing should occur.\n \n\n a list of pair of coordinate in user space: a pair for each line.\n \n\n the width of each line.\n \n\n the color of the lines.\n \n\n COLOR4D\n\nC++: GRLineArray(class EDA_RECT *, class wxDC *, int &, int, class KIGFX::COLOR4D) --> void", pybind11::arg("aClipBox"), pybind11::arg("aDC"), pybind11::arg("aLines"), pybind11::arg("aWidth"), pybind11::arg("aColor"));

	// GRDrawAnchor(class EDA_RECT *, class wxDC *, int, int, int, class KIGFX::COLOR4D) file:gr_basic.h line:234
	M("").def("GRDrawAnchor", (void (*)(class EDA_RECT *, class wxDC *, int, int, int, class KIGFX::COLOR4D)) &GRDrawAnchor, "C++: GRDrawAnchor(class EDA_RECT *, class wxDC *, int, int, int, class KIGFX::COLOR4D) --> void", pybind11::arg("aClipBox"), pybind11::arg("aDC"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("aSize"), pybind11::arg("aColor"));

	// GRDrawWrappedText(class wxDC &, const class wxString &) file:gr_basic.h line:242
	M("").def("GRDrawWrappedText", (void (*)(class wxDC &, const class wxString &)) &GRDrawWrappedText, "Draw text centered on a wxDC with wrapping.\n\n \n wxDC instance onto which the text will be drawn.\n \n\n the text to draw.\n\nC++: GRDrawWrappedText(class wxDC &, const class wxString &) --> void", pybind11::arg("aDC"), pybind11::arg("aText"));

	// PCB_LAYER_ID file:layers_id_colors_and_visibility.h line:69
	pybind11::enum_<PCB_LAYER_ID>(M(""), "PCB_LAYER_ID", pybind11::arithmetic(), "This is the definition of all layers used in Pcbnew.\n\n The PCB layer types are fixed at value 0 through LAYER_ID_COUNT to ensure compatibility\n with legacy board files.")
		.value("UNDEFINED_LAYER", UNDEFINED_LAYER)
		.value("UNSELECTED_LAYER", UNSELECTED_LAYER)
		.value("PCBNEW_LAYER_ID_START", PCBNEW_LAYER_ID_START)
		.value("F_Cu", F_Cu)
		.value("In1_Cu", In1_Cu)
		.value("In2_Cu", In2_Cu)
		.value("In3_Cu", In3_Cu)
		.value("In4_Cu", In4_Cu)
		.value("In5_Cu", In5_Cu)
		.value("In6_Cu", In6_Cu)
		.value("In7_Cu", In7_Cu)
		.value("In8_Cu", In8_Cu)
		.value("In9_Cu", In9_Cu)
		.value("In10_Cu", In10_Cu)
		.value("In11_Cu", In11_Cu)
		.value("In12_Cu", In12_Cu)
		.value("In13_Cu", In13_Cu)
		.value("In14_Cu", In14_Cu)
		.value("In15_Cu", In15_Cu)
		.value("In16_Cu", In16_Cu)
		.value("In17_Cu", In17_Cu)
		.value("In18_Cu", In18_Cu)
		.value("In19_Cu", In19_Cu)
		.value("In20_Cu", In20_Cu)
		.value("In21_Cu", In21_Cu)
		.value("In22_Cu", In22_Cu)
		.value("In23_Cu", In23_Cu)
		.value("In24_Cu", In24_Cu)
		.value("In25_Cu", In25_Cu)
		.value("In26_Cu", In26_Cu)
		.value("In27_Cu", In27_Cu)
		.value("In28_Cu", In28_Cu)
		.value("In29_Cu", In29_Cu)
		.value("In30_Cu", In30_Cu)
		.value("B_Cu", B_Cu)
		.value("B_Adhes", B_Adhes)
		.value("F_Adhes", F_Adhes)
		.value("B_Paste", B_Paste)
		.value("F_Paste", F_Paste)
		.value("B_SilkS", B_SilkS)
		.value("F_SilkS", F_SilkS)
		.value("B_Mask", B_Mask)
		.value("F_Mask", F_Mask)
		.value("Dwgs_User", Dwgs_User)
		.value("Cmts_User", Cmts_User)
		.value("Eco1_User", Eco1_User)
		.value("Eco2_User", Eco2_User)
		.value("Edge_Cuts", Edge_Cuts)
		.value("Margin", Margin)
		.value("B_CrtYd", B_CrtYd)
		.value("F_CrtYd", F_CrtYd)
		.value("B_Fab", B_Fab)
		.value("F_Fab", F_Fab)
		.value("User_1", User_1)
		.value("User_2", User_2)
		.value("User_3", User_3)
		.value("User_4", User_4)
		.value("User_5", User_5)
		.value("User_6", User_6)
		.value("User_7", User_7)
		.value("User_8", User_8)
		.value("User_9", User_9)
		.value("Rescue", Rescue)
		.value("PCB_LAYER_ID_COUNT", PCB_LAYER_ID_COUNT)
		.export_values();

;

	// NETNAMES_LAYER_ID file:layers_id_colors_and_visibility.h line:154
	pybind11::enum_<NETNAMES_LAYER_ID>(M(""), "NETNAMES_LAYER_ID", pybind11::arithmetic(), "Dedicated layers for net names used in Pcbnew")
		.value("NETNAMES_LAYER_ID_START", NETNAMES_LAYER_ID_START)
		.value("NETNAMES_LAYER_ID_RESERVED", NETNAMES_LAYER_ID_RESERVED)
		.value("LAYER_PAD_FR_NETNAMES", LAYER_PAD_FR_NETNAMES)
		.value("LAYER_PAD_BK_NETNAMES", LAYER_PAD_BK_NETNAMES)
		.value("LAYER_PAD_NETNAMES", LAYER_PAD_NETNAMES)
		.value("LAYER_VIA_NETNAMES", LAYER_VIA_NETNAMES)
		.value("NETNAMES_LAYER_ID_END", NETNAMES_LAYER_ID_END)
		.export_values();

;

	// GAL_LAYER_ID file:layers_id_colors_and_visibility.h line:189
	pybind11::enum_<GAL_LAYER_ID>(M(""), "GAL_LAYER_ID", pybind11::arithmetic(), "GAL layers are \"virtual\" layers, i.e. not tied into design data.\n  Some layers here are shared between applications.\n\n  NOTE: Be very careful where you add new layers here.  Layers below GAL_LAYER_ID_BITMASK_END\n  must never be re-ordered and new layers must always be added after this value, because the\n  layers before this value are mapped to bit locations in legacy board files.\n\n  The values in this enum that are used to store visibility state are explicitly encoded with an\n  offset from GAL_LAYER_ID_START, which is explicitly encoded itself. The exact value of\n  GAL_LAYER_ID_START is not that sensitive, but the offsets should never be changed or else any\n  existing visibility settings will be disrupted.")
		.value("GAL_LAYER_ID_START", GAL_LAYER_ID_START)
		.value("LAYER_VIAS", LAYER_VIAS)
		.value("LAYER_VIA_MICROVIA", LAYER_VIA_MICROVIA)
		.value("LAYER_VIA_BBLIND", LAYER_VIA_BBLIND)
		.value("LAYER_VIA_THROUGH", LAYER_VIA_THROUGH)
		.value("LAYER_NON_PLATEDHOLES", LAYER_NON_PLATEDHOLES)
		.value("LAYER_MOD_TEXT_FR", LAYER_MOD_TEXT_FR)
		.value("LAYER_MOD_TEXT_BK", LAYER_MOD_TEXT_BK)
		.value("LAYER_MOD_TEXT_INVISIBLE", LAYER_MOD_TEXT_INVISIBLE)
		.value("LAYER_ANCHOR", LAYER_ANCHOR)
		.value("LAYER_PAD_FR", LAYER_PAD_FR)
		.value("LAYER_PAD_BK", LAYER_PAD_BK)
		.value("LAYER_RATSNEST", LAYER_RATSNEST)
		.value("LAYER_GRID", LAYER_GRID)
		.value("LAYER_GRID_AXES", LAYER_GRID_AXES)
		.value("LAYER_NO_CONNECTS", LAYER_NO_CONNECTS)
		.value("LAYER_MOD_FR", LAYER_MOD_FR)
		.value("LAYER_MOD_BK", LAYER_MOD_BK)
		.value("LAYER_MOD_VALUES", LAYER_MOD_VALUES)
		.value("LAYER_MOD_REFERENCES", LAYER_MOD_REFERENCES)
		.value("LAYER_TRACKS", LAYER_TRACKS)
		.value("LAYER_PADS_TH", LAYER_PADS_TH)
		.value("LAYER_PAD_PLATEDHOLES", LAYER_PAD_PLATEDHOLES)
		.value("LAYER_VIA_HOLES", LAYER_VIA_HOLES)
		.value("LAYER_DRC_ERROR", LAYER_DRC_ERROR)
		.value("LAYER_DRAWINGSHEET", LAYER_DRAWINGSHEET)
		.value("LAYER_GP_OVERLAY", LAYER_GP_OVERLAY)
		.value("LAYER_SELECT_OVERLAY", LAYER_SELECT_OVERLAY)
		.value("LAYER_PCB_BACKGROUND", LAYER_PCB_BACKGROUND)
		.value("LAYER_CURSOR", LAYER_CURSOR)
		.value("LAYER_AUX_ITEMS", LAYER_AUX_ITEMS)
		.value("LAYER_DRAW_BITMAPS", LAYER_DRAW_BITMAPS)
		.value("GAL_LAYER_ID_BITMASK_END", GAL_LAYER_ID_BITMASK_END)
		.value("LAYER_PADS", LAYER_PADS)
		.value("LAYER_ZONES", LAYER_ZONES)
		.value("LAYER_PAD_HOLEWALLS", LAYER_PAD_HOLEWALLS)
		.value("LAYER_VIA_HOLEWALLS", LAYER_VIA_HOLEWALLS)
		.value("LAYER_DRC_WARNING", LAYER_DRC_WARNING)
		.value("LAYER_DRC_EXCLUSION", LAYER_DRC_EXCLUSION)
		.value("LAYER_MARKER_SHADOWS", LAYER_MARKER_SHADOWS)
		.value("LAYER_DRAWINGSHEET_PAGE1", LAYER_DRAWINGSHEET_PAGE1)
		.value("LAYER_DRAWINGSHEET_PAGEn", LAYER_DRAWINGSHEET_PAGEn)
		.value("LAYER_ZONE_START", LAYER_ZONE_START)
		.value("LAYER_ZONE_END", LAYER_ZONE_END)
		.value("GAL_LAYER_ID_END", GAL_LAYER_ID_END)
		.export_values();

;

}

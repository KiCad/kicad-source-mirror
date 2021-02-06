#include <memory> // std::allocator
#include <sstream> // __str__
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

// wxDC file:wx/dc.h line:783
struct PyCallBack_wxDC : public wxDC {
	using wxDC::wxDC;

	int GetResolution() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDC *>(this), "GetResolution");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxDC::GetResolution();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDC *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxDC::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDC *>(this), "CreateRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxObject::CreateRefData();
	}
	class wxRefCounter * CloneRefData(const class wxRefCounter * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDC *>(this), "CloneRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxObject::CloneRefData(a0);
	}
};

void bind_wx_dc(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxDC file:wx/dc.h line:783
		pybind11::class_<wxDC, std::shared_ptr<wxDC>, PyCallBack_wxDC, wxObject> cl(M(""), "wxDC", "");
		cl.def("CopyAttributes", (void (wxDC::*)(const class wxDC &)) &wxDC::CopyAttributes, "C++: wxDC::CopyAttributes(const class wxDC &) --> void", pybind11::arg("dc"));
		cl.def("GetImpl", (class wxDCImpl * (wxDC::*)()) &wxDC::GetImpl, "C++: wxDC::GetImpl() --> class wxDCImpl *", pybind11::return_value_policy::automatic);
		cl.def("GetWindow", (class wxWindow * (wxDC::*)() const) &wxDC::GetWindow, "C++: wxDC::GetWindow() const --> class wxWindow *", pybind11::return_value_policy::automatic);
		cl.def("GetHandle", (void * (wxDC::*)() const) &wxDC::GetHandle, "C++: wxDC::GetHandle() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("IsOk", (bool (wxDC::*)() const) &wxDC::IsOk, "C++: wxDC::IsOk() const --> bool");
		cl.def("CanDrawBitmap", (bool (wxDC::*)() const) &wxDC::CanDrawBitmap, "C++: wxDC::CanDrawBitmap() const --> bool");
		cl.def("CanGetTextExtent", (bool (wxDC::*)() const) &wxDC::CanGetTextExtent, "C++: wxDC::CanGetTextExtent() const --> bool");
		cl.def("GetSize", (void (wxDC::*)(int *, int *) const) &wxDC::GetSize, "C++: wxDC::GetSize(int *, int *) const --> void", pybind11::arg("width"), pybind11::arg("height"));
		cl.def("GetSize", (class wxSize (wxDC::*)() const) &wxDC::GetSize, "C++: wxDC::GetSize() const --> class wxSize");
		cl.def("GetSizeMM", (void (wxDC::*)(int *, int *) const) &wxDC::GetSizeMM, "C++: wxDC::GetSizeMM(int *, int *) const --> void", pybind11::arg("width"), pybind11::arg("height"));
		cl.def("GetSizeMM", (class wxSize (wxDC::*)() const) &wxDC::GetSizeMM, "C++: wxDC::GetSizeMM() const --> class wxSize");
		cl.def("GetDepth", (int (wxDC::*)() const) &wxDC::GetDepth, "C++: wxDC::GetDepth() const --> int");
		cl.def("GetPPI", (class wxSize (wxDC::*)() const) &wxDC::GetPPI, "C++: wxDC::GetPPI() const --> class wxSize");
		cl.def("GetResolution", (int (wxDC::*)() const) &wxDC::GetResolution, "C++: wxDC::GetResolution() const --> int");
		cl.def("GetContentScaleFactor", (double (wxDC::*)() const) &wxDC::GetContentScaleFactor, "C++: wxDC::GetContentScaleFactor() const --> double");
		cl.def("SetLayoutDirection", (void (wxDC::*)(enum wxLayoutDirection)) &wxDC::SetLayoutDirection, "C++: wxDC::SetLayoutDirection(enum wxLayoutDirection) --> void", pybind11::arg("dir"));
		cl.def("GetLayoutDirection", (enum wxLayoutDirection (wxDC::*)() const) &wxDC::GetLayoutDirection, "C++: wxDC::GetLayoutDirection() const --> enum wxLayoutDirection");
		cl.def("StartDoc", (bool (wxDC::*)(const class wxString &)) &wxDC::StartDoc, "C++: wxDC::StartDoc(const class wxString &) --> bool", pybind11::arg("message"));
		cl.def("EndDoc", (void (wxDC::*)()) &wxDC::EndDoc, "C++: wxDC::EndDoc() --> void");
		cl.def("StartPage", (void (wxDC::*)()) &wxDC::StartPage, "C++: wxDC::StartPage() --> void");
		cl.def("EndPage", (void (wxDC::*)()) &wxDC::EndPage, "C++: wxDC::EndPage() --> void");
		cl.def("CalcBoundingBox", (void (wxDC::*)(int, int)) &wxDC::CalcBoundingBox, "C++: wxDC::CalcBoundingBox(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("ResetBoundingBox", (void (wxDC::*)()) &wxDC::ResetBoundingBox, "C++: wxDC::ResetBoundingBox() --> void");
		cl.def("MinX", (int (wxDC::*)() const) &wxDC::MinX, "C++: wxDC::MinX() const --> int");
		cl.def("MaxX", (int (wxDC::*)() const) &wxDC::MaxX, "C++: wxDC::MaxX() const --> int");
		cl.def("MinY", (int (wxDC::*)() const) &wxDC::MinY, "C++: wxDC::MinY() const --> int");
		cl.def("MaxY", (int (wxDC::*)() const) &wxDC::MaxY, "C++: wxDC::MaxY() const --> int");
		cl.def("SetFont", (void (wxDC::*)(const class wxFont &)) &wxDC::SetFont, "C++: wxDC::SetFont(const class wxFont &) --> void", pybind11::arg("font"));
		cl.def("GetFont", (const class wxFont & (wxDC::*)() const) &wxDC::GetFont, "C++: wxDC::GetFont() const --> const class wxFont &", pybind11::return_value_policy::automatic);
		cl.def("SetPen", (void (wxDC::*)(const class wxPen &)) &wxDC::SetPen, "C++: wxDC::SetPen(const class wxPen &) --> void", pybind11::arg("pen"));
		cl.def("GetPen", (const class wxPen & (wxDC::*)() const) &wxDC::GetPen, "C++: wxDC::GetPen() const --> const class wxPen &", pybind11::return_value_policy::automatic);
		cl.def("SetBrush", (void (wxDC::*)(const class wxBrush &)) &wxDC::SetBrush, "C++: wxDC::SetBrush(const class wxBrush &) --> void", pybind11::arg("brush"));
		cl.def("GetBrush", (const class wxBrush & (wxDC::*)() const) &wxDC::GetBrush, "C++: wxDC::GetBrush() const --> const class wxBrush &", pybind11::return_value_policy::automatic);
		cl.def("SetBackground", (void (wxDC::*)(const class wxBrush &)) &wxDC::SetBackground, "C++: wxDC::SetBackground(const class wxBrush &) --> void", pybind11::arg("brush"));
		cl.def("GetBackground", (const class wxBrush & (wxDC::*)() const) &wxDC::GetBackground, "C++: wxDC::GetBackground() const --> const class wxBrush &", pybind11::return_value_policy::automatic);
		cl.def("SetBackgroundMode", (void (wxDC::*)(int)) &wxDC::SetBackgroundMode, "C++: wxDC::SetBackgroundMode(int) --> void", pybind11::arg("mode"));
		cl.def("GetBackgroundMode", (int (wxDC::*)() const) &wxDC::GetBackgroundMode, "C++: wxDC::GetBackgroundMode() const --> int");
		cl.def("SetTextForeground", (void (wxDC::*)(const class wxColour &)) &wxDC::SetTextForeground, "C++: wxDC::SetTextForeground(const class wxColour &) --> void", pybind11::arg("colour"));
		cl.def("GetTextForeground", (const class wxColour & (wxDC::*)() const) &wxDC::GetTextForeground, "C++: wxDC::GetTextForeground() const --> const class wxColour &", pybind11::return_value_policy::automatic);
		cl.def("SetTextBackground", (void (wxDC::*)(const class wxColour &)) &wxDC::SetTextBackground, "C++: wxDC::SetTextBackground(const class wxColour &) --> void", pybind11::arg("colour"));
		cl.def("GetTextBackground", (const class wxColour & (wxDC::*)() const) &wxDC::GetTextBackground, "C++: wxDC::GetTextBackground() const --> const class wxColour &", pybind11::return_value_policy::automatic);
		cl.def("SetLogicalFunction", (void (wxDC::*)(enum wxRasterOperationMode)) &wxDC::SetLogicalFunction, "C++: wxDC::SetLogicalFunction(enum wxRasterOperationMode) --> void", pybind11::arg("function"));
		cl.def("GetLogicalFunction", (enum wxRasterOperationMode (wxDC::*)() const) &wxDC::GetLogicalFunction, "C++: wxDC::GetLogicalFunction() const --> enum wxRasterOperationMode");
		cl.def("GetCharHeight", (int (wxDC::*)() const) &wxDC::GetCharHeight, "C++: wxDC::GetCharHeight() const --> int");
		cl.def("GetCharWidth", (int (wxDC::*)() const) &wxDC::GetCharWidth, "C++: wxDC::GetCharWidth() const --> int");
		cl.def("GetFontMetrics", (struct wxFontMetrics (wxDC::*)() const) &wxDC::GetFontMetrics, "C++: wxDC::GetFontMetrics() const --> struct wxFontMetrics");
		cl.def("GetTextExtent", [](wxDC const &o, const class wxString & a0, int * a1, int * a2) -> void { return o.GetTextExtent(a0, a1, a2); }, "", pybind11::arg("string"), pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetTextExtent", [](wxDC const &o, const class wxString & a0, int * a1, int * a2, int * a3) -> void { return o.GetTextExtent(a0, a1, a2, a3); }, "", pybind11::arg("string"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("descent"));
		cl.def("GetTextExtent", [](wxDC const &o, const class wxString & a0, int * a1, int * a2, int * a3, int * a4) -> void { return o.GetTextExtent(a0, a1, a2, a3, a4); }, "", pybind11::arg("string"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("descent"), pybind11::arg("externalLeading"));
		cl.def("GetTextExtent", (void (wxDC::*)(const class wxString &, int *, int *, int *, int *, const class wxFont *) const) &wxDC::GetTextExtent, "C++: wxDC::GetTextExtent(const class wxString &, int *, int *, int *, int *, const class wxFont *) const --> void", pybind11::arg("string"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("descent"), pybind11::arg("externalLeading"), pybind11::arg("theFont"));
		cl.def("GetTextExtent", (class wxSize (wxDC::*)(const class wxString &) const) &wxDC::GetTextExtent, "C++: wxDC::GetTextExtent(const class wxString &) const --> class wxSize", pybind11::arg("string"));
		cl.def("GetMultiLineTextExtent", [](wxDC const &o, const class wxString & a0, int * a1, int * a2) -> void { return o.GetMultiLineTextExtent(a0, a1, a2); }, "", pybind11::arg("string"), pybind11::arg("width"), pybind11::arg("height"));
		cl.def("GetMultiLineTextExtent", [](wxDC const &o, const class wxString & a0, int * a1, int * a2, int * a3) -> void { return o.GetMultiLineTextExtent(a0, a1, a2, a3); }, "", pybind11::arg("string"), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("heightLine"));
		cl.def("GetMultiLineTextExtent", (void (wxDC::*)(const class wxString &, int *, int *, int *, const class wxFont *) const) &wxDC::GetMultiLineTextExtent, "C++: wxDC::GetMultiLineTextExtent(const class wxString &, int *, int *, int *, const class wxFont *) const --> void", pybind11::arg("string"), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("heightLine"), pybind11::arg("font"));
		cl.def("GetMultiLineTextExtent", (class wxSize (wxDC::*)(const class wxString &) const) &wxDC::GetMultiLineTextExtent, "C++: wxDC::GetMultiLineTextExtent(const class wxString &) const --> class wxSize", pybind11::arg("string"));
		cl.def("GetPartialTextExtents", (bool (wxDC::*)(const class wxString &, class wxArrayInt &) const) &wxDC::GetPartialTextExtents, "C++: wxDC::GetPartialTextExtents(const class wxString &, class wxArrayInt &) const --> bool", pybind11::arg("text"), pybind11::arg("widths"));
		cl.def("Clear", (void (wxDC::*)()) &wxDC::Clear, "C++: wxDC::Clear() --> void");
		cl.def("SetClippingRegion", (void (wxDC::*)(int, int, int, int)) &wxDC::SetClippingRegion, "C++: wxDC::SetClippingRegion(int, int, int, int) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("width"), pybind11::arg("height"));
		cl.def("SetClippingRegion", (void (wxDC::*)(const class wxPoint &, const class wxSize &)) &wxDC::SetClippingRegion, "C++: wxDC::SetClippingRegion(const class wxPoint &, const class wxSize &) --> void", pybind11::arg("pt"), pybind11::arg("sz"));
		cl.def("SetClippingRegion", (void (wxDC::*)(const class wxRect &)) &wxDC::SetClippingRegion, "C++: wxDC::SetClippingRegion(const class wxRect &) --> void", pybind11::arg("rect"));
		cl.def("SetDeviceClippingRegion", (void (wxDC::*)(const class wxRegion &)) &wxDC::SetDeviceClippingRegion, "C++: wxDC::SetDeviceClippingRegion(const class wxRegion &) --> void", pybind11::arg("region"));
		cl.def("SetClippingRegion", (void (wxDC::*)(const class wxRegion &)) &wxDC::SetClippingRegion, "C++: wxDC::SetClippingRegion(const class wxRegion &) --> void", pybind11::arg("region"));
		cl.def("DestroyClippingRegion", (void (wxDC::*)()) &wxDC::DestroyClippingRegion, "C++: wxDC::DestroyClippingRegion() --> void");
		cl.def("GetClippingBox", (void (wxDC::*)(int *, int *, int *, int *) const) &wxDC::GetClippingBox, "C++: wxDC::GetClippingBox(int *, int *, int *, int *) const --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("GetClippingBox", (void (wxDC::*)(class wxRect &) const) &wxDC::GetClippingBox, "C++: wxDC::GetClippingBox(class wxRect &) const --> void", pybind11::arg("rect"));
		cl.def("DeviceToLogicalX", (int (wxDC::*)(int) const) &wxDC::DeviceToLogicalX, "C++: wxDC::DeviceToLogicalX(int) const --> int", pybind11::arg("x"));
		cl.def("DeviceToLogicalY", (int (wxDC::*)(int) const) &wxDC::DeviceToLogicalY, "C++: wxDC::DeviceToLogicalY(int) const --> int", pybind11::arg("y"));
		cl.def("DeviceToLogicalXRel", (int (wxDC::*)(int) const) &wxDC::DeviceToLogicalXRel, "C++: wxDC::DeviceToLogicalXRel(int) const --> int", pybind11::arg("x"));
		cl.def("DeviceToLogicalYRel", (int (wxDC::*)(int) const) &wxDC::DeviceToLogicalYRel, "C++: wxDC::DeviceToLogicalYRel(int) const --> int", pybind11::arg("y"));
		cl.def("LogicalToDeviceX", (int (wxDC::*)(int) const) &wxDC::LogicalToDeviceX, "C++: wxDC::LogicalToDeviceX(int) const --> int", pybind11::arg("x"));
		cl.def("LogicalToDeviceY", (int (wxDC::*)(int) const) &wxDC::LogicalToDeviceY, "C++: wxDC::LogicalToDeviceY(int) const --> int", pybind11::arg("y"));
		cl.def("LogicalToDeviceXRel", (int (wxDC::*)(int) const) &wxDC::LogicalToDeviceXRel, "C++: wxDC::LogicalToDeviceXRel(int) const --> int", pybind11::arg("x"));
		cl.def("LogicalToDeviceYRel", (int (wxDC::*)(int) const) &wxDC::LogicalToDeviceYRel, "C++: wxDC::LogicalToDeviceYRel(int) const --> int", pybind11::arg("y"));
		cl.def("SetMapMode", (void (wxDC::*)(enum wxMappingMode)) &wxDC::SetMapMode, "C++: wxDC::SetMapMode(enum wxMappingMode) --> void", pybind11::arg("mode"));
		cl.def("GetMapMode", (enum wxMappingMode (wxDC::*)() const) &wxDC::GetMapMode, "C++: wxDC::GetMapMode() const --> enum wxMappingMode");
		cl.def("SetUserScale", (void (wxDC::*)(double, double)) &wxDC::SetUserScale, "C++: wxDC::SetUserScale(double, double) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetUserScale", (void (wxDC::*)(double *, double *) const) &wxDC::GetUserScale, "C++: wxDC::GetUserScale(double *, double *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("SetLogicalScale", (void (wxDC::*)(double, double)) &wxDC::SetLogicalScale, "C++: wxDC::SetLogicalScale(double, double) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetLogicalScale", (void (wxDC::*)(double *, double *) const) &wxDC::GetLogicalScale, "C++: wxDC::GetLogicalScale(double *, double *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("SetLogicalOrigin", (void (wxDC::*)(int, int)) &wxDC::SetLogicalOrigin, "C++: wxDC::SetLogicalOrigin(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetLogicalOrigin", (void (wxDC::*)(int *, int *) const) &wxDC::GetLogicalOrigin, "C++: wxDC::GetLogicalOrigin(int *, int *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetLogicalOrigin", (class wxPoint (wxDC::*)() const) &wxDC::GetLogicalOrigin, "C++: wxDC::GetLogicalOrigin() const --> class wxPoint");
		cl.def("SetDeviceOrigin", (void (wxDC::*)(int, int)) &wxDC::SetDeviceOrigin, "C++: wxDC::SetDeviceOrigin(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetDeviceOrigin", (void (wxDC::*)(int *, int *) const) &wxDC::GetDeviceOrigin, "C++: wxDC::GetDeviceOrigin(int *, int *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetDeviceOrigin", (class wxPoint (wxDC::*)() const) &wxDC::GetDeviceOrigin, "C++: wxDC::GetDeviceOrigin() const --> class wxPoint");
		cl.def("SetAxisOrientation", (void (wxDC::*)(bool, bool)) &wxDC::SetAxisOrientation, "C++: wxDC::SetAxisOrientation(bool, bool) --> void", pybind11::arg("xLeftRight"), pybind11::arg("yBottomUp"));
		cl.def("CanUseTransformMatrix", (bool (wxDC::*)() const) &wxDC::CanUseTransformMatrix, "C++: wxDC::CanUseTransformMatrix() const --> bool");
		cl.def("SetTransformMatrix", (bool (wxDC::*)(const class wxAffineMatrix2D &)) &wxDC::SetTransformMatrix, "C++: wxDC::SetTransformMatrix(const class wxAffineMatrix2D &) --> bool", pybind11::arg("matrix"));
		cl.def("GetTransformMatrix", (class wxAffineMatrix2D (wxDC::*)() const) &wxDC::GetTransformMatrix, "C++: wxDC::GetTransformMatrix() const --> class wxAffineMatrix2D");
		cl.def("ResetTransformMatrix", (void (wxDC::*)()) &wxDC::ResetTransformMatrix, "C++: wxDC::ResetTransformMatrix() --> void");
		cl.def("SetDeviceLocalOrigin", (void (wxDC::*)(int, int)) &wxDC::SetDeviceLocalOrigin, "C++: wxDC::SetDeviceLocalOrigin(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("FloodFill", [](wxDC &o, int const & a0, int const & a1, const class wxColour & a2) -> bool { return o.FloodFill(a0, a1, a2); }, "", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("col"));
		cl.def("FloodFill", (bool (wxDC::*)(int, int, const class wxColour &, enum wxFloodFillStyle)) &wxDC::FloodFill, "C++: wxDC::FloodFill(int, int, const class wxColour &, enum wxFloodFillStyle) --> bool", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("col"), pybind11::arg("style"));
		cl.def("FloodFill", [](wxDC &o, const class wxPoint & a0, const class wxColour & a1) -> bool { return o.FloodFill(a0, a1); }, "", pybind11::arg("pt"), pybind11::arg("col"));
		cl.def("FloodFill", (bool (wxDC::*)(const class wxPoint &, const class wxColour &, enum wxFloodFillStyle)) &wxDC::FloodFill, "C++: wxDC::FloodFill(const class wxPoint &, const class wxColour &, enum wxFloodFillStyle) --> bool", pybind11::arg("pt"), pybind11::arg("col"), pybind11::arg("style"));
		cl.def("GradientFillConcentric", (void (wxDC::*)(const class wxRect &, const class wxColour &, const class wxColour &)) &wxDC::GradientFillConcentric, "C++: wxDC::GradientFillConcentric(const class wxRect &, const class wxColour &, const class wxColour &) --> void", pybind11::arg("rect"), pybind11::arg("initialColour"), pybind11::arg("destColour"));
		cl.def("GradientFillConcentric", (void (wxDC::*)(const class wxRect &, const class wxColour &, const class wxColour &, const class wxPoint &)) &wxDC::GradientFillConcentric, "C++: wxDC::GradientFillConcentric(const class wxRect &, const class wxColour &, const class wxColour &, const class wxPoint &) --> void", pybind11::arg("rect"), pybind11::arg("initialColour"), pybind11::arg("destColour"), pybind11::arg("circleCenter"));
		cl.def("GradientFillLinear", [](wxDC &o, const class wxRect & a0, const class wxColour & a1, const class wxColour & a2) -> void { return o.GradientFillLinear(a0, a1, a2); }, "", pybind11::arg("rect"), pybind11::arg("initialColour"), pybind11::arg("destColour"));
		cl.def("GradientFillLinear", (void (wxDC::*)(const class wxRect &, const class wxColour &, const class wxColour &, enum wxDirection)) &wxDC::GradientFillLinear, "C++: wxDC::GradientFillLinear(const class wxRect &, const class wxColour &, const class wxColour &, enum wxDirection) --> void", pybind11::arg("rect"), pybind11::arg("initialColour"), pybind11::arg("destColour"), pybind11::arg("nDirection"));
		cl.def("GetPixel", (bool (wxDC::*)(int, int, class wxColour *) const) &wxDC::GetPixel, "C++: wxDC::GetPixel(int, int, class wxColour *) const --> bool", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("col"));
		cl.def("GetPixel", (bool (wxDC::*)(const class wxPoint &, class wxColour *) const) &wxDC::GetPixel, "C++: wxDC::GetPixel(const class wxPoint &, class wxColour *) const --> bool", pybind11::arg("pt"), pybind11::arg("col"));
		cl.def("DrawLine", (void (wxDC::*)(int, int, int, int)) &wxDC::DrawLine, "C++: wxDC::DrawLine(int, int, int, int) --> void", pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"));
		cl.def("DrawLine", (void (wxDC::*)(const class wxPoint &, const class wxPoint &)) &wxDC::DrawLine, "C++: wxDC::DrawLine(const class wxPoint &, const class wxPoint &) --> void", pybind11::arg("pt1"), pybind11::arg("pt2"));
		cl.def("CrossHair", (void (wxDC::*)(int, int)) &wxDC::CrossHair, "C++: wxDC::CrossHair(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("CrossHair", (void (wxDC::*)(const class wxPoint &)) &wxDC::CrossHair, "C++: wxDC::CrossHair(const class wxPoint &) --> void", pybind11::arg("pt"));
		cl.def("DrawArc", (void (wxDC::*)(int, int, int, int, int, int)) &wxDC::DrawArc, "C++: wxDC::DrawArc(int, int, int, int, int, int) --> void", pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"), pybind11::arg("xc"), pybind11::arg("yc"));
		cl.def("DrawArc", (void (wxDC::*)(const class wxPoint &, const class wxPoint &, const class wxPoint &)) &wxDC::DrawArc, "C++: wxDC::DrawArc(const class wxPoint &, const class wxPoint &, const class wxPoint &) --> void", pybind11::arg("pt1"), pybind11::arg("pt2"), pybind11::arg("centre"));
		cl.def("DrawCheckMark", (void (wxDC::*)(int, int, int, int)) &wxDC::DrawCheckMark, "C++: wxDC::DrawCheckMark(int, int, int, int) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("width"), pybind11::arg("height"));
		cl.def("DrawCheckMark", (void (wxDC::*)(const class wxRect &)) &wxDC::DrawCheckMark, "C++: wxDC::DrawCheckMark(const class wxRect &) --> void", pybind11::arg("rect"));
		cl.def("DrawEllipticArc", (void (wxDC::*)(int, int, int, int, double, double)) &wxDC::DrawEllipticArc, "C++: wxDC::DrawEllipticArc(int, int, int, int, double, double) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"), pybind11::arg("sa"), pybind11::arg("ea"));
		cl.def("DrawEllipticArc", (void (wxDC::*)(const class wxPoint &, const class wxSize &, double, double)) &wxDC::DrawEllipticArc, "C++: wxDC::DrawEllipticArc(const class wxPoint &, const class wxSize &, double, double) --> void", pybind11::arg("pt"), pybind11::arg("sz"), pybind11::arg("sa"), pybind11::arg("ea"));
		cl.def("DrawPoint", (void (wxDC::*)(int, int)) &wxDC::DrawPoint, "C++: wxDC::DrawPoint(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("DrawPoint", (void (wxDC::*)(const class wxPoint &)) &wxDC::DrawPoint, "C++: wxDC::DrawPoint(const class wxPoint &) --> void", pybind11::arg("pt"));
		cl.def("DrawLines", [](wxDC &o, const class wxPointList * a0) -> void { return o.DrawLines(a0); }, "", pybind11::arg("list"));
		cl.def("DrawLines", [](wxDC &o, const class wxPointList * a0, int const & a1) -> void { return o.DrawLines(a0, a1); }, "", pybind11::arg("list"), pybind11::arg("xoffset"));
		cl.def("DrawLines", (void (wxDC::*)(const class wxPointList *, int, int)) &wxDC::DrawLines, "C++: wxDC::DrawLines(const class wxPointList *, int, int) --> void", pybind11::arg("list"), pybind11::arg("xoffset"), pybind11::arg("yoffset"));
		cl.def("DrawLines", [](wxDC &o, const class wxList * a0) -> void { return o.DrawLines(a0); }, "", pybind11::arg("list"));
		cl.def("DrawLines", [](wxDC &o, const class wxList * a0, int const & a1) -> void { return o.DrawLines(a0, a1); }, "", pybind11::arg("list"), pybind11::arg("xoffset"));
		cl.def("DrawLines", (void (wxDC::*)(const class wxList *, int, int)) &wxDC::DrawLines, "C++: wxDC::DrawLines(const class wxList *, int, int) --> void", pybind11::arg("list"), pybind11::arg("xoffset"), pybind11::arg("yoffset"));
		cl.def("DrawPolygon", [](wxDC &o, const class wxPointList * a0) -> void { return o.DrawPolygon(a0); }, "", pybind11::arg("list"));
		cl.def("DrawPolygon", [](wxDC &o, const class wxPointList * a0, int const & a1) -> void { return o.DrawPolygon(a0, a1); }, "", pybind11::arg("list"), pybind11::arg("xoffset"));
		cl.def("DrawPolygon", [](wxDC &o, const class wxPointList * a0, int const & a1, int const & a2) -> void { return o.DrawPolygon(a0, a1, a2); }, "", pybind11::arg("list"), pybind11::arg("xoffset"), pybind11::arg("yoffset"));
		cl.def("DrawPolygon", (void (wxDC::*)(const class wxPointList *, int, int, enum wxPolygonFillMode)) &wxDC::DrawPolygon, "C++: wxDC::DrawPolygon(const class wxPointList *, int, int, enum wxPolygonFillMode) --> void", pybind11::arg("list"), pybind11::arg("xoffset"), pybind11::arg("yoffset"), pybind11::arg("fillStyle"));
		cl.def("DrawPolygon", [](wxDC &o, const class wxList * a0) -> void { return o.DrawPolygon(a0); }, "", pybind11::arg("list"));
		cl.def("DrawPolygon", [](wxDC &o, const class wxList * a0, int const & a1) -> void { return o.DrawPolygon(a0, a1); }, "", pybind11::arg("list"), pybind11::arg("xoffset"));
		cl.def("DrawPolygon", [](wxDC &o, const class wxList * a0, int const & a1, int const & a2) -> void { return o.DrawPolygon(a0, a1, a2); }, "", pybind11::arg("list"), pybind11::arg("xoffset"), pybind11::arg("yoffset"));
		cl.def("DrawPolygon", (void (wxDC::*)(const class wxList *, int, int, enum wxPolygonFillMode)) &wxDC::DrawPolygon, "C++: wxDC::DrawPolygon(const class wxList *, int, int, enum wxPolygonFillMode) --> void", pybind11::arg("list"), pybind11::arg("xoffset"), pybind11::arg("yoffset"), pybind11::arg("fillStyle"));
		cl.def("DrawRectangle", (void (wxDC::*)(int, int, int, int)) &wxDC::DrawRectangle, "C++: wxDC::DrawRectangle(int, int, int, int) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("width"), pybind11::arg("height"));
		cl.def("DrawRectangle", (void (wxDC::*)(const class wxPoint &, const class wxSize &)) &wxDC::DrawRectangle, "C++: wxDC::DrawRectangle(const class wxPoint &, const class wxSize &) --> void", pybind11::arg("pt"), pybind11::arg("sz"));
		cl.def("DrawRectangle", (void (wxDC::*)(const class wxRect &)) &wxDC::DrawRectangle, "C++: wxDC::DrawRectangle(const class wxRect &) --> void", pybind11::arg("rect"));
		cl.def("DrawRoundedRectangle", (void (wxDC::*)(int, int, int, int, double)) &wxDC::DrawRoundedRectangle, "C++: wxDC::DrawRoundedRectangle(int, int, int, int, double) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("radius"));
		cl.def("DrawRoundedRectangle", (void (wxDC::*)(const class wxPoint &, const class wxSize &, double)) &wxDC::DrawRoundedRectangle, "C++: wxDC::DrawRoundedRectangle(const class wxPoint &, const class wxSize &, double) --> void", pybind11::arg("pt"), pybind11::arg("sz"), pybind11::arg("radius"));
		cl.def("DrawRoundedRectangle", (void (wxDC::*)(const class wxRect &, double)) &wxDC::DrawRoundedRectangle, "C++: wxDC::DrawRoundedRectangle(const class wxRect &, double) --> void", pybind11::arg("r"), pybind11::arg("radius"));
		cl.def("DrawCircle", (void (wxDC::*)(int, int, int)) &wxDC::DrawCircle, "C++: wxDC::DrawCircle(int, int, int) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("radius"));
		cl.def("DrawCircle", (void (wxDC::*)(const class wxPoint &, int)) &wxDC::DrawCircle, "C++: wxDC::DrawCircle(const class wxPoint &, int) --> void", pybind11::arg("pt"), pybind11::arg("radius"));
		cl.def("DrawEllipse", (void (wxDC::*)(int, int, int, int)) &wxDC::DrawEllipse, "C++: wxDC::DrawEllipse(int, int, int, int) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("width"), pybind11::arg("height"));
		cl.def("DrawEllipse", (void (wxDC::*)(const class wxPoint &, const class wxSize &)) &wxDC::DrawEllipse, "C++: wxDC::DrawEllipse(const class wxPoint &, const class wxSize &) --> void", pybind11::arg("pt"), pybind11::arg("sz"));
		cl.def("DrawEllipse", (void (wxDC::*)(const class wxRect &)) &wxDC::DrawEllipse, "C++: wxDC::DrawEllipse(const class wxRect &) --> void", pybind11::arg("rect"));
		cl.def("DrawIcon", (void (wxDC::*)(const class wxIcon &, int, int)) &wxDC::DrawIcon, "C++: wxDC::DrawIcon(const class wxIcon &, int, int) --> void", pybind11::arg("icon"), pybind11::arg("x"), pybind11::arg("y"));
		cl.def("DrawIcon", (void (wxDC::*)(const class wxIcon &, const class wxPoint &)) &wxDC::DrawIcon, "C++: wxDC::DrawIcon(const class wxIcon &, const class wxPoint &) --> void", pybind11::arg("icon"), pybind11::arg("pt"));
		cl.def("DrawBitmap", [](wxDC &o, const class wxBitmap & a0, int const & a1, int const & a2) -> void { return o.DrawBitmap(a0, a1, a2); }, "", pybind11::arg("bmp"), pybind11::arg("x"), pybind11::arg("y"));
		cl.def("DrawBitmap", (void (wxDC::*)(const class wxBitmap &, int, int, bool)) &wxDC::DrawBitmap, "C++: wxDC::DrawBitmap(const class wxBitmap &, int, int, bool) --> void", pybind11::arg("bmp"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("useMask"));
		cl.def("DrawBitmap", [](wxDC &o, const class wxBitmap & a0, const class wxPoint & a1) -> void { return o.DrawBitmap(a0, a1); }, "", pybind11::arg("bmp"), pybind11::arg("pt"));
		cl.def("DrawBitmap", (void (wxDC::*)(const class wxBitmap &, const class wxPoint &, bool)) &wxDC::DrawBitmap, "C++: wxDC::DrawBitmap(const class wxBitmap &, const class wxPoint &, bool) --> void", pybind11::arg("bmp"), pybind11::arg("pt"), pybind11::arg("useMask"));
		cl.def("DrawText", (void (wxDC::*)(const class wxString &, int, int)) &wxDC::DrawText, "C++: wxDC::DrawText(const class wxString &, int, int) --> void", pybind11::arg("text"), pybind11::arg("x"), pybind11::arg("y"));
		cl.def("DrawText", (void (wxDC::*)(const class wxString &, const class wxPoint &)) &wxDC::DrawText, "C++: wxDC::DrawText(const class wxString &, const class wxPoint &) --> void", pybind11::arg("text"), pybind11::arg("pt"));
		cl.def("DrawRotatedText", (void (wxDC::*)(const class wxString &, int, int, double)) &wxDC::DrawRotatedText, "C++: wxDC::DrawRotatedText(const class wxString &, int, int, double) --> void", pybind11::arg("text"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("angle"));
		cl.def("DrawRotatedText", (void (wxDC::*)(const class wxString &, const class wxPoint &, double)) &wxDC::DrawRotatedText, "C++: wxDC::DrawRotatedText(const class wxString &, const class wxPoint &, double) --> void", pybind11::arg("text"), pybind11::arg("pt"), pybind11::arg("angle"));
		cl.def("DrawLabel", [](wxDC &o, const class wxString & a0, const class wxBitmap & a1, const class wxRect & a2) -> void { return o.DrawLabel(a0, a1, a2); }, "", pybind11::arg("text"), pybind11::arg("image"), pybind11::arg("rect"));
		cl.def("DrawLabel", [](wxDC &o, const class wxString & a0, const class wxBitmap & a1, const class wxRect & a2, int const & a3) -> void { return o.DrawLabel(a0, a1, a2, a3); }, "", pybind11::arg("text"), pybind11::arg("image"), pybind11::arg("rect"), pybind11::arg("alignment"));
		cl.def("DrawLabel", [](wxDC &o, const class wxString & a0, const class wxBitmap & a1, const class wxRect & a2, int const & a3, int const & a4) -> void { return o.DrawLabel(a0, a1, a2, a3, a4); }, "", pybind11::arg("text"), pybind11::arg("image"), pybind11::arg("rect"), pybind11::arg("alignment"), pybind11::arg("indexAccel"));
		cl.def("DrawLabel", (void (wxDC::*)(const class wxString &, const class wxBitmap &, const class wxRect &, int, int, class wxRect *)) &wxDC::DrawLabel, "C++: wxDC::DrawLabel(const class wxString &, const class wxBitmap &, const class wxRect &, int, int, class wxRect *) --> void", pybind11::arg("text"), pybind11::arg("image"), pybind11::arg("rect"), pybind11::arg("alignment"), pybind11::arg("indexAccel"), pybind11::arg("rectBounding"));
		cl.def("DrawLabel", [](wxDC &o, const class wxString & a0, const class wxRect & a1) -> void { return o.DrawLabel(a0, a1); }, "", pybind11::arg("text"), pybind11::arg("rect"));
		cl.def("DrawLabel", [](wxDC &o, const class wxString & a0, const class wxRect & a1, int const & a2) -> void { return o.DrawLabel(a0, a1, a2); }, "", pybind11::arg("text"), pybind11::arg("rect"), pybind11::arg("alignment"));
		cl.def("DrawLabel", (void (wxDC::*)(const class wxString &, const class wxRect &, int, int)) &wxDC::DrawLabel, "C++: wxDC::DrawLabel(const class wxString &, const class wxRect &, int, int) --> void", pybind11::arg("text"), pybind11::arg("rect"), pybind11::arg("alignment"), pybind11::arg("indexAccel"));
		cl.def("Blit", [](wxDC &o, int const & a0, int const & a1, int const & a2, int const & a3, class wxDC * a4, int const & a5, int const & a6) -> bool { return o.Blit(a0, a1, a2, a3, a4, a5, a6); }, "", pybind11::arg("xdest"), pybind11::arg("ydest"), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("source"), pybind11::arg("xsrc"), pybind11::arg("ysrc"));
		cl.def("Blit", [](wxDC &o, int const & a0, int const & a1, int const & a2, int const & a3, class wxDC * a4, int const & a5, int const & a6, enum wxRasterOperationMode const & a7) -> bool { return o.Blit(a0, a1, a2, a3, a4, a5, a6, a7); }, "", pybind11::arg("xdest"), pybind11::arg("ydest"), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("source"), pybind11::arg("xsrc"), pybind11::arg("ysrc"), pybind11::arg("rop"));
		cl.def("Blit", [](wxDC &o, int const & a0, int const & a1, int const & a2, int const & a3, class wxDC * a4, int const & a5, int const & a6, enum wxRasterOperationMode const & a7, bool const & a8) -> bool { return o.Blit(a0, a1, a2, a3, a4, a5, a6, a7, a8); }, "", pybind11::arg("xdest"), pybind11::arg("ydest"), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("source"), pybind11::arg("xsrc"), pybind11::arg("ysrc"), pybind11::arg("rop"), pybind11::arg("useMask"));
		cl.def("Blit", [](wxDC &o, int const & a0, int const & a1, int const & a2, int const & a3, class wxDC * a4, int const & a5, int const & a6, enum wxRasterOperationMode const & a7, bool const & a8, int const & a9) -> bool { return o.Blit(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9); }, "", pybind11::arg("xdest"), pybind11::arg("ydest"), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("source"), pybind11::arg("xsrc"), pybind11::arg("ysrc"), pybind11::arg("rop"), pybind11::arg("useMask"), pybind11::arg("xsrcMask"));
		cl.def("Blit", (bool (wxDC::*)(int, int, int, int, class wxDC *, int, int, enum wxRasterOperationMode, bool, int, int)) &wxDC::Blit, "C++: wxDC::Blit(int, int, int, int, class wxDC *, int, int, enum wxRasterOperationMode, bool, int, int) --> bool", pybind11::arg("xdest"), pybind11::arg("ydest"), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("source"), pybind11::arg("xsrc"), pybind11::arg("ysrc"), pybind11::arg("rop"), pybind11::arg("useMask"), pybind11::arg("xsrcMask"), pybind11::arg("ysrcMask"));
		cl.def("Blit", [](wxDC &o, const class wxPoint & a0, const class wxSize & a1, class wxDC * a2, const class wxPoint & a3) -> bool { return o.Blit(a0, a1, a2, a3); }, "", pybind11::arg("destPt"), pybind11::arg("sz"), pybind11::arg("source"), pybind11::arg("srcPt"));
		cl.def("Blit", [](wxDC &o, const class wxPoint & a0, const class wxSize & a1, class wxDC * a2, const class wxPoint & a3, enum wxRasterOperationMode const & a4) -> bool { return o.Blit(a0, a1, a2, a3, a4); }, "", pybind11::arg("destPt"), pybind11::arg("sz"), pybind11::arg("source"), pybind11::arg("srcPt"), pybind11::arg("rop"));
		cl.def("Blit", [](wxDC &o, const class wxPoint & a0, const class wxSize & a1, class wxDC * a2, const class wxPoint & a3, enum wxRasterOperationMode const & a4, bool const & a5) -> bool { return o.Blit(a0, a1, a2, a3, a4, a5); }, "", pybind11::arg("destPt"), pybind11::arg("sz"), pybind11::arg("source"), pybind11::arg("srcPt"), pybind11::arg("rop"), pybind11::arg("useMask"));
		cl.def("Blit", (bool (wxDC::*)(const class wxPoint &, const class wxSize &, class wxDC *, const class wxPoint &, enum wxRasterOperationMode, bool, const class wxPoint &)) &wxDC::Blit, "C++: wxDC::Blit(const class wxPoint &, const class wxSize &, class wxDC *, const class wxPoint &, enum wxRasterOperationMode, bool, const class wxPoint &) --> bool", pybind11::arg("destPt"), pybind11::arg("sz"), pybind11::arg("source"), pybind11::arg("srcPt"), pybind11::arg("rop"), pybind11::arg("useMask"), pybind11::arg("srcPtMask"));
		cl.def("StretchBlit", [](wxDC &o, int const & a0, int const & a1, int const & a2, int const & a3, class wxDC * a4, int const & a5, int const & a6, int const & a7, int const & a8) -> bool { return o.StretchBlit(a0, a1, a2, a3, a4, a5, a6, a7, a8); }, "", pybind11::arg("dstX"), pybind11::arg("dstY"), pybind11::arg("dstWidth"), pybind11::arg("dstHeight"), pybind11::arg("source"), pybind11::arg("srcX"), pybind11::arg("srcY"), pybind11::arg("srcWidth"), pybind11::arg("srcHeight"));
		cl.def("StretchBlit", [](wxDC &o, int const & a0, int const & a1, int const & a2, int const & a3, class wxDC * a4, int const & a5, int const & a6, int const & a7, int const & a8, enum wxRasterOperationMode const & a9) -> bool { return o.StretchBlit(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9); }, "", pybind11::arg("dstX"), pybind11::arg("dstY"), pybind11::arg("dstWidth"), pybind11::arg("dstHeight"), pybind11::arg("source"), pybind11::arg("srcX"), pybind11::arg("srcY"), pybind11::arg("srcWidth"), pybind11::arg("srcHeight"), pybind11::arg("rop"));
		cl.def("StretchBlit", [](wxDC &o, int const & a0, int const & a1, int const & a2, int const & a3, class wxDC * a4, int const & a5, int const & a6, int const & a7, int const & a8, enum wxRasterOperationMode const & a9, bool const & a10) -> bool { return o.StretchBlit(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }, "", pybind11::arg("dstX"), pybind11::arg("dstY"), pybind11::arg("dstWidth"), pybind11::arg("dstHeight"), pybind11::arg("source"), pybind11::arg("srcX"), pybind11::arg("srcY"), pybind11::arg("srcWidth"), pybind11::arg("srcHeight"), pybind11::arg("rop"), pybind11::arg("useMask"));
		cl.def("StretchBlit", [](wxDC &o, int const & a0, int const & a1, int const & a2, int const & a3, class wxDC * a4, int const & a5, int const & a6, int const & a7, int const & a8, enum wxRasterOperationMode const & a9, bool const & a10, int const & a11) -> bool { return o.StretchBlit(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11); }, "", pybind11::arg("dstX"), pybind11::arg("dstY"), pybind11::arg("dstWidth"), pybind11::arg("dstHeight"), pybind11::arg("source"), pybind11::arg("srcX"), pybind11::arg("srcY"), pybind11::arg("srcWidth"), pybind11::arg("srcHeight"), pybind11::arg("rop"), pybind11::arg("useMask"), pybind11::arg("srcMaskX"));
		cl.def("StretchBlit", (bool (wxDC::*)(int, int, int, int, class wxDC *, int, int, int, int, enum wxRasterOperationMode, bool, int, int)) &wxDC::StretchBlit, "C++: wxDC::StretchBlit(int, int, int, int, class wxDC *, int, int, int, int, enum wxRasterOperationMode, bool, int, int) --> bool", pybind11::arg("dstX"), pybind11::arg("dstY"), pybind11::arg("dstWidth"), pybind11::arg("dstHeight"), pybind11::arg("source"), pybind11::arg("srcX"), pybind11::arg("srcY"), pybind11::arg("srcWidth"), pybind11::arg("srcHeight"), pybind11::arg("rop"), pybind11::arg("useMask"), pybind11::arg("srcMaskX"), pybind11::arg("srcMaskY"));
		cl.def("StretchBlit", [](wxDC &o, const class wxPoint & a0, const class wxSize & a1, class wxDC * a2, const class wxPoint & a3, const class wxSize & a4) -> bool { return o.StretchBlit(a0, a1, a2, a3, a4); }, "", pybind11::arg("dstPt"), pybind11::arg("dstSize"), pybind11::arg("source"), pybind11::arg("srcPt"), pybind11::arg("srcSize"));
		cl.def("StretchBlit", [](wxDC &o, const class wxPoint & a0, const class wxSize & a1, class wxDC * a2, const class wxPoint & a3, const class wxSize & a4, enum wxRasterOperationMode const & a5) -> bool { return o.StretchBlit(a0, a1, a2, a3, a4, a5); }, "", pybind11::arg("dstPt"), pybind11::arg("dstSize"), pybind11::arg("source"), pybind11::arg("srcPt"), pybind11::arg("srcSize"), pybind11::arg("rop"));
		cl.def("StretchBlit", [](wxDC &o, const class wxPoint & a0, const class wxSize & a1, class wxDC * a2, const class wxPoint & a3, const class wxSize & a4, enum wxRasterOperationMode const & a5, bool const & a6) -> bool { return o.StretchBlit(a0, a1, a2, a3, a4, a5, a6); }, "", pybind11::arg("dstPt"), pybind11::arg("dstSize"), pybind11::arg("source"), pybind11::arg("srcPt"), pybind11::arg("srcSize"), pybind11::arg("rop"), pybind11::arg("useMask"));
		cl.def("StretchBlit", (bool (wxDC::*)(const class wxPoint &, const class wxSize &, class wxDC *, const class wxPoint &, const class wxSize &, enum wxRasterOperationMode, bool, const class wxPoint &)) &wxDC::StretchBlit, "C++: wxDC::StretchBlit(const class wxPoint &, const class wxSize &, class wxDC *, const class wxPoint &, const class wxSize &, enum wxRasterOperationMode, bool, const class wxPoint &) --> bool", pybind11::arg("dstPt"), pybind11::arg("dstSize"), pybind11::arg("source"), pybind11::arg("srcPt"), pybind11::arg("srcSize"), pybind11::arg("rop"), pybind11::arg("useMask"), pybind11::arg("srcMaskPt"));
		cl.def("GetAsBitmap", [](wxDC const &o) -> wxBitmap { return o.GetAsBitmap(); }, "");
		cl.def("GetAsBitmap", (class wxBitmap (wxDC::*)(const class wxRect *) const) &wxDC::GetAsBitmap, "C++: wxDC::GetAsBitmap(const class wxRect *) const --> class wxBitmap", pybind11::arg("subrect"));
		cl.def("DrawSpline", (void (wxDC::*)(int, int, int, int, int, int)) &wxDC::DrawSpline, "C++: wxDC::DrawSpline(int, int, int, int, int, int) --> void", pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"), pybind11::arg("x3"), pybind11::arg("y3"));
		cl.def("DrawSpline", (void (wxDC::*)(const class wxPointList *)) &wxDC::DrawSpline, "C++: wxDC::DrawSpline(const class wxPointList *) --> void", pybind11::arg("points"));
		cl.def("GetTextExtent", [](wxDC const &o, const class wxString & a0, long * a1, long * a2) -> void { return o.GetTextExtent(a0, a1, a2); }, "", pybind11::arg("string"), pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetTextExtent", [](wxDC const &o, const class wxString & a0, long * a1, long * a2, long * a3) -> void { return o.GetTextExtent(a0, a1, a2, a3); }, "", pybind11::arg("string"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("descent"));
		cl.def("GetTextExtent", [](wxDC const &o, const class wxString & a0, long * a1, long * a2, long * a3, long * a4) -> void { return o.GetTextExtent(a0, a1, a2, a3, a4); }, "", pybind11::arg("string"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("descent"), pybind11::arg("externalLeading"));
		cl.def("GetTextExtent", (void (wxDC::*)(const class wxString &, long *, long *, long *, long *, const class wxFont *) const) &wxDC::GetTextExtent, "C++: wxDC::GetTextExtent(const class wxString &, long *, long *, long *, long *, const class wxFont *) const --> void", pybind11::arg("string"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("descent"), pybind11::arg("externalLeading"), pybind11::arg("theFont"));
		cl.def("GetLogicalOrigin", (void (wxDC::*)(long *, long *) const) &wxDC::GetLogicalOrigin, "C++: wxDC::GetLogicalOrigin(long *, long *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetDeviceOrigin", (void (wxDC::*)(long *, long *) const) &wxDC::GetDeviceOrigin, "C++: wxDC::GetDeviceOrigin(long *, long *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetClippingBox", (void (wxDC::*)(long *, long *, long *, long *) const) &wxDC::GetClippingBox, "C++: wxDC::GetClippingBox(long *, long *, long *, long *) const --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("DrawObject", (void (wxDC::*)(class wxDrawObject *)) &wxDC::DrawObject, "C++: wxDC::DrawObject(class wxDrawObject *) --> void", pybind11::arg("drawobject"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxDC::*)() const) &wxDC::GetClassInfo, "C++: wxDC::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
	}
}

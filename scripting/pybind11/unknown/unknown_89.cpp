#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/dc.h> // wxDC
#include <wx/dc.h> // wxDCFactory
#include <wx/dc.h> // wxDCImpl
#include <wx/dc.h> // wxDrawObject
#include <wx/dc.h> // wxFloodFillStyle
#include <wx/dc.h> // wxFontMetrics
#include <wx/dc.h> // wxMappingMode
#include <wx/dc.h> // wxNativeDCFactory
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

// wxAffineMatrix2D file: line:21
struct PyCallBack_wxAffineMatrix2D : public wxAffineMatrix2D {
	using wxAffineMatrix2D::wxAffineMatrix2D;

	void Set(const struct wxMatrix2D & a0, const class wxPoint2DDouble & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2D *>(this), "Set");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAffineMatrix2D::Set(a0, a1);
	}
	void Get(struct wxMatrix2D * a0, class wxPoint2DDouble * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2D *>(this), "Get");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAffineMatrix2D::Get(a0, a1);
	}
	void Concat(const class wxAffineMatrix2DBase & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2D *>(this), "Concat");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAffineMatrix2D::Concat(a0);
	}
	bool Invert() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2D *>(this), "Invert");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAffineMatrix2D::Invert();
	}
	bool IsIdentity() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2D *>(this), "IsIdentity");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAffineMatrix2D::IsIdentity();
	}
	bool IsEqual(const class wxAffineMatrix2DBase & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2D *>(this), "IsEqual");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxAffineMatrix2D::IsEqual(a0);
	}
	void Translate(double a0, double a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2D *>(this), "Translate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAffineMatrix2D::Translate(a0, a1);
	}
	void Scale(double a0, double a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2D *>(this), "Scale");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAffineMatrix2D::Scale(a0, a1);
	}
	void Rotate(double a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2D *>(this), "Rotate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxAffineMatrix2D::Rotate(a0);
	}
	class wxPoint2DDouble DoTransformPoint(const class wxPoint2DDouble & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2D *>(this), "DoTransformPoint");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPoint2DDouble>::value) {
				static pybind11::detail::override_caster_t<class wxPoint2DDouble> caster;
				return pybind11::detail::cast_ref<class wxPoint2DDouble>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPoint2DDouble>(std::move(o));
		}
		return wxAffineMatrix2D::DoTransformPoint(a0);
	}
	class wxPoint2DDouble DoTransformDistance(const class wxPoint2DDouble & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2D *>(this), "DoTransformDistance");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPoint2DDouble>::value) {
				static pybind11::detail::override_caster_t<class wxPoint2DDouble> caster;
				return pybind11::detail::cast_ref<class wxPoint2DDouble>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPoint2DDouble>(std::move(o));
		}
		return wxAffineMatrix2D::DoTransformDistance(a0);
	}
};

// wxDrawObject file:wx/dc.h line:148
struct PyCallBack_wxDrawObject : public wxDrawObject {
	using wxDrawObject::wxDrawObject;

	void Draw(class wxDC & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDrawObject *>(this), "Draw");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxDrawObject::Draw(a0);
	}
	void CalcBoundingBox(int a0, int a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDrawObject *>(this), "CalcBoundingBox");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxDrawObject::CalcBoundingBox(a0, a1);
	}
	int GetType() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDrawObject *>(this), "GetType");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxDrawObject::GetType\"");
	}
};

void bind_unknown_unknown_89(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxAffineMatrix2D file: line:21
		pybind11::class_<wxAffineMatrix2D, std::shared_ptr<wxAffineMatrix2D>, PyCallBack_wxAffineMatrix2D, wxAffineMatrix2DBase> cl(M(""), "wxAffineMatrix2D", "");
		cl.def( pybind11::init( [](){ return new wxAffineMatrix2D(); }, [](){ return new PyCallBack_wxAffineMatrix2D(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxAffineMatrix2D const &o){ return new PyCallBack_wxAffineMatrix2D(o); } ) );
		cl.def( pybind11::init( [](wxAffineMatrix2D const &o){ return new wxAffineMatrix2D(o); } ) );
		cl.def("Set", (void (wxAffineMatrix2D::*)(const struct wxMatrix2D &, const class wxPoint2DDouble &)) &wxAffineMatrix2D::Set, "C++: wxAffineMatrix2D::Set(const struct wxMatrix2D &, const class wxPoint2DDouble &) --> void", pybind11::arg("mat2D"), pybind11::arg("tr"));
		cl.def("Get", (void (wxAffineMatrix2D::*)(struct wxMatrix2D *, class wxPoint2DDouble *) const) &wxAffineMatrix2D::Get, "C++: wxAffineMatrix2D::Get(struct wxMatrix2D *, class wxPoint2DDouble *) const --> void", pybind11::arg("mat2D"), pybind11::arg("tr"));
		cl.def("Concat", (void (wxAffineMatrix2D::*)(const class wxAffineMatrix2DBase &)) &wxAffineMatrix2D::Concat, "C++: wxAffineMatrix2D::Concat(const class wxAffineMatrix2DBase &) --> void", pybind11::arg("t"));
		cl.def("Invert", (bool (wxAffineMatrix2D::*)()) &wxAffineMatrix2D::Invert, "C++: wxAffineMatrix2D::Invert() --> bool");
		cl.def("IsIdentity", (bool (wxAffineMatrix2D::*)() const) &wxAffineMatrix2D::IsIdentity, "C++: wxAffineMatrix2D::IsIdentity() const --> bool");
		cl.def("IsEqual", (bool (wxAffineMatrix2D::*)(const class wxAffineMatrix2DBase &) const) &wxAffineMatrix2D::IsEqual, "C++: wxAffineMatrix2D::IsEqual(const class wxAffineMatrix2DBase &) const --> bool", pybind11::arg("t"));
		cl.def("Translate", (void (wxAffineMatrix2D::*)(double, double)) &wxAffineMatrix2D::Translate, "C++: wxAffineMatrix2D::Translate(double, double) --> void", pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("Scale", (void (wxAffineMatrix2D::*)(double, double)) &wxAffineMatrix2D::Scale, "C++: wxAffineMatrix2D::Scale(double, double) --> void", pybind11::arg("xScale"), pybind11::arg("yScale"));
		cl.def("Rotate", (void (wxAffineMatrix2D::*)(double)) &wxAffineMatrix2D::Rotate, "C++: wxAffineMatrix2D::Rotate(double) --> void", pybind11::arg("cRadians"));
		cl.def("assign", (class wxAffineMatrix2D & (wxAffineMatrix2D::*)(const class wxAffineMatrix2D &)) &wxAffineMatrix2D::operator=, "C++: wxAffineMatrix2D::operator=(const class wxAffineMatrix2D &) --> class wxAffineMatrix2D &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// wxRasterOperationMode file:wx/dc.h line:50
	pybind11::enum_<wxRasterOperationMode>(M(""), "wxRasterOperationMode", pybind11::arithmetic(), "")
		.value("wxCLEAR", wxCLEAR)
		.value("wxXOR", wxXOR)
		.value("wxINVERT", wxINVERT)
		.value("wxOR_REVERSE", wxOR_REVERSE)
		.value("wxAND_REVERSE", wxAND_REVERSE)
		.value("wxCOPY", wxCOPY)
		.value("wxAND", wxAND)
		.value("wxAND_INVERT", wxAND_INVERT)
		.value("wxNO_OP", wxNO_OP)
		.value("wxNOR", wxNOR)
		.value("wxEQUIV", wxEQUIV)
		.value("wxSRC_INVERT", wxSRC_INVERT)
		.value("wxOR_INVERT", wxOR_INVERT)
		.value("wxNAND", wxNAND)
		.value("wxOR", wxOR)
		.value("wxSET", wxSET)
		.value("wxROP_BLACK", wxROP_BLACK)
		.value("wxBLIT_BLACKNESS", wxBLIT_BLACKNESS)
		.value("wxROP_XORPEN", wxROP_XORPEN)
		.value("wxBLIT_SRCINVERT", wxBLIT_SRCINVERT)
		.value("wxROP_NOT", wxROP_NOT)
		.value("wxBLIT_DSTINVERT", wxBLIT_DSTINVERT)
		.value("wxROP_MERGEPENNOT", wxROP_MERGEPENNOT)
		.value("wxBLIT_00DD0228", wxBLIT_00DD0228)
		.value("wxROP_MASKPENNOT", wxROP_MASKPENNOT)
		.value("wxBLIT_SRCERASE", wxBLIT_SRCERASE)
		.value("wxROP_COPYPEN", wxROP_COPYPEN)
		.value("wxBLIT_SRCCOPY", wxBLIT_SRCCOPY)
		.value("wxROP_MASKPEN", wxROP_MASKPEN)
		.value("wxBLIT_SRCAND", wxBLIT_SRCAND)
		.value("wxROP_MASKNOTPEN", wxROP_MASKNOTPEN)
		.value("wxBLIT_00220326", wxBLIT_00220326)
		.value("wxROP_NOP", wxROP_NOP)
		.value("wxBLIT_00AA0029", wxBLIT_00AA0029)
		.value("wxROP_NOTMERGEPEN", wxROP_NOTMERGEPEN)
		.value("wxBLIT_NOTSRCERASE", wxBLIT_NOTSRCERASE)
		.value("wxROP_NOTXORPEN", wxROP_NOTXORPEN)
		.value("wxBLIT_00990066", wxBLIT_00990066)
		.value("wxROP_NOTCOPYPEN", wxROP_NOTCOPYPEN)
		.value("wxBLIT_NOTSCRCOPY", wxBLIT_NOTSCRCOPY)
		.value("wxROP_MERGENOTPEN", wxROP_MERGENOTPEN)
		.value("wxBLIT_MERGEPAINT", wxBLIT_MERGEPAINT)
		.value("wxROP_NOTMASKPEN", wxROP_NOTMASKPEN)
		.value("wxBLIT_007700E6", wxBLIT_007700E6)
		.value("wxROP_MERGEPEN", wxROP_MERGEPEN)
		.value("wxBLIT_SRCPAINT", wxBLIT_SRCPAINT)
		.value("wxROP_WHITE", wxROP_WHITE)
		.value("wxBLIT_WHITENESS", wxBLIT_WHITENESS)
		.export_values();

;

	// wxFloodFillStyle file:wx/dc.h line:105
	pybind11::enum_<wxFloodFillStyle>(M(""), "wxFloodFillStyle", pybind11::arithmetic(), "")
		.value("wxFLOOD_SURFACE", wxFLOOD_SURFACE)
		.value("wxFLOOD_BORDER", wxFLOOD_BORDER)
		.export_values();

;

	// wxMappingMode file:wx/dc.h line:112
	pybind11::enum_<wxMappingMode>(M(""), "wxMappingMode", pybind11::arithmetic(), "")
		.value("wxMM_TEXT", wxMM_TEXT)
		.value("wxMM_METRIC", wxMM_METRIC)
		.value("wxMM_LOMETRIC", wxMM_LOMETRIC)
		.value("wxMM_TWIPS", wxMM_TWIPS)
		.value("wxMM_POINTS", wxMM_POINTS)
		.export_values();

;

	{ // wxFontMetrics file:wx/dc.h line:122
		pybind11::class_<wxFontMetrics, std::shared_ptr<wxFontMetrics>> cl(M(""), "wxFontMetrics", "");
		cl.def( pybind11::init( [](){ return new wxFontMetrics(); } ) );
		cl.def( pybind11::init( [](wxFontMetrics const &o){ return new wxFontMetrics(o); } ) );
		cl.def_readwrite("height", &wxFontMetrics::height);
		cl.def_readwrite("ascent", &wxFontMetrics::ascent);
		cl.def_readwrite("descent", &wxFontMetrics::descent);
		cl.def_readwrite("internalLeading", &wxFontMetrics::internalLeading);
		cl.def_readwrite("externalLeading", &wxFontMetrics::externalLeading);
		cl.def_readwrite("averageWidth", &wxFontMetrics::averageWidth);
	}
	{ // wxDrawObject file:wx/dc.h line:148
		pybind11::class_<wxDrawObject, std::shared_ptr<wxDrawObject>, PyCallBack_wxDrawObject> cl(M(""), "wxDrawObject", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxDrawObject(); } ) );
		cl.def("Draw", (void (wxDrawObject::*)(class wxDC &) const) &wxDrawObject::Draw, "C++: wxDrawObject::Draw(class wxDC &) const --> void", pybind11::arg(""));
		cl.def("CalcBoundingBox", (void (wxDrawObject::*)(int, int)) &wxDrawObject::CalcBoundingBox, "C++: wxDrawObject::CalcBoundingBox(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("ResetBoundingBox", (void (wxDrawObject::*)()) &wxDrawObject::ResetBoundingBox, "C++: wxDrawObject::ResetBoundingBox() --> void");
		cl.def("MinX", (int (wxDrawObject::*)() const) &wxDrawObject::MinX, "C++: wxDrawObject::MinX() const --> int");
		cl.def("MaxX", (int (wxDrawObject::*)() const) &wxDrawObject::MaxX, "C++: wxDrawObject::MaxX() const --> int");
		cl.def("MinY", (int (wxDrawObject::*)() const) &wxDrawObject::MinY, "C++: wxDrawObject::MinY() const --> int");
		cl.def("MaxY", (int (wxDrawObject::*)() const) &wxDrawObject::MaxY, "C++: wxDrawObject::MaxY() const --> int");
		cl.def("GetType", (int (wxDrawObject::*)()) &wxDrawObject::GetType, "C++: wxDrawObject::GetType() --> int");
		cl.def("assign", (class wxDrawObject & (wxDrawObject::*)(const class wxDrawObject &)) &wxDrawObject::operator=, "C++: wxDrawObject::operator=(const class wxDrawObject &) --> class wxDrawObject &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxDCFactory file:wx/dc.h line:213
		pybind11::class_<wxDCFactory, std::shared_ptr<wxDCFactory>> cl(M(""), "wxDCFactory", "");
		cl.def_static("Set", (void (*)(class wxDCFactory *)) &wxDCFactory::Set, "C++: wxDCFactory::Set(class wxDCFactory *) --> void", pybind11::arg("factory"));
		cl.def_static("Get", (class wxDCFactory * (*)()) &wxDCFactory::Get, "C++: wxDCFactory::Get() --> class wxDCFactory *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxDCFactory & (wxDCFactory::*)(const class wxDCFactory &)) &wxDCFactory::operator=, "C++: wxDCFactory::operator=(const class wxDCFactory &) --> class wxDCFactory &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxNativeDCFactory file:wx/dc.h line:241
		pybind11::class_<wxNativeDCFactory, std::shared_ptr<wxNativeDCFactory>, wxDCFactory> cl(M(""), "wxNativeDCFactory", "");
		cl.def( pybind11::init( [](){ return new wxNativeDCFactory(); } ) );
		cl.def( pybind11::init( [](wxNativeDCFactory const &o){ return new wxNativeDCFactory(o); } ) );
		cl.def("assign", (class wxNativeDCFactory & (wxNativeDCFactory::*)(const class wxNativeDCFactory &)) &wxNativeDCFactory::operator=, "C++: wxNativeDCFactory::operator=(const class wxNativeDCFactory &) --> class wxNativeDCFactory &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxDCImpl file:wx/dc.h line:262
		pybind11::class_<wxDCImpl, std::shared_ptr<wxDCImpl>, wxObject> cl(M(""), "wxDCImpl", "");
		cl.def("GetOwner", (class wxDC * (wxDCImpl::*)() const) &wxDCImpl::GetOwner, "C++: wxDCImpl::GetOwner() const --> class wxDC *", pybind11::return_value_policy::automatic);
		cl.def("GetWindow", (class wxWindow * (wxDCImpl::*)() const) &wxDCImpl::GetWindow, "C++: wxDCImpl::GetWindow() const --> class wxWindow *", pybind11::return_value_policy::automatic);
		cl.def("IsOk", (bool (wxDCImpl::*)() const) &wxDCImpl::IsOk, "C++: wxDCImpl::IsOk() const --> bool");
		cl.def("CanDrawBitmap", (bool (wxDCImpl::*)() const) &wxDCImpl::CanDrawBitmap, "C++: wxDCImpl::CanDrawBitmap() const --> bool");
		cl.def("CanGetTextExtent", (bool (wxDCImpl::*)() const) &wxDCImpl::CanGetTextExtent, "C++: wxDCImpl::CanGetTextExtent() const --> bool");
		cl.def("GetCairoContext", (void * (wxDCImpl::*)() const) &wxDCImpl::GetCairoContext, "C++: wxDCImpl::GetCairoContext() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("GetHandle", (void * (wxDCImpl::*)() const) &wxDCImpl::GetHandle, "C++: wxDCImpl::GetHandle() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("DoGetSize", (void (wxDCImpl::*)(int *, int *) const) &wxDCImpl::DoGetSize, "C++: wxDCImpl::DoGetSize(int *, int *) const --> void", pybind11::arg("width"), pybind11::arg("height"));
		cl.def("GetSize", (void (wxDCImpl::*)(int *, int *) const) &wxDCImpl::GetSize, "C++: wxDCImpl::GetSize(int *, int *) const --> void", pybind11::arg("width"), pybind11::arg("height"));
		cl.def("GetSize", (class wxSize (wxDCImpl::*)() const) &wxDCImpl::GetSize, "C++: wxDCImpl::GetSize() const --> class wxSize");
		cl.def("DoGetSizeMM", (void (wxDCImpl::*)(int *, int *) const) &wxDCImpl::DoGetSizeMM, "C++: wxDCImpl::DoGetSizeMM(int *, int *) const --> void", pybind11::arg("width"), pybind11::arg("height"));
		cl.def("GetDepth", (int (wxDCImpl::*)() const) &wxDCImpl::GetDepth, "C++: wxDCImpl::GetDepth() const --> int");
		cl.def("GetPPI", (class wxSize (wxDCImpl::*)() const) &wxDCImpl::GetPPI, "C++: wxDCImpl::GetPPI() const --> class wxSize");
		cl.def("SetLayoutDirection", (void (wxDCImpl::*)(enum wxLayoutDirection)) &wxDCImpl::SetLayoutDirection, "C++: wxDCImpl::SetLayoutDirection(enum wxLayoutDirection) --> void", pybind11::arg(""));
		cl.def("GetLayoutDirection", (enum wxLayoutDirection (wxDCImpl::*)() const) &wxDCImpl::GetLayoutDirection, "C++: wxDCImpl::GetLayoutDirection() const --> enum wxLayoutDirection");
		cl.def("StartDoc", (bool (wxDCImpl::*)(const class wxString &)) &wxDCImpl::StartDoc, "C++: wxDCImpl::StartDoc(const class wxString &) --> bool", pybind11::arg(""));
		cl.def("EndDoc", (void (wxDCImpl::*)()) &wxDCImpl::EndDoc, "C++: wxDCImpl::EndDoc() --> void");
		cl.def("StartPage", (void (wxDCImpl::*)()) &wxDCImpl::StartPage, "C++: wxDCImpl::StartPage() --> void");
		cl.def("EndPage", (void (wxDCImpl::*)()) &wxDCImpl::EndPage, "C++: wxDCImpl::EndPage() --> void");
		cl.def("Flush", (void (wxDCImpl::*)()) &wxDCImpl::Flush, "C++: wxDCImpl::Flush() --> void");
		cl.def("CalcBoundingBox", (void (wxDCImpl::*)(int, int)) &wxDCImpl::CalcBoundingBox, "C++: wxDCImpl::CalcBoundingBox(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("ResetBoundingBox", (void (wxDCImpl::*)()) &wxDCImpl::ResetBoundingBox, "C++: wxDCImpl::ResetBoundingBox() --> void");
		cl.def("MinX", (int (wxDCImpl::*)() const) &wxDCImpl::MinX, "C++: wxDCImpl::MinX() const --> int");
		cl.def("MaxX", (int (wxDCImpl::*)() const) &wxDCImpl::MaxX, "C++: wxDCImpl::MaxX() const --> int");
		cl.def("MinY", (int (wxDCImpl::*)() const) &wxDCImpl::MinY, "C++: wxDCImpl::MinY() const --> int");
		cl.def("MaxY", (int (wxDCImpl::*)() const) &wxDCImpl::MaxY, "C++: wxDCImpl::MaxY() const --> int");
		cl.def("SetFont", (void (wxDCImpl::*)(const class wxFont &)) &wxDCImpl::SetFont, "C++: wxDCImpl::SetFont(const class wxFont &) --> void", pybind11::arg("font"));
		cl.def("GetFont", (const class wxFont & (wxDCImpl::*)() const) &wxDCImpl::GetFont, "C++: wxDCImpl::GetFont() const --> const class wxFont &", pybind11::return_value_policy::automatic);
		cl.def("SetPen", (void (wxDCImpl::*)(const class wxPen &)) &wxDCImpl::SetPen, "C++: wxDCImpl::SetPen(const class wxPen &) --> void", pybind11::arg("pen"));
		cl.def("GetPen", (const class wxPen & (wxDCImpl::*)() const) &wxDCImpl::GetPen, "C++: wxDCImpl::GetPen() const --> const class wxPen &", pybind11::return_value_policy::automatic);
		cl.def("SetBrush", (void (wxDCImpl::*)(const class wxBrush &)) &wxDCImpl::SetBrush, "C++: wxDCImpl::SetBrush(const class wxBrush &) --> void", pybind11::arg("brush"));
		cl.def("GetBrush", (const class wxBrush & (wxDCImpl::*)() const) &wxDCImpl::GetBrush, "C++: wxDCImpl::GetBrush() const --> const class wxBrush &", pybind11::return_value_policy::automatic);
		cl.def("SetBackground", (void (wxDCImpl::*)(const class wxBrush &)) &wxDCImpl::SetBackground, "C++: wxDCImpl::SetBackground(const class wxBrush &) --> void", pybind11::arg("brush"));
		cl.def("GetBackground", (const class wxBrush & (wxDCImpl::*)() const) &wxDCImpl::GetBackground, "C++: wxDCImpl::GetBackground() const --> const class wxBrush &", pybind11::return_value_policy::automatic);
		cl.def("SetBackgroundMode", (void (wxDCImpl::*)(int)) &wxDCImpl::SetBackgroundMode, "C++: wxDCImpl::SetBackgroundMode(int) --> void", pybind11::arg("mode"));
		cl.def("GetBackgroundMode", (int (wxDCImpl::*)() const) &wxDCImpl::GetBackgroundMode, "C++: wxDCImpl::GetBackgroundMode() const --> int");
		cl.def("SetTextForeground", (void (wxDCImpl::*)(const class wxColour &)) &wxDCImpl::SetTextForeground, "C++: wxDCImpl::SetTextForeground(const class wxColour &) --> void", pybind11::arg("colour"));
		cl.def("GetTextForeground", (const class wxColour & (wxDCImpl::*)() const) &wxDCImpl::GetTextForeground, "C++: wxDCImpl::GetTextForeground() const --> const class wxColour &", pybind11::return_value_policy::automatic);
		cl.def("SetTextBackground", (void (wxDCImpl::*)(const class wxColour &)) &wxDCImpl::SetTextBackground, "C++: wxDCImpl::SetTextBackground(const class wxColour &) --> void", pybind11::arg("colour"));
		cl.def("GetTextBackground", (const class wxColour & (wxDCImpl::*)() const) &wxDCImpl::GetTextBackground, "C++: wxDCImpl::GetTextBackground() const --> const class wxColour &", pybind11::return_value_policy::automatic);
		cl.def("InheritAttributes", (void (wxDCImpl::*)(class wxWindow *)) &wxDCImpl::InheritAttributes, "C++: wxDCImpl::InheritAttributes(class wxWindow *) --> void", pybind11::arg("win"));
		cl.def("SetLogicalFunction", (void (wxDCImpl::*)(enum wxRasterOperationMode)) &wxDCImpl::SetLogicalFunction, "C++: wxDCImpl::SetLogicalFunction(enum wxRasterOperationMode) --> void", pybind11::arg("function"));
		cl.def("GetLogicalFunction", (enum wxRasterOperationMode (wxDCImpl::*)() const) &wxDCImpl::GetLogicalFunction, "C++: wxDCImpl::GetLogicalFunction() const --> enum wxRasterOperationMode");
		cl.def("GetCharHeight", (int (wxDCImpl::*)() const) &wxDCImpl::GetCharHeight, "C++: wxDCImpl::GetCharHeight() const --> int");
		cl.def("GetCharWidth", (int (wxDCImpl::*)() const) &wxDCImpl::GetCharWidth, "C++: wxDCImpl::GetCharWidth() const --> int");
		cl.def("DoGetFontMetrics", (void (wxDCImpl::*)(int *, int *, int *, int *, int *, int *) const) &wxDCImpl::DoGetFontMetrics, "C++: wxDCImpl::DoGetFontMetrics(int *, int *, int *, int *, int *, int *) const --> void", pybind11::arg("height"), pybind11::arg("ascent"), pybind11::arg("descent"), pybind11::arg("internalLeading"), pybind11::arg("externalLeading"), pybind11::arg("averageWidth"));
		cl.def("DoGetTextExtent", [](wxDCImpl const &o, const class wxString & a0, int * a1, int * a2) -> void { return o.DoGetTextExtent(a0, a1, a2); }, "", pybind11::arg("string"), pybind11::arg("x"), pybind11::arg("y"));
		cl.def("DoGetTextExtent", [](wxDCImpl const &o, const class wxString & a0, int * a1, int * a2, int * a3) -> void { return o.DoGetTextExtent(a0, a1, a2, a3); }, "", pybind11::arg("string"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("descent"));
		cl.def("DoGetTextExtent", [](wxDCImpl const &o, const class wxString & a0, int * a1, int * a2, int * a3, int * a4) -> void { return o.DoGetTextExtent(a0, a1, a2, a3, a4); }, "", pybind11::arg("string"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("descent"), pybind11::arg("externalLeading"));
		cl.def("DoGetTextExtent", (void (wxDCImpl::*)(const class wxString &, int *, int *, int *, int *, const class wxFont *) const) &wxDCImpl::DoGetTextExtent, "C++: wxDCImpl::DoGetTextExtent(const class wxString &, int *, int *, int *, int *, const class wxFont *) const --> void", pybind11::arg("string"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("descent"), pybind11::arg("externalLeading"), pybind11::arg("theFont"));
		cl.def("GetMultiLineTextExtent", [](wxDCImpl const &o, const class wxString & a0, int * a1, int * a2) -> void { return o.GetMultiLineTextExtent(a0, a1, a2); }, "", pybind11::arg("string"), pybind11::arg("width"), pybind11::arg("height"));
		cl.def("GetMultiLineTextExtent", [](wxDCImpl const &o, const class wxString & a0, int * a1, int * a2, int * a3) -> void { return o.GetMultiLineTextExtent(a0, a1, a2, a3); }, "", pybind11::arg("string"), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("heightLine"));
		cl.def("GetMultiLineTextExtent", (void (wxDCImpl::*)(const class wxString &, int *, int *, int *, const class wxFont *) const) &wxDCImpl::GetMultiLineTextExtent, "C++: wxDCImpl::GetMultiLineTextExtent(const class wxString &, int *, int *, int *, const class wxFont *) const --> void", pybind11::arg("string"), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("heightLine"), pybind11::arg("font"));
		cl.def("DoGetPartialTextExtents", (bool (wxDCImpl::*)(const class wxString &, class wxArrayInt &) const) &wxDCImpl::DoGetPartialTextExtents, "C++: wxDCImpl::DoGetPartialTextExtents(const class wxString &, class wxArrayInt &) const --> bool", pybind11::arg("text"), pybind11::arg("widths"));
		cl.def("Clear", (void (wxDCImpl::*)()) &wxDCImpl::Clear, "C++: wxDCImpl::Clear() --> void");
		cl.def("DoSetClippingRegion", (void (wxDCImpl::*)(int, int, int, int)) &wxDCImpl::DoSetClippingRegion, "C++: wxDCImpl::DoSetClippingRegion(int, int, int, int) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("DoSetDeviceClippingRegion", (void (wxDCImpl::*)(const class wxRegion &)) &wxDCImpl::DoSetDeviceClippingRegion, "C++: wxDCImpl::DoSetDeviceClippingRegion(const class wxRegion &) --> void", pybind11::arg("region"));
		cl.def("DoGetClippingBox", (void (wxDCImpl::*)(int *, int *, int *, int *) const) &wxDCImpl::DoGetClippingBox, "C++: wxDCImpl::DoGetClippingBox(int *, int *, int *, int *) const --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"));
		cl.def("DestroyClippingRegion", (void (wxDCImpl::*)()) &wxDCImpl::DestroyClippingRegion, "C++: wxDCImpl::DestroyClippingRegion() --> void");
		cl.def("DeviceToLogicalX", (int (wxDCImpl::*)(int) const) &wxDCImpl::DeviceToLogicalX, "C++: wxDCImpl::DeviceToLogicalX(int) const --> int", pybind11::arg("x"));
		cl.def("DeviceToLogicalY", (int (wxDCImpl::*)(int) const) &wxDCImpl::DeviceToLogicalY, "C++: wxDCImpl::DeviceToLogicalY(int) const --> int", pybind11::arg("y"));
		cl.def("DeviceToLogicalXRel", (int (wxDCImpl::*)(int) const) &wxDCImpl::DeviceToLogicalXRel, "C++: wxDCImpl::DeviceToLogicalXRel(int) const --> int", pybind11::arg("x"));
		cl.def("DeviceToLogicalYRel", (int (wxDCImpl::*)(int) const) &wxDCImpl::DeviceToLogicalYRel, "C++: wxDCImpl::DeviceToLogicalYRel(int) const --> int", pybind11::arg("y"));
		cl.def("LogicalToDeviceX", (int (wxDCImpl::*)(int) const) &wxDCImpl::LogicalToDeviceX, "C++: wxDCImpl::LogicalToDeviceX(int) const --> int", pybind11::arg("x"));
		cl.def("LogicalToDeviceY", (int (wxDCImpl::*)(int) const) &wxDCImpl::LogicalToDeviceY, "C++: wxDCImpl::LogicalToDeviceY(int) const --> int", pybind11::arg("y"));
		cl.def("LogicalToDeviceXRel", (int (wxDCImpl::*)(int) const) &wxDCImpl::LogicalToDeviceXRel, "C++: wxDCImpl::LogicalToDeviceXRel(int) const --> int", pybind11::arg("x"));
		cl.def("LogicalToDeviceYRel", (int (wxDCImpl::*)(int) const) &wxDCImpl::LogicalToDeviceYRel, "C++: wxDCImpl::LogicalToDeviceYRel(int) const --> int", pybind11::arg("y"));
		cl.def("SetMapMode", (void (wxDCImpl::*)(enum wxMappingMode)) &wxDCImpl::SetMapMode, "C++: wxDCImpl::SetMapMode(enum wxMappingMode) --> void", pybind11::arg("mode"));
		cl.def("GetMapMode", (enum wxMappingMode (wxDCImpl::*)() const) &wxDCImpl::GetMapMode, "C++: wxDCImpl::GetMapMode() const --> enum wxMappingMode");
		cl.def("SetUserScale", (void (wxDCImpl::*)(double, double)) &wxDCImpl::SetUserScale, "C++: wxDCImpl::SetUserScale(double, double) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetUserScale", (void (wxDCImpl::*)(double *, double *) const) &wxDCImpl::GetUserScale, "C++: wxDCImpl::GetUserScale(double *, double *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("SetLogicalScale", (void (wxDCImpl::*)(double, double)) &wxDCImpl::SetLogicalScale, "C++: wxDCImpl::SetLogicalScale(double, double) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetLogicalScale", (void (wxDCImpl::*)(double *, double *) const) &wxDCImpl::GetLogicalScale, "C++: wxDCImpl::GetLogicalScale(double *, double *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("SetLogicalOrigin", (void (wxDCImpl::*)(int, int)) &wxDCImpl::SetLogicalOrigin, "C++: wxDCImpl::SetLogicalOrigin(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("DoGetLogicalOrigin", (void (wxDCImpl::*)(int *, int *) const) &wxDCImpl::DoGetLogicalOrigin, "C++: wxDCImpl::DoGetLogicalOrigin(int *, int *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("SetDeviceOrigin", (void (wxDCImpl::*)(int, int)) &wxDCImpl::SetDeviceOrigin, "C++: wxDCImpl::SetDeviceOrigin(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("DoGetDeviceOrigin", (void (wxDCImpl::*)(int *, int *) const) &wxDCImpl::DoGetDeviceOrigin, "C++: wxDCImpl::DoGetDeviceOrigin(int *, int *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("CanUseTransformMatrix", (bool (wxDCImpl::*)() const) &wxDCImpl::CanUseTransformMatrix, "C++: wxDCImpl::CanUseTransformMatrix() const --> bool");
		cl.def("SetTransformMatrix", (bool (wxDCImpl::*)(const class wxAffineMatrix2D &)) &wxDCImpl::SetTransformMatrix, "C++: wxDCImpl::SetTransformMatrix(const class wxAffineMatrix2D &) --> bool", pybind11::arg(""));
		cl.def("GetTransformMatrix", (class wxAffineMatrix2D (wxDCImpl::*)() const) &wxDCImpl::GetTransformMatrix, "C++: wxDCImpl::GetTransformMatrix() const --> class wxAffineMatrix2D");
		cl.def("ResetTransformMatrix", (void (wxDCImpl::*)()) &wxDCImpl::ResetTransformMatrix, "C++: wxDCImpl::ResetTransformMatrix() --> void");
		cl.def("SetDeviceLocalOrigin", (void (wxDCImpl::*)(int, int)) &wxDCImpl::SetDeviceLocalOrigin, "C++: wxDCImpl::SetDeviceLocalOrigin(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("ComputeScaleAndOrigin", (void (wxDCImpl::*)()) &wxDCImpl::ComputeScaleAndOrigin, "C++: wxDCImpl::ComputeScaleAndOrigin() --> void");
		cl.def("SetAxisOrientation", (void (wxDCImpl::*)(bool, bool)) &wxDCImpl::SetAxisOrientation, "C++: wxDCImpl::SetAxisOrientation(bool, bool) --> void", pybind11::arg("xLeftRight"), pybind11::arg("yBottomUp"));
		cl.def("GetContentScaleFactor", (double (wxDCImpl::*)() const) &wxDCImpl::GetContentScaleFactor, "C++: wxDCImpl::GetContentScaleFactor() const --> double");
		cl.def("DoFloodFill", [](wxDCImpl &o, int const & a0, int const & a1, const class wxColour & a2) -> bool { return o.DoFloodFill(a0, a1, a2); }, "", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("col"));
		cl.def("DoFloodFill", (bool (wxDCImpl::*)(int, int, const class wxColour &, enum wxFloodFillStyle)) &wxDCImpl::DoFloodFill, "C++: wxDCImpl::DoFloodFill(int, int, const class wxColour &, enum wxFloodFillStyle) --> bool", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("col"), pybind11::arg("style"));
		cl.def("DoGradientFillLinear", [](wxDCImpl &o, const class wxRect & a0, const class wxColour & a1, const class wxColour & a2) -> void { return o.DoGradientFillLinear(a0, a1, a2); }, "", pybind11::arg("rect"), pybind11::arg("initialColour"), pybind11::arg("destColour"));
		cl.def("DoGradientFillLinear", (void (wxDCImpl::*)(const class wxRect &, const class wxColour &, const class wxColour &, enum wxDirection)) &wxDCImpl::DoGradientFillLinear, "C++: wxDCImpl::DoGradientFillLinear(const class wxRect &, const class wxColour &, const class wxColour &, enum wxDirection) --> void", pybind11::arg("rect"), pybind11::arg("initialColour"), pybind11::arg("destColour"), pybind11::arg("nDirection"));
		cl.def("DoGradientFillConcentric", (void (wxDCImpl::*)(const class wxRect &, const class wxColour &, const class wxColour &, const class wxPoint &)) &wxDCImpl::DoGradientFillConcentric, "C++: wxDCImpl::DoGradientFillConcentric(const class wxRect &, const class wxColour &, const class wxColour &, const class wxPoint &) --> void", pybind11::arg("rect"), pybind11::arg("initialColour"), pybind11::arg("destColour"), pybind11::arg("circleCenter"));
		cl.def("DoGetPixel", (bool (wxDCImpl::*)(int, int, class wxColour *) const) &wxDCImpl::DoGetPixel, "C++: wxDCImpl::DoGetPixel(int, int, class wxColour *) const --> bool", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("col"));
		cl.def("DoDrawPoint", (void (wxDCImpl::*)(int, int)) &wxDCImpl::DoDrawPoint, "C++: wxDCImpl::DoDrawPoint(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("DoDrawLine", (void (wxDCImpl::*)(int, int, int, int)) &wxDCImpl::DoDrawLine, "C++: wxDCImpl::DoDrawLine(int, int, int, int) --> void", pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"));
		cl.def("DoDrawArc", (void (wxDCImpl::*)(int, int, int, int, int, int)) &wxDCImpl::DoDrawArc, "C++: wxDCImpl::DoDrawArc(int, int, int, int, int, int) --> void", pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"), pybind11::arg("xc"), pybind11::arg("yc"));
		cl.def("DoDrawCheckMark", (void (wxDCImpl::*)(int, int, int, int)) &wxDCImpl::DoDrawCheckMark, "C++: wxDCImpl::DoDrawCheckMark(int, int, int, int) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("width"), pybind11::arg("height"));
		cl.def("DoDrawEllipticArc", (void (wxDCImpl::*)(int, int, int, int, double, double)) &wxDCImpl::DoDrawEllipticArc, "C++: wxDCImpl::DoDrawEllipticArc(int, int, int, int, double, double) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h"), pybind11::arg("sa"), pybind11::arg("ea"));
		cl.def("DoDrawRectangle", (void (wxDCImpl::*)(int, int, int, int)) &wxDCImpl::DoDrawRectangle, "C++: wxDCImpl::DoDrawRectangle(int, int, int, int) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("width"), pybind11::arg("height"));
		cl.def("DoDrawRoundedRectangle", (void (wxDCImpl::*)(int, int, int, int, double)) &wxDCImpl::DoDrawRoundedRectangle, "C++: wxDCImpl::DoDrawRoundedRectangle(int, int, int, int, double) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("radius"));
		cl.def("DoDrawEllipse", (void (wxDCImpl::*)(int, int, int, int)) &wxDCImpl::DoDrawEllipse, "C++: wxDCImpl::DoDrawEllipse(int, int, int, int) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("width"), pybind11::arg("height"));
		cl.def("DoCrossHair", (void (wxDCImpl::*)(int, int)) &wxDCImpl::DoCrossHair, "C++: wxDCImpl::DoCrossHair(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("DoDrawIcon", (void (wxDCImpl::*)(const class wxIcon &, int, int)) &wxDCImpl::DoDrawIcon, "C++: wxDCImpl::DoDrawIcon(const class wxIcon &, int, int) --> void", pybind11::arg("icon"), pybind11::arg("x"), pybind11::arg("y"));
		cl.def("DoDrawBitmap", [](wxDCImpl &o, const class wxBitmap & a0, int const & a1, int const & a2) -> void { return o.DoDrawBitmap(a0, a1, a2); }, "", pybind11::arg("bmp"), pybind11::arg("x"), pybind11::arg("y"));
		cl.def("DoDrawBitmap", (void (wxDCImpl::*)(const class wxBitmap &, int, int, bool)) &wxDCImpl::DoDrawBitmap, "C++: wxDCImpl::DoDrawBitmap(const class wxBitmap &, int, int, bool) --> void", pybind11::arg("bmp"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("useMask"));
		cl.def("DoDrawText", (void (wxDCImpl::*)(const class wxString &, int, int)) &wxDCImpl::DoDrawText, "C++: wxDCImpl::DoDrawText(const class wxString &, int, int) --> void", pybind11::arg("text"), pybind11::arg("x"), pybind11::arg("y"));
		cl.def("DoDrawRotatedText", (void (wxDCImpl::*)(const class wxString &, int, int, double)) &wxDCImpl::DoDrawRotatedText, "C++: wxDCImpl::DoDrawRotatedText(const class wxString &, int, int, double) --> void", pybind11::arg("text"), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("angle"));
		cl.def("DoBlit", [](wxDCImpl &o, int const & a0, int const & a1, int const & a2, int const & a3, class wxDC * a4, int const & a5, int const & a6) -> bool { return o.DoBlit(a0, a1, a2, a3, a4, a5, a6); }, "", pybind11::arg("xdest"), pybind11::arg("ydest"), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("source"), pybind11::arg("xsrc"), pybind11::arg("ysrc"));
		cl.def("DoBlit", [](wxDCImpl &o, int const & a0, int const & a1, int const & a2, int const & a3, class wxDC * a4, int const & a5, int const & a6, enum wxRasterOperationMode const & a7) -> bool { return o.DoBlit(a0, a1, a2, a3, a4, a5, a6, a7); }, "", pybind11::arg("xdest"), pybind11::arg("ydest"), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("source"), pybind11::arg("xsrc"), pybind11::arg("ysrc"), pybind11::arg("rop"));
		cl.def("DoBlit", [](wxDCImpl &o, int const & a0, int const & a1, int const & a2, int const & a3, class wxDC * a4, int const & a5, int const & a6, enum wxRasterOperationMode const & a7, bool const & a8) -> bool { return o.DoBlit(a0, a1, a2, a3, a4, a5, a6, a7, a8); }, "", pybind11::arg("xdest"), pybind11::arg("ydest"), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("source"), pybind11::arg("xsrc"), pybind11::arg("ysrc"), pybind11::arg("rop"), pybind11::arg("useMask"));
		cl.def("DoBlit", [](wxDCImpl &o, int const & a0, int const & a1, int const & a2, int const & a3, class wxDC * a4, int const & a5, int const & a6, enum wxRasterOperationMode const & a7, bool const & a8, int const & a9) -> bool { return o.DoBlit(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9); }, "", pybind11::arg("xdest"), pybind11::arg("ydest"), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("source"), pybind11::arg("xsrc"), pybind11::arg("ysrc"), pybind11::arg("rop"), pybind11::arg("useMask"), pybind11::arg("xsrcMask"));
		cl.def("DoBlit", (bool (wxDCImpl::*)(int, int, int, int, class wxDC *, int, int, enum wxRasterOperationMode, bool, int, int)) &wxDCImpl::DoBlit, "C++: wxDCImpl::DoBlit(int, int, int, int, class wxDC *, int, int, enum wxRasterOperationMode, bool, int, int) --> bool", pybind11::arg("xdest"), pybind11::arg("ydest"), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("source"), pybind11::arg("xsrc"), pybind11::arg("ysrc"), pybind11::arg("rop"), pybind11::arg("useMask"), pybind11::arg("xsrcMask"), pybind11::arg("ysrcMask"));
		cl.def("DoStretchBlit", [](wxDCImpl &o, int const & a0, int const & a1, int const & a2, int const & a3, class wxDC * a4, int const & a5, int const & a6, int const & a7, int const & a8) -> bool { return o.DoStretchBlit(a0, a1, a2, a3, a4, a5, a6, a7, a8); }, "", pybind11::arg("xdest"), pybind11::arg("ydest"), pybind11::arg("dstWidth"), pybind11::arg("dstHeight"), pybind11::arg("source"), pybind11::arg("xsrc"), pybind11::arg("ysrc"), pybind11::arg("srcWidth"), pybind11::arg("srcHeight"));
		cl.def("DoStretchBlit", [](wxDCImpl &o, int const & a0, int const & a1, int const & a2, int const & a3, class wxDC * a4, int const & a5, int const & a6, int const & a7, int const & a8, enum wxRasterOperationMode const & a9) -> bool { return o.DoStretchBlit(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9); }, "", pybind11::arg("xdest"), pybind11::arg("ydest"), pybind11::arg("dstWidth"), pybind11::arg("dstHeight"), pybind11::arg("source"), pybind11::arg("xsrc"), pybind11::arg("ysrc"), pybind11::arg("srcWidth"), pybind11::arg("srcHeight"), pybind11::arg("rop"));
		cl.def("DoStretchBlit", [](wxDCImpl &o, int const & a0, int const & a1, int const & a2, int const & a3, class wxDC * a4, int const & a5, int const & a6, int const & a7, int const & a8, enum wxRasterOperationMode const & a9, bool const & a10) -> bool { return o.DoStretchBlit(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10); }, "", pybind11::arg("xdest"), pybind11::arg("ydest"), pybind11::arg("dstWidth"), pybind11::arg("dstHeight"), pybind11::arg("source"), pybind11::arg("xsrc"), pybind11::arg("ysrc"), pybind11::arg("srcWidth"), pybind11::arg("srcHeight"), pybind11::arg("rop"), pybind11::arg("useMask"));
		cl.def("DoStretchBlit", [](wxDCImpl &o, int const & a0, int const & a1, int const & a2, int const & a3, class wxDC * a4, int const & a5, int const & a6, int const & a7, int const & a8, enum wxRasterOperationMode const & a9, bool const & a10, int const & a11) -> bool { return o.DoStretchBlit(a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11); }, "", pybind11::arg("xdest"), pybind11::arg("ydest"), pybind11::arg("dstWidth"), pybind11::arg("dstHeight"), pybind11::arg("source"), pybind11::arg("xsrc"), pybind11::arg("ysrc"), pybind11::arg("srcWidth"), pybind11::arg("srcHeight"), pybind11::arg("rop"), pybind11::arg("useMask"), pybind11::arg("xsrcMask"));
		cl.def("DoStretchBlit", (bool (wxDCImpl::*)(int, int, int, int, class wxDC *, int, int, int, int, enum wxRasterOperationMode, bool, int, int)) &wxDCImpl::DoStretchBlit, "C++: wxDCImpl::DoStretchBlit(int, int, int, int, class wxDC *, int, int, int, int, enum wxRasterOperationMode, bool, int, int) --> bool", pybind11::arg("xdest"), pybind11::arg("ydest"), pybind11::arg("dstWidth"), pybind11::arg("dstHeight"), pybind11::arg("source"), pybind11::arg("xsrc"), pybind11::arg("ysrc"), pybind11::arg("srcWidth"), pybind11::arg("srcHeight"), pybind11::arg("rop"), pybind11::arg("useMask"), pybind11::arg("xsrcMask"), pybind11::arg("ysrcMask"));
		cl.def("DoGetAsBitmap", (class wxBitmap (wxDCImpl::*)(const class wxRect *) const) &wxDCImpl::DoGetAsBitmap, "C++: wxDCImpl::DoGetAsBitmap(const class wxRect *) const --> class wxBitmap", pybind11::arg(""));
		cl.def("DrawLines", (void (wxDCImpl::*)(const class wxPointList *, int, int)) &wxDCImpl::DrawLines, "C++: wxDCImpl::DrawLines(const class wxPointList *, int, int) --> void", pybind11::arg("list"), pybind11::arg("xoffset"), pybind11::arg("yoffset"));
		cl.def("DrawPolygon", (void (wxDCImpl::*)(const class wxPointList *, int, int, enum wxPolygonFillMode)) &wxDCImpl::DrawPolygon, "C++: wxDCImpl::DrawPolygon(const class wxPointList *, int, int, enum wxPolygonFillMode) --> void", pybind11::arg("list"), pybind11::arg("xoffset"), pybind11::arg("yoffset"), pybind11::arg("fillStyle"));
		cl.def("DrawSpline", (void (wxDCImpl::*)(int, int, int, int, int, int)) &wxDCImpl::DrawSpline, "C++: wxDCImpl::DrawSpline(int, int, int, int, int, int) --> void", pybind11::arg("x1"), pybind11::arg("y1"), pybind11::arg("x2"), pybind11::arg("y2"), pybind11::arg("x3"), pybind11::arg("y3"));
		cl.def("DrawSpline", (void (wxDCImpl::*)(const class wxPointList *)) &wxDCImpl::DrawSpline, "C++: wxDCImpl::DrawSpline(const class wxPointList *) --> void", pybind11::arg("points"));
		cl.def("DoDrawSpline", (void (wxDCImpl::*)(const class wxPointList *)) &wxDCImpl::DoDrawSpline, "C++: wxDCImpl::DoDrawSpline(const class wxPointList *) --> void", pybind11::arg("points"));
		cl.def("DoSelect", (void (wxDCImpl::*)(const class wxBitmap &)) &wxDCImpl::DoSelect, "C++: wxDCImpl::DoSelect(const class wxBitmap &) --> void", pybind11::arg(""));
		cl.def("GetSelectedBitmap", (class wxBitmap & (wxDCImpl::*)()) &wxDCImpl::GetSelectedBitmap, "C++: wxDCImpl::GetSelectedBitmap() --> class wxBitmap &", pybind11::return_value_policy::automatic);
		cl.def("GetPaperRect", (class wxRect (wxDCImpl::*)() const) &wxDCImpl::GetPaperRect, "C++: wxDCImpl::GetPaperRect() const --> class wxRect");
		cl.def("GetResolution", (int (wxDCImpl::*)() const) &wxDCImpl::GetResolution, "C++: wxDCImpl::GetResolution() const --> int");
		cl.def("GetClassInfo", (class wxClassInfo * (wxDCImpl::*)() const) &wxDCImpl::GetClassInfo, "C++: wxDCImpl::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxDCImpl & (wxDCImpl::*)(const class wxDCImpl &)) &wxDCImpl::operator=, "C++: wxDCImpl::operator=(const class wxDCImpl &) --> class wxDCImpl &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}

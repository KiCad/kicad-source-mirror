#include <sstream> // __str__

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxwxPointListNode file: line:57
struct PyCallBack_wxwxPointListNode : public wxwxPointListNode {
	using wxwxPointListNode::wxwxPointListNode;

	void DeleteData() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxwxPointListNode *>(this), "DeleteData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxwxPointListNode::DeleteData();
	}
};

void bind_unknown_unknown_30(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxIsSameDouble(double, double) file: line:141
	M("").def("wxIsSameDouble", (bool (*)(double, double)) &wxIsSameDouble, "C++: wxIsSameDouble(double, double) --> bool", pybind11::arg("x"), pybind11::arg("y"));

	// wxIsNullDouble(double) file: line:146
	M("").def("wxIsNullDouble", (bool (*)(double)) &wxIsNullDouble, "C++: wxIsNullDouble(double) --> bool", pybind11::arg("x"));

	// wxRound(double) file: line:148
	M("").def("wxRound", (int (*)(double)) &wxRound, "C++: wxRound(double) --> int", pybind11::arg("x"));

	// wxConvertFromIeeeExtended(const signed char *) file: line:174
	M("").def("wxConvertFromIeeeExtended", (double (*)(const signed char *)) &wxConvertFromIeeeExtended, "C++: wxConvertFromIeeeExtended(const signed char *) --> double", pybind11::arg("bytes"));

	// wxConvertToIeeeExtended(double, signed char *) file: line:175
	M("").def("wxConvertToIeeeExtended", (void (*)(double, signed char *)) &wxConvertToIeeeExtended, "C++: wxConvertToIeeeExtended(double, signed char *) --> void", pybind11::arg("num"), pybind11::arg("bytes"));

	// ConvertFromIeeeExtended(const signed char *) file: line:179
	M("").def("ConvertFromIeeeExtended", (double (*)(const signed char *)) &ConvertFromIeeeExtended, "C++: ConvertFromIeeeExtended(const signed char *) --> double", pybind11::arg("bytes"));

	// ConvertToIeeeExtended(double, signed char *) file: line:180
	M("").def("ConvertToIeeeExtended", (void (*)(double, signed char *)) &ConvertToIeeeExtended, "C++: ConvertToIeeeExtended(double, signed char *) --> void", pybind11::arg("num"), pybind11::arg("bytes"));

	// wxBitmapType file: line:47
	pybind11::enum_<wxBitmapType>(M(""), "wxBitmapType", pybind11::arithmetic(), "")
		.value("wxBITMAP_TYPE_INVALID", wxBITMAP_TYPE_INVALID)
		.value("wxBITMAP_TYPE_BMP", wxBITMAP_TYPE_BMP)
		.value("wxBITMAP_TYPE_BMP_RESOURCE", wxBITMAP_TYPE_BMP_RESOURCE)
		.value("wxBITMAP_TYPE_RESOURCE", wxBITMAP_TYPE_RESOURCE)
		.value("wxBITMAP_TYPE_ICO", wxBITMAP_TYPE_ICO)
		.value("wxBITMAP_TYPE_ICO_RESOURCE", wxBITMAP_TYPE_ICO_RESOURCE)
		.value("wxBITMAP_TYPE_CUR", wxBITMAP_TYPE_CUR)
		.value("wxBITMAP_TYPE_CUR_RESOURCE", wxBITMAP_TYPE_CUR_RESOURCE)
		.value("wxBITMAP_TYPE_XBM", wxBITMAP_TYPE_XBM)
		.value("wxBITMAP_TYPE_XBM_DATA", wxBITMAP_TYPE_XBM_DATA)
		.value("wxBITMAP_TYPE_XPM", wxBITMAP_TYPE_XPM)
		.value("wxBITMAP_TYPE_XPM_DATA", wxBITMAP_TYPE_XPM_DATA)
		.value("wxBITMAP_TYPE_TIFF", wxBITMAP_TYPE_TIFF)
		.value("wxBITMAP_TYPE_TIF", wxBITMAP_TYPE_TIF)
		.value("wxBITMAP_TYPE_TIFF_RESOURCE", wxBITMAP_TYPE_TIFF_RESOURCE)
		.value("wxBITMAP_TYPE_TIF_RESOURCE", wxBITMAP_TYPE_TIF_RESOURCE)
		.value("wxBITMAP_TYPE_GIF", wxBITMAP_TYPE_GIF)
		.value("wxBITMAP_TYPE_GIF_RESOURCE", wxBITMAP_TYPE_GIF_RESOURCE)
		.value("wxBITMAP_TYPE_PNG", wxBITMAP_TYPE_PNG)
		.value("wxBITMAP_TYPE_PNG_RESOURCE", wxBITMAP_TYPE_PNG_RESOURCE)
		.value("wxBITMAP_TYPE_JPEG", wxBITMAP_TYPE_JPEG)
		.value("wxBITMAP_TYPE_JPEG_RESOURCE", wxBITMAP_TYPE_JPEG_RESOURCE)
		.value("wxBITMAP_TYPE_PNM", wxBITMAP_TYPE_PNM)
		.value("wxBITMAP_TYPE_PNM_RESOURCE", wxBITMAP_TYPE_PNM_RESOURCE)
		.value("wxBITMAP_TYPE_PCX", wxBITMAP_TYPE_PCX)
		.value("wxBITMAP_TYPE_PCX_RESOURCE", wxBITMAP_TYPE_PCX_RESOURCE)
		.value("wxBITMAP_TYPE_PICT", wxBITMAP_TYPE_PICT)
		.value("wxBITMAP_TYPE_PICT_RESOURCE", wxBITMAP_TYPE_PICT_RESOURCE)
		.value("wxBITMAP_TYPE_ICON", wxBITMAP_TYPE_ICON)
		.value("wxBITMAP_TYPE_ICON_RESOURCE", wxBITMAP_TYPE_ICON_RESOURCE)
		.value("wxBITMAP_TYPE_ANI", wxBITMAP_TYPE_ANI)
		.value("wxBITMAP_TYPE_IFF", wxBITMAP_TYPE_IFF)
		.value("wxBITMAP_TYPE_TGA", wxBITMAP_TYPE_TGA)
		.value("wxBITMAP_TYPE_MACCURSOR", wxBITMAP_TYPE_MACCURSOR)
		.value("wxBITMAP_TYPE_MACCURSOR_RESOURCE", wxBITMAP_TYPE_MACCURSOR_RESOURCE)
		.value("wxBITMAP_TYPE_MAX", wxBITMAP_TYPE_MAX)
		.value("wxBITMAP_TYPE_ANY", wxBITMAP_TYPE_ANY)
		.export_values();

;

	// wxPolygonFillMode file: line:90
	pybind11::enum_<wxPolygonFillMode>(M(""), "wxPolygonFillMode", pybind11::arithmetic(), "")
		.value("wxODDEVEN_RULE", wxODDEVEN_RULE)
		.value("wxWINDING_RULE", wxWINDING_RULE)
		.export_values();

;

	// wxStockCursor file: line:97
	pybind11::enum_<wxStockCursor>(M(""), "wxStockCursor", pybind11::arithmetic(), "")
		.value("wxCURSOR_NONE", wxCURSOR_NONE)
		.value("wxCURSOR_ARROW", wxCURSOR_ARROW)
		.value("wxCURSOR_RIGHT_ARROW", wxCURSOR_RIGHT_ARROW)
		.value("wxCURSOR_BULLSEYE", wxCURSOR_BULLSEYE)
		.value("wxCURSOR_CHAR", wxCURSOR_CHAR)
		.value("wxCURSOR_CROSS", wxCURSOR_CROSS)
		.value("wxCURSOR_HAND", wxCURSOR_HAND)
		.value("wxCURSOR_IBEAM", wxCURSOR_IBEAM)
		.value("wxCURSOR_LEFT_BUTTON", wxCURSOR_LEFT_BUTTON)
		.value("wxCURSOR_MAGNIFIER", wxCURSOR_MAGNIFIER)
		.value("wxCURSOR_MIDDLE_BUTTON", wxCURSOR_MIDDLE_BUTTON)
		.value("wxCURSOR_NO_ENTRY", wxCURSOR_NO_ENTRY)
		.value("wxCURSOR_PAINT_BRUSH", wxCURSOR_PAINT_BRUSH)
		.value("wxCURSOR_PENCIL", wxCURSOR_PENCIL)
		.value("wxCURSOR_POINT_LEFT", wxCURSOR_POINT_LEFT)
		.value("wxCURSOR_POINT_RIGHT", wxCURSOR_POINT_RIGHT)
		.value("wxCURSOR_QUESTION_ARROW", wxCURSOR_QUESTION_ARROW)
		.value("wxCURSOR_RIGHT_BUTTON", wxCURSOR_RIGHT_BUTTON)
		.value("wxCURSOR_SIZENESW", wxCURSOR_SIZENESW)
		.value("wxCURSOR_SIZENS", wxCURSOR_SIZENS)
		.value("wxCURSOR_SIZENWSE", wxCURSOR_SIZENWSE)
		.value("wxCURSOR_SIZEWE", wxCURSOR_SIZEWE)
		.value("wxCURSOR_SIZING", wxCURSOR_SIZING)
		.value("wxCURSOR_SPRAYCAN", wxCURSOR_SPRAYCAN)
		.value("wxCURSOR_WAIT", wxCURSOR_WAIT)
		.value("wxCURSOR_WATCH", wxCURSOR_WATCH)
		.value("wxCURSOR_BLANK", wxCURSOR_BLANK)
		.value("wxCURSOR_ARROWWAIT", wxCURSOR_ARROWWAIT)
		.value("wxCURSOR_MAX", wxCURSOR_MAX)
		.export_values();

;

	{ // wxSize file: line:250
		pybind11::class_<wxSize, std::shared_ptr<wxSize>> cl(M(""), "wxSize", "");
		cl.def( pybind11::init( [](){ return new wxSize(); } ) );
		cl.def( pybind11::init<int, int>(), pybind11::arg("xx"), pybind11::arg("yy") );

		cl.def( pybind11::init( [](wxSize const &o){ return new wxSize(o); } ) );
		cl.def_readwrite("x", &wxSize::x);
		cl.def_readwrite("y", &wxSize::y);
		cl.def("__iadd__", (class wxSize & (wxSize::*)(const class wxSize &)) &wxSize::operator+=, "C++: wxSize::operator+=(const class wxSize &) --> class wxSize &", pybind11::return_value_policy::automatic, pybind11::arg("sz"));
		cl.def("__isub__", (class wxSize & (wxSize::*)(const class wxSize &)) &wxSize::operator-=, "C++: wxSize::operator-=(const class wxSize &) --> class wxSize &", pybind11::return_value_policy::automatic, pybind11::arg("sz"));
		cl.def("__idiv__", (class wxSize & (wxSize::*)(int)) &wxSize::operator/=, "C++: wxSize::operator/=(int) --> class wxSize &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("__imul__", (class wxSize & (wxSize::*)(int)) &wxSize::operator*=, "C++: wxSize::operator*=(int) --> class wxSize &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("__idiv__", (class wxSize & (wxSize::*)(unsigned int)) &wxSize::operator/=, "C++: wxSize::operator/=(unsigned int) --> class wxSize &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("__imul__", (class wxSize & (wxSize::*)(unsigned int)) &wxSize::operator*=, "C++: wxSize::operator*=(unsigned int) --> class wxSize &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("__idiv__", (class wxSize & (wxSize::*)(long)) &wxSize::operator/=, "C++: wxSize::operator/=(long) --> class wxSize &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("__imul__", (class wxSize & (wxSize::*)(long)) &wxSize::operator*=, "C++: wxSize::operator*=(long) --> class wxSize &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("__idiv__", (class wxSize & (wxSize::*)(unsigned long)) &wxSize::operator/=, "C++: wxSize::operator/=(unsigned long) --> class wxSize &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("__imul__", (class wxSize & (wxSize::*)(unsigned long)) &wxSize::operator*=, "C++: wxSize::operator*=(unsigned long) --> class wxSize &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("__idiv__", (class wxSize & (wxSize::*)(double)) &wxSize::operator/=, "C++: wxSize::operator/=(double) --> class wxSize &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("__imul__", (class wxSize & (wxSize::*)(double)) &wxSize::operator*=, "C++: wxSize::operator*=(double) --> class wxSize &", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("IncTo", (void (wxSize::*)(const class wxSize &)) &wxSize::IncTo, "C++: wxSize::IncTo(const class wxSize &) --> void", pybind11::arg("sz"));
		cl.def("DecTo", (void (wxSize::*)(const class wxSize &)) &wxSize::DecTo, "C++: wxSize::DecTo(const class wxSize &) --> void", pybind11::arg("sz"));
		cl.def("DecToIfSpecified", (void (wxSize::*)(const class wxSize &)) &wxSize::DecToIfSpecified, "C++: wxSize::DecToIfSpecified(const class wxSize &) --> void", pybind11::arg("sz"));
		cl.def("IncBy", (void (wxSize::*)(int, int)) &wxSize::IncBy, "C++: wxSize::IncBy(int, int) --> void", pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("IncBy", (void (wxSize::*)(const class wxPoint &)) &wxSize::IncBy, "C++: wxSize::IncBy(const class wxPoint &) --> void", pybind11::arg("pt"));
		cl.def("IncBy", (void (wxSize::*)(const class wxSize &)) &wxSize::IncBy, "C++: wxSize::IncBy(const class wxSize &) --> void", pybind11::arg("sz"));
		cl.def("IncBy", (void (wxSize::*)(int)) &wxSize::IncBy, "C++: wxSize::IncBy(int) --> void", pybind11::arg("d"));
		cl.def("DecBy", (void (wxSize::*)(int, int)) &wxSize::DecBy, "C++: wxSize::DecBy(int, int) --> void", pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("DecBy", (void (wxSize::*)(const class wxPoint &)) &wxSize::DecBy, "C++: wxSize::DecBy(const class wxPoint &) --> void", pybind11::arg("pt"));
		cl.def("DecBy", (void (wxSize::*)(const class wxSize &)) &wxSize::DecBy, "C++: wxSize::DecBy(const class wxSize &) --> void", pybind11::arg("sz"));
		cl.def("DecBy", (void (wxSize::*)(int)) &wxSize::DecBy, "C++: wxSize::DecBy(int) --> void", pybind11::arg("d"));
		cl.def("Scale", (class wxSize & (wxSize::*)(float, float)) &wxSize::Scale, "C++: wxSize::Scale(float, float) --> class wxSize &", pybind11::return_value_policy::automatic, pybind11::arg("xscale"), pybind11::arg("yscale"));
		cl.def("Set", (void (wxSize::*)(int, int)) &wxSize::Set, "C++: wxSize::Set(int, int) --> void", pybind11::arg("xx"), pybind11::arg("yy"));
		cl.def("SetWidth", (void (wxSize::*)(int)) &wxSize::SetWidth, "C++: wxSize::SetWidth(int) --> void", pybind11::arg("w"));
		cl.def("SetHeight", (void (wxSize::*)(int)) &wxSize::SetHeight, "C++: wxSize::SetHeight(int) --> void", pybind11::arg("h"));
		cl.def("GetWidth", (int (wxSize::*)() const) &wxSize::GetWidth, "C++: wxSize::GetWidth() const --> int");
		cl.def("GetHeight", (int (wxSize::*)() const) &wxSize::GetHeight, "C++: wxSize::GetHeight() const --> int");
		cl.def("IsFullySpecified", (bool (wxSize::*)() const) &wxSize::IsFullySpecified, "C++: wxSize::IsFullySpecified() const --> bool");
		cl.def("SetDefaults", (void (wxSize::*)(const class wxSize &)) &wxSize::SetDefaults, "C++: wxSize::SetDefaults(const class wxSize &) --> void", pybind11::arg("size"));
		cl.def("GetX", (int (wxSize::*)() const) &wxSize::GetX, "C++: wxSize::GetX() const --> int");
		cl.def("GetY", (int (wxSize::*)() const) &wxSize::GetY, "C++: wxSize::GetY() const --> int");
		cl.def("assign", (class wxSize & (wxSize::*)(const class wxSize &)) &wxSize::operator=, "C++: wxSize::operator=(const class wxSize &) --> class wxSize &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxRealPoint file: line:422
		pybind11::class_<wxRealPoint, std::shared_ptr<wxRealPoint>> cl(M(""), "wxRealPoint", "");
		cl.def( pybind11::init( [](){ return new wxRealPoint(); } ) );
		cl.def( pybind11::init<double, double>(), pybind11::arg("xx"), pybind11::arg("yy") );

		cl.def( pybind11::init<const class wxPoint &>(), pybind11::arg("pt") );

		cl.def( pybind11::init( [](wxRealPoint const &o){ return new wxRealPoint(o); } ) );
		cl.def_readwrite("x", &wxRealPoint::x);
		cl.def_readwrite("y", &wxRealPoint::y);
		cl.def("__iadd__", (class wxRealPoint & (wxRealPoint::*)(const class wxRealPoint &)) &wxRealPoint::operator+=, "C++: wxRealPoint::operator+=(const class wxRealPoint &) --> class wxRealPoint &", pybind11::return_value_policy::automatic, pybind11::arg("p"));
		cl.def("__isub__", (class wxRealPoint & (wxRealPoint::*)(const class wxRealPoint &)) &wxRealPoint::operator-=, "C++: wxRealPoint::operator-=(const class wxRealPoint &) --> class wxRealPoint &", pybind11::return_value_policy::automatic, pybind11::arg("p"));
		cl.def("__iadd__", (class wxRealPoint & (wxRealPoint::*)(const class wxSize &)) &wxRealPoint::operator+=, "C++: wxRealPoint::operator+=(const class wxSize &) --> class wxRealPoint &", pybind11::return_value_policy::automatic, pybind11::arg("s"));
		cl.def("__isub__", (class wxRealPoint & (wxRealPoint::*)(const class wxSize &)) &wxRealPoint::operator-=, "C++: wxRealPoint::operator-=(const class wxSize &) --> class wxRealPoint &", pybind11::return_value_policy::automatic, pybind11::arg("s"));
	}
	{ // wxPoint file: line:540
		pybind11::class_<wxPoint, std::shared_ptr<wxPoint>> cl(M(""), "wxPoint", "");
		cl.def( pybind11::init( [](){ return new wxPoint(); } ) );
		cl.def( pybind11::init<int, int>(), pybind11::arg("xx"), pybind11::arg("yy") );

		cl.def( pybind11::init<const class wxRealPoint &>(), pybind11::arg("pt") );

		cl.def( pybind11::init( [](wxPoint const &o){ return new wxPoint(o); } ) );
		cl.def_readwrite("x", &wxPoint::x);
		cl.def_readwrite("y", &wxPoint::y);
		cl.def("__iadd__", (class wxPoint & (wxPoint::*)(const class wxPoint &)) &wxPoint::operator+=, "C++: wxPoint::operator+=(const class wxPoint &) --> class wxPoint &", pybind11::return_value_policy::automatic, pybind11::arg("p"));
		cl.def("__isub__", (class wxPoint & (wxPoint::*)(const class wxPoint &)) &wxPoint::operator-=, "C++: wxPoint::operator-=(const class wxPoint &) --> class wxPoint &", pybind11::return_value_policy::automatic, pybind11::arg("p"));
		cl.def("__iadd__", (class wxPoint & (wxPoint::*)(const class wxSize &)) &wxPoint::operator+=, "C++: wxPoint::operator+=(const class wxSize &) --> class wxPoint &", pybind11::return_value_policy::automatic, pybind11::arg("s"));
		cl.def("__isub__", (class wxPoint & (wxPoint::*)(const class wxSize &)) &wxPoint::operator-=, "C++: wxPoint::operator-=(const class wxSize &) --> class wxPoint &", pybind11::return_value_policy::automatic, pybind11::arg("s"));
		cl.def("IsFullySpecified", (bool (wxPoint::*)() const) &wxPoint::IsFullySpecified, "C++: wxPoint::IsFullySpecified() const --> bool");
		cl.def("SetDefaults", (void (wxPoint::*)(const class wxPoint &)) &wxPoint::SetDefaults, "C++: wxPoint::SetDefaults(const class wxPoint &) --> void", pybind11::arg("pt"));
		cl.def("assign", (class wxPoint & (wxPoint::*)(const class wxPoint &)) &wxPoint::operator=, "C++: wxPoint::operator=(const class wxPoint &) --> class wxPoint &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxwxPointListNode file: line:57
		pybind11::class_<wxwxPointListNode, std::shared_ptr<wxwxPointListNode>, PyCallBack_wxwxPointListNode, wxNodeBase> cl(M(""), "wxwxPointListNode", "");
		cl.def( pybind11::init( [](){ return new wxwxPointListNode(); }, [](){ return new PyCallBack_wxwxPointListNode(); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0){ return new wxwxPointListNode(a0); }, [](class wxListBase * a0){ return new PyCallBack_wxwxPointListNode(a0); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxwxPointListNode * a1){ return new wxwxPointListNode(a0, a1); }, [](class wxListBase * a0, class wxwxPointListNode * a1){ return new PyCallBack_wxwxPointListNode(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxwxPointListNode * a1, class wxwxPointListNode * a2){ return new wxwxPointListNode(a0, a1, a2); }, [](class wxListBase * a0, class wxwxPointListNode * a1, class wxwxPointListNode * a2){ return new PyCallBack_wxwxPointListNode(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxwxPointListNode * a1, class wxwxPointListNode * a2, class wxPoint * a3){ return new wxwxPointListNode(a0, a1, a2, a3); }, [](class wxListBase * a0, class wxwxPointListNode * a1, class wxwxPointListNode * a2, class wxPoint * a3){ return new PyCallBack_wxwxPointListNode(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init<class wxListBase *, class wxwxPointListNode *, class wxwxPointListNode *, class wxPoint *, const class wxListKey &>(), pybind11::arg("list"), pybind11::arg("previous"), pybind11::arg("next"), pybind11::arg("data"), pybind11::arg("key") );

		cl.def("GetNext", (class wxwxPointListNode * (wxwxPointListNode::*)() const) &wxwxPointListNode::GetNext, "C++: wxwxPointListNode::GetNext() const --> class wxwxPointListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetPrevious", (class wxwxPointListNode * (wxwxPointListNode::*)() const) &wxwxPointListNode::GetPrevious, "C++: wxwxPointListNode::GetPrevious() const --> class wxwxPointListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetData", (class wxPoint * (wxwxPointListNode::*)() const) &wxwxPointListNode::GetData, "C++: wxwxPointListNode::GetData() const --> class wxPoint *", pybind11::return_value_policy::automatic);
		cl.def("SetData", (void (wxwxPointListNode::*)(class wxPoint *)) &wxwxPointListNode::SetData, "C++: wxwxPointListNode::SetData(class wxPoint *) --> void", pybind11::arg("data"));
	}
}

#include <assert.h> // __assert
#include <assert.h> // __assert_fail
#include <assert.h> // __assert_perror_fail
#include <bits/types/__locale_t.h> // __locale_struct
#include <core/typeinfo.h> // BaseType
#include <core/typeinfo.h> // KICAD_T
#include <cstdio> // _IO_FILE
#include <cwchar> // btowc
#include <cwchar> // fgetwc
#include <cwchar> // fgetwc_unlocked
#include <cwchar> // fgetws
#include <cwchar> // fgetws_unlocked
#include <cwchar> // fputwc
#include <cwchar> // fputwc_unlocked
#include <cwchar> // fputws
#include <cwchar> // fputws_unlocked
#include <cwchar> // fwide
#include <cwchar> // fwprintf
#include <cwchar> // fwscanf
#include <cwchar> // getwc
#include <cwchar> // getwc_unlocked
#include <cwchar> // getwchar
#include <cwchar> // getwchar_unlocked
#include <cwchar> // putwc
#include <cwchar> // putwc_unlocked
#include <cwchar> // putwchar
#include <cwchar> // putwchar_unlocked
#include <cwchar> // swprintf
#include <cwchar> // swscanf
#include <cwchar> // ungetwc
#include <cwchar> // wcpcpy
#include <cwchar> // wcpncpy
#include <cwchar> // wcscasecmp
#include <cwchar> // wcscasecmp_l
#include <cwchar> // wcscat
#include <cwchar> // wcschr
#include <cwchar> // wcschrnul
#include <cwchar> // wcscmp
#include <cwchar> // wcscoll
#include <cwchar> // wcscoll_l
#include <cwchar> // wcscpy
#include <cwchar> // wcscspn
#include <cwchar> // wcsdup
#include <cwchar> // wcsftime
#include <cwchar> // wcsftime_l
#include <cwchar> // wcslen
#include <cwchar> // wcsncasecmp
#include <cwchar> // wcsncasecmp_l
#include <cwchar> // wcsncat
#include <cwchar> // wcsncmp
#include <cwchar> // wcsncpy
#include <cwchar> // wcsnlen
#include <cwchar> // wcspbrk
#include <cwchar> // wcsrchr
#include <cwchar> // wcsspn
#include <cwchar> // wcsstr
#include <cwchar> // wcswcs
#include <cwchar> // wcswidth
#include <cwchar> // wcsxfrm
#include <cwchar> // wcsxfrm_l
#include <cwchar> // wctob
#include <cwchar> // wcwidth
#include <cwchar> // wmemchr
#include <cwchar> // wmemcmp
#include <cwchar> // wmemcpy
#include <cwchar> // wmemmove
#include <cwchar> // wmempcpy
#include <cwchar> // wmemset
#include <cwchar> // wprintf
#include <cwchar> // wscanf
#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <time.h> // tm

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_core_typeinfo(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// KICAD_T file:core/typeinfo.h line:77
	pybind11::enum_<KICAD_T>(M(""), "KICAD_T", pybind11::arithmetic(), "The set of class identification values stored in #EDA_ITEM::m_structType")
		.value("NOT_USED", NOT_USED)
		.value("EOT", EOT)
		.value("TYPE_NOT_INIT", TYPE_NOT_INIT)
		.value("PCB_T", PCB_T)
		.value("SCREEN_T", SCREEN_T)
		.value("PCB_FOOTPRINT_T", PCB_FOOTPRINT_T)
		.value("PCB_PAD_T", PCB_PAD_T)
		.value("PCB_SHAPE_T", PCB_SHAPE_T)
		.value("PCB_TEXT_T", PCB_TEXT_T)
		.value("PCB_FP_TEXT_T", PCB_FP_TEXT_T)
		.value("PCB_FP_SHAPE_T", PCB_FP_SHAPE_T)
		.value("PCB_FP_ZONE_T", PCB_FP_ZONE_T)
		.value("PCB_TRACE_T", PCB_TRACE_T)
		.value("PCB_VIA_T", PCB_VIA_T)
		.value("PCB_ARC_T", PCB_ARC_T)
		.value("PCB_MARKER_T", PCB_MARKER_T)
		.value("PCB_DIMENSION_T", PCB_DIMENSION_T)
		.value("PCB_DIM_ALIGNED_T", PCB_DIM_ALIGNED_T)
		.value("PCB_DIM_LEADER_T", PCB_DIM_LEADER_T)
		.value("PCB_DIM_CENTER_T", PCB_DIM_CENTER_T)
		.value("PCB_DIM_ORTHOGONAL_T", PCB_DIM_ORTHOGONAL_T)
		.value("PCB_TARGET_T", PCB_TARGET_T)
		.value("PCB_ZONE_T", PCB_ZONE_T)
		.value("PCB_ITEM_LIST_T", PCB_ITEM_LIST_T)
		.value("PCB_NETINFO_T", PCB_NETINFO_T)
		.value("PCB_GROUP_T", PCB_GROUP_T)
		.value("PCB_LOCATE_STDVIA_T", PCB_LOCATE_STDVIA_T)
		.value("PCB_LOCATE_UVIA_T", PCB_LOCATE_UVIA_T)
		.value("PCB_LOCATE_BBVIA_T", PCB_LOCATE_BBVIA_T)
		.value("PCB_LOCATE_TEXT_T", PCB_LOCATE_TEXT_T)
		.value("PCB_LOCATE_GRAPHIC_T", PCB_LOCATE_GRAPHIC_T)
		.value("PCB_LOCATE_HOLE_T", PCB_LOCATE_HOLE_T)
		.value("PCB_LOCATE_PTH_T", PCB_LOCATE_PTH_T)
		.value("PCB_LOCATE_NPTH_T", PCB_LOCATE_NPTH_T)
		.value("PCB_LOCATE_BOARD_EDGE_T", PCB_LOCATE_BOARD_EDGE_T)
		.value("SCH_MARKER_T", SCH_MARKER_T)
		.value("SCH_JUNCTION_T", SCH_JUNCTION_T)
		.value("SCH_NO_CONNECT_T", SCH_NO_CONNECT_T)
		.value("SCH_BUS_WIRE_ENTRY_T", SCH_BUS_WIRE_ENTRY_T)
		.value("SCH_BUS_BUS_ENTRY_T", SCH_BUS_BUS_ENTRY_T)
		.value("SCH_LINE_T", SCH_LINE_T)
		.value("SCH_BITMAP_T", SCH_BITMAP_T)
		.value("SCH_TEXT_T", SCH_TEXT_T)
		.value("SCH_LABEL_T", SCH_LABEL_T)
		.value("SCH_GLOBAL_LABEL_T", SCH_GLOBAL_LABEL_T)
		.value("SCH_HIER_LABEL_T", SCH_HIER_LABEL_T)
		.value("SCH_FIELD_T", SCH_FIELD_T)
		.value("SCH_COMPONENT_T", SCH_COMPONENT_T)
		.value("SCH_SHEET_PIN_T", SCH_SHEET_PIN_T)
		.value("SCH_SHEET_T", SCH_SHEET_T)
		.value("SCH_PIN_T", SCH_PIN_T)
		.value("SCH_FIELD_LOCATE_REFERENCE_T", SCH_FIELD_LOCATE_REFERENCE_T)
		.value("SCH_FIELD_LOCATE_VALUE_T", SCH_FIELD_LOCATE_VALUE_T)
		.value("SCH_FIELD_LOCATE_FOOTPRINT_T", SCH_FIELD_LOCATE_FOOTPRINT_T)
		.value("SCH_FIELD_LOCATE_DATASHEET_T", SCH_FIELD_LOCATE_DATASHEET_T)
		.value("SCH_LINE_LOCATE_WIRE_T", SCH_LINE_LOCATE_WIRE_T)
		.value("SCH_LINE_LOCATE_BUS_T", SCH_LINE_LOCATE_BUS_T)
		.value("SCH_LINE_LOCATE_GRAPHIC_LINE_T", SCH_LINE_LOCATE_GRAPHIC_LINE_T)
		.value("SCH_LABEL_LOCATE_WIRE_T", SCH_LABEL_LOCATE_WIRE_T)
		.value("SCH_LABEL_LOCATE_BUS_T", SCH_LABEL_LOCATE_BUS_T)
		.value("SCH_COMPONENT_LOCATE_POWER_T", SCH_COMPONENT_LOCATE_POWER_T)
		.value("SCH_LOCATE_ANY_T", SCH_LOCATE_ANY_T)
		.value("SCH_SCREEN_T", SCH_SCREEN_T)
		.value("SCHEMATIC_T", SCHEMATIC_T)
		.value("LIB_PART_T", LIB_PART_T)
		.value("LIB_ALIAS_T", LIB_ALIAS_T)
		.value("LIB_ARC_T", LIB_ARC_T)
		.value("LIB_CIRCLE_T", LIB_CIRCLE_T)
		.value("LIB_TEXT_T", LIB_TEXT_T)
		.value("LIB_RECTANGLE_T", LIB_RECTANGLE_T)
		.value("LIB_POLYLINE_T", LIB_POLYLINE_T)
		.value("LIB_BEZIER_T", LIB_BEZIER_T)
		.value("LIB_PIN_T", LIB_PIN_T)
		.value("LIB_FIELD_T", LIB_FIELD_T)
		.value("GERBER_LAYOUT_T", GERBER_LAYOUT_T)
		.value("GERBER_DRAW_ITEM_T", GERBER_DRAW_ITEM_T)
		.value("GERBER_IMAGE_T", GERBER_IMAGE_T)
		.value("WSG_LINE_T", WSG_LINE_T)
		.value("WSG_RECT_T", WSG_RECT_T)
		.value("WSG_POLY_T", WSG_POLY_T)
		.value("WSG_TEXT_T", WSG_TEXT_T)
		.value("WSG_BITMAP_T", WSG_BITMAP_T)
		.value("WSG_PAGE_T", WSG_PAGE_T)
		.value("WS_PROXY_UNDO_ITEM_T", WS_PROXY_UNDO_ITEM_T)
		.value("WS_PROXY_UNDO_ITEM_PLUS_T", WS_PROXY_UNDO_ITEM_PLUS_T)
		.value("SYMBOL_LIB_TABLE_T", SYMBOL_LIB_TABLE_T)
		.value("FP_LIB_TABLE_T", FP_LIB_TABLE_T)
		.value("PART_LIBS_T", PART_LIBS_T)
		.value("SEARCH_STACK_T", SEARCH_STACK_T)
		.value("S3D_CACHE_T", S3D_CACHE_T)
		.value("MAX_STRUCT_TYPE_ID", MAX_STRUCT_TYPE_ID)
		.export_values();

;

	// BaseType(const enum KICAD_T) file:core/typeinfo.h line:235
	M("").def("BaseType", (enum KICAD_T (*)(const enum KICAD_T)) &BaseType, "Returns the underlying type of the given type.\n\n This is useful for finding the element type given one of the \"non-type\" types such as\n SCH_LINE_LOCATE_WIRE_T.\n\n \n Given type to resolve.\n \n\n Base type.\n\nC++: BaseType(const enum KICAD_T) --> enum KICAD_T", pybind11::arg("aType"));

	// wxSetDefaultAssertHandler() file: line:127
	M("").def("wxSetDefaultAssertHandler", (void (*)()) &wxSetDefaultAssertHandler, "C++: wxSetDefaultAssertHandler() --> void");

	// wxDisableAsserts() file: line:145
	M("").def("wxDisableAsserts", (void (*)()) &wxDisableAsserts, "C++: wxDisableAsserts() --> void");

	// wxOnAssert(const char *, int, const char *, const char *) file: line:180
	M("").def("wxOnAssert", (void (*)(const char *, int, const char *, const char *)) &wxOnAssert, "C++: wxOnAssert(const char *, int, const char *, const char *) --> void", pybind11::arg("file"), pybind11::arg("line"), pybind11::arg("func"), pybind11::arg("cond"));

	// wxOnAssert(const char *, int, const char *, const char *, const char *) file: line:185
	M("").def("wxOnAssert", (void (*)(const char *, int, const char *, const char *, const char *)) &wxOnAssert, "C++: wxOnAssert(const char *, int, const char *, const char *, const char *) --> void", pybind11::arg("file"), pybind11::arg("line"), pybind11::arg("func"), pybind11::arg("cond"), pybind11::arg("msg"));

	// wxOnAssert(const char *, int, const char *, const char *, const wchar_t *) file: line:191
	M("").def("wxOnAssert", (void (*)(const char *, int, const char *, const char *, const wchar_t *)) &wxOnAssert, "C++: wxOnAssert(const char *, int, const char *, const char *, const wchar_t *) --> void", pybind11::arg("file"), pybind11::arg("line"), pybind11::arg("func"), pybind11::arg("cond"), pybind11::arg("msg"));

	// wxOnAssert(const wchar_t *, int, const char *, const wchar_t *, const wchar_t *) file: line:201
	M("").def("wxOnAssert", [](const wchar_t * a0, int const & a1, const char * a2, const wchar_t * a3) -> void { return wxOnAssert(a0, a1, a2, a3); }, "", pybind11::arg("file"), pybind11::arg("line"), pybind11::arg("func"), pybind11::arg("cond"));
	M("").def("wxOnAssert", (void (*)(const wchar_t *, int, const char *, const wchar_t *, const wchar_t *)) &wxOnAssert, "C++: wxOnAssert(const wchar_t *, int, const char *, const wchar_t *, const wchar_t *) --> void", pybind11::arg("file"), pybind11::arg("line"), pybind11::arg("func"), pybind11::arg("cond"), pybind11::arg("msg"));

	// wxOnAssert(const class wxString &, int, const class wxString &, const class wxString &, const class wxString &) file: line:210
	M("").def("wxOnAssert", (void (*)(const class wxString &, int, const class wxString &, const class wxString &, const class wxString &)) &wxOnAssert, "C++: wxOnAssert(const class wxString &, int, const class wxString &, const class wxString &, const class wxString &) --> void", pybind11::arg("file"), pybind11::arg("line"), pybind11::arg("func"), pybind11::arg("cond"), pybind11::arg("msg"));

	// wxOnAssert(const class wxString &, int, const class wxString &, const class wxString &) file: line:216
	M("").def("wxOnAssert", (void (*)(const class wxString &, int, const class wxString &, const class wxString &)) &wxOnAssert, "C++: wxOnAssert(const class wxString &, int, const class wxString &, const class wxString &) --> void", pybind11::arg("file"), pybind11::arg("line"), pybind11::arg("func"), pybind11::arg("cond"));

	// wxOnAssert(const char *, int, const char *, const char *, const class wxCStrData &) file: line:221
	M("").def("wxOnAssert", (void (*)(const char *, int, const char *, const char *, const class wxCStrData &)) &wxOnAssert, "C++: wxOnAssert(const char *, int, const char *, const char *, const class wxCStrData &) --> void", pybind11::arg("file"), pybind11::arg("line"), pybind11::arg("func"), pybind11::arg("cond"), pybind11::arg("msg"));

	// wxOnAssert(const char *, int, const char *, const char *, const class wxString &) file: line:227
	M("").def("wxOnAssert", (void (*)(const char *, int, const char *, const char *, const class wxString &)) &wxOnAssert, "C++: wxOnAssert(const char *, int, const char *, const char *, const class wxString &) --> void", pybind11::arg("file"), pybind11::arg("line"), pybind11::arg("func"), pybind11::arg("cond"), pybind11::arg("msg"));

	// wxTrap() file: line:262
	M("").def("wxTrap", (void (*)()) &wxTrap, "C++: wxTrap() --> void");

	// wxAbort() file: line:332
	M("").def("wxAbort", (void (*)()) &wxAbort, "C++: wxAbort() --> void");

	// wxIsDebuggerRunning() file: line:469
	M("").def("wxIsDebuggerRunning", (bool (*)()) &wxIsDebuggerRunning, "C++: wxIsDebuggerRunning() --> bool");

	// wxAssertIsEqual(int, int) file: line:479
	M("").def("wxAssertIsEqual", (bool (*)(int, int)) &wxAssertIsEqual, "C++: wxAssertIsEqual(int, int) --> bool", pybind11::arg("x"), pybind11::arg("y"));

	// wxSwap(unsigned long &, unsigned long &) file: line:775
	M("").def("wxSwap", (void (*)(unsigned long &, unsigned long &)) &wxSwap<unsigned long>, "C++: wxSwap(unsigned long &, unsigned long &) --> void", pybind11::arg("first"), pybind11::arg("second"));

	// wxSwap(class wxString *&, class wxString *&) file: line:775
	M("").def("wxSwap", (void (*)(class wxString *&, class wxString *&)) &wxSwap<wxString *>, "C++: wxSwap(class wxString *&, class wxString *&) --> void", pybind11::arg("first"), pybind11::arg("second"));

	// wxSwap(bool &, bool &) file: line:775
	M("").def("wxSwap", (void (*)(bool &, bool &)) &wxSwap<bool>, "C++: wxSwap(bool &, bool &) --> void", pybind11::arg("first"), pybind11::arg("second"));

	// wxSwap(char *&, char *&) file: line:775
	M("").def("wxSwap", (void (*)(char *&, char *&)) &wxSwap<char *>, "C++: wxSwap(char *&, char *&) --> void", pybind11::arg("first"), pybind11::arg("second"));

	// wxSwap(short *&, short *&) file: line:775
	M("").def("wxSwap", (void (*)(short *&, short *&)) &wxSwap<short *>, "C++: wxSwap(short *&, short *&) --> void", pybind11::arg("first"), pybind11::arg("second"));

	// wxSwap(int *&, int *&) file: line:775
	M("").def("wxSwap", (void (*)(int *&, int *&)) &wxSwap<int *>, "C++: wxSwap(int *&, int *&) --> void", pybind11::arg("first"), pybind11::arg("second"));

	// wxSwap(long *&, long *&) file: line:775
	M("").def("wxSwap", (void (*)(long *&, long *&)) &wxSwap<long *>, "C++: wxSwap(long *&, long *&) --> void", pybind11::arg("first"), pybind11::arg("second"));

	// wxSwap(unsigned long *&, unsigned long *&) file: line:775
	M("").def("wxSwap", (void (*)(unsigned long *&, unsigned long *&)) &wxSwap<unsigned long *>, "C++: wxSwap(unsigned long *&, unsigned long *&) --> void", pybind11::arg("first"), pybind11::arg("second"));

	// wxSwap(double *&, double *&) file: line:775
	M("").def("wxSwap", (void (*)(double *&, double *&)) &wxSwap<double *>, "C++: wxSwap(double *&, double *&) --> void", pybind11::arg("first"), pybind11::arg("second"));

	// wxUnusedVar(const int &) file: line:912
	M("").def("wxUnusedVar", (void (*)(const int &)) &wxUnusedVar<int>, "C++: wxUnusedVar(const int &) --> void", pybind11::arg(""));

	// wxUnusedVar(const union wxAnyValueBuffer &) file: line:912
	M("").def("wxUnusedVar", (void (*)(const union wxAnyValueBuffer &)) &wxUnusedVar<wxAnyValueBuffer>, "C++: wxUnusedVar(const union wxAnyValueBuffer &) --> void", pybind11::arg(""));

	// wxUnusedVar(class wxAnyValueType *const &) file: line:912
	M("").def("wxUnusedVar", (void (*)(class wxAnyValueType *const &)) &wxUnusedVar<wxAnyValueType *>, "C++: wxUnusedVar(class wxAnyValueType *const &) --> void", pybind11::arg(""));

	// wxUnusedVar(const bool &) file: line:912
	M("").def("wxUnusedVar", (void (*)(const bool &)) &wxUnusedVar<bool>, "C++: wxUnusedVar(const bool &) --> void", pybind11::arg(""));

}

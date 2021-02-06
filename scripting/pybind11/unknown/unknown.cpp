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

void bind_unknown_unknown(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxPtrToUInt(const void *) file: line:1324
	M("").def("wxPtrToUInt", (unsigned long (*)(const void *)) &wxPtrToUInt, "C++: wxPtrToUInt(const void *) --> unsigned long", pybind11::arg("p"));

	// wxUIntToPtr(unsigned long) file: line:1350
	M("").def("wxUIntToPtr", (void * (*)(unsigned long)) &wxUIntToPtr, "C++: wxUIntToPtr(unsigned long) --> void *", pybind11::return_value_policy::automatic, pybind11::arg("p"));

	// wxGeometryCentre file: line:1663
	pybind11::enum_<wxGeometryCentre>(M(""), "wxGeometryCentre", pybind11::arithmetic(), "")
		.value("wxCENTRE", wxCENTRE)
		.value("wxCENTER", wxCENTER)
		.export_values();

;

	// wxOrientation file: line:1675
	pybind11::enum_<wxOrientation>(M(""), "wxOrientation", pybind11::arithmetic(), "")
		.value("wxHORIZONTAL", wxHORIZONTAL)
		.value("wxVERTICAL", wxVERTICAL)
		.value("wxBOTH", wxBOTH)
		.value("wxORIENTATION_MASK", wxORIENTATION_MASK)
		.export_values();

;

	// wxDirection file: line:1687
	pybind11::enum_<wxDirection>(M(""), "wxDirection", pybind11::arithmetic(), "")
		.value("wxLEFT", wxLEFT)
		.value("wxRIGHT", wxRIGHT)
		.value("wxUP", wxUP)
		.value("wxDOWN", wxDOWN)
		.value("wxTOP", wxTOP)
		.value("wxBOTTOM", wxBOTTOM)
		.value("wxNORTH", wxNORTH)
		.value("wxSOUTH", wxSOUTH)
		.value("wxWEST", wxWEST)
		.value("wxEAST", wxEAST)
		.value("wxALL", wxALL)
		.value("wxDIRECTION_MASK", wxDIRECTION_MASK)
		.export_values();

;

	// wxAlignment file: line:1708
	pybind11::enum_<wxAlignment>(M(""), "wxAlignment", pybind11::arithmetic(), "")
		.value("wxALIGN_INVALID", wxALIGN_INVALID)
		.value("wxALIGN_NOT", wxALIGN_NOT)
		.value("wxALIGN_CENTER_HORIZONTAL", wxALIGN_CENTER_HORIZONTAL)
		.value("wxALIGN_CENTRE_HORIZONTAL", wxALIGN_CENTRE_HORIZONTAL)
		.value("wxALIGN_LEFT", wxALIGN_LEFT)
		.value("wxALIGN_TOP", wxALIGN_TOP)
		.value("wxALIGN_RIGHT", wxALIGN_RIGHT)
		.value("wxALIGN_BOTTOM", wxALIGN_BOTTOM)
		.value("wxALIGN_CENTER_VERTICAL", wxALIGN_CENTER_VERTICAL)
		.value("wxALIGN_CENTRE_VERTICAL", wxALIGN_CENTRE_VERTICAL)
		.value("wxALIGN_CENTER", wxALIGN_CENTER)
		.value("wxALIGN_CENTRE", wxALIGN_CENTRE)
		.value("wxALIGN_MASK", wxALIGN_MASK)
		.export_values();

;

	// wxSizerFlagBits file: line:1735
	pybind11::enum_<wxSizerFlagBits>(M(""), "wxSizerFlagBits", pybind11::arithmetic(), "")
		.value("wxADJUST_MINSIZE", wxADJUST_MINSIZE)
		.value("wxFIXED_MINSIZE", wxFIXED_MINSIZE)
		.value("wxRESERVE_SPACE_EVEN_IF_HIDDEN", wxRESERVE_SPACE_EVEN_IF_HIDDEN)
		.value("wxSIZER_FLAG_BITS_MASK", wxSIZER_FLAG_BITS_MASK)
		.export_values();

;

	// wxStretch file: line:1753
	pybind11::enum_<wxStretch>(M(""), "wxStretch", pybind11::arithmetic(), "")
		.value("wxSTRETCH_NOT", wxSTRETCH_NOT)
		.value("wxSHRINK", wxSHRINK)
		.value("wxGROW", wxGROW)
		.value("wxEXPAND", wxEXPAND)
		.value("wxSHAPED", wxSHAPED)
		.value("wxTILE", wxTILE)
		.value("wxSTRETCH_MASK", wxSTRETCH_MASK)
		.export_values();

;

	// wxBorder file: line:1767
	pybind11::enum_<wxBorder>(M(""), "wxBorder", pybind11::arithmetic(), "")
		.value("wxBORDER_DEFAULT", wxBORDER_DEFAULT)
		.value("wxBORDER_NONE", wxBORDER_NONE)
		.value("wxBORDER_STATIC", wxBORDER_STATIC)
		.value("wxBORDER_SIMPLE", wxBORDER_SIMPLE)
		.value("wxBORDER_RAISED", wxBORDER_RAISED)
		.value("wxBORDER_SUNKEN", wxBORDER_SUNKEN)
		.value("wxBORDER_DOUBLE", wxBORDER_DOUBLE)
		.value("wxBORDER_THEME", wxBORDER_THEME)
		.value("wxBORDER_MASK", wxBORDER_MASK)
		.export_values();

;

	// wxBackgroundStyle file: line:2129
	pybind11::enum_<wxBackgroundStyle>(M(""), "wxBackgroundStyle", pybind11::arithmetic(), "")
		.value("wxBG_STYLE_ERASE", wxBG_STYLE_ERASE)
		.value("wxBG_STYLE_SYSTEM", wxBG_STYLE_SYSTEM)
		.value("wxBG_STYLE_PAINT", wxBG_STYLE_PAINT)
		.value("wxBG_STYLE_TRANSPARENT", wxBG_STYLE_TRANSPARENT)
		.value("wxBG_STYLE_COLOUR", wxBG_STYLE_COLOUR)
		.value("wxBG_STYLE_CUSTOM", wxBG_STYLE_CUSTOM)
		.export_values();

;

	// wxKeyType file: line:2170
	pybind11::enum_<wxKeyType>(M(""), "wxKeyType", pybind11::arithmetic(), "")
		.value("wxKEY_NONE", wxKEY_NONE)
		.value("wxKEY_INTEGER", wxKEY_INTEGER)
		.value("wxKEY_STRING", wxKEY_STRING)
		.export_values();

;

	// wxStandardID file: line:2182
	pybind11::enum_<wxStandardID>(M(""), "wxStandardID", pybind11::arithmetic(), "")
		.value("wxID_AUTO_LOWEST", wxID_AUTO_LOWEST)
		.value("wxID_AUTO_HIGHEST", wxID_AUTO_HIGHEST)
		.value("wxID_NONE", wxID_NONE)
		.value("wxID_SEPARATOR", wxID_SEPARATOR)
		.value("wxID_ANY", wxID_ANY)
		.value("wxID_LOWEST", wxID_LOWEST)
		.value("wxID_OPEN", wxID_OPEN)
		.value("wxID_CLOSE", wxID_CLOSE)
		.value("wxID_NEW", wxID_NEW)
		.value("wxID_SAVE", wxID_SAVE)
		.value("wxID_SAVEAS", wxID_SAVEAS)
		.value("wxID_REVERT", wxID_REVERT)
		.value("wxID_EXIT", wxID_EXIT)
		.value("wxID_UNDO", wxID_UNDO)
		.value("wxID_REDO", wxID_REDO)
		.value("wxID_HELP", wxID_HELP)
		.value("wxID_PRINT", wxID_PRINT)
		.value("wxID_PRINT_SETUP", wxID_PRINT_SETUP)
		.value("wxID_PAGE_SETUP", wxID_PAGE_SETUP)
		.value("wxID_PREVIEW", wxID_PREVIEW)
		.value("wxID_ABOUT", wxID_ABOUT)
		.value("wxID_HELP_CONTENTS", wxID_HELP_CONTENTS)
		.value("wxID_HELP_INDEX", wxID_HELP_INDEX)
		.value("wxID_HELP_SEARCH", wxID_HELP_SEARCH)
		.value("wxID_HELP_COMMANDS", wxID_HELP_COMMANDS)
		.value("wxID_HELP_PROCEDURES", wxID_HELP_PROCEDURES)
		.value("wxID_HELP_CONTEXT", wxID_HELP_CONTEXT)
		.value("wxID_CLOSE_ALL", wxID_CLOSE_ALL)
		.value("wxID_PREFERENCES", wxID_PREFERENCES)
		.value("wxID_EDIT", wxID_EDIT)
		.value("wxID_CUT", wxID_CUT)
		.value("wxID_COPY", wxID_COPY)
		.value("wxID_PASTE", wxID_PASTE)
		.value("wxID_CLEAR", wxID_CLEAR)
		.value("wxID_FIND", wxID_FIND)
		.value("wxID_DUPLICATE", wxID_DUPLICATE)
		.value("wxID_SELECTALL", wxID_SELECTALL)
		.value("wxID_DELETE", wxID_DELETE)
		.value("wxID_REPLACE", wxID_REPLACE)
		.value("wxID_REPLACE_ALL", wxID_REPLACE_ALL)
		.value("wxID_PROPERTIES", wxID_PROPERTIES)
		.value("wxID_VIEW_DETAILS", wxID_VIEW_DETAILS)
		.value("wxID_VIEW_LARGEICONS", wxID_VIEW_LARGEICONS)
		.value("wxID_VIEW_SMALLICONS", wxID_VIEW_SMALLICONS)
		.value("wxID_VIEW_LIST", wxID_VIEW_LIST)
		.value("wxID_VIEW_SORTDATE", wxID_VIEW_SORTDATE)
		.value("wxID_VIEW_SORTNAME", wxID_VIEW_SORTNAME)
		.value("wxID_VIEW_SORTSIZE", wxID_VIEW_SORTSIZE)
		.value("wxID_VIEW_SORTTYPE", wxID_VIEW_SORTTYPE)
		.value("wxID_FILE", wxID_FILE)
		.value("wxID_FILE1", wxID_FILE1)
		.value("wxID_FILE2", wxID_FILE2)
		.value("wxID_FILE3", wxID_FILE3)
		.value("wxID_FILE4", wxID_FILE4)
		.value("wxID_FILE5", wxID_FILE5)
		.value("wxID_FILE6", wxID_FILE6)
		.value("wxID_FILE7", wxID_FILE7)
		.value("wxID_FILE8", wxID_FILE8)
		.value("wxID_FILE9", wxID_FILE9)
		.value("wxID_OK", wxID_OK)
		.value("wxID_CANCEL", wxID_CANCEL)
		.value("wxID_APPLY", wxID_APPLY)
		.value("wxID_YES", wxID_YES)
		.value("wxID_NO", wxID_NO)
		.value("wxID_STATIC", wxID_STATIC)
		.value("wxID_FORWARD", wxID_FORWARD)
		.value("wxID_BACKWARD", wxID_BACKWARD)
		.value("wxID_DEFAULT", wxID_DEFAULT)
		.value("wxID_MORE", wxID_MORE)
		.value("wxID_SETUP", wxID_SETUP)
		.value("wxID_RESET", wxID_RESET)
		.value("wxID_CONTEXT_HELP", wxID_CONTEXT_HELP)
		.value("wxID_YESTOALL", wxID_YESTOALL)
		.value("wxID_NOTOALL", wxID_NOTOALL)
		.value("wxID_ABORT", wxID_ABORT)
		.value("wxID_RETRY", wxID_RETRY)
		.value("wxID_IGNORE", wxID_IGNORE)
		.value("wxID_ADD", wxID_ADD)
		.value("wxID_REMOVE", wxID_REMOVE)
		.value("wxID_UP", wxID_UP)
		.value("wxID_DOWN", wxID_DOWN)
		.value("wxID_HOME", wxID_HOME)
		.value("wxID_REFRESH", wxID_REFRESH)
		.value("wxID_STOP", wxID_STOP)
		.value("wxID_INDEX", wxID_INDEX)
		.value("wxID_BOLD", wxID_BOLD)
		.value("wxID_ITALIC", wxID_ITALIC)
		.value("wxID_JUSTIFY_CENTER", wxID_JUSTIFY_CENTER)
		.value("wxID_JUSTIFY_FILL", wxID_JUSTIFY_FILL)
		.value("wxID_JUSTIFY_RIGHT", wxID_JUSTIFY_RIGHT)
		.value("wxID_JUSTIFY_LEFT", wxID_JUSTIFY_LEFT)
		.value("wxID_UNDERLINE", wxID_UNDERLINE)
		.value("wxID_INDENT", wxID_INDENT)
		.value("wxID_UNINDENT", wxID_UNINDENT)
		.value("wxID_ZOOM_100", wxID_ZOOM_100)
		.value("wxID_ZOOM_FIT", wxID_ZOOM_FIT)
		.value("wxID_ZOOM_IN", wxID_ZOOM_IN)
		.value("wxID_ZOOM_OUT", wxID_ZOOM_OUT)
		.value("wxID_UNDELETE", wxID_UNDELETE)
		.value("wxID_REVERT_TO_SAVED", wxID_REVERT_TO_SAVED)
		.value("wxID_CDROM", wxID_CDROM)
		.value("wxID_CONVERT", wxID_CONVERT)
		.value("wxID_EXECUTE", wxID_EXECUTE)
		.value("wxID_FLOPPY", wxID_FLOPPY)
		.value("wxID_HARDDISK", wxID_HARDDISK)
		.value("wxID_BOTTOM", wxID_BOTTOM)
		.value("wxID_FIRST", wxID_FIRST)
		.value("wxID_LAST", wxID_LAST)
		.value("wxID_TOP", wxID_TOP)
		.value("wxID_INFO", wxID_INFO)
		.value("wxID_JUMP_TO", wxID_JUMP_TO)
		.value("wxID_NETWORK", wxID_NETWORK)
		.value("wxID_SELECT_COLOR", wxID_SELECT_COLOR)
		.value("wxID_SELECT_FONT", wxID_SELECT_FONT)
		.value("wxID_SORT_ASCENDING", wxID_SORT_ASCENDING)
		.value("wxID_SORT_DESCENDING", wxID_SORT_DESCENDING)
		.value("wxID_SPELL_CHECK", wxID_SPELL_CHECK)
		.value("wxID_STRIKETHROUGH", wxID_STRIKETHROUGH)
		.value("wxID_SYSTEM_MENU", wxID_SYSTEM_MENU)
		.value("wxID_CLOSE_FRAME", wxID_CLOSE_FRAME)
		.value("wxID_MOVE_FRAME", wxID_MOVE_FRAME)
		.value("wxID_RESIZE_FRAME", wxID_RESIZE_FRAME)
		.value("wxID_MAXIMIZE_FRAME", wxID_MAXIMIZE_FRAME)
		.value("wxID_ICONIZE_FRAME", wxID_ICONIZE_FRAME)
		.value("wxID_RESTORE_FRAME", wxID_RESTORE_FRAME)
		.value("wxID_MDI_WINDOW_FIRST", wxID_MDI_WINDOW_FIRST)
		.value("wxID_MDI_WINDOW_CASCADE", wxID_MDI_WINDOW_CASCADE)
		.value("wxID_MDI_WINDOW_TILE_HORZ", wxID_MDI_WINDOW_TILE_HORZ)
		.value("wxID_MDI_WINDOW_TILE_VERT", wxID_MDI_WINDOW_TILE_VERT)
		.value("wxID_MDI_WINDOW_ARRANGE_ICONS", wxID_MDI_WINDOW_ARRANGE_ICONS)
		.value("wxID_MDI_WINDOW_PREV", wxID_MDI_WINDOW_PREV)
		.value("wxID_MDI_WINDOW_NEXT", wxID_MDI_WINDOW_NEXT)
		.value("wxID_MDI_WINDOW_LAST", wxID_MDI_WINDOW_LAST)
		.value("wxID_OSX_MENU_FIRST", wxID_OSX_MENU_FIRST)
		.value("wxID_OSX_HIDE", wxID_OSX_HIDE)
		.value("wxID_OSX_HIDEOTHERS", wxID_OSX_HIDEOTHERS)
		.value("wxID_OSX_SHOWALL", wxID_OSX_SHOWALL)
		.value("wxID_OSX_SERVICES", wxID_OSX_SERVICES)
		.value("wxID_OSX_MENU_LAST", wxID_OSX_MENU_LAST)
		.value("wxID_FILEDLGG", wxID_FILEDLGG)
		.value("wxID_FILECTRL", wxID_FILECTRL)
		.value("wxID_HIGHEST", wxID_HIGHEST)
		.export_values();

;

	{ // wxWindowIDRef file: line:28
		pybind11::class_<wxWindowIDRef, std::shared_ptr<wxWindowIDRef>> cl(M(""), "wxWindowIDRef", "");
		cl.def( pybind11::init( [](){ return new wxWindowIDRef(); } ) );
		cl.def( pybind11::init<int>(), pybind11::arg("id") );

		cl.def( pybind11::init<long>(), pybind11::arg("id") );

		cl.def( pybind11::init( [](wxWindowIDRef const &o){ return new wxWindowIDRef(o); } ) );
		cl.def("assign", (class wxWindowIDRef & (wxWindowIDRef::*)(int)) &wxWindowIDRef::operator=, "C++: wxWindowIDRef::operator=(int) --> class wxWindowIDRef &", pybind11::return_value_policy::automatic, pybind11::arg("id"));
		cl.def("assign", (class wxWindowIDRef & (wxWindowIDRef::*)(long)) &wxWindowIDRef::operator=, "C++: wxWindowIDRef::operator=(long) --> class wxWindowIDRef &", pybind11::return_value_policy::automatic, pybind11::arg("id"));
		cl.def("assign", (class wxWindowIDRef & (wxWindowIDRef::*)(const class wxWindowIDRef &)) &wxWindowIDRef::operator=, "C++: wxWindowIDRef::operator=(const class wxWindowIDRef &) --> class wxWindowIDRef &", pybind11::return_value_policy::automatic, pybind11::arg("id"));
		cl.def("GetValue", (int (wxWindowIDRef::*)() const) &wxWindowIDRef::GetValue, "C++: wxWindowIDRef::GetValue() const --> int");
	}
	{ // wxIdManager file: line:174
		pybind11::class_<wxIdManager, std::shared_ptr<wxIdManager>> cl(M(""), "wxIdManager", "");
		cl.def( pybind11::init( [](){ return new wxIdManager(); } ) );
		cl.def_static("ReserveId", []() -> int { return wxIdManager::ReserveId(); }, "");
		cl.def_static("ReserveId", (int (*)(int)) &wxIdManager::ReserveId, "C++: wxIdManager::ReserveId(int) --> int", pybind11::arg("count"));
		cl.def_static("UnreserveId", [](int const & a0) -> void { return wxIdManager::UnreserveId(a0); }, "", pybind11::arg("id"));
		cl.def_static("UnreserveId", (void (*)(int, int)) &wxIdManager::UnreserveId, "C++: wxIdManager::UnreserveId(int, int) --> void", pybind11::arg("id"), pybind11::arg("count"));
	}
	// wxItemKind file: line:2394
	pybind11::enum_<wxItemKind>(M(""), "wxItemKind", pybind11::arithmetic(), "")
		.value("wxITEM_SEPARATOR", wxITEM_SEPARATOR)
		.value("wxITEM_NORMAL", wxITEM_NORMAL)
		.value("wxITEM_CHECK", wxITEM_CHECK)
		.value("wxITEM_RADIO", wxITEM_RADIO)
		.value("wxITEM_DROPDOWN", wxITEM_DROPDOWN)
		.value("wxITEM_MAX", wxITEM_MAX)
		.export_values();

;

	// wxCheckBoxState file: line:2408
	pybind11::enum_<wxCheckBoxState>(M(""), "wxCheckBoxState", pybind11::arithmetic(), "")
		.value("wxCHK_UNCHECKED", wxCHK_UNCHECKED)
		.value("wxCHK_CHECKED", wxCHK_CHECKED)
		.value("wxCHK_UNDETERMINED", wxCHK_UNDETERMINED)
		.export_values();

;

	// wxHitTest file: line:2417
	pybind11::enum_<wxHitTest>(M(""), "wxHitTest", pybind11::arithmetic(), "")
		.value("wxHT_NOWHERE", wxHT_NOWHERE)
		.value("wxHT_SCROLLBAR_FIRST", wxHT_SCROLLBAR_FIRST)
		.value("wxHT_SCROLLBAR_ARROW_LINE_1", wxHT_SCROLLBAR_ARROW_LINE_1)
		.value("wxHT_SCROLLBAR_ARROW_LINE_2", wxHT_SCROLLBAR_ARROW_LINE_2)
		.value("wxHT_SCROLLBAR_ARROW_PAGE_1", wxHT_SCROLLBAR_ARROW_PAGE_1)
		.value("wxHT_SCROLLBAR_ARROW_PAGE_2", wxHT_SCROLLBAR_ARROW_PAGE_2)
		.value("wxHT_SCROLLBAR_THUMB", wxHT_SCROLLBAR_THUMB)
		.value("wxHT_SCROLLBAR_BAR_1", wxHT_SCROLLBAR_BAR_1)
		.value("wxHT_SCROLLBAR_BAR_2", wxHT_SCROLLBAR_BAR_2)
		.value("wxHT_SCROLLBAR_LAST", wxHT_SCROLLBAR_LAST)
		.value("wxHT_WINDOW_OUTSIDE", wxHT_WINDOW_OUTSIDE)
		.value("wxHT_WINDOW_INSIDE", wxHT_WINDOW_INSIDE)
		.value("wxHT_WINDOW_VERT_SCROLLBAR", wxHT_WINDOW_VERT_SCROLLBAR)
		.value("wxHT_WINDOW_HORZ_SCROLLBAR", wxHT_WINDOW_HORZ_SCROLLBAR)
		.value("wxHT_WINDOW_CORNER", wxHT_WINDOW_CORNER)
		.value("wxHT_MAX", wxHT_MAX)
		.export_values();

;

	// wxHatchStyle file: line:2472
	pybind11::enum_<wxHatchStyle>(M(""), "wxHatchStyle", pybind11::arithmetic(), "")
		.value("wxHATCHSTYLE_INVALID", wxHATCHSTYLE_INVALID)
		.value("wxHATCHSTYLE_FIRST", wxHATCHSTYLE_FIRST)
		.value("wxHATCHSTYLE_BDIAGONAL", wxHATCHSTYLE_BDIAGONAL)
		.value("wxHATCHSTYLE_CROSSDIAG", wxHATCHSTYLE_CROSSDIAG)
		.value("wxHATCHSTYLE_FDIAGONAL", wxHATCHSTYLE_FDIAGONAL)
		.value("wxHATCHSTYLE_CROSS", wxHATCHSTYLE_CROSS)
		.value("wxHATCHSTYLE_HORIZONTAL", wxHATCHSTYLE_HORIZONTAL)
		.value("wxHATCHSTYLE_VERTICAL", wxHATCHSTYLE_VERTICAL)
		.value("wxHATCHSTYLE_LAST", wxHATCHSTYLE_LAST)
		.export_values();

;

	// wxDeprecatedGUIConstants file: line:2499
	pybind11::enum_<wxDeprecatedGUIConstants>(M(""), "wxDeprecatedGUIConstants", pybind11::arithmetic(), "")
		.value("wxDEFAULT", wxDEFAULT)
		.value("wxDECORATIVE", wxDECORATIVE)
		.value("wxROMAN", wxROMAN)
		.value("wxSCRIPT", wxSCRIPT)
		.value("wxSWISS", wxSWISS)
		.value("wxMODERN", wxMODERN)
		.value("wxTELETYPE", wxTELETYPE)
		.value("wxVARIABLE", wxVARIABLE)
		.value("wxFIXED", wxFIXED)
		.value("wxNORMAL", wxNORMAL)
		.value("wxLIGHT", wxLIGHT)
		.value("wxBOLD", wxBOLD)
		.value("wxITALIC", wxITALIC)
		.value("wxSLANT", wxSLANT)
		.value("wxSOLID", wxSOLID)
		.value("wxDOT", wxDOT)
		.value("wxLONG_DASH", wxLONG_DASH)
		.value("wxSHORT_DASH", wxSHORT_DASH)
		.value("wxDOT_DASH", wxDOT_DASH)
		.value("wxUSER_DASH", wxUSER_DASH)
		.value("wxTRANSPARENT", wxTRANSPARENT)
		.value("wxSTIPPLE_MASK_OPAQUE", wxSTIPPLE_MASK_OPAQUE)
		.value("wxSTIPPLE_MASK", wxSTIPPLE_MASK)
		.value("wxSTIPPLE", wxSTIPPLE)
		.value("wxBDIAGONAL_HATCH", wxBDIAGONAL_HATCH)
		.value("wxCROSSDIAG_HATCH", wxCROSSDIAG_HATCH)
		.value("wxFDIAGONAL_HATCH", wxFDIAGONAL_HATCH)
		.value("wxCROSS_HATCH", wxCROSS_HATCH)
		.value("wxHORIZONTAL_HATCH", wxHORIZONTAL_HATCH)
		.value("wxVERTICAL_HATCH", wxVERTICAL_HATCH)
		.value("wxFIRST_HATCH", wxFIRST_HATCH)
		.value("wxLAST_HATCH", wxLAST_HATCH)
		.export_values();

;

	// wxDataFormatId file: line:2560
	pybind11::enum_<wxDataFormatId>(M(""), "wxDataFormatId", pybind11::arithmetic(), "")
		.value("wxDF_INVALID", wxDF_INVALID)
		.value("wxDF_TEXT", wxDF_TEXT)
		.value("wxDF_BITMAP", wxDF_BITMAP)
		.value("wxDF_METAFILE", wxDF_METAFILE)
		.value("wxDF_SYLK", wxDF_SYLK)
		.value("wxDF_DIF", wxDF_DIF)
		.value("wxDF_TIFF", wxDF_TIFF)
		.value("wxDF_OEMTEXT", wxDF_OEMTEXT)
		.value("wxDF_DIB", wxDF_DIB)
		.value("wxDF_PALETTE", wxDF_PALETTE)
		.value("wxDF_PENDATA", wxDF_PENDATA)
		.value("wxDF_RIFF", wxDF_RIFF)
		.value("wxDF_WAVE", wxDF_WAVE)
		.value("wxDF_UNICODETEXT", wxDF_UNICODETEXT)
		.value("wxDF_ENHMETAFILE", wxDF_ENHMETAFILE)
		.value("wxDF_FILENAME", wxDF_FILENAME)
		.value("wxDF_LOCALE", wxDF_LOCALE)
		.value("wxDF_PRIVATE", wxDF_PRIVATE)
		.value("wxDF_HTML", wxDF_HTML)
		.value("wxDF_MAX", wxDF_MAX)
		.export_values();

;

}

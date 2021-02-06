#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/dc.h> // wxDC

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxEventConnectionRef file: line:3788
struct PyCallBack_wxEventConnectionRef : public wxEventConnectionRef {
	using wxEventConnectionRef::wxEventConnectionRef;

	void OnObjectDestroy() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventConnectionRef *>(this), "OnObjectDestroy");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEventConnectionRef::OnObjectDestroy();
	}
	class wxEventConnectionRef * ToEventConnection() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventConnectionRef *>(this), "ToEventConnection");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEventConnectionRef *>::value) {
				static pybind11::detail::override_caster_t<class wxEventConnectionRef *> caster;
				return pybind11::detail::cast_ref<class wxEventConnectionRef *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEventConnectionRef *>(std::move(o));
		}
		return wxEventConnectionRef::ToEventConnection();
	}
};

// wxEventBlocker file: line:3868
struct PyCallBack_wxEventBlocker : public wxEventBlocker {
	using wxEventBlocker::wxEventBlocker;

	bool ProcessEvent(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "ProcessEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEventBlocker::ProcessEvent(a0);
	}
	void SetNextHandler(class wxEvtHandler * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "SetNextHandler");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEvtHandler::SetNextHandler(a0);
	}
	void SetPreviousHandler(class wxEvtHandler * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "SetPreviousHandler");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEvtHandler::SetPreviousHandler(a0);
	}
	void QueueEvent(class wxEvent * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "QueueEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEvtHandler::QueueEvent(a0);
	}
	void AddPendingEvent(const class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "AddPendingEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEvtHandler::AddPendingEvent(a0);
	}
	bool SearchEventTable(struct wxEventTable & a0, class wxEvent & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "SearchEventTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEvtHandler::SearchEventTable(a0, a1);
	}
	bool TryBefore(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "TryBefore");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEvtHandler::TryBefore(a0);
	}
	bool TryAfter(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "TryAfter");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEvtHandler::TryAfter(a0);
	}
	bool TryValidator(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "TryValidator");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEvtHandler::TryValidator(a0);
	}
	bool TryParent(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "TryParent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEvtHandler::TryParent(a0);
	}
	const struct wxEventTable * GetEventTable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "GetEventTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const struct wxEventTable *>::value) {
				static pybind11::detail::override_caster_t<const struct wxEventTable *> caster;
				return pybind11::detail::cast_ref<const struct wxEventTable *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const struct wxEventTable *>(std::move(o));
		}
		return wxEvtHandler::GetEventTable();
	}
	class wxEventHashTable & GetEventHashTable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "GetEventHashTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEventHashTable &>::value) {
				static pybind11::detail::override_caster_t<class wxEventHashTable &> caster;
				return pybind11::detail::cast_ref<class wxEventHashTable &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEventHashTable &>(std::move(o));
		}
		return wxEvtHandler::GetEventHashTable();
	}
	void DoSetClientObject(class wxClientData * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "DoSetClientObject");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEvtHandler::DoSetClientObject(a0);
	}
	class wxClientData * DoGetClientObject() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "DoGetClientObject");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClientData *>::value) {
				static pybind11::detail::override_caster_t<class wxClientData *> caster;
				return pybind11::detail::cast_ref<class wxClientData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClientData *>(std::move(o));
		}
		return wxEvtHandler::DoGetClientObject();
	}
	void DoSetClientData(void * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "DoSetClientData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEvtHandler::DoSetClientData(a0);
	}
	void * DoGetClientData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "DoGetClientData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void *>::value) {
				static pybind11::detail::override_caster_t<void *> caster;
				return pybind11::detail::cast_ref<void *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void *>(std::move(o));
		}
		return wxEvtHandler::DoGetClientData();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxEvtHandler::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventBlocker *>(this), "CloneRefData");
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

void bind_unknown_unknown_62(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxEventConnectionRef file: line:3788
		pybind11::class_<wxEventConnectionRef, std::shared_ptr<wxEventConnectionRef>, PyCallBack_wxEventConnectionRef, wxTrackerNode> cl(M(""), "wxEventConnectionRef", "");
		cl.def( pybind11::init( [](){ return new wxEventConnectionRef(); }, [](){ return new PyCallBack_wxEventConnectionRef(); } ) );
		cl.def( pybind11::init<class wxEvtHandler *, class wxEvtHandler *>(), pybind11::arg("src"), pybind11::arg("sink") );

		cl.def( pybind11::init( [](PyCallBack_wxEventConnectionRef const &o){ return new PyCallBack_wxEventConnectionRef(o); } ) );
		cl.def( pybind11::init( [](wxEventConnectionRef const &o){ return new wxEventConnectionRef(o); } ) );
		cl.def("OnObjectDestroy", (void (wxEventConnectionRef::*)()) &wxEventConnectionRef::OnObjectDestroy, "C++: wxEventConnectionRef::OnObjectDestroy() --> void");
		cl.def("ToEventConnection", (class wxEventConnectionRef * (wxEventConnectionRef::*)()) &wxEventConnectionRef::ToEventConnection, "C++: wxEventConnectionRef::ToEventConnection() --> class wxEventConnectionRef *", pybind11::return_value_policy::automatic);
		cl.def("IncRef", (void (wxEventConnectionRef::*)()) &wxEventConnectionRef::IncRef, "C++: wxEventConnectionRef::IncRef() --> void");
		cl.def("DecRef", (void (wxEventConnectionRef::*)()) &wxEventConnectionRef::DecRef, "C++: wxEventConnectionRef::DecRef() --> void");
	}
	// wxPostEvent(class wxEvtHandler *, const class wxEvent &) file: line:3834
	M("").def("wxPostEvent", (void (*)(class wxEvtHandler *, const class wxEvent &)) &wxPostEvent, "C++: wxPostEvent(class wxEvtHandler *, const class wxEvent &) --> void", pybind11::arg("dest"), pybind11::arg("event"));

	// wxQueueEvent(class wxEvtHandler *, class wxEvent *) file: line:3844
	M("").def("wxQueueEvent", (void (*)(class wxEvtHandler *, class wxEvent *)) &wxQueueEvent, "C++: wxQueueEvent(class wxEvtHandler *, class wxEvent *) --> void", pybind11::arg("dest"), pybind11::arg("event"));

	{ // wxEventBlocker file: line:3868
		pybind11::class_<wxEventBlocker, std::shared_ptr<wxEventBlocker>, PyCallBack_wxEventBlocker, wxEvtHandler> cl(M(""), "wxEventBlocker", "");
		cl.def( pybind11::init( [](class wxWindow * a0){ return new wxEventBlocker(a0); }, [](class wxWindow * a0){ return new PyCallBack_wxEventBlocker(a0); } ), "doc");
		cl.def( pybind11::init<class wxWindow *, int>(), pybind11::arg("win"), pybind11::arg("type") );

		cl.def("Block", (void (wxEventBlocker::*)(int)) &wxEventBlocker::Block, "C++: wxEventBlocker::Block(int) --> void", pybind11::arg("type"));
		cl.def("ProcessEvent", (bool (wxEventBlocker::*)(class wxEvent &)) &wxEventBlocker::ProcessEvent, "C++: wxEventBlocker::ProcessEvent(class wxEvent &) --> bool", pybind11::arg("event"));
	}
	// wxFindFocusDescendant(class wxWindow *) file: line:4456
	M("").def("wxFindFocusDescendant", (class wxWindow * (*)(class wxWindow *)) &wxFindFocusDescendant, "C++: wxFindFocusDescendant(class wxWindow *) --> class wxWindow *", pybind11::return_value_policy::automatic, pybind11::arg("ancestor"));

	// wxFontFamily file: line:37
	pybind11::enum_<wxFontFamily>(M(""), "wxFontFamily", pybind11::arithmetic(), "")
		.value("wxFONTFAMILY_DEFAULT", wxFONTFAMILY_DEFAULT)
		.value("wxFONTFAMILY_DECORATIVE", wxFONTFAMILY_DECORATIVE)
		.value("wxFONTFAMILY_ROMAN", wxFONTFAMILY_ROMAN)
		.value("wxFONTFAMILY_SCRIPT", wxFONTFAMILY_SCRIPT)
		.value("wxFONTFAMILY_SWISS", wxFONTFAMILY_SWISS)
		.value("wxFONTFAMILY_MODERN", wxFONTFAMILY_MODERN)
		.value("wxFONTFAMILY_TELETYPE", wxFONTFAMILY_TELETYPE)
		.value("wxFONTFAMILY_MAX", wxFONTFAMILY_MAX)
		.value("wxFONTFAMILY_UNKNOWN", wxFONTFAMILY_UNKNOWN)
		.export_values();

;

	// wxFontStyle file: line:51
	pybind11::enum_<wxFontStyle>(M(""), "wxFontStyle", pybind11::arithmetic(), "")
		.value("wxFONTSTYLE_NORMAL", wxFONTSTYLE_NORMAL)
		.value("wxFONTSTYLE_ITALIC", wxFONTSTYLE_ITALIC)
		.value("wxFONTSTYLE_SLANT", wxFONTSTYLE_SLANT)
		.value("wxFONTSTYLE_MAX", wxFONTSTYLE_MAX)
		.export_values();

;

	// wxFontWeight file: line:60
	pybind11::enum_<wxFontWeight>(M(""), "wxFontWeight", pybind11::arithmetic(), "")
		.value("wxFONTWEIGHT_NORMAL", wxFONTWEIGHT_NORMAL)
		.value("wxFONTWEIGHT_LIGHT", wxFONTWEIGHT_LIGHT)
		.value("wxFONTWEIGHT_BOLD", wxFONTWEIGHT_BOLD)
		.value("wxFONTWEIGHT_MAX", wxFONTWEIGHT_MAX)
		.export_values();

;

	// wxFontSymbolicSize file: line:69
	pybind11::enum_<wxFontSymbolicSize>(M(""), "wxFontSymbolicSize", pybind11::arithmetic(), "")
		.value("wxFONTSIZE_XX_SMALL", wxFONTSIZE_XX_SMALL)
		.value("wxFONTSIZE_X_SMALL", wxFONTSIZE_X_SMALL)
		.value("wxFONTSIZE_SMALL", wxFONTSIZE_SMALL)
		.value("wxFONTSIZE_MEDIUM", wxFONTSIZE_MEDIUM)
		.value("wxFONTSIZE_LARGE", wxFONTSIZE_LARGE)
		.value("wxFONTSIZE_X_LARGE", wxFONTSIZE_X_LARGE)
		.value("wxFONTSIZE_XX_LARGE", wxFONTSIZE_XX_LARGE)
		.export_values();

;

	// wxFontFlag file: line:81
	pybind11::enum_<wxFontFlag>(M(""), "wxFontFlag", pybind11::arithmetic(), "")
		.value("wxFONTFLAG_DEFAULT", wxFONTFLAG_DEFAULT)
		.value("wxFONTFLAG_ITALIC", wxFONTFLAG_ITALIC)
		.value("wxFONTFLAG_SLANT", wxFONTFLAG_SLANT)
		.value("wxFONTFLAG_LIGHT", wxFONTFLAG_LIGHT)
		.value("wxFONTFLAG_BOLD", wxFONTFLAG_BOLD)
		.value("wxFONTFLAG_ANTIALIASED", wxFONTFLAG_ANTIALIASED)
		.value("wxFONTFLAG_NOT_ANTIALIASED", wxFONTFLAG_NOT_ANTIALIASED)
		.value("wxFONTFLAG_UNDERLINED", wxFONTFLAG_UNDERLINED)
		.value("wxFONTFLAG_STRIKETHROUGH", wxFONTFLAG_STRIKETHROUGH)
		.value("wxFONTFLAG_MASK", wxFONTFLAG_MASK)
		.export_values();

;

	{ // wxFontInfo file: line:117
		pybind11::class_<wxFontInfo, std::shared_ptr<wxFontInfo>> cl(M(""), "wxFontInfo", "");
		cl.def( pybind11::init( [](){ return new wxFontInfo(); } ) );
		cl.def( pybind11::init<int>(), pybind11::arg("pointSize") );

		cl.def( pybind11::init<const class wxSize &>(), pybind11::arg("pixelSize") );

		cl.def( pybind11::init( [](wxFontInfo const &o){ return new wxFontInfo(o); } ) );
		cl.def("Family", (class wxFontInfo & (wxFontInfo::*)(enum wxFontFamily)) &wxFontInfo::Family, "C++: wxFontInfo::Family(enum wxFontFamily) --> class wxFontInfo &", pybind11::return_value_policy::automatic, pybind11::arg("family"));
		cl.def("FaceName", (class wxFontInfo & (wxFontInfo::*)(const class wxString &)) &wxFontInfo::FaceName, "C++: wxFontInfo::FaceName(const class wxString &) --> class wxFontInfo &", pybind11::return_value_policy::automatic, pybind11::arg("faceName"));
		cl.def("Bold", [](wxFontInfo &o) -> wxFontInfo & { return o.Bold(); }, "", pybind11::return_value_policy::automatic);
		cl.def("Bold", (class wxFontInfo & (wxFontInfo::*)(bool)) &wxFontInfo::Bold, "C++: wxFontInfo::Bold(bool) --> class wxFontInfo &", pybind11::return_value_policy::automatic, pybind11::arg("bold"));
		cl.def("Light", [](wxFontInfo &o) -> wxFontInfo & { return o.Light(); }, "", pybind11::return_value_policy::automatic);
		cl.def("Light", (class wxFontInfo & (wxFontInfo::*)(bool)) &wxFontInfo::Light, "C++: wxFontInfo::Light(bool) --> class wxFontInfo &", pybind11::return_value_policy::automatic, pybind11::arg("light"));
		cl.def("Italic", [](wxFontInfo &o) -> wxFontInfo & { return o.Italic(); }, "", pybind11::return_value_policy::automatic);
		cl.def("Italic", (class wxFontInfo & (wxFontInfo::*)(bool)) &wxFontInfo::Italic, "C++: wxFontInfo::Italic(bool) --> class wxFontInfo &", pybind11::return_value_policy::automatic, pybind11::arg("italic"));
		cl.def("Slant", [](wxFontInfo &o) -> wxFontInfo & { return o.Slant(); }, "", pybind11::return_value_policy::automatic);
		cl.def("Slant", (class wxFontInfo & (wxFontInfo::*)(bool)) &wxFontInfo::Slant, "C++: wxFontInfo::Slant(bool) --> class wxFontInfo &", pybind11::return_value_policy::automatic, pybind11::arg("slant"));
		cl.def("AntiAliased", [](wxFontInfo &o) -> wxFontInfo & { return o.AntiAliased(); }, "", pybind11::return_value_policy::automatic);
		cl.def("AntiAliased", (class wxFontInfo & (wxFontInfo::*)(bool)) &wxFontInfo::AntiAliased, "C++: wxFontInfo::AntiAliased(bool) --> class wxFontInfo &", pybind11::return_value_policy::automatic, pybind11::arg("antiAliased"));
		cl.def("Underlined", [](wxFontInfo &o) -> wxFontInfo & { return o.Underlined(); }, "", pybind11::return_value_policy::automatic);
		cl.def("Underlined", (class wxFontInfo & (wxFontInfo::*)(bool)) &wxFontInfo::Underlined, "C++: wxFontInfo::Underlined(bool) --> class wxFontInfo &", pybind11::return_value_policy::automatic, pybind11::arg("underlined"));
		cl.def("Strikethrough", [](wxFontInfo &o) -> wxFontInfo & { return o.Strikethrough(); }, "", pybind11::return_value_policy::automatic);
		cl.def("Strikethrough", (class wxFontInfo & (wxFontInfo::*)(bool)) &wxFontInfo::Strikethrough, "C++: wxFontInfo::Strikethrough(bool) --> class wxFontInfo &", pybind11::return_value_policy::automatic, pybind11::arg("strikethrough"));
		cl.def("Encoding", (class wxFontInfo & (wxFontInfo::*)(enum wxFontEncoding)) &wxFontInfo::Encoding, "C++: wxFontInfo::Encoding(enum wxFontEncoding) --> class wxFontInfo &", pybind11::return_value_policy::automatic, pybind11::arg("encoding"));
		cl.def("AllFlags", (class wxFontInfo & (wxFontInfo::*)(int)) &wxFontInfo::AllFlags, "C++: wxFontInfo::AllFlags(int) --> class wxFontInfo &", pybind11::return_value_policy::automatic, pybind11::arg("flags"));
		cl.def("IsUsingSizeInPixels", (bool (wxFontInfo::*)() const) &wxFontInfo::IsUsingSizeInPixels, "C++: wxFontInfo::IsUsingSizeInPixels() const --> bool");
		cl.def("GetPointSize", (int (wxFontInfo::*)() const) &wxFontInfo::GetPointSize, "C++: wxFontInfo::GetPointSize() const --> int");
		cl.def("GetPixelSize", (class wxSize (wxFontInfo::*)() const) &wxFontInfo::GetPixelSize, "C++: wxFontInfo::GetPixelSize() const --> class wxSize");
		cl.def("GetFamily", (enum wxFontFamily (wxFontInfo::*)() const) &wxFontInfo::GetFamily, "C++: wxFontInfo::GetFamily() const --> enum wxFontFamily");
		cl.def("GetFaceName", (const class wxString & (wxFontInfo::*)() const) &wxFontInfo::GetFaceName, "C++: wxFontInfo::GetFaceName() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetStyle", (enum wxFontStyle (wxFontInfo::*)() const) &wxFontInfo::GetStyle, "C++: wxFontInfo::GetStyle() const --> enum wxFontStyle");
		cl.def("GetWeight", (enum wxFontWeight (wxFontInfo::*)() const) &wxFontInfo::GetWeight, "C++: wxFontInfo::GetWeight() const --> enum wxFontWeight");
		cl.def("IsAntiAliased", (bool (wxFontInfo::*)() const) &wxFontInfo::IsAntiAliased, "C++: wxFontInfo::IsAntiAliased() const --> bool");
		cl.def("IsUnderlined", (bool (wxFontInfo::*)() const) &wxFontInfo::IsUnderlined, "C++: wxFontInfo::IsUnderlined() const --> bool");
		cl.def("IsStrikethrough", (bool (wxFontInfo::*)() const) &wxFontInfo::IsStrikethrough, "C++: wxFontInfo::IsStrikethrough() const --> bool");
		cl.def("GetEncoding", (enum wxFontEncoding (wxFontInfo::*)() const) &wxFontInfo::GetEncoding, "C++: wxFontInfo::GetEncoding() const --> enum wxFontEncoding");
		cl.def("assign", (class wxFontInfo & (wxFontInfo::*)(const class wxFontInfo &)) &wxFontInfo::operator=, "C++: wxFontInfo::operator=(const class wxFontInfo &) --> class wxFontInfo &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxFontBase file: line:258
		pybind11::class_<wxFontBase, std::shared_ptr<wxFontBase>, wxGDIObject> cl(M(""), "wxFontBase", "");
		cl.def_static("New", [](int const & a0, int const & a1, int const & a2, int const & a3) -> wxFont * { return wxFontBase::New(a0, a1, a2, a3); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"));
		cl.def_static("New", [](int const & a0, int const & a1, int const & a2, int const & a3, bool const & a4) -> wxFont * { return wxFontBase::New(a0, a1, a2, a3, a4); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"));
		cl.def_static("New", [](int const & a0, int const & a1, int const & a2, int const & a3, bool const & a4, const class wxString & a5) -> wxFont * { return wxFontBase::New(a0, a1, a2, a3, a4, a5); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"), pybind11::arg("face"));
		cl.def_static("New", (class wxFont * (*)(int, int, int, int, bool, const class wxString &, enum wxFontEncoding)) &wxFontBase::New, "C++: wxFontBase::New(int, int, int, int, bool, const class wxString &, enum wxFontEncoding) --> class wxFont *", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"), pybind11::arg("face"), pybind11::arg("encoding"));
		cl.def_static("New", [](const class wxSize & a0, int const & a1, int const & a2, int const & a3) -> wxFont * { return wxFontBase::New(a0, a1, a2, a3); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pixelSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"));
		cl.def_static("New", [](const class wxSize & a0, int const & a1, int const & a2, int const & a3, bool const & a4) -> wxFont * { return wxFontBase::New(a0, a1, a2, a3, a4); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pixelSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"));
		cl.def_static("New", [](const class wxSize & a0, int const & a1, int const & a2, int const & a3, bool const & a4, const class wxString & a5) -> wxFont * { return wxFontBase::New(a0, a1, a2, a3, a4, a5); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pixelSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"), pybind11::arg("face"));
		cl.def_static("New", (class wxFont * (*)(const class wxSize &, int, int, int, bool, const class wxString &, enum wxFontEncoding)) &wxFontBase::New, "C++: wxFontBase::New(const class wxSize &, int, int, int, bool, const class wxString &, enum wxFontEncoding) --> class wxFont *", pybind11::return_value_policy::automatic, pybind11::arg("pixelSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"), pybind11::arg("face"), pybind11::arg("encoding"));
		cl.def_static("New", [](int const & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3) -> wxFont * { return wxFontBase::New(a0, a1, a2, a3); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"));
		cl.def_static("New", [](int const & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3, bool const & a4) -> wxFont * { return wxFontBase::New(a0, a1, a2, a3, a4); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"));
		cl.def_static("New", [](int const & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3, bool const & a4, const class wxString & a5) -> wxFont * { return wxFontBase::New(a0, a1, a2, a3, a4, a5); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"), pybind11::arg("face"));
		cl.def_static("New", (class wxFont * (*)(int, enum wxFontFamily, enum wxFontStyle, enum wxFontWeight, bool, const class wxString &, enum wxFontEncoding)) &wxFontBase::New, "C++: wxFontBase::New(int, enum wxFontFamily, enum wxFontStyle, enum wxFontWeight, bool, const class wxString &, enum wxFontEncoding) --> class wxFont *", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"), pybind11::arg("face"), pybind11::arg("encoding"));
		cl.def_static("New", [](const class wxSize & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3) -> wxFont * { return wxFontBase::New(a0, a1, a2, a3); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pixelSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"));
		cl.def_static("New", [](const class wxSize & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3, bool const & a4) -> wxFont * { return wxFontBase::New(a0, a1, a2, a3, a4); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pixelSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"));
		cl.def_static("New", [](const class wxSize & a0, enum wxFontFamily const & a1, enum wxFontStyle const & a2, enum wxFontWeight const & a3, bool const & a4, const class wxString & a5) -> wxFont * { return wxFontBase::New(a0, a1, a2, a3, a4, a5); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pixelSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"), pybind11::arg("face"));
		cl.def_static("New", (class wxFont * (*)(const class wxSize &, enum wxFontFamily, enum wxFontStyle, enum wxFontWeight, bool, const class wxString &, enum wxFontEncoding)) &wxFontBase::New, "C++: wxFontBase::New(const class wxSize &, enum wxFontFamily, enum wxFontStyle, enum wxFontWeight, bool, const class wxString &, enum wxFontEncoding) --> class wxFont *", pybind11::return_value_policy::automatic, pybind11::arg("pixelSize"), pybind11::arg("family"), pybind11::arg("style"), pybind11::arg("weight"), pybind11::arg("underlined"), pybind11::arg("face"), pybind11::arg("encoding"));
		cl.def_static("New", [](int const & a0, enum wxFontFamily const & a1) -> wxFont * { return wxFontBase::New(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"));
		cl.def_static("New", [](int const & a0, enum wxFontFamily const & a1, int const & a2) -> wxFont * { return wxFontBase::New(a0, a1, a2); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("flags"));
		cl.def_static("New", [](int const & a0, enum wxFontFamily const & a1, int const & a2, const class wxString & a3) -> wxFont * { return wxFontBase::New(a0, a1, a2, a3); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("flags"), pybind11::arg("face"));
		cl.def_static("New", (class wxFont * (*)(int, enum wxFontFamily, int, const class wxString &, enum wxFontEncoding)) &wxFontBase::New, "C++: wxFontBase::New(int, enum wxFontFamily, int, const class wxString &, enum wxFontEncoding) --> class wxFont *", pybind11::return_value_policy::automatic, pybind11::arg("pointSize"), pybind11::arg("family"), pybind11::arg("flags"), pybind11::arg("face"), pybind11::arg("encoding"));
		cl.def_static("New", [](const class wxSize & a0, enum wxFontFamily const & a1) -> wxFont * { return wxFontBase::New(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pixelSize"), pybind11::arg("family"));
		cl.def_static("New", [](const class wxSize & a0, enum wxFontFamily const & a1, int const & a2) -> wxFont * { return wxFontBase::New(a0, a1, a2); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pixelSize"), pybind11::arg("family"), pybind11::arg("flags"));
		cl.def_static("New", [](const class wxSize & a0, enum wxFontFamily const & a1, int const & a2, const class wxString & a3) -> wxFont * { return wxFontBase::New(a0, a1, a2, a3); }, "", pybind11::return_value_policy::automatic, pybind11::arg("pixelSize"), pybind11::arg("family"), pybind11::arg("flags"), pybind11::arg("face"));
		cl.def_static("New", (class wxFont * (*)(const class wxSize &, enum wxFontFamily, int, const class wxString &, enum wxFontEncoding)) &wxFontBase::New, "C++: wxFontBase::New(const class wxSize &, enum wxFontFamily, int, const class wxString &, enum wxFontEncoding) --> class wxFont *", pybind11::return_value_policy::automatic, pybind11::arg("pixelSize"), pybind11::arg("family"), pybind11::arg("flags"), pybind11::arg("face"), pybind11::arg("encoding"));
		cl.def_static("New", (class wxFont * (*)(const class wxString &)) &wxFontBase::New, "C++: wxFontBase::New(const class wxString &) --> class wxFont *", pybind11::return_value_policy::automatic, pybind11::arg("strNativeFontDesc"));
		cl.def("__eq__", (bool (wxFontBase::*)(const class wxFont &) const) &wxFontBase::operator==, "C++: wxFontBase::operator==(const class wxFont &) const --> bool", pybind11::arg("font"));
		cl.def("__ne__", (bool (wxFontBase::*)(const class wxFont &) const) &wxFontBase::operator!=, "C++: wxFontBase::operator!=(const class wxFont &) const --> bool", pybind11::arg("font"));
		cl.def("GetPointSize", (int (wxFontBase::*)() const) &wxFontBase::GetPointSize, "C++: wxFontBase::GetPointSize() const --> int");
		cl.def("GetPixelSize", (class wxSize (wxFontBase::*)() const) &wxFontBase::GetPixelSize, "C++: wxFontBase::GetPixelSize() const --> class wxSize");
		cl.def("IsUsingSizeInPixels", (bool (wxFontBase::*)() const) &wxFontBase::IsUsingSizeInPixels, "C++: wxFontBase::IsUsingSizeInPixels() const --> bool");
		cl.def("GetFamily", (enum wxFontFamily (wxFontBase::*)() const) &wxFontBase::GetFamily, "C++: wxFontBase::GetFamily() const --> enum wxFontFamily");
		cl.def("GetStyle", (enum wxFontStyle (wxFontBase::*)() const) &wxFontBase::GetStyle, "C++: wxFontBase::GetStyle() const --> enum wxFontStyle");
		cl.def("GetWeight", (enum wxFontWeight (wxFontBase::*)() const) &wxFontBase::GetWeight, "C++: wxFontBase::GetWeight() const --> enum wxFontWeight");
		cl.def("GetUnderlined", (bool (wxFontBase::*)() const) &wxFontBase::GetUnderlined, "C++: wxFontBase::GetUnderlined() const --> bool");
		cl.def("GetStrikethrough", (bool (wxFontBase::*)() const) &wxFontBase::GetStrikethrough, "C++: wxFontBase::GetStrikethrough() const --> bool");
		cl.def("GetFaceName", (class wxString (wxFontBase::*)() const) &wxFontBase::GetFaceName, "C++: wxFontBase::GetFaceName() const --> class wxString");
		cl.def("GetEncoding", (enum wxFontEncoding (wxFontBase::*)() const) &wxFontBase::GetEncoding, "C++: wxFontBase::GetEncoding() const --> enum wxFontEncoding");
		cl.def("IsFixedWidth", (bool (wxFontBase::*)() const) &wxFontBase::IsFixedWidth, "C++: wxFontBase::IsFixedWidth() const --> bool");
		cl.def("GetNativeFontInfoDesc", (class wxString (wxFontBase::*)() const) &wxFontBase::GetNativeFontInfoDesc, "C++: wxFontBase::GetNativeFontInfoDesc() const --> class wxString");
		cl.def("GetNativeFontInfoUserDesc", (class wxString (wxFontBase::*)() const) &wxFontBase::GetNativeFontInfoUserDesc, "C++: wxFontBase::GetNativeFontInfoUserDesc() const --> class wxString");
		cl.def("SetPointSize", (void (wxFontBase::*)(int)) &wxFontBase::SetPointSize, "C++: wxFontBase::SetPointSize(int) --> void", pybind11::arg("pointSize"));
		cl.def("SetPixelSize", (void (wxFontBase::*)(const class wxSize &)) &wxFontBase::SetPixelSize, "C++: wxFontBase::SetPixelSize(const class wxSize &) --> void", pybind11::arg("pixelSize"));
		cl.def("SetFamily", (void (wxFontBase::*)(enum wxFontFamily)) &wxFontBase::SetFamily, "C++: wxFontBase::SetFamily(enum wxFontFamily) --> void", pybind11::arg("family"));
		cl.def("SetStyle", (void (wxFontBase::*)(enum wxFontStyle)) &wxFontBase::SetStyle, "C++: wxFontBase::SetStyle(enum wxFontStyle) --> void", pybind11::arg("style"));
		cl.def("SetWeight", (void (wxFontBase::*)(enum wxFontWeight)) &wxFontBase::SetWeight, "C++: wxFontBase::SetWeight(enum wxFontWeight) --> void", pybind11::arg("weight"));
		cl.def("SetUnderlined", (void (wxFontBase::*)(bool)) &wxFontBase::SetUnderlined, "C++: wxFontBase::SetUnderlined(bool) --> void", pybind11::arg("underlined"));
		cl.def("SetStrikethrough", (void (wxFontBase::*)(bool)) &wxFontBase::SetStrikethrough, "C++: wxFontBase::SetStrikethrough(bool) --> void", pybind11::arg(""));
		cl.def("SetEncoding", (void (wxFontBase::*)(enum wxFontEncoding)) &wxFontBase::SetEncoding, "C++: wxFontBase::SetEncoding(enum wxFontEncoding) --> void", pybind11::arg("encoding"));
		cl.def("SetFaceName", (bool (wxFontBase::*)(const class wxString &)) &wxFontBase::SetFaceName, "C++: wxFontBase::SetFaceName(const class wxString &) --> bool", pybind11::arg("faceName"));
		cl.def("SetNativeFontInfo", (bool (wxFontBase::*)(const class wxString &)) &wxFontBase::SetNativeFontInfo, "C++: wxFontBase::SetNativeFontInfo(const class wxString &) --> bool", pybind11::arg("info"));
		cl.def("SetNativeFontInfoUserDesc", (bool (wxFontBase::*)(const class wxString &)) &wxFontBase::SetNativeFontInfoUserDesc, "C++: wxFontBase::SetNativeFontInfoUserDesc(const class wxString &) --> bool", pybind11::arg("info"));
		cl.def("SetSymbolicSize", (void (wxFontBase::*)(enum wxFontSymbolicSize)) &wxFontBase::SetSymbolicSize, "C++: wxFontBase::SetSymbolicSize(enum wxFontSymbolicSize) --> void", pybind11::arg("size"));
		cl.def("SetSymbolicSizeRelativeTo", (void (wxFontBase::*)(enum wxFontSymbolicSize, int)) &wxFontBase::SetSymbolicSizeRelativeTo, "C++: wxFontBase::SetSymbolicSizeRelativeTo(enum wxFontSymbolicSize, int) --> void", pybind11::arg("size"), pybind11::arg("base"));
		cl.def_static("AdjustToSymbolicSize", (int (*)(enum wxFontSymbolicSize, int)) &wxFontBase::AdjustToSymbolicSize, "C++: wxFontBase::AdjustToSymbolicSize(enum wxFontSymbolicSize, int) --> int", pybind11::arg("size"), pybind11::arg("base"));
		cl.def("GetFamilyString", (class wxString (wxFontBase::*)() const) &wxFontBase::GetFamilyString, "C++: wxFontBase::GetFamilyString() const --> class wxString");
		cl.def("GetStyleString", (class wxString (wxFontBase::*)() const) &wxFontBase::GetStyleString, "C++: wxFontBase::GetStyleString() const --> class wxString");
		cl.def("GetWeightString", (class wxString (wxFontBase::*)() const) &wxFontBase::GetWeightString, "C++: wxFontBase::GetWeightString() const --> class wxString");
		cl.def_static("GetDefaultEncoding", (enum wxFontEncoding (*)()) &wxFontBase::GetDefaultEncoding, "C++: wxFontBase::GetDefaultEncoding() --> enum wxFontEncoding");
		cl.def_static("SetDefaultEncoding", (void (*)(enum wxFontEncoding)) &wxFontBase::SetDefaultEncoding, "C++: wxFontBase::SetDefaultEncoding(enum wxFontEncoding) --> void", pybind11::arg("encoding"));
		cl.def("SetNoAntiAliasing", [](wxFontBase &o) -> void { return o.SetNoAntiAliasing(); }, "");
		cl.def("SetNoAntiAliasing", (void (wxFontBase::*)(bool)) &wxFontBase::SetNoAntiAliasing, "C++: wxFontBase::SetNoAntiAliasing(bool) --> void", pybind11::arg("no"));
		cl.def("GetNoAntiAliasing", (bool (wxFontBase::*)() const) &wxFontBase::GetNoAntiAliasing, "C++: wxFontBase::GetNoAntiAliasing() const --> bool");
		cl.def("assign", (class wxFontBase & (wxFontBase::*)(const class wxFontBase &)) &wxFontBase::operator=, "C++: wxFontBase::operator=(const class wxFontBase &) --> class wxFontBase &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}

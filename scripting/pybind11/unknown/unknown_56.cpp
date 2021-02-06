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

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxScrollEvent file: line:1647
struct PyCallBack_wxScrollEvent : public wxScrollEvent {
	using wxScrollEvent::wxScrollEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxScrollEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxScrollEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxScrollEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxScrollEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxScrollEvent *>(this), "GetEventCategory");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxEventCategory>::value) {
				static pybind11::detail::override_caster_t<enum wxEventCategory> caster;
				return pybind11::detail::cast_ref<enum wxEventCategory>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxEventCategory>(std::move(o));
		}
		return wxCommandEvent::GetEventCategory();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxScrollEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxScrollEvent *>(this), "CloneRefData");
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

// wxScrollWinEvent file: line:1677
struct PyCallBack_wxScrollWinEvent : public wxScrollWinEvent {
	using wxScrollWinEvent::wxScrollWinEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxScrollWinEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxScrollWinEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxScrollWinEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxScrollWinEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxScrollWinEvent *>(this), "GetEventCategory");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxEventCategory>::value) {
				static pybind11::detail::override_caster_t<enum wxEventCategory> caster;
				return pybind11::detail::cast_ref<enum wxEventCategory>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxEventCategory>(std::move(o));
		}
		return wxEvent::GetEventCategory();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxScrollWinEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxScrollWinEvent *>(this), "CloneRefData");
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

// wxMouseEvent file: line:1726
struct PyCallBack_wxMouseEvent : public wxMouseEvent {
	using wxMouseEvent::wxMouseEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMouseEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxMouseEvent::Clone();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMouseEvent *>(this), "GetEventCategory");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxEventCategory>::value) {
				static pybind11::detail::override_caster_t<enum wxEventCategory> caster;
				return pybind11::detail::cast_ref<enum wxEventCategory>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxEventCategory>(std::move(o));
		}
		return wxMouseEvent::GetEventCategory();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMouseEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxMouseEvent::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMouseEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMouseEvent *>(this), "CloneRefData");
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

// wxSetCursorEvent file: line:1861
struct PyCallBack_wxSetCursorEvent : public wxSetCursorEvent {
	using wxSetCursorEvent::wxSetCursorEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxSetCursorEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxSetCursorEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxSetCursorEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxSetCursorEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxSetCursorEvent *>(this), "GetEventCategory");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxEventCategory>::value) {
				static pybind11::detail::override_caster_t<enum wxEventCategory> caster;
				return pybind11::detail::cast_ref<enum wxEventCategory>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxEventCategory>(std::move(o));
		}
		return wxEvent::GetEventCategory();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxSetCursorEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxSetCursorEvent *>(this), "CloneRefData");
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

// wxKeyEvent file: line:1930
struct PyCallBack_wxKeyEvent : public wxKeyEvent {
	using wxKeyEvent::wxKeyEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxKeyEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxKeyEvent::Clone();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxKeyEvent *>(this), "GetEventCategory");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxEventCategory>::value) {
				static pybind11::detail::override_caster_t<enum wxEventCategory> caster;
				return pybind11::detail::cast_ref<enum wxEventCategory>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxEventCategory>(std::move(o));
		}
		return wxKeyEvent::GetEventCategory();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxKeyEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxKeyEvent::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxKeyEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxKeyEvent *>(this), "CloneRefData");
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

void bind_unknown_unknown_56(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxScrollEvent file: line:1647
		pybind11::class_<wxScrollEvent, std::shared_ptr<wxScrollEvent>, PyCallBack_wxScrollEvent, wxCommandEvent> cl(M(""), "wxScrollEvent", "");
		cl.def( pybind11::init( [](){ return new wxScrollEvent(); }, [](){ return new PyCallBack_wxScrollEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxScrollEvent(a0); }, [](int const & a0){ return new PyCallBack_wxScrollEvent(a0); } ), "doc");
		cl.def( pybind11::init( [](int const & a0, int const & a1){ return new wxScrollEvent(a0, a1); }, [](int const & a0, int const & a1){ return new PyCallBack_wxScrollEvent(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](int const & a0, int const & a1, int const & a2){ return new wxScrollEvent(a0, a1, a2); }, [](int const & a0, int const & a1, int const & a2){ return new PyCallBack_wxScrollEvent(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init<int, int, int, int>(), pybind11::arg("commandType"), pybind11::arg("winid"), pybind11::arg("pos"), pybind11::arg("orient") );

		cl.def( pybind11::init( [](PyCallBack_wxScrollEvent const &o){ return new PyCallBack_wxScrollEvent(o); } ) );
		cl.def( pybind11::init( [](wxScrollEvent const &o){ return new wxScrollEvent(o); } ) );
		cl.def("GetOrientation", (int (wxScrollEvent::*)() const) &wxScrollEvent::GetOrientation, "C++: wxScrollEvent::GetOrientation() const --> int");
		cl.def("GetPosition", (int (wxScrollEvent::*)() const) &wxScrollEvent::GetPosition, "C++: wxScrollEvent::GetPosition() const --> int");
		cl.def("SetOrientation", (void (wxScrollEvent::*)(int)) &wxScrollEvent::SetOrientation, "C++: wxScrollEvent::SetOrientation(int) --> void", pybind11::arg("orient"));
		cl.def("SetPosition", (void (wxScrollEvent::*)(int)) &wxScrollEvent::SetPosition, "C++: wxScrollEvent::SetPosition(int) --> void", pybind11::arg("pos"));
		cl.def("Clone", (class wxEvent * (wxScrollEvent::*)() const) &wxScrollEvent::Clone, "C++: wxScrollEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxScrollEvent::*)() const) &wxScrollEvent::GetClassInfo, "C++: wxScrollEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxScrollEvent::wxCreateObject, "C++: wxScrollEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxScrollWinEvent file: line:1677
		pybind11::class_<wxScrollWinEvent, std::shared_ptr<wxScrollWinEvent>, PyCallBack_wxScrollWinEvent, wxEvent> cl(M(""), "wxScrollWinEvent", "");
		cl.def( pybind11::init( [](){ return new wxScrollWinEvent(); }, [](){ return new PyCallBack_wxScrollWinEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxScrollWinEvent(a0); }, [](int const & a0){ return new PyCallBack_wxScrollWinEvent(a0); } ), "doc");
		cl.def( pybind11::init( [](int const & a0, int const & a1){ return new wxScrollWinEvent(a0, a1); }, [](int const & a0, int const & a1){ return new PyCallBack_wxScrollWinEvent(a0, a1); } ), "doc");
		cl.def( pybind11::init<int, int, int>(), pybind11::arg("commandType"), pybind11::arg("pos"), pybind11::arg("orient") );

		cl.def( pybind11::init( [](PyCallBack_wxScrollWinEvent const &o){ return new PyCallBack_wxScrollWinEvent(o); } ) );
		cl.def( pybind11::init( [](wxScrollWinEvent const &o){ return new wxScrollWinEvent(o); } ) );
		cl.def("GetOrientation", (int (wxScrollWinEvent::*)() const) &wxScrollWinEvent::GetOrientation, "C++: wxScrollWinEvent::GetOrientation() const --> int");
		cl.def("GetPosition", (int (wxScrollWinEvent::*)() const) &wxScrollWinEvent::GetPosition, "C++: wxScrollWinEvent::GetPosition() const --> int");
		cl.def("SetOrientation", (void (wxScrollWinEvent::*)(int)) &wxScrollWinEvent::SetOrientation, "C++: wxScrollWinEvent::SetOrientation(int) --> void", pybind11::arg("orient"));
		cl.def("SetPosition", (void (wxScrollWinEvent::*)(int)) &wxScrollWinEvent::SetPosition, "C++: wxScrollWinEvent::SetPosition(int) --> void", pybind11::arg("pos"));
		cl.def("Clone", (class wxEvent * (wxScrollWinEvent::*)() const) &wxScrollWinEvent::Clone, "C++: wxScrollWinEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxScrollWinEvent::*)() const) &wxScrollWinEvent::GetClassInfo, "C++: wxScrollWinEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxScrollWinEvent::wxCreateObject, "C++: wxScrollWinEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	// wxMouseWheelAxis file: line:1720
	pybind11::enum_<wxMouseWheelAxis>(M(""), "wxMouseWheelAxis", pybind11::arithmetic(), "")
		.value("wxMOUSE_WHEEL_VERTICAL", wxMOUSE_WHEEL_VERTICAL)
		.value("wxMOUSE_WHEEL_HORIZONTAL", wxMOUSE_WHEEL_HORIZONTAL)
		.export_values();

;

	{ // wxMouseEvent file: line:1726
		pybind11::class_<wxMouseEvent, std::shared_ptr<wxMouseEvent>, PyCallBack_wxMouseEvent, wxEvent, wxMouseState> cl(M(""), "wxMouseEvent", "");
		cl.def( pybind11::init( [](){ return new wxMouseEvent(); }, [](){ return new PyCallBack_wxMouseEvent(); } ), "doc");
		cl.def( pybind11::init<int>(), pybind11::arg("mouseType") );

		cl.def( pybind11::init( [](PyCallBack_wxMouseEvent const &o){ return new PyCallBack_wxMouseEvent(o); } ) );
		cl.def( pybind11::init( [](wxMouseEvent const &o){ return new wxMouseEvent(o); } ) );
		cl.def_readwrite("m_clickCount", &wxMouseEvent::m_clickCount);
		cl.def_readwrite("m_wheelAxis", &wxMouseEvent::m_wheelAxis);
		cl.def_readwrite("m_wheelRotation", &wxMouseEvent::m_wheelRotation);
		cl.def_readwrite("m_wheelDelta", &wxMouseEvent::m_wheelDelta);
		cl.def_readwrite("m_linesPerAction", &wxMouseEvent::m_linesPerAction);
		cl.def_readwrite("m_columnsPerAction", &wxMouseEvent::m_columnsPerAction);
		cl.def("IsButton", (bool (wxMouseEvent::*)() const) &wxMouseEvent::IsButton, "C++: wxMouseEvent::IsButton() const --> bool");
		cl.def("ButtonDown", [](wxMouseEvent const &o) -> bool { return o.ButtonDown(); }, "");
		cl.def("ButtonDown", (bool (wxMouseEvent::*)(int) const) &wxMouseEvent::ButtonDown, "C++: wxMouseEvent::ButtonDown(int) const --> bool", pybind11::arg("but"));
		cl.def("ButtonDClick", [](wxMouseEvent const &o) -> bool { return o.ButtonDClick(); }, "");
		cl.def("ButtonDClick", (bool (wxMouseEvent::*)(int) const) &wxMouseEvent::ButtonDClick, "C++: wxMouseEvent::ButtonDClick(int) const --> bool", pybind11::arg("but"));
		cl.def("ButtonUp", [](wxMouseEvent const &o) -> bool { return o.ButtonUp(); }, "");
		cl.def("ButtonUp", (bool (wxMouseEvent::*)(int) const) &wxMouseEvent::ButtonUp, "C++: wxMouseEvent::ButtonUp(int) const --> bool", pybind11::arg("but"));
		cl.def("Button", (bool (wxMouseEvent::*)(int) const) &wxMouseEvent::Button, "C++: wxMouseEvent::Button(int) const --> bool", pybind11::arg("but"));
		cl.def("GetButton", (int (wxMouseEvent::*)() const) &wxMouseEvent::GetButton, "C++: wxMouseEvent::GetButton() const --> int");
		cl.def("LeftDown", (bool (wxMouseEvent::*)() const) &wxMouseEvent::LeftDown, "C++: wxMouseEvent::LeftDown() const --> bool");
		cl.def("MiddleDown", (bool (wxMouseEvent::*)() const) &wxMouseEvent::MiddleDown, "C++: wxMouseEvent::MiddleDown() const --> bool");
		cl.def("RightDown", (bool (wxMouseEvent::*)() const) &wxMouseEvent::RightDown, "C++: wxMouseEvent::RightDown() const --> bool");
		cl.def("Aux1Down", (bool (wxMouseEvent::*)() const) &wxMouseEvent::Aux1Down, "C++: wxMouseEvent::Aux1Down() const --> bool");
		cl.def("Aux2Down", (bool (wxMouseEvent::*)() const) &wxMouseEvent::Aux2Down, "C++: wxMouseEvent::Aux2Down() const --> bool");
		cl.def("LeftUp", (bool (wxMouseEvent::*)() const) &wxMouseEvent::LeftUp, "C++: wxMouseEvent::LeftUp() const --> bool");
		cl.def("MiddleUp", (bool (wxMouseEvent::*)() const) &wxMouseEvent::MiddleUp, "C++: wxMouseEvent::MiddleUp() const --> bool");
		cl.def("RightUp", (bool (wxMouseEvent::*)() const) &wxMouseEvent::RightUp, "C++: wxMouseEvent::RightUp() const --> bool");
		cl.def("Aux1Up", (bool (wxMouseEvent::*)() const) &wxMouseEvent::Aux1Up, "C++: wxMouseEvent::Aux1Up() const --> bool");
		cl.def("Aux2Up", (bool (wxMouseEvent::*)() const) &wxMouseEvent::Aux2Up, "C++: wxMouseEvent::Aux2Up() const --> bool");
		cl.def("LeftDClick", (bool (wxMouseEvent::*)() const) &wxMouseEvent::LeftDClick, "C++: wxMouseEvent::LeftDClick() const --> bool");
		cl.def("MiddleDClick", (bool (wxMouseEvent::*)() const) &wxMouseEvent::MiddleDClick, "C++: wxMouseEvent::MiddleDClick() const --> bool");
		cl.def("RightDClick", (bool (wxMouseEvent::*)() const) &wxMouseEvent::RightDClick, "C++: wxMouseEvent::RightDClick() const --> bool");
		cl.def("Aux1DClick", (bool (wxMouseEvent::*)() const) &wxMouseEvent::Aux1DClick, "C++: wxMouseEvent::Aux1DClick() const --> bool");
		cl.def("Aux2DClick", (bool (wxMouseEvent::*)() const) &wxMouseEvent::Aux2DClick, "C++: wxMouseEvent::Aux2DClick() const --> bool");
		cl.def("Dragging", (bool (wxMouseEvent::*)() const) &wxMouseEvent::Dragging, "C++: wxMouseEvent::Dragging() const --> bool");
		cl.def("Moving", (bool (wxMouseEvent::*)() const) &wxMouseEvent::Moving, "C++: wxMouseEvent::Moving() const --> bool");
		cl.def("Entering", (bool (wxMouseEvent::*)() const) &wxMouseEvent::Entering, "C++: wxMouseEvent::Entering() const --> bool");
		cl.def("Leaving", (bool (wxMouseEvent::*)() const) &wxMouseEvent::Leaving, "C++: wxMouseEvent::Leaving() const --> bool");
		cl.def("GetClickCount", (int (wxMouseEvent::*)() const) &wxMouseEvent::GetClickCount, "C++: wxMouseEvent::GetClickCount() const --> int");
		cl.def("GetLogicalPosition", (class wxPoint (wxMouseEvent::*)(const class wxDC &) const) &wxMouseEvent::GetLogicalPosition, "C++: wxMouseEvent::GetLogicalPosition(const class wxDC &) const --> class wxPoint", pybind11::arg("dc"));
		cl.def("GetWheelRotation", (int (wxMouseEvent::*)() const) &wxMouseEvent::GetWheelRotation, "C++: wxMouseEvent::GetWheelRotation() const --> int");
		cl.def("GetWheelDelta", (int (wxMouseEvent::*)() const) &wxMouseEvent::GetWheelDelta, "C++: wxMouseEvent::GetWheelDelta() const --> int");
		cl.def("GetWheelAxis", (enum wxMouseWheelAxis (wxMouseEvent::*)() const) &wxMouseEvent::GetWheelAxis, "C++: wxMouseEvent::GetWheelAxis() const --> enum wxMouseWheelAxis");
		cl.def("GetLinesPerAction", (int (wxMouseEvent::*)() const) &wxMouseEvent::GetLinesPerAction, "C++: wxMouseEvent::GetLinesPerAction() const --> int");
		cl.def("GetColumnsPerAction", (int (wxMouseEvent::*)() const) &wxMouseEvent::GetColumnsPerAction, "C++: wxMouseEvent::GetColumnsPerAction() const --> int");
		cl.def("IsPageScroll", (bool (wxMouseEvent::*)() const) &wxMouseEvent::IsPageScroll, "C++: wxMouseEvent::IsPageScroll() const --> bool");
		cl.def("Clone", (class wxEvent * (wxMouseEvent::*)() const) &wxMouseEvent::Clone, "C++: wxMouseEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetEventCategory", (enum wxEventCategory (wxMouseEvent::*)() const) &wxMouseEvent::GetEventCategory, "C++: wxMouseEvent::GetEventCategory() const --> enum wxEventCategory");
		cl.def("assign", (class wxMouseEvent & (wxMouseEvent::*)(const class wxMouseEvent &)) &wxMouseEvent::operator=, "C++: wxMouseEvent::operator=(const class wxMouseEvent &) --> class wxMouseEvent &", pybind11::return_value_policy::automatic, pybind11::arg("event"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxMouseEvent::*)() const) &wxMouseEvent::GetClassInfo, "C++: wxMouseEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxMouseEvent::wxCreateObject, "C++: wxMouseEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxSetCursorEvent file: line:1861
		pybind11::class_<wxSetCursorEvent, std::shared_ptr<wxSetCursorEvent>, PyCallBack_wxSetCursorEvent, wxEvent> cl(M(""), "wxSetCursorEvent", "");
		cl.def( pybind11::init( [](){ return new wxSetCursorEvent(); }, [](){ return new PyCallBack_wxSetCursorEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxSetCursorEvent(a0); }, [](int const & a0){ return new PyCallBack_wxSetCursorEvent(a0); } ), "doc");
		cl.def( pybind11::init<int, int>(), pybind11::arg("x"), pybind11::arg("y") );

		cl.def( pybind11::init( [](PyCallBack_wxSetCursorEvent const &o){ return new PyCallBack_wxSetCursorEvent(o); } ) );
		cl.def( pybind11::init( [](wxSetCursorEvent const &o){ return new wxSetCursorEvent(o); } ) );
		cl.def("GetX", (int (wxSetCursorEvent::*)() const) &wxSetCursorEvent::GetX, "C++: wxSetCursorEvent::GetX() const --> int");
		cl.def("GetY", (int (wxSetCursorEvent::*)() const) &wxSetCursorEvent::GetY, "C++: wxSetCursorEvent::GetY() const --> int");
		cl.def("SetCursor", (void (wxSetCursorEvent::*)(const class wxCursor &)) &wxSetCursorEvent::SetCursor, "C++: wxSetCursorEvent::SetCursor(const class wxCursor &) --> void", pybind11::arg("cursor"));
		cl.def("GetCursor", (const class wxCursor & (wxSetCursorEvent::*)() const) &wxSetCursorEvent::GetCursor, "C++: wxSetCursorEvent::GetCursor() const --> const class wxCursor &", pybind11::return_value_policy::automatic);
		cl.def("HasCursor", (bool (wxSetCursorEvent::*)() const) &wxSetCursorEvent::HasCursor, "C++: wxSetCursorEvent::HasCursor() const --> bool");
		cl.def("Clone", (class wxEvent * (wxSetCursorEvent::*)() const) &wxSetCursorEvent::Clone, "C++: wxSetCursorEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxSetCursorEvent::*)() const) &wxSetCursorEvent::GetClassInfo, "C++: wxSetCursorEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxSetCursorEvent::wxCreateObject, "C++: wxSetCursorEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	// wxKeyCategoryFlags file: line:1907
	pybind11::enum_<wxKeyCategoryFlags>(M(""), "wxKeyCategoryFlags", pybind11::arithmetic(), "")
		.value("WXK_CATEGORY_ARROW", WXK_CATEGORY_ARROW)
		.value("WXK_CATEGORY_PAGING", WXK_CATEGORY_PAGING)
		.value("WXK_CATEGORY_JUMP", WXK_CATEGORY_JUMP)
		.value("WXK_CATEGORY_TAB", WXK_CATEGORY_TAB)
		.value("WXK_CATEGORY_CUT", WXK_CATEGORY_CUT)
		.value("WXK_CATEGORY_NAVIGATION", WXK_CATEGORY_NAVIGATION)
		.export_values();

;

	{ // wxKeyEvent file: line:1930
		pybind11::class_<wxKeyEvent, std::shared_ptr<wxKeyEvent>, PyCallBack_wxKeyEvent, wxEvent, wxKeyboardState> cl(M(""), "wxKeyEvent", "");
		cl.def( pybind11::init( [](){ return new wxKeyEvent(); }, [](){ return new PyCallBack_wxKeyEvent(); } ), "doc");
		cl.def( pybind11::init<int>(), pybind11::arg("keyType") );

		cl.def( pybind11::init( [](PyCallBack_wxKeyEvent const &o){ return new PyCallBack_wxKeyEvent(o); } ) );
		cl.def( pybind11::init( [](wxKeyEvent const &o){ return new wxKeyEvent(o); } ) );
		cl.def( pybind11::init<int, const class wxKeyEvent &>(), pybind11::arg("eventType"), pybind11::arg("evt") );

		cl.def_readwrite("m_x", &wxKeyEvent::m_x);
		cl.def_readwrite("m_y", &wxKeyEvent::m_y);
		cl.def_readwrite("m_keyCode", &wxKeyEvent::m_keyCode);
		cl.def_readwrite("m_uniChar", &wxKeyEvent::m_uniChar);
		cl.def_readwrite("m_rawCode", &wxKeyEvent::m_rawCode);
		cl.def_readwrite("m_rawFlags", &wxKeyEvent::m_rawFlags);
		cl.def("GetKeyCode", (int (wxKeyEvent::*)() const) &wxKeyEvent::GetKeyCode, "C++: wxKeyEvent::GetKeyCode() const --> int");
		cl.def("IsKeyInCategory", (bool (wxKeyEvent::*)(int) const) &wxKeyEvent::IsKeyInCategory, "C++: wxKeyEvent::IsKeyInCategory(int) const --> bool", pybind11::arg("category"));
		cl.def("GetUnicodeKey", (wchar_t (wxKeyEvent::*)() const) &wxKeyEvent::GetUnicodeKey, "C++: wxKeyEvent::GetUnicodeKey() const --> wchar_t");
		cl.def("GetRawKeyCode", (unsigned int (wxKeyEvent::*)() const) &wxKeyEvent::GetRawKeyCode, "C++: wxKeyEvent::GetRawKeyCode() const --> unsigned int");
		cl.def("GetRawKeyFlags", (unsigned int (wxKeyEvent::*)() const) &wxKeyEvent::GetRawKeyFlags, "C++: wxKeyEvent::GetRawKeyFlags() const --> unsigned int");
		cl.def("GetPosition", (void (wxKeyEvent::*)(int *, int *) const) &wxKeyEvent::GetPosition, "C++: wxKeyEvent::GetPosition(int *, int *) const --> void", pybind11::arg("xpos"), pybind11::arg("ypos"));
		cl.def("GetPosition", (void (wxKeyEvent::*)(long *, long *) const) &wxKeyEvent::GetPosition, "C++: wxKeyEvent::GetPosition(long *, long *) const --> void", pybind11::arg("xpos"), pybind11::arg("ypos"));
		cl.def("GetPosition", (class wxPoint (wxKeyEvent::*)() const) &wxKeyEvent::GetPosition, "C++: wxKeyEvent::GetPosition() const --> class wxPoint");
		cl.def("GetX", (int (wxKeyEvent::*)() const) &wxKeyEvent::GetX, "C++: wxKeyEvent::GetX() const --> int");
		cl.def("GetY", (int (wxKeyEvent::*)() const) &wxKeyEvent::GetY, "C++: wxKeyEvent::GetY() const --> int");
		cl.def("DoAllowNextEvent", (void (wxKeyEvent::*)()) &wxKeyEvent::DoAllowNextEvent, "C++: wxKeyEvent::DoAllowNextEvent() --> void");
		cl.def("IsNextEventAllowed", (bool (wxKeyEvent::*)() const) &wxKeyEvent::IsNextEventAllowed, "C++: wxKeyEvent::IsNextEventAllowed() const --> bool");
		cl.def("Clone", (class wxEvent * (wxKeyEvent::*)() const) &wxKeyEvent::Clone, "C++: wxKeyEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetEventCategory", (enum wxEventCategory (wxKeyEvent::*)() const) &wxKeyEvent::GetEventCategory, "C++: wxKeyEvent::GetEventCategory() const --> enum wxEventCategory");
		cl.def("assign", (class wxKeyEvent & (wxKeyEvent::*)(const class wxKeyEvent &)) &wxKeyEvent::operator=, "C++: wxKeyEvent::operator=(const class wxKeyEvent &) --> class wxKeyEvent &", pybind11::return_value_policy::automatic, pybind11::arg("evt"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxKeyEvent::*)() const) &wxKeyEvent::GetClassInfo, "C++: wxKeyEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxKeyEvent::wxCreateObject, "C++: wxKeyEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
}

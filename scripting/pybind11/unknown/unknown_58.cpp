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

// wxShowEvent file: line:2447
struct PyCallBack_wxShowEvent : public wxShowEvent {
	using wxShowEvent::wxShowEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxShowEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxShowEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxShowEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxShowEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxShowEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxShowEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxShowEvent *>(this), "CloneRefData");
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

// wxIconizeEvent file: line:2479
struct PyCallBack_wxIconizeEvent : public wxIconizeEvent {
	using wxIconizeEvent::wxIconizeEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIconizeEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxIconizeEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIconizeEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxIconizeEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIconizeEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIconizeEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIconizeEvent *>(this), "CloneRefData");
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

// wxMaximizeEvent file: line:2507
struct PyCallBack_wxMaximizeEvent : public wxMaximizeEvent {
	using wxMaximizeEvent::wxMaximizeEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMaximizeEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxMaximizeEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMaximizeEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxMaximizeEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMaximizeEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMaximizeEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMaximizeEvent *>(this), "CloneRefData");
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

// wxJoystickEvent file: line:2545
struct PyCallBack_wxJoystickEvent : public wxJoystickEvent {
	using wxJoystickEvent::wxJoystickEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxJoystickEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxJoystickEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxJoystickEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxJoystickEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxJoystickEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxJoystickEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxJoystickEvent *>(this), "CloneRefData");
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

// wxDropFilesEvent file: line:2624
struct PyCallBack_wxDropFilesEvent : public wxDropFilesEvent {
	using wxDropFilesEvent::wxDropFilesEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDropFilesEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxDropFilesEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDropFilesEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxDropFilesEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDropFilesEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDropFilesEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDropFilesEvent *>(this), "CloneRefData");
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

// wxUpdateUIEvent file: line:2688
struct PyCallBack_wxUpdateUIEvent : public wxUpdateUIEvent {
	using wxUpdateUIEvent::wxUpdateUIEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxUpdateUIEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxUpdateUIEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxUpdateUIEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxUpdateUIEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxUpdateUIEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxUpdateUIEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxUpdateUIEvent *>(this), "CloneRefData");
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

// wxSysColourChangedEvent file: line:2776
struct PyCallBack_wxSysColourChangedEvent : public wxSysColourChangedEvent {
	using wxSysColourChangedEvent::wxSysColourChangedEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxSysColourChangedEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxSysColourChangedEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxSysColourChangedEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxSysColourChangedEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxSysColourChangedEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxSysColourChangedEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxSysColourChangedEvent *>(this), "CloneRefData");
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

void bind_unknown_unknown_58(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxShowEvent file: line:2447
		pybind11::class_<wxShowEvent, std::shared_ptr<wxShowEvent>, PyCallBack_wxShowEvent, wxEvent> cl(M(""), "wxShowEvent", "");
		cl.def( pybind11::init( [](){ return new wxShowEvent(); }, [](){ return new PyCallBack_wxShowEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxShowEvent(a0); }, [](int const & a0){ return new PyCallBack_wxShowEvent(a0); } ), "doc");
		cl.def( pybind11::init<int, bool>(), pybind11::arg("winid"), pybind11::arg("show") );

		cl.def( pybind11::init( [](PyCallBack_wxShowEvent const &o){ return new PyCallBack_wxShowEvent(o); } ) );
		cl.def( pybind11::init( [](wxShowEvent const &o){ return new wxShowEvent(o); } ) );
		cl.def("SetShow", (void (wxShowEvent::*)(bool)) &wxShowEvent::SetShow, "C++: wxShowEvent::SetShow(bool) --> void", pybind11::arg("show"));
		cl.def("IsShown", (bool (wxShowEvent::*)() const) &wxShowEvent::IsShown, "C++: wxShowEvent::IsShown() const --> bool");
		cl.def("GetShow", (bool (wxShowEvent::*)() const) &wxShowEvent::GetShow, "C++: wxShowEvent::GetShow() const --> bool");
		cl.def("Clone", (class wxEvent * (wxShowEvent::*)() const) &wxShowEvent::Clone, "C++: wxShowEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxShowEvent::*)() const) &wxShowEvent::GetClassInfo, "C++: wxShowEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxShowEvent::wxCreateObject, "C++: wxShowEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxIconizeEvent file: line:2479
		pybind11::class_<wxIconizeEvent, std::shared_ptr<wxIconizeEvent>, PyCallBack_wxIconizeEvent, wxEvent> cl(M(""), "wxIconizeEvent", "");
		cl.def( pybind11::init( [](){ return new wxIconizeEvent(); }, [](){ return new PyCallBack_wxIconizeEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxIconizeEvent(a0); }, [](int const & a0){ return new PyCallBack_wxIconizeEvent(a0); } ), "doc");
		cl.def( pybind11::init<int, bool>(), pybind11::arg("winid"), pybind11::arg("iconized") );

		cl.def( pybind11::init( [](PyCallBack_wxIconizeEvent const &o){ return new PyCallBack_wxIconizeEvent(o); } ) );
		cl.def( pybind11::init( [](wxIconizeEvent const &o){ return new wxIconizeEvent(o); } ) );
		cl.def("Iconized", (bool (wxIconizeEvent::*)() const) &wxIconizeEvent::Iconized, "C++: wxIconizeEvent::Iconized() const --> bool");
		cl.def("IsIconized", (bool (wxIconizeEvent::*)() const) &wxIconizeEvent::IsIconized, "C++: wxIconizeEvent::IsIconized() const --> bool");
		cl.def("Clone", (class wxEvent * (wxIconizeEvent::*)() const) &wxIconizeEvent::Clone, "C++: wxIconizeEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxIconizeEvent::*)() const) &wxIconizeEvent::GetClassInfo, "C++: wxIconizeEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxIconizeEvent::wxCreateObject, "C++: wxIconizeEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxMaximizeEvent file: line:2507
		pybind11::class_<wxMaximizeEvent, std::shared_ptr<wxMaximizeEvent>, PyCallBack_wxMaximizeEvent, wxEvent> cl(M(""), "wxMaximizeEvent", "");
		cl.def( pybind11::init( [](){ return new wxMaximizeEvent(); }, [](){ return new PyCallBack_wxMaximizeEvent(); } ), "doc");
		cl.def( pybind11::init<int>(), pybind11::arg("winid") );

		cl.def( pybind11::init( [](PyCallBack_wxMaximizeEvent const &o){ return new PyCallBack_wxMaximizeEvent(o); } ) );
		cl.def( pybind11::init( [](wxMaximizeEvent const &o){ return new wxMaximizeEvent(o); } ) );
		cl.def("Clone", (class wxEvent * (wxMaximizeEvent::*)() const) &wxMaximizeEvent::Clone, "C++: wxMaximizeEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxMaximizeEvent::*)() const) &wxMaximizeEvent::GetClassInfo, "C++: wxMaximizeEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxMaximizeEvent::wxCreateObject, "C++: wxMaximizeEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxJoystickEvent file: line:2545
		pybind11::class_<wxJoystickEvent, std::shared_ptr<wxJoystickEvent>, PyCallBack_wxJoystickEvent, wxEvent> cl(M(""), "wxJoystickEvent", "");
		cl.def( pybind11::init( [](){ return new wxJoystickEvent(); }, [](){ return new PyCallBack_wxJoystickEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxJoystickEvent(a0); }, [](int const & a0){ return new PyCallBack_wxJoystickEvent(a0); } ), "doc");
		cl.def( pybind11::init( [](int const & a0, int const & a1){ return new wxJoystickEvent(a0, a1); }, [](int const & a0, int const & a1){ return new PyCallBack_wxJoystickEvent(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](int const & a0, int const & a1, int const & a2){ return new wxJoystickEvent(a0, a1, a2); }, [](int const & a0, int const & a1, int const & a2){ return new PyCallBack_wxJoystickEvent(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init<int, int, int, int>(), pybind11::arg("type"), pybind11::arg("state"), pybind11::arg("joystick"), pybind11::arg("change") );

		cl.def( pybind11::init( [](PyCallBack_wxJoystickEvent const &o){ return new PyCallBack_wxJoystickEvent(o); } ) );
		cl.def( pybind11::init( [](wxJoystickEvent const &o){ return new wxJoystickEvent(o); } ) );
		cl.def("GetPosition", (class wxPoint (wxJoystickEvent::*)() const) &wxJoystickEvent::GetPosition, "C++: wxJoystickEvent::GetPosition() const --> class wxPoint");
		cl.def("GetZPosition", (int (wxJoystickEvent::*)() const) &wxJoystickEvent::GetZPosition, "C++: wxJoystickEvent::GetZPosition() const --> int");
		cl.def("GetButtonState", (int (wxJoystickEvent::*)() const) &wxJoystickEvent::GetButtonState, "C++: wxJoystickEvent::GetButtonState() const --> int");
		cl.def("GetButtonChange", (int (wxJoystickEvent::*)() const) &wxJoystickEvent::GetButtonChange, "C++: wxJoystickEvent::GetButtonChange() const --> int");
		cl.def("GetJoystick", (int (wxJoystickEvent::*)() const) &wxJoystickEvent::GetJoystick, "C++: wxJoystickEvent::GetJoystick() const --> int");
		cl.def("SetJoystick", (void (wxJoystickEvent::*)(int)) &wxJoystickEvent::SetJoystick, "C++: wxJoystickEvent::SetJoystick(int) --> void", pybind11::arg("stick"));
		cl.def("SetButtonState", (void (wxJoystickEvent::*)(int)) &wxJoystickEvent::SetButtonState, "C++: wxJoystickEvent::SetButtonState(int) --> void", pybind11::arg("state"));
		cl.def("SetButtonChange", (void (wxJoystickEvent::*)(int)) &wxJoystickEvent::SetButtonChange, "C++: wxJoystickEvent::SetButtonChange(int) --> void", pybind11::arg("change"));
		cl.def("SetPosition", (void (wxJoystickEvent::*)(const class wxPoint &)) &wxJoystickEvent::SetPosition, "C++: wxJoystickEvent::SetPosition(const class wxPoint &) --> void", pybind11::arg("pos"));
		cl.def("SetZPosition", (void (wxJoystickEvent::*)(int)) &wxJoystickEvent::SetZPosition, "C++: wxJoystickEvent::SetZPosition(int) --> void", pybind11::arg("zPos"));
		cl.def("IsButton", (bool (wxJoystickEvent::*)() const) &wxJoystickEvent::IsButton, "C++: wxJoystickEvent::IsButton() const --> bool");
		cl.def("IsMove", (bool (wxJoystickEvent::*)() const) &wxJoystickEvent::IsMove, "C++: wxJoystickEvent::IsMove() const --> bool");
		cl.def("IsZMove", (bool (wxJoystickEvent::*)() const) &wxJoystickEvent::IsZMove, "C++: wxJoystickEvent::IsZMove() const --> bool");
		cl.def("ButtonDown", [](wxJoystickEvent const &o) -> bool { return o.ButtonDown(); }, "");
		cl.def("ButtonDown", (bool (wxJoystickEvent::*)(int) const) &wxJoystickEvent::ButtonDown, "C++: wxJoystickEvent::ButtonDown(int) const --> bool", pybind11::arg("but"));
		cl.def("ButtonUp", [](wxJoystickEvent const &o) -> bool { return o.ButtonUp(); }, "");
		cl.def("ButtonUp", (bool (wxJoystickEvent::*)(int) const) &wxJoystickEvent::ButtonUp, "C++: wxJoystickEvent::ButtonUp(int) const --> bool", pybind11::arg("but"));
		cl.def("ButtonIsDown", [](wxJoystickEvent const &o) -> bool { return o.ButtonIsDown(); }, "");
		cl.def("ButtonIsDown", (bool (wxJoystickEvent::*)(int) const) &wxJoystickEvent::ButtonIsDown, "C++: wxJoystickEvent::ButtonIsDown(int) const --> bool", pybind11::arg("but"));
		cl.def("Clone", (class wxEvent * (wxJoystickEvent::*)() const) &wxJoystickEvent::Clone, "C++: wxJoystickEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxJoystickEvent::*)() const) &wxJoystickEvent::GetClassInfo, "C++: wxJoystickEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxJoystickEvent::wxCreateObject, "C++: wxJoystickEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxDropFilesEvent file: line:2624
		pybind11::class_<wxDropFilesEvent, std::shared_ptr<wxDropFilesEvent>, PyCallBack_wxDropFilesEvent, wxEvent> cl(M(""), "wxDropFilesEvent", "");
		cl.def( pybind11::init( [](){ return new wxDropFilesEvent(); }, [](){ return new PyCallBack_wxDropFilesEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxDropFilesEvent(a0); }, [](int const & a0){ return new PyCallBack_wxDropFilesEvent(a0); } ), "doc");
		cl.def( pybind11::init( [](int const & a0, int const & a1){ return new wxDropFilesEvent(a0, a1); }, [](int const & a0, int const & a1){ return new PyCallBack_wxDropFilesEvent(a0, a1); } ), "doc");
		cl.def( pybind11::init<int, int, class wxString *>(), pybind11::arg("type"), pybind11::arg("noFiles"), pybind11::arg("files") );

		cl.def( pybind11::init( [](PyCallBack_wxDropFilesEvent const &o){ return new PyCallBack_wxDropFilesEvent(o); } ) );
		cl.def( pybind11::init( [](wxDropFilesEvent const &o){ return new wxDropFilesEvent(o); } ) );
		cl.def_readwrite("m_noFiles", &wxDropFilesEvent::m_noFiles);
		cl.def_readwrite("m_pos", &wxDropFilesEvent::m_pos);
		cl.def("GetPosition", (class wxPoint (wxDropFilesEvent::*)() const) &wxDropFilesEvent::GetPosition, "C++: wxDropFilesEvent::GetPosition() const --> class wxPoint");
		cl.def("GetNumberOfFiles", (int (wxDropFilesEvent::*)() const) &wxDropFilesEvent::GetNumberOfFiles, "C++: wxDropFilesEvent::GetNumberOfFiles() const --> int");
		cl.def("GetFiles", (class wxString * (wxDropFilesEvent::*)() const) &wxDropFilesEvent::GetFiles, "C++: wxDropFilesEvent::GetFiles() const --> class wxString *", pybind11::return_value_policy::automatic);
		cl.def("Clone", (class wxEvent * (wxDropFilesEvent::*)() const) &wxDropFilesEvent::Clone, "C++: wxDropFilesEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxDropFilesEvent::*)() const) &wxDropFilesEvent::GetClassInfo, "C++: wxDropFilesEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxDropFilesEvent::wxCreateObject, "C++: wxDropFilesEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	// wxUpdateUIMode file: line:2678
	pybind11::enum_<wxUpdateUIMode>(M(""), "wxUpdateUIMode", pybind11::arithmetic(), "")
		.value("wxUPDATE_UI_PROCESS_ALL", wxUPDATE_UI_PROCESS_ALL)
		.value("wxUPDATE_UI_PROCESS_SPECIFIED", wxUPDATE_UI_PROCESS_SPECIFIED)
		.export_values();

;

	{ // wxUpdateUIEvent file: line:2688
		pybind11::class_<wxUpdateUIEvent, std::shared_ptr<wxUpdateUIEvent>, PyCallBack_wxUpdateUIEvent, wxCommandEvent> cl(M(""), "wxUpdateUIEvent", "");
		cl.def( pybind11::init( [](){ return new wxUpdateUIEvent(); }, [](){ return new PyCallBack_wxUpdateUIEvent(); } ), "doc");
		cl.def( pybind11::init<int>(), pybind11::arg("commandId") );

		cl.def( pybind11::init( [](PyCallBack_wxUpdateUIEvent const &o){ return new PyCallBack_wxUpdateUIEvent(o); } ) );
		cl.def( pybind11::init( [](wxUpdateUIEvent const &o){ return new wxUpdateUIEvent(o); } ) );
		cl.def("GetChecked", (bool (wxUpdateUIEvent::*)() const) &wxUpdateUIEvent::GetChecked, "C++: wxUpdateUIEvent::GetChecked() const --> bool");
		cl.def("GetEnabled", (bool (wxUpdateUIEvent::*)() const) &wxUpdateUIEvent::GetEnabled, "C++: wxUpdateUIEvent::GetEnabled() const --> bool");
		cl.def("GetShown", (bool (wxUpdateUIEvent::*)() const) &wxUpdateUIEvent::GetShown, "C++: wxUpdateUIEvent::GetShown() const --> bool");
		cl.def("GetText", (class wxString (wxUpdateUIEvent::*)() const) &wxUpdateUIEvent::GetText, "C++: wxUpdateUIEvent::GetText() const --> class wxString");
		cl.def("GetSetText", (bool (wxUpdateUIEvent::*)() const) &wxUpdateUIEvent::GetSetText, "C++: wxUpdateUIEvent::GetSetText() const --> bool");
		cl.def("GetSetChecked", (bool (wxUpdateUIEvent::*)() const) &wxUpdateUIEvent::GetSetChecked, "C++: wxUpdateUIEvent::GetSetChecked() const --> bool");
		cl.def("GetSetEnabled", (bool (wxUpdateUIEvent::*)() const) &wxUpdateUIEvent::GetSetEnabled, "C++: wxUpdateUIEvent::GetSetEnabled() const --> bool");
		cl.def("GetSetShown", (bool (wxUpdateUIEvent::*)() const) &wxUpdateUIEvent::GetSetShown, "C++: wxUpdateUIEvent::GetSetShown() const --> bool");
		cl.def("Check", (void (wxUpdateUIEvent::*)(bool)) &wxUpdateUIEvent::Check, "C++: wxUpdateUIEvent::Check(bool) --> void", pybind11::arg("check"));
		cl.def("Enable", (void (wxUpdateUIEvent::*)(bool)) &wxUpdateUIEvent::Enable, "C++: wxUpdateUIEvent::Enable(bool) --> void", pybind11::arg("enable"));
		cl.def("Show", (void (wxUpdateUIEvent::*)(bool)) &wxUpdateUIEvent::Show, "C++: wxUpdateUIEvent::Show(bool) --> void", pybind11::arg("show"));
		cl.def("SetText", (void (wxUpdateUIEvent::*)(const class wxString &)) &wxUpdateUIEvent::SetText, "C++: wxUpdateUIEvent::SetText(const class wxString &) --> void", pybind11::arg("text"));
		cl.def_static("SetUpdateInterval", (void (*)(long)) &wxUpdateUIEvent::SetUpdateInterval, "C++: wxUpdateUIEvent::SetUpdateInterval(long) --> void", pybind11::arg("updateInterval"));
		cl.def_static("GetUpdateInterval", (long (*)()) &wxUpdateUIEvent::GetUpdateInterval, "C++: wxUpdateUIEvent::GetUpdateInterval() --> long");
		cl.def_static("CanUpdate", (bool (*)(class wxWindowBase *)) &wxUpdateUIEvent::CanUpdate, "C++: wxUpdateUIEvent::CanUpdate(class wxWindowBase *) --> bool", pybind11::arg("win"));
		cl.def_static("ResetUpdateTime", (void (*)()) &wxUpdateUIEvent::ResetUpdateTime, "C++: wxUpdateUIEvent::ResetUpdateTime() --> void");
		cl.def_static("SetMode", (void (*)(enum wxUpdateUIMode)) &wxUpdateUIEvent::SetMode, "C++: wxUpdateUIEvent::SetMode(enum wxUpdateUIMode) --> void", pybind11::arg("mode"));
		cl.def_static("GetMode", (enum wxUpdateUIMode (*)()) &wxUpdateUIEvent::GetMode, "C++: wxUpdateUIEvent::GetMode() --> enum wxUpdateUIMode");
		cl.def("Clone", (class wxEvent * (wxUpdateUIEvent::*)() const) &wxUpdateUIEvent::Clone, "C++: wxUpdateUIEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxUpdateUIEvent::*)() const) &wxUpdateUIEvent::GetClassInfo, "C++: wxUpdateUIEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxUpdateUIEvent::wxCreateObject, "C++: wxUpdateUIEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxSysColourChangedEvent file: line:2776
		pybind11::class_<wxSysColourChangedEvent, std::shared_ptr<wxSysColourChangedEvent>, PyCallBack_wxSysColourChangedEvent, wxEvent> cl(M(""), "wxSysColourChangedEvent", "");
		cl.def( pybind11::init( [](){ return new wxSysColourChangedEvent(); }, [](){ return new PyCallBack_wxSysColourChangedEvent(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxSysColourChangedEvent const &o){ return new PyCallBack_wxSysColourChangedEvent(o); } ) );
		cl.def( pybind11::init( [](wxSysColourChangedEvent const &o){ return new wxSysColourChangedEvent(o); } ) );
		cl.def("Clone", (class wxEvent * (wxSysColourChangedEvent::*)() const) &wxSysColourChangedEvent::Clone, "C++: wxSysColourChangedEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxSysColourChangedEvent::*)() const) &wxSysColourChangedEvent::GetClassInfo, "C++: wxSysColourChangedEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxSysColourChangedEvent::wxCreateObject, "C++: wxSysColourChangedEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
}

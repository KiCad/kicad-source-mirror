#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxEvent file: line:946
struct PyCallBack_wxEvent : public wxEvent {
	using wxEvent::wxEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxEvent::Clone\"");
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvent *>(this), "GetEventCategory");
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
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxEvent::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvent *>(this), "CloneRefData");
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

// wxIdleEvent file: line:1270
struct PyCallBack_wxIdleEvent : public wxIdleEvent {
	using wxIdleEvent::wxIdleEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIdleEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxIdleEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIdleEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxIdleEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIdleEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIdleEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIdleEvent *>(this), "CloneRefData");
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

// wxThreadEvent file: line:1306
struct PyCallBack_wxThreadEvent : public wxThreadEvent {
	using wxThreadEvent::wxThreadEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxThreadEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxThreadEvent::Clone();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxThreadEvent *>(this), "GetEventCategory");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxEventCategory>::value) {
				static pybind11::detail::override_caster_t<enum wxEventCategory> caster;
				return pybind11::detail::cast_ref<enum wxEventCategory>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxEventCategory>(std::move(o));
		}
		return wxThreadEvent::GetEventCategory();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxThreadEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxThreadEvent::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxThreadEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxThreadEvent *>(this), "CloneRefData");
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

// wxAsyncMethodCallEvent file: line:1347
struct PyCallBack_wxAsyncMethodCallEvent : public wxAsyncMethodCallEvent {
	using wxAsyncMethodCallEvent::wxAsyncMethodCallEvent;

	void Execute() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAsyncMethodCallEvent *>(this), "Execute");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAsyncMethodCallEvent::Execute\"");
	}
	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAsyncMethodCallEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxEvent::Clone\"");
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAsyncMethodCallEvent *>(this), "GetEventCategory");
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
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAsyncMethodCallEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxEvent::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAsyncMethodCallEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAsyncMethodCallEvent *>(this), "CloneRefData");
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

// wxCommandEvent file: line:1551
struct PyCallBack_wxCommandEvent : public wxCommandEvent {
	using wxCommandEvent::wxCommandEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCommandEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxCommandEvent::Clone();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCommandEvent *>(this), "GetEventCategory");
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
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCommandEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxCommandEvent::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCommandEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCommandEvent *>(this), "CloneRefData");
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

// wxNotifyEvent file: line:1603
struct PyCallBack_wxNotifyEvent : public wxNotifyEvent {
	using wxNotifyEvent::wxNotifyEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxNotifyEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxNotifyEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxNotifyEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxNotifyEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxNotifyEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxNotifyEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxNotifyEvent *>(this), "CloneRefData");
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

void bind_unknown_unknown_55(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxEventPropagation file: line:872
	pybind11::enum_<wxEventPropagation>(M(""), "wxEventPropagation", pybind11::arithmetic(), "")
		.value("wxEVENT_PROPAGATE_NONE", wxEVENT_PROPAGATE_NONE)
		.value("wxEVENT_PROPAGATE_MAX", wxEVENT_PROPAGATE_MAX)
		.export_values();

;

	// wxEventCategory file: line:883
	pybind11::enum_<wxEventCategory>(M(""), "wxEventCategory", pybind11::arithmetic(), "")
		.value("wxEVT_CATEGORY_UI", wxEVT_CATEGORY_UI)
		.value("wxEVT_CATEGORY_USER_INPUT", wxEVT_CATEGORY_USER_INPUT)
		.value("wxEVT_CATEGORY_SOCKET", wxEVT_CATEGORY_SOCKET)
		.value("wxEVT_CATEGORY_TIMER", wxEVT_CATEGORY_TIMER)
		.value("wxEVT_CATEGORY_THREAD", wxEVT_CATEGORY_THREAD)
		.value("wxEVT_CATEGORY_UNKNOWN", wxEVT_CATEGORY_UNKNOWN)
		.value("wxEVT_CATEGORY_CLIPBOARD", wxEVT_CATEGORY_CLIPBOARD)
		.value("wxEVT_CATEGORY_NATIVE_EVENTS", wxEVT_CATEGORY_NATIVE_EVENTS)
		.value("wxEVT_CATEGORY_ALL", wxEVT_CATEGORY_ALL)
		.export_values();

;

	{ // wxEvent file: line:946
		pybind11::class_<wxEvent, std::shared_ptr<wxEvent>, PyCallBack_wxEvent, wxObject> cl(M(""), "wxEvent", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new PyCallBack_wxEvent(a0); } ), "doc");
		cl.def( pybind11::init<int, int>(), pybind11::arg("winid"), pybind11::arg("commandType") );

		cl.def("SetEventType", (void (wxEvent::*)(int)) &wxEvent::SetEventType, "C++: wxEvent::SetEventType(int) --> void", pybind11::arg("typ"));
		cl.def("GetEventType", (int (wxEvent::*)() const) &wxEvent::GetEventType, "C++: wxEvent::GetEventType() const --> int");
		cl.def("GetEventObject", (class wxObject * (wxEvent::*)() const) &wxEvent::GetEventObject, "C++: wxEvent::GetEventObject() const --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("SetEventObject", (void (wxEvent::*)(class wxObject *)) &wxEvent::SetEventObject, "C++: wxEvent::SetEventObject(class wxObject *) --> void", pybind11::arg("obj"));
		cl.def("GetTimestamp", (long (wxEvent::*)() const) &wxEvent::GetTimestamp, "C++: wxEvent::GetTimestamp() const --> long");
		cl.def("SetTimestamp", [](wxEvent &o) -> void { return o.SetTimestamp(); }, "");
		cl.def("SetTimestamp", (void (wxEvent::*)(long)) &wxEvent::SetTimestamp, "C++: wxEvent::SetTimestamp(long) --> void", pybind11::arg("ts"));
		cl.def("GetId", (int (wxEvent::*)() const) &wxEvent::GetId, "C++: wxEvent::GetId() const --> int");
		cl.def("SetId", (void (wxEvent::*)(int)) &wxEvent::SetId, "C++: wxEvent::SetId(int) --> void", pybind11::arg("Id"));
		cl.def("GetEventUserData", (class wxObject * (wxEvent::*)() const) &wxEvent::GetEventUserData, "C++: wxEvent::GetEventUserData() const --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("Skip", [](wxEvent &o) -> void { return o.Skip(); }, "");
		cl.def("Skip", (void (wxEvent::*)(bool)) &wxEvent::Skip, "C++: wxEvent::Skip(bool) --> void", pybind11::arg("skip"));
		cl.def("GetSkipped", (bool (wxEvent::*)() const) &wxEvent::GetSkipped, "C++: wxEvent::GetSkipped() const --> bool");
		cl.def("Clone", (class wxEvent * (wxEvent::*)() const) &wxEvent::Clone, "C++: wxEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetEventCategory", (enum wxEventCategory (wxEvent::*)() const) &wxEvent::GetEventCategory, "C++: wxEvent::GetEventCategory() const --> enum wxEventCategory");
		cl.def("IsCommandEvent", (bool (wxEvent::*)() const) &wxEvent::IsCommandEvent, "C++: wxEvent::IsCommandEvent() const --> bool");
		cl.def("ShouldPropagate", (bool (wxEvent::*)() const) &wxEvent::ShouldPropagate, "C++: wxEvent::ShouldPropagate() const --> bool");
		cl.def("StopPropagation", (int (wxEvent::*)()) &wxEvent::StopPropagation, "C++: wxEvent::StopPropagation() --> int");
		cl.def("ResumePropagation", (void (wxEvent::*)(int)) &wxEvent::ResumePropagation, "C++: wxEvent::ResumePropagation(int) --> void", pybind11::arg("propagationLevel"));
		cl.def("GetPropagatedFrom", (class wxEvtHandler * (wxEvent::*)() const) &wxEvent::GetPropagatedFrom, "C++: wxEvent::GetPropagatedFrom() const --> class wxEvtHandler *", pybind11::return_value_policy::automatic);
		cl.def("WasProcessed", (bool (wxEvent::*)()) &wxEvent::WasProcessed, "C++: wxEvent::WasProcessed() --> bool");
		cl.def("SetWillBeProcessedAgain", (void (wxEvent::*)()) &wxEvent::SetWillBeProcessedAgain, "C++: wxEvent::SetWillBeProcessedAgain() --> void");
		cl.def("WillBeProcessedAgain", (bool (wxEvent::*)()) &wxEvent::WillBeProcessedAgain, "C++: wxEvent::WillBeProcessedAgain() --> bool");
		cl.def("ShouldProcessOnlyIn", (bool (wxEvent::*)(class wxEvtHandler *) const) &wxEvent::ShouldProcessOnlyIn, "C++: wxEvent::ShouldProcessOnlyIn(class wxEvtHandler *) const --> bool", pybind11::arg("h"));
		cl.def("DidntHonourProcessOnlyIn", (void (wxEvent::*)()) &wxEvent::DidntHonourProcessOnlyIn, "C++: wxEvent::DidntHonourProcessOnlyIn() --> void");
		cl.def("GetClassInfo", (class wxClassInfo * (wxEvent::*)() const) &wxEvent::GetClassInfo, "C++: wxEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
	}
	{ // wxPropagationDisabler file: line:1118
		pybind11::class_<wxPropagationDisabler, std::shared_ptr<wxPropagationDisabler>> cl(M(""), "wxPropagationDisabler", "");
		cl.def( pybind11::init<class wxEvent &>(), pybind11::arg("event") );

	}
	{ // wxPropagateOnce file: line:1142
		pybind11::class_<wxPropagateOnce, std::shared_ptr<wxPropagateOnce>> cl(M(""), "wxPropagateOnce", "");
		cl.def( pybind11::init( [](class wxEvent & a0){ return new wxPropagateOnce(a0); } ), "doc" , pybind11::arg("event"));
		cl.def( pybind11::init<class wxEvent &, class wxEvtHandler *>(), pybind11::arg("event"), pybind11::arg("handler") );

	}
	{ // wxEventProcessInHandlerOnly file: line:1174
		pybind11::class_<wxEventProcessInHandlerOnly, std::shared_ptr<wxEventProcessInHandlerOnly>> cl(M(""), "wxEventProcessInHandlerOnly", "");
		cl.def( pybind11::init<class wxEvent &, class wxEvtHandler *>(), pybind11::arg("event"), pybind11::arg("handler") );

	}
	{ // wxEventBasicPayloadMixin file: line:1197
		pybind11::class_<wxEventBasicPayloadMixin, std::shared_ptr<wxEventBasicPayloadMixin>> cl(M(""), "wxEventBasicPayloadMixin", "");
		cl.def( pybind11::init( [](){ return new wxEventBasicPayloadMixin(); } ) );
		cl.def( pybind11::init( [](wxEventBasicPayloadMixin const &o){ return new wxEventBasicPayloadMixin(o); } ) );
		cl.def("SetString", (void (wxEventBasicPayloadMixin::*)(const class wxString &)) &wxEventBasicPayloadMixin::SetString, "C++: wxEventBasicPayloadMixin::SetString(const class wxString &) --> void", pybind11::arg("s"));
		cl.def("GetString", (const class wxString & (wxEventBasicPayloadMixin::*)() const) &wxEventBasicPayloadMixin::GetString, "C++: wxEventBasicPayloadMixin::GetString() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("SetInt", (void (wxEventBasicPayloadMixin::*)(int)) &wxEventBasicPayloadMixin::SetInt, "C++: wxEventBasicPayloadMixin::SetInt(int) --> void", pybind11::arg("i"));
		cl.def("GetInt", (int (wxEventBasicPayloadMixin::*)() const) &wxEventBasicPayloadMixin::GetInt, "C++: wxEventBasicPayloadMixin::GetInt() const --> int");
		cl.def("SetExtraLong", (void (wxEventBasicPayloadMixin::*)(long)) &wxEventBasicPayloadMixin::SetExtraLong, "C++: wxEventBasicPayloadMixin::SetExtraLong(long) --> void", pybind11::arg("extraLong"));
		cl.def("GetExtraLong", (long (wxEventBasicPayloadMixin::*)() const) &wxEventBasicPayloadMixin::GetExtraLong, "C++: wxEventBasicPayloadMixin::GetExtraLong() const --> long");
	}
	{ // wxEventAnyPayloadMixin file: line:1225
		pybind11::class_<wxEventAnyPayloadMixin, std::shared_ptr<wxEventAnyPayloadMixin>, wxEventBasicPayloadMixin> cl(M(""), "wxEventAnyPayloadMixin", "");
		cl.def( pybind11::init( [](){ return new wxEventAnyPayloadMixin(); } ) );
		cl.def( pybind11::init( [](wxEventAnyPayloadMixin const &o){ return new wxEventAnyPayloadMixin(o); } ) );
	}
	// wxIdleMode file: line:1260
	pybind11::enum_<wxIdleMode>(M(""), "wxIdleMode", pybind11::arithmetic(), "")
		.value("wxIDLE_PROCESS_ALL", wxIDLE_PROCESS_ALL)
		.value("wxIDLE_PROCESS_SPECIFIED", wxIDLE_PROCESS_SPECIFIED)
		.export_values();

;

	{ // wxIdleEvent file: line:1270
		pybind11::class_<wxIdleEvent, std::shared_ptr<wxIdleEvent>, PyCallBack_wxIdleEvent, wxEvent> cl(M(""), "wxIdleEvent", "");
		cl.def( pybind11::init( [](){ return new wxIdleEvent(); }, [](){ return new PyCallBack_wxIdleEvent(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxIdleEvent const &o){ return new PyCallBack_wxIdleEvent(o); } ) );
		cl.def( pybind11::init( [](wxIdleEvent const &o){ return new wxIdleEvent(o); } ) );
		cl.def("RequestMore", [](wxIdleEvent &o) -> void { return o.RequestMore(); }, "");
		cl.def("RequestMore", (void (wxIdleEvent::*)(bool)) &wxIdleEvent::RequestMore, "C++: wxIdleEvent::RequestMore(bool) --> void", pybind11::arg("needMore"));
		cl.def("MoreRequested", (bool (wxIdleEvent::*)() const) &wxIdleEvent::MoreRequested, "C++: wxIdleEvent::MoreRequested() const --> bool");
		cl.def("Clone", (class wxEvent * (wxIdleEvent::*)() const) &wxIdleEvent::Clone, "C++: wxIdleEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def_static("SetMode", (void (*)(enum wxIdleMode)) &wxIdleEvent::SetMode, "C++: wxIdleEvent::SetMode(enum wxIdleMode) --> void", pybind11::arg("mode"));
		cl.def_static("GetMode", (enum wxIdleMode (*)()) &wxIdleEvent::GetMode, "C++: wxIdleEvent::GetMode() --> enum wxIdleMode");
		cl.def("GetClassInfo", (class wxClassInfo * (wxIdleEvent::*)() const) &wxIdleEvent::GetClassInfo, "C++: wxIdleEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxIdleEvent::wxCreateObject, "C++: wxIdleEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxThreadEvent file: line:1306
		pybind11::class_<wxThreadEvent, std::shared_ptr<wxThreadEvent>, PyCallBack_wxThreadEvent, wxEvent, wxEventAnyPayloadMixin> cl(M(""), "wxThreadEvent", "");
		cl.def( pybind11::init( [](){ return new wxThreadEvent(); }, [](){ return new PyCallBack_wxThreadEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxThreadEvent(a0); }, [](int const & a0){ return new PyCallBack_wxThreadEvent(a0); } ), "doc");
		cl.def( pybind11::init<int, int>(), pybind11::arg("eventType"), pybind11::arg("id") );

		cl.def( pybind11::init( [](PyCallBack_wxThreadEvent const &o){ return new PyCallBack_wxThreadEvent(o); } ) );
		cl.def( pybind11::init( [](wxThreadEvent const &o){ return new wxThreadEvent(o); } ) );
		cl.def("Clone", (class wxEvent * (wxThreadEvent::*)() const) &wxThreadEvent::Clone, "C++: wxThreadEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetEventCategory", (enum wxEventCategory (wxThreadEvent::*)() const) &wxThreadEvent::GetEventCategory, "C++: wxThreadEvent::GetEventCategory() const --> enum wxEventCategory");
		cl.def("GetClassInfo", (class wxClassInfo * (wxThreadEvent::*)() const) &wxThreadEvent::GetClassInfo, "C++: wxThreadEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxThreadEvent::wxCreateObject, "C++: wxThreadEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxAsyncMethodCallEvent file: line:1347
		pybind11::class_<wxAsyncMethodCallEvent, std::shared_ptr<wxAsyncMethodCallEvent>, PyCallBack_wxAsyncMethodCallEvent, wxEvent> cl(M(""), "wxAsyncMethodCallEvent", "");
		cl.def( pybind11::init<class wxObject *>(), pybind11::arg("object") );

		cl.def(pybind11::init<PyCallBack_wxAsyncMethodCallEvent const &>());
		cl.def("Execute", (void (wxAsyncMethodCallEvent::*)()) &wxAsyncMethodCallEvent::Execute, "C++: wxAsyncMethodCallEvent::Execute() --> void");
		cl.def("assign", (class wxAsyncMethodCallEvent & (wxAsyncMethodCallEvent::*)(const class wxAsyncMethodCallEvent &)) &wxAsyncMethodCallEvent::operator=, "C++: wxAsyncMethodCallEvent::operator=(const class wxAsyncMethodCallEvent &) --> class wxAsyncMethodCallEvent &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxCommandEvent file: line:1551
		pybind11::class_<wxCommandEvent, std::shared_ptr<wxCommandEvent>, PyCallBack_wxCommandEvent, wxEvent, wxEventBasicPayloadMixin> cl(M(""), "wxCommandEvent", "");
		cl.def( pybind11::init( [](){ return new wxCommandEvent(); }, [](){ return new PyCallBack_wxCommandEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxCommandEvent(a0); }, [](int const & a0){ return new PyCallBack_wxCommandEvent(a0); } ), "doc");
		cl.def( pybind11::init<int, int>(), pybind11::arg("commandType"), pybind11::arg("winid") );

		cl.def( pybind11::init( [](PyCallBack_wxCommandEvent const &o){ return new PyCallBack_wxCommandEvent(o); } ) );
		cl.def( pybind11::init( [](wxCommandEvent const &o){ return new wxCommandEvent(o); } ) );
		cl.def("SetClientData", (void (wxCommandEvent::*)(void *)) &wxCommandEvent::SetClientData, "C++: wxCommandEvent::SetClientData(void *) --> void", pybind11::arg("clientData"));
		cl.def("GetClientData", (void * (wxCommandEvent::*)() const) &wxCommandEvent::GetClientData, "C++: wxCommandEvent::GetClientData() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("SetClientObject", (void (wxCommandEvent::*)(class wxClientData *)) &wxCommandEvent::SetClientObject, "C++: wxCommandEvent::SetClientObject(class wxClientData *) --> void", pybind11::arg("clientObject"));
		cl.def("GetClientObject", (class wxClientData * (wxCommandEvent::*)() const) &wxCommandEvent::GetClientObject, "C++: wxCommandEvent::GetClientObject() const --> class wxClientData *", pybind11::return_value_policy::automatic);
		cl.def("GetString", (class wxString (wxCommandEvent::*)() const) &wxCommandEvent::GetString, "C++: wxCommandEvent::GetString() const --> class wxString");
		cl.def("GetSelection", (int (wxCommandEvent::*)() const) &wxCommandEvent::GetSelection, "C++: wxCommandEvent::GetSelection() const --> int");
		cl.def("IsChecked", (bool (wxCommandEvent::*)() const) &wxCommandEvent::IsChecked, "C++: wxCommandEvent::IsChecked() const --> bool");
		cl.def("IsSelection", (bool (wxCommandEvent::*)() const) &wxCommandEvent::IsSelection, "C++: wxCommandEvent::IsSelection() const --> bool");
		cl.def("Clone", (class wxEvent * (wxCommandEvent::*)() const) &wxCommandEvent::Clone, "C++: wxCommandEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetEventCategory", (enum wxEventCategory (wxCommandEvent::*)() const) &wxCommandEvent::GetEventCategory, "C++: wxCommandEvent::GetEventCategory() const --> enum wxEventCategory");
		cl.def("GetClassInfo", (class wxClassInfo * (wxCommandEvent::*)() const) &wxCommandEvent::GetClassInfo, "C++: wxCommandEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxCommandEvent::wxCreateObject, "C++: wxCommandEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxNotifyEvent file: line:1603
		pybind11::class_<wxNotifyEvent, std::shared_ptr<wxNotifyEvent>, PyCallBack_wxNotifyEvent, wxCommandEvent> cl(M(""), "wxNotifyEvent", "");
		cl.def( pybind11::init( [](){ return new wxNotifyEvent(); }, [](){ return new PyCallBack_wxNotifyEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxNotifyEvent(a0); }, [](int const & a0){ return new PyCallBack_wxNotifyEvent(a0); } ), "doc");
		cl.def( pybind11::init<int, int>(), pybind11::arg("commandType"), pybind11::arg("winid") );

		cl.def( pybind11::init( [](PyCallBack_wxNotifyEvent const &o){ return new PyCallBack_wxNotifyEvent(o); } ) );
		cl.def( pybind11::init( [](wxNotifyEvent const &o){ return new wxNotifyEvent(o); } ) );
		cl.def("Veto", (void (wxNotifyEvent::*)()) &wxNotifyEvent::Veto, "C++: wxNotifyEvent::Veto() --> void");
		cl.def("Allow", (void (wxNotifyEvent::*)()) &wxNotifyEvent::Allow, "C++: wxNotifyEvent::Allow() --> void");
		cl.def("IsAllowed", (bool (wxNotifyEvent::*)() const) &wxNotifyEvent::IsAllowed, "C++: wxNotifyEvent::IsAllowed() const --> bool");
		cl.def("Clone", (class wxEvent * (wxNotifyEvent::*)() const) &wxNotifyEvent::Clone, "C++: wxNotifyEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxNotifyEvent::*)() const) &wxNotifyEvent::GetClassInfo, "C++: wxNotifyEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxNotifyEvent::wxCreateObject, "C++: wxNotifyEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
}

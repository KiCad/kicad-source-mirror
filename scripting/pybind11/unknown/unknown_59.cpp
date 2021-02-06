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

// wxMouseCaptureChangedEvent file: line:2795
struct PyCallBack_wxMouseCaptureChangedEvent : public wxMouseCaptureChangedEvent {
	using wxMouseCaptureChangedEvent::wxMouseCaptureChangedEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMouseCaptureChangedEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxMouseCaptureChangedEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMouseCaptureChangedEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxMouseCaptureChangedEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMouseCaptureChangedEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMouseCaptureChangedEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMouseCaptureChangedEvent *>(this), "CloneRefData");
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

// wxMouseCaptureLostEvent file: line:2825
struct PyCallBack_wxMouseCaptureLostEvent : public wxMouseCaptureLostEvent {
	using wxMouseCaptureLostEvent::wxMouseCaptureLostEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMouseCaptureLostEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxMouseCaptureLostEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMouseCaptureLostEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxMouseCaptureLostEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMouseCaptureLostEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMouseCaptureLostEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMouseCaptureLostEvent *>(this), "CloneRefData");
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

// wxDisplayChangedEvent file: line:2844
struct PyCallBack_wxDisplayChangedEvent : public wxDisplayChangedEvent {
	using wxDisplayChangedEvent::wxDisplayChangedEvent;

	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDisplayChangedEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxDisplayChangedEvent::GetClassInfo();
	}
	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDisplayChangedEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxDisplayChangedEvent::Clone();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDisplayChangedEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDisplayChangedEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDisplayChangedEvent *>(this), "CloneRefData");
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

// wxPaletteChangedEvent file: line:2861
struct PyCallBack_wxPaletteChangedEvent : public wxPaletteChangedEvent {
	using wxPaletteChangedEvent::wxPaletteChangedEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaletteChangedEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxPaletteChangedEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaletteChangedEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxPaletteChangedEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaletteChangedEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaletteChangedEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaletteChangedEvent *>(this), "CloneRefData");
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

// wxQueryNewPaletteEvent file: line:2891
struct PyCallBack_wxQueryNewPaletteEvent : public wxQueryNewPaletteEvent {
	using wxQueryNewPaletteEvent::wxQueryNewPaletteEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxQueryNewPaletteEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxQueryNewPaletteEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxQueryNewPaletteEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxQueryNewPaletteEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxQueryNewPaletteEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxQueryNewPaletteEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxQueryNewPaletteEvent *>(this), "CloneRefData");
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

// wxNavigationKeyEvent file: line:2921
struct PyCallBack_wxNavigationKeyEvent : public wxNavigationKeyEvent {
	using wxNavigationKeyEvent::wxNavigationKeyEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxNavigationKeyEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxNavigationKeyEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxNavigationKeyEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxNavigationKeyEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxNavigationKeyEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxNavigationKeyEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxNavigationKeyEvent *>(this), "CloneRefData");
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

// wxWindowCreateEvent file: line:2993
struct PyCallBack_wxWindowCreateEvent : public wxWindowCreateEvent {
	using wxWindowCreateEvent::wxWindowCreateEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowCreateEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxWindowCreateEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowCreateEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxWindowCreateEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowCreateEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowCreateEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowCreateEvent *>(this), "CloneRefData");
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

// wxWindowDestroyEvent file: line:3006
struct PyCallBack_wxWindowDestroyEvent : public wxWindowDestroyEvent {
	using wxWindowDestroyEvent::wxWindowDestroyEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowDestroyEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxWindowDestroyEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowDestroyEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxWindowDestroyEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowDestroyEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowDestroyEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowDestroyEvent *>(this), "CloneRefData");
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

// wxHelpEvent file: line:3025
struct PyCallBack_wxHelpEvent : public wxHelpEvent {
	using wxHelpEvent::wxHelpEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxHelpEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxHelpEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxHelpEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxHelpEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxHelpEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxHelpEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxHelpEvent *>(this), "CloneRefData");
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

void bind_unknown_unknown_59(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxMouseCaptureChangedEvent file: line:2795
		pybind11::class_<wxMouseCaptureChangedEvent, std::shared_ptr<wxMouseCaptureChangedEvent>, PyCallBack_wxMouseCaptureChangedEvent, wxEvent> cl(M(""), "wxMouseCaptureChangedEvent", "");
		cl.def( pybind11::init( [](){ return new wxMouseCaptureChangedEvent(); }, [](){ return new PyCallBack_wxMouseCaptureChangedEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxMouseCaptureChangedEvent(a0); }, [](int const & a0){ return new PyCallBack_wxMouseCaptureChangedEvent(a0); } ), "doc");
		cl.def( pybind11::init<int, class wxWindow *>(), pybind11::arg("winid"), pybind11::arg("gainedCapture") );

		cl.def( pybind11::init( [](PyCallBack_wxMouseCaptureChangedEvent const &o){ return new PyCallBack_wxMouseCaptureChangedEvent(o); } ) );
		cl.def( pybind11::init( [](wxMouseCaptureChangedEvent const &o){ return new wxMouseCaptureChangedEvent(o); } ) );
		cl.def("Clone", (class wxEvent * (wxMouseCaptureChangedEvent::*)() const) &wxMouseCaptureChangedEvent::Clone, "C++: wxMouseCaptureChangedEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetCapturedWindow", (class wxWindow * (wxMouseCaptureChangedEvent::*)() const) &wxMouseCaptureChangedEvent::GetCapturedWindow, "C++: wxMouseCaptureChangedEvent::GetCapturedWindow() const --> class wxWindow *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxMouseCaptureChangedEvent::*)() const) &wxMouseCaptureChangedEvent::GetClassInfo, "C++: wxMouseCaptureChangedEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxMouseCaptureChangedEvent::wxCreateObject, "C++: wxMouseCaptureChangedEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxMouseCaptureLostEvent file: line:2825
		pybind11::class_<wxMouseCaptureLostEvent, std::shared_ptr<wxMouseCaptureLostEvent>, PyCallBack_wxMouseCaptureLostEvent, wxEvent> cl(M(""), "wxMouseCaptureLostEvent", "");
		cl.def( pybind11::init( [](){ return new wxMouseCaptureLostEvent(); }, [](){ return new PyCallBack_wxMouseCaptureLostEvent(); } ), "doc");
		cl.def( pybind11::init<int>(), pybind11::arg("winid") );

		cl.def( pybind11::init( [](PyCallBack_wxMouseCaptureLostEvent const &o){ return new PyCallBack_wxMouseCaptureLostEvent(o); } ) );
		cl.def( pybind11::init( [](wxMouseCaptureLostEvent const &o){ return new wxMouseCaptureLostEvent(o); } ) );
		cl.def("Clone", (class wxEvent * (wxMouseCaptureLostEvent::*)() const) &wxMouseCaptureLostEvent::Clone, "C++: wxMouseCaptureLostEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxMouseCaptureLostEvent::*)() const) &wxMouseCaptureLostEvent::GetClassInfo, "C++: wxMouseCaptureLostEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxMouseCaptureLostEvent::wxCreateObject, "C++: wxMouseCaptureLostEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxDisplayChangedEvent file: line:2844
		pybind11::class_<wxDisplayChangedEvent, std::shared_ptr<wxDisplayChangedEvent>, PyCallBack_wxDisplayChangedEvent, wxEvent> cl(M(""), "wxDisplayChangedEvent", "");
		cl.def( pybind11::init( [](){ return new wxDisplayChangedEvent(); }, [](){ return new PyCallBack_wxDisplayChangedEvent(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxDisplayChangedEvent const &o){ return new PyCallBack_wxDisplayChangedEvent(o); } ) );
		cl.def( pybind11::init( [](wxDisplayChangedEvent const &o){ return new wxDisplayChangedEvent(o); } ) );
		cl.def("GetClassInfo", (class wxClassInfo * (wxDisplayChangedEvent::*)() const) &wxDisplayChangedEvent::GetClassInfo, "C++: wxDisplayChangedEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxDisplayChangedEvent::wxCreateObject, "C++: wxDisplayChangedEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("Clone", (class wxEvent * (wxDisplayChangedEvent::*)() const) &wxDisplayChangedEvent::Clone, "C++: wxDisplayChangedEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
	}
	{ // wxPaletteChangedEvent file: line:2861
		pybind11::class_<wxPaletteChangedEvent, std::shared_ptr<wxPaletteChangedEvent>, PyCallBack_wxPaletteChangedEvent, wxEvent> cl(M(""), "wxPaletteChangedEvent", "");
		cl.def( pybind11::init( [](){ return new wxPaletteChangedEvent(); }, [](){ return new PyCallBack_wxPaletteChangedEvent(); } ), "doc");
		cl.def( pybind11::init<int>(), pybind11::arg("winid") );

		cl.def( pybind11::init( [](PyCallBack_wxPaletteChangedEvent const &o){ return new PyCallBack_wxPaletteChangedEvent(o); } ) );
		cl.def( pybind11::init( [](wxPaletteChangedEvent const &o){ return new wxPaletteChangedEvent(o); } ) );
		cl.def("SetChangedWindow", (void (wxPaletteChangedEvent::*)(class wxWindow *)) &wxPaletteChangedEvent::SetChangedWindow, "C++: wxPaletteChangedEvent::SetChangedWindow(class wxWindow *) --> void", pybind11::arg("win"));
		cl.def("GetChangedWindow", (class wxWindow * (wxPaletteChangedEvent::*)() const) &wxPaletteChangedEvent::GetChangedWindow, "C++: wxPaletteChangedEvent::GetChangedWindow() const --> class wxWindow *", pybind11::return_value_policy::automatic);
		cl.def("Clone", (class wxEvent * (wxPaletteChangedEvent::*)() const) &wxPaletteChangedEvent::Clone, "C++: wxPaletteChangedEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxPaletteChangedEvent::*)() const) &wxPaletteChangedEvent::GetClassInfo, "C++: wxPaletteChangedEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxPaletteChangedEvent::wxCreateObject, "C++: wxPaletteChangedEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxQueryNewPaletteEvent file: line:2891
		pybind11::class_<wxQueryNewPaletteEvent, std::shared_ptr<wxQueryNewPaletteEvent>, PyCallBack_wxQueryNewPaletteEvent, wxEvent> cl(M(""), "wxQueryNewPaletteEvent", "");
		cl.def( pybind11::init( [](){ return new wxQueryNewPaletteEvent(); }, [](){ return new PyCallBack_wxQueryNewPaletteEvent(); } ), "doc");
		cl.def( pybind11::init<int>(), pybind11::arg("winid") );

		cl.def( pybind11::init( [](PyCallBack_wxQueryNewPaletteEvent const &o){ return new PyCallBack_wxQueryNewPaletteEvent(o); } ) );
		cl.def( pybind11::init( [](wxQueryNewPaletteEvent const &o){ return new wxQueryNewPaletteEvent(o); } ) );
		cl.def("SetPaletteRealized", (void (wxQueryNewPaletteEvent::*)(bool)) &wxQueryNewPaletteEvent::SetPaletteRealized, "C++: wxQueryNewPaletteEvent::SetPaletteRealized(bool) --> void", pybind11::arg("realized"));
		cl.def("GetPaletteRealized", (bool (wxQueryNewPaletteEvent::*)() const) &wxQueryNewPaletteEvent::GetPaletteRealized, "C++: wxQueryNewPaletteEvent::GetPaletteRealized() const --> bool");
		cl.def("Clone", (class wxEvent * (wxQueryNewPaletteEvent::*)() const) &wxQueryNewPaletteEvent::Clone, "C++: wxQueryNewPaletteEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxQueryNewPaletteEvent::*)() const) &wxQueryNewPaletteEvent::GetClassInfo, "C++: wxQueryNewPaletteEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxQueryNewPaletteEvent::wxCreateObject, "C++: wxQueryNewPaletteEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxNavigationKeyEvent file: line:2921
		pybind11::class_<wxNavigationKeyEvent, std::shared_ptr<wxNavigationKeyEvent>, PyCallBack_wxNavigationKeyEvent, wxEvent> cl(M(""), "wxNavigationKeyEvent", "");
		cl.def( pybind11::init( [](){ return new wxNavigationKeyEvent(); }, [](){ return new PyCallBack_wxNavigationKeyEvent(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxNavigationKeyEvent const &o){ return new PyCallBack_wxNavigationKeyEvent(o); } ) );
		cl.def( pybind11::init( [](wxNavigationKeyEvent const &o){ return new wxNavigationKeyEvent(o); } ) );

		pybind11::enum_<wxNavigationKeyEvent::wxNavigationKeyEventFlags>(cl, "wxNavigationKeyEventFlags", pybind11::arithmetic(), "")
			.value("IsBackward", wxNavigationKeyEvent::IsBackward)
			.value("IsForward", wxNavigationKeyEvent::IsForward)
			.value("WinChange", wxNavigationKeyEvent::WinChange)
			.value("FromTab", wxNavigationKeyEvent::FromTab)
			.export_values();

		cl.def_readwrite("m_flags", &wxNavigationKeyEvent::m_flags);
		cl.def("GetDirection", (bool (wxNavigationKeyEvent::*)() const) &wxNavigationKeyEvent::GetDirection, "C++: wxNavigationKeyEvent::GetDirection() const --> bool");
		cl.def("SetDirection", (void (wxNavigationKeyEvent::*)(bool)) &wxNavigationKeyEvent::SetDirection, "C++: wxNavigationKeyEvent::SetDirection(bool) --> void", pybind11::arg("bForward"));
		cl.def("IsWindowChange", (bool (wxNavigationKeyEvent::*)() const) &wxNavigationKeyEvent::IsWindowChange, "C++: wxNavigationKeyEvent::IsWindowChange() const --> bool");
		cl.def("SetWindowChange", (void (wxNavigationKeyEvent::*)(bool)) &wxNavigationKeyEvent::SetWindowChange, "C++: wxNavigationKeyEvent::SetWindowChange(bool) --> void", pybind11::arg("bIs"));
		cl.def("IsFromTab", (bool (wxNavigationKeyEvent::*)() const) &wxNavigationKeyEvent::IsFromTab, "C++: wxNavigationKeyEvent::IsFromTab() const --> bool");
		cl.def("SetFromTab", (void (wxNavigationKeyEvent::*)(bool)) &wxNavigationKeyEvent::SetFromTab, "C++: wxNavigationKeyEvent::SetFromTab(bool) --> void", pybind11::arg("bIs"));
		cl.def("GetCurrentFocus", (class wxWindow * (wxNavigationKeyEvent::*)() const) &wxNavigationKeyEvent::GetCurrentFocus, "C++: wxNavigationKeyEvent::GetCurrentFocus() const --> class wxWindow *", pybind11::return_value_policy::automatic);
		cl.def("SetCurrentFocus", (void (wxNavigationKeyEvent::*)(class wxWindow *)) &wxNavigationKeyEvent::SetCurrentFocus, "C++: wxNavigationKeyEvent::SetCurrentFocus(class wxWindow *) --> void", pybind11::arg("win"));
		cl.def("SetFlags", (void (wxNavigationKeyEvent::*)(long)) &wxNavigationKeyEvent::SetFlags, "C++: wxNavigationKeyEvent::SetFlags(long) --> void", pybind11::arg("flags"));
		cl.def("Clone", (class wxEvent * (wxNavigationKeyEvent::*)() const) &wxNavigationKeyEvent::Clone, "C++: wxNavigationKeyEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxNavigationKeyEvent::*)() const) &wxNavigationKeyEvent::GetClassInfo, "C++: wxNavigationKeyEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxNavigationKeyEvent::wxCreateObject, "C++: wxNavigationKeyEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxWindowCreateEvent file: line:2993
		pybind11::class_<wxWindowCreateEvent, std::shared_ptr<wxWindowCreateEvent>, PyCallBack_wxWindowCreateEvent, wxCommandEvent> cl(M(""), "wxWindowCreateEvent", "");
		cl.def( pybind11::init( [](){ return new wxWindowCreateEvent(); }, [](){ return new PyCallBack_wxWindowCreateEvent(); } ), "doc");
		cl.def( pybind11::init<class wxWindow *>(), pybind11::arg("win") );

		cl.def( pybind11::init( [](PyCallBack_wxWindowCreateEvent const &o){ return new PyCallBack_wxWindowCreateEvent(o); } ) );
		cl.def( pybind11::init( [](wxWindowCreateEvent const &o){ return new wxWindowCreateEvent(o); } ) );
		cl.def("GetWindow", (class wxWindow * (wxWindowCreateEvent::*)() const) &wxWindowCreateEvent::GetWindow, "C++: wxWindowCreateEvent::GetWindow() const --> class wxWindow *", pybind11::return_value_policy::automatic);
		cl.def("Clone", (class wxEvent * (wxWindowCreateEvent::*)() const) &wxWindowCreateEvent::Clone, "C++: wxWindowCreateEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxWindowCreateEvent::*)() const) &wxWindowCreateEvent::GetClassInfo, "C++: wxWindowCreateEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxWindowCreateEvent::wxCreateObject, "C++: wxWindowCreateEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxWindowDestroyEvent file: line:3006
		pybind11::class_<wxWindowDestroyEvent, std::shared_ptr<wxWindowDestroyEvent>, PyCallBack_wxWindowDestroyEvent, wxCommandEvent> cl(M(""), "wxWindowDestroyEvent", "");
		cl.def( pybind11::init( [](){ return new wxWindowDestroyEvent(); }, [](){ return new PyCallBack_wxWindowDestroyEvent(); } ), "doc");
		cl.def( pybind11::init<class wxWindow *>(), pybind11::arg("win") );

		cl.def( pybind11::init( [](PyCallBack_wxWindowDestroyEvent const &o){ return new PyCallBack_wxWindowDestroyEvent(o); } ) );
		cl.def( pybind11::init( [](wxWindowDestroyEvent const &o){ return new wxWindowDestroyEvent(o); } ) );
		cl.def("GetWindow", (class wxWindow * (wxWindowDestroyEvent::*)() const) &wxWindowDestroyEvent::GetWindow, "C++: wxWindowDestroyEvent::GetWindow() const --> class wxWindow *", pybind11::return_value_policy::automatic);
		cl.def("Clone", (class wxEvent * (wxWindowDestroyEvent::*)() const) &wxWindowDestroyEvent::Clone, "C++: wxWindowDestroyEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxWindowDestroyEvent::*)() const) &wxWindowDestroyEvent::GetClassInfo, "C++: wxWindowDestroyEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxWindowDestroyEvent::wxCreateObject, "C++: wxWindowDestroyEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxHelpEvent file: line:3025
		pybind11::class_<wxHelpEvent, std::shared_ptr<wxHelpEvent>, PyCallBack_wxHelpEvent, wxCommandEvent> cl(M(""), "wxHelpEvent", "");
		cl.def( pybind11::init( [](){ return new wxHelpEvent(); }, [](){ return new PyCallBack_wxHelpEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxHelpEvent(a0); }, [](int const & a0){ return new PyCallBack_wxHelpEvent(a0); } ), "doc");
		cl.def( pybind11::init( [](int const & a0, int const & a1){ return new wxHelpEvent(a0, a1); }, [](int const & a0, int const & a1){ return new PyCallBack_wxHelpEvent(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](int const & a0, int const & a1, const class wxPoint & a2){ return new wxHelpEvent(a0, a1, a2); }, [](int const & a0, int const & a1, const class wxPoint & a2){ return new PyCallBack_wxHelpEvent(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init<int, int, const class wxPoint &, enum wxHelpEvent::Origin>(), pybind11::arg("type"), pybind11::arg("winid"), pybind11::arg("pt"), pybind11::arg("origin") );

		cl.def( pybind11::init( [](PyCallBack_wxHelpEvent const &o){ return new PyCallBack_wxHelpEvent(o); } ) );
		cl.def( pybind11::init( [](wxHelpEvent const &o){ return new wxHelpEvent(o); } ) );

		pybind11::enum_<wxHelpEvent::Origin>(cl, "Origin", pybind11::arithmetic(), "")
			.value("Origin_Unknown", wxHelpEvent::Origin_Unknown)
			.value("Origin_Keyboard", wxHelpEvent::Origin_Keyboard)
			.value("Origin_HelpButton", wxHelpEvent::Origin_HelpButton)
			.export_values();

		cl.def("GetPosition", (const class wxPoint & (wxHelpEvent::*)() const) &wxHelpEvent::GetPosition, "C++: wxHelpEvent::GetPosition() const --> const class wxPoint &", pybind11::return_value_policy::automatic);
		cl.def("SetPosition", (void (wxHelpEvent::*)(const class wxPoint &)) &wxHelpEvent::SetPosition, "C++: wxHelpEvent::SetPosition(const class wxPoint &) --> void", pybind11::arg("pos"));
		cl.def("GetLink", (const class wxString & (wxHelpEvent::*)() const) &wxHelpEvent::GetLink, "C++: wxHelpEvent::GetLink() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("SetLink", (void (wxHelpEvent::*)(const class wxString &)) &wxHelpEvent::SetLink, "C++: wxHelpEvent::SetLink(const class wxString &) --> void", pybind11::arg("link"));
		cl.def("GetTarget", (const class wxString & (wxHelpEvent::*)() const) &wxHelpEvent::GetTarget, "C++: wxHelpEvent::GetTarget() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("SetTarget", (void (wxHelpEvent::*)(const class wxString &)) &wxHelpEvent::SetTarget, "C++: wxHelpEvent::SetTarget(const class wxString &) --> void", pybind11::arg("target"));
		cl.def("Clone", (class wxEvent * (wxHelpEvent::*)() const) &wxHelpEvent::Clone, "C++: wxHelpEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetOrigin", (enum wxHelpEvent::Origin (wxHelpEvent::*)() const) &wxHelpEvent::GetOrigin, "C++: wxHelpEvent::GetOrigin() const --> enum wxHelpEvent::Origin");
		cl.def("SetOrigin", (void (wxHelpEvent::*)(enum wxHelpEvent::Origin)) &wxHelpEvent::SetOrigin, "C++: wxHelpEvent::SetOrigin(enum wxHelpEvent::Origin) --> void", pybind11::arg("origin"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxHelpEvent::*)() const) &wxHelpEvent::GetClassInfo, "C++: wxHelpEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxHelpEvent::wxCreateObject, "C++: wxHelpEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
}

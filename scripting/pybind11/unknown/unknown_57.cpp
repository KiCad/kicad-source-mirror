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

// wxSizeEvent file: line:2082
struct PyCallBack_wxSizeEvent : public wxSizeEvent {
	using wxSizeEvent::wxSizeEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxSizeEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxSizeEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxSizeEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxSizeEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxSizeEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxSizeEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxSizeEvent *>(this), "CloneRefData");
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

// wxMoveEvent file: line:2121
struct PyCallBack_wxMoveEvent : public wxMoveEvent {
	using wxMoveEvent::wxMoveEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMoveEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxMoveEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMoveEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxMoveEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMoveEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMoveEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMoveEvent *>(this), "CloneRefData");
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

// wxPaintEvent file: line:2167
struct PyCallBack_wxPaintEvent : public wxPaintEvent {
	using wxPaintEvent::wxPaintEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaintEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxPaintEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaintEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxPaintEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaintEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaintEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaintEvent *>(this), "CloneRefData");
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

// wxNcPaintEvent file: line:2200
struct PyCallBack_wxNcPaintEvent : public wxNcPaintEvent {
	using wxNcPaintEvent::wxNcPaintEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxNcPaintEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxNcPaintEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxNcPaintEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxNcPaintEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxNcPaintEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxNcPaintEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxNcPaintEvent *>(this), "CloneRefData");
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

// wxEraseEvent file: line:2218
struct PyCallBack_wxEraseEvent : public wxEraseEvent {
	using wxEraseEvent::wxEraseEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEraseEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxEraseEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEraseEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxEraseEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEraseEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEraseEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEraseEvent *>(this), "CloneRefData");
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

// wxFocusEvent file: line:2248
struct PyCallBack_wxFocusEvent : public wxFocusEvent {
	using wxFocusEvent::wxFocusEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFocusEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxFocusEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFocusEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxFocusEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFocusEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFocusEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFocusEvent *>(this), "CloneRefData");
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

// wxChildFocusEvent file: line:2276
struct PyCallBack_wxChildFocusEvent : public wxChildFocusEvent {
	using wxChildFocusEvent::wxChildFocusEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxChildFocusEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxChildFocusEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxChildFocusEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxChildFocusEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxChildFocusEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxChildFocusEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxChildFocusEvent *>(this), "CloneRefData");
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

// wxActivateEvent file: line:2296
struct PyCallBack_wxActivateEvent : public wxActivateEvent {
	using wxActivateEvent::wxActivateEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxActivateEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxActivateEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxActivateEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxActivateEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxActivateEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxActivateEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxActivateEvent *>(this), "CloneRefData");
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

// wxInitDialogEvent file: line:2339
struct PyCallBack_wxInitDialogEvent : public wxInitDialogEvent {
	using wxInitDialogEvent::wxInitDialogEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxInitDialogEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxInitDialogEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxInitDialogEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxInitDialogEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxInitDialogEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxInitDialogEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxInitDialogEvent *>(this), "CloneRefData");
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

// wxMenuEvent file: line:2359
struct PyCallBack_wxMenuEvent : public wxMenuEvent {
	using wxMenuEvent::wxMenuEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMenuEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxMenuEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMenuEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxMenuEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMenuEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMenuEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMenuEvent *>(this), "CloneRefData");
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

// wxCloseEvent file: line:2394
struct PyCallBack_wxCloseEvent : public wxCloseEvent {
	using wxCloseEvent::wxCloseEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCloseEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxCloseEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCloseEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxCloseEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCloseEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCloseEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCloseEvent *>(this), "CloneRefData");
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

void bind_unknown_unknown_57(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxSizeEvent file: line:2082
		pybind11::class_<wxSizeEvent, std::shared_ptr<wxSizeEvent>, PyCallBack_wxSizeEvent, wxEvent> cl(M(""), "wxSizeEvent", "");
		cl.def( pybind11::init( [](){ return new wxSizeEvent(); }, [](){ return new PyCallBack_wxSizeEvent(); } ) );
		cl.def( pybind11::init( [](const class wxSize & a0){ return new wxSizeEvent(a0); }, [](const class wxSize & a0){ return new PyCallBack_wxSizeEvent(a0); } ), "doc");
		cl.def( pybind11::init<const class wxSize &, int>(), pybind11::arg("sz"), pybind11::arg("winid") );

		cl.def( pybind11::init( [](PyCallBack_wxSizeEvent const &o){ return new PyCallBack_wxSizeEvent(o); } ) );
		cl.def( pybind11::init( [](wxSizeEvent const &o){ return new wxSizeEvent(o); } ) );
		cl.def( pybind11::init( [](const class wxRect & a0){ return new wxSizeEvent(a0); }, [](const class wxRect & a0){ return new PyCallBack_wxSizeEvent(a0); } ), "doc");
		cl.def( pybind11::init<const class wxRect &, int>(), pybind11::arg("rect"), pybind11::arg("id") );

		cl.def_readwrite("m_size", &wxSizeEvent::m_size);
		cl.def_readwrite("m_rect", &wxSizeEvent::m_rect);
		cl.def("GetSize", (class wxSize (wxSizeEvent::*)() const) &wxSizeEvent::GetSize, "C++: wxSizeEvent::GetSize() const --> class wxSize");
		cl.def("SetSize", (void (wxSizeEvent::*)(class wxSize)) &wxSizeEvent::SetSize, "C++: wxSizeEvent::SetSize(class wxSize) --> void", pybind11::arg("size"));
		cl.def("GetRect", (class wxRect (wxSizeEvent::*)() const) &wxSizeEvent::GetRect, "C++: wxSizeEvent::GetRect() const --> class wxRect");
		cl.def("SetRect", (void (wxSizeEvent::*)(const class wxRect &)) &wxSizeEvent::SetRect, "C++: wxSizeEvent::SetRect(const class wxRect &) --> void", pybind11::arg("rect"));
		cl.def("Clone", (class wxEvent * (wxSizeEvent::*)() const) &wxSizeEvent::Clone, "C++: wxSizeEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxSizeEvent::*)() const) &wxSizeEvent::GetClassInfo, "C++: wxSizeEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxSizeEvent::wxCreateObject, "C++: wxSizeEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxMoveEvent file: line:2121
		pybind11::class_<wxMoveEvent, std::shared_ptr<wxMoveEvent>, PyCallBack_wxMoveEvent, wxEvent> cl(M(""), "wxMoveEvent", "");
		cl.def( pybind11::init( [](){ return new wxMoveEvent(); }, [](){ return new PyCallBack_wxMoveEvent(); } ) );
		cl.def( pybind11::init( [](const class wxPoint & a0){ return new wxMoveEvent(a0); }, [](const class wxPoint & a0){ return new PyCallBack_wxMoveEvent(a0); } ), "doc");
		cl.def( pybind11::init<const class wxPoint &, int>(), pybind11::arg("pos"), pybind11::arg("winid") );

		cl.def( pybind11::init( [](PyCallBack_wxMoveEvent const &o){ return new PyCallBack_wxMoveEvent(o); } ) );
		cl.def( pybind11::init( [](wxMoveEvent const &o){ return new wxMoveEvent(o); } ) );
		cl.def( pybind11::init( [](const class wxRect & a0){ return new wxMoveEvent(a0); }, [](const class wxRect & a0){ return new PyCallBack_wxMoveEvent(a0); } ), "doc");
		cl.def( pybind11::init<const class wxRect &, int>(), pybind11::arg("rect"), pybind11::arg("id") );

		cl.def("GetPosition", (class wxPoint (wxMoveEvent::*)() const) &wxMoveEvent::GetPosition, "C++: wxMoveEvent::GetPosition() const --> class wxPoint");
		cl.def("SetPosition", (void (wxMoveEvent::*)(const class wxPoint &)) &wxMoveEvent::SetPosition, "C++: wxMoveEvent::SetPosition(const class wxPoint &) --> void", pybind11::arg("pos"));
		cl.def("GetRect", (class wxRect (wxMoveEvent::*)() const) &wxMoveEvent::GetRect, "C++: wxMoveEvent::GetRect() const --> class wxRect");
		cl.def("SetRect", (void (wxMoveEvent::*)(const class wxRect &)) &wxMoveEvent::SetRect, "C++: wxMoveEvent::SetRect(const class wxRect &) --> void", pybind11::arg("rect"));
		cl.def("Clone", (class wxEvent * (wxMoveEvent::*)() const) &wxMoveEvent::Clone, "C++: wxMoveEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxMoveEvent::*)() const) &wxMoveEvent::GetClassInfo, "C++: wxMoveEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxMoveEvent::wxCreateObject, "C++: wxMoveEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxPaintEvent file: line:2167
		pybind11::class_<wxPaintEvent, std::shared_ptr<wxPaintEvent>, PyCallBack_wxPaintEvent, wxEvent> cl(M(""), "wxPaintEvent", "");
		cl.def( pybind11::init( [](){ return new wxPaintEvent(); }, [](){ return new PyCallBack_wxPaintEvent(); } ), "doc");
		cl.def( pybind11::init<int>(), pybind11::arg("Id") );

		cl.def( pybind11::init( [](PyCallBack_wxPaintEvent const &o){ return new PyCallBack_wxPaintEvent(o); } ) );
		cl.def( pybind11::init( [](wxPaintEvent const &o){ return new wxPaintEvent(o); } ) );
		cl.def("Clone", (class wxEvent * (wxPaintEvent::*)() const) &wxPaintEvent::Clone, "C++: wxPaintEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxPaintEvent::*)() const) &wxPaintEvent::GetClassInfo, "C++: wxPaintEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxPaintEvent::wxCreateObject, "C++: wxPaintEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxNcPaintEvent file: line:2200
		pybind11::class_<wxNcPaintEvent, std::shared_ptr<wxNcPaintEvent>, PyCallBack_wxNcPaintEvent, wxEvent> cl(M(""), "wxNcPaintEvent", "");
		cl.def( pybind11::init( [](){ return new wxNcPaintEvent(); }, [](){ return new PyCallBack_wxNcPaintEvent(); } ), "doc");
		cl.def( pybind11::init<int>(), pybind11::arg("winid") );

		cl.def( pybind11::init( [](PyCallBack_wxNcPaintEvent const &o){ return new PyCallBack_wxNcPaintEvent(o); } ) );
		cl.def( pybind11::init( [](wxNcPaintEvent const &o){ return new wxNcPaintEvent(o); } ) );
		cl.def("Clone", (class wxEvent * (wxNcPaintEvent::*)() const) &wxNcPaintEvent::Clone, "C++: wxNcPaintEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxNcPaintEvent::*)() const) &wxNcPaintEvent::GetClassInfo, "C++: wxNcPaintEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxNcPaintEvent::wxCreateObject, "C++: wxNcPaintEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxEraseEvent file: line:2218
		pybind11::class_<wxEraseEvent, std::shared_ptr<wxEraseEvent>, PyCallBack_wxEraseEvent, wxEvent> cl(M(""), "wxEraseEvent", "");
		cl.def( pybind11::init( [](){ return new wxEraseEvent(); }, [](){ return new PyCallBack_wxEraseEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxEraseEvent(a0); }, [](int const & a0){ return new PyCallBack_wxEraseEvent(a0); } ), "doc");
		cl.def( pybind11::init<int, class wxDC *>(), pybind11::arg("Id"), pybind11::arg("dc") );

		cl.def( pybind11::init( [](PyCallBack_wxEraseEvent const &o){ return new PyCallBack_wxEraseEvent(o); } ) );
		cl.def( pybind11::init( [](wxEraseEvent const &o){ return new wxEraseEvent(o); } ) );
		cl.def("GetDC", (class wxDC * (wxEraseEvent::*)() const) &wxEraseEvent::GetDC, "C++: wxEraseEvent::GetDC() const --> class wxDC *", pybind11::return_value_policy::automatic);
		cl.def("Clone", (class wxEvent * (wxEraseEvent::*)() const) &wxEraseEvent::Clone, "C++: wxEraseEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxEraseEvent::*)() const) &wxEraseEvent::GetClassInfo, "C++: wxEraseEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxEraseEvent::wxCreateObject, "C++: wxEraseEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxFocusEvent file: line:2248
		pybind11::class_<wxFocusEvent, std::shared_ptr<wxFocusEvent>, PyCallBack_wxFocusEvent, wxEvent> cl(M(""), "wxFocusEvent", "");
		cl.def( pybind11::init( [](){ return new wxFocusEvent(); }, [](){ return new PyCallBack_wxFocusEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxFocusEvent(a0); }, [](int const & a0){ return new PyCallBack_wxFocusEvent(a0); } ), "doc");
		cl.def( pybind11::init<int, int>(), pybind11::arg("type"), pybind11::arg("winid") );

		cl.def( pybind11::init( [](PyCallBack_wxFocusEvent const &o){ return new PyCallBack_wxFocusEvent(o); } ) );
		cl.def( pybind11::init( [](wxFocusEvent const &o){ return new wxFocusEvent(o); } ) );
		cl.def("GetWindow", (class wxWindow * (wxFocusEvent::*)() const) &wxFocusEvent::GetWindow, "C++: wxFocusEvent::GetWindow() const --> class wxWindow *", pybind11::return_value_policy::automatic);
		cl.def("SetWindow", (void (wxFocusEvent::*)(class wxWindow *)) &wxFocusEvent::SetWindow, "C++: wxFocusEvent::SetWindow(class wxWindow *) --> void", pybind11::arg("win"));
		cl.def("Clone", (class wxEvent * (wxFocusEvent::*)() const) &wxFocusEvent::Clone, "C++: wxFocusEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxFocusEvent::*)() const) &wxFocusEvent::GetClassInfo, "C++: wxFocusEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxFocusEvent::wxCreateObject, "C++: wxFocusEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxChildFocusEvent file: line:2276
		pybind11::class_<wxChildFocusEvent, std::shared_ptr<wxChildFocusEvent>, PyCallBack_wxChildFocusEvent, wxCommandEvent> cl(M(""), "wxChildFocusEvent", "");
		cl.def( pybind11::init( [](){ return new wxChildFocusEvent(); }, [](){ return new PyCallBack_wxChildFocusEvent(); } ), "doc");
		cl.def( pybind11::init<class wxWindow *>(), pybind11::arg("win") );

		cl.def( pybind11::init( [](PyCallBack_wxChildFocusEvent const &o){ return new PyCallBack_wxChildFocusEvent(o); } ) );
		cl.def( pybind11::init( [](wxChildFocusEvent const &o){ return new wxChildFocusEvent(o); } ) );
		cl.def("GetWindow", (class wxWindow * (wxChildFocusEvent::*)() const) &wxChildFocusEvent::GetWindow, "C++: wxChildFocusEvent::GetWindow() const --> class wxWindow *", pybind11::return_value_policy::automatic);
		cl.def("Clone", (class wxEvent * (wxChildFocusEvent::*)() const) &wxChildFocusEvent::Clone, "C++: wxChildFocusEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxChildFocusEvent::*)() const) &wxChildFocusEvent::GetClassInfo, "C++: wxChildFocusEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxChildFocusEvent::wxCreateObject, "C++: wxChildFocusEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxActivateEvent file: line:2296
		pybind11::class_<wxActivateEvent, std::shared_ptr<wxActivateEvent>, PyCallBack_wxActivateEvent, wxEvent> cl(M(""), "wxActivateEvent", "");
		cl.def( pybind11::init( [](){ return new wxActivateEvent(); }, [](){ return new PyCallBack_wxActivateEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxActivateEvent(a0); }, [](int const & a0){ return new PyCallBack_wxActivateEvent(a0); } ), "doc");
		cl.def( pybind11::init( [](int const & a0, bool const & a1){ return new wxActivateEvent(a0, a1); }, [](int const & a0, bool const & a1){ return new PyCallBack_wxActivateEvent(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](int const & a0, bool const & a1, int const & a2){ return new wxActivateEvent(a0, a1, a2); }, [](int const & a0, bool const & a1, int const & a2){ return new PyCallBack_wxActivateEvent(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init<int, bool, int, enum wxActivateEvent::Reason>(), pybind11::arg("type"), pybind11::arg("active"), pybind11::arg("Id"), pybind11::arg("activationReason") );

		cl.def( pybind11::init( [](PyCallBack_wxActivateEvent const &o){ return new PyCallBack_wxActivateEvent(o); } ) );
		cl.def( pybind11::init( [](wxActivateEvent const &o){ return new wxActivateEvent(o); } ) );

		pybind11::enum_<wxActivateEvent::Reason>(cl, "Reason", pybind11::arithmetic(), "")
			.value("Reason_Mouse", wxActivateEvent::Reason_Mouse)
			.value("Reason_Unknown", wxActivateEvent::Reason_Unknown)
			.export_values();

		cl.def("GetActive", (bool (wxActivateEvent::*)() const) &wxActivateEvent::GetActive, "C++: wxActivateEvent::GetActive() const --> bool");
		cl.def("GetActivationReason", (enum wxActivateEvent::Reason (wxActivateEvent::*)() const) &wxActivateEvent::GetActivationReason, "C++: wxActivateEvent::GetActivationReason() const --> enum wxActivateEvent::Reason");
		cl.def("Clone", (class wxEvent * (wxActivateEvent::*)() const) &wxActivateEvent::Clone, "C++: wxActivateEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxActivateEvent::*)() const) &wxActivateEvent::GetClassInfo, "C++: wxActivateEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxActivateEvent::wxCreateObject, "C++: wxActivateEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxInitDialogEvent file: line:2339
		pybind11::class_<wxInitDialogEvent, std::shared_ptr<wxInitDialogEvent>, PyCallBack_wxInitDialogEvent, wxEvent> cl(M(""), "wxInitDialogEvent", "");
		cl.def( pybind11::init( [](){ return new wxInitDialogEvent(); }, [](){ return new PyCallBack_wxInitDialogEvent(); } ), "doc");
		cl.def( pybind11::init<int>(), pybind11::arg("Id") );

		cl.def( pybind11::init( [](PyCallBack_wxInitDialogEvent const &o){ return new PyCallBack_wxInitDialogEvent(o); } ) );
		cl.def( pybind11::init( [](wxInitDialogEvent const &o){ return new wxInitDialogEvent(o); } ) );
		cl.def("Clone", (class wxEvent * (wxInitDialogEvent::*)() const) &wxInitDialogEvent::Clone, "C++: wxInitDialogEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxInitDialogEvent::*)() const) &wxInitDialogEvent::GetClassInfo, "C++: wxInitDialogEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxInitDialogEvent::wxCreateObject, "C++: wxInitDialogEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxMenuEvent file: line:2359
		pybind11::class_<wxMenuEvent, std::shared_ptr<wxMenuEvent>, PyCallBack_wxMenuEvent, wxEvent> cl(M(""), "wxMenuEvent", "");
		cl.def( pybind11::init( [](PyCallBack_wxMenuEvent const &o){ return new PyCallBack_wxMenuEvent(o); } ) );
		cl.def( pybind11::init( [](wxMenuEvent const &o){ return new wxMenuEvent(o); } ) );
		cl.def("GetMenuId", (int (wxMenuEvent::*)() const) &wxMenuEvent::GetMenuId, "C++: wxMenuEvent::GetMenuId() const --> int");
		cl.def("IsPopup", (bool (wxMenuEvent::*)() const) &wxMenuEvent::IsPopup, "C++: wxMenuEvent::IsPopup() const --> bool");
		cl.def("Clone", (class wxEvent * (wxMenuEvent::*)() const) &wxMenuEvent::Clone, "C++: wxMenuEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxMenuEvent::*)() const) &wxMenuEvent::GetClassInfo, "C++: wxMenuEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxMenuEvent::wxCreateObject, "C++: wxMenuEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxCloseEvent file: line:2394
		pybind11::class_<wxCloseEvent, std::shared_ptr<wxCloseEvent>, PyCallBack_wxCloseEvent, wxEvent> cl(M(""), "wxCloseEvent", "");
		cl.def( pybind11::init( [](){ return new wxCloseEvent(); }, [](){ return new PyCallBack_wxCloseEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxCloseEvent(a0); }, [](int const & a0){ return new PyCallBack_wxCloseEvent(a0); } ), "doc");
		cl.def( pybind11::init<int, int>(), pybind11::arg("type"), pybind11::arg("winid") );

		cl.def( pybind11::init( [](PyCallBack_wxCloseEvent const &o){ return new PyCallBack_wxCloseEvent(o); } ) );
		cl.def( pybind11::init( [](wxCloseEvent const &o){ return new wxCloseEvent(o); } ) );
		cl.def("SetLoggingOff", (void (wxCloseEvent::*)(bool)) &wxCloseEvent::SetLoggingOff, "C++: wxCloseEvent::SetLoggingOff(bool) --> void", pybind11::arg("logOff"));
		cl.def("GetLoggingOff", (bool (wxCloseEvent::*)() const) &wxCloseEvent::GetLoggingOff, "C++: wxCloseEvent::GetLoggingOff() const --> bool");
		cl.def("Veto", [](wxCloseEvent &o) -> void { return o.Veto(); }, "");
		cl.def("Veto", (void (wxCloseEvent::*)(bool)) &wxCloseEvent::Veto, "C++: wxCloseEvent::Veto(bool) --> void", pybind11::arg("veto"));
		cl.def("SetCanVeto", (void (wxCloseEvent::*)(bool)) &wxCloseEvent::SetCanVeto, "C++: wxCloseEvent::SetCanVeto(bool) --> void", pybind11::arg("canVeto"));
		cl.def("CanVeto", (bool (wxCloseEvent::*)() const) &wxCloseEvent::CanVeto, "C++: wxCloseEvent::CanVeto() const --> bool");
		cl.def("GetVeto", (bool (wxCloseEvent::*)() const) &wxCloseEvent::GetVeto, "C++: wxCloseEvent::GetVeto() const --> bool");
		cl.def("Clone", (class wxEvent * (wxCloseEvent::*)() const) &wxCloseEvent::Clone, "C++: wxCloseEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxCloseEvent::*)() const) &wxCloseEvent::GetClassInfo, "C++: wxCloseEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxCloseEvent::wxCreateObject, "C++: wxCloseEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
}

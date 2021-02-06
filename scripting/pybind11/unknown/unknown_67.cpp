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

// wxValidator file: line:35
struct PyCallBack_wxValidator : public wxValidator {
	using wxValidator::wxValidator;

	class wxObject * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxObject *>::value) {
				static pybind11::detail::override_caster_t<class wxObject *> caster;
				return pybind11::detail::cast_ref<class wxObject *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxObject *>(std::move(o));
		}
		return wxValidator::Clone();
	}
	bool Validate(class wxWindow * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "Validate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxValidator::Validate(a0);
	}
	bool TransferToWindow() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "TransferToWindow");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxValidator::TransferToWindow();
	}
	bool TransferFromWindow() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "TransferFromWindow");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxValidator::TransferFromWindow();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxValidator::GetClassInfo();
	}
	void SetNextHandler(class wxEvtHandler * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "SetNextHandler");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "SetPreviousHandler");
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
	bool ProcessEvent(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "ProcessEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEvtHandler::ProcessEvent(a0);
	}
	void QueueEvent(class wxEvent * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "QueueEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "AddPendingEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "SearchEventTable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "TryBefore");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "TryAfter");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "TryValidator");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "TryParent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "GetEventTable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "GetEventHashTable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "DoSetClientObject");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "DoGetClientObject");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "DoSetClientData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "DoGetClientData");
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
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxValidator *>(this), "CloneRefData");
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

// wxPaletteBase file: line:22
struct PyCallBack_wxPaletteBase : public wxPaletteBase {
	using wxPaletteBase::wxPaletteBase;

	int GetColoursCount() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaletteBase *>(this), "GetColoursCount");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxPaletteBase::GetColoursCount();
	}
	bool IsOk() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaletteBase *>(this), "IsOk");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxGDIObject::IsOk();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaletteBase *>(this), "CreateRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxGDIObject::CreateRefData();
	}
	class wxRefCounter * CloneRefData(const class wxRefCounter * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaletteBase *>(this), "CloneRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxGDIObject::CloneRefData(a0);
	}
	class wxGDIRefData * CreateGDIRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaletteBase *>(this), "CreateGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxGDIObject::CreateGDIRefData\"");
	}
	class wxGDIRefData * CloneGDIRefData(const class wxGDIRefData * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaletteBase *>(this), "CloneGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxGDIObject::CloneGDIRefData\"");
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPaletteBase *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxGDIObject::GetClassInfo();
	}
};

// wxAcceleratorTable file: line:18
struct PyCallBack_wxAcceleratorTable : public wxAcceleratorTable {
	using wxAcceleratorTable::wxAcceleratorTable;

	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAcceleratorTable *>(this), "CreateRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxAcceleratorTable::CreateRefData();
	}
	class wxRefCounter * CloneRefData(const class wxRefCounter * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAcceleratorTable *>(this), "CloneRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxAcceleratorTable::CloneRefData(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAcceleratorTable *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxAcceleratorTable::GetClassInfo();
	}
};

// wxWindowListNode file: line:151
struct PyCallBack_wxWindowListNode : public wxWindowListNode {
	using wxWindowListNode::wxWindowListNode;

	void DeleteData() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowListNode *>(this), "DeleteData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxWindowListNode::DeleteData();
	}
};

// wxWindowList file: line:151
struct PyCallBack_wxWindowList : public wxWindowList {
	using wxWindowList::wxWindowList;

	class wxWindowListNode * Find(const class wxListKey & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowList *>(this), "Find");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxWindowListNode *>::value) {
				static pybind11::detail::override_caster_t<class wxWindowListNode *> caster;
				return pybind11::detail::cast_ref<class wxWindowListNode *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxWindowListNode *>(std::move(o));
		}
		return wxWindowList::Find(a0);
	}
	class wxNodeBase * CreateNode(class wxNodeBase * a0, class wxNodeBase * a1, void * a2, const class wxListKey & a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowList *>(this), "CreateNode");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxNodeBase *>::value) {
				static pybind11::detail::override_caster_t<class wxNodeBase *> caster;
				return pybind11::detail::cast_ref<class wxNodeBase *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxNodeBase *>(std::move(o));
		}
		return wxWindowList::CreateNode(a0, a1, a2, a3);
	}
};

void bind_unknown_unknown_67(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxGetLocale() file: line:356
	M("").def("wxGetLocale", (class wxLocale * (*)()) &wxGetLocale, "C++: wxGetLocale() --> class wxLocale *", pybind11::return_value_policy::automatic);

	{ // wxValidator file: line:35
		pybind11::class_<wxValidator, std::shared_ptr<wxValidator>, PyCallBack_wxValidator, wxEvtHandler> cl(M(""), "wxValidator", "");
		cl.def( pybind11::init( [](){ return new wxValidator(); }, [](){ return new PyCallBack_wxValidator(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxValidator const &o){ return new PyCallBack_wxValidator(o); } ) );
		cl.def( pybind11::init( [](wxValidator const &o){ return new wxValidator(o); } ) );
		cl.def("Clone", (class wxObject * (wxValidator::*)() const) &wxValidator::Clone, "C++: wxValidator::Clone() const --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("Copy", (bool (wxValidator::*)(const class wxValidator &)) &wxValidator::Copy, "C++: wxValidator::Copy(const class wxValidator &) --> bool", pybind11::arg("val"));
		cl.def("Validate", (bool (wxValidator::*)(class wxWindow *)) &wxValidator::Validate, "C++: wxValidator::Validate(class wxWindow *) --> bool", pybind11::arg(""));
		cl.def("TransferToWindow", (bool (wxValidator::*)()) &wxValidator::TransferToWindow, "C++: wxValidator::TransferToWindow() --> bool");
		cl.def("TransferFromWindow", (bool (wxValidator::*)()) &wxValidator::TransferFromWindow, "C++: wxValidator::TransferFromWindow() --> bool");
		cl.def("GetWindow", (class wxWindow * (wxValidator::*)() const) &wxValidator::GetWindow, "C++: wxValidator::GetWindow() const --> class wxWindow *", pybind11::return_value_policy::automatic);
		cl.def("SetWindow", (void (wxValidator::*)(class wxWindowBase *)) &wxValidator::SetWindow, "C++: wxValidator::SetWindow(class wxWindowBase *) --> void", pybind11::arg("win"));
		cl.def_static("SuppressBellOnError", []() -> void { return wxValidator::SuppressBellOnError(); }, "");
		cl.def_static("SuppressBellOnError", (void (*)(bool)) &wxValidator::SuppressBellOnError, "C++: wxValidator::SuppressBellOnError(bool) --> void", pybind11::arg("suppress"));
		cl.def_static("IsSilent", (bool (*)()) &wxValidator::IsSilent, "C++: wxValidator::IsSilent() --> bool");
		cl.def_static("SetBellOnError", []() -> void { return wxValidator::SetBellOnError(); }, "");
		cl.def_static("SetBellOnError", (void (*)(bool)) &wxValidator::SetBellOnError, "C++: wxValidator::SetBellOnError(bool) --> void", pybind11::arg("doIt"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxValidator::*)() const) &wxValidator::GetClassInfo, "C++: wxValidator::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxValidator::wxCreateObject, "C++: wxValidator::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxPaletteBase file: line:22
		pybind11::class_<wxPaletteBase, std::shared_ptr<wxPaletteBase>, PyCallBack_wxPaletteBase, wxGDIObject> cl(M(""), "wxPaletteBase", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxPaletteBase(); } ) );
		cl.def("GetColoursCount", (int (wxPaletteBase::*)() const) &wxPaletteBase::GetColoursCount, "C++: wxPaletteBase::GetColoursCount() const --> int");
		cl.def("assign", (class wxPaletteBase & (wxPaletteBase::*)(const class wxPaletteBase &)) &wxPaletteBase::operator=, "C++: wxPaletteBase::operator=(const class wxPaletteBase &) --> class wxPaletteBase &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// wxAcceleratorEntryFlags file: line:29
	pybind11::enum_<wxAcceleratorEntryFlags>(M(""), "wxAcceleratorEntryFlags", pybind11::arithmetic(), "")
		.value("wxACCEL_NORMAL", wxACCEL_NORMAL)
		.value("wxACCEL_ALT", wxACCEL_ALT)
		.value("wxACCEL_CTRL", wxACCEL_CTRL)
		.value("wxACCEL_SHIFT", wxACCEL_SHIFT)
		.value("wxACCEL_RAW_CTRL", wxACCEL_RAW_CTRL)
		.value("wxACCEL_CMD", wxACCEL_CMD)
		.export_values();

;

	{ // wxAcceleratorEntry file: line:47
		pybind11::class_<wxAcceleratorEntry, std::shared_ptr<wxAcceleratorEntry>> cl(M(""), "wxAcceleratorEntry", "");
		cl.def( pybind11::init( [](wxAcceleratorEntry const &o){ return new wxAcceleratorEntry(o); } ) );
		cl.def_static("Create", (class wxAcceleratorEntry * (*)(const class wxString &)) &wxAcceleratorEntry::Create, "C++: wxAcceleratorEntry::Create(const class wxString &) --> class wxAcceleratorEntry *", pybind11::return_value_policy::automatic, pybind11::arg("str"));
		cl.def("assign", (class wxAcceleratorEntry & (wxAcceleratorEntry::*)(const class wxAcceleratorEntry &)) &wxAcceleratorEntry::operator=, "C++: wxAcceleratorEntry::operator=(const class wxAcceleratorEntry &) --> class wxAcceleratorEntry &", pybind11::return_value_policy::automatic, pybind11::arg("entry"));
		cl.def("GetFlags", (int (wxAcceleratorEntry::*)() const) &wxAcceleratorEntry::GetFlags, "C++: wxAcceleratorEntry::GetFlags() const --> int");
		cl.def("GetKeyCode", (int (wxAcceleratorEntry::*)() const) &wxAcceleratorEntry::GetKeyCode, "C++: wxAcceleratorEntry::GetKeyCode() const --> int");
		cl.def("GetCommand", (int (wxAcceleratorEntry::*)() const) &wxAcceleratorEntry::GetCommand, "C++: wxAcceleratorEntry::GetCommand() const --> int");
		cl.def("__eq__", (bool (wxAcceleratorEntry::*)(const class wxAcceleratorEntry &) const) &wxAcceleratorEntry::operator==, "C++: wxAcceleratorEntry::operator==(const class wxAcceleratorEntry &) const --> bool", pybind11::arg("entry"));
		cl.def("__ne__", (bool (wxAcceleratorEntry::*)(const class wxAcceleratorEntry &) const) &wxAcceleratorEntry::operator!=, "C++: wxAcceleratorEntry::operator!=(const class wxAcceleratorEntry &) const --> bool", pybind11::arg("entry"));
		cl.def("IsOk", (bool (wxAcceleratorEntry::*)() const) &wxAcceleratorEntry::IsOk, "C++: wxAcceleratorEntry::IsOk() const --> bool");
		cl.def("ToString", (class wxString (wxAcceleratorEntry::*)() const) &wxAcceleratorEntry::ToString, "C++: wxAcceleratorEntry::ToString() const --> class wxString");
		cl.def("ToRawString", (class wxString (wxAcceleratorEntry::*)() const) &wxAcceleratorEntry::ToRawString, "C++: wxAcceleratorEntry::ToRawString() const --> class wxString");
		cl.def("FromString", (bool (wxAcceleratorEntry::*)(const class wxString &)) &wxAcceleratorEntry::FromString, "C++: wxAcceleratorEntry::FromString(const class wxString &) --> bool", pybind11::arg("str"));
	}
	{ // wxAcceleratorTable file: line:18
		pybind11::class_<wxAcceleratorTable, std::shared_ptr<wxAcceleratorTable>, PyCallBack_wxAcceleratorTable, wxObject> cl(M(""), "wxAcceleratorTable", "");
		cl.def( pybind11::init( [](){ return new wxAcceleratorTable(); }, [](){ return new PyCallBack_wxAcceleratorTable(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxAcceleratorTable const &o){ return new PyCallBack_wxAcceleratorTable(o); } ) );
		cl.def( pybind11::init( [](wxAcceleratorTable const &o){ return new wxAcceleratorTable(o); } ) );
		cl.def("Ok", (bool (wxAcceleratorTable::*)() const) &wxAcceleratorTable::Ok, "C++: wxAcceleratorTable::Ok() const --> bool");
		cl.def("IsOk", (bool (wxAcceleratorTable::*)() const) &wxAcceleratorTable::IsOk, "C++: wxAcceleratorTable::IsOk() const --> bool");
		cl.def("Add", (void (wxAcceleratorTable::*)(const class wxAcceleratorEntry &)) &wxAcceleratorTable::Add, "C++: wxAcceleratorTable::Add(const class wxAcceleratorEntry &) --> void", pybind11::arg("entry"));
		cl.def("Remove", (void (wxAcceleratorTable::*)(const class wxAcceleratorEntry &)) &wxAcceleratorTable::Remove, "C++: wxAcceleratorTable::Remove(const class wxAcceleratorEntry &) --> void", pybind11::arg("entry"));
		cl.def("GetCommand", (int (wxAcceleratorTable::*)(const class wxKeyEvent &) const) &wxAcceleratorTable::GetCommand, "C++: wxAcceleratorTable::GetCommand(const class wxKeyEvent &) const --> int", pybind11::arg("event"));
		cl.def("GetEntry", (const class wxAcceleratorEntry * (wxAcceleratorTable::*)(const class wxKeyEvent &) const) &wxAcceleratorTable::GetEntry, "C++: wxAcceleratorTable::GetEntry(const class wxKeyEvent &) const --> const class wxAcceleratorEntry *", pybind11::return_value_policy::automatic, pybind11::arg("event"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxAcceleratorTable::*)() const) &wxAcceleratorTable::GetClassInfo, "C++: wxAcceleratorTable::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxAcceleratorTable::wxCreateObject, "C++: wxAcceleratorTable::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxAcceleratorTable & (wxAcceleratorTable::*)(const class wxAcceleratorTable &)) &wxAcceleratorTable::operator=, "C++: wxAcceleratorTable::operator=(const class wxAcceleratorTable &) --> class wxAcceleratorTable &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxVisualAttributes file: line:96
		pybind11::class_<wxVisualAttributes, std::shared_ptr<wxVisualAttributes>> cl(M(""), "wxVisualAttributes", "");
		cl.def( pybind11::init( [](wxVisualAttributes const &o){ return new wxVisualAttributes(o); } ) );
		cl.def( pybind11::init( [](){ return new wxVisualAttributes(); } ) );
		cl.def_readwrite("font", &wxVisualAttributes::font);
		cl.def_readwrite("colFg", &wxVisualAttributes::colFg);
		cl.def_readwrite("colBg", &wxVisualAttributes::colBg);
	}
	// wxWindowVariant file: line:111
	pybind11::enum_<wxWindowVariant>(M(""), "wxWindowVariant", pybind11::arithmetic(), "")
		.value("wxWINDOW_VARIANT_NORMAL", wxWINDOW_VARIANT_NORMAL)
		.value("wxWINDOW_VARIANT_SMALL", wxWINDOW_VARIANT_SMALL)
		.value("wxWINDOW_VARIANT_MINI", wxWINDOW_VARIANT_MINI)
		.value("wxWINDOW_VARIANT_LARGE", wxWINDOW_VARIANT_LARGE)
		.value("wxWINDOW_VARIANT_MAX", wxWINDOW_VARIANT_MAX)
		.export_values();

;

	// wxShowEffect file: line:125
	pybind11::enum_<wxShowEffect>(M(""), "wxShowEffect", pybind11::arithmetic(), "")
		.value("wxSHOW_EFFECT_NONE", wxSHOW_EFFECT_NONE)
		.value("wxSHOW_EFFECT_ROLL_TO_LEFT", wxSHOW_EFFECT_ROLL_TO_LEFT)
		.value("wxSHOW_EFFECT_ROLL_TO_RIGHT", wxSHOW_EFFECT_ROLL_TO_RIGHT)
		.value("wxSHOW_EFFECT_ROLL_TO_TOP", wxSHOW_EFFECT_ROLL_TO_TOP)
		.value("wxSHOW_EFFECT_ROLL_TO_BOTTOM", wxSHOW_EFFECT_ROLL_TO_BOTTOM)
		.value("wxSHOW_EFFECT_SLIDE_TO_LEFT", wxSHOW_EFFECT_SLIDE_TO_LEFT)
		.value("wxSHOW_EFFECT_SLIDE_TO_RIGHT", wxSHOW_EFFECT_SLIDE_TO_RIGHT)
		.value("wxSHOW_EFFECT_SLIDE_TO_TOP", wxSHOW_EFFECT_SLIDE_TO_TOP)
		.value("wxSHOW_EFFECT_SLIDE_TO_BOTTOM", wxSHOW_EFFECT_SLIDE_TO_BOTTOM)
		.value("wxSHOW_EFFECT_BLEND", wxSHOW_EFFECT_BLEND)
		.value("wxSHOW_EFFECT_EXPAND", wxSHOW_EFFECT_EXPAND)
		.value("wxSHOW_EFFECT_MAX", wxSHOW_EFFECT_MAX)
		.export_values();

;

	{ // wxWindowListNode file: line:151
		pybind11::class_<wxWindowListNode, std::shared_ptr<wxWindowListNode>, PyCallBack_wxWindowListNode, wxNodeBase> cl(M(""), "wxWindowListNode", "");
		cl.def( pybind11::init( [](){ return new wxWindowListNode(); }, [](){ return new PyCallBack_wxWindowListNode(); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0){ return new wxWindowListNode(a0); }, [](class wxListBase * a0){ return new PyCallBack_wxWindowListNode(a0); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxWindowListNode * a1){ return new wxWindowListNode(a0, a1); }, [](class wxListBase * a0, class wxWindowListNode * a1){ return new PyCallBack_wxWindowListNode(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxWindowListNode * a1, class wxWindowListNode * a2){ return new wxWindowListNode(a0, a1, a2); }, [](class wxListBase * a0, class wxWindowListNode * a1, class wxWindowListNode * a2){ return new PyCallBack_wxWindowListNode(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init( [](class wxListBase * a0, class wxWindowListNode * a1, class wxWindowListNode * a2, class wxWindow * a3){ return new wxWindowListNode(a0, a1, a2, a3); }, [](class wxListBase * a0, class wxWindowListNode * a1, class wxWindowListNode * a2, class wxWindow * a3){ return new PyCallBack_wxWindowListNode(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init<class wxListBase *, class wxWindowListNode *, class wxWindowListNode *, class wxWindow *, const class wxListKey &>(), pybind11::arg("list"), pybind11::arg("previous"), pybind11::arg("next"), pybind11::arg("data"), pybind11::arg("key") );

		cl.def("GetNext", (class wxWindowListNode * (wxWindowListNode::*)() const) &wxWindowListNode::GetNext, "C++: wxWindowListNode::GetNext() const --> class wxWindowListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetPrevious", (class wxWindowListNode * (wxWindowListNode::*)() const) &wxWindowListNode::GetPrevious, "C++: wxWindowListNode::GetPrevious() const --> class wxWindowListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetData", (class wxWindow * (wxWindowListNode::*)() const) &wxWindowListNode::GetData, "C++: wxWindowListNode::GetData() const --> class wxWindow *", pybind11::return_value_policy::automatic);
		cl.def("SetData", (void (wxWindowListNode::*)(class wxWindow *)) &wxWindowListNode::SetData, "C++: wxWindowListNode::SetData(class wxWindow *) --> void", pybind11::arg("data"));
	}
	{ // wxWindowList file: line:151
		pybind11::class_<wxWindowList, std::shared_ptr<wxWindowList>, PyCallBack_wxWindowList, wxListBase> cl(M(""), "wxWindowList", "");
		cl.def( pybind11::init( [](){ return new wxWindowList(); }, [](){ return new PyCallBack_wxWindowList(); } ), "doc");
		cl.def( pybind11::init<enum wxKeyType>(), pybind11::arg("keyType") );

		cl.def( pybind11::init( [](PyCallBack_wxWindowList const &o){ return new PyCallBack_wxWindowList(o); } ) );
		cl.def( pybind11::init( [](wxWindowList const &o){ return new wxWindowList(o); } ) );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxWindowList(a0); }, [](unsigned long const & a0){ return new PyCallBack_wxWindowList(a0); } ), "doc");
		cl.def( pybind11::init<unsigned long, class wxWindow *const &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init<const class wxWindowList::const_iterator &, const class wxWindowList::const_iterator &>(), pybind11::arg("first"), pybind11::arg("last") );

		cl.def("assign", (class wxWindowList & (wxWindowList::*)(const class wxWindowList &)) &wxWindowList::operator=, "C++: wxWindowList::operator=(const class wxWindowList &) --> class wxWindowList &", pybind11::return_value_policy::automatic, pybind11::arg("list"));
		cl.def("GetFirst", (class wxWindowListNode * (wxWindowList::*)() const) &wxWindowList::GetFirst, "C++: wxWindowList::GetFirst() const --> class wxWindowListNode *", pybind11::return_value_policy::automatic);
		cl.def("GetLast", (class wxWindowListNode * (wxWindowList::*)() const) &wxWindowList::GetLast, "C++: wxWindowList::GetLast() const --> class wxWindowListNode *", pybind11::return_value_policy::automatic);
		cl.def("Item", (class wxWindowListNode * (wxWindowList::*)(unsigned long) const) &wxWindowList::Item, "C++: wxWindowList::Item(unsigned long) const --> class wxWindowListNode *", pybind11::return_value_policy::automatic, pybind11::arg("index"));
		cl.def("__getitem__", (class wxWindow * (wxWindowList::*)(unsigned long) const) &wxWindowList::operator[], "C++: wxWindowList::operator[](unsigned long) const --> class wxWindow *", pybind11::return_value_policy::automatic, pybind11::arg("index"));
		cl.def("Append", (class wxWindowListNode * (wxWindowList::*)(class wxWindowBase *)) &wxWindowList::Append, "C++: wxWindowList::Append(class wxWindowBase *) --> class wxWindowListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Insert", (class wxWindowListNode * (wxWindowList::*)(class wxWindowBase *)) &wxWindowList::Insert, "C++: wxWindowList::Insert(class wxWindowBase *) --> class wxWindowListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Insert", (class wxWindowListNode * (wxWindowList::*)(unsigned long, class wxWindowBase *)) &wxWindowList::Insert, "C++: wxWindowList::Insert(unsigned long, class wxWindowBase *) --> class wxWindowListNode *", pybind11::return_value_policy::automatic, pybind11::arg("pos"), pybind11::arg("object"));
		cl.def("Insert", (class wxWindowListNode * (wxWindowList::*)(class wxWindowListNode *, class wxWindowBase *)) &wxWindowList::Insert, "C++: wxWindowList::Insert(class wxWindowListNode *, class wxWindowBase *) --> class wxWindowListNode *", pybind11::return_value_policy::automatic, pybind11::arg("prev"), pybind11::arg("object"));
		cl.def("Append", (class wxWindowListNode * (wxWindowList::*)(long, void *)) &wxWindowList::Append, "C++: wxWindowList::Append(long, void *) --> class wxWindowListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"), pybind11::arg("object"));
		cl.def("Append", (class wxWindowListNode * (wxWindowList::*)(const wchar_t *, void *)) &wxWindowList::Append, "C++: wxWindowList::Append(const wchar_t *, void *) --> class wxWindowListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"), pybind11::arg("object"));
		cl.def("DetachNode", (class wxWindowListNode * (wxWindowList::*)(class wxWindowListNode *)) &wxWindowList::DetachNode, "C++: wxWindowList::DetachNode(class wxWindowListNode *) --> class wxWindowListNode *", pybind11::return_value_policy::automatic, pybind11::arg("node"));
		cl.def("DeleteNode", (bool (wxWindowList::*)(class wxWindowListNode *)) &wxWindowList::DeleteNode, "C++: wxWindowList::DeleteNode(class wxWindowListNode *) --> bool", pybind11::arg("node"));
		cl.def("DeleteObject", (bool (wxWindowList::*)(class wxWindowBase *)) &wxWindowList::DeleteObject, "C++: wxWindowList::DeleteObject(class wxWindowBase *) --> bool", pybind11::arg("object"));
		cl.def("Erase", (void (wxWindowList::*)(class wxWindowListNode *)) &wxWindowList::Erase, "C++: wxWindowList::Erase(class wxWindowListNode *) --> void", pybind11::arg("it"));
		cl.def("Find", (class wxWindowListNode * (wxWindowList::*)(const class wxWindowBase *) const) &wxWindowList::Find, "C++: wxWindowList::Find(const class wxWindowBase *) const --> class wxWindowListNode *", pybind11::return_value_policy::automatic, pybind11::arg("object"));
		cl.def("Find", (class wxWindowListNode * (wxWindowList::*)(const class wxListKey &) const) &wxWindowList::Find, "C++: wxWindowList::Find(const class wxListKey &) const --> class wxWindowListNode *", pybind11::return_value_policy::automatic, pybind11::arg("key"));
		cl.def("Member", (bool (wxWindowList::*)(const class wxWindowBase *) const) &wxWindowList::Member, "C++: wxWindowList::Member(const class wxWindowBase *) const --> bool", pybind11::arg("object"));
		cl.def("IndexOf", (int (wxWindowList::*)(class wxWindowBase *) const) &wxWindowList::IndexOf, "C++: wxWindowList::IndexOf(class wxWindowBase *) const --> int", pybind11::arg("object"));
		cl.def("begin", (class wxWindowList::iterator (wxWindowList::*)()) &wxWindowList::begin, "C++: wxWindowList::begin() --> class wxWindowList::iterator");
		cl.def("end", (class wxWindowList::iterator (wxWindowList::*)()) &wxWindowList::end, "C++: wxWindowList::end() --> class wxWindowList::iterator");
		cl.def("rbegin", (class wxWindowList::reverse_iterator (wxWindowList::*)()) &wxWindowList::rbegin, "C++: wxWindowList::rbegin() --> class wxWindowList::reverse_iterator");
		cl.def("rend", (class wxWindowList::reverse_iterator (wxWindowList::*)()) &wxWindowList::rend, "C++: wxWindowList::rend() --> class wxWindowList::reverse_iterator");
		cl.def("resize", [](wxWindowList &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxWindowList::*)(unsigned long, class wxWindow *)) &wxWindowList::resize, "C++: wxWindowList::resize(unsigned long, class wxWindow *) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("size", (unsigned long (wxWindowList::*)() const) &wxWindowList::size, "C++: wxWindowList::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxWindowList::*)() const) &wxWindowList::max_size, "C++: wxWindowList::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxWindowList::*)() const) &wxWindowList::empty, "C++: wxWindowList::empty() const --> bool");
		cl.def("front", (class wxWindow *& (wxWindowList::*)()) &wxWindowList::front, "C++: wxWindowList::front() --> class wxWindow *&", pybind11::return_value_policy::automatic);
		cl.def("back", (class wxWindow *& (wxWindowList::*)()) &wxWindowList::back, "C++: wxWindowList::back() --> class wxWindow *&", pybind11::return_value_policy::automatic);
		cl.def("push_front", [](wxWindowList &o) -> void { return o.push_front(); }, "");
		cl.def("push_front", (void (wxWindowList::*)(class wxWindow *const &)) &wxWindowList::push_front, "C++: wxWindowList::push_front(class wxWindow *const &) --> void", pybind11::arg("v"));
		cl.def("pop_front", (void (wxWindowList::*)()) &wxWindowList::pop_front, "C++: wxWindowList::pop_front() --> void");
		cl.def("push_back", [](wxWindowList &o) -> void { return o.push_back(); }, "");
		cl.def("push_back", (void (wxWindowList::*)(class wxWindow *const &)) &wxWindowList::push_back, "C++: wxWindowList::push_back(class wxWindow *const &) --> void", pybind11::arg("v"));
		cl.def("pop_back", (void (wxWindowList::*)()) &wxWindowList::pop_back, "C++: wxWindowList::pop_back() --> void");
		cl.def("assign", (void (wxWindowList::*)(class wxWindowList::const_iterator, const class wxWindowList::const_iterator &)) &wxWindowList::assign, "C++: wxWindowList::assign(class wxWindowList::const_iterator, const class wxWindowList::const_iterator &) --> void", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("assign", [](wxWindowList &o, unsigned long const & a0) -> void { return o.assign(a0); }, "", pybind11::arg("n"));
		cl.def("assign", (void (wxWindowList::*)(unsigned long, class wxWindow *const &)) &wxWindowList::assign, "C++: wxWindowList::assign(unsigned long, class wxWindow *const &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", (class wxWindowList::iterator (wxWindowList::*)(const class wxWindowList::iterator &, class wxWindow *const &)) &wxWindowList::insert, "C++: wxWindowList::insert(const class wxWindowList::iterator &, class wxWindow *const &) --> class wxWindowList::iterator", pybind11::arg("it"), pybind11::arg("v"));
		cl.def("insert", (void (wxWindowList::*)(const class wxWindowList::iterator &, unsigned long, class wxWindow *const &)) &wxWindowList::insert, "C++: wxWindowList::insert(const class wxWindowList::iterator &, unsigned long, class wxWindow *const &) --> void", pybind11::arg("it"), pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", (void (wxWindowList::*)(const class wxWindowList::iterator &, class wxWindowList::const_iterator, const class wxWindowList::const_iterator &)) &wxWindowList::insert, "C++: wxWindowList::insert(const class wxWindowList::iterator &, class wxWindowList::const_iterator, const class wxWindowList::const_iterator &) --> void", pybind11::arg("it"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("erase", (class wxWindowList::iterator (wxWindowList::*)(const class wxWindowList::iterator &)) &wxWindowList::erase, "C++: wxWindowList::erase(const class wxWindowList::iterator &) --> class wxWindowList::iterator", pybind11::arg("it"));
		cl.def("erase", (class wxWindowList::iterator (wxWindowList::*)(const class wxWindowList::iterator &, const class wxWindowList::iterator &)) &wxWindowList::erase, "C++: wxWindowList::erase(const class wxWindowList::iterator &, const class wxWindowList::iterator &) --> class wxWindowList::iterator", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("clear", (void (wxWindowList::*)()) &wxWindowList::clear, "C++: wxWindowList::clear() --> void");
		cl.def("splice", (void (wxWindowList::*)(const class wxWindowList::iterator &, class wxWindowList &, const class wxWindowList::iterator &, const class wxWindowList::iterator &)) &wxWindowList::splice, "C++: wxWindowList::splice(const class wxWindowList::iterator &, class wxWindowList &, const class wxWindowList::iterator &, const class wxWindowList::iterator &) --> void", pybind11::arg("it"), pybind11::arg("l"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("splice", (void (wxWindowList::*)(const class wxWindowList::iterator &, class wxWindowList &)) &wxWindowList::splice, "C++: wxWindowList::splice(const class wxWindowList::iterator &, class wxWindowList &) --> void", pybind11::arg("it"), pybind11::arg("l"));
		cl.def("splice", (void (wxWindowList::*)(const class wxWindowList::iterator &, class wxWindowList &, const class wxWindowList::iterator &)) &wxWindowList::splice, "C++: wxWindowList::splice(const class wxWindowList::iterator &, class wxWindowList &, const class wxWindowList::iterator &) --> void", pybind11::arg("it"), pybind11::arg("l"), pybind11::arg("first"));
		cl.def("remove", (void (wxWindowList::*)(class wxWindow *const &)) &wxWindowList::remove, "C++: wxWindowList::remove(class wxWindow *const &) --> void", pybind11::arg("v"));
		cl.def("reverse", (void (wxWindowList::*)()) &wxWindowList::reverse, "C++: wxWindowList::reverse() --> void");

		{ // wxWindowList::compatibility_iterator file: line:705
			auto & enclosing_class = cl;
			pybind11::class_<wxWindowList::compatibility_iterator, std::shared_ptr<wxWindowList::compatibility_iterator>> cl(enclosing_class, "compatibility_iterator", "");
			cl.def( pybind11::init( [](){ return new wxWindowList::compatibility_iterator(); } ), "doc" );
			cl.def( pybind11::init<class wxWindowListNode *>(), pybind11::arg("ptr") );

		}

		{ // wxWindowList::iterator file: line:802
			auto & enclosing_class = cl;
			pybind11::class_<wxWindowList::iterator, std::shared_ptr<wxWindowList::iterator>> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init<class wxWindowListNode *, class wxWindowListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxWindowList::iterator(); } ) );
			cl.def( pybind11::init( [](wxWindowList::iterator const &o){ return new wxWindowList::iterator(o); } ) );
			cl.def("__mul__", (class wxWindow *& (wxWindowList::iterator::*)() const) &wxWindowList::iterator::operator*, "C++: wxWindowList::iterator::operator*() const --> class wxWindow *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxWindowList::iterator & (wxWindowList::iterator::*)()) &wxWindowList::iterator::operator++, "C++: wxWindowList::iterator::operator++() --> class wxWindowList::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxWindowList::iterator (wxWindowList::iterator::*)(int)) &wxWindowList::iterator::operator++, "C++: wxWindowList::iterator::operator++(int) --> const class wxWindowList::iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxWindowList::iterator & (wxWindowList::iterator::*)()) &wxWindowList::iterator::operator--, "C++: wxWindowList::iterator::operator--() --> class wxWindowList::iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxWindowList::iterator (wxWindowList::iterator::*)(int)) &wxWindowList::iterator::operator--, "C++: wxWindowList::iterator::operator--(int) --> const class wxWindowList::iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxWindowList::iterator::*)(const class wxWindowList::iterator &) const) &wxWindowList::iterator::operator!=, "C++: wxWindowList::iterator::operator!=(const class wxWindowList::iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxWindowList::iterator::*)(const class wxWindowList::iterator &) const) &wxWindowList::iterator::operator==, "C++: wxWindowList::iterator::operator==(const class wxWindowList::iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxWindowList::const_iterator file: line:852
			auto & enclosing_class = cl;
			pybind11::class_<wxWindowList::const_iterator, std::shared_ptr<wxWindowList::const_iterator>> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init<class wxWindowListNode *, class wxWindowListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxWindowList::const_iterator(); } ) );
			cl.def( pybind11::init<const class wxWindowList::iterator &>(), pybind11::arg("it") );

			cl.def( pybind11::init( [](wxWindowList::const_iterator const &o){ return new wxWindowList::const_iterator(o); } ) );
			cl.def("__mul__", (class wxWindow *const & (wxWindowList::const_iterator::*)() const) &wxWindowList::const_iterator::operator*, "C++: wxWindowList::const_iterator::operator*() const --> class wxWindow *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxWindowList::const_iterator & (wxWindowList::const_iterator::*)()) &wxWindowList::const_iterator::operator++, "C++: wxWindowList::const_iterator::operator++() --> class wxWindowList::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxWindowList::const_iterator (wxWindowList::const_iterator::*)(int)) &wxWindowList::const_iterator::operator++, "C++: wxWindowList::const_iterator::operator++(int) --> const class wxWindowList::const_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxWindowList::const_iterator & (wxWindowList::const_iterator::*)()) &wxWindowList::const_iterator::operator--, "C++: wxWindowList::const_iterator::operator--() --> class wxWindowList::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxWindowList::const_iterator (wxWindowList::const_iterator::*)(int)) &wxWindowList::const_iterator::operator--, "C++: wxWindowList::const_iterator::operator--(int) --> const class wxWindowList::const_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxWindowList::const_iterator::*)(const class wxWindowList::const_iterator &) const) &wxWindowList::const_iterator::operator!=, "C++: wxWindowList::const_iterator::operator!=(const class wxWindowList::const_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxWindowList::const_iterator::*)(const class wxWindowList::const_iterator &) const) &wxWindowList::const_iterator::operator==, "C++: wxWindowList::const_iterator::operator==(const class wxWindowList::const_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxWindowList::reverse_iterator file: line:905
			auto & enclosing_class = cl;
			pybind11::class_<wxWindowList::reverse_iterator, std::shared_ptr<wxWindowList::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init<class wxWindowListNode *, class wxWindowListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxWindowList::reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxWindowList::reverse_iterator const &o){ return new wxWindowList::reverse_iterator(o); } ) );
			cl.def("__mul__", (class wxWindow *& (wxWindowList::reverse_iterator::*)() const) &wxWindowList::reverse_iterator::operator*, "C++: wxWindowList::reverse_iterator::operator*() const --> class wxWindow *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxWindowList::reverse_iterator & (wxWindowList::reverse_iterator::*)()) &wxWindowList::reverse_iterator::operator++, "C++: wxWindowList::reverse_iterator::operator++() --> class wxWindowList::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxWindowList::reverse_iterator (wxWindowList::reverse_iterator::*)(int)) &wxWindowList::reverse_iterator::operator++, "C++: wxWindowList::reverse_iterator::operator++(int) --> const class wxWindowList::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxWindowList::reverse_iterator & (wxWindowList::reverse_iterator::*)()) &wxWindowList::reverse_iterator::operator--, "C++: wxWindowList::reverse_iterator::operator--() --> class wxWindowList::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxWindowList::reverse_iterator (wxWindowList::reverse_iterator::*)(int)) &wxWindowList::reverse_iterator::operator--, "C++: wxWindowList::reverse_iterator::operator--(int) --> const class wxWindowList::reverse_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxWindowList::reverse_iterator::*)(const class wxWindowList::reverse_iterator &) const) &wxWindowList::reverse_iterator::operator!=, "C++: wxWindowList::reverse_iterator::operator!=(const class wxWindowList::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxWindowList::reverse_iterator::*)(const class wxWindowList::reverse_iterator &) const) &wxWindowList::reverse_iterator::operator==, "C++: wxWindowList::reverse_iterator::operator==(const class wxWindowList::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxWindowList::const_reverse_iterator file: line:944
			auto & enclosing_class = cl;
			pybind11::class_<wxWindowList::const_reverse_iterator, std::shared_ptr<wxWindowList::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init<class wxWindowListNode *, class wxWindowListNode *>(), pybind11::arg("node"), pybind11::arg("init") );

			cl.def( pybind11::init( [](){ return new wxWindowList::const_reverse_iterator(); } ) );
			cl.def( pybind11::init<const class wxWindowList::reverse_iterator &>(), pybind11::arg("it") );

			cl.def( pybind11::init( [](wxWindowList::const_reverse_iterator const &o){ return new wxWindowList::const_reverse_iterator(o); } ) );
			cl.def("__mul__", (class wxWindow *const & (wxWindowList::const_reverse_iterator::*)() const) &wxWindowList::const_reverse_iterator::operator*, "C++: wxWindowList::const_reverse_iterator::operator*() const --> class wxWindow *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxWindowList::const_reverse_iterator & (wxWindowList::const_reverse_iterator::*)()) &wxWindowList::const_reverse_iterator::operator++, "C++: wxWindowList::const_reverse_iterator::operator++() --> class wxWindowList::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxWindowList::const_reverse_iterator (wxWindowList::const_reverse_iterator::*)(int)) &wxWindowList::const_reverse_iterator::operator++, "C++: wxWindowList::const_reverse_iterator::operator++(int) --> const class wxWindowList::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxWindowList::const_reverse_iterator & (wxWindowList::const_reverse_iterator::*)()) &wxWindowList::const_reverse_iterator::operator--, "C++: wxWindowList::const_reverse_iterator::operator--() --> class wxWindowList::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxWindowList::const_reverse_iterator (wxWindowList::const_reverse_iterator::*)(int)) &wxWindowList::const_reverse_iterator::operator--, "C++: wxWindowList::const_reverse_iterator::operator--(int) --> const class wxWindowList::const_reverse_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxWindowList::const_reverse_iterator::*)(const class wxWindowList::const_reverse_iterator &) const) &wxWindowList::const_reverse_iterator::operator!=, "C++: wxWindowList::const_reverse_iterator::operator!=(const class wxWindowList::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxWindowList::const_reverse_iterator::*)(const class wxWindowList::const_reverse_iterator &) const) &wxWindowList::const_reverse_iterator::operator==, "C++: wxWindowList::const_reverse_iterator::operator==(const class wxWindowList::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
}

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

// wxControlContainer file: line:136
struct PyCallBack_wxControlContainer : public wxControlContainer {
	using wxControlContainer::wxControlContainer;

	bool SetFocusToChild() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxControlContainer *>(this), "SetFocusToChild");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxControlContainer::SetFocusToChild();
	}
};

// wxDialogBase file: line:68
struct PyCallBack_wxDialogBase : public wxDialogBase {
	using wxDialogBase::wxDialogBase;

	int ShowModal() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogBase *>(this), "ShowModal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxDialogBase::ShowModal\"");
	}
	void EndModal(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogBase *>(this), "EndModal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxDialogBase::EndModal\"");
	}
	bool IsModal() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogBase *>(this), "IsModal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxDialogBase::IsModal\"");
	}
	void ShowWindowModal() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogBase *>(this), "ShowWindowModal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxDialogBase::ShowWindowModal();
	}
	void SendWindowModalDialogEvent(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogBase *>(this), "SendWindowModalDialogEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxDialogBase::SendWindowModalDialogEvent(a0);
	}
	bool DoLayoutAdaptation() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogBase *>(this), "DoLayoutAdaptation");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxDialogBase::DoLayoutAdaptation();
	}
	bool CanDoLayoutAdaptation() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogBase *>(this), "CanDoLayoutAdaptation");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxDialogBase::CanDoLayoutAdaptation();
	}
	class wxWindow * GetContentWindow() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogBase *>(this), "GetContentWindow");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxWindow *>::value) {
				static pybind11::detail::override_caster_t<class wxWindow *> caster;
				return pybind11::detail::cast_ref<class wxWindow *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxWindow *>(std::move(o));
		}
		return wxDialogBase::GetContentWindow();
	}
	enum wxDialogModality GetModality() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogBase *>(this), "GetModality");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxDialogModality>::value) {
				static pybind11::detail::override_caster_t<enum wxDialogModality> caster;
				return pybind11::detail::cast_ref<enum wxDialogModality>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxDialogModality>(std::move(o));
		}
		return wxDialogBase::GetModality();
	}
	bool IsEscapeKey(const class wxKeyEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogBase *>(this), "IsEscapeKey");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxDialogBase::IsEscapeKey(a0);
	}
	const struct wxEventTable * GetEventTable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogBase *>(this), "GetEventTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const struct wxEventTable *>::value) {
				static pybind11::detail::override_caster_t<const struct wxEventTable *> caster;
				return pybind11::detail::cast_ref<const struct wxEventTable *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const struct wxEventTable *>(std::move(o));
		}
		return wxDialogBase::GetEventTable();
	}
	class wxEventHashTable & GetEventHashTable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogBase *>(this), "GetEventHashTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEventHashTable &>::value) {
				static pybind11::detail::override_caster_t<class wxEventHashTable &> caster;
				return pybind11::detail::cast_ref<class wxEventHashTable &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEventHashTable &>(std::move(o));
		}
		return wxDialogBase::GetEventHashTable();
	}
};

// wxDialogLayoutAdapter file: line:285
struct PyCallBack_wxDialogLayoutAdapter : public wxDialogLayoutAdapter {
	using wxDialogLayoutAdapter::wxDialogLayoutAdapter;

	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogLayoutAdapter *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxDialogLayoutAdapter::GetClassInfo();
	}
	bool CanDoLayoutAdaptation(class wxDialog * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogLayoutAdapter *>(this), "CanDoLayoutAdaptation");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxDialogLayoutAdapter::CanDoLayoutAdaptation\"");
	}
	bool DoLayoutAdaptation(class wxDialog * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogLayoutAdapter *>(this), "DoLayoutAdaptation");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxDialogLayoutAdapter::DoLayoutAdaptation\"");
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogLayoutAdapter *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialogLayoutAdapter *>(this), "CloneRefData");
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

// wxStandardDialogLayoutAdapter file: line:303
struct PyCallBack_wxStandardDialogLayoutAdapter : public wxStandardDialogLayoutAdapter {
	using wxStandardDialogLayoutAdapter::wxStandardDialogLayoutAdapter;

	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStandardDialogLayoutAdapter *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxStandardDialogLayoutAdapter::GetClassInfo();
	}
	bool CanDoLayoutAdaptation(class wxDialog * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStandardDialogLayoutAdapter *>(this), "CanDoLayoutAdaptation");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxStandardDialogLayoutAdapter::CanDoLayoutAdaptation(a0);
	}
	bool DoLayoutAdaptation(class wxDialog * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStandardDialogLayoutAdapter *>(this), "DoLayoutAdaptation");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxStandardDialogLayoutAdapter::DoLayoutAdaptation(a0);
	}
	bool FitWithScrolling(class wxDialog * a0, class wxWindowList & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStandardDialogLayoutAdapter *>(this), "FitWithScrolling");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxStandardDialogLayoutAdapter::FitWithScrolling(a0, a1);
	}
	int MustScroll(class wxDialog * a0, class wxSize & a1, class wxSize & a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStandardDialogLayoutAdapter *>(this), "MustScroll");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxStandardDialogLayoutAdapter::MustScroll(a0, a1, a2);
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStandardDialogLayoutAdapter *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStandardDialogLayoutAdapter *>(this), "CloneRefData");
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

// wxDialog file: line:19
struct PyCallBack_wxDialog : public wxDialog {
	using wxDialog::wxDialog;

	bool Show(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialog *>(this), "Show");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxDialog::Show(a0);
	}
	int ShowModal() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialog *>(this), "ShowModal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxDialog::ShowModal();
	}
	void EndModal(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialog *>(this), "EndModal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxDialog::EndModal(a0);
	}
	bool IsModal() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialog *>(this), "IsModal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxDialog::IsModal();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialog *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxDialog::GetClassInfo();
	}
	void ShowWindowModal() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialog *>(this), "ShowWindowModal");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxDialogBase::ShowWindowModal();
	}
	void SendWindowModalDialogEvent(int a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialog *>(this), "SendWindowModalDialogEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxDialogBase::SendWindowModalDialogEvent(a0);
	}
	bool DoLayoutAdaptation() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialog *>(this), "DoLayoutAdaptation");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxDialogBase::DoLayoutAdaptation();
	}
	bool CanDoLayoutAdaptation() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialog *>(this), "CanDoLayoutAdaptation");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxDialogBase::CanDoLayoutAdaptation();
	}
	class wxWindow * GetContentWindow() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialog *>(this), "GetContentWindow");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxWindow *>::value) {
				static pybind11::detail::override_caster_t<class wxWindow *> caster;
				return pybind11::detail::cast_ref<class wxWindow *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxWindow *>(std::move(o));
		}
		return wxDialogBase::GetContentWindow();
	}
	enum wxDialogModality GetModality() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialog *>(this), "GetModality");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxDialogModality>::value) {
				static pybind11::detail::override_caster_t<enum wxDialogModality> caster;
				return pybind11::detail::cast_ref<enum wxDialogModality>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxDialogModality>(std::move(o));
		}
		return wxDialogBase::GetModality();
	}
	bool IsEscapeKey(const class wxKeyEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialog *>(this), "IsEscapeKey");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxDialogBase::IsEscapeKey(a0);
	}
	const struct wxEventTable * GetEventTable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialog *>(this), "GetEventTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const struct wxEventTable *>::value) {
				static pybind11::detail::override_caster_t<const struct wxEventTable *> caster;
				return pybind11::detail::cast_ref<const struct wxEventTable *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const struct wxEventTable *>(std::move(o));
		}
		return wxDialogBase::GetEventTable();
	}
	class wxEventHashTable & GetEventHashTable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDialog *>(this), "GetEventHashTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEventHashTable &>::value) {
				static pybind11::detail::override_caster_t<class wxEventHashTable &> caster;
				return pybind11::detail::cast_ref<class wxEventHashTable &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEventHashTable &>(std::move(o));
		}
		return wxDialogBase::GetEventHashTable();
	}
};

void bind_unknown_unknown_78(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxControlContainer file: line:136
		pybind11::class_<wxControlContainer, std::shared_ptr<wxControlContainer>, PyCallBack_wxControlContainer, wxControlContainerBase> cl(M(""), "wxControlContainer", "");
		cl.def( pybind11::init( [](){ return new wxControlContainer(); }, [](){ return new PyCallBack_wxControlContainer(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxControlContainer const &o){ return new PyCallBack_wxControlContainer(o); } ) );
		cl.def( pybind11::init( [](wxControlContainer const &o){ return new wxControlContainer(o); } ) );
		cl.def("assign", (class wxControlContainer & (wxControlContainer::*)(const class wxControlContainer &)) &wxControlContainer::operator=, "C++: wxControlContainer::operator=(const class wxControlContainer &) --> class wxControlContainer &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// wxAtomicInc(unsigned int &) file: line:34
	M("").def("wxAtomicInc", (void (*)(unsigned int &)) &wxAtomicInc, "C++: wxAtomicInc(unsigned int &) --> void", pybind11::arg("value"));

	// wxAtomicDec(unsigned int &) file: line:39
	M("").def("wxAtomicDec", (unsigned int (*)(unsigned int &)) &wxAtomicDec, "C++: wxAtomicDec(unsigned int &) --> unsigned int", pybind11::arg("value"));

	// wxAtomicInc(int &) file: line:147
	M("").def("wxAtomicInc", (void (*)(int &)) &wxAtomicInc, "C++: wxAtomicInc(int &) --> void", pybind11::arg("value"));

	// wxAtomicDec(int &) file: line:148
	M("").def("wxAtomicDec", (int (*)(int &)) &wxAtomicDec, "C++: wxAtomicDec(int &) --> int", pybind11::arg("value"));

	// wxDialogLayoutAdaptationMode file: line:52
	pybind11::enum_<wxDialogLayoutAdaptationMode>(M(""), "wxDialogLayoutAdaptationMode", pybind11::arithmetic(), "")
		.value("wxDIALOG_ADAPTATION_MODE_DEFAULT", wxDIALOG_ADAPTATION_MODE_DEFAULT)
		.value("wxDIALOG_ADAPTATION_MODE_ENABLED", wxDIALOG_ADAPTATION_MODE_ENABLED)
		.value("wxDIALOG_ADAPTATION_MODE_DISABLED", wxDIALOG_ADAPTATION_MODE_DISABLED)
		.export_values();

;

	// wxDialogModality file: line:59
	pybind11::enum_<wxDialogModality>(M(""), "wxDialogModality", pybind11::arithmetic(), "")
		.value("wxDIALOG_MODALITY_NONE", wxDIALOG_MODALITY_NONE)
		.value("wxDIALOG_MODALITY_WINDOW_MODAL", wxDIALOG_MODALITY_WINDOW_MODAL)
		.value("wxDIALOG_MODALITY_APP_MODAL", wxDIALOG_MODALITY_APP_MODAL)
		.export_values();

;

	{ // wxDialogBase file: line:68
		pybind11::class_<wxDialogBase, std::shared_ptr<wxDialogBase>, PyCallBack_wxDialogBase> cl(M(""), "wxDialogBase", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxDialogBase(); } ) );
		cl.def("ShowModal", (int (wxDialogBase::*)()) &wxDialogBase::ShowModal, "C++: wxDialogBase::ShowModal() --> int");
		cl.def("EndModal", (void (wxDialogBase::*)(int)) &wxDialogBase::EndModal, "C++: wxDialogBase::EndModal(int) --> void", pybind11::arg("retCode"));
		cl.def("IsModal", (bool (wxDialogBase::*)() const) &wxDialogBase::IsModal, "C++: wxDialogBase::IsModal() const --> bool");
		cl.def("ShowWindowModal", (void (wxDialogBase::*)()) &wxDialogBase::ShowWindowModal, "C++: wxDialogBase::ShowWindowModal() --> void");
		cl.def("SendWindowModalDialogEvent", (void (wxDialogBase::*)(int)) &wxDialogBase::SendWindowModalDialogEvent, "C++: wxDialogBase::SendWindowModalDialogEvent(int) --> void", pybind11::arg("type"));
		cl.def("SetReturnCode", (void (wxDialogBase::*)(int)) &wxDialogBase::SetReturnCode, "C++: wxDialogBase::SetReturnCode(int) --> void", pybind11::arg("returnCode"));
		cl.def("GetReturnCode", (int (wxDialogBase::*)() const) &wxDialogBase::GetReturnCode, "C++: wxDialogBase::GetReturnCode() const --> int");
		cl.def("SetAffirmativeId", (void (wxDialogBase::*)(int)) &wxDialogBase::SetAffirmativeId, "C++: wxDialogBase::SetAffirmativeId(int) --> void", pybind11::arg("affirmativeId"));
		cl.def("GetAffirmativeId", (int (wxDialogBase::*)() const) &wxDialogBase::GetAffirmativeId, "C++: wxDialogBase::GetAffirmativeId() const --> int");
		cl.def("SetEscapeId", (void (wxDialogBase::*)(int)) &wxDialogBase::SetEscapeId, "C++: wxDialogBase::SetEscapeId(int) --> void", pybind11::arg("escapeId"));
		cl.def("GetEscapeId", (int (wxDialogBase::*)() const) &wxDialogBase::GetEscapeId, "C++: wxDialogBase::GetEscapeId() const --> int");
		cl.def("GetParentForModalDialog", (class wxWindow * (wxDialogBase::*)(class wxWindow *, long) const) &wxDialogBase::GetParentForModalDialog, "C++: wxDialogBase::GetParentForModalDialog(class wxWindow *, long) const --> class wxWindow *", pybind11::return_value_policy::automatic, pybind11::arg("parent"), pybind11::arg("style"));
		cl.def("GetParentForModalDialog", (class wxWindow * (wxDialogBase::*)() const) &wxDialogBase::GetParentForModalDialog, "C++: wxDialogBase::GetParentForModalDialog() const --> class wxWindow *", pybind11::return_value_policy::automatic);
		cl.def("DoLayoutAdaptation", (bool (wxDialogBase::*)()) &wxDialogBase::DoLayoutAdaptation, "C++: wxDialogBase::DoLayoutAdaptation() --> bool");
		cl.def("CanDoLayoutAdaptation", (bool (wxDialogBase::*)()) &wxDialogBase::CanDoLayoutAdaptation, "C++: wxDialogBase::CanDoLayoutAdaptation() --> bool");
		cl.def("GetContentWindow", (class wxWindow * (wxDialogBase::*)() const) &wxDialogBase::GetContentWindow, "C++: wxDialogBase::GetContentWindow() const --> class wxWindow *", pybind11::return_value_policy::automatic);
		cl.def("AddMainButtonId", (void (wxDialogBase::*)(int)) &wxDialogBase::AddMainButtonId, "C++: wxDialogBase::AddMainButtonId(int) --> void", pybind11::arg("id"));
		cl.def("GetMainButtonIds", (class wxArrayInt & (wxDialogBase::*)()) &wxDialogBase::GetMainButtonIds, "C++: wxDialogBase::GetMainButtonIds() --> class wxArrayInt &", pybind11::return_value_policy::automatic);
		cl.def("IsMainButtonId", (bool (wxDialogBase::*)(int) const) &wxDialogBase::IsMainButtonId, "C++: wxDialogBase::IsMainButtonId(int) const --> bool", pybind11::arg("id"));
		cl.def("SetLayoutAdaptationLevel", (void (wxDialogBase::*)(int)) &wxDialogBase::SetLayoutAdaptationLevel, "C++: wxDialogBase::SetLayoutAdaptationLevel(int) --> void", pybind11::arg("level"));
		cl.def("GetLayoutAdaptationLevel", (int (wxDialogBase::*)() const) &wxDialogBase::GetLayoutAdaptationLevel, "C++: wxDialogBase::GetLayoutAdaptationLevel() const --> int");
		cl.def("SetLayoutAdaptationMode", (void (wxDialogBase::*)(enum wxDialogLayoutAdaptationMode)) &wxDialogBase::SetLayoutAdaptationMode, "Override global adaptation enabled/disabled status\n\nC++: wxDialogBase::SetLayoutAdaptationMode(enum wxDialogLayoutAdaptationMode) --> void", pybind11::arg("mode"));
		cl.def("GetLayoutAdaptationMode", (enum wxDialogLayoutAdaptationMode (wxDialogBase::*)() const) &wxDialogBase::GetLayoutAdaptationMode, "C++: wxDialogBase::GetLayoutAdaptationMode() const --> enum wxDialogLayoutAdaptationMode");
		cl.def("SetLayoutAdaptationDone", (void (wxDialogBase::*)(bool)) &wxDialogBase::SetLayoutAdaptationDone, "C++: wxDialogBase::SetLayoutAdaptationDone(bool) --> void", pybind11::arg("adaptationDone"));
		cl.def("GetLayoutAdaptationDone", (bool (wxDialogBase::*)() const) &wxDialogBase::GetLayoutAdaptationDone, "C++: wxDialogBase::GetLayoutAdaptationDone() const --> bool");
		cl.def_static("SetLayoutAdapter", (class wxDialogLayoutAdapter * (*)(class wxDialogLayoutAdapter *)) &wxDialogBase::SetLayoutAdapter, "C++: wxDialogBase::SetLayoutAdapter(class wxDialogLayoutAdapter *) --> class wxDialogLayoutAdapter *", pybind11::return_value_policy::automatic, pybind11::arg("adapter"));
		cl.def_static("GetLayoutAdapter", (class wxDialogLayoutAdapter * (*)()) &wxDialogBase::GetLayoutAdapter, "C++: wxDialogBase::GetLayoutAdapter() --> class wxDialogLayoutAdapter *", pybind11::return_value_policy::automatic);
		cl.def_static("IsLayoutAdaptationEnabled", (bool (*)()) &wxDialogBase::IsLayoutAdaptationEnabled, "C++: wxDialogBase::IsLayoutAdaptationEnabled() --> bool");
		cl.def_static("EnableLayoutAdaptation", (void (*)(bool)) &wxDialogBase::EnableLayoutAdaptation, "C++: wxDialogBase::EnableLayoutAdaptation(bool) --> void", pybind11::arg("enable"));
		cl.def("GetModality", (enum wxDialogModality (wxDialogBase::*)() const) &wxDialogBase::GetModality, "C++: wxDialogBase::GetModality() const --> enum wxDialogModality");
	}
	{ // wxDialogLayoutAdapter file: line:285
		pybind11::class_<wxDialogLayoutAdapter, std::shared_ptr<wxDialogLayoutAdapter>, PyCallBack_wxDialogLayoutAdapter, wxObject> cl(M(""), "wxDialogLayoutAdapter", "Base class for layout adapters - code that, for example, turns a dialog into a\n scrolling dialog if there isn't enough screen space. You can derive further\n adapter classes to do any other kind of adaptation, such as applying a watermark, or adding\n a help mechanism.");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxDialogLayoutAdapter(); } ) );
		cl.def(pybind11::init<PyCallBack_wxDialogLayoutAdapter const &>());
		cl.def("GetClassInfo", (class wxClassInfo * (wxDialogLayoutAdapter::*)() const) &wxDialogLayoutAdapter::GetClassInfo, "C++: wxDialogLayoutAdapter::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def("CanDoLayoutAdaptation", (bool (wxDialogLayoutAdapter::*)(class wxDialog *)) &wxDialogLayoutAdapter::CanDoLayoutAdaptation, "C++: wxDialogLayoutAdapter::CanDoLayoutAdaptation(class wxDialog *) --> bool", pybind11::arg("dialog"));
		cl.def("DoLayoutAdaptation", (bool (wxDialogLayoutAdapter::*)(class wxDialog *)) &wxDialogLayoutAdapter::DoLayoutAdaptation, "C++: wxDialogLayoutAdapter::DoLayoutAdaptation(class wxDialog *) --> bool", pybind11::arg("dialog"));
		cl.def("assign", (class wxDialogLayoutAdapter & (wxDialogLayoutAdapter::*)(const class wxDialogLayoutAdapter &)) &wxDialogLayoutAdapter::operator=, "C++: wxDialogLayoutAdapter::operator=(const class wxDialogLayoutAdapter &) --> class wxDialogLayoutAdapter &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxStandardDialogLayoutAdapter file: line:303
		pybind11::class_<wxStandardDialogLayoutAdapter, std::shared_ptr<wxStandardDialogLayoutAdapter>, PyCallBack_wxStandardDialogLayoutAdapter, wxDialogLayoutAdapter> cl(M(""), "wxStandardDialogLayoutAdapter", "Standard adapter. Does scrolling adaptation for paged and regular dialogs.\n\n ");
		cl.def( pybind11::init( [](){ return new wxStandardDialogLayoutAdapter(); }, [](){ return new PyCallBack_wxStandardDialogLayoutAdapter(); } ) );
		cl.def("GetClassInfo", (class wxClassInfo * (wxStandardDialogLayoutAdapter::*)() const) &wxStandardDialogLayoutAdapter::GetClassInfo, "C++: wxStandardDialogLayoutAdapter::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def("CanDoLayoutAdaptation", (bool (wxStandardDialogLayoutAdapter::*)(class wxDialog *)) &wxStandardDialogLayoutAdapter::CanDoLayoutAdaptation, "C++: wxStandardDialogLayoutAdapter::CanDoLayoutAdaptation(class wxDialog *) --> bool", pybind11::arg("dialog"));
		cl.def("DoLayoutAdaptation", (bool (wxStandardDialogLayoutAdapter::*)(class wxDialog *)) &wxStandardDialogLayoutAdapter::DoLayoutAdaptation, "C++: wxStandardDialogLayoutAdapter::DoLayoutAdaptation(class wxDialog *) --> bool", pybind11::arg("dialog"));
		cl.def("FitWithScrolling", (bool (wxStandardDialogLayoutAdapter::*)(class wxDialog *, class wxWindowList &)) &wxStandardDialogLayoutAdapter::FitWithScrolling, "C++: wxStandardDialogLayoutAdapter::FitWithScrolling(class wxDialog *, class wxWindowList &) --> bool", pybind11::arg("dialog"), pybind11::arg("windows"));
		cl.def_static("DoFitWithScrolling", (bool (*)(class wxDialog *, class wxWindowList &)) &wxStandardDialogLayoutAdapter::DoFitWithScrolling, "C++: wxStandardDialogLayoutAdapter::DoFitWithScrolling(class wxDialog *, class wxWindowList &) --> bool", pybind11::arg("dialog"), pybind11::arg("windows"));
		cl.def("MustScroll", (int (wxStandardDialogLayoutAdapter::*)(class wxDialog *, class wxSize &, class wxSize &)) &wxStandardDialogLayoutAdapter::MustScroll, "C++: wxStandardDialogLayoutAdapter::MustScroll(class wxDialog *, class wxSize &, class wxSize &) --> int", pybind11::arg("dialog"), pybind11::arg("windowSize"), pybind11::arg("displaySize"));
		cl.def_static("DoMustScroll", (int (*)(class wxDialog *, class wxSize &, class wxSize &)) &wxStandardDialogLayoutAdapter::DoMustScroll, "C++: wxStandardDialogLayoutAdapter::DoMustScroll(class wxDialog *, class wxSize &, class wxSize &) --> int", pybind11::arg("dialog"), pybind11::arg("windowSize"), pybind11::arg("displaySize"));
		cl.def("assign", (class wxStandardDialogLayoutAdapter & (wxStandardDialogLayoutAdapter::*)(const class wxStandardDialogLayoutAdapter &)) &wxStandardDialogLayoutAdapter::operator=, "C++: wxStandardDialogLayoutAdapter::operator=(const class wxStandardDialogLayoutAdapter &) --> class wxStandardDialogLayoutAdapter &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxDialog file: line:19
		pybind11::class_<wxDialog, std::shared_ptr<wxDialog>, PyCallBack_wxDialog, wxDialogBase> cl(M(""), "wxDialog", "");
		cl.def( pybind11::init( [](){ return new wxDialog(); }, [](){ return new PyCallBack_wxDialog(); } ) );
		cl.def( pybind11::init( [](class wxWindow * a0, int const & a1, const class wxString & a2){ return new wxDialog(a0, a1, a2); }, [](class wxWindow * a0, int const & a1, const class wxString & a2){ return new PyCallBack_wxDialog(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init( [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3){ return new wxDialog(a0, a1, a2, a3); }, [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3){ return new PyCallBack_wxDialog(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init( [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3, const class wxSize & a4){ return new wxDialog(a0, a1, a2, a3, a4); }, [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3, const class wxSize & a4){ return new PyCallBack_wxDialog(a0, a1, a2, a3, a4); } ), "doc");
		cl.def( pybind11::init( [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3, const class wxSize & a4, long const & a5){ return new wxDialog(a0, a1, a2, a3, a4, a5); }, [](class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3, const class wxSize & a4, long const & a5){ return new PyCallBack_wxDialog(a0, a1, a2, a3, a4, a5); } ), "doc");
		cl.def( pybind11::init<class wxWindow *, int, const class wxString &, const class wxPoint &, const class wxSize &, long, const class wxString &>(), pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("title"), pybind11::arg("pos"), pybind11::arg("size"), pybind11::arg("style"), pybind11::arg("name") );

		cl.def( pybind11::init( [](PyCallBack_wxDialog const &o){ return new PyCallBack_wxDialog(o); } ) );
		cl.def( pybind11::init( [](wxDialog const &o){ return new wxDialog(o); } ) );
		cl.def("Create", [](wxDialog &o, class wxWindow * a0, int const & a1, const class wxString & a2) -> bool { return o.Create(a0, a1, a2); }, "", pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("title"));
		cl.def("Create", [](wxDialog &o, class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3) -> bool { return o.Create(a0, a1, a2, a3); }, "", pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("title"), pybind11::arg("pos"));
		cl.def("Create", [](wxDialog &o, class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3, const class wxSize & a4) -> bool { return o.Create(a0, a1, a2, a3, a4); }, "", pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("title"), pybind11::arg("pos"), pybind11::arg("size"));
		cl.def("Create", [](wxDialog &o, class wxWindow * a0, int const & a1, const class wxString & a2, const class wxPoint & a3, const class wxSize & a4, long const & a5) -> bool { return o.Create(a0, a1, a2, a3, a4, a5); }, "", pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("title"), pybind11::arg("pos"), pybind11::arg("size"), pybind11::arg("style"));
		cl.def("Create", (bool (wxDialog::*)(class wxWindow *, int, const class wxString &, const class wxPoint &, const class wxSize &, long, const class wxString &)) &wxDialog::Create, "C++: wxDialog::Create(class wxWindow *, int, const class wxString &, const class wxPoint &, const class wxSize &, long, const class wxString &) --> bool", pybind11::arg("parent"), pybind11::arg("id"), pybind11::arg("title"), pybind11::arg("pos"), pybind11::arg("size"), pybind11::arg("style"), pybind11::arg("name"));
		cl.def("Show", [](wxDialog &o) -> bool { return o.Show(); }, "");
		cl.def("Show", (bool (wxDialog::*)(bool)) &wxDialog::Show, "C++: wxDialog::Show(bool) --> bool", pybind11::arg("show"));
		cl.def("ShowModal", (int (wxDialog::*)()) &wxDialog::ShowModal, "C++: wxDialog::ShowModal() --> int");
		cl.def("EndModal", (void (wxDialog::*)(int)) &wxDialog::EndModal, "C++: wxDialog::EndModal(int) --> void", pybind11::arg("retCode"));
		cl.def("IsModal", (bool (wxDialog::*)() const) &wxDialog::IsModal, "C++: wxDialog::IsModal() const --> bool");
		cl.def("GetClassInfo", (class wxClassInfo * (wxDialog::*)() const) &wxDialog::GetClassInfo, "C++: wxDialog::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxDialog::wxCreateObject, "C++: wxDialog::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxDialog & (wxDialog::*)(const class wxDialog &)) &wxDialog::operator=, "C++: wxDialog::operator=(const class wxDialog &) --> class wxDialog &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}

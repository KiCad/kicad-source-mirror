#include <bitmap_types.h> // KiBitmap
#include <bitmap_types.h> // KiBitmapNew
#include <bitmap_types.h> // KiIconScale
#include <bitmap_types.h> // KiScaledBitmap
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
#include <wx/fdrepdlg.h> // wxFindDialogEvent
#include <wx/fdrepdlg.h> // wxFindReplaceData
#include <wx/fdrepdlg.h> // wxFindReplaceDialogBase
#include <wx/fdrepdlg.h> // wxFindReplaceDialogStyles
#include <wx/fdrepdlg.h> // wxFindReplaceFlags

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxWindowModalDialogEvent file: line:372
struct PyCallBack_wxWindowModalDialogEvent : public wxWindowModalDialogEvent {
	using wxWindowModalDialogEvent::wxWindowModalDialogEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowModalDialogEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxWindowModalDialogEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowModalDialogEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxWindowModalDialogEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowModalDialogEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowModalDialogEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxWindowModalDialogEvent *>(this), "CloneRefData");
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

// wxFindReplaceData file:wx/fdrepdlg.h line:62
struct PyCallBack_wxFindReplaceData : public wxFindReplaceData {
	using wxFindReplaceData::wxFindReplaceData;

	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceData *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxObject::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceData *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceData *>(this), "CloneRefData");
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

// wxFindReplaceDialogBase file:wx/fdrepdlg.h line:95
struct PyCallBack_wxFindReplaceDialogBase : public wxFindReplaceDialogBase {
	using wxFindReplaceDialogBase::wxFindReplaceDialogBase;

	bool Show(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialogBase *>(this), "Show");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialogBase *>(this), "ShowModal");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialogBase *>(this), "EndModal");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialogBase *>(this), "IsModal");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialogBase *>(this), "GetClassInfo");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialogBase *>(this), "ShowWindowModal");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialogBase *>(this), "SendWindowModalDialogEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialogBase *>(this), "DoLayoutAdaptation");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialogBase *>(this), "CanDoLayoutAdaptation");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialogBase *>(this), "GetContentWindow");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialogBase *>(this), "GetModality");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialogBase *>(this), "IsEscapeKey");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialogBase *>(this), "GetEventTable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialogBase *>(this), "GetEventHashTable");
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

// wxFindReplaceDialog file: line:130
struct PyCallBack_wxFindReplaceDialog : public wxFindReplaceDialog {
	using wxFindReplaceDialog::wxFindReplaceDialog;

	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialog *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxFindReplaceDialog::GetClassInfo();
	}
	const struct wxEventTable * GetEventTable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialog *>(this), "GetEventTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const struct wxEventTable *>::value) {
				static pybind11::detail::override_caster_t<const struct wxEventTable *> caster;
				return pybind11::detail::cast_ref<const struct wxEventTable *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const struct wxEventTable *>(std::move(o));
		}
		return wxFindReplaceDialog::GetEventTable();
	}
	class wxEventHashTable & GetEventHashTable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialog *>(this), "GetEventHashTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEventHashTable &>::value) {
				static pybind11::detail::override_caster_t<class wxEventHashTable &> caster;
				return pybind11::detail::cast_ref<class wxEventHashTable &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEventHashTable &>(std::move(o));
		}
		return wxFindReplaceDialog::GetEventHashTable();
	}
	bool Show(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialog *>(this), "Show");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialog *>(this), "ShowModal");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialog *>(this), "EndModal");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialog *>(this), "IsModal");
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
	void ShowWindowModal() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialog *>(this), "ShowWindowModal");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialog *>(this), "SendWindowModalDialogEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialog *>(this), "DoLayoutAdaptation");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialog *>(this), "CanDoLayoutAdaptation");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialog *>(this), "GetContentWindow");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialog *>(this), "GetModality");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindReplaceDialog *>(this), "IsEscapeKey");
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
};

// wxFindDialogEvent file:wx/fdrepdlg.h line:139
struct PyCallBack_wxFindDialogEvent : public wxFindDialogEvent {
	using wxFindDialogEvent::wxFindDialogEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindDialogEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxFindDialogEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindDialogEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxFindDialogEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindDialogEvent *>(this), "GetEventCategory");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindDialogEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFindDialogEvent *>(this), "CloneRefData");
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

void bind_unknown_unknown_79(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxWindowModalDialogEvent file: line:372
		pybind11::class_<wxWindowModalDialogEvent, std::shared_ptr<wxWindowModalDialogEvent>, PyCallBack_wxWindowModalDialogEvent, wxCommandEvent> cl(M(""), "wxWindowModalDialogEvent", "");
		cl.def( pybind11::init( [](){ return new wxWindowModalDialogEvent(); }, [](){ return new PyCallBack_wxWindowModalDialogEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxWindowModalDialogEvent(a0); }, [](int const & a0){ return new PyCallBack_wxWindowModalDialogEvent(a0); } ), "doc");
		cl.def( pybind11::init<int, int>(), pybind11::arg("commandType"), pybind11::arg("id") );

		cl.def( pybind11::init( [](PyCallBack_wxWindowModalDialogEvent const &o){ return new PyCallBack_wxWindowModalDialogEvent(o); } ) );
		cl.def( pybind11::init( [](wxWindowModalDialogEvent const &o){ return new wxWindowModalDialogEvent(o); } ) );
		cl.def("GetDialog", (class wxDialog * (wxWindowModalDialogEvent::*)() const) &wxWindowModalDialogEvent::GetDialog, "C++: wxWindowModalDialogEvent::GetDialog() const --> class wxDialog *", pybind11::return_value_policy::automatic);
		cl.def("GetReturnCode", (int (wxWindowModalDialogEvent::*)() const) &wxWindowModalDialogEvent::GetReturnCode, "C++: wxWindowModalDialogEvent::GetReturnCode() const --> int");
		cl.def("Clone", (class wxEvent * (wxWindowModalDialogEvent::*)() const) &wxWindowModalDialogEvent::Clone, "C++: wxWindowModalDialogEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxWindowModalDialogEvent::*)() const) &wxWindowModalDialogEvent::GetClassInfo, "C++: wxWindowModalDialogEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxWindowModalDialogEvent::wxCreateObject, "C++: wxWindowModalDialogEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	// wxFindReplaceFlags file:wx/fdrepdlg.h line:30
	pybind11::enum_<wxFindReplaceFlags>(M(""), "wxFindReplaceFlags", pybind11::arithmetic(), "")
		.value("wxFR_DOWN", wxFR_DOWN)
		.value("wxFR_WHOLEWORD", wxFR_WHOLEWORD)
		.value("wxFR_MATCHCASE", wxFR_MATCHCASE)
		.export_values();

;

	// wxFindReplaceDialogStyles file:wx/fdrepdlg.h line:43
	pybind11::enum_<wxFindReplaceDialogStyles>(M(""), "wxFindReplaceDialogStyles", pybind11::arithmetic(), "")
		.value("wxFR_REPLACEDIALOG", wxFR_REPLACEDIALOG)
		.value("wxFR_NOUPDOWN", wxFR_NOUPDOWN)
		.value("wxFR_NOMATCHCASE", wxFR_NOMATCHCASE)
		.value("wxFR_NOWHOLEWORD", wxFR_NOWHOLEWORD)
		.export_values();

;

	{ // wxFindReplaceData file:wx/fdrepdlg.h line:62
		pybind11::class_<wxFindReplaceData, std::shared_ptr<wxFindReplaceData>, PyCallBack_wxFindReplaceData, wxObject> cl(M(""), "wxFindReplaceData", "");
		cl.def( pybind11::init( [](){ return new wxFindReplaceData(); }, [](){ return new PyCallBack_wxFindReplaceData(); } ) );
		cl.def( pybind11::init<unsigned int>(), pybind11::arg("flags") );

		cl.def( pybind11::init( [](PyCallBack_wxFindReplaceData const &o){ return new PyCallBack_wxFindReplaceData(o); } ) );
		cl.def( pybind11::init( [](wxFindReplaceData const &o){ return new wxFindReplaceData(o); } ) );
		cl.def("GetFindString", (const class wxString & (wxFindReplaceData::*)() const) &wxFindReplaceData::GetFindString, "C++: wxFindReplaceData::GetFindString() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetReplaceString", (const class wxString & (wxFindReplaceData::*)() const) &wxFindReplaceData::GetReplaceString, "C++: wxFindReplaceData::GetReplaceString() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetFlags", (int (wxFindReplaceData::*)() const) &wxFindReplaceData::GetFlags, "C++: wxFindReplaceData::GetFlags() const --> int");
		cl.def("SetFlags", (void (wxFindReplaceData::*)(unsigned int)) &wxFindReplaceData::SetFlags, "C++: wxFindReplaceData::SetFlags(unsigned int) --> void", pybind11::arg("flags"));
		cl.def("SetFindString", (void (wxFindReplaceData::*)(const class wxString &)) &wxFindReplaceData::SetFindString, "C++: wxFindReplaceData::SetFindString(const class wxString &) --> void", pybind11::arg("str"));
		cl.def("SetReplaceString", (void (wxFindReplaceData::*)(const class wxString &)) &wxFindReplaceData::SetReplaceString, "C++: wxFindReplaceData::SetReplaceString(const class wxString &) --> void", pybind11::arg("str"));
		cl.def("assign", (class wxFindReplaceData & (wxFindReplaceData::*)(const class wxFindReplaceData &)) &wxFindReplaceData::operator=, "C++: wxFindReplaceData::operator=(const class wxFindReplaceData &) --> class wxFindReplaceData &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxFindReplaceDialogBase file:wx/fdrepdlg.h line:95
		pybind11::class_<wxFindReplaceDialogBase, std::shared_ptr<wxFindReplaceDialogBase>, PyCallBack_wxFindReplaceDialogBase, wxDialog> cl(M(""), "wxFindReplaceDialogBase", "");
		cl.def( pybind11::init( [](){ return new wxFindReplaceDialogBase(); }, [](){ return new PyCallBack_wxFindReplaceDialogBase(); } ) );
		cl.def( pybind11::init( [](class wxWindow * a0, class wxFindReplaceData * a1, const class wxString & a2){ return new wxFindReplaceDialogBase(a0, a1, a2); }, [](class wxWindow * a0, class wxFindReplaceData * a1, const class wxString & a2){ return new PyCallBack_wxFindReplaceDialogBase(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init<class wxWindow *, class wxFindReplaceData *, const class wxString &, int>(), pybind11::arg(""), pybind11::arg("data"), pybind11::arg(""), pybind11::arg("") );

		cl.def("GetData", (const class wxFindReplaceData * (wxFindReplaceDialogBase::*)() const) &wxFindReplaceDialogBase::GetData, "C++: wxFindReplaceDialogBase::GetData() const --> const class wxFindReplaceData *", pybind11::return_value_policy::automatic);
		cl.def("SetData", (void (wxFindReplaceDialogBase::*)(class wxFindReplaceData *)) &wxFindReplaceDialogBase::SetData, "C++: wxFindReplaceDialogBase::SetData(class wxFindReplaceData *) --> void", pybind11::arg("data"));
		cl.def("Send", (void (wxFindReplaceDialogBase::*)(class wxFindDialogEvent &)) &wxFindReplaceDialogBase::Send, "C++: wxFindReplaceDialogBase::Send(class wxFindDialogEvent &) --> void", pybind11::arg("event"));
	}
	{ // wxFindReplaceDialog file: line:130
		pybind11::class_<wxFindReplaceDialog, std::shared_ptr<wxFindReplaceDialog>, PyCallBack_wxFindReplaceDialog, wxFindReplaceDialogBase> cl(M(""), "wxFindReplaceDialog", "");
		cl.def( pybind11::init( [](){ return new wxFindReplaceDialog(); }, [](){ return new PyCallBack_wxFindReplaceDialog(); } ) );
		cl.def( pybind11::init( [](class wxWindow * a0, class wxFindReplaceData * a1, const class wxString & a2){ return new wxFindReplaceDialog(a0, a1, a2); }, [](class wxWindow * a0, class wxFindReplaceData * a1, const class wxString & a2){ return new PyCallBack_wxFindReplaceDialog(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init<class wxWindow *, class wxFindReplaceData *, const class wxString &, int>(), pybind11::arg("parent"), pybind11::arg("data"), pybind11::arg("title"), pybind11::arg("style") );

		cl.def( pybind11::init( [](PyCallBack_wxFindReplaceDialog const &o){ return new PyCallBack_wxFindReplaceDialog(o); } ) );
		cl.def( pybind11::init( [](wxFindReplaceDialog const &o){ return new wxFindReplaceDialog(o); } ) );
		cl.def("Create", [](wxFindReplaceDialog &o, class wxWindow * a0, class wxFindReplaceData * a1, const class wxString & a2) -> bool { return o.Create(a0, a1, a2); }, "", pybind11::arg("parent"), pybind11::arg("data"), pybind11::arg("title"));
		cl.def("Create", (bool (wxFindReplaceDialog::*)(class wxWindow *, class wxFindReplaceData *, const class wxString &, int)) &wxFindReplaceDialog::Create, "C++: wxFindReplaceDialog::Create(class wxWindow *, class wxFindReplaceData *, const class wxString &, int) --> bool", pybind11::arg("parent"), pybind11::arg("data"), pybind11::arg("title"), pybind11::arg("style"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxFindReplaceDialog::*)() const) &wxFindReplaceDialog::GetClassInfo, "C++: wxFindReplaceDialog::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxFindReplaceDialog::wxCreateObject, "C++: wxFindReplaceDialog::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxFindReplaceDialog & (wxFindReplaceDialog::*)(const class wxFindReplaceDialog &)) &wxFindReplaceDialog::operator=, "C++: wxFindReplaceDialog::operator=(const class wxFindReplaceDialog &) --> class wxFindReplaceDialog &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxFindDialogEvent file:wx/fdrepdlg.h line:139
		pybind11::class_<wxFindDialogEvent, std::shared_ptr<wxFindDialogEvent>, PyCallBack_wxFindDialogEvent, wxCommandEvent> cl(M(""), "wxFindDialogEvent", "");
		cl.def( pybind11::init( [](){ return new wxFindDialogEvent(); }, [](){ return new PyCallBack_wxFindDialogEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxFindDialogEvent(a0); }, [](int const & a0){ return new PyCallBack_wxFindDialogEvent(a0); } ), "doc");
		cl.def( pybind11::init<int, int>(), pybind11::arg("commandType"), pybind11::arg("id") );

		cl.def( pybind11::init( [](PyCallBack_wxFindDialogEvent const &o){ return new PyCallBack_wxFindDialogEvent(o); } ) );
		cl.def( pybind11::init( [](wxFindDialogEvent const &o){ return new wxFindDialogEvent(o); } ) );
		cl.def("GetFlags", (int (wxFindDialogEvent::*)() const) &wxFindDialogEvent::GetFlags, "C++: wxFindDialogEvent::GetFlags() const --> int");
		cl.def("GetFindString", (class wxString (wxFindDialogEvent::*)() const) &wxFindDialogEvent::GetFindString, "C++: wxFindDialogEvent::GetFindString() const --> class wxString");
		cl.def("GetReplaceString", (const class wxString & (wxFindDialogEvent::*)() const) &wxFindDialogEvent::GetReplaceString, "C++: wxFindDialogEvent::GetReplaceString() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetDialog", (class wxFindReplaceDialog * (wxFindDialogEvent::*)() const) &wxFindDialogEvent::GetDialog, "C++: wxFindDialogEvent::GetDialog() const --> class wxFindReplaceDialog *", pybind11::return_value_policy::automatic);
		cl.def("SetFlags", (void (wxFindDialogEvent::*)(int)) &wxFindDialogEvent::SetFlags, "C++: wxFindDialogEvent::SetFlags(int) --> void", pybind11::arg("flags"));
		cl.def("SetFindString", (void (wxFindDialogEvent::*)(const class wxString &)) &wxFindDialogEvent::SetFindString, "C++: wxFindDialogEvent::SetFindString(const class wxString &) --> void", pybind11::arg("str"));
		cl.def("SetReplaceString", (void (wxFindDialogEvent::*)(const class wxString &)) &wxFindDialogEvent::SetReplaceString, "C++: wxFindDialogEvent::SetReplaceString(const class wxString &) --> void", pybind11::arg("str"));
		cl.def("Clone", (class wxEvent * (wxFindDialogEvent::*)() const) &wxFindDialogEvent::Clone, "C++: wxFindDialogEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxFindDialogEvent::*)() const) &wxFindDialogEvent::GetClassInfo, "C++: wxFindDialogEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxFindDialogEvent::wxCreateObject, "C++: wxFindDialogEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	// KiBitmap(int) file:bitmap_types.h line:44
	M("").def("KiBitmap", (class wxBitmap (*)(int)) &KiBitmap, "Construct a wxBitmap from a memory record, held in a BITMAP_DEF.\n\nC++: KiBitmap(int) --> class wxBitmap", pybind11::arg("aBitmap"));

	// KiScaledBitmap(int, class wxWindow *) file:bitmap_types.h line:56
	M("").def("KiScaledBitmap", (class wxBitmap (*)(int, class wxWindow *)) &KiScaledBitmap, "Construct a wxBitmap from a memory record, scaling it if device DPI demands it.\n\n Internally this may use caching to avoid scaling the same image twice. If caching\n is used, it's guaranteed threadsafe.\n\n \n bitmap definition\n \n\n target window for scaling context\n\nC++: KiScaledBitmap(int, class wxWindow *) --> class wxBitmap", pybind11::arg("aBitmap"), pybind11::arg("aWindow"));

	// KiScaledBitmap(const class wxBitmap &, class wxWindow *) file:bitmap_types.h line:64
	M("").def("KiScaledBitmap", (class wxBitmap (*)(const class wxBitmap &, class wxWindow *)) &KiScaledBitmap, "Overload of the above function that takes another wxBitmap as a parameter.\n\n \n bitmap definition\n \n\n target window for scaling context\n\nC++: KiScaledBitmap(const class wxBitmap &, class wxWindow *) --> class wxBitmap", pybind11::arg("aBitmap"), pybind11::arg("aWindow"));

	// KiIconScale(class wxWindow *) file:bitmap_types.h line:70
	M("").def("KiIconScale", (int (*)(class wxWindow *)) &KiIconScale, "Return the automatic scale factor that would be used for a given window by\n KiScaledBitmap and KiScaledSeparator.\n\nC++: KiIconScale(class wxWindow *) --> int", pybind11::arg("aWindow"));

	// KiBitmapNew(int) file:bitmap_types.h line:77
	M("").def("KiBitmapNew", (class wxBitmap * (*)(int)) &KiBitmapNew, "Allocate a wxBitmap on heap from a memory record, held in a BITMAP_DEF.\n\n \n wxBitmap* - caller owns it.\n\nC++: KiBitmapNew(int) --> class wxBitmap *", pybind11::return_value_policy::automatic, pybind11::arg("aBitmap"));

}

#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <time.h> // tm
#include <wx/dc.h> // wxDC
#include <wx/dc.h> // wxDCImpl
#include <wx/dc.h> // wxDrawObject
#include <wx/dc.h> // wxFloodFillStyle
#include <wx/dc.h> // wxFontMetrics
#include <wx/dc.h> // wxMappingMode
#include <wx/dc.h> // wxRasterOperationMode
#include <wx/pen.h> // wxPenCap
#include <wx/pen.h> // wxPenJoin
#include <wx/pen.h> // wxPenStyle
#include <wx/propgrid/property.h> // wxPGAttributeStorage
#include <wx/propgrid/property.h> // wxPGCell
#include <wx/propgrid/property.h> // wxPGCellData
#include <wx/propgrid/property.h> // wxPGCellRenderer
#include <wx/propgrid/property.h> // wxPGChoices
#include <wx/propgrid/property.h> // wxPGPaintData
#include <wx/propgrid/property.h> // wxPGProperty
#include <wx/propgrid/property.h> // wxPGPropertyFlags
#include <wx/propgrid/property.h> // wxPGRootProperty
#include <wx/propgrid/property.h> // wxPropertyCategory

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxPGRootProperty file:wx/propgrid/property.h line:2524
struct PyCallBack_wxPGRootProperty : public wxPGRootProperty {
	using wxPGRootProperty::wxPGRootProperty;

	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxPGRootProperty::GetClassInfo();
	}
	bool StringToValue(class wxVariant & a0, const class wxString & a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "StringToValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxPGRootProperty::StringToValue(a0, a1, a2);
	}
	void OnSetValue() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "OnSetValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPGProperty::OnSetValue();
	}
	class wxVariant DoGetValue() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "DoGetValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxVariant>::value) {
				static pybind11::detail::override_caster_t<class wxVariant> caster;
				return pybind11::detail::cast_ref<class wxVariant>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxVariant>(std::move(o));
		}
		return wxPGProperty::DoGetValue();
	}
	bool IntToValue(class wxVariant & a0, int a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "IntToValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxPGProperty::IntToValue(a0, a1, a2);
	}
	class wxString ValueToString(class wxVariant & a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "ValueToString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return wxPGProperty::ValueToString(a0, a1);
	}
	class wxSize OnMeasureImage(int a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "OnMeasureImage");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxPGProperty::OnMeasureImage(a0);
	}
	class wxVariant ChildChanged(class wxVariant & a0, int a1, class wxVariant & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "ChildChanged");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxVariant>::value) {
				static pybind11::detail::override_caster_t<class wxVariant> caster;
				return pybind11::detail::cast_ref<class wxVariant>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxVariant>(std::move(o));
		}
		return wxPGProperty::ChildChanged(a0, a1, a2);
	}
	class wxValidator * DoGetValidator() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "DoGetValidator");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxValidator *>::value) {
				static pybind11::detail::override_caster_t<class wxValidator *> caster;
				return pybind11::detail::cast_ref<class wxValidator *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxValidator *>(std::move(o));
		}
		return wxPGProperty::DoGetValidator();
	}
	void OnCustomPaint(class wxDC & a0, const class wxRect & a1, struct wxPGPaintData & a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "OnCustomPaint");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPGProperty::OnCustomPaint(a0, a1, a2);
	}
	class wxPGCellRenderer * GetCellRenderer(int a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "GetCellRenderer");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPGCellRenderer *>::value) {
				static pybind11::detail::override_caster_t<class wxPGCellRenderer *> caster;
				return pybind11::detail::cast_ref<class wxPGCellRenderer *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPGCellRenderer *>(std::move(o));
		}
		return wxPGProperty::GetCellRenderer(a0);
	}
	int GetChoiceSelection() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "GetChoiceSelection");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxPGProperty::GetChoiceSelection();
	}
	void RefreshChildren() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "RefreshChildren");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPGProperty::RefreshChildren();
	}
	bool DoSetAttribute(const class wxString & a0, class wxVariant & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "DoSetAttribute");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxPGProperty::DoSetAttribute(a0, a1);
	}
	class wxVariant DoGetAttribute(const class wxString & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "DoGetAttribute");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxVariant>::value) {
				static pybind11::detail::override_caster_t<class wxVariant> caster;
				return pybind11::detail::cast_ref<class wxVariant>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxVariant>(std::move(o));
		}
		return wxPGProperty::DoGetAttribute(a0);
	}
	void OnValidationFailure(class wxVariant & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "OnValidationFailure");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPGProperty::OnValidationFailure(a0);
	}
	class wxString GetValueAsString(int a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "GetValueAsString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return wxPGProperty::GetValueAsString(a0);
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGRootProperty *>(this), "CloneRefData");
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

// wxPropertyCategory file:wx/propgrid/property.h line:2548
struct PyCallBack_wxPropertyCategory : public wxPropertyCategory {
	using wxPropertyCategory::wxPropertyCategory;

	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxPropertyCategory::GetClassInfo();
	}
	class wxString ValueToString(class wxVariant & a0, int a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "ValueToString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return wxPropertyCategory::ValueToString(a0, a1);
	}
	class wxString GetValueAsString(int a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "GetValueAsString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxString>::value) {
				static pybind11::detail::override_caster_t<class wxString> caster;
				return pybind11::detail::cast_ref<class wxString>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxString>(std::move(o));
		}
		return wxPropertyCategory::GetValueAsString(a0);
	}
	void OnSetValue() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "OnSetValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPGProperty::OnSetValue();
	}
	class wxVariant DoGetValue() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "DoGetValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxVariant>::value) {
				static pybind11::detail::override_caster_t<class wxVariant> caster;
				return pybind11::detail::cast_ref<class wxVariant>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxVariant>(std::move(o));
		}
		return wxPGProperty::DoGetValue();
	}
	bool StringToValue(class wxVariant & a0, const class wxString & a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "StringToValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxPGProperty::StringToValue(a0, a1, a2);
	}
	bool IntToValue(class wxVariant & a0, int a1, int a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "IntToValue");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxPGProperty::IntToValue(a0, a1, a2);
	}
	class wxSize OnMeasureImage(int a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "OnMeasureImage");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxSize>::value) {
				static pybind11::detail::override_caster_t<class wxSize> caster;
				return pybind11::detail::cast_ref<class wxSize>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxSize>(std::move(o));
		}
		return wxPGProperty::OnMeasureImage(a0);
	}
	class wxVariant ChildChanged(class wxVariant & a0, int a1, class wxVariant & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "ChildChanged");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxVariant>::value) {
				static pybind11::detail::override_caster_t<class wxVariant> caster;
				return pybind11::detail::cast_ref<class wxVariant>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxVariant>(std::move(o));
		}
		return wxPGProperty::ChildChanged(a0, a1, a2);
	}
	class wxValidator * DoGetValidator() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "DoGetValidator");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxValidator *>::value) {
				static pybind11::detail::override_caster_t<class wxValidator *> caster;
				return pybind11::detail::cast_ref<class wxValidator *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxValidator *>(std::move(o));
		}
		return wxPGProperty::DoGetValidator();
	}
	void OnCustomPaint(class wxDC & a0, const class wxRect & a1, struct wxPGPaintData & a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "OnCustomPaint");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPGProperty::OnCustomPaint(a0, a1, a2);
	}
	class wxPGCellRenderer * GetCellRenderer(int a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "GetCellRenderer");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPGCellRenderer *>::value) {
				static pybind11::detail::override_caster_t<class wxPGCellRenderer *> caster;
				return pybind11::detail::cast_ref<class wxPGCellRenderer *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPGCellRenderer *>(std::move(o));
		}
		return wxPGProperty::GetCellRenderer(a0);
	}
	int GetChoiceSelection() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "GetChoiceSelection");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxPGProperty::GetChoiceSelection();
	}
	void RefreshChildren() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "RefreshChildren");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPGProperty::RefreshChildren();
	}
	bool DoSetAttribute(const class wxString & a0, class wxVariant & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "DoSetAttribute");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxPGProperty::DoSetAttribute(a0, a1);
	}
	class wxVariant DoGetAttribute(const class wxString & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "DoGetAttribute");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxVariant>::value) {
				static pybind11::detail::override_caster_t<class wxVariant> caster;
				return pybind11::detail::cast_ref<class wxVariant>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxVariant>(std::move(o));
		}
		return wxPGProperty::DoGetAttribute(a0);
	}
	void OnValidationFailure(class wxVariant & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "OnValidationFailure");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxPGProperty::OnValidationFailure(a0);
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPropertyCategory *>(this), "CloneRefData");
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

void bind_wx_propgrid_property_2(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxPGRootProperty file:wx/propgrid/property.h line:2524
		pybind11::class_<wxPGRootProperty, std::shared_ptr<wxPGRootProperty>, PyCallBack_wxPGRootProperty, wxPGProperty> cl(M(""), "wxPGRootProperty", "Root parent property.");
		cl.def( pybind11::init( [](){ return new wxPGRootProperty(); }, [](){ return new PyCallBack_wxPGRootProperty(); } ), "doc");
		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("name") );

		cl.def( pybind11::init( [](PyCallBack_wxPGRootProperty const &o){ return new PyCallBack_wxPGRootProperty(o); } ) );
		cl.def( pybind11::init( [](wxPGRootProperty const &o){ return new wxPGRootProperty(o); } ) );
		cl.def("GetClassInfo", (class wxClassInfo * (wxPGRootProperty::*)() const) &wxPGRootProperty::GetClassInfo, "C++: wxPGRootProperty::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxPGRootProperty::wxCreateObject, "C++: wxPGRootProperty::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("StringToValue", (bool (wxPGRootProperty::*)(class wxVariant &, const class wxString &, int) const) &wxPGRootProperty::StringToValue, "C++: wxPGRootProperty::StringToValue(class wxVariant &, const class wxString &, int) const --> bool", pybind11::arg(""), pybind11::arg(""), pybind11::arg(""));
		cl.def("assign", (class wxPGRootProperty & (wxPGRootProperty::*)(const class wxPGRootProperty &)) &wxPGRootProperty::operator=, "C++: wxPGRootProperty::operator=(const class wxPGRootProperty &) --> class wxPGRootProperty &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxPropertyCategory file:wx/propgrid/property.h line:2548
		pybind11::class_<wxPropertyCategory, std::shared_ptr<wxPropertyCategory>, PyCallBack_wxPropertyCategory, wxPGProperty> cl(M(""), "wxPropertyCategory", "Category (caption) property.");
		cl.def( pybind11::init( [](){ return new wxPropertyCategory(); }, [](){ return new PyCallBack_wxPropertyCategory(); } ) );
		cl.def( pybind11::init( [](const class wxString & a0){ return new wxPropertyCategory(a0); }, [](const class wxString & a0){ return new PyCallBack_wxPropertyCategory(a0); } ), "doc");
		cl.def( pybind11::init<const class wxString &, const class wxString &>(), pybind11::arg("label"), pybind11::arg("name") );

		cl.def( pybind11::init( [](PyCallBack_wxPropertyCategory const &o){ return new PyCallBack_wxPropertyCategory(o); } ) );
		cl.def( pybind11::init( [](wxPropertyCategory const &o){ return new wxPropertyCategory(o); } ) );
		cl.def("GetClassInfo", (class wxClassInfo * (wxPropertyCategory::*)() const) &wxPropertyCategory::GetClassInfo, "C++: wxPropertyCategory::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxPropertyCategory::wxCreateObject, "C++: wxPropertyCategory::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("GetTextExtent", (int (wxPropertyCategory::*)(const class wxWindow *, const class wxFont &) const) &wxPropertyCategory::GetTextExtent, "C++: wxPropertyCategory::GetTextExtent(const class wxWindow *, const class wxFont &) const --> int", pybind11::arg("wnd"), pybind11::arg("font"));
		cl.def("ValueToString", (class wxString (wxPropertyCategory::*)(class wxVariant &, int) const) &wxPropertyCategory::ValueToString, "C++: wxPropertyCategory::ValueToString(class wxVariant &, int) const --> class wxString", pybind11::arg("value"), pybind11::arg("argFlags"));
		cl.def("GetValueAsString", [](wxPropertyCategory const &o) -> wxString { return o.GetValueAsString(); }, "");
		cl.def("GetValueAsString", (class wxString (wxPropertyCategory::*)(int) const) &wxPropertyCategory::GetValueAsString, "C++: wxPropertyCategory::GetValueAsString(int) const --> class wxString", pybind11::arg("argFlags"));
		cl.def("assign", (class wxPropertyCategory & (wxPropertyCategory::*)(const class wxPropertyCategory &)) &wxPropertyCategory::operator=, "C++: wxPropertyCategory::operator=(const class wxPropertyCategory &) --> class wxPropertyCategory &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}

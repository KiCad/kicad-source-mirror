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
#include <wx/propgrid/property.h> // wxPGChoiceEntry
#include <wx/propgrid/property.h> // wxPGChoices
#include <wx/propgrid/property.h> // wxPGChoicesData
#include <wx/propgrid/property.h> // wxPGPaintData
#include <wx/propgrid/property.h> // wxPGProperty
#include <wx/propgrid/property.h> // wxPGPropertyFlags

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxPGProperty file:wx/propgrid/property.h line:1116
struct PyCallBack_wxPGProperty : public wxPGProperty {
	using wxPGProperty::wxPGProperty;

	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxPGProperty::GetClassInfo();
	}
	void OnSetValue() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "OnSetValue");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "DoGetValue");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "StringToValue");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "IntToValue");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "ValueToString");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "OnMeasureImage");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "ChildChanged");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "DoGetValidator");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "OnCustomPaint");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "GetCellRenderer");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "GetChoiceSelection");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "RefreshChildren");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "DoSetAttribute");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "DoGetAttribute");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "OnValidationFailure");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "GetValueAsString");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPGProperty *>(this), "CloneRefData");
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

void bind_wx_propgrid_property_1(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxPGProperty file:wx/propgrid/property.h line:1116
		pybind11::class_<wxPGProperty, std::shared_ptr<wxPGProperty>, PyCallBack_wxPGProperty, wxObject> cl(M(""), "wxPGProperty", "wxPGProperty is base class for all wxPropertyGrid properties.\n\n    NB: Full class overview is now only present in\n        interface/wx/propgrid/property.h.\n\n    {wxpropgrid}\n    \n");
		cl.def( pybind11::init( [](){ return new wxPGProperty(); }, [](){ return new PyCallBack_wxPGProperty(); } ) );
		cl.def( pybind11::init<const class wxString &, const class wxString &>(), pybind11::arg("label"), pybind11::arg("name") );

		cl.def( pybind11::init( [](PyCallBack_wxPGProperty const &o){ return new PyCallBack_wxPGProperty(o); } ) );
		cl.def( pybind11::init( [](wxPGProperty const &o){ return new wxPGProperty(o); } ) );
		cl.def("GetClassInfo", (class wxClassInfo * (wxPGProperty::*)() const) &wxPGProperty::GetClassInfo, "C++: wxPGProperty::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def("OnSetValue", (void (wxPGProperty::*)()) &wxPGProperty::OnSetValue, "This virtual function is called after m_value has been set.\n\n        \n\n        - If m_value was set to Null variant (ie. unspecified value),\n          OnSetValue() will not be called.\n        - m_value may be of any variant type. Typically properties internally\n          support only one variant type, and as such OnSetValue() provides a\n          good opportunity to convert\n          supported values into internal type.\n        - Default implementation does nothing.\n\nC++: wxPGProperty::OnSetValue() --> void");
		cl.def("DoGetValue", (class wxVariant (wxPGProperty::*)() const) &wxPGProperty::DoGetValue, "Override this to return something else than m_value as the value.\n\nC++: wxPGProperty::DoGetValue() const --> class wxVariant");
		cl.def("StringToValue", [](wxPGProperty const &o, class wxVariant & a0, const class wxString & a1) -> bool { return o.StringToValue(a0, a1); }, "", pybind11::arg("variant"), pybind11::arg("text"));
		cl.def("StringToValue", (bool (wxPGProperty::*)(class wxVariant &, const class wxString &, int) const) &wxPGProperty::StringToValue, "Converts text into wxVariant value appropriate for this property.\n\n        \n\n            On function entry this is the old value (should not be wxNullVariant\n            in normal cases). Translated value must be assigned back to it.\n\n        \n\n            Text to be translated into variant.\n\n        \n\n            If wxPG_FULL_VALUE is set, returns complete, storable value instead\n            of displayable one (they may be different).\n            If wxPG_COMPOSITE_FRAGMENT is set, text is interpreted as a part of\n            composite property string value (as generated by ValueToString()\n            called with this same flag).\n\n        \n Returns  if resulting wxVariant value was different.\n\n        \n Default implementation converts semicolon delimited tokens into\n                child values. Only works for properties with children.\n\n                You might want to take into account that m_value is Null variant\n                if property value is unspecified (which is usually only case if\n                you explicitly enabled that sort behaviour).\n\nC++: wxPGProperty::StringToValue(class wxVariant &, const class wxString &, int) const --> bool", pybind11::arg("variant"), pybind11::arg("text"), pybind11::arg("argFlags"));
		cl.def("IntToValue", [](wxPGProperty const &o, class wxVariant & a0, int const & a1) -> bool { return o.IntToValue(a0, a1); }, "", pybind11::arg("value"), pybind11::arg("number"));
		cl.def("IntToValue", (bool (wxPGProperty::*)(class wxVariant &, int, int) const) &wxPGProperty::IntToValue, "Converts integer (possibly a choice selection) into wxVariant value\n        appropriate for this property.\n\n        \n\n            On function entry this is the old value (should not be wxNullVariant\n            in normal cases). Translated value must be assigned back to it.\n\n        \n\n            Integer to be translated into variant.\n\n        \n\n            If wxPG_FULL_VALUE is set, returns complete, storable value instead\n            of displayable one.\n\n        \n Returns  if resulting wxVariant value was different.\n\n        \n\n        - If property is not supposed to use choice or spinctrl or other editor\n          with int-based value, it is not necessary to implement this method.\n        - Default implementation simply assign given int to m_value.\n        - If property uses choice control, and displays a dialog on some choice\n          items, then it is preferred to display that dialog in IntToValue\n          instead of OnEvent.\n        - You might want to take into account that m_value is Null variant if\n          property value is unspecified (which is usually only case if you\n          explicitly enabled that sort behaviour).\n\nC++: wxPGProperty::IntToValue(class wxVariant &, int, int) const --> bool", pybind11::arg("value"), pybind11::arg("number"), pybind11::arg("argFlags"));
		cl.def("ValueToString", [](wxPGProperty const &o, class wxVariant & a0) -> wxString { return o.ValueToString(a0); }, "", pybind11::arg("value"));
		cl.def("ValueToString", (class wxString (wxPGProperty::*)(class wxVariant &, int) const) &wxPGProperty::ValueToString, "Converts property value into a text representation.\n\n        \n\n            Value to be converted.\n\n        \n\n            If 0 (default value), then displayed string is returned.\n            If wxPG_FULL_VALUE is set, returns complete, storable string value\n            instead of displayable. If wxPG_EDITABLE_VALUE is set, returns\n            string value that must be editable in textctrl. If\n            wxPG_COMPOSITE_FRAGMENT is set, returns text that is appropriate to\n            display as a part of string property's composite text\n            representation.\n\n        \n Default implementation calls GenerateComposedValue().\n\nC++: wxPGProperty::ValueToString(class wxVariant &, int) const --> class wxString", pybind11::arg("value"), pybind11::arg("argFlags"));
		cl.def("SetValueFromString", [](wxPGProperty &o, const class wxString & a0) -> bool { return o.SetValueFromString(a0); }, "", pybind11::arg("text"));
		cl.def("SetValueFromString", (bool (wxPGProperty::*)(const class wxString &, int)) &wxPGProperty::SetValueFromString, "Converts string to a value, and if successful, calls SetValue() on it.\n        Default behaviour is to do nothing.\n        \n\n\n        String to get the value from.\n        \n\n\n        true if value was changed.\n\nC++: wxPGProperty::SetValueFromString(const class wxString &, int) --> bool", pybind11::arg("text"), pybind11::arg("flags"));
		cl.def("SetValueFromInt", [](wxPGProperty &o, long const & a0) -> bool { return o.SetValueFromInt(a0); }, "", pybind11::arg("value"));
		cl.def("SetValueFromInt", (bool (wxPGProperty::*)(long, int)) &wxPGProperty::SetValueFromInt, "Converts integer to a value, and if successful, calls SetValue() on it.\n        Default behaviour is to do nothing.\n        \n\n\n            Int to get the value from.\n        \n\n\n            If has wxPG_FULL_VALUE, then the value given is a actual value and\n            not an index.\n        \n\n\n            True if value was changed.\n\nC++: wxPGProperty::SetValueFromInt(long, int) --> bool", pybind11::arg("value"), pybind11::arg("flags"));
		cl.def("OnMeasureImage", [](wxPGProperty const &o) -> wxSize { return o.OnMeasureImage(); }, "");
		cl.def("OnMeasureImage", (class wxSize (wxPGProperty::*)(int) const) &wxPGProperty::OnMeasureImage, "Returns size of the custom painted image in front of property.\n\n        This method must be overridden to return non-default value if\n        OnCustomPaint is to be called.\n        \n\n\n            Normally -1, but can be an index to the property's list of items.\n        \n\n\n        - Default behaviour is to return wxSize(0,0), which means no image.\n        - Default image width or height is indicated with dimension -1.\n        - You can also return wxPG_DEFAULT_IMAGE_SIZE, i.e. wxSize(-1, -1).\n\nC++: wxPGProperty::OnMeasureImage(int) const --> class wxSize", pybind11::arg("item"));
		cl.def("ChildChanged", (class wxVariant (wxPGProperty::*)(class wxVariant &, int, class wxVariant &) const) &wxPGProperty::ChildChanged, "Called after value of a child property has been altered. Must return\n        new value of the whole property (after any alterations warranted by\n        child's new value).\n\n        Note that this function is usually called at the time that value of\n        this property, or given child property, is still pending for change,\n        and as such, result of GetValue() or m_value should not be relied\n        on.\n\n        Sample pseudo-code implementation:\n\n        \n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n        \n\n            Value of this property. Changed value should be returned (in\n            previous versions of wxPropertyGrid it was only necessary to\n            write value back to this argument).\n        \n\n\n            Index of child changed (you can use Item(childIndex) to get\n            child property).\n        \n\n\n            (Pending) value of the child property.\n\n        \n\n            Modified value of the whole property.\n\nC++: wxPGProperty::ChildChanged(class wxVariant &, int, class wxVariant &) const --> class wxVariant", pybind11::arg("thisValue"), pybind11::arg("childIndex"), pybind11::arg("childValue"));
		cl.def("DoGetValidator", (class wxValidator * (wxPGProperty::*)() const) &wxPGProperty::DoGetValidator, "Returns pointer to the wxValidator that should be used\n        with the editor of this property (NULL for no validator).\n        Setting validator explicitly via SetPropertyValidator\n        will override this.\n\n        In most situations, code like this should work well\n        (macros are used to maintain one actual validator instance,\n        so on the second call the function exits within the first\n        macro):\n\n        \n\n\n\n\n\n\n\n\n\n\n\n\n\n        \n\n        You can get common filename validator by returning\n        wxFileProperty::GetClassValidator(). wxDirProperty,\n        for example, uses it.\n\nC++: wxPGProperty::DoGetValidator() const --> class wxValidator *", pybind11::return_value_policy::automatic);
		cl.def("OnCustomPaint", (void (wxPGProperty::*)(class wxDC &, const class wxRect &, struct wxPGPaintData &)) &wxPGProperty::OnCustomPaint, "Override to paint an image in front of the property value text or\n        drop-down list item (but only if wxPGProperty::OnMeasureImage is\n        overridden as well).\n\n        If property's OnMeasureImage() returns size that has height != 0 but\n        less than row height ( < 0 has special meanings), wxPropertyGrid calls\n        this method to draw a custom image in a limited area in front of the\n        editor control or value text/graphics, and if control has drop-down\n        list, then the image is drawn there as well (even in the case\n        OnMeasureImage() returned higher height than row height).\n\n        NOTE: Following applies when OnMeasureImage() returns a \"flexible\"\n        height ( using wxPG_FLEXIBLE_SIZE(W,H) macro), which implies variable\n        height items: If rect.x is < 0, then this is a measure item call, which\n        means that dc is invalid and only thing that should be done is to set\n        paintdata.m_drawnHeight to the height of the image of item at index\n        paintdata.m_choiceItem. This call may be done even as often as once\n        every drop-down popup show.\n\n        \n\n            wxDC to paint on.\n        \n\n\n            Box reserved for custom graphics. Includes surrounding rectangle,\n            if any. If x is < 0, then this is a measure item call (see above).\n        \n\n\n            wxPGPaintData structure with much useful data.\n\n        \n\n            - You can actually exceed rect width, but if you do so then\n              paintdata.m_drawnWidth must be set to the full width drawn in\n              pixels.\n            - Due to technical reasons, rect's height will be default even if\n              custom height was reported during measure call.\n            - Brush is guaranteed to be default background colour. It has been\n              already used to clear the background of area being painted. It\n              can be modified.\n            - Pen is guaranteed to be 1-wide 'black' (or whatever is the proper\n              colour) pen for drawing framing rectangle. It can be changed as\n              well.\n\n        \n ValueToString()\n\nC++: wxPGProperty::OnCustomPaint(class wxDC &, const class wxRect &, struct wxPGPaintData &) --> void", pybind11::arg("dc"), pybind11::arg("rect"), pybind11::arg("paintdata"));
		cl.def("GetCellRenderer", (class wxPGCellRenderer * (wxPGProperty::*)(int) const) &wxPGProperty::GetCellRenderer, "Returns used wxPGCellRenderer instance for given property column\n        (label=0, value=1).\n\n        Default implementation returns editor's renderer for all columns.\n\nC++: wxPGProperty::GetCellRenderer(int) const --> class wxPGCellRenderer *", pybind11::return_value_policy::automatic, pybind11::arg("column"));
		cl.def("GetChoiceSelection", (int (wxPGProperty::*)() const) &wxPGProperty::GetChoiceSelection, "Returns which choice is currently selected. Only applies to properties\n        which have choices.\n\n        Needs to reimplemented in derived class if property value does not\n        map directly to a choice. Integer as index, bool, and string usually do.\n\nC++: wxPGProperty::GetChoiceSelection() const --> int");
		cl.def("RefreshChildren", (void (wxPGProperty::*)()) &wxPGProperty::RefreshChildren, "Refresh values of child properties.\n\n        Automatically called after value is set.\n\nC++: wxPGProperty::RefreshChildren() --> void");
		cl.def("DoSetAttribute", (bool (wxPGProperty::*)(const class wxString &, class wxVariant &)) &wxPGProperty::DoSetAttribute, "Reimplement this member function to add special handling for\n        attributes of this property.\n\n        \n Return  to have the attribute automatically stored in\n                m_attributes. Default implementation simply does that and\n                nothing else.\n\n        \n To actually set property attribute values from the\n                 application, use wxPGProperty::SetAttribute() instead.\n\nC++: wxPGProperty::DoSetAttribute(const class wxString &, class wxVariant &) --> bool", pybind11::arg("name"), pybind11::arg("value"));
		cl.def("DoGetAttribute", (class wxVariant (wxPGProperty::*)(const class wxString &) const) &wxPGProperty::DoGetAttribute, "Returns value of an attribute.\n\n        Override if custom handling of attributes is needed.\n\n        Default implementation simply return NULL variant.\n\nC++: wxPGProperty::DoGetAttribute(const class wxString &) const --> class wxVariant", pybind11::arg("name"));
		cl.def("OnValidationFailure", (void (wxPGProperty::*)(class wxVariant &)) &wxPGProperty::OnValidationFailure, "Called whenever validation has failed with given pending value.\n\n        \n If you implement this in your custom property class, please\n                 remember to call the baser implementation as well, since they\n                 may use it to revert property into pre-change state.\n\nC++: wxPGProperty::OnValidationFailure(class wxVariant &) --> void", pybind11::arg("pendingValue"));
		cl.def("AddChoice", [](wxPGProperty &o, const class wxString & a0) -> int { return o.AddChoice(a0); }, "", pybind11::arg("label"));
		cl.def("AddChoice", (int (wxPGProperty::*)(const class wxString &, int)) &wxPGProperty::AddChoice, "Append a new choice to property's list of choices.\n\nC++: wxPGProperty::AddChoice(const class wxString &, int) --> int", pybind11::arg("label"), pybind11::arg("value"));
		cl.def("AreChildrenComponents", (bool (wxPGProperty::*)() const) &wxPGProperty::AreChildrenComponents, "Returns true if children of this property are component values (for\n        instance, points size, face name, and is_underlined are component\n        values of a font).\n\nC++: wxPGProperty::AreChildrenComponents() const --> bool");
		cl.def("DeleteChildren", (void (wxPGProperty::*)()) &wxPGProperty::DeleteChildren, "Deletes children of the property.\n\nC++: wxPGProperty::DeleteChildren() --> void");
		cl.def("DeleteChoice", (void (wxPGProperty::*)(int)) &wxPGProperty::DeleteChoice, "Removes entry from property's wxPGChoices and editor control (if it is\n        active).\n\n        If selected item is deleted, then the value is set to unspecified.\n\nC++: wxPGProperty::DeleteChoice(int) --> void", pybind11::arg("index"));
		cl.def("Enable", [](wxPGProperty &o) -> void { return o.Enable(); }, "");
		cl.def("Enable", (void (wxPGProperty::*)(bool)) &wxPGProperty::Enable, "Enables or disables the property. Disabled property usually appears\n        as having grey text.\n\n        \n\n            If , property is disabled instead.\n\n        \n wxPropertyGridInterface::EnableProperty()\n\nC++: wxPGProperty::Enable(bool) --> void", pybind11::arg("enable"));
		cl.def("EnableCommonValue", [](wxPGProperty &o) -> void { return o.EnableCommonValue(); }, "");
		cl.def("EnableCommonValue", (void (wxPGProperty::*)(bool)) &wxPGProperty::EnableCommonValue, "Call to enable or disable usage of common value (integer value that can\n        be selected for properties instead of their normal values) for this\n        property.\n\n        Common values are disabled by the default for all properties.\n\nC++: wxPGProperty::EnableCommonValue(bool) --> void", pybind11::arg("enable"));
		cl.def("GenerateComposedValue", (class wxString (wxPGProperty::*)() const) &wxPGProperty::GenerateComposedValue, "Composes text from values of child properties.\n\nC++: wxPGProperty::GenerateComposedValue() const --> class wxString");
		cl.def("GetLabel", (const class wxString & (wxPGProperty::*)() const) &wxPGProperty::GetLabel, "Returns property's label. \n\nC++: wxPGProperty::GetLabel() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetName", (class wxString (wxPGProperty::*)() const) &wxPGProperty::GetName, "Returns property's name with all (non-category, non-root) parents. \n\nC++: wxPGProperty::GetName() const --> class wxString");
		cl.def("GetBaseName", (const class wxString & (wxPGProperty::*)() const) &wxPGProperty::GetBaseName, "Returns property's base name (ie parent's name is not added in any\n        case)\n\nC++: wxPGProperty::GetBaseName() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetChoices", (const class wxPGChoices & (wxPGProperty::*)() const) &wxPGProperty::GetChoices, "Returns read-only reference to property's list of choices.\n\nC++: wxPGProperty::GetChoices() const --> const class wxPGChoices &", pybind11::return_value_policy::automatic);
		cl.def("GetY", (int (wxPGProperty::*)() const) &wxPGProperty::GetY, "Returns coordinate to the top y of the property. Note that the\n        position of scrollbars is not taken into account.\n\nC++: wxPGProperty::GetY() const --> int");
		cl.def("GetValue", (class wxVariant (wxPGProperty::*)() const) &wxPGProperty::GetValue, "C++: wxPGProperty::GetValue() const --> class wxVariant");
		cl.def("GetValueRef", (class wxVariant & (wxPGProperty::*)()) &wxPGProperty::GetValueRef, "Returns reference to the internal stored value. GetValue is preferred\n        way to get the actual value, since GetValueRef ignores DoGetValue,\n        which may override stored value.\n\nC++: wxPGProperty::GetValueRef() --> class wxVariant &", pybind11::return_value_policy::automatic);
		cl.def("GetValuePlain", (class wxVariant (wxPGProperty::*)() const) &wxPGProperty::GetValuePlain, "C++: wxPGProperty::GetValuePlain() const --> class wxVariant");
		cl.def("GetValueAsString", [](wxPGProperty const &o) -> wxString { return o.GetValueAsString(); }, "");
		cl.def("GetValueAsString", (class wxString (wxPGProperty::*)(int) const) &wxPGProperty::GetValueAsString, "Returns text representation of property's value.\n\n        \n\n            If 0 (default value), then displayed string is returned.\n            If wxPG_FULL_VALUE is set, returns complete, storable string value\n            instead of displayable. If wxPG_EDITABLE_VALUE is set, returns\n            string value that must be editable in textctrl. If\n            wxPG_COMPOSITE_FRAGMENT is set, returns text that is appropriate to\n            display as a part of string property's composite text\n            representation.\n\n        \n In older versions, this function used to be overridden to convert\n                 property's value into a string representation. This function is\n                 now handled by ValueToString(), and overriding this function now\n                 will result in run-time assertion failure.\n\nC++: wxPGProperty::GetValueAsString(int) const --> class wxString", pybind11::arg("argFlags"));
		cl.def("GetValueString", [](wxPGProperty const &o) -> wxString { return o.GetValueString(); }, "");
		cl.def("GetValueString", (class wxString (wxPGProperty::*)(int) const) &wxPGProperty::GetValueString, "C++: wxPGProperty::GetValueString(int) const --> class wxString", pybind11::arg("argFlags"));
		cl.def("GetCell", (class wxPGCell & (wxPGProperty::*)(unsigned int)) &wxPGProperty::GetCell, "Returns wxPGCell of given column, creating one if necessary.\n\nC++: wxPGProperty::GetCell(unsigned int) --> class wxPGCell &", pybind11::return_value_policy::automatic, pybind11::arg("column"));
		cl.def("GetOrCreateCell", (class wxPGCell & (wxPGProperty::*)(unsigned int)) &wxPGProperty::GetOrCreateCell, "Returns wxPGCell of given column, creating one if necessary.\n\nC++: wxPGProperty::GetOrCreateCell(unsigned int) --> class wxPGCell &", pybind11::return_value_policy::automatic, pybind11::arg("column"));
		cl.def("GetDisplayedCommonValueCount", (int (wxPGProperty::*)() const) &wxPGProperty::GetDisplayedCommonValueCount, "Return number of displayed common values for this property.\n\nC++: wxPGProperty::GetDisplayedCommonValueCount() const --> int");
		cl.def("GetDisplayedString", (class wxString (wxPGProperty::*)() const) &wxPGProperty::GetDisplayedString, "C++: wxPGProperty::GetDisplayedString() const --> class wxString");
		cl.def("GetHintText", (class wxString (wxPGProperty::*)() const) &wxPGProperty::GetHintText, "Returns property's hint text (shown in empty value cell).\n\nC++: wxPGProperty::GetHintText() const --> class wxString");
		cl.def("GetMainParent", (class wxPGProperty * (wxPGProperty::*)() const) &wxPGProperty::GetMainParent, "Returns highest level non-category, non-root parent. Useful when you\n        have nested wxCustomProperties/wxParentProperties.\n        \n\n\n        Thus, if immediate parent is root or category, this will return the\n        property itself.\n\nC++: wxPGProperty::GetMainParent() const --> class wxPGProperty *", pybind11::return_value_policy::automatic);
		cl.def("GetParent", (class wxPGProperty * (wxPGProperty::*)() const) &wxPGProperty::GetParent, "Return parent of property \n\nC++: wxPGProperty::GetParent() const --> class wxPGProperty *", pybind11::return_value_policy::automatic);
		cl.def("IsTextEditable", (bool (wxPGProperty::*)() const) &wxPGProperty::IsTextEditable, "Returns true if property has editable wxTextCtrl when selected.\n\n        \n\n        Although disabled properties do not displayed editor, they still\n        return True here as being disabled is considered a temporary\n        condition (unlike being read-only or having limited editing enabled).\n\nC++: wxPGProperty::IsTextEditable() const --> bool");
		cl.def("IsValueUnspecified", (bool (wxPGProperty::*)() const) &wxPGProperty::IsValueUnspecified, "C++: wxPGProperty::IsValueUnspecified() const --> bool");
		cl.def("HasFlag", (unsigned int (wxPGProperty::*)(enum wxPGPropertyFlags) const) &wxPGProperty::HasFlag, "Returns non-zero if property has given flag set.\n\n        \n propgrid_propflags\n\nC++: wxPGProperty::HasFlag(enum wxPGPropertyFlags) const --> unsigned int", pybind11::arg("flag"));
		cl.def("GetAttributes", (const class wxPGAttributeStorage & (wxPGProperty::*)() const) &wxPGProperty::GetAttributes, "Returns comma-delimited string of property attributes.\n\nC++: wxPGProperty::GetAttributes() const --> const class wxPGAttributeStorage &", pybind11::return_value_policy::automatic);
		cl.def("GetAttributesAsList", (class wxVariant (wxPGProperty::*)() const) &wxPGProperty::GetAttributesAsList, "Returns m_attributes as list wxVariant.\n\nC++: wxPGProperty::GetAttributesAsList() const --> class wxVariant");
		cl.def("GetFlags", (unsigned int (wxPGProperty::*)() const) &wxPGProperty::GetFlags, "Returns property flags.\n\nC++: wxPGProperty::GetFlags() const --> unsigned int");
		cl.def("GetValueType", (class wxString (wxPGProperty::*)() const) &wxPGProperty::GetValueType, "C++: wxPGProperty::GetValueType() const --> class wxString");
		cl.def("GetCommonValue", (int (wxPGProperty::*)() const) &wxPGProperty::GetCommonValue, "Returns common value selected for this property. -1 for none.\n\nC++: wxPGProperty::GetCommonValue() const --> int");
		cl.def("HasVisibleChildren", (bool (wxPGProperty::*)() const) &wxPGProperty::HasVisibleChildren, "Returns true if property has even one visible child.\n\nC++: wxPGProperty::HasVisibleChildren() const --> bool");
		cl.def("InsertChild", (class wxPGProperty * (wxPGProperty::*)(int, class wxPGProperty *)) &wxPGProperty::InsertChild, "Use this member function to add independent (ie. regular) children to\n        a property.\n\n        \n Inserted childProperty.\n\n        \n wxPropertyGrid is not automatically refreshed by this\n                 function.\n\n        \n AddPrivateChild()\n\nC++: wxPGProperty::InsertChild(int, class wxPGProperty *) --> class wxPGProperty *", pybind11::return_value_policy::automatic, pybind11::arg("index"), pybind11::arg("childProperty"));
		cl.def("InsertChoice", [](wxPGProperty &o, const class wxString & a0, int const & a1) -> int { return o.InsertChoice(a0, a1); }, "", pybind11::arg("label"), pybind11::arg("index"));
		cl.def("InsertChoice", (int (wxPGProperty::*)(const class wxString &, int, int)) &wxPGProperty::InsertChoice, "Inserts a new choice to property's list of choices.\n\nC++: wxPGProperty::InsertChoice(const class wxString &, int, int) --> int", pybind11::arg("label"), pybind11::arg("index"), pybind11::arg("value"));
		cl.def("IsCategory", (bool (wxPGProperty::*)() const) &wxPGProperty::IsCategory, "Returns true if this property is actually a wxPropertyCategory.\n\nC++: wxPGProperty::IsCategory() const --> bool");
		cl.def("IsRoot", (bool (wxPGProperty::*)() const) &wxPGProperty::IsRoot, "Returns true if this property is actually a wxRootProperty.\n\nC++: wxPGProperty::IsRoot() const --> bool");
		cl.def("IsSubProperty", (bool (wxPGProperty::*)() const) &wxPGProperty::IsSubProperty, "Returns true if this is a sub-property. \n\nC++: wxPGProperty::IsSubProperty() const --> bool");
		cl.def("GetLastVisibleSubItem", (const class wxPGProperty * (wxPGProperty::*)() const) &wxPGProperty::GetLastVisibleSubItem, "Returns last visible sub-property, recursively.\n\nC++: wxPGProperty::GetLastVisibleSubItem() const --> const class wxPGProperty *", pybind11::return_value_policy::automatic);
		cl.def("GetDefaultValue", (class wxVariant (wxPGProperty::*)() const) &wxPGProperty::GetDefaultValue, "C++: wxPGProperty::GetDefaultValue() const --> class wxVariant");
		cl.def("GetMaxLength", (int (wxPGProperty::*)() const) &wxPGProperty::GetMaxLength, "C++: wxPGProperty::GetMaxLength() const --> int");
		cl.def("AreAllChildrenSpecified", [](wxPGProperty const &o) -> bool { return o.AreAllChildrenSpecified(); }, "");
		cl.def("AreAllChildrenSpecified", (bool (wxPGProperty::*)(class wxVariant *) const) &wxPGProperty::AreAllChildrenSpecified, "Determines, recursively, if all children are not unspecified.\n\n        \n\n            Assumes members in this wxVariant list as pending\n            replacement values.\n\nC++: wxPGProperty::AreAllChildrenSpecified(class wxVariant *) const --> bool", pybind11::arg("pendingList"));
		cl.def("UpdateParentValues", (class wxPGProperty * (wxPGProperty::*)()) &wxPGProperty::UpdateParentValues, "Updates composed values of parent non-category properties, recursively.\n        Returns topmost property updated.\n\n        \n\n        - Must not call SetValue() (as can be called in it).\n\nC++: wxPGProperty::UpdateParentValues() --> class wxPGProperty *", pybind11::return_value_policy::automatic);
		cl.def("UsesAutoUnspecified", (bool (wxPGProperty::*)() const) &wxPGProperty::UsesAutoUnspecified, "Returns true if containing grid uses wxPG_EX_AUTO_UNSPECIFIED_VALUES.\n\nC++: wxPGProperty::UsesAutoUnspecified() const --> bool");
		cl.def("GetValueImage", (class wxBitmap * (wxPGProperty::*)() const) &wxPGProperty::GetValueImage, "C++: wxPGProperty::GetValueImage() const --> class wxBitmap *", pybind11::return_value_policy::automatic);
		cl.def("GetAttribute", (class wxVariant (wxPGProperty::*)(const class wxString &) const) &wxPGProperty::GetAttribute, "C++: wxPGProperty::GetAttribute(const class wxString &) const --> class wxVariant", pybind11::arg("name"));
		cl.def("GetAttribute", (class wxString (wxPGProperty::*)(const class wxString &, const class wxString &) const) &wxPGProperty::GetAttribute, "Returns named attribute, as string, if found.\n\n        Otherwise defVal is returned.\n\nC++: wxPGProperty::GetAttribute(const class wxString &, const class wxString &) const --> class wxString", pybind11::arg("name"), pybind11::arg("defVal"));
		cl.def("GetAttributeAsLong", (long (wxPGProperty::*)(const class wxString &, long) const) &wxPGProperty::GetAttributeAsLong, "Returns named attribute, as long, if found.\n\n        Otherwise defVal is returned.\n\nC++: wxPGProperty::GetAttributeAsLong(const class wxString &, long) const --> long", pybind11::arg("name"), pybind11::arg("defVal"));
		cl.def("GetAttributeAsDouble", (double (wxPGProperty::*)(const class wxString &, double) const) &wxPGProperty::GetAttributeAsDouble, "Returns named attribute, as double, if found.\n\n        Otherwise defVal is returned.\n\nC++: wxPGProperty::GetAttributeAsDouble(const class wxString &, double) const --> double", pybind11::arg("name"), pybind11::arg("defVal"));
		cl.def("GetDepth", (unsigned int (wxPGProperty::*)() const) &wxPGProperty::GetDepth, "C++: wxPGProperty::GetDepth() const --> unsigned int");
		cl.def("GetFlagsAsString", (class wxString (wxPGProperty::*)(unsigned int) const) &wxPGProperty::GetFlagsAsString, "Gets flags as a'|' delimited string. Note that flag names are not\n        prepended with 'wxPG_PROP_'.\n        \n\n\n        String will only be made to include flags combined by this parameter.\n\nC++: wxPGProperty::GetFlagsAsString(unsigned int) const --> class wxString", pybind11::arg("flagsMask"));
		cl.def("GetIndexInParent", (unsigned int (wxPGProperty::*)() const) &wxPGProperty::GetIndexInParent, "Returns position in parent's array. \n\nC++: wxPGProperty::GetIndexInParent() const --> unsigned int");
		cl.def("Hide", [](wxPGProperty &o, bool const & a0) -> bool { return o.Hide(a0); }, "", pybind11::arg("hide"));
		cl.def("Hide", (bool (wxPGProperty::*)(bool, int)) &wxPGProperty::Hide, "Hides or reveals the property.\n        \n\n\n            true for hide, false for reveal.\n        \n\n\n            By default changes are applied recursively. Set this paramter\n            wxPG_DONT_RECURSE to prevent this.\n\nC++: wxPGProperty::Hide(bool, int) --> bool", pybind11::arg("hide"), pybind11::arg("flags"));
		cl.def("IsExpanded", (bool (wxPGProperty::*)() const) &wxPGProperty::IsExpanded, "C++: wxPGProperty::IsExpanded() const --> bool");
		cl.def("IsVisible", (bool (wxPGProperty::*)() const) &wxPGProperty::IsVisible, "Returns true if all parents expanded.\n\nC++: wxPGProperty::IsVisible() const --> bool");
		cl.def("IsEnabled", (bool (wxPGProperty::*)() const) &wxPGProperty::IsEnabled, "C++: wxPGProperty::IsEnabled() const --> bool");
		cl.def("RecreateEditor", (bool (wxPGProperty::*)()) &wxPGProperty::RecreateEditor, "If property's editor is created this forces its recreation.\n        Useful in SetAttribute etc. Returns true if actually did anything.\n\nC++: wxPGProperty::RecreateEditor() --> bool");
		cl.def("RefreshEditor", (void (wxPGProperty::*)()) &wxPGProperty::RefreshEditor, "If property's editor is active, then update it's value.\n\nC++: wxPGProperty::RefreshEditor() --> void");
		cl.def("SetAttribute", (void (wxPGProperty::*)(const class wxString &, class wxVariant)) &wxPGProperty::SetAttribute, "Sets an attribute for this property.\n        \n\n\n        Text identifier of attribute. See \n        \n\n\n        Value of attribute.\n\nC++: wxPGProperty::SetAttribute(const class wxString &, class wxVariant) --> void", pybind11::arg("name"), pybind11::arg("value"));
		cl.def("SetAttributes", (void (wxPGProperty::*)(const class wxPGAttributeStorage &)) &wxPGProperty::SetAttributes, "C++: wxPGProperty::SetAttributes(const class wxPGAttributeStorage &) --> void", pybind11::arg("attributes"));
		cl.def("SetAutoUnspecified", [](wxPGProperty &o) -> void { return o.SetAutoUnspecified(); }, "");
		cl.def("SetAutoUnspecified", (void (wxPGProperty::*)(bool)) &wxPGProperty::SetAutoUnspecified, "Set if user can change the property's value to unspecified by\n        modifying the value of the editor control (usually by clearing\n        it).  Currently, this can work with following properties:\n        wxIntProperty, wxUIntProperty, wxFloatProperty, wxEditEnumProperty.\n\n        \n\n            Whether to enable or disable this behaviour (it is disabled by\n            default).\n\nC++: wxPGProperty::SetAutoUnspecified(bool) --> void", pybind11::arg("enable"));
		cl.def("SetBackgroundColour", [](wxPGProperty &o, const class wxColour & a0) -> void { return o.SetBackgroundColour(a0); }, "", pybind11::arg("colour"));
		cl.def("SetBackgroundColour", (void (wxPGProperty::*)(const class wxColour &, int)) &wxPGProperty::SetBackgroundColour, "Sets property's background colour.\n\n        \n\n            Background colour to use.\n\n        \n\n            Default is wxPG_RECURSE which causes colour to be set recursively.\n            Omit this flag to only set colour for the property in question\n            and not any of its children.\n\nC++: wxPGProperty::SetBackgroundColour(const class wxColour &, int) --> void", pybind11::arg("colour"), pybind11::arg("flags"));
		cl.def("SetTextColour", [](wxPGProperty &o, const class wxColour & a0) -> void { return o.SetTextColour(a0); }, "", pybind11::arg("colour"));
		cl.def("SetTextColour", (void (wxPGProperty::*)(const class wxColour &, int)) &wxPGProperty::SetTextColour, "Sets property's text colour.\n\n        \n\n            Text colour to use.\n\n        \n\n            Default is wxPG_RECURSE which causes colour to be set recursively.\n            Omit this flag to only set colour for the property in question\n            and not any of its children.\n\nC++: wxPGProperty::SetTextColour(const class wxColour &, int) --> void", pybind11::arg("colour"), pybind11::arg("flags"));
		cl.def("SetDefaultValue", (void (wxPGProperty::*)(class wxVariant &)) &wxPGProperty::SetDefaultValue, "Set default value of a property. Synonymous to\n\n        \n\n\n    \n\nC++: wxPGProperty::SetDefaultValue(class wxVariant &) --> void", pybind11::arg("value"));
		cl.def("SetEditor", (void (wxPGProperty::*)(const class wxString &)) &wxPGProperty::SetEditor, "Sets editor for a property.\n\nC++: wxPGProperty::SetEditor(const class wxString &) --> void", pybind11::arg("editorName"));
		cl.def("SetCell", (void (wxPGProperty::*)(int, const class wxPGCell &)) &wxPGProperty::SetCell, "Sets cell information for given column.\n\nC++: wxPGProperty::SetCell(int, const class wxPGCell &) --> void", pybind11::arg("column"), pybind11::arg("cell"));
		cl.def("SetCommonValue", (void (wxPGProperty::*)(int)) &wxPGProperty::SetCommonValue, "Sets common value selected for this property. -1 for none.\n\nC++: wxPGProperty::SetCommonValue(int) --> void", pybind11::arg("commonValue"));
		cl.def("SetFlagsFromString", (void (wxPGProperty::*)(const class wxString &)) &wxPGProperty::SetFlagsFromString, "Sets flags from a '|' delimited string. Note that flag names are not\n        prepended with 'wxPG_PROP_'.\n\nC++: wxPGProperty::SetFlagsFromString(const class wxString &) --> void", pybind11::arg("str"));
		cl.def("SetModifiedStatus", (void (wxPGProperty::*)(bool)) &wxPGProperty::SetModifiedStatus, "Sets property's \"is it modified?\" flag. Affects children recursively.\n\nC++: wxPGProperty::SetModifiedStatus(bool) --> void", pybind11::arg("modified"));
		cl.def("SetValueInEvent", (void (wxPGProperty::*)(class wxVariant) const) &wxPGProperty::SetValueInEvent, "Call in OnEvent(), OnButtonClick() etc. to change the property value\n        based on user input.\n\n        \n\n        This method is const since it doesn't actually modify value, but posts\n        given variant as pending value, stored in wxPropertyGrid.\n\nC++: wxPGProperty::SetValueInEvent(class wxVariant) const --> void", pybind11::arg("value"));
		cl.def("SetValue", [](wxPGProperty &o, class wxVariant const & a0) -> void { return o.SetValue(a0); }, "", pybind11::arg("value"));
		cl.def("SetValue", [](wxPGProperty &o, class wxVariant const & a0, class wxVariant * a1) -> void { return o.SetValue(a0, a1); }, "", pybind11::arg("value"), pybind11::arg("pList"));
		cl.def("SetValue", (void (wxPGProperty::*)(class wxVariant, class wxVariant *, int)) &wxPGProperty::SetValue, "Call this to set value of the property.\n\n        Unlike methods in wxPropertyGrid, this does not automatically update\n        the display.\n\n        \n\n        Use wxPropertyGrid::ChangePropertyValue() instead if you need to run\n        through validation process and send property change event.\n\n        If you need to change property value in event, based on user input, use\n        SetValueInEvent() instead.\n\n        \n\n            Pointer to list variant that contains child values. Used to\n            indicate which children should be marked as modified.\n\n        \n\n            Various flags (for instance, wxPG_SETVAL_REFRESH_EDITOR, which is\n            enabled by default).\n\nC++: wxPGProperty::SetValue(class wxVariant, class wxVariant *, int) --> void", pybind11::arg("value"), pybind11::arg("pList"), pybind11::arg("flags"));
		cl.def("SetValueImage", (void (wxPGProperty::*)(class wxBitmap &)) &wxPGProperty::SetValueImage, "Set wxBitmap in front of the value. This bitmap may be ignored\n        by custom cell renderers.\n\nC++: wxPGProperty::SetValueImage(class wxBitmap &) --> void", pybind11::arg("bmp"));
		cl.def("SetChoiceSelection", (void (wxPGProperty::*)(int)) &wxPGProperty::SetChoiceSelection, "Sets selected choice and changes property value.\n\n        Tries to retain value type, although currently if it is not string,\n        then it is forced to integer.\n\nC++: wxPGProperty::SetChoiceSelection(int) --> void", pybind11::arg("newValue"));
		cl.def("SetExpanded", (void (wxPGProperty::*)(bool)) &wxPGProperty::SetExpanded, "C++: wxPGProperty::SetExpanded(bool) --> void", pybind11::arg("expanded"));
		cl.def("ChangeFlag", (void (wxPGProperty::*)(enum wxPGPropertyFlags, bool)) &wxPGProperty::ChangeFlag, "Sets or clears given property flag. Mainly for internal use.\n\n        \n Setting a property flag never has any side-effect, and is\n                 intended almost exclusively for internal use. So, for\n                 example, if you want to disable a property, call\n                 Enable(false) instead of setting wxPG_PROP_DISABLED flag.\n\n        \n HasFlag(), GetFlags()\n\nC++: wxPGProperty::ChangeFlag(enum wxPGPropertyFlags, bool) --> void", pybind11::arg("flag"), pybind11::arg("set"));
		cl.def("SetFlagRecursively", (void (wxPGProperty::*)(enum wxPGPropertyFlags, bool)) &wxPGProperty::SetFlagRecursively, "Sets or clears given property flag, recursively. This function is\n        primarily intended for internal use.\n\n        \n ChangeFlag()\n\nC++: wxPGProperty::SetFlagRecursively(enum wxPGPropertyFlags, bool) --> void", pybind11::arg("flag"), pybind11::arg("set"));
		cl.def("SetHelpString", (void (wxPGProperty::*)(const class wxString &)) &wxPGProperty::SetHelpString, "C++: wxPGProperty::SetHelpString(const class wxString &) --> void", pybind11::arg("helpString"));
		cl.def("SetLabel", (void (wxPGProperty::*)(const class wxString &)) &wxPGProperty::SetLabel, "C++: wxPGProperty::SetLabel(const class wxString &) --> void", pybind11::arg("label"));
		cl.def("SetName", (void (wxPGProperty::*)(const class wxString &)) &wxPGProperty::SetName, "C++: wxPGProperty::SetName(const class wxString &) --> void", pybind11::arg("newName"));
		cl.def("SetParentalType", (void (wxPGProperty::*)(int)) &wxPGProperty::SetParentalType, "Changes what sort of parent this property is for its children.\n\n        \n\n            Use one of the following values: wxPG_PROP_MISC_PARENT (for\n            generic parents), wxPG_PROP_CATEGORY (for categories), or\n            wxPG_PROP_AGGREGATE (for derived property classes with private\n            children).\n\n        \n You generally do not need to call this function.\n\nC++: wxPGProperty::SetParentalType(int) --> void", pybind11::arg("flag"));
		cl.def("SetValueToUnspecified", (void (wxPGProperty::*)()) &wxPGProperty::SetValueToUnspecified, "C++: wxPGProperty::SetValueToUnspecified() --> void");
		cl.def("SetValuePlain", (void (wxPGProperty::*)(class wxVariant)) &wxPGProperty::SetValuePlain, "C++: wxPGProperty::SetValuePlain(class wxVariant) --> void", pybind11::arg("value"));
		cl.def("SetValidator", (void (wxPGProperty::*)(const class wxValidator &)) &wxPGProperty::SetValidator, "Sets wxValidator for a property\n\nC++: wxPGProperty::SetValidator(const class wxValidator &) --> void", pybind11::arg("validator"));
		cl.def("GetValidator", (class wxValidator * (wxPGProperty::*)() const) &wxPGProperty::GetValidator, "Gets assignable version of property's validator. \n\nC++: wxPGProperty::GetValidator() const --> class wxValidator *", pybind11::return_value_policy::automatic);
		cl.def("GetClientData", (void * (wxPGProperty::*)() const) &wxPGProperty::GetClientData, "Returns client data (void*) of a property.\n\nC++: wxPGProperty::GetClientData() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("SetClientData", (void (wxPGProperty::*)(void *)) &wxPGProperty::SetClientData, "Sets client data (void*) of a property.\n        \n\n\n        This untyped client data has to be deleted manually.\n\nC++: wxPGProperty::SetClientData(void *) --> void", pybind11::arg("clientData"));
		cl.def("SetClientObject", (void (wxPGProperty::*)(class wxClientData *)) &wxPGProperty::SetClientObject, "Returns client object of a property.\n\nC++: wxPGProperty::SetClientObject(class wxClientData *) --> void", pybind11::arg("clientObject"));
		cl.def("GetClientObject", (class wxClientData * (wxPGProperty::*)() const) &wxPGProperty::GetClientObject, "Sets managed client object of a property.\n\nC++: wxPGProperty::GetClientObject() const --> class wxClientData *", pybind11::return_value_policy::automatic);
		cl.def("SetChoices", (bool (wxPGProperty::*)(const class wxPGChoices &)) &wxPGProperty::SetChoices, "Sets new set of choices for the property.\n\n        \n This operation deselects the property and clears its\n                 value.\n\nC++: wxPGProperty::SetChoices(const class wxPGChoices &) --> bool", pybind11::arg("choices"));
		cl.def("SetMaxLength", (bool (wxPGProperty::*)(int)) &wxPGProperty::SetMaxLength, "Set max length of text in text editor.\n\nC++: wxPGProperty::SetMaxLength(int) --> bool", pybind11::arg("maxLen"));
		cl.def("SetWasModified", [](wxPGProperty &o) -> void { return o.SetWasModified(); }, "");
		cl.def("SetWasModified", (void (wxPGProperty::*)(bool)) &wxPGProperty::SetWasModified, "Call with 'false' in OnSetValue to cancel value changes after all\n        (ie. cancel 'true' returned by StringToValue() or IntToValue()).\n\nC++: wxPGProperty::SetWasModified(bool) --> void", pybind11::arg("set"));
		cl.def("GetHelpString", (const class wxString & (wxPGProperty::*)() const) &wxPGProperty::GetHelpString, "C++: wxPGProperty::GetHelpString() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("IsSomeParent", (bool (wxPGProperty::*)(class wxPGProperty *) const) &wxPGProperty::IsSomeParent, "C++: wxPGProperty::IsSomeParent(class wxPGProperty *) const --> bool", pybind11::arg("candidate_parent"));
		cl.def("AdaptListToValue", (void (wxPGProperty::*)(class wxVariant &, class wxVariant *) const) &wxPGProperty::AdaptListToValue, "Adapts list variant into proper value using consecutive\n        ChildChanged-calls.\n\nC++: wxPGProperty::AdaptListToValue(class wxVariant &, class wxVariant *) const --> void", pybind11::arg("list"), pybind11::arg("value"));
		cl.def("AddChild", (void (wxPGProperty::*)(class wxPGProperty *)) &wxPGProperty::AddChild, "C++: wxPGProperty::AddChild(class wxPGProperty *) --> void", pybind11::arg("prop"));
		cl.def("AddPrivateChild", (void (wxPGProperty::*)(class wxPGProperty *)) &wxPGProperty::AddPrivateChild, "Adds a private child property. If you use this instead of\n        wxPropertyGridInterface::Insert() or\n        wxPropertyGridInterface::AppendIn(), then property's parental\n        type will automatically be set up to wxPG_PROP_AGGREGATE. In other\n        words, all properties of this property will become private.\n\nC++: wxPGProperty::AddPrivateChild(class wxPGProperty *) --> void", pybind11::arg("prop"));
		cl.def("AppendChild", (class wxPGProperty * (wxPGProperty::*)(class wxPGProperty *)) &wxPGProperty::AppendChild, "Appends a new child property.\n\nC++: wxPGProperty::AppendChild(class wxPGProperty *) --> class wxPGProperty *", pybind11::return_value_policy::automatic, pybind11::arg("prop"));
		cl.def("GetChildrenHeight", [](wxPGProperty const &o, int const & a0) -> int { return o.GetChildrenHeight(a0); }, "", pybind11::arg("lh"));
		cl.def("GetChildrenHeight", (int (wxPGProperty::*)(int, int) const) &wxPGProperty::GetChildrenHeight, "Returns height of children, recursively, and\n        by taking expanded/collapsed status into account.\n\n        iMax is used when finding property y-positions.\n\nC++: wxPGProperty::GetChildrenHeight(int, int) const --> int", pybind11::arg("lh"), pybind11::arg("iMax"));
		cl.def("GetChildCount", (unsigned int (wxPGProperty::*)() const) &wxPGProperty::GetChildCount, "Returns number of child properties \n\nC++: wxPGProperty::GetChildCount() const --> unsigned int");
		cl.def("Item", (class wxPGProperty * (wxPGProperty::*)(unsigned int) const) &wxPGProperty::Item, "Returns sub-property at index i. \n\nC++: wxPGProperty::Item(unsigned int) const --> class wxPGProperty *", pybind11::return_value_policy::automatic, pybind11::arg("i"));
		cl.def("Last", (class wxPGProperty * (wxPGProperty::*)() const) &wxPGProperty::Last, "Returns last sub-property.\n\nC++: wxPGProperty::Last() const --> class wxPGProperty *", pybind11::return_value_policy::automatic);
		cl.def("Index", (int (wxPGProperty::*)(const class wxPGProperty *) const) &wxPGProperty::Index, "Returns index of given child property. \n\nC++: wxPGProperty::Index(const class wxPGProperty *) const --> int", pybind11::arg("p"));
		cl.def("FixIndicesOfChildren", [](wxPGProperty &o) -> void { return o.FixIndicesOfChildren(); }, "");
		cl.def("FixIndicesOfChildren", (void (wxPGProperty::*)(unsigned int)) &wxPGProperty::FixIndicesOfChildren, "C++: wxPGProperty::FixIndicesOfChildren(unsigned int) --> void", pybind11::arg("starthere"));
		cl.def("GetImageOffset", (int (wxPGProperty::*)(int) const) &wxPGProperty::GetImageOffset, "Converts image width into full image offset, with margins.\n\nC++: wxPGProperty::GetImageOffset(int) const --> int", pybind11::arg("imageWidth"));
		cl.def("GetItemAtY", (class wxPGProperty * (wxPGProperty::*)(unsigned int, unsigned int, unsigned int *) const) &wxPGProperty::GetItemAtY, "C++: wxPGProperty::GetItemAtY(unsigned int, unsigned int, unsigned int *) const --> class wxPGProperty *", pybind11::return_value_policy::automatic, pybind11::arg("y"), pybind11::arg("lh"), pybind11::arg("nextItemY"));
		cl.def("GetItemAtY", (class wxPGProperty * (wxPGProperty::*)(unsigned int) const) &wxPGProperty::GetItemAtY, "Returns property at given virtual y coordinate.\n\nC++: wxPGProperty::GetItemAtY(unsigned int) const --> class wxPGProperty *", pybind11::return_value_policy::automatic, pybind11::arg("y"));
		cl.def("GetPropertyByName", (class wxPGProperty * (wxPGProperty::*)(const class wxString &) const) &wxPGProperty::GetPropertyByName, "Returns (direct) child property with given name (or NULL if not found).\n\nC++: wxPGProperty::GetPropertyByName(const class wxString &) const --> class wxPGProperty *", pybind11::return_value_policy::automatic, pybind11::arg("name"));
		cl.def("assign", (class wxPGProperty & (wxPGProperty::*)(const class wxPGProperty &)) &wxPGProperty::operator=, "C++: wxPGProperty::operator=(const class wxPGProperty &) --> class wxPGProperty &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}

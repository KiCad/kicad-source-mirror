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

// wxClientDataContainer file: line:125
struct PyCallBack_wxClientDataContainer : public wxClientDataContainer {
	using wxClientDataContainer::wxClientDataContainer;

	void DoSetClientObject(class wxClientData * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxClientDataContainer *>(this), "DoSetClientObject");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxClientDataContainer::DoSetClientObject(a0);
	}
	class wxClientData * DoGetClientObject() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxClientDataContainer *>(this), "DoGetClientObject");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClientData *>::value) {
				static pybind11::detail::override_caster_t<class wxClientData *> caster;
				return pybind11::detail::cast_ref<class wxClientData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClientData *>(std::move(o));
		}
		return wxClientDataContainer::DoGetClientObject();
	}
	void DoSetClientData(void * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxClientDataContainer *>(this), "DoSetClientData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxClientDataContainer::DoSetClientData(a0);
	}
	void * DoGetClientData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxClientDataContainer *>(this), "DoGetClientData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void *>::value) {
				static pybind11::detail::override_caster_t<void *> caster;
				return pybind11::detail::cast_ref<void *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void *>(std::move(o));
		}
		return wxClientDataContainer::DoGetClientData();
	}
};

void bind_unknown_unknown_25(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxShadowObjectFields file: line:26
		pybind11::class_<wxShadowObjectFields, std::shared_ptr<wxShadowObjectFields>, wxShadowObjectFields_wxImplementation_HashTable> cl(M(""), "wxShadowObjectFields", "");
		cl.def( pybind11::init( [](){ return new wxShadowObjectFields(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxShadowObjectFields(a0); } ), "doc" , pybind11::arg("hint"));
		cl.def( pybind11::init( [](unsigned long const & a0, struct wxStringHash const & a1){ return new wxShadowObjectFields(a0, a1); } ), "doc" , pybind11::arg("hint"), pybind11::arg("hf"));
		cl.def( pybind11::init<unsigned long, struct wxStringHash, struct wxStringEqual>(), pybind11::arg("hint"), pybind11::arg("hf"), pybind11::arg("eq") );

		cl.def( pybind11::init( [](wxShadowObjectFields const &o){ return new wxShadowObjectFields(o); } ) );
		cl.def("__getitem__", (void *& (wxShadowObjectFields::*)(const class wxString &)) &wxShadowObjectFields::operator[], "C++: wxShadowObjectFields::operator[](const class wxString &) --> void *&", pybind11::return_value_policy::automatic, pybind11::arg("key"));
		cl.def("find", (class wxShadowObjectFields_wxImplementation_HashTable::iterator (wxShadowObjectFields::*)(const class wxString &)) &wxShadowObjectFields::find, "C++: wxShadowObjectFields::find(const class wxString &) --> class wxShadowObjectFields_wxImplementation_HashTable::iterator", pybind11::arg("key"));
		cl.def("insert", (class wxShadowObjectFields::Insert_Result (wxShadowObjectFields::*)(const class wxShadowObjectFields_wxImplementation_Pair &)) &wxShadowObjectFields::insert, "C++: wxShadowObjectFields::insert(const class wxShadowObjectFields_wxImplementation_Pair &) --> class wxShadowObjectFields::Insert_Result", pybind11::arg("v"));
		cl.def("erase", (unsigned long (wxShadowObjectFields::*)(const class wxString &)) &wxShadowObjectFields::erase, "C++: wxShadowObjectFields::erase(const class wxString &) --> unsigned long", pybind11::arg("k"));
		cl.def("erase", (void (wxShadowObjectFields::*)(const class wxShadowObjectFields_wxImplementation_HashTable::iterator &)) &wxShadowObjectFields::erase, "C++: wxShadowObjectFields::erase(const class wxShadowObjectFields_wxImplementation_HashTable::iterator &) --> void", pybind11::arg("it"));
		cl.def("count", (unsigned long (wxShadowObjectFields::*)(const class wxString &)) &wxShadowObjectFields::count, "C++: wxShadowObjectFields::count(const class wxString &) --> unsigned long", pybind11::arg("key"));
		cl.def("assign", (class wxShadowObjectFields & (wxShadowObjectFields::*)(const class wxShadowObjectFields &)) &wxShadowObjectFields::operator=, "C++: wxShadowObjectFields::operator=(const class wxShadowObjectFields &) --> class wxShadowObjectFields &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxShadowObjectFields::Insert_Result file: line:645
			auto & enclosing_class = cl;
			pybind11::class_<wxShadowObjectFields::Insert_Result, std::shared_ptr<wxShadowObjectFields::Insert_Result>> cl(enclosing_class, "Insert_Result", "");
			cl.def( pybind11::init<const class wxShadowObjectFields_wxImplementation_HashTable::iterator &, const bool &>(), pybind11::arg("f"), pybind11::arg("s") );

			cl.def( pybind11::init( [](wxShadowObjectFields::Insert_Result const &o){ return new wxShadowObjectFields::Insert_Result(o); } ) );
			cl.def_readwrite("first", &wxShadowObjectFields::Insert_Result::first);
			cl.def_readwrite("second", &wxShadowObjectFields::Insert_Result::second);
		}

	}
	{ // wxShadowObject file: line:30
		pybind11::class_<wxShadowObject, std::shared_ptr<wxShadowObject>> cl(M(""), "wxShadowObject", "");
		cl.def( pybind11::init( [](){ return new wxShadowObject(); } ) );
		cl.def("InvokeMethod", (bool (wxShadowObject::*)(const class wxString &, void *, void *, int *)) &wxShadowObject::InvokeMethod, "C++: wxShadowObject::InvokeMethod(const class wxString &, void *, void *, int *) --> bool", pybind11::arg("name"), pybind11::arg("window"), pybind11::arg("param"), pybind11::arg("returnValue"));
		cl.def("AddField", [](wxShadowObject &o, const class wxString & a0) -> void { return o.AddField(a0); }, "", pybind11::arg("name"));
		cl.def("AddField", (void (wxShadowObject::*)(const class wxString &, void *)) &wxShadowObject::AddField, "C++: wxShadowObject::AddField(const class wxString &, void *) --> void", pybind11::arg("name"), pybind11::arg("initialValue"));
		cl.def("SetField", (void (wxShadowObject::*)(const class wxString &, void *)) &wxShadowObject::SetField, "C++: wxShadowObject::SetField(const class wxString &, void *) --> void", pybind11::arg("name"), pybind11::arg("value"));
		cl.def("GetField", [](wxShadowObject &o, const class wxString & a0) -> void * { return o.GetField(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("name"));
		cl.def("GetField", (void * (wxShadowObject::*)(const class wxString &, void *)) &wxShadowObject::GetField, "C++: wxShadowObject::GetField(const class wxString &, void *) --> void *", pybind11::return_value_policy::automatic, pybind11::arg("name"), pybind11::arg("defaultValue"));
	}
	// wxClientDataType file: line:90
	pybind11::enum_<wxClientDataType>(M(""), "wxClientDataType", pybind11::arithmetic(), "")
		.value("wxClientData_None", wxClientData_None)
		.value("wxClientData_Object", wxClientData_Object)
		.value("wxClientData_Void", wxClientData_Void)
		.export_values();

;

	{ // wxClientData file: line:97
		pybind11::class_<wxClientData, std::shared_ptr<wxClientData>> cl(M(""), "wxClientData", "");
		cl.def( pybind11::init( [](){ return new wxClientData(); } ) );
		cl.def( pybind11::init( [](wxClientData const &o){ return new wxClientData(o); } ) );
		cl.def("assign", (class wxClientData & (wxClientData::*)(const class wxClientData &)) &wxClientData::operator=, "C++: wxClientData::operator=(const class wxClientData &) --> class wxClientData &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxStringClientData file: line:104
		pybind11::class_<wxStringClientData, std::shared_ptr<wxStringClientData>, wxClientData> cl(M(""), "wxStringClientData", "");
		cl.def( pybind11::init( [](){ return new wxStringClientData(); } ) );
		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("data") );

		cl.def( pybind11::init( [](wxStringClientData const &o){ return new wxStringClientData(o); } ) );
		cl.def("SetData", (void (wxStringClientData::*)(const class wxString &)) &wxStringClientData::SetData, "C++: wxStringClientData::SetData(const class wxString &) --> void", pybind11::arg("data"));
		cl.def("GetData", (const class wxString & (wxStringClientData::*)() const) &wxStringClientData::GetData, "C++: wxStringClientData::GetData() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxStringClientData & (wxStringClientData::*)(const class wxStringClientData &)) &wxStringClientData::operator=, "C++: wxStringClientData::operator=(const class wxStringClientData &) --> class wxStringClientData &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxClientDataContainer file: line:125
		pybind11::class_<wxClientDataContainer, std::shared_ptr<wxClientDataContainer>, PyCallBack_wxClientDataContainer> cl(M(""), "wxClientDataContainer", "");
		cl.def( pybind11::init( [](){ return new wxClientDataContainer(); }, [](){ return new PyCallBack_wxClientDataContainer(); } ) );
		cl.def("SetClientObject", (void (wxClientDataContainer::*)(class wxClientData *)) &wxClientDataContainer::SetClientObject, "C++: wxClientDataContainer::SetClientObject(class wxClientData *) --> void", pybind11::arg("data"));
		cl.def("GetClientObject", (class wxClientData * (wxClientDataContainer::*)() const) &wxClientDataContainer::GetClientObject, "C++: wxClientDataContainer::GetClientObject() const --> class wxClientData *", pybind11::return_value_policy::automatic);
		cl.def("SetClientData", (void (wxClientDataContainer::*)(void *)) &wxClientDataContainer::SetClientData, "C++: wxClientDataContainer::SetClientData(void *) --> void", pybind11::arg("data"));
		cl.def("GetClientData", (void * (wxClientDataContainer::*)() const) &wxClientDataContainer::GetClientData, "C++: wxClientDataContainer::GetClientData() const --> void *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxClientDataContainer & (wxClientDataContainer::*)(const class wxClientDataContainer &)) &wxClientDataContainer::operator=, "C++: wxClientDataContainer::operator=(const class wxClientDataContainer &) --> class wxClientDataContainer &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxScopeGuardImplBase file: line:92
		pybind11::class_<wxScopeGuardImplBase, wxScopeGuardImplBase*> cl(M(""), "wxScopeGuardImplBase", "");
		cl.def( pybind11::init( [](){ return new wxScopeGuardImplBase(); } ) );
		cl.def( pybind11::init( [](wxScopeGuardImplBase const &o){ return new wxScopeGuardImplBase(o); } ) );
		cl.def("Dismiss", (void (wxScopeGuardImplBase::*)() const) &wxScopeGuardImplBase::Dismiss, "C++: wxScopeGuardImplBase::Dismiss() const --> void");
		cl.def("WasDismissed", (bool (wxScopeGuardImplBase::*)() const) &wxScopeGuardImplBase::WasDismissed, "C++: wxScopeGuardImplBase::WasDismissed() const --> bool");
	}
}

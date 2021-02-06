#include <common.h> // TimestampDir
#include <eda_units.h> // EDA_UNITS
#include <functional> // std::function
#include <inspectable.h> // INSPECTABLE
#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/propgrid/property.h> // wxPGChoices

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_common(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// TimestampDir(const class wxString &, const class wxString &) file:common.h line:173
	M("").def("TimestampDir", (long long (*)(const class wxString &, const class wxString &)) &TimestampDir, "C++: TimestampDir(const class wxString &, const class wxString &) --> long long", pybind11::arg("aDirPath"), pybind11::arg("aFilespec"));

	{ // PROPERTY_MANAGER file: line:63
		pybind11::class_<PROPERTY_MANAGER, std::shared_ptr<PROPERTY_MANAGER>> cl(M(""), "PROPERTY_MANAGER", "Provide class metadata. Each class handled by PROPERTY_MANAGER\n needs to be described using AddProperty(), AddTypeCast() and InheritsAfter() methods.\n\n Enum types use a dedicated property type (PROPERTY_ENUM), define its possible values\n with ENUM_MAP class, then describe the type using macros:\n - DECLARE_ENUM_TO_WXANY (in header files)\n - IMPLEMENT_ENUM_TO_WXANY (in source files)\n - ENUM_TO_WXANY (*most often used*; combines DECLARE and IMPLEMENT macros,\n   if there is no need to share the description using header files)\n\n Once all classes are described, the property list must be build using\n Rebuild() method.");
		cl.def( pybind11::init( [](PROPERTY_MANAGER const &o){ return new PROPERTY_MANAGER(o); } ) );
		cl.def_static("Instance", (class PROPERTY_MANAGER & (*)()) &PROPERTY_MANAGER::Instance, "C++: PROPERTY_MANAGER::Instance() --> class PROPERTY_MANAGER &", pybind11::return_value_policy::automatic);
		cl.def("RegisterType", (void (PROPERTY_MANAGER::*)(unsigned long, const class wxString &)) &PROPERTY_MANAGER::RegisterType, "Associate a name with a type.\n\n Build a map to provide faster type look-up.\n\n \n is the type identifier (obtained using TYPE_HASH()).\n \n\n is the type name.\n\nC++: PROPERTY_MANAGER::RegisterType(unsigned long, const class wxString &) --> void", pybind11::arg("aType"), pybind11::arg("aName"));
		cl.def("ResolveType", (const class wxString & (PROPERTY_MANAGER::*)(unsigned long) const) &PROPERTY_MANAGER::ResolveType, "Return name of a type.\n\n \n is the type identifier (obtained using TYPE_HASH()).\n \n\n Name of the type or empty string, if not available.\n\nC++: PROPERTY_MANAGER::ResolveType(unsigned long) const --> const class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("aType"));
		cl.def("GetProperty", (class PROPERTY_BASE * (PROPERTY_MANAGER::*)(unsigned long, const class wxString &) const) &PROPERTY_MANAGER::GetProperty, "Return a property for a specific type.\n\n \n is the type identifier (obtained using TYPE_HASH()).\n \n\n is the property name used during class registration.\n \n\n Requested property or null pointer if requested property does not exist.\n\nC++: PROPERTY_MANAGER::GetProperty(unsigned long, const class wxString &) const --> class PROPERTY_BASE *", pybind11::return_value_policy::automatic, pybind11::arg("aType"), pybind11::arg("aProperty"));
		cl.def("GetProperties", (const int & (PROPERTY_MANAGER::*)(unsigned long) const) &PROPERTY_MANAGER::GetProperties, "Return all properties for a specific type.\n\n \n is the type identifier (obtained using TYPE_HASH()).\n \n\n Vector storing all properties of the requested type.\n\nC++: PROPERTY_MANAGER::GetProperties(unsigned long) const --> const int &", pybind11::return_value_policy::automatic, pybind11::arg("aType"));
		cl.def("TypeCast", (const void * (PROPERTY_MANAGER::*)(const void *, unsigned long, unsigned long) const) &PROPERTY_MANAGER::TypeCast, "Cast a type to another type. Used for correct type-casting of types with\n multi-inheritance. Requires registration of an appropriate converter (AddTypeCast).\n\n \n is a pointer to the casted object.\n \n\n is aSource type identifier (obtained using TYPE_HASH()).\n \n\n is the desired type identifier (obtained using TYPE_HASH()).\n \n\n Properly casted pointer of aTarget type.     *\n\n \n AddTypeCast\n\nC++: PROPERTY_MANAGER::TypeCast(const void *, unsigned long, unsigned long) const --> const void *", pybind11::return_value_policy::automatic, pybind11::arg("aSource"), pybind11::arg("aBase"), pybind11::arg("aTarget"));
		cl.def("TypeCast", (void * (PROPERTY_MANAGER::*)(void *, unsigned long, unsigned long) const) &PROPERTY_MANAGER::TypeCast, "C++: PROPERTY_MANAGER::TypeCast(void *, unsigned long, unsigned long) const --> void *", pybind11::return_value_policy::automatic, pybind11::arg("aSource"), pybind11::arg("aBase"), pybind11::arg("aTarget"));
		cl.def("AddProperty", (void (PROPERTY_MANAGER::*)(class PROPERTY_BASE *)) &PROPERTY_MANAGER::AddProperty, "Register a property.\n\n \n is the property to register.\n\nC++: PROPERTY_MANAGER::AddProperty(class PROPERTY_BASE *) --> void", pybind11::arg("aProperty"));
		cl.def("ReplaceProperty", (void (PROPERTY_MANAGER::*)(unsigned long, const class wxString &, class PROPERTY_BASE *)) &PROPERTY_MANAGER::ReplaceProperty, "Replace an existing property for a specific type.\n\n It is used to modify a property that has been inherited from a base class.\n This method is used instead of AddProperty().\n\n \n is the base class type the delivers the original property.\n \n\n is the name of the replaced property.\n \n\n is the property replacing the inherited one.\n\nC++: PROPERTY_MANAGER::ReplaceProperty(unsigned long, const class wxString &, class PROPERTY_BASE *) --> void", pybind11::arg("aBase"), pybind11::arg("aName"), pybind11::arg("aNew"));
		cl.def("AddTypeCast", (void (PROPERTY_MANAGER::*)(class TYPE_CAST_BASE *)) &PROPERTY_MANAGER::AddTypeCast, "Register a type converter. Required prior TypeCast() usage.\n\n \n is the type converter to register.\n\nC++: PROPERTY_MANAGER::AddTypeCast(class TYPE_CAST_BASE *) --> void", pybind11::arg("aCast"));
		cl.def("InheritsAfter", (void (PROPERTY_MANAGER::*)(unsigned long, unsigned long)) &PROPERTY_MANAGER::InheritsAfter, "Declare an inheritance relationship between types.\n\n \n is the base type identifier (obtained using TYPE_HASH()).\n \n\n is the derived type identifier (obtained using TYPE_HASH()).\n\nC++: PROPERTY_MANAGER::InheritsAfter(unsigned long, unsigned long) --> void", pybind11::arg("aDerived"), pybind11::arg("aBase"));
		cl.def("IsOfType", (bool (PROPERTY_MANAGER::*)(unsigned long, unsigned long) const) &PROPERTY_MANAGER::IsOfType, "Return true if aDerived is inherited from aBase.\n\nC++: PROPERTY_MANAGER::IsOfType(unsigned long, unsigned long) const --> bool", pybind11::arg("aDerived"), pybind11::arg("aBase"));
		cl.def("GetUnits", (enum EDA_UNITS (PROPERTY_MANAGER::*)() const) &PROPERTY_MANAGER::GetUnits, "C++: PROPERTY_MANAGER::GetUnits() const --> enum EDA_UNITS");
		cl.def("SetUnits", (void (PROPERTY_MANAGER::*)(enum EDA_UNITS)) &PROPERTY_MANAGER::SetUnits, "C++: PROPERTY_MANAGER::SetUnits(enum EDA_UNITS) --> void", pybind11::arg("aUnits"));
		cl.def("Rebuild", (void (PROPERTY_MANAGER::*)()) &PROPERTY_MANAGER::Rebuild, "Rebuild the list of all registered properties. Needs to be called\n once before GetProperty()/GetProperties() are used.\n\nC++: PROPERTY_MANAGER::Rebuild() --> void");
		cl.def("GetAllClasses", (int (PROPERTY_MANAGER::*)()) &PROPERTY_MANAGER::GetAllClasses, "C++: PROPERTY_MANAGER::GetAllClasses() --> int");
		cl.def("GetMatchingClasses", (int (PROPERTY_MANAGER::*)(class PROPERTY_BASE *)) &PROPERTY_MANAGER::GetMatchingClasses, "C++: PROPERTY_MANAGER::GetMatchingClasses(class PROPERTY_BASE *) --> int", pybind11::arg("aProperty"));

		{ // PROPERTY_MANAGER::CLASS_INFO file: line:180
			auto & enclosing_class = cl;
			pybind11::class_<PROPERTY_MANAGER::CLASS_INFO, std::shared_ptr<PROPERTY_MANAGER::CLASS_INFO>> cl(enclosing_class, "CLASS_INFO", "");
			cl.def( pybind11::init( [](){ return new PROPERTY_MANAGER::CLASS_INFO(); } ) );
			cl.def( pybind11::init( [](PROPERTY_MANAGER::CLASS_INFO const &o){ return new PROPERTY_MANAGER::CLASS_INFO(o); } ) );
			cl.def_readwrite("name", &PROPERTY_MANAGER::CLASS_INFO::name);
			cl.def_readwrite("type", &PROPERTY_MANAGER::CLASS_INFO::type);
			cl.def_readwrite("properties", &PROPERTY_MANAGER::CLASS_INFO::properties);
			cl.def("assign", (struct PROPERTY_MANAGER::CLASS_INFO & (PROPERTY_MANAGER::CLASS_INFO::*)(const struct PROPERTY_MANAGER::CLASS_INFO &)) &PROPERTY_MANAGER::CLASS_INFO::operator=, "C++: PROPERTY_MANAGER::CLASS_INFO::operator=(const struct PROPERTY_MANAGER::CLASS_INFO &) --> struct PROPERTY_MANAGER::CLASS_INFO &", pybind11::return_value_policy::automatic, pybind11::arg(""));
		}

		{ // PROPERTY_MANAGER::CLASS_DESC file: line:200
			auto & enclosing_class = cl;
			pybind11::class_<PROPERTY_MANAGER::CLASS_DESC, std::shared_ptr<PROPERTY_MANAGER::CLASS_DESC>> cl(enclosing_class, "CLASS_DESC", "");
			cl.def( pybind11::init<unsigned long>(), pybind11::arg("aId") );

			cl.def_readonly("m_id", &PROPERTY_MANAGER::CLASS_DESC::m_id);
			cl.def_readwrite("m_bases", &PROPERTY_MANAGER::CLASS_DESC::m_bases);
			cl.def_readwrite("m_ownProperties", &PROPERTY_MANAGER::CLASS_DESC::m_ownProperties);
			cl.def_readwrite("m_typeCasts", &PROPERTY_MANAGER::CLASS_DESC::m_typeCasts);
			cl.def_readwrite("m_allProperties", &PROPERTY_MANAGER::CLASS_DESC::m_allProperties);
			cl.def_readwrite("m_replaced", &PROPERTY_MANAGER::CLASS_DESC::m_replaced);
			cl.def("rebuild", (void (PROPERTY_MANAGER::CLASS_DESC::*)()) &PROPERTY_MANAGER::CLASS_DESC::rebuild, "C++: PROPERTY_MANAGER::CLASS_DESC::rebuild() --> void");
			cl.def("collectPropsRecur", (void (PROPERTY_MANAGER::CLASS_DESC::*)(int &, int &) const) &PROPERTY_MANAGER::CLASS_DESC::collectPropsRecur, "C++: PROPERTY_MANAGER::CLASS_DESC::collectPropsRecur(int &, int &) const --> void", pybind11::arg("aResult"), pybind11::arg("aReplaced"));
		}

	}
	{ // wxAssert_wxArrayPGProperty file: line:377
		pybind11::class_<wxAssert_wxArrayPGProperty, std::shared_ptr<wxAssert_wxArrayPGProperty>> cl(M(""), "wxAssert_wxArrayPGProperty", "");
		cl.def( pybind11::init( [](){ return new wxAssert_wxArrayPGProperty(); } ) );
		cl.def_readwrite("TypeTooBigToBeStoredInwxBaseArrayPtrVoid", &wxAssert_wxArrayPGProperty::TypeTooBigToBeStoredInwxBaseArrayPtrVoid);
	}
	{ // wxArrayPGProperty file: line:295
		pybind11::class_<wxArrayPGProperty, std::shared_ptr<wxArrayPGProperty>, wxBaseArrayPtrVoid> cl(M(""), "wxArrayPGProperty", "");
		cl.def( pybind11::init( [](){ return new wxArrayPGProperty(); } ) );
		cl.def( pybind11::init<unsigned long>(), pybind11::arg("n") );

		cl.def( pybind11::init<unsigned long, class wxPGProperty *const &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init( [](wxArrayPGProperty const &o){ return new wxArrayPGProperty(o); } ) );
		cl.def("__getitem__", (class wxPGProperty *& (wxArrayPGProperty::*)(unsigned long) const) &wxArrayPGProperty::operator[], "C++: wxArrayPGProperty::operator[](unsigned long) const --> class wxPGProperty *&", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Item", (class wxPGProperty *& (wxArrayPGProperty::*)(unsigned long) const) &wxArrayPGProperty::Item, "C++: wxArrayPGProperty::Item(unsigned long) const --> class wxPGProperty *&", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Last", (class wxPGProperty *& (wxArrayPGProperty::*)() const) &wxArrayPGProperty::Last, "C++: wxArrayPGProperty::Last() const --> class wxPGProperty *&", pybind11::return_value_policy::automatic);
		cl.def("Index", [](wxArrayPGProperty const &o, class wxPGProperty * a0) -> int { return o.Index(a0); }, "", pybind11::arg("lItem"));
		cl.def("Index", (int (wxArrayPGProperty::*)(class wxPGProperty *, bool) const) &wxArrayPGProperty::Index, "C++: wxArrayPGProperty::Index(class wxPGProperty *, bool) const --> int", pybind11::arg("lItem"), pybind11::arg("bFromEnd"));
		cl.def("Add", [](wxArrayPGProperty &o, class wxPGProperty * a0) -> void { return o.Add(a0); }, "", pybind11::arg("lItem"));
		cl.def("Add", (void (wxArrayPGProperty::*)(class wxPGProperty *, unsigned long)) &wxArrayPGProperty::Add, "C++: wxArrayPGProperty::Add(class wxPGProperty *, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("nInsert"));
		cl.def("Insert", [](wxArrayPGProperty &o, class wxPGProperty * a0, unsigned long const & a1) -> void { return o.Insert(a0, a1); }, "", pybind11::arg("lItem"), pybind11::arg("uiIndex"));
		cl.def("Insert", (void (wxArrayPGProperty::*)(class wxPGProperty *, unsigned long, unsigned long)) &wxArrayPGProperty::Insert, "C++: wxArrayPGProperty::Insert(class wxPGProperty *, unsigned long, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("uiIndex"), pybind11::arg("nInsert"));
		cl.def("RemoveAt", [](wxArrayPGProperty &o, unsigned long const & a0) -> void { return o.RemoveAt(a0); }, "", pybind11::arg("uiIndex"));
		cl.def("RemoveAt", (void (wxArrayPGProperty::*)(unsigned long, unsigned long)) &wxArrayPGProperty::RemoveAt, "C++: wxArrayPGProperty::RemoveAt(unsigned long, unsigned long) --> void", pybind11::arg("uiIndex"), pybind11::arg("nRemove"));
		cl.def("Remove", (void (wxArrayPGProperty::*)(class wxPGProperty *)) &wxArrayPGProperty::Remove, "C++: wxArrayPGProperty::Remove(class wxPGProperty *) --> void", pybind11::arg("lItem"));
		cl.def("assign", (void (wxArrayPGProperty::*)(unsigned long, class wxPGProperty *const &)) &wxArrayPGProperty::assign, "C++: wxArrayPGProperty::assign(unsigned long, class wxPGProperty *const &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("back", (class wxPGProperty *& (wxArrayPGProperty::*)()) &wxArrayPGProperty::back, "C++: wxArrayPGProperty::back() --> class wxPGProperty *&", pybind11::return_value_policy::automatic);
		cl.def("begin", (class wxPGProperty ** (wxArrayPGProperty::*)()) &wxArrayPGProperty::begin, "C++: wxArrayPGProperty::begin() --> class wxPGProperty **", pybind11::return_value_policy::automatic);
		cl.def("capacity", (unsigned long (wxArrayPGProperty::*)() const) &wxArrayPGProperty::capacity, "C++: wxArrayPGProperty::capacity() const --> unsigned long");
		cl.def("end", (class wxPGProperty ** (wxArrayPGProperty::*)()) &wxArrayPGProperty::end, "C++: wxArrayPGProperty::end() --> class wxPGProperty **", pybind11::return_value_policy::automatic);
		cl.def("front", (class wxPGProperty *& (wxArrayPGProperty::*)()) &wxArrayPGProperty::front, "C++: wxArrayPGProperty::front() --> class wxPGProperty *&", pybind11::return_value_policy::automatic);
		cl.def("pop_back", (void (wxArrayPGProperty::*)()) &wxArrayPGProperty::pop_back, "C++: wxArrayPGProperty::pop_back() --> void");
		cl.def("push_back", (void (wxArrayPGProperty::*)(class wxPGProperty *const &)) &wxArrayPGProperty::push_back, "C++: wxArrayPGProperty::push_back(class wxPGProperty *const &) --> void", pybind11::arg("v"));
		cl.def("rbegin", (class wxArrayPGProperty::reverse_iterator (wxArrayPGProperty::*)()) &wxArrayPGProperty::rbegin, "C++: wxArrayPGProperty::rbegin() --> class wxArrayPGProperty::reverse_iterator");
		cl.def("rend", (class wxArrayPGProperty::reverse_iterator (wxArrayPGProperty::*)()) &wxArrayPGProperty::rend, "C++: wxArrayPGProperty::rend() --> class wxArrayPGProperty::reverse_iterator");
		cl.def("reserve", (void (wxArrayPGProperty::*)(unsigned long)) &wxArrayPGProperty::reserve, "C++: wxArrayPGProperty::reserve(unsigned long) --> void", pybind11::arg("n"));
		cl.def("resize", [](wxArrayPGProperty &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxArrayPGProperty::*)(unsigned long, class wxPGProperty *)) &wxArrayPGProperty::resize, "C++: wxArrayPGProperty::resize(unsigned long, class wxPGProperty *) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("swap", (void (wxArrayPGProperty::*)(class wxArrayPGProperty &)) &wxArrayPGProperty::swap, "C++: wxArrayPGProperty::swap(class wxArrayPGProperty &) --> void", pybind11::arg("other"));
		cl.def("assign", (class wxArrayPGProperty & (wxArrayPGProperty::*)(const class wxArrayPGProperty &)) &wxArrayPGProperty::operator=, "C++: wxArrayPGProperty::operator=(const class wxArrayPGProperty &) --> class wxArrayPGProperty &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxArrayPGProperty::reverse_iterator file: line:400
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayPGProperty::reverse_iterator, std::shared_ptr<wxArrayPGProperty::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayPGProperty::reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxArrayPGProperty::reverse_iterator const &o){ return new wxArrayPGProperty::reverse_iterator(o); } ) );
			cl.def("__mul__", (class wxPGProperty *& (wxArrayPGProperty::reverse_iterator::*)() const) &wxArrayPGProperty::reverse_iterator::operator*, "C++: wxArrayPGProperty::reverse_iterator::operator*() const --> class wxPGProperty *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayPGProperty::reverse_iterator & (wxArrayPGProperty::reverse_iterator::*)()) &wxArrayPGProperty::reverse_iterator::operator++, "C++: wxArrayPGProperty::reverse_iterator::operator++() --> class wxArrayPGProperty::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayPGProperty::reverse_iterator (wxArrayPGProperty::reverse_iterator::*)(int)) &wxArrayPGProperty::reverse_iterator::operator++, "C++: wxArrayPGProperty::reverse_iterator::operator++(int) --> const class wxArrayPGProperty::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayPGProperty::reverse_iterator & (wxArrayPGProperty::reverse_iterator::*)()) &wxArrayPGProperty::reverse_iterator::operator--, "C++: wxArrayPGProperty::reverse_iterator::operator--() --> class wxArrayPGProperty::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayPGProperty::reverse_iterator (wxArrayPGProperty::reverse_iterator::*)(int)) &wxArrayPGProperty::reverse_iterator::operator--, "C++: wxArrayPGProperty::reverse_iterator::operator--(int) --> const class wxArrayPGProperty::reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayPGProperty::reverse_iterator::*)(const class wxArrayPGProperty::reverse_iterator &) const) &wxArrayPGProperty::reverse_iterator::operator==, "C++: wxArrayPGProperty::reverse_iterator::operator==(const class wxArrayPGProperty::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayPGProperty::reverse_iterator::*)(const class wxArrayPGProperty::reverse_iterator &) const) &wxArrayPGProperty::reverse_iterator::operator!=, "C++: wxArrayPGProperty::reverse_iterator::operator!=(const class wxArrayPGProperty::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxArrayPGProperty::const_reverse_iterator file: line:432
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayPGProperty::const_reverse_iterator, std::shared_ptr<wxArrayPGProperty::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayPGProperty::const_reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxArrayPGProperty::const_reverse_iterator const &o){ return new wxArrayPGProperty::const_reverse_iterator(o); } ) );
			cl.def( pybind11::init<const class wxArrayPGProperty::reverse_iterator &>(), pybind11::arg("it") );

			cl.def("__mul__", (class wxPGProperty *const & (wxArrayPGProperty::const_reverse_iterator::*)() const) &wxArrayPGProperty::const_reverse_iterator::operator*, "C++: wxArrayPGProperty::const_reverse_iterator::operator*() const --> class wxPGProperty *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayPGProperty::const_reverse_iterator & (wxArrayPGProperty::const_reverse_iterator::*)()) &wxArrayPGProperty::const_reverse_iterator::operator++, "C++: wxArrayPGProperty::const_reverse_iterator::operator++() --> class wxArrayPGProperty::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayPGProperty::const_reverse_iterator (wxArrayPGProperty::const_reverse_iterator::*)(int)) &wxArrayPGProperty::const_reverse_iterator::operator++, "C++: wxArrayPGProperty::const_reverse_iterator::operator++(int) --> const class wxArrayPGProperty::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayPGProperty::const_reverse_iterator & (wxArrayPGProperty::const_reverse_iterator::*)()) &wxArrayPGProperty::const_reverse_iterator::operator--, "C++: wxArrayPGProperty::const_reverse_iterator::operator--() --> class wxArrayPGProperty::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayPGProperty::const_reverse_iterator (wxArrayPGProperty::const_reverse_iterator::*)(int)) &wxArrayPGProperty::const_reverse_iterator::operator--, "C++: wxArrayPGProperty::const_reverse_iterator::operator--(int) --> const class wxArrayPGProperty::const_reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayPGProperty::const_reverse_iterator::*)(const class wxArrayPGProperty::const_reverse_iterator &) const) &wxArrayPGProperty::const_reverse_iterator::operator==, "C++: wxArrayPGProperty::const_reverse_iterator::operator==(const class wxArrayPGProperty::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayPGProperty::const_reverse_iterator::*)(const class wxArrayPGProperty::const_reverse_iterator &) const) &wxArrayPGProperty::const_reverse_iterator::operator!=, "C++: wxArrayPGProperty::const_reverse_iterator::operator!=(const class wxArrayPGProperty::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
}

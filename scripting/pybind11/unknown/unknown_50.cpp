#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <time.h> // tm

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxDateTimeHolidayAuthority file: line:1554
struct PyCallBack_wxDateTimeHolidayAuthority : public wxDateTimeHolidayAuthority {
	using wxDateTimeHolidayAuthority::wxDateTimeHolidayAuthority;

	bool DoIsHoliday(const class wxDateTime & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDateTimeHolidayAuthority *>(this), "DoIsHoliday");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxDateTimeHolidayAuthority::DoIsHoliday\"");
	}
	unsigned long DoGetHolidaysInRange(const class wxDateTime & a0, const class wxDateTime & a1, class wxDateTimeArray & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDateTimeHolidayAuthority *>(this), "DoGetHolidaysInRange");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxDateTimeHolidayAuthority::DoGetHolidaysInRange\"");
	}
};

// wxDateTimeWorkDays file: line:1599
struct PyCallBack_wxDateTimeWorkDays : public wxDateTimeWorkDays {
	using wxDateTimeWorkDays::wxDateTimeWorkDays;

	bool DoIsHoliday(const class wxDateTime & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDateTimeWorkDays *>(this), "DoIsHoliday");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxDateTimeWorkDays::DoIsHoliday(a0);
	}
	unsigned long DoGetHolidaysInRange(const class wxDateTime & a0, const class wxDateTime & a1, class wxDateTimeArray & a2) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDateTimeWorkDays *>(this), "DoGetHolidaysInRange");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxDateTimeWorkDays::DoGetHolidaysInRange(a0, a1, a2);
	}
};

void bind_unknown_unknown_50(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxAssert_wxHolidayAuthoritiesArray file: line:141
		pybind11::class_<wxAssert_wxHolidayAuthoritiesArray, std::shared_ptr<wxAssert_wxHolidayAuthoritiesArray>> cl(M(""), "wxAssert_wxHolidayAuthoritiesArray", "");
		cl.def( pybind11::init( [](){ return new wxAssert_wxHolidayAuthoritiesArray(); } ) );
		cl.def_readwrite("TypeTooBigToBeStoredInwxBaseArrayPtrVoid", &wxAssert_wxHolidayAuthoritiesArray::TypeTooBigToBeStoredInwxBaseArrayPtrVoid);
	}
	{ // wxHolidayAuthoritiesArray file: line:1550
		pybind11::class_<wxHolidayAuthoritiesArray, std::shared_ptr<wxHolidayAuthoritiesArray>, wxBaseArrayPtrVoid> cl(M(""), "wxHolidayAuthoritiesArray", "");
		cl.def( pybind11::init( [](){ return new wxHolidayAuthoritiesArray(); } ) );
		cl.def( pybind11::init<unsigned long>(), pybind11::arg("n") );

		cl.def( pybind11::init<unsigned long, class wxDateTimeHolidayAuthority *const &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init( [](wxHolidayAuthoritiesArray const &o){ return new wxHolidayAuthoritiesArray(o); } ) );
		cl.def("__getitem__", (class wxDateTimeHolidayAuthority *& (wxHolidayAuthoritiesArray::*)(unsigned long) const) &wxHolidayAuthoritiesArray::operator[], "C++: wxHolidayAuthoritiesArray::operator[](unsigned long) const --> class wxDateTimeHolidayAuthority *&", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Item", (class wxDateTimeHolidayAuthority *& (wxHolidayAuthoritiesArray::*)(unsigned long) const) &wxHolidayAuthoritiesArray::Item, "C++: wxHolidayAuthoritiesArray::Item(unsigned long) const --> class wxDateTimeHolidayAuthority *&", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Last", (class wxDateTimeHolidayAuthority *& (wxHolidayAuthoritiesArray::*)() const) &wxHolidayAuthoritiesArray::Last, "C++: wxHolidayAuthoritiesArray::Last() const --> class wxDateTimeHolidayAuthority *&", pybind11::return_value_policy::automatic);
		cl.def("Index", [](wxHolidayAuthoritiesArray const &o, class wxDateTimeHolidayAuthority * a0) -> int { return o.Index(a0); }, "", pybind11::arg("lItem"));
		cl.def("Index", (int (wxHolidayAuthoritiesArray::*)(class wxDateTimeHolidayAuthority *, bool) const) &wxHolidayAuthoritiesArray::Index, "C++: wxHolidayAuthoritiesArray::Index(class wxDateTimeHolidayAuthority *, bool) const --> int", pybind11::arg("lItem"), pybind11::arg("bFromEnd"));
		cl.def("Add", [](wxHolidayAuthoritiesArray &o, class wxDateTimeHolidayAuthority * a0) -> void { return o.Add(a0); }, "", pybind11::arg("lItem"));
		cl.def("Add", (void (wxHolidayAuthoritiesArray::*)(class wxDateTimeHolidayAuthority *, unsigned long)) &wxHolidayAuthoritiesArray::Add, "C++: wxHolidayAuthoritiesArray::Add(class wxDateTimeHolidayAuthority *, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("nInsert"));
		cl.def("Insert", [](wxHolidayAuthoritiesArray &o, class wxDateTimeHolidayAuthority * a0, unsigned long const & a1) -> void { return o.Insert(a0, a1); }, "", pybind11::arg("lItem"), pybind11::arg("uiIndex"));
		cl.def("Insert", (void (wxHolidayAuthoritiesArray::*)(class wxDateTimeHolidayAuthority *, unsigned long, unsigned long)) &wxHolidayAuthoritiesArray::Insert, "C++: wxHolidayAuthoritiesArray::Insert(class wxDateTimeHolidayAuthority *, unsigned long, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("uiIndex"), pybind11::arg("nInsert"));
		cl.def("RemoveAt", [](wxHolidayAuthoritiesArray &o, unsigned long const & a0) -> void { return o.RemoveAt(a0); }, "", pybind11::arg("uiIndex"));
		cl.def("RemoveAt", (void (wxHolidayAuthoritiesArray::*)(unsigned long, unsigned long)) &wxHolidayAuthoritiesArray::RemoveAt, "C++: wxHolidayAuthoritiesArray::RemoveAt(unsigned long, unsigned long) --> void", pybind11::arg("uiIndex"), pybind11::arg("nRemove"));
		cl.def("Remove", (void (wxHolidayAuthoritiesArray::*)(class wxDateTimeHolidayAuthority *)) &wxHolidayAuthoritiesArray::Remove, "C++: wxHolidayAuthoritiesArray::Remove(class wxDateTimeHolidayAuthority *) --> void", pybind11::arg("lItem"));
		cl.def("assign", (void (wxHolidayAuthoritiesArray::*)(unsigned long, class wxDateTimeHolidayAuthority *const &)) &wxHolidayAuthoritiesArray::assign, "C++: wxHolidayAuthoritiesArray::assign(unsigned long, class wxDateTimeHolidayAuthority *const &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("back", (class wxDateTimeHolidayAuthority *& (wxHolidayAuthoritiesArray::*)()) &wxHolidayAuthoritiesArray::back, "C++: wxHolidayAuthoritiesArray::back() --> class wxDateTimeHolidayAuthority *&", pybind11::return_value_policy::automatic);
		cl.def("begin", (class wxDateTimeHolidayAuthority ** (wxHolidayAuthoritiesArray::*)()) &wxHolidayAuthoritiesArray::begin, "C++: wxHolidayAuthoritiesArray::begin() --> class wxDateTimeHolidayAuthority **", pybind11::return_value_policy::automatic);
		cl.def("capacity", (unsigned long (wxHolidayAuthoritiesArray::*)() const) &wxHolidayAuthoritiesArray::capacity, "C++: wxHolidayAuthoritiesArray::capacity() const --> unsigned long");
		cl.def("end", (class wxDateTimeHolidayAuthority ** (wxHolidayAuthoritiesArray::*)()) &wxHolidayAuthoritiesArray::end, "C++: wxHolidayAuthoritiesArray::end() --> class wxDateTimeHolidayAuthority **", pybind11::return_value_policy::automatic);
		cl.def("front", (class wxDateTimeHolidayAuthority *& (wxHolidayAuthoritiesArray::*)()) &wxHolidayAuthoritiesArray::front, "C++: wxHolidayAuthoritiesArray::front() --> class wxDateTimeHolidayAuthority *&", pybind11::return_value_policy::automatic);
		cl.def("pop_back", (void (wxHolidayAuthoritiesArray::*)()) &wxHolidayAuthoritiesArray::pop_back, "C++: wxHolidayAuthoritiesArray::pop_back() --> void");
		cl.def("push_back", (void (wxHolidayAuthoritiesArray::*)(class wxDateTimeHolidayAuthority *const &)) &wxHolidayAuthoritiesArray::push_back, "C++: wxHolidayAuthoritiesArray::push_back(class wxDateTimeHolidayAuthority *const &) --> void", pybind11::arg("v"));
		cl.def("rbegin", (class wxHolidayAuthoritiesArray::reverse_iterator (wxHolidayAuthoritiesArray::*)()) &wxHolidayAuthoritiesArray::rbegin, "C++: wxHolidayAuthoritiesArray::rbegin() --> class wxHolidayAuthoritiesArray::reverse_iterator");
		cl.def("rend", (class wxHolidayAuthoritiesArray::reverse_iterator (wxHolidayAuthoritiesArray::*)()) &wxHolidayAuthoritiesArray::rend, "C++: wxHolidayAuthoritiesArray::rend() --> class wxHolidayAuthoritiesArray::reverse_iterator");
		cl.def("reserve", (void (wxHolidayAuthoritiesArray::*)(unsigned long)) &wxHolidayAuthoritiesArray::reserve, "C++: wxHolidayAuthoritiesArray::reserve(unsigned long) --> void", pybind11::arg("n"));
		cl.def("resize", [](wxHolidayAuthoritiesArray &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxHolidayAuthoritiesArray::*)(unsigned long, class wxDateTimeHolidayAuthority *)) &wxHolidayAuthoritiesArray::resize, "C++: wxHolidayAuthoritiesArray::resize(unsigned long, class wxDateTimeHolidayAuthority *) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("swap", (void (wxHolidayAuthoritiesArray::*)(class wxHolidayAuthoritiesArray &)) &wxHolidayAuthoritiesArray::swap, "C++: wxHolidayAuthoritiesArray::swap(class wxHolidayAuthoritiesArray &) --> void", pybind11::arg("other"));
		cl.def("assign", (class wxHolidayAuthoritiesArray & (wxHolidayAuthoritiesArray::*)(const class wxHolidayAuthoritiesArray &)) &wxHolidayAuthoritiesArray::operator=, "C++: wxHolidayAuthoritiesArray::operator=(const class wxHolidayAuthoritiesArray &) --> class wxHolidayAuthoritiesArray &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxHolidayAuthoritiesArray::reverse_iterator file: line:400
			auto & enclosing_class = cl;
			pybind11::class_<wxHolidayAuthoritiesArray::reverse_iterator, std::shared_ptr<wxHolidayAuthoritiesArray::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxHolidayAuthoritiesArray::reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxHolidayAuthoritiesArray::reverse_iterator const &o){ return new wxHolidayAuthoritiesArray::reverse_iterator(o); } ) );
			cl.def("__mul__", (class wxDateTimeHolidayAuthority *& (wxHolidayAuthoritiesArray::reverse_iterator::*)() const) &wxHolidayAuthoritiesArray::reverse_iterator::operator*, "C++: wxHolidayAuthoritiesArray::reverse_iterator::operator*() const --> class wxDateTimeHolidayAuthority *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxHolidayAuthoritiesArray::reverse_iterator & (wxHolidayAuthoritiesArray::reverse_iterator::*)()) &wxHolidayAuthoritiesArray::reverse_iterator::operator++, "C++: wxHolidayAuthoritiesArray::reverse_iterator::operator++() --> class wxHolidayAuthoritiesArray::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxHolidayAuthoritiesArray::reverse_iterator (wxHolidayAuthoritiesArray::reverse_iterator::*)(int)) &wxHolidayAuthoritiesArray::reverse_iterator::operator++, "C++: wxHolidayAuthoritiesArray::reverse_iterator::operator++(int) --> const class wxHolidayAuthoritiesArray::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxHolidayAuthoritiesArray::reverse_iterator & (wxHolidayAuthoritiesArray::reverse_iterator::*)()) &wxHolidayAuthoritiesArray::reverse_iterator::operator--, "C++: wxHolidayAuthoritiesArray::reverse_iterator::operator--() --> class wxHolidayAuthoritiesArray::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxHolidayAuthoritiesArray::reverse_iterator (wxHolidayAuthoritiesArray::reverse_iterator::*)(int)) &wxHolidayAuthoritiesArray::reverse_iterator::operator--, "C++: wxHolidayAuthoritiesArray::reverse_iterator::operator--(int) --> const class wxHolidayAuthoritiesArray::reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxHolidayAuthoritiesArray::reverse_iterator::*)(const class wxHolidayAuthoritiesArray::reverse_iterator &) const) &wxHolidayAuthoritiesArray::reverse_iterator::operator==, "C++: wxHolidayAuthoritiesArray::reverse_iterator::operator==(const class wxHolidayAuthoritiesArray::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxHolidayAuthoritiesArray::reverse_iterator::*)(const class wxHolidayAuthoritiesArray::reverse_iterator &) const) &wxHolidayAuthoritiesArray::reverse_iterator::operator!=, "C++: wxHolidayAuthoritiesArray::reverse_iterator::operator!=(const class wxHolidayAuthoritiesArray::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxHolidayAuthoritiesArray::const_reverse_iterator file: line:432
			auto & enclosing_class = cl;
			pybind11::class_<wxHolidayAuthoritiesArray::const_reverse_iterator, std::shared_ptr<wxHolidayAuthoritiesArray::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxHolidayAuthoritiesArray::const_reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxHolidayAuthoritiesArray::const_reverse_iterator const &o){ return new wxHolidayAuthoritiesArray::const_reverse_iterator(o); } ) );
			cl.def( pybind11::init<const class wxHolidayAuthoritiesArray::reverse_iterator &>(), pybind11::arg("it") );

			cl.def("__mul__", (class wxDateTimeHolidayAuthority *const & (wxHolidayAuthoritiesArray::const_reverse_iterator::*)() const) &wxHolidayAuthoritiesArray::const_reverse_iterator::operator*, "C++: wxHolidayAuthoritiesArray::const_reverse_iterator::operator*() const --> class wxDateTimeHolidayAuthority *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxHolidayAuthoritiesArray::const_reverse_iterator & (wxHolidayAuthoritiesArray::const_reverse_iterator::*)()) &wxHolidayAuthoritiesArray::const_reverse_iterator::operator++, "C++: wxHolidayAuthoritiesArray::const_reverse_iterator::operator++() --> class wxHolidayAuthoritiesArray::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxHolidayAuthoritiesArray::const_reverse_iterator (wxHolidayAuthoritiesArray::const_reverse_iterator::*)(int)) &wxHolidayAuthoritiesArray::const_reverse_iterator::operator++, "C++: wxHolidayAuthoritiesArray::const_reverse_iterator::operator++(int) --> const class wxHolidayAuthoritiesArray::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxHolidayAuthoritiesArray::const_reverse_iterator & (wxHolidayAuthoritiesArray::const_reverse_iterator::*)()) &wxHolidayAuthoritiesArray::const_reverse_iterator::operator--, "C++: wxHolidayAuthoritiesArray::const_reverse_iterator::operator--() --> class wxHolidayAuthoritiesArray::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxHolidayAuthoritiesArray::const_reverse_iterator (wxHolidayAuthoritiesArray::const_reverse_iterator::*)(int)) &wxHolidayAuthoritiesArray::const_reverse_iterator::operator--, "C++: wxHolidayAuthoritiesArray::const_reverse_iterator::operator--(int) --> const class wxHolidayAuthoritiesArray::const_reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxHolidayAuthoritiesArray::const_reverse_iterator::*)(const class wxHolidayAuthoritiesArray::const_reverse_iterator &) const) &wxHolidayAuthoritiesArray::const_reverse_iterator::operator==, "C++: wxHolidayAuthoritiesArray::const_reverse_iterator::operator==(const class wxHolidayAuthoritiesArray::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxHolidayAuthoritiesArray::const_reverse_iterator::*)(const class wxHolidayAuthoritiesArray::const_reverse_iterator &) const) &wxHolidayAuthoritiesArray::const_reverse_iterator::operator!=, "C++: wxHolidayAuthoritiesArray::const_reverse_iterator::operator!=(const class wxHolidayAuthoritiesArray::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
	{ // wxDateTimeHolidayAuthority file: line:1554
		pybind11::class_<wxDateTimeHolidayAuthority, std::shared_ptr<wxDateTimeHolidayAuthority>, PyCallBack_wxDateTimeHolidayAuthority> cl(M(""), "wxDateTimeHolidayAuthority", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxDateTimeHolidayAuthority(); } ) );
		cl.def(pybind11::init<PyCallBack_wxDateTimeHolidayAuthority const &>());
		cl.def_static("IsHoliday", (bool (*)(const class wxDateTime &)) &wxDateTimeHolidayAuthority::IsHoliday, "C++: wxDateTimeHolidayAuthority::IsHoliday(const class wxDateTime &) --> bool", pybind11::arg("dt"));
		cl.def_static("GetHolidaysInRange", (unsigned long (*)(const class wxDateTime &, const class wxDateTime &, class wxDateTimeArray &)) &wxDateTimeHolidayAuthority::GetHolidaysInRange, "C++: wxDateTimeHolidayAuthority::GetHolidaysInRange(const class wxDateTime &, const class wxDateTime &, class wxDateTimeArray &) --> unsigned long", pybind11::arg("dtStart"), pybind11::arg("dtEnd"), pybind11::arg("holidays"));
		cl.def_static("ClearAllAuthorities", (void (*)()) &wxDateTimeHolidayAuthority::ClearAllAuthorities, "C++: wxDateTimeHolidayAuthority::ClearAllAuthorities() --> void");
		cl.def_static("AddAuthority", (void (*)(class wxDateTimeHolidayAuthority *)) &wxDateTimeHolidayAuthority::AddAuthority, "C++: wxDateTimeHolidayAuthority::AddAuthority(class wxDateTimeHolidayAuthority *) --> void", pybind11::arg("auth"));
		cl.def("assign", (class wxDateTimeHolidayAuthority & (wxDateTimeHolidayAuthority::*)(const class wxDateTimeHolidayAuthority &)) &wxDateTimeHolidayAuthority::operator=, "C++: wxDateTimeHolidayAuthority::operator=(const class wxDateTimeHolidayAuthority &) --> class wxDateTimeHolidayAuthority &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxDateTimeWorkDays file: line:1599
		pybind11::class_<wxDateTimeWorkDays, std::shared_ptr<wxDateTimeWorkDays>, PyCallBack_wxDateTimeWorkDays, wxDateTimeHolidayAuthority> cl(M(""), "wxDateTimeWorkDays", "");
		cl.def( pybind11::init( [](){ return new wxDateTimeWorkDays(); }, [](){ return new PyCallBack_wxDateTimeWorkDays(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxDateTimeWorkDays const &o){ return new PyCallBack_wxDateTimeWorkDays(o); } ) );
		cl.def( pybind11::init( [](wxDateTimeWorkDays const &o){ return new wxDateTimeWorkDays(o); } ) );
		cl.def("assign", (class wxDateTimeWorkDays & (wxDateTimeWorkDays::*)(const class wxDateTimeWorkDays &)) &wxDateTimeWorkDays::operator=, "C++: wxDateTimeWorkDays::operator=(const class wxDateTimeWorkDays &) --> class wxDateTimeWorkDays &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}

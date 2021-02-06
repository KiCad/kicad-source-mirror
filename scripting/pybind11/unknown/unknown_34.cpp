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

void bind_unknown_unknown_34(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxArrayString file: line:120
		pybind11::class_<wxArrayString, std::shared_ptr<wxArrayString>> cl(M(""), "wxArrayString", "");
		cl.def( pybind11::init( [](){ return new wxArrayString(); } ) );
		cl.def( pybind11::init<int>(), pybind11::arg("autoSort") );

		cl.def( pybind11::init<unsigned long, const class wxString *>(), pybind11::arg("sz"), pybind11::arg("a") );

		cl.def( pybind11::init( [](wxArrayString const &o){ return new wxArrayString(o); } ) );
		cl.def( pybind11::init<const class wxString *, const class wxString *>(), pybind11::arg("first"), pybind11::arg("last") );

		cl.def( pybind11::init<unsigned long, const class wxString &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def("assign", (void (wxArrayString::*)(const class wxString *, const class wxString *)) &wxArrayString::assign<const wxString *>, "C++: wxArrayString::assign(const class wxString *, const class wxString *) --> void", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("assign", (class wxArrayString & (wxArrayString::*)(const class wxArrayString &)) &wxArrayString::operator=, "C++: wxArrayString::operator=(const class wxArrayString &) --> class wxArrayString &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("Empty", (void (wxArrayString::*)()) &wxArrayString::Empty, "C++: wxArrayString::Empty() --> void");
		cl.def("Clear", (void (wxArrayString::*)()) &wxArrayString::Clear, "C++: wxArrayString::Clear() --> void");
		cl.def("Alloc", (void (wxArrayString::*)(unsigned long)) &wxArrayString::Alloc, "C++: wxArrayString::Alloc(unsigned long) --> void", pybind11::arg("nCount"));
		cl.def("Shrink", (void (wxArrayString::*)()) &wxArrayString::Shrink, "C++: wxArrayString::Shrink() --> void");
		cl.def("GetCount", (unsigned long (wxArrayString::*)() const) &wxArrayString::GetCount, "C++: wxArrayString::GetCount() const --> unsigned long");
		cl.def("IsEmpty", (bool (wxArrayString::*)() const) &wxArrayString::IsEmpty, "C++: wxArrayString::IsEmpty() const --> bool");
		cl.def("Count", (unsigned long (wxArrayString::*)() const) &wxArrayString::Count, "C++: wxArrayString::Count() const --> unsigned long");
		cl.def("Item", (class wxString & (wxArrayString::*)(unsigned long)) &wxArrayString::Item, "C++: wxArrayString::Item(unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nIndex"));
		cl.def("__getitem__", (class wxString & (wxArrayString::*)(unsigned long)) &wxArrayString::operator[], "C++: wxArrayString::operator[](unsigned long) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("nIndex"));
		cl.def("Last", (class wxString & (wxArrayString::*)()) &wxArrayString::Last, "C++: wxArrayString::Last() --> class wxString &", pybind11::return_value_policy::automatic);
		cl.def("Index", [](wxArrayString const &o, const class wxString & a0) -> int { return o.Index(a0); }, "", pybind11::arg("str"));
		cl.def("Index", [](wxArrayString const &o, const class wxString & a0, bool const & a1) -> int { return o.Index(a0, a1); }, "", pybind11::arg("str"), pybind11::arg("bCase"));
		cl.def("Index", (int (wxArrayString::*)(const class wxString &, bool, bool) const) &wxArrayString::Index, "C++: wxArrayString::Index(const class wxString &, bool, bool) const --> int", pybind11::arg("str"), pybind11::arg("bCase"), pybind11::arg("bFromEnd"));
		cl.def("Add", [](wxArrayString &o, const class wxString & a0) -> unsigned long { return o.Add(a0); }, "", pybind11::arg("str"));
		cl.def("Add", (unsigned long (wxArrayString::*)(const class wxString &, unsigned long)) &wxArrayString::Add, "C++: wxArrayString::Add(const class wxString &, unsigned long) --> unsigned long", pybind11::arg("str"), pybind11::arg("nInsert"));
		cl.def("Insert", [](wxArrayString &o, const class wxString & a0, unsigned long const & a1) -> void { return o.Insert(a0, a1); }, "", pybind11::arg("str"), pybind11::arg("uiIndex"));
		cl.def("Insert", (void (wxArrayString::*)(const class wxString &, unsigned long, unsigned long)) &wxArrayString::Insert, "C++: wxArrayString::Insert(const class wxString &, unsigned long, unsigned long) --> void", pybind11::arg("str"), pybind11::arg("uiIndex"), pybind11::arg("nInsert"));
		cl.def("SetCount", (void (wxArrayString::*)(unsigned long)) &wxArrayString::SetCount, "C++: wxArrayString::SetCount(unsigned long) --> void", pybind11::arg("count"));
		cl.def("Remove", (void (wxArrayString::*)(const class wxString &)) &wxArrayString::Remove, "C++: wxArrayString::Remove(const class wxString &) --> void", pybind11::arg("sz"));
		cl.def("RemoveAt", [](wxArrayString &o, unsigned long const & a0) -> void { return o.RemoveAt(a0); }, "", pybind11::arg("nIndex"));
		cl.def("RemoveAt", (void (wxArrayString::*)(unsigned long, unsigned long)) &wxArrayString::RemoveAt, "C++: wxArrayString::RemoveAt(unsigned long, unsigned long) --> void", pybind11::arg("nIndex"), pybind11::arg("nRemove"));
		cl.def("Sort", [](wxArrayString &o) -> void { return o.Sort(); }, "");
		cl.def("Sort", (void (wxArrayString::*)(bool)) &wxArrayString::Sort, "C++: wxArrayString::Sort(bool) --> void", pybind11::arg("reverseOrder"));
		cl.def("__eq__", (bool (wxArrayString::*)(const class wxArrayString &) const) &wxArrayString::operator==, "C++: wxArrayString::operator==(const class wxArrayString &) const --> bool", pybind11::arg("a"));
		cl.def("__ne__", (bool (wxArrayString::*)(const class wxArrayString &) const) &wxArrayString::operator!=, "C++: wxArrayString::operator!=(const class wxArrayString &) const --> bool", pybind11::arg("a"));
		cl.def("assign", (void (wxArrayString::*)(unsigned long, const class wxString &)) &wxArrayString::assign, "C++: wxArrayString::assign(unsigned long, const class wxString &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("back", (class wxString & (wxArrayString::*)()) &wxArrayString::back, "C++: wxArrayString::back() --> class wxString &", pybind11::return_value_policy::automatic);
		cl.def("begin", (class wxString * (wxArrayString::*)()) &wxArrayString::begin, "C++: wxArrayString::begin() --> class wxString *", pybind11::return_value_policy::automatic);
		cl.def("capacity", (unsigned long (wxArrayString::*)() const) &wxArrayString::capacity, "C++: wxArrayString::capacity() const --> unsigned long");
		cl.def("clear", (void (wxArrayString::*)()) &wxArrayString::clear, "C++: wxArrayString::clear() --> void");
		cl.def("empty", (bool (wxArrayString::*)() const) &wxArrayString::empty, "C++: wxArrayString::empty() const --> bool");
		cl.def("end", (class wxString * (wxArrayString::*)()) &wxArrayString::end, "C++: wxArrayString::end() --> class wxString *", pybind11::return_value_policy::automatic);
		cl.def("erase", (class wxString * (wxArrayString::*)(class wxString *, class wxString *)) &wxArrayString::erase, "C++: wxArrayString::erase(class wxString *, class wxString *) --> class wxString *", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"));
		cl.def("erase", (class wxString * (wxArrayString::*)(class wxString *)) &wxArrayString::erase, "C++: wxArrayString::erase(class wxString *) --> class wxString *", pybind11::return_value_policy::automatic, pybind11::arg("it"));
		cl.def("front", (class wxString & (wxArrayString::*)()) &wxArrayString::front, "C++: wxArrayString::front() --> class wxString &", pybind11::return_value_policy::automatic);
		cl.def("insert", (void (wxArrayString::*)(class wxString *, unsigned long, const class wxString &)) &wxArrayString::insert, "C++: wxArrayString::insert(class wxString *, unsigned long, const class wxString &) --> void", pybind11::arg("it"), pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", [](wxArrayString &o, class wxString * a0) -> wxString * { return o.insert(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("it"));
		cl.def("insert", (class wxString * (wxArrayString::*)(class wxString *, const class wxString &)) &wxArrayString::insert, "C++: wxArrayString::insert(class wxString *, const class wxString &) --> class wxString *", pybind11::return_value_policy::automatic, pybind11::arg("it"), pybind11::arg("v"));
		cl.def("insert", (void (wxArrayString::*)(class wxString *, const class wxString *, const class wxString *)) &wxArrayString::insert, "C++: wxArrayString::insert(class wxString *, const class wxString *, const class wxString *) --> void", pybind11::arg("it"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("max_size", (unsigned long (wxArrayString::*)() const) &wxArrayString::max_size, "C++: wxArrayString::max_size() const --> unsigned long");
		cl.def("pop_back", (void (wxArrayString::*)()) &wxArrayString::pop_back, "C++: wxArrayString::pop_back() --> void");
		cl.def("push_back", (void (wxArrayString::*)(const class wxString &)) &wxArrayString::push_back, "C++: wxArrayString::push_back(const class wxString &) --> void", pybind11::arg("v"));
		cl.def("rbegin", (class wxArrayString::reverse_iterator (wxArrayString::*)()) &wxArrayString::rbegin, "C++: wxArrayString::rbegin() --> class wxArrayString::reverse_iterator");
		cl.def("rend", (class wxArrayString::reverse_iterator (wxArrayString::*)()) &wxArrayString::rend, "C++: wxArrayString::rend() --> class wxArrayString::reverse_iterator");
		cl.def("reserve", (void (wxArrayString::*)(unsigned long)) &wxArrayString::reserve, "C++: wxArrayString::reserve(unsigned long) --> void", pybind11::arg("n"));
		cl.def("resize", [](wxArrayString &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxArrayString::*)(unsigned long, class wxString)) &wxArrayString::resize, "C++: wxArrayString::resize(unsigned long, class wxString) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("size", (unsigned long (wxArrayString::*)() const) &wxArrayString::size, "C++: wxArrayString::size() const --> unsigned long");
		cl.def("swap", (void (wxArrayString::*)(class wxArrayString &)) &wxArrayString::swap, "C++: wxArrayString::swap(class wxArrayString &) --> void", pybind11::arg("other"));

		{ // wxArrayString::reverse_iterator file: line:241
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayString::reverse_iterator, std::shared_ptr<wxArrayString::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayString::reverse_iterator(); } ) );
			cl.def( pybind11::init<class wxString *>(), pybind11::arg("ptr") );

			cl.def( pybind11::init( [](wxArrayString::reverse_iterator const &o){ return new wxArrayString::reverse_iterator(o); } ) );
			cl.def("__mul__", (class wxString & (wxArrayString::reverse_iterator::*)() const) &wxArrayString::reverse_iterator::operator*, "C++: wxArrayString::reverse_iterator::operator*() const --> class wxString &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayString::reverse_iterator & (wxArrayString::reverse_iterator::*)()) &wxArrayString::reverse_iterator::operator++, "C++: wxArrayString::reverse_iterator::operator++() --> class wxArrayString::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayString::reverse_iterator (wxArrayString::reverse_iterator::*)(int)) &wxArrayString::reverse_iterator::operator++, "C++: wxArrayString::reverse_iterator::operator++(int) --> const class wxArrayString::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayString::reverse_iterator & (wxArrayString::reverse_iterator::*)()) &wxArrayString::reverse_iterator::operator--, "C++: wxArrayString::reverse_iterator::operator--() --> class wxArrayString::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayString::reverse_iterator (wxArrayString::reverse_iterator::*)(int)) &wxArrayString::reverse_iterator::operator--, "C++: wxArrayString::reverse_iterator::operator--(int) --> const class wxArrayString::reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayString::reverse_iterator::*)(const class wxArrayString::reverse_iterator &) const) &wxArrayString::reverse_iterator::operator==, "C++: wxArrayString::reverse_iterator::operator==(const class wxArrayString::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayString::reverse_iterator::*)(const class wxArrayString::reverse_iterator &) const) &wxArrayString::reverse_iterator::operator!=, "C++: wxArrayString::reverse_iterator::operator!=(const class wxArrayString::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxArrayString::const_reverse_iterator file: line:267
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayString::const_reverse_iterator, std::shared_ptr<wxArrayString::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayString::const_reverse_iterator(); } ) );
			cl.def( pybind11::init<const class wxString *>(), pybind11::arg("ptr") );

			cl.def( pybind11::init( [](wxArrayString::const_reverse_iterator const &o){ return new wxArrayString::const_reverse_iterator(o); } ) );
			cl.def( pybind11::init<const class wxArrayString::reverse_iterator &>(), pybind11::arg("it") );

			cl.def("__mul__", (const class wxString & (wxArrayString::const_reverse_iterator::*)() const) &wxArrayString::const_reverse_iterator::operator*, "C++: wxArrayString::const_reverse_iterator::operator*() const --> const class wxString &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayString::const_reverse_iterator & (wxArrayString::const_reverse_iterator::*)()) &wxArrayString::const_reverse_iterator::operator++, "C++: wxArrayString::const_reverse_iterator::operator++() --> class wxArrayString::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayString::const_reverse_iterator (wxArrayString::const_reverse_iterator::*)(int)) &wxArrayString::const_reverse_iterator::operator++, "C++: wxArrayString::const_reverse_iterator::operator++(int) --> const class wxArrayString::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayString::const_reverse_iterator & (wxArrayString::const_reverse_iterator::*)()) &wxArrayString::const_reverse_iterator::operator--, "C++: wxArrayString::const_reverse_iterator::operator--() --> class wxArrayString::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayString::const_reverse_iterator (wxArrayString::const_reverse_iterator::*)(int)) &wxArrayString::const_reverse_iterator::operator--, "C++: wxArrayString::const_reverse_iterator::operator--(int) --> const class wxArrayString::const_reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayString::const_reverse_iterator::*)(const class wxArrayString::const_reverse_iterator &) const) &wxArrayString::const_reverse_iterator::operator==, "C++: wxArrayString::const_reverse_iterator::operator==(const class wxArrayString::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayString::const_reverse_iterator::*)(const class wxArrayString::const_reverse_iterator &) const) &wxArrayString::const_reverse_iterator::operator!=, "C++: wxArrayString::const_reverse_iterator::operator!=(const class wxArrayString::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
	{ // wxSortedArrayString file: line:380
		pybind11::class_<wxSortedArrayString, std::shared_ptr<wxSortedArrayString>, wxArrayString> cl(M(""), "wxSortedArrayString", "");
		cl.def( pybind11::init( [](){ return new wxSortedArrayString(); } ) );
		cl.def( pybind11::init<const class wxArrayString &>(), pybind11::arg("array") );

		cl.def( pybind11::init( [](wxSortedArrayString const &o){ return new wxSortedArrayString(o); } ) );
		cl.def("assign", (class wxSortedArrayString & (wxSortedArrayString::*)(const class wxSortedArrayString &)) &wxSortedArrayString::operator=, "C++: wxSortedArrayString::operator=(const class wxSortedArrayString &) --> class wxSortedArrayString &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxCArrayString file: line:393
		pybind11::class_<wxCArrayString, std::shared_ptr<wxCArrayString>> cl(M(""), "wxCArrayString", "");
		cl.def( pybind11::init<const class wxArrayString &>(), pybind11::arg("array") );

		cl.def("GetCount", (unsigned long (wxCArrayString::*)() const) &wxCArrayString::GetCount, "C++: wxCArrayString::GetCount() const --> unsigned long");
		cl.def("GetStrings", (class wxString * (wxCArrayString::*)()) &wxCArrayString::GetStrings, "C++: wxCArrayString::GetStrings() --> class wxString *", pybind11::return_value_policy::automatic);
		cl.def("Release", (class wxString * (wxCArrayString::*)()) &wxCArrayString::Release, "C++: wxCArrayString::Release() --> class wxString *", pybind11::return_value_policy::automatic);
	}
}

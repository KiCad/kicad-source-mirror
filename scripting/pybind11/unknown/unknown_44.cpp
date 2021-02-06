#include <sstream> // __str__

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

void bind_unknown_unknown_44(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxAssert_wxArrayLong file: line:17
		pybind11::class_<wxAssert_wxArrayLong, std::shared_ptr<wxAssert_wxArrayLong>> cl(M(""), "wxAssert_wxArrayLong", "");
		cl.def( pybind11::init( [](){ return new wxAssert_wxArrayLong(); } ) );
		cl.def_readwrite("TypeTooBigToBeStoredInwxBaseArrayLong", &wxAssert_wxArrayLong::TypeTooBigToBeStoredInwxBaseArrayLong);
	}
	{ // wxArrayLong file: line:1024
		pybind11::class_<wxArrayLong, std::shared_ptr<wxArrayLong>, wxBaseArrayLong> cl(M(""), "wxArrayLong", "");
		cl.def( pybind11::init( [](){ return new wxArrayLong(); } ) );
		cl.def( pybind11::init<unsigned long>(), pybind11::arg("n") );

		cl.def( pybind11::init<unsigned long, const long &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init<const long *, const long *>(), pybind11::arg("first"), pybind11::arg("last") );

		cl.def( pybind11::init( [](wxArrayLong const &o){ return new wxArrayLong(o); } ) );
		cl.def("__getitem__", (long & (wxArrayLong::*)(unsigned long) const) &wxArrayLong::operator[], "C++: wxArrayLong::operator[](unsigned long) const --> long &", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Item", (long & (wxArrayLong::*)(unsigned long) const) &wxArrayLong::Item, "C++: wxArrayLong::Item(unsigned long) const --> long &", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Last", (long & (wxArrayLong::*)() const) &wxArrayLong::Last, "C++: wxArrayLong::Last() const --> long &", pybind11::return_value_policy::automatic);
		cl.def("Index", [](wxArrayLong const &o, long const & a0) -> int { return o.Index(a0); }, "", pybind11::arg("lItem"));
		cl.def("Index", (int (wxArrayLong::*)(long, bool) const) &wxArrayLong::Index, "C++: wxArrayLong::Index(long, bool) const --> int", pybind11::arg("lItem"), pybind11::arg("bFromEnd"));
		cl.def("Add", [](wxArrayLong &o, long const & a0) -> void { return o.Add(a0); }, "", pybind11::arg("lItem"));
		cl.def("Add", (void (wxArrayLong::*)(long, unsigned long)) &wxArrayLong::Add, "C++: wxArrayLong::Add(long, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("nInsert"));
		cl.def("Insert", [](wxArrayLong &o, long const & a0, unsigned long const & a1) -> void { return o.Insert(a0, a1); }, "", pybind11::arg("lItem"), pybind11::arg("uiIndex"));
		cl.def("Insert", (void (wxArrayLong::*)(long, unsigned long, unsigned long)) &wxArrayLong::Insert, "C++: wxArrayLong::Insert(long, unsigned long, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("uiIndex"), pybind11::arg("nInsert"));
		cl.def("RemoveAt", [](wxArrayLong &o, unsigned long const & a0) -> void { return o.RemoveAt(a0); }, "", pybind11::arg("uiIndex"));
		cl.def("RemoveAt", (void (wxArrayLong::*)(unsigned long, unsigned long)) &wxArrayLong::RemoveAt, "C++: wxArrayLong::RemoveAt(unsigned long, unsigned long) --> void", pybind11::arg("uiIndex"), pybind11::arg("nRemove"));
		cl.def("Remove", (void (wxArrayLong::*)(long)) &wxArrayLong::Remove, "C++: wxArrayLong::Remove(long) --> void", pybind11::arg("lItem"));
		cl.def("assign", (void (wxArrayLong::*)(const long *, const long *)) &wxArrayLong::assign, "C++: wxArrayLong::assign(const long *, const long *) --> void", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("assign", (void (wxArrayLong::*)(unsigned long, const long &)) &wxArrayLong::assign, "C++: wxArrayLong::assign(unsigned long, const long &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("back", (long & (wxArrayLong::*)()) &wxArrayLong::back, "C++: wxArrayLong::back() --> long &", pybind11::return_value_policy::automatic);
		cl.def("begin", (long * (wxArrayLong::*)()) &wxArrayLong::begin, "C++: wxArrayLong::begin() --> long *", pybind11::return_value_policy::automatic);
		cl.def("capacity", (unsigned long (wxArrayLong::*)() const) &wxArrayLong::capacity, "C++: wxArrayLong::capacity() const --> unsigned long");
		cl.def("end", (long * (wxArrayLong::*)()) &wxArrayLong::end, "C++: wxArrayLong::end() --> long *", pybind11::return_value_policy::automatic);
		cl.def("erase", (long * (wxArrayLong::*)(long *, long *)) &wxArrayLong::erase, "C++: wxArrayLong::erase(long *, long *) --> long *", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"));
		cl.def("erase", (long * (wxArrayLong::*)(long *)) &wxArrayLong::erase, "C++: wxArrayLong::erase(long *) --> long *", pybind11::return_value_policy::automatic, pybind11::arg("it"));
		cl.def("front", (long & (wxArrayLong::*)()) &wxArrayLong::front, "C++: wxArrayLong::front() --> long &", pybind11::return_value_policy::automatic);
		cl.def("insert", (void (wxArrayLong::*)(long *, unsigned long, const long &)) &wxArrayLong::insert, "C++: wxArrayLong::insert(long *, unsigned long, const long &) --> void", pybind11::arg("it"), pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", [](wxArrayLong &o, long * a0) -> long * { return o.insert(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("it"));
		cl.def("insert", (long * (wxArrayLong::*)(long *, const long &)) &wxArrayLong::insert, "C++: wxArrayLong::insert(long *, const long &) --> long *", pybind11::return_value_policy::automatic, pybind11::arg("it"), pybind11::arg("v"));
		cl.def("insert", (void (wxArrayLong::*)(long *, const long *, const long *)) &wxArrayLong::insert, "C++: wxArrayLong::insert(long *, const long *, const long *) --> void", pybind11::arg("it"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("pop_back", (void (wxArrayLong::*)()) &wxArrayLong::pop_back, "C++: wxArrayLong::pop_back() --> void");
		cl.def("push_back", (void (wxArrayLong::*)(const long &)) &wxArrayLong::push_back, "C++: wxArrayLong::push_back(const long &) --> void", pybind11::arg("v"));
		cl.def("rbegin", (class wxArrayLong::reverse_iterator (wxArrayLong::*)()) &wxArrayLong::rbegin, "C++: wxArrayLong::rbegin() --> class wxArrayLong::reverse_iterator");
		cl.def("rend", (class wxArrayLong::reverse_iterator (wxArrayLong::*)()) &wxArrayLong::rend, "C++: wxArrayLong::rend() --> class wxArrayLong::reverse_iterator");
		cl.def("reserve", (void (wxArrayLong::*)(unsigned long)) &wxArrayLong::reserve, "C++: wxArrayLong::reserve(unsigned long) --> void", pybind11::arg("n"));
		cl.def("resize", [](wxArrayLong &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxArrayLong::*)(unsigned long, long)) &wxArrayLong::resize, "C++: wxArrayLong::resize(unsigned long, long) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("swap", (void (wxArrayLong::*)(class wxArrayLong &)) &wxArrayLong::swap, "C++: wxArrayLong::swap(class wxArrayLong &) --> void", pybind11::arg("other"));
		cl.def("assign", (class wxArrayLong & (wxArrayLong::*)(const class wxArrayLong &)) &wxArrayLong::operator=, "C++: wxArrayLong::operator=(const class wxArrayLong &) --> class wxArrayLong &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxArrayLong::reverse_iterator file: line:400
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayLong::reverse_iterator, std::shared_ptr<wxArrayLong::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayLong::reverse_iterator(); } ) );
			cl.def( pybind11::init<long *>(), pybind11::arg("ptr") );

			cl.def( pybind11::init( [](wxArrayLong::reverse_iterator const &o){ return new wxArrayLong::reverse_iterator(o); } ) );
			cl.def("__mul__", (long & (wxArrayLong::reverse_iterator::*)() const) &wxArrayLong::reverse_iterator::operator*, "C++: wxArrayLong::reverse_iterator::operator*() const --> long &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayLong::reverse_iterator & (wxArrayLong::reverse_iterator::*)()) &wxArrayLong::reverse_iterator::operator++, "C++: wxArrayLong::reverse_iterator::operator++() --> class wxArrayLong::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayLong::reverse_iterator (wxArrayLong::reverse_iterator::*)(int)) &wxArrayLong::reverse_iterator::operator++, "C++: wxArrayLong::reverse_iterator::operator++(int) --> const class wxArrayLong::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayLong::reverse_iterator & (wxArrayLong::reverse_iterator::*)()) &wxArrayLong::reverse_iterator::operator--, "C++: wxArrayLong::reverse_iterator::operator--() --> class wxArrayLong::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayLong::reverse_iterator (wxArrayLong::reverse_iterator::*)(int)) &wxArrayLong::reverse_iterator::operator--, "C++: wxArrayLong::reverse_iterator::operator--(int) --> const class wxArrayLong::reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayLong::reverse_iterator::*)(const class wxArrayLong::reverse_iterator &) const) &wxArrayLong::reverse_iterator::operator==, "C++: wxArrayLong::reverse_iterator::operator==(const class wxArrayLong::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayLong::reverse_iterator::*)(const class wxArrayLong::reverse_iterator &) const) &wxArrayLong::reverse_iterator::operator!=, "C++: wxArrayLong::reverse_iterator::operator!=(const class wxArrayLong::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxArrayLong::const_reverse_iterator file: line:432
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayLong::const_reverse_iterator, std::shared_ptr<wxArrayLong::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayLong::const_reverse_iterator(); } ) );
			cl.def( pybind11::init<const long *>(), pybind11::arg("ptr") );

			cl.def( pybind11::init( [](wxArrayLong::const_reverse_iterator const &o){ return new wxArrayLong::const_reverse_iterator(o); } ) );
			cl.def( pybind11::init<const class wxArrayLong::reverse_iterator &>(), pybind11::arg("it") );

			cl.def("__mul__", (const long & (wxArrayLong::const_reverse_iterator::*)() const) &wxArrayLong::const_reverse_iterator::operator*, "C++: wxArrayLong::const_reverse_iterator::operator*() const --> const long &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayLong::const_reverse_iterator & (wxArrayLong::const_reverse_iterator::*)()) &wxArrayLong::const_reverse_iterator::operator++, "C++: wxArrayLong::const_reverse_iterator::operator++() --> class wxArrayLong::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayLong::const_reverse_iterator (wxArrayLong::const_reverse_iterator::*)(int)) &wxArrayLong::const_reverse_iterator::operator++, "C++: wxArrayLong::const_reverse_iterator::operator++(int) --> const class wxArrayLong::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayLong::const_reverse_iterator & (wxArrayLong::const_reverse_iterator::*)()) &wxArrayLong::const_reverse_iterator::operator--, "C++: wxArrayLong::const_reverse_iterator::operator--() --> class wxArrayLong::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayLong::const_reverse_iterator (wxArrayLong::const_reverse_iterator::*)(int)) &wxArrayLong::const_reverse_iterator::operator--, "C++: wxArrayLong::const_reverse_iterator::operator--(int) --> const class wxArrayLong::const_reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayLong::const_reverse_iterator::*)(const class wxArrayLong::const_reverse_iterator &) const) &wxArrayLong::const_reverse_iterator::operator==, "C++: wxArrayLong::const_reverse_iterator::operator==(const class wxArrayLong::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayLong::const_reverse_iterator::*)(const class wxArrayLong::const_reverse_iterator &) const) &wxArrayLong::const_reverse_iterator::operator!=, "C++: wxArrayLong::const_reverse_iterator::operator!=(const class wxArrayLong::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
	{ // wxAssert_wxArrayPtrVoid file: line:26
		pybind11::class_<wxAssert_wxArrayPtrVoid, std::shared_ptr<wxAssert_wxArrayPtrVoid>> cl(M(""), "wxAssert_wxArrayPtrVoid", "");
		cl.def( pybind11::init( [](){ return new wxAssert_wxArrayPtrVoid(); } ) );
		cl.def_readwrite("TypeTooBigToBeStoredInwxBaseArrayPtrVoid", &wxAssert_wxArrayPtrVoid::TypeTooBigToBeStoredInwxBaseArrayPtrVoid);
	}
	{ // wxArrayPtrVoid file: line:1025
		pybind11::class_<wxArrayPtrVoid, std::shared_ptr<wxArrayPtrVoid>, wxBaseArrayPtrVoid> cl(M(""), "wxArrayPtrVoid", "");
		cl.def( pybind11::init( [](){ return new wxArrayPtrVoid(); } ) );
		cl.def( pybind11::init<unsigned long>(), pybind11::arg("n") );

		cl.def( pybind11::init<unsigned long, void *const &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init( [](wxArrayPtrVoid const &o){ return new wxArrayPtrVoid(o); } ) );
		cl.def("__getitem__", (void *& (wxArrayPtrVoid::*)(unsigned long) const) &wxArrayPtrVoid::operator[], "C++: wxArrayPtrVoid::operator[](unsigned long) const --> void *&", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Item", (void *& (wxArrayPtrVoid::*)(unsigned long) const) &wxArrayPtrVoid::Item, "C++: wxArrayPtrVoid::Item(unsigned long) const --> void *&", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Last", (void *& (wxArrayPtrVoid::*)() const) &wxArrayPtrVoid::Last, "C++: wxArrayPtrVoid::Last() const --> void *&", pybind11::return_value_policy::automatic);
		cl.def("Index", [](wxArrayPtrVoid const &o, void * a0) -> int { return o.Index(a0); }, "", pybind11::arg("lItem"));
		cl.def("Index", (int (wxArrayPtrVoid::*)(void *, bool) const) &wxArrayPtrVoid::Index, "C++: wxArrayPtrVoid::Index(void *, bool) const --> int", pybind11::arg("lItem"), pybind11::arg("bFromEnd"));
		cl.def("Add", [](wxArrayPtrVoid &o, void * a0) -> void { return o.Add(a0); }, "", pybind11::arg("lItem"));
		cl.def("Add", (void (wxArrayPtrVoid::*)(void *, unsigned long)) &wxArrayPtrVoid::Add, "C++: wxArrayPtrVoid::Add(void *, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("nInsert"));
		cl.def("Insert", [](wxArrayPtrVoid &o, void * a0, unsigned long const & a1) -> void { return o.Insert(a0, a1); }, "", pybind11::arg("lItem"), pybind11::arg("uiIndex"));
		cl.def("Insert", (void (wxArrayPtrVoid::*)(void *, unsigned long, unsigned long)) &wxArrayPtrVoid::Insert, "C++: wxArrayPtrVoid::Insert(void *, unsigned long, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("uiIndex"), pybind11::arg("nInsert"));
		cl.def("RemoveAt", [](wxArrayPtrVoid &o, unsigned long const & a0) -> void { return o.RemoveAt(a0); }, "", pybind11::arg("uiIndex"));
		cl.def("RemoveAt", (void (wxArrayPtrVoid::*)(unsigned long, unsigned long)) &wxArrayPtrVoid::RemoveAt, "C++: wxArrayPtrVoid::RemoveAt(unsigned long, unsigned long) --> void", pybind11::arg("uiIndex"), pybind11::arg("nRemove"));
		cl.def("Remove", (void (wxArrayPtrVoid::*)(void *)) &wxArrayPtrVoid::Remove, "C++: wxArrayPtrVoid::Remove(void *) --> void", pybind11::arg("lItem"));
		cl.def("assign", (void (wxArrayPtrVoid::*)(unsigned long, void *const &)) &wxArrayPtrVoid::assign, "C++: wxArrayPtrVoid::assign(unsigned long, void *const &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("back", (void *& (wxArrayPtrVoid::*)()) &wxArrayPtrVoid::back, "C++: wxArrayPtrVoid::back() --> void *&", pybind11::return_value_policy::automatic);
		cl.def("begin", (void ** (wxArrayPtrVoid::*)()) &wxArrayPtrVoid::begin, "C++: wxArrayPtrVoid::begin() --> void **", pybind11::return_value_policy::automatic);
		cl.def("capacity", (unsigned long (wxArrayPtrVoid::*)() const) &wxArrayPtrVoid::capacity, "C++: wxArrayPtrVoid::capacity() const --> unsigned long");
		cl.def("end", (void ** (wxArrayPtrVoid::*)()) &wxArrayPtrVoid::end, "C++: wxArrayPtrVoid::end() --> void **", pybind11::return_value_policy::automatic);
		cl.def("front", (void *& (wxArrayPtrVoid::*)()) &wxArrayPtrVoid::front, "C++: wxArrayPtrVoid::front() --> void *&", pybind11::return_value_policy::automatic);
		cl.def("pop_back", (void (wxArrayPtrVoid::*)()) &wxArrayPtrVoid::pop_back, "C++: wxArrayPtrVoid::pop_back() --> void");
		cl.def("push_back", (void (wxArrayPtrVoid::*)(void *const &)) &wxArrayPtrVoid::push_back, "C++: wxArrayPtrVoid::push_back(void *const &) --> void", pybind11::arg("v"));
		cl.def("rbegin", (class wxArrayPtrVoid::reverse_iterator (wxArrayPtrVoid::*)()) &wxArrayPtrVoid::rbegin, "C++: wxArrayPtrVoid::rbegin() --> class wxArrayPtrVoid::reverse_iterator");
		cl.def("rend", (class wxArrayPtrVoid::reverse_iterator (wxArrayPtrVoid::*)()) &wxArrayPtrVoid::rend, "C++: wxArrayPtrVoid::rend() --> class wxArrayPtrVoid::reverse_iterator");
		cl.def("reserve", (void (wxArrayPtrVoid::*)(unsigned long)) &wxArrayPtrVoid::reserve, "C++: wxArrayPtrVoid::reserve(unsigned long) --> void", pybind11::arg("n"));
		cl.def("resize", [](wxArrayPtrVoid &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxArrayPtrVoid::*)(unsigned long, void *)) &wxArrayPtrVoid::resize, "C++: wxArrayPtrVoid::resize(unsigned long, void *) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("swap", (void (wxArrayPtrVoid::*)(class wxArrayPtrVoid &)) &wxArrayPtrVoid::swap, "C++: wxArrayPtrVoid::swap(class wxArrayPtrVoid &) --> void", pybind11::arg("other"));
		cl.def("assign", (class wxArrayPtrVoid & (wxArrayPtrVoid::*)(const class wxArrayPtrVoid &)) &wxArrayPtrVoid::operator=, "C++: wxArrayPtrVoid::operator=(const class wxArrayPtrVoid &) --> class wxArrayPtrVoid &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxArrayPtrVoid::reverse_iterator file: line:400
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayPtrVoid::reverse_iterator, std::shared_ptr<wxArrayPtrVoid::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayPtrVoid::reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxArrayPtrVoid::reverse_iterator const &o){ return new wxArrayPtrVoid::reverse_iterator(o); } ) );
			cl.def("__mul__", (void *& (wxArrayPtrVoid::reverse_iterator::*)() const) &wxArrayPtrVoid::reverse_iterator::operator*, "C++: wxArrayPtrVoid::reverse_iterator::operator*() const --> void *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayPtrVoid::reverse_iterator & (wxArrayPtrVoid::reverse_iterator::*)()) &wxArrayPtrVoid::reverse_iterator::operator++, "C++: wxArrayPtrVoid::reverse_iterator::operator++() --> class wxArrayPtrVoid::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayPtrVoid::reverse_iterator (wxArrayPtrVoid::reverse_iterator::*)(int)) &wxArrayPtrVoid::reverse_iterator::operator++, "C++: wxArrayPtrVoid::reverse_iterator::operator++(int) --> const class wxArrayPtrVoid::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayPtrVoid::reverse_iterator & (wxArrayPtrVoid::reverse_iterator::*)()) &wxArrayPtrVoid::reverse_iterator::operator--, "C++: wxArrayPtrVoid::reverse_iterator::operator--() --> class wxArrayPtrVoid::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayPtrVoid::reverse_iterator (wxArrayPtrVoid::reverse_iterator::*)(int)) &wxArrayPtrVoid::reverse_iterator::operator--, "C++: wxArrayPtrVoid::reverse_iterator::operator--(int) --> const class wxArrayPtrVoid::reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayPtrVoid::reverse_iterator::*)(const class wxArrayPtrVoid::reverse_iterator &) const) &wxArrayPtrVoid::reverse_iterator::operator==, "C++: wxArrayPtrVoid::reverse_iterator::operator==(const class wxArrayPtrVoid::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayPtrVoid::reverse_iterator::*)(const class wxArrayPtrVoid::reverse_iterator &) const) &wxArrayPtrVoid::reverse_iterator::operator!=, "C++: wxArrayPtrVoid::reverse_iterator::operator!=(const class wxArrayPtrVoid::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxArrayPtrVoid::const_reverse_iterator file: line:432
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayPtrVoid::const_reverse_iterator, std::shared_ptr<wxArrayPtrVoid::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayPtrVoid::const_reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxArrayPtrVoid::const_reverse_iterator const &o){ return new wxArrayPtrVoid::const_reverse_iterator(o); } ) );
			cl.def( pybind11::init<const class wxArrayPtrVoid::reverse_iterator &>(), pybind11::arg("it") );

			cl.def("__mul__", (void *const & (wxArrayPtrVoid::const_reverse_iterator::*)() const) &wxArrayPtrVoid::const_reverse_iterator::operator*, "C++: wxArrayPtrVoid::const_reverse_iterator::operator*() const --> void *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayPtrVoid::const_reverse_iterator & (wxArrayPtrVoid::const_reverse_iterator::*)()) &wxArrayPtrVoid::const_reverse_iterator::operator++, "C++: wxArrayPtrVoid::const_reverse_iterator::operator++() --> class wxArrayPtrVoid::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayPtrVoid::const_reverse_iterator (wxArrayPtrVoid::const_reverse_iterator::*)(int)) &wxArrayPtrVoid::const_reverse_iterator::operator++, "C++: wxArrayPtrVoid::const_reverse_iterator::operator++(int) --> const class wxArrayPtrVoid::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayPtrVoid::const_reverse_iterator & (wxArrayPtrVoid::const_reverse_iterator::*)()) &wxArrayPtrVoid::const_reverse_iterator::operator--, "C++: wxArrayPtrVoid::const_reverse_iterator::operator--() --> class wxArrayPtrVoid::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayPtrVoid::const_reverse_iterator (wxArrayPtrVoid::const_reverse_iterator::*)(int)) &wxArrayPtrVoid::const_reverse_iterator::operator--, "C++: wxArrayPtrVoid::const_reverse_iterator::operator--(int) --> const class wxArrayPtrVoid::const_reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayPtrVoid::const_reverse_iterator::*)(const class wxArrayPtrVoid::const_reverse_iterator &) const) &wxArrayPtrVoid::const_reverse_iterator::operator==, "C++: wxArrayPtrVoid::const_reverse_iterator::operator==(const class wxArrayPtrVoid::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayPtrVoid::const_reverse_iterator::*)(const class wxArrayPtrVoid::const_reverse_iterator &) const) &wxArrayPtrVoid::const_reverse_iterator::operator!=, "C++: wxArrayPtrVoid::const_reverse_iterator::operator!=(const class wxArrayPtrVoid::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
}

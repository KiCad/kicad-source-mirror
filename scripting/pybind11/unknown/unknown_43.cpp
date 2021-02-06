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

void bind_unknown_unknown_43(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxAssert_wxArrayInt file: line:185
		pybind11::class_<wxAssert_wxArrayInt, std::shared_ptr<wxAssert_wxArrayInt>> cl(M(""), "wxAssert_wxArrayInt", "");
		cl.def( pybind11::init( [](){ return new wxAssert_wxArrayInt(); } ) );
		cl.def_readwrite("TypeTooBigToBeStoredInwxBaseArrayInt", &wxAssert_wxArrayInt::TypeTooBigToBeStoredInwxBaseArrayInt);
	}
	{ // wxArrayInt file: line:1022
		pybind11::class_<wxArrayInt, std::shared_ptr<wxArrayInt>, wxBaseArrayInt> cl(M(""), "wxArrayInt", "");
		cl.def( pybind11::init( [](){ return new wxArrayInt(); } ) );
		cl.def( pybind11::init<unsigned long>(), pybind11::arg("n") );

		cl.def( pybind11::init<unsigned long, const int &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init<const int *, const int *>(), pybind11::arg("first"), pybind11::arg("last") );

		cl.def( pybind11::init( [](wxArrayInt const &o){ return new wxArrayInt(o); } ) );
		cl.def("__getitem__", (int & (wxArrayInt::*)(unsigned long) const) &wxArrayInt::operator[], "C++: wxArrayInt::operator[](unsigned long) const --> int &", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Item", (int & (wxArrayInt::*)(unsigned long) const) &wxArrayInt::Item, "C++: wxArrayInt::Item(unsigned long) const --> int &", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Last", (int & (wxArrayInt::*)() const) &wxArrayInt::Last, "C++: wxArrayInt::Last() const --> int &", pybind11::return_value_policy::automatic);
		cl.def("Index", [](wxArrayInt const &o, int const & a0) -> int { return o.Index(a0); }, "", pybind11::arg("lItem"));
		cl.def("Index", (int (wxArrayInt::*)(int, bool) const) &wxArrayInt::Index, "C++: wxArrayInt::Index(int, bool) const --> int", pybind11::arg("lItem"), pybind11::arg("bFromEnd"));
		cl.def("Add", [](wxArrayInt &o, int const & a0) -> void { return o.Add(a0); }, "", pybind11::arg("lItem"));
		cl.def("Add", (void (wxArrayInt::*)(int, unsigned long)) &wxArrayInt::Add, "C++: wxArrayInt::Add(int, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("nInsert"));
		cl.def("Insert", [](wxArrayInt &o, int const & a0, unsigned long const & a1) -> void { return o.Insert(a0, a1); }, "", pybind11::arg("lItem"), pybind11::arg("uiIndex"));
		cl.def("Insert", (void (wxArrayInt::*)(int, unsigned long, unsigned long)) &wxArrayInt::Insert, "C++: wxArrayInt::Insert(int, unsigned long, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("uiIndex"), pybind11::arg("nInsert"));
		cl.def("RemoveAt", [](wxArrayInt &o, unsigned long const & a0) -> void { return o.RemoveAt(a0); }, "", pybind11::arg("uiIndex"));
		cl.def("RemoveAt", (void (wxArrayInt::*)(unsigned long, unsigned long)) &wxArrayInt::RemoveAt, "C++: wxArrayInt::RemoveAt(unsigned long, unsigned long) --> void", pybind11::arg("uiIndex"), pybind11::arg("nRemove"));
		cl.def("Remove", (void (wxArrayInt::*)(int)) &wxArrayInt::Remove, "C++: wxArrayInt::Remove(int) --> void", pybind11::arg("lItem"));
		cl.def("assign", (void (wxArrayInt::*)(const int *, const int *)) &wxArrayInt::assign, "C++: wxArrayInt::assign(const int *, const int *) --> void", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("assign", (void (wxArrayInt::*)(unsigned long, const int &)) &wxArrayInt::assign, "C++: wxArrayInt::assign(unsigned long, const int &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("back", (int & (wxArrayInt::*)()) &wxArrayInt::back, "C++: wxArrayInt::back() --> int &", pybind11::return_value_policy::automatic);
		cl.def("begin", (int * (wxArrayInt::*)()) &wxArrayInt::begin, "C++: wxArrayInt::begin() --> int *", pybind11::return_value_policy::automatic);
		cl.def("capacity", (unsigned long (wxArrayInt::*)() const) &wxArrayInt::capacity, "C++: wxArrayInt::capacity() const --> unsigned long");
		cl.def("end", (int * (wxArrayInt::*)()) &wxArrayInt::end, "C++: wxArrayInt::end() --> int *", pybind11::return_value_policy::automatic);
		cl.def("erase", (int * (wxArrayInt::*)(int *, int *)) &wxArrayInt::erase, "C++: wxArrayInt::erase(int *, int *) --> int *", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"));
		cl.def("erase", (int * (wxArrayInt::*)(int *)) &wxArrayInt::erase, "C++: wxArrayInt::erase(int *) --> int *", pybind11::return_value_policy::automatic, pybind11::arg("it"));
		cl.def("front", (int & (wxArrayInt::*)()) &wxArrayInt::front, "C++: wxArrayInt::front() --> int &", pybind11::return_value_policy::automatic);
		cl.def("insert", (void (wxArrayInt::*)(int *, unsigned long, const int &)) &wxArrayInt::insert, "C++: wxArrayInt::insert(int *, unsigned long, const int &) --> void", pybind11::arg("it"), pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", [](wxArrayInt &o, int * a0) -> int * { return o.insert(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("it"));
		cl.def("insert", (int * (wxArrayInt::*)(int *, const int &)) &wxArrayInt::insert, "C++: wxArrayInt::insert(int *, const int &) --> int *", pybind11::return_value_policy::automatic, pybind11::arg("it"), pybind11::arg("v"));
		cl.def("insert", (void (wxArrayInt::*)(int *, const int *, const int *)) &wxArrayInt::insert, "C++: wxArrayInt::insert(int *, const int *, const int *) --> void", pybind11::arg("it"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("pop_back", (void (wxArrayInt::*)()) &wxArrayInt::pop_back, "C++: wxArrayInt::pop_back() --> void");
		cl.def("push_back", (void (wxArrayInt::*)(const int &)) &wxArrayInt::push_back, "C++: wxArrayInt::push_back(const int &) --> void", pybind11::arg("v"));
		cl.def("rbegin", (class wxArrayInt::reverse_iterator (wxArrayInt::*)()) &wxArrayInt::rbegin, "C++: wxArrayInt::rbegin() --> class wxArrayInt::reverse_iterator");
		cl.def("rend", (class wxArrayInt::reverse_iterator (wxArrayInt::*)()) &wxArrayInt::rend, "C++: wxArrayInt::rend() --> class wxArrayInt::reverse_iterator");
		cl.def("reserve", (void (wxArrayInt::*)(unsigned long)) &wxArrayInt::reserve, "C++: wxArrayInt::reserve(unsigned long) --> void", pybind11::arg("n"));
		cl.def("resize", [](wxArrayInt &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxArrayInt::*)(unsigned long, int)) &wxArrayInt::resize, "C++: wxArrayInt::resize(unsigned long, int) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("swap", (void (wxArrayInt::*)(class wxArrayInt &)) &wxArrayInt::swap, "C++: wxArrayInt::swap(class wxArrayInt &) --> void", pybind11::arg("other"));
		cl.def("assign", (class wxArrayInt & (wxArrayInt::*)(const class wxArrayInt &)) &wxArrayInt::operator=, "C++: wxArrayInt::operator=(const class wxArrayInt &) --> class wxArrayInt &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxArrayInt::reverse_iterator file: line:400
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayInt::reverse_iterator, std::shared_ptr<wxArrayInt::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayInt::reverse_iterator(); } ) );
			cl.def( pybind11::init<int *>(), pybind11::arg("ptr") );

			cl.def( pybind11::init( [](wxArrayInt::reverse_iterator const &o){ return new wxArrayInt::reverse_iterator(o); } ) );
			cl.def("__mul__", (int & (wxArrayInt::reverse_iterator::*)() const) &wxArrayInt::reverse_iterator::operator*, "C++: wxArrayInt::reverse_iterator::operator*() const --> int &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayInt::reverse_iterator & (wxArrayInt::reverse_iterator::*)()) &wxArrayInt::reverse_iterator::operator++, "C++: wxArrayInt::reverse_iterator::operator++() --> class wxArrayInt::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayInt::reverse_iterator (wxArrayInt::reverse_iterator::*)(int)) &wxArrayInt::reverse_iterator::operator++, "C++: wxArrayInt::reverse_iterator::operator++(int) --> const class wxArrayInt::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayInt::reverse_iterator & (wxArrayInt::reverse_iterator::*)()) &wxArrayInt::reverse_iterator::operator--, "C++: wxArrayInt::reverse_iterator::operator--() --> class wxArrayInt::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayInt::reverse_iterator (wxArrayInt::reverse_iterator::*)(int)) &wxArrayInt::reverse_iterator::operator--, "C++: wxArrayInt::reverse_iterator::operator--(int) --> const class wxArrayInt::reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayInt::reverse_iterator::*)(const class wxArrayInt::reverse_iterator &) const) &wxArrayInt::reverse_iterator::operator==, "C++: wxArrayInt::reverse_iterator::operator==(const class wxArrayInt::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayInt::reverse_iterator::*)(const class wxArrayInt::reverse_iterator &) const) &wxArrayInt::reverse_iterator::operator!=, "C++: wxArrayInt::reverse_iterator::operator!=(const class wxArrayInt::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxArrayInt::const_reverse_iterator file: line:432
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayInt::const_reverse_iterator, std::shared_ptr<wxArrayInt::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayInt::const_reverse_iterator(); } ) );
			cl.def( pybind11::init<const int *>(), pybind11::arg("ptr") );

			cl.def( pybind11::init( [](wxArrayInt::const_reverse_iterator const &o){ return new wxArrayInt::const_reverse_iterator(o); } ) );
			cl.def( pybind11::init<const class wxArrayInt::reverse_iterator &>(), pybind11::arg("it") );

			cl.def("__mul__", (const int & (wxArrayInt::const_reverse_iterator::*)() const) &wxArrayInt::const_reverse_iterator::operator*, "C++: wxArrayInt::const_reverse_iterator::operator*() const --> const int &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayInt::const_reverse_iterator & (wxArrayInt::const_reverse_iterator::*)()) &wxArrayInt::const_reverse_iterator::operator++, "C++: wxArrayInt::const_reverse_iterator::operator++() --> class wxArrayInt::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayInt::const_reverse_iterator (wxArrayInt::const_reverse_iterator::*)(int)) &wxArrayInt::const_reverse_iterator::operator++, "C++: wxArrayInt::const_reverse_iterator::operator++(int) --> const class wxArrayInt::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayInt::const_reverse_iterator & (wxArrayInt::const_reverse_iterator::*)()) &wxArrayInt::const_reverse_iterator::operator--, "C++: wxArrayInt::const_reverse_iterator::operator--() --> class wxArrayInt::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayInt::const_reverse_iterator (wxArrayInt::const_reverse_iterator::*)(int)) &wxArrayInt::const_reverse_iterator::operator--, "C++: wxArrayInt::const_reverse_iterator::operator--(int) --> const class wxArrayInt::const_reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayInt::const_reverse_iterator::*)(const class wxArrayInt::const_reverse_iterator &) const) &wxArrayInt::const_reverse_iterator::operator==, "C++: wxArrayInt::const_reverse_iterator::operator==(const class wxArrayInt::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayInt::const_reverse_iterator::*)(const class wxArrayInt::const_reverse_iterator &) const) &wxArrayInt::const_reverse_iterator::operator!=, "C++: wxArrayInt::const_reverse_iterator::operator!=(const class wxArrayInt::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
	{ // wxAssert_wxArrayDouble file: line:8
		pybind11::class_<wxAssert_wxArrayDouble, std::shared_ptr<wxAssert_wxArrayDouble>> cl(M(""), "wxAssert_wxArrayDouble", "");
		cl.def( pybind11::init( [](){ return new wxAssert_wxArrayDouble(); } ) );
		cl.def_readwrite("TypeTooBigToBeStoredInwxBaseArrayDouble", &wxAssert_wxArrayDouble::TypeTooBigToBeStoredInwxBaseArrayDouble);
	}
	{ // wxArrayDouble file: line:1023
		pybind11::class_<wxArrayDouble, std::shared_ptr<wxArrayDouble>, wxBaseArrayDouble> cl(M(""), "wxArrayDouble", "");
		cl.def( pybind11::init( [](){ return new wxArrayDouble(); } ) );
		cl.def( pybind11::init<unsigned long>(), pybind11::arg("n") );

		cl.def( pybind11::init<unsigned long, const double &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init<const double *, const double *>(), pybind11::arg("first"), pybind11::arg("last") );

		cl.def( pybind11::init( [](wxArrayDouble const &o){ return new wxArrayDouble(o); } ) );
		cl.def("__getitem__", (double & (wxArrayDouble::*)(unsigned long) const) &wxArrayDouble::operator[], "C++: wxArrayDouble::operator[](unsigned long) const --> double &", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Item", (double & (wxArrayDouble::*)(unsigned long) const) &wxArrayDouble::Item, "C++: wxArrayDouble::Item(unsigned long) const --> double &", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Last", (double & (wxArrayDouble::*)() const) &wxArrayDouble::Last, "C++: wxArrayDouble::Last() const --> double &", pybind11::return_value_policy::automatic);
		cl.def("Index", [](wxArrayDouble const &o, double const & a0) -> int { return o.Index(a0); }, "", pybind11::arg("lItem"));
		cl.def("Index", (int (wxArrayDouble::*)(double, bool) const) &wxArrayDouble::Index, "C++: wxArrayDouble::Index(double, bool) const --> int", pybind11::arg("lItem"), pybind11::arg("bFromEnd"));
		cl.def("Add", [](wxArrayDouble &o, double const & a0) -> void { return o.Add(a0); }, "", pybind11::arg("lItem"));
		cl.def("Add", (void (wxArrayDouble::*)(double, unsigned long)) &wxArrayDouble::Add, "C++: wxArrayDouble::Add(double, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("nInsert"));
		cl.def("Insert", [](wxArrayDouble &o, double const & a0, unsigned long const & a1) -> void { return o.Insert(a0, a1); }, "", pybind11::arg("lItem"), pybind11::arg("uiIndex"));
		cl.def("Insert", (void (wxArrayDouble::*)(double, unsigned long, unsigned long)) &wxArrayDouble::Insert, "C++: wxArrayDouble::Insert(double, unsigned long, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("uiIndex"), pybind11::arg("nInsert"));
		cl.def("RemoveAt", [](wxArrayDouble &o, unsigned long const & a0) -> void { return o.RemoveAt(a0); }, "", pybind11::arg("uiIndex"));
		cl.def("RemoveAt", (void (wxArrayDouble::*)(unsigned long, unsigned long)) &wxArrayDouble::RemoveAt, "C++: wxArrayDouble::RemoveAt(unsigned long, unsigned long) --> void", pybind11::arg("uiIndex"), pybind11::arg("nRemove"));
		cl.def("Remove", (void (wxArrayDouble::*)(double)) &wxArrayDouble::Remove, "C++: wxArrayDouble::Remove(double) --> void", pybind11::arg("lItem"));
		cl.def("assign", (void (wxArrayDouble::*)(const double *, const double *)) &wxArrayDouble::assign, "C++: wxArrayDouble::assign(const double *, const double *) --> void", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("assign", (void (wxArrayDouble::*)(unsigned long, const double &)) &wxArrayDouble::assign, "C++: wxArrayDouble::assign(unsigned long, const double &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("back", (double & (wxArrayDouble::*)()) &wxArrayDouble::back, "C++: wxArrayDouble::back() --> double &", pybind11::return_value_policy::automatic);
		cl.def("begin", (double * (wxArrayDouble::*)()) &wxArrayDouble::begin, "C++: wxArrayDouble::begin() --> double *", pybind11::return_value_policy::automatic);
		cl.def("capacity", (unsigned long (wxArrayDouble::*)() const) &wxArrayDouble::capacity, "C++: wxArrayDouble::capacity() const --> unsigned long");
		cl.def("end", (double * (wxArrayDouble::*)()) &wxArrayDouble::end, "C++: wxArrayDouble::end() --> double *", pybind11::return_value_policy::automatic);
		cl.def("erase", (double * (wxArrayDouble::*)(double *, double *)) &wxArrayDouble::erase, "C++: wxArrayDouble::erase(double *, double *) --> double *", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"));
		cl.def("erase", (double * (wxArrayDouble::*)(double *)) &wxArrayDouble::erase, "C++: wxArrayDouble::erase(double *) --> double *", pybind11::return_value_policy::automatic, pybind11::arg("it"));
		cl.def("front", (double & (wxArrayDouble::*)()) &wxArrayDouble::front, "C++: wxArrayDouble::front() --> double &", pybind11::return_value_policy::automatic);
		cl.def("insert", (void (wxArrayDouble::*)(double *, unsigned long, const double &)) &wxArrayDouble::insert, "C++: wxArrayDouble::insert(double *, unsigned long, const double &) --> void", pybind11::arg("it"), pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", [](wxArrayDouble &o, double * a0) -> double * { return o.insert(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("it"));
		cl.def("insert", (double * (wxArrayDouble::*)(double *, const double &)) &wxArrayDouble::insert, "C++: wxArrayDouble::insert(double *, const double &) --> double *", pybind11::return_value_policy::automatic, pybind11::arg("it"), pybind11::arg("v"));
		cl.def("insert", (void (wxArrayDouble::*)(double *, const double *, const double *)) &wxArrayDouble::insert, "C++: wxArrayDouble::insert(double *, const double *, const double *) --> void", pybind11::arg("it"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("pop_back", (void (wxArrayDouble::*)()) &wxArrayDouble::pop_back, "C++: wxArrayDouble::pop_back() --> void");
		cl.def("push_back", (void (wxArrayDouble::*)(const double &)) &wxArrayDouble::push_back, "C++: wxArrayDouble::push_back(const double &) --> void", pybind11::arg("v"));
		cl.def("rbegin", (class wxArrayDouble::reverse_iterator (wxArrayDouble::*)()) &wxArrayDouble::rbegin, "C++: wxArrayDouble::rbegin() --> class wxArrayDouble::reverse_iterator");
		cl.def("rend", (class wxArrayDouble::reverse_iterator (wxArrayDouble::*)()) &wxArrayDouble::rend, "C++: wxArrayDouble::rend() --> class wxArrayDouble::reverse_iterator");
		cl.def("reserve", (void (wxArrayDouble::*)(unsigned long)) &wxArrayDouble::reserve, "C++: wxArrayDouble::reserve(unsigned long) --> void", pybind11::arg("n"));
		cl.def("resize", [](wxArrayDouble &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxArrayDouble::*)(unsigned long, double)) &wxArrayDouble::resize, "C++: wxArrayDouble::resize(unsigned long, double) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("swap", (void (wxArrayDouble::*)(class wxArrayDouble &)) &wxArrayDouble::swap, "C++: wxArrayDouble::swap(class wxArrayDouble &) --> void", pybind11::arg("other"));
		cl.def("assign", (class wxArrayDouble & (wxArrayDouble::*)(const class wxArrayDouble &)) &wxArrayDouble::operator=, "C++: wxArrayDouble::operator=(const class wxArrayDouble &) --> class wxArrayDouble &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxArrayDouble::reverse_iterator file: line:400
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayDouble::reverse_iterator, std::shared_ptr<wxArrayDouble::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayDouble::reverse_iterator(); } ) );
			cl.def( pybind11::init<double *>(), pybind11::arg("ptr") );

			cl.def( pybind11::init( [](wxArrayDouble::reverse_iterator const &o){ return new wxArrayDouble::reverse_iterator(o); } ) );
			cl.def("__mul__", (double & (wxArrayDouble::reverse_iterator::*)() const) &wxArrayDouble::reverse_iterator::operator*, "C++: wxArrayDouble::reverse_iterator::operator*() const --> double &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayDouble::reverse_iterator & (wxArrayDouble::reverse_iterator::*)()) &wxArrayDouble::reverse_iterator::operator++, "C++: wxArrayDouble::reverse_iterator::operator++() --> class wxArrayDouble::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayDouble::reverse_iterator (wxArrayDouble::reverse_iterator::*)(int)) &wxArrayDouble::reverse_iterator::operator++, "C++: wxArrayDouble::reverse_iterator::operator++(int) --> const class wxArrayDouble::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayDouble::reverse_iterator & (wxArrayDouble::reverse_iterator::*)()) &wxArrayDouble::reverse_iterator::operator--, "C++: wxArrayDouble::reverse_iterator::operator--() --> class wxArrayDouble::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayDouble::reverse_iterator (wxArrayDouble::reverse_iterator::*)(int)) &wxArrayDouble::reverse_iterator::operator--, "C++: wxArrayDouble::reverse_iterator::operator--(int) --> const class wxArrayDouble::reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayDouble::reverse_iterator::*)(const class wxArrayDouble::reverse_iterator &) const) &wxArrayDouble::reverse_iterator::operator==, "C++: wxArrayDouble::reverse_iterator::operator==(const class wxArrayDouble::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayDouble::reverse_iterator::*)(const class wxArrayDouble::reverse_iterator &) const) &wxArrayDouble::reverse_iterator::operator!=, "C++: wxArrayDouble::reverse_iterator::operator!=(const class wxArrayDouble::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxArrayDouble::const_reverse_iterator file: line:432
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayDouble::const_reverse_iterator, std::shared_ptr<wxArrayDouble::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayDouble::const_reverse_iterator(); } ) );
			cl.def( pybind11::init<const double *>(), pybind11::arg("ptr") );

			cl.def( pybind11::init( [](wxArrayDouble::const_reverse_iterator const &o){ return new wxArrayDouble::const_reverse_iterator(o); } ) );
			cl.def( pybind11::init<const class wxArrayDouble::reverse_iterator &>(), pybind11::arg("it") );

			cl.def("__mul__", (const double & (wxArrayDouble::const_reverse_iterator::*)() const) &wxArrayDouble::const_reverse_iterator::operator*, "C++: wxArrayDouble::const_reverse_iterator::operator*() const --> const double &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayDouble::const_reverse_iterator & (wxArrayDouble::const_reverse_iterator::*)()) &wxArrayDouble::const_reverse_iterator::operator++, "C++: wxArrayDouble::const_reverse_iterator::operator++() --> class wxArrayDouble::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayDouble::const_reverse_iterator (wxArrayDouble::const_reverse_iterator::*)(int)) &wxArrayDouble::const_reverse_iterator::operator++, "C++: wxArrayDouble::const_reverse_iterator::operator++(int) --> const class wxArrayDouble::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayDouble::const_reverse_iterator & (wxArrayDouble::const_reverse_iterator::*)()) &wxArrayDouble::const_reverse_iterator::operator--, "C++: wxArrayDouble::const_reverse_iterator::operator--() --> class wxArrayDouble::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayDouble::const_reverse_iterator (wxArrayDouble::const_reverse_iterator::*)(int)) &wxArrayDouble::const_reverse_iterator::operator--, "C++: wxArrayDouble::const_reverse_iterator::operator--(int) --> const class wxArrayDouble::const_reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayDouble::const_reverse_iterator::*)(const class wxArrayDouble::const_reverse_iterator &) const) &wxArrayDouble::const_reverse_iterator::operator==, "C++: wxArrayDouble::const_reverse_iterator::operator==(const class wxArrayDouble::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayDouble::const_reverse_iterator::*)(const class wxArrayDouble::const_reverse_iterator &) const) &wxArrayDouble::const_reverse_iterator::operator!=, "C++: wxArrayDouble::const_reverse_iterator::operator!=(const class wxArrayDouble::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
}

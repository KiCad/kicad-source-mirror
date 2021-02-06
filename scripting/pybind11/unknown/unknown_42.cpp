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

void bind_unknown_unknown_42(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxBaseArrayShort file: line:840
		pybind11::class_<wxBaseArrayShort, std::shared_ptr<wxBaseArrayShort>> cl(M(""), "wxBaseArrayShort", "");
		cl.def( pybind11::init( [](){ return new wxBaseArrayShort(); } ) );
		cl.def( pybind11::init( [](wxBaseArrayShort const &o){ return new wxBaseArrayShort(o); } ) );
		cl.def("assign", (class wxBaseArrayShort & (wxBaseArrayShort::*)(const class wxBaseArrayShort &)) &wxBaseArrayShort::operator=, "C++: wxBaseArrayShort::operator=(const class wxBaseArrayShort &) --> class wxBaseArrayShort &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("Empty", (void (wxBaseArrayShort::*)()) &wxBaseArrayShort::Empty, "C++: wxBaseArrayShort::Empty() --> void");
		cl.def("Clear", (void (wxBaseArrayShort::*)()) &wxBaseArrayShort::Clear, "C++: wxBaseArrayShort::Clear() --> void");
		cl.def("Alloc", (void (wxBaseArrayShort::*)(unsigned long)) &wxBaseArrayShort::Alloc, "C++: wxBaseArrayShort::Alloc(unsigned long) --> void", pybind11::arg("n"));
		cl.def("Shrink", (void (wxBaseArrayShort::*)()) &wxBaseArrayShort::Shrink, "C++: wxBaseArrayShort::Shrink() --> void");
		cl.def("GetCount", (unsigned long (wxBaseArrayShort::*)() const) &wxBaseArrayShort::GetCount, "C++: wxBaseArrayShort::GetCount() const --> unsigned long");
		cl.def("SetCount", [](wxBaseArrayShort &o, unsigned long const & a0) -> void { return o.SetCount(a0); }, "", pybind11::arg("n"));
		cl.def("SetCount", (void (wxBaseArrayShort::*)(unsigned long, short)) &wxBaseArrayShort::SetCount, "C++: wxBaseArrayShort::SetCount(unsigned long, short) --> void", pybind11::arg("n"), pybind11::arg("defval"));
		cl.def("IsEmpty", (bool (wxBaseArrayShort::*)() const) &wxBaseArrayShort::IsEmpty, "C++: wxBaseArrayShort::IsEmpty() const --> bool");
		cl.def("Count", (unsigned long (wxBaseArrayShort::*)() const) &wxBaseArrayShort::Count, "C++: wxBaseArrayShort::Count() const --> unsigned long");
		cl.def("clear", (void (wxBaseArrayShort::*)()) &wxBaseArrayShort::clear, "C++: wxBaseArrayShort::clear() --> void");
		cl.def("empty", (bool (wxBaseArrayShort::*)() const) &wxBaseArrayShort::empty, "C++: wxBaseArrayShort::empty() const --> bool");
		cl.def("max_size", (unsigned long (wxBaseArrayShort::*)() const) &wxBaseArrayShort::max_size, "C++: wxBaseArrayShort::max_size() const --> unsigned long");
		cl.def("size", (unsigned long (wxBaseArrayShort::*)() const) &wxBaseArrayShort::size, "C++: wxBaseArrayShort::size() const --> unsigned long");
	}
	{ // wxBaseArrayInt file: line:841
		pybind11::class_<wxBaseArrayInt, std::shared_ptr<wxBaseArrayInt>> cl(M(""), "wxBaseArrayInt", "");
		cl.def( pybind11::init( [](){ return new wxBaseArrayInt(); } ) );
		cl.def( pybind11::init( [](wxBaseArrayInt const &o){ return new wxBaseArrayInt(o); } ) );
		cl.def("assign", (class wxBaseArrayInt & (wxBaseArrayInt::*)(const class wxBaseArrayInt &)) &wxBaseArrayInt::operator=, "C++: wxBaseArrayInt::operator=(const class wxBaseArrayInt &) --> class wxBaseArrayInt &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("Empty", (void (wxBaseArrayInt::*)()) &wxBaseArrayInt::Empty, "C++: wxBaseArrayInt::Empty() --> void");
		cl.def("Clear", (void (wxBaseArrayInt::*)()) &wxBaseArrayInt::Clear, "C++: wxBaseArrayInt::Clear() --> void");
		cl.def("Alloc", (void (wxBaseArrayInt::*)(unsigned long)) &wxBaseArrayInt::Alloc, "C++: wxBaseArrayInt::Alloc(unsigned long) --> void", pybind11::arg("n"));
		cl.def("Shrink", (void (wxBaseArrayInt::*)()) &wxBaseArrayInt::Shrink, "C++: wxBaseArrayInt::Shrink() --> void");
		cl.def("GetCount", (unsigned long (wxBaseArrayInt::*)() const) &wxBaseArrayInt::GetCount, "C++: wxBaseArrayInt::GetCount() const --> unsigned long");
		cl.def("SetCount", [](wxBaseArrayInt &o, unsigned long const & a0) -> void { return o.SetCount(a0); }, "", pybind11::arg("n"));
		cl.def("SetCount", (void (wxBaseArrayInt::*)(unsigned long, int)) &wxBaseArrayInt::SetCount, "C++: wxBaseArrayInt::SetCount(unsigned long, int) --> void", pybind11::arg("n"), pybind11::arg("defval"));
		cl.def("IsEmpty", (bool (wxBaseArrayInt::*)() const) &wxBaseArrayInt::IsEmpty, "C++: wxBaseArrayInt::IsEmpty() const --> bool");
		cl.def("Count", (unsigned long (wxBaseArrayInt::*)() const) &wxBaseArrayInt::Count, "C++: wxBaseArrayInt::Count() const --> unsigned long");
		cl.def("clear", (void (wxBaseArrayInt::*)()) &wxBaseArrayInt::clear, "C++: wxBaseArrayInt::clear() --> void");
		cl.def("empty", (bool (wxBaseArrayInt::*)() const) &wxBaseArrayInt::empty, "C++: wxBaseArrayInt::empty() const --> bool");
		cl.def("max_size", (unsigned long (wxBaseArrayInt::*)() const) &wxBaseArrayInt::max_size, "C++: wxBaseArrayInt::max_size() const --> unsigned long");
		cl.def("size", (unsigned long (wxBaseArrayInt::*)() const) &wxBaseArrayInt::size, "C++: wxBaseArrayInt::size() const --> unsigned long");
	}
	{ // wxBaseArrayLong file: line:842
		pybind11::class_<wxBaseArrayLong, std::shared_ptr<wxBaseArrayLong>> cl(M(""), "wxBaseArrayLong", "");
		cl.def( pybind11::init( [](){ return new wxBaseArrayLong(); } ) );
		cl.def( pybind11::init( [](wxBaseArrayLong const &o){ return new wxBaseArrayLong(o); } ) );
		cl.def("assign", (class wxBaseArrayLong & (wxBaseArrayLong::*)(const class wxBaseArrayLong &)) &wxBaseArrayLong::operator=, "C++: wxBaseArrayLong::operator=(const class wxBaseArrayLong &) --> class wxBaseArrayLong &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("Empty", (void (wxBaseArrayLong::*)()) &wxBaseArrayLong::Empty, "C++: wxBaseArrayLong::Empty() --> void");
		cl.def("Clear", (void (wxBaseArrayLong::*)()) &wxBaseArrayLong::Clear, "C++: wxBaseArrayLong::Clear() --> void");
		cl.def("Alloc", (void (wxBaseArrayLong::*)(unsigned long)) &wxBaseArrayLong::Alloc, "C++: wxBaseArrayLong::Alloc(unsigned long) --> void", pybind11::arg("n"));
		cl.def("Shrink", (void (wxBaseArrayLong::*)()) &wxBaseArrayLong::Shrink, "C++: wxBaseArrayLong::Shrink() --> void");
		cl.def("GetCount", (unsigned long (wxBaseArrayLong::*)() const) &wxBaseArrayLong::GetCount, "C++: wxBaseArrayLong::GetCount() const --> unsigned long");
		cl.def("SetCount", [](wxBaseArrayLong &o, unsigned long const & a0) -> void { return o.SetCount(a0); }, "", pybind11::arg("n"));
		cl.def("SetCount", (void (wxBaseArrayLong::*)(unsigned long, long)) &wxBaseArrayLong::SetCount, "C++: wxBaseArrayLong::SetCount(unsigned long, long) --> void", pybind11::arg("n"), pybind11::arg("defval"));
		cl.def("IsEmpty", (bool (wxBaseArrayLong::*)() const) &wxBaseArrayLong::IsEmpty, "C++: wxBaseArrayLong::IsEmpty() const --> bool");
		cl.def("Count", (unsigned long (wxBaseArrayLong::*)() const) &wxBaseArrayLong::Count, "C++: wxBaseArrayLong::Count() const --> unsigned long");
		cl.def("clear", (void (wxBaseArrayLong::*)()) &wxBaseArrayLong::clear, "C++: wxBaseArrayLong::clear() --> void");
		cl.def("empty", (bool (wxBaseArrayLong::*)() const) &wxBaseArrayLong::empty, "C++: wxBaseArrayLong::empty() const --> bool");
		cl.def("max_size", (unsigned long (wxBaseArrayLong::*)() const) &wxBaseArrayLong::max_size, "C++: wxBaseArrayLong::max_size() const --> unsigned long");
		cl.def("size", (unsigned long (wxBaseArrayLong::*)() const) &wxBaseArrayLong::size, "C++: wxBaseArrayLong::size() const --> unsigned long");
	}
	{ // wxBaseArraySizeT file: line:843
		pybind11::class_<wxBaseArraySizeT, std::shared_ptr<wxBaseArraySizeT>> cl(M(""), "wxBaseArraySizeT", "");
		cl.def( pybind11::init( [](){ return new wxBaseArraySizeT(); } ) );
		cl.def( pybind11::init( [](wxBaseArraySizeT const &o){ return new wxBaseArraySizeT(o); } ) );
		cl.def("assign", (class wxBaseArraySizeT & (wxBaseArraySizeT::*)(const class wxBaseArraySizeT &)) &wxBaseArraySizeT::operator=, "C++: wxBaseArraySizeT::operator=(const class wxBaseArraySizeT &) --> class wxBaseArraySizeT &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("Empty", (void (wxBaseArraySizeT::*)()) &wxBaseArraySizeT::Empty, "C++: wxBaseArraySizeT::Empty() --> void");
		cl.def("Clear", (void (wxBaseArraySizeT::*)()) &wxBaseArraySizeT::Clear, "C++: wxBaseArraySizeT::Clear() --> void");
		cl.def("Alloc", (void (wxBaseArraySizeT::*)(unsigned long)) &wxBaseArraySizeT::Alloc, "C++: wxBaseArraySizeT::Alloc(unsigned long) --> void", pybind11::arg("n"));
		cl.def("Shrink", (void (wxBaseArraySizeT::*)()) &wxBaseArraySizeT::Shrink, "C++: wxBaseArraySizeT::Shrink() --> void");
		cl.def("GetCount", (unsigned long (wxBaseArraySizeT::*)() const) &wxBaseArraySizeT::GetCount, "C++: wxBaseArraySizeT::GetCount() const --> unsigned long");
		cl.def("SetCount", [](wxBaseArraySizeT &o, unsigned long const & a0) -> void { return o.SetCount(a0); }, "", pybind11::arg("n"));
		cl.def("SetCount", (void (wxBaseArraySizeT::*)(unsigned long, unsigned long)) &wxBaseArraySizeT::SetCount, "C++: wxBaseArraySizeT::SetCount(unsigned long, unsigned long) --> void", pybind11::arg("n"), pybind11::arg("defval"));
		cl.def("IsEmpty", (bool (wxBaseArraySizeT::*)() const) &wxBaseArraySizeT::IsEmpty, "C++: wxBaseArraySizeT::IsEmpty() const --> bool");
		cl.def("Count", (unsigned long (wxBaseArraySizeT::*)() const) &wxBaseArraySizeT::Count, "C++: wxBaseArraySizeT::Count() const --> unsigned long");
		cl.def("clear", (void (wxBaseArraySizeT::*)()) &wxBaseArraySizeT::clear, "C++: wxBaseArraySizeT::clear() --> void");
		cl.def("empty", (bool (wxBaseArraySizeT::*)() const) &wxBaseArraySizeT::empty, "C++: wxBaseArraySizeT::empty() const --> bool");
		cl.def("max_size", (unsigned long (wxBaseArraySizeT::*)() const) &wxBaseArraySizeT::max_size, "C++: wxBaseArraySizeT::max_size() const --> unsigned long");
		cl.def("size", (unsigned long (wxBaseArraySizeT::*)() const) &wxBaseArraySizeT::size, "C++: wxBaseArraySizeT::size() const --> unsigned long");
	}
	{ // wxBaseArrayDouble file: line:844
		pybind11::class_<wxBaseArrayDouble, std::shared_ptr<wxBaseArrayDouble>> cl(M(""), "wxBaseArrayDouble", "");
		cl.def( pybind11::init( [](){ return new wxBaseArrayDouble(); } ) );
		cl.def( pybind11::init( [](wxBaseArrayDouble const &o){ return new wxBaseArrayDouble(o); } ) );
		cl.def("assign", (class wxBaseArrayDouble & (wxBaseArrayDouble::*)(const class wxBaseArrayDouble &)) &wxBaseArrayDouble::operator=, "C++: wxBaseArrayDouble::operator=(const class wxBaseArrayDouble &) --> class wxBaseArrayDouble &", pybind11::return_value_policy::automatic, pybind11::arg("src"));
		cl.def("Empty", (void (wxBaseArrayDouble::*)()) &wxBaseArrayDouble::Empty, "C++: wxBaseArrayDouble::Empty() --> void");
		cl.def("Clear", (void (wxBaseArrayDouble::*)()) &wxBaseArrayDouble::Clear, "C++: wxBaseArrayDouble::Clear() --> void");
		cl.def("Alloc", (void (wxBaseArrayDouble::*)(unsigned long)) &wxBaseArrayDouble::Alloc, "C++: wxBaseArrayDouble::Alloc(unsigned long) --> void", pybind11::arg("n"));
		cl.def("Shrink", (void (wxBaseArrayDouble::*)()) &wxBaseArrayDouble::Shrink, "C++: wxBaseArrayDouble::Shrink() --> void");
		cl.def("GetCount", (unsigned long (wxBaseArrayDouble::*)() const) &wxBaseArrayDouble::GetCount, "C++: wxBaseArrayDouble::GetCount() const --> unsigned long");
		cl.def("SetCount", [](wxBaseArrayDouble &o, unsigned long const & a0) -> void { return o.SetCount(a0); }, "", pybind11::arg("n"));
		cl.def("SetCount", (void (wxBaseArrayDouble::*)(unsigned long, double)) &wxBaseArrayDouble::SetCount, "C++: wxBaseArrayDouble::SetCount(unsigned long, double) --> void", pybind11::arg("n"), pybind11::arg("defval"));
		cl.def("IsEmpty", (bool (wxBaseArrayDouble::*)() const) &wxBaseArrayDouble::IsEmpty, "C++: wxBaseArrayDouble::IsEmpty() const --> bool");
		cl.def("Count", (unsigned long (wxBaseArrayDouble::*)() const) &wxBaseArrayDouble::Count, "C++: wxBaseArrayDouble::Count() const --> unsigned long");
		cl.def("clear", (void (wxBaseArrayDouble::*)()) &wxBaseArrayDouble::clear, "C++: wxBaseArrayDouble::clear() --> void");
		cl.def("empty", (bool (wxBaseArrayDouble::*)() const) &wxBaseArrayDouble::empty, "C++: wxBaseArrayDouble::empty() const --> bool");
		cl.def("max_size", (unsigned long (wxBaseArrayDouble::*)() const) &wxBaseArrayDouble::max_size, "C++: wxBaseArrayDouble::max_size() const --> unsigned long");
		cl.def("size", (unsigned long (wxBaseArrayDouble::*)() const) &wxBaseArrayDouble::size, "C++: wxBaseArrayDouble::size() const --> unsigned long");
	}
	{ // wxAssert_wxArrayShort file: line:176
		pybind11::class_<wxAssert_wxArrayShort, std::shared_ptr<wxAssert_wxArrayShort>> cl(M(""), "wxAssert_wxArrayShort", "");
		cl.def( pybind11::init( [](){ return new wxAssert_wxArrayShort(); } ) );
		cl.def_readwrite("TypeTooBigToBeStoredInwxBaseArrayShort", &wxAssert_wxArrayShort::TypeTooBigToBeStoredInwxBaseArrayShort);
	}
	{ // wxArrayShort file: line:1021
		pybind11::class_<wxArrayShort, std::shared_ptr<wxArrayShort>, wxBaseArrayShort> cl(M(""), "wxArrayShort", "");
		cl.def( pybind11::init( [](){ return new wxArrayShort(); } ) );
		cl.def( pybind11::init<unsigned long>(), pybind11::arg("n") );

		cl.def( pybind11::init<unsigned long, const short &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init<const short *, const short *>(), pybind11::arg("first"), pybind11::arg("last") );

		cl.def( pybind11::init( [](wxArrayShort const &o){ return new wxArrayShort(o); } ) );
		cl.def("__getitem__", (short & (wxArrayShort::*)(unsigned long) const) &wxArrayShort::operator[], "C++: wxArrayShort::operator[](unsigned long) const --> short &", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Item", (short & (wxArrayShort::*)(unsigned long) const) &wxArrayShort::Item, "C++: wxArrayShort::Item(unsigned long) const --> short &", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Last", (short & (wxArrayShort::*)() const) &wxArrayShort::Last, "C++: wxArrayShort::Last() const --> short &", pybind11::return_value_policy::automatic);
		cl.def("Index", [](wxArrayShort const &o, short const & a0) -> int { return o.Index(a0); }, "", pybind11::arg("lItem"));
		cl.def("Index", (int (wxArrayShort::*)(short, bool) const) &wxArrayShort::Index, "C++: wxArrayShort::Index(short, bool) const --> int", pybind11::arg("lItem"), pybind11::arg("bFromEnd"));
		cl.def("Add", [](wxArrayShort &o, short const & a0) -> void { return o.Add(a0); }, "", pybind11::arg("lItem"));
		cl.def("Add", (void (wxArrayShort::*)(short, unsigned long)) &wxArrayShort::Add, "C++: wxArrayShort::Add(short, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("nInsert"));
		cl.def("Insert", [](wxArrayShort &o, short const & a0, unsigned long const & a1) -> void { return o.Insert(a0, a1); }, "", pybind11::arg("lItem"), pybind11::arg("uiIndex"));
		cl.def("Insert", (void (wxArrayShort::*)(short, unsigned long, unsigned long)) &wxArrayShort::Insert, "C++: wxArrayShort::Insert(short, unsigned long, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("uiIndex"), pybind11::arg("nInsert"));
		cl.def("RemoveAt", [](wxArrayShort &o, unsigned long const & a0) -> void { return o.RemoveAt(a0); }, "", pybind11::arg("uiIndex"));
		cl.def("RemoveAt", (void (wxArrayShort::*)(unsigned long, unsigned long)) &wxArrayShort::RemoveAt, "C++: wxArrayShort::RemoveAt(unsigned long, unsigned long) --> void", pybind11::arg("uiIndex"), pybind11::arg("nRemove"));
		cl.def("Remove", (void (wxArrayShort::*)(short)) &wxArrayShort::Remove, "C++: wxArrayShort::Remove(short) --> void", pybind11::arg("lItem"));
		cl.def("assign", (void (wxArrayShort::*)(const short *, const short *)) &wxArrayShort::assign, "C++: wxArrayShort::assign(const short *, const short *) --> void", pybind11::arg("first"), pybind11::arg("last"));
		cl.def("assign", (void (wxArrayShort::*)(unsigned long, const short &)) &wxArrayShort::assign, "C++: wxArrayShort::assign(unsigned long, const short &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("back", (short & (wxArrayShort::*)()) &wxArrayShort::back, "C++: wxArrayShort::back() --> short &", pybind11::return_value_policy::automatic);
		cl.def("begin", (short * (wxArrayShort::*)()) &wxArrayShort::begin, "C++: wxArrayShort::begin() --> short *", pybind11::return_value_policy::automatic);
		cl.def("capacity", (unsigned long (wxArrayShort::*)() const) &wxArrayShort::capacity, "C++: wxArrayShort::capacity() const --> unsigned long");
		cl.def("end", (short * (wxArrayShort::*)()) &wxArrayShort::end, "C++: wxArrayShort::end() --> short *", pybind11::return_value_policy::automatic);
		cl.def("erase", (short * (wxArrayShort::*)(short *, short *)) &wxArrayShort::erase, "C++: wxArrayShort::erase(short *, short *) --> short *", pybind11::return_value_policy::automatic, pybind11::arg("first"), pybind11::arg("last"));
		cl.def("erase", (short * (wxArrayShort::*)(short *)) &wxArrayShort::erase, "C++: wxArrayShort::erase(short *) --> short *", pybind11::return_value_policy::automatic, pybind11::arg("it"));
		cl.def("front", (short & (wxArrayShort::*)()) &wxArrayShort::front, "C++: wxArrayShort::front() --> short &", pybind11::return_value_policy::automatic);
		cl.def("insert", (void (wxArrayShort::*)(short *, unsigned long, const short &)) &wxArrayShort::insert, "C++: wxArrayShort::insert(short *, unsigned long, const short &) --> void", pybind11::arg("it"), pybind11::arg("n"), pybind11::arg("v"));
		cl.def("insert", [](wxArrayShort &o, short * a0) -> short * { return o.insert(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("it"));
		cl.def("insert", (short * (wxArrayShort::*)(short *, const short &)) &wxArrayShort::insert, "C++: wxArrayShort::insert(short *, const short &) --> short *", pybind11::return_value_policy::automatic, pybind11::arg("it"), pybind11::arg("v"));
		cl.def("insert", (void (wxArrayShort::*)(short *, const short *, const short *)) &wxArrayShort::insert, "C++: wxArrayShort::insert(short *, const short *, const short *) --> void", pybind11::arg("it"), pybind11::arg("first"), pybind11::arg("last"));
		cl.def("pop_back", (void (wxArrayShort::*)()) &wxArrayShort::pop_back, "C++: wxArrayShort::pop_back() --> void");
		cl.def("push_back", (void (wxArrayShort::*)(const short &)) &wxArrayShort::push_back, "C++: wxArrayShort::push_back(const short &) --> void", pybind11::arg("v"));
		cl.def("rbegin", (class wxArrayShort::reverse_iterator (wxArrayShort::*)()) &wxArrayShort::rbegin, "C++: wxArrayShort::rbegin() --> class wxArrayShort::reverse_iterator");
		cl.def("rend", (class wxArrayShort::reverse_iterator (wxArrayShort::*)()) &wxArrayShort::rend, "C++: wxArrayShort::rend() --> class wxArrayShort::reverse_iterator");
		cl.def("reserve", (void (wxArrayShort::*)(unsigned long)) &wxArrayShort::reserve, "C++: wxArrayShort::reserve(unsigned long) --> void", pybind11::arg("n"));
		cl.def("resize", [](wxArrayShort &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxArrayShort::*)(unsigned long, short)) &wxArrayShort::resize, "C++: wxArrayShort::resize(unsigned long, short) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("swap", (void (wxArrayShort::*)(class wxArrayShort &)) &wxArrayShort::swap, "C++: wxArrayShort::swap(class wxArrayShort &) --> void", pybind11::arg("other"));
		cl.def("assign", (class wxArrayShort & (wxArrayShort::*)(const class wxArrayShort &)) &wxArrayShort::operator=, "C++: wxArrayShort::operator=(const class wxArrayShort &) --> class wxArrayShort &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxArrayShort::reverse_iterator file: line:400
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayShort::reverse_iterator, std::shared_ptr<wxArrayShort::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayShort::reverse_iterator(); } ) );
			cl.def( pybind11::init<short *>(), pybind11::arg("ptr") );

			cl.def( pybind11::init( [](wxArrayShort::reverse_iterator const &o){ return new wxArrayShort::reverse_iterator(o); } ) );
			cl.def("__mul__", (short & (wxArrayShort::reverse_iterator::*)() const) &wxArrayShort::reverse_iterator::operator*, "C++: wxArrayShort::reverse_iterator::operator*() const --> short &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayShort::reverse_iterator & (wxArrayShort::reverse_iterator::*)()) &wxArrayShort::reverse_iterator::operator++, "C++: wxArrayShort::reverse_iterator::operator++() --> class wxArrayShort::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayShort::reverse_iterator (wxArrayShort::reverse_iterator::*)(int)) &wxArrayShort::reverse_iterator::operator++, "C++: wxArrayShort::reverse_iterator::operator++(int) --> const class wxArrayShort::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayShort::reverse_iterator & (wxArrayShort::reverse_iterator::*)()) &wxArrayShort::reverse_iterator::operator--, "C++: wxArrayShort::reverse_iterator::operator--() --> class wxArrayShort::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayShort::reverse_iterator (wxArrayShort::reverse_iterator::*)(int)) &wxArrayShort::reverse_iterator::operator--, "C++: wxArrayShort::reverse_iterator::operator--(int) --> const class wxArrayShort::reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayShort::reverse_iterator::*)(const class wxArrayShort::reverse_iterator &) const) &wxArrayShort::reverse_iterator::operator==, "C++: wxArrayShort::reverse_iterator::operator==(const class wxArrayShort::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayShort::reverse_iterator::*)(const class wxArrayShort::reverse_iterator &) const) &wxArrayShort::reverse_iterator::operator!=, "C++: wxArrayShort::reverse_iterator::operator!=(const class wxArrayShort::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxArrayShort::const_reverse_iterator file: line:432
			auto & enclosing_class = cl;
			pybind11::class_<wxArrayShort::const_reverse_iterator, std::shared_ptr<wxArrayShort::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxArrayShort::const_reverse_iterator(); } ) );
			cl.def( pybind11::init<const short *>(), pybind11::arg("ptr") );

			cl.def( pybind11::init( [](wxArrayShort::const_reverse_iterator const &o){ return new wxArrayShort::const_reverse_iterator(o); } ) );
			cl.def( pybind11::init<const class wxArrayShort::reverse_iterator &>(), pybind11::arg("it") );

			cl.def("__mul__", (const short & (wxArrayShort::const_reverse_iterator::*)() const) &wxArrayShort::const_reverse_iterator::operator*, "C++: wxArrayShort::const_reverse_iterator::operator*() const --> const short &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxArrayShort::const_reverse_iterator & (wxArrayShort::const_reverse_iterator::*)()) &wxArrayShort::const_reverse_iterator::operator++, "C++: wxArrayShort::const_reverse_iterator::operator++() --> class wxArrayShort::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxArrayShort::const_reverse_iterator (wxArrayShort::const_reverse_iterator::*)(int)) &wxArrayShort::const_reverse_iterator::operator++, "C++: wxArrayShort::const_reverse_iterator::operator++(int) --> const class wxArrayShort::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxArrayShort::const_reverse_iterator & (wxArrayShort::const_reverse_iterator::*)()) &wxArrayShort::const_reverse_iterator::operator--, "C++: wxArrayShort::const_reverse_iterator::operator--() --> class wxArrayShort::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxArrayShort::const_reverse_iterator (wxArrayShort::const_reverse_iterator::*)(int)) &wxArrayShort::const_reverse_iterator::operator--, "C++: wxArrayShort::const_reverse_iterator::operator--(int) --> const class wxArrayShort::const_reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxArrayShort::const_reverse_iterator::*)(const class wxArrayShort::const_reverse_iterator &) const) &wxArrayShort::const_reverse_iterator::operator==, "C++: wxArrayShort::const_reverse_iterator::operator==(const class wxArrayShort::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxArrayShort::const_reverse_iterator::*)(const class wxArrayShort::const_reverse_iterator &) const) &wxArrayShort::const_reverse_iterator::operator!=, "C++: wxArrayShort::const_reverse_iterator::operator!=(const class wxArrayShort::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
}

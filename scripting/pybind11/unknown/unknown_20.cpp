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

void bind_unknown_unknown_20(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxPointerHash file: line:567
		pybind11::class_<wxPointerHash, std::shared_ptr<wxPointerHash>> cl(M(""), "wxPointerHash", "");
		cl.def( pybind11::init( [](){ return new wxPointerHash(); } ) );
		cl.def( pybind11::init( [](wxPointerHash const &o){ return new wxPointerHash(o); } ) );
		cl.def("__call__", (unsigned long (wxPointerHash::*)(const void *) const) &wxPointerHash::operator(), "C++: wxPointerHash::operator()(const void *) const --> unsigned long", pybind11::arg("k"));
		cl.def("assign", (struct wxPointerHash & (wxPointerHash::*)(const struct wxPointerHash &)) &wxPointerHash::operator=, "C++: wxPointerHash::operator=(const struct wxPointerHash &) --> struct wxPointerHash &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxPointerEqual file: line:580
		pybind11::class_<wxPointerEqual, std::shared_ptr<wxPointerEqual>> cl(M(""), "wxPointerEqual", "");
		cl.def( pybind11::init( [](){ return new wxPointerEqual(); } ) );
		cl.def( pybind11::init( [](wxPointerEqual const &o){ return new wxPointerEqual(o); } ) );
		cl.def("__call__", (bool (wxPointerEqual::*)(const void *, const void *) const) &wxPointerEqual::operator(), "C++: wxPointerEqual::operator()(const void *, const void *) const --> bool", pybind11::arg("a"), pybind11::arg("b"));
		cl.def("assign", (struct wxPointerEqual & (wxPointerEqual::*)(const struct wxPointerEqual &)) &wxPointerEqual::operator=, "C++: wxPointerEqual::operator=(const struct wxPointerEqual &) --> struct wxPointerEqual &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxStringHash file: line:589
		pybind11::class_<wxStringHash, std::shared_ptr<wxStringHash>> cl(M(""), "wxStringHash", "");
		cl.def( pybind11::init( [](){ return new wxStringHash(); } ) );
		cl.def( pybind11::init( [](wxStringHash const &o){ return new wxStringHash(o); } ) );
		cl.def("__call__", (unsigned long (wxStringHash::*)(const class wxString &) const) &wxStringHash::operator(), "C++: wxStringHash::operator()(const class wxString &) const --> unsigned long", pybind11::arg("x"));
		cl.def("__call__", (unsigned long (wxStringHash::*)(const wchar_t *) const) &wxStringHash::operator(), "C++: wxStringHash::operator()(const wchar_t *) const --> unsigned long", pybind11::arg("x"));
		cl.def("__call__", (unsigned long (wxStringHash::*)(const char *) const) &wxStringHash::operator(), "C++: wxStringHash::operator()(const char *) const --> unsigned long", pybind11::arg("x"));
		cl.def_static("wxCharStringHash", (unsigned long (*)(const wchar_t *)) &wxStringHash::wxCharStringHash, "C++: wxStringHash::wxCharStringHash(const wchar_t *) --> unsigned long", pybind11::arg("x"));
		cl.def_static("charStringHash", (unsigned long (*)(const char *)) &wxStringHash::charStringHash, "C++: wxStringHash::charStringHash(const char *) --> unsigned long", pybind11::arg("x"));
		cl.def_static("stringHash", (unsigned long (*)(const wchar_t *)) &wxStringHash::stringHash, "C++: wxStringHash::stringHash(const wchar_t *) --> unsigned long", pybind11::arg(""));
		cl.def_static("stringHash", (unsigned long (*)(const char *)) &wxStringHash::stringHash, "C++: wxStringHash::stringHash(const char *) --> unsigned long", pybind11::arg(""));
		cl.def("assign", (struct wxStringHash & (wxStringHash::*)(const struct wxStringHash &)) &wxStringHash::operator=, "C++: wxStringHash::operator=(const struct wxStringHash &) --> struct wxStringHash &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxStringEqual file: line:614
		pybind11::class_<wxStringEqual, std::shared_ptr<wxStringEqual>> cl(M(""), "wxStringEqual", "");
		cl.def( pybind11::init( [](){ return new wxStringEqual(); } ) );
		cl.def( pybind11::init( [](wxStringEqual const &o){ return new wxStringEqual(o); } ) );
		cl.def("__call__", (bool (wxStringEqual::*)(const class wxString &, const class wxString &) const) &wxStringEqual::operator(), "C++: wxStringEqual::operator()(const class wxString &, const class wxString &) const --> bool", pybind11::arg("a"), pybind11::arg("b"));
		cl.def("__call__", (bool (wxStringEqual::*)(const wchar_t *, const wchar_t *) const) &wxStringEqual::operator(), "C++: wxStringEqual::operator()(const wchar_t *, const wchar_t *) const --> bool", pybind11::arg("a"), pybind11::arg("b"));
		cl.def("__call__", (bool (wxStringEqual::*)(const char *, const char *) const) &wxStringEqual::operator(), "C++: wxStringEqual::operator()(const char *, const char *) const --> bool", pybind11::arg("a"), pybind11::arg("b"));
		cl.def("assign", (struct wxStringEqual & (wxStringEqual::*)(const struct wxStringEqual &)) &wxStringEqual::operator=, "C++: wxStringEqual::operator=(const struct wxStringEqual &) --> struct wxStringEqual &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxLongToLongHashMap_wxImplementation_Pair file: line:74
		pybind11::class_<wxLongToLongHashMap_wxImplementation_Pair, std::shared_ptr<wxLongToLongHashMap_wxImplementation_Pair>> cl(M(""), "wxLongToLongHashMap_wxImplementation_Pair", "");
		cl.def( pybind11::init<const long &, const long &>(), pybind11::arg("f"), pybind11::arg("s") );

		cl.def( pybind11::init( [](wxLongToLongHashMap_wxImplementation_Pair const &o){ return new wxLongToLongHashMap_wxImplementation_Pair(o); } ) );
		cl.def_readwrite("first", &wxLongToLongHashMap_wxImplementation_Pair::first);
		cl.def_readwrite("second", &wxLongToLongHashMap_wxImplementation_Pair::second);
	}
	{ // wxLongToLongHashMap_wxImplementation_KeyEx file: line:76
		pybind11::class_<wxLongToLongHashMap_wxImplementation_KeyEx, std::shared_ptr<wxLongToLongHashMap_wxImplementation_KeyEx>> cl(M(""), "wxLongToLongHashMap_wxImplementation_KeyEx", "");
		cl.def( pybind11::init( [](){ return new wxLongToLongHashMap_wxImplementation_KeyEx(); } ) );
		cl.def( pybind11::init( [](wxLongToLongHashMap_wxImplementation_KeyEx const &o){ return new wxLongToLongHashMap_wxImplementation_KeyEx(o); } ) );
		cl.def("__call__", (const long & (wxLongToLongHashMap_wxImplementation_KeyEx::*)(const class wxLongToLongHashMap_wxImplementation_Pair &) const) &wxLongToLongHashMap_wxImplementation_KeyEx::operator(), "C++: wxLongToLongHashMap_wxImplementation_KeyEx::operator()(const class wxLongToLongHashMap_wxImplementation_Pair &) const --> const long &", pybind11::return_value_policy::automatic, pybind11::arg("pair"));
		cl.def("assign", (class wxLongToLongHashMap_wxImplementation_KeyEx & (wxLongToLongHashMap_wxImplementation_KeyEx::*)(const class wxLongToLongHashMap_wxImplementation_KeyEx &)) &wxLongToLongHashMap_wxImplementation_KeyEx::operator=, "C++: wxLongToLongHashMap_wxImplementation_KeyEx::operator=(const class wxLongToLongHashMap_wxImplementation_KeyEx &) --> class wxLongToLongHashMap_wxImplementation_KeyEx &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxLongToLongHashMap_wxImplementation_HashTable file: line:79
		pybind11::class_<wxLongToLongHashMap_wxImplementation_HashTable, std::shared_ptr<wxLongToLongHashMap_wxImplementation_HashTable>> cl(M(""), "wxLongToLongHashMap_wxImplementation_HashTable", "");
		cl.def( pybind11::init( [](){ return new wxLongToLongHashMap_wxImplementation_HashTable(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxLongToLongHashMap_wxImplementation_HashTable(a0); } ), "doc" , pybind11::arg("sz"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxIntegerHash & a1){ return new wxLongToLongHashMap_wxImplementation_HashTable(a0, a1); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxIntegerHash & a1, const struct wxIntegerEqual & a2){ return new wxLongToLongHashMap_wxImplementation_HashTable(a0, a1, a2); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"));
		cl.def( pybind11::init<unsigned long, const struct wxIntegerHash &, const struct wxIntegerEqual &, const class wxLongToLongHashMap_wxImplementation_KeyEx &>(), pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"), pybind11::arg("k_ex") );

		cl.def( pybind11::init( [](wxLongToLongHashMap_wxImplementation_HashTable const &o){ return new wxLongToLongHashMap_wxImplementation_HashTable(o); } ) );
		cl.def("assign", (const class wxLongToLongHashMap_wxImplementation_HashTable & (wxLongToLongHashMap_wxImplementation_HashTable::*)(const class wxLongToLongHashMap_wxImplementation_HashTable &)) &wxLongToLongHashMap_wxImplementation_HashTable::operator=, "C++: wxLongToLongHashMap_wxImplementation_HashTable::operator=(const class wxLongToLongHashMap_wxImplementation_HashTable &) --> const class wxLongToLongHashMap_wxImplementation_HashTable &", pybind11::return_value_policy::automatic, pybind11::arg("ht"));
		cl.def("hash_funct", (struct wxIntegerHash (wxLongToLongHashMap_wxImplementation_HashTable::*)()) &wxLongToLongHashMap_wxImplementation_HashTable::hash_funct, "C++: wxLongToLongHashMap_wxImplementation_HashTable::hash_funct() --> struct wxIntegerHash");
		cl.def("key_eq", (struct wxIntegerEqual (wxLongToLongHashMap_wxImplementation_HashTable::*)()) &wxLongToLongHashMap_wxImplementation_HashTable::key_eq, "C++: wxLongToLongHashMap_wxImplementation_HashTable::key_eq() --> struct wxIntegerEqual");
		cl.def("clear", (void (wxLongToLongHashMap_wxImplementation_HashTable::*)()) &wxLongToLongHashMap_wxImplementation_HashTable::clear, "C++: wxLongToLongHashMap_wxImplementation_HashTable::clear() --> void");
		cl.def("size", (unsigned long (wxLongToLongHashMap_wxImplementation_HashTable::*)() const) &wxLongToLongHashMap_wxImplementation_HashTable::size, "C++: wxLongToLongHashMap_wxImplementation_HashTable::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxLongToLongHashMap_wxImplementation_HashTable::*)() const) &wxLongToLongHashMap_wxImplementation_HashTable::max_size, "C++: wxLongToLongHashMap_wxImplementation_HashTable::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxLongToLongHashMap_wxImplementation_HashTable::*)() const) &wxLongToLongHashMap_wxImplementation_HashTable::empty, "C++: wxLongToLongHashMap_wxImplementation_HashTable::empty() const --> bool");
		cl.def("end", (class wxLongToLongHashMap_wxImplementation_HashTable::iterator (wxLongToLongHashMap_wxImplementation_HashTable::*)()) &wxLongToLongHashMap_wxImplementation_HashTable::end, "C++: wxLongToLongHashMap_wxImplementation_HashTable::end() --> class wxLongToLongHashMap_wxImplementation_HashTable::iterator");
		cl.def("begin", (class wxLongToLongHashMap_wxImplementation_HashTable::iterator (wxLongToLongHashMap_wxImplementation_HashTable::*)()) &wxLongToLongHashMap_wxImplementation_HashTable::begin, "C++: wxLongToLongHashMap_wxImplementation_HashTable::begin() --> class wxLongToLongHashMap_wxImplementation_HashTable::iterator");
		cl.def("erase", (unsigned long (wxLongToLongHashMap_wxImplementation_HashTable::*)(const long &)) &wxLongToLongHashMap_wxImplementation_HashTable::erase, "C++: wxLongToLongHashMap_wxImplementation_HashTable::erase(const long &) --> unsigned long", pybind11::arg("key"));

		{ // wxLongToLongHashMap_wxImplementation_HashTable::Node file: line:160
			auto & enclosing_class = cl;
			pybind11::class_<wxLongToLongHashMap_wxImplementation_HashTable::Node, std::shared_ptr<wxLongToLongHashMap_wxImplementation_HashTable::Node>, _wxHashTable_NodeBase> cl(enclosing_class, "Node", "");
			cl.def( pybind11::init<const class wxLongToLongHashMap_wxImplementation_Pair &>(), pybind11::arg("value") );

			cl.def( pybind11::init( [](wxLongToLongHashMap_wxImplementation_HashTable::Node const &o){ return new wxLongToLongHashMap_wxImplementation_HashTable::Node(o); } ) );
			cl.def_readwrite("m_value", &wxLongToLongHashMap_wxImplementation_HashTable::Node::m_value);
			cl.def("next", (struct wxLongToLongHashMap_wxImplementation_HashTable::Node * (wxLongToLongHashMap_wxImplementation_HashTable::Node::*)()) &wxLongToLongHashMap_wxImplementation_HashTable::Node::next, "C++: wxLongToLongHashMap_wxImplementation_HashTable::Node::next() --> struct wxLongToLongHashMap_wxImplementation_HashTable::Node *", pybind11::return_value_policy::automatic);
		}

		{ // wxLongToLongHashMap_wxImplementation_HashTable::Iterator file: line:179
			auto & enclosing_class = cl;
			pybind11::class_<wxLongToLongHashMap_wxImplementation_HashTable::Iterator, std::shared_ptr<wxLongToLongHashMap_wxImplementation_HashTable::Iterator>> cl(enclosing_class, "Iterator", "");
			cl.def( pybind11::init( [](){ return new wxLongToLongHashMap_wxImplementation_HashTable::Iterator(); } ) );
			cl.def( pybind11::init<struct wxLongToLongHashMap_wxImplementation_HashTable::Node *, const class wxLongToLongHashMap_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxLongToLongHashMap_wxImplementation_HashTable::Iterator const &o){ return new wxLongToLongHashMap_wxImplementation_HashTable::Iterator(o); } ) );
			cl.def("__eq__", (bool (wxLongToLongHashMap_wxImplementation_HashTable::Iterator::*)(const class wxLongToLongHashMap_wxImplementation_HashTable::Iterator &) const) &wxLongToLongHashMap_wxImplementation_HashTable::Iterator::operator==, "C++: wxLongToLongHashMap_wxImplementation_HashTable::Iterator::operator==(const class wxLongToLongHashMap_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxLongToLongHashMap_wxImplementation_HashTable::Iterator::*)(const class wxLongToLongHashMap_wxImplementation_HashTable::Iterator &) const) &wxLongToLongHashMap_wxImplementation_HashTable::Iterator::operator!=, "C++: wxLongToLongHashMap_wxImplementation_HashTable::Iterator::operator!=(const class wxLongToLongHashMap_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxLongToLongHashMap_wxImplementation_HashTable::iterator file: line:213
			auto & enclosing_class = cl;
			pybind11::class_<wxLongToLongHashMap_wxImplementation_HashTable::iterator, std::shared_ptr<wxLongToLongHashMap_wxImplementation_HashTable::iterator>, wxLongToLongHashMap_wxImplementation_HashTable::Iterator> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init( [](){ return new wxLongToLongHashMap_wxImplementation_HashTable::iterator(); } ) );
			cl.def( pybind11::init<struct wxLongToLongHashMap_wxImplementation_HashTable::Node *, class wxLongToLongHashMap_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxLongToLongHashMap_wxImplementation_HashTable::iterator const &o){ return new wxLongToLongHashMap_wxImplementation_HashTable::iterator(o); } ) );
			cl.def("plus_plus", (class wxLongToLongHashMap_wxImplementation_HashTable::iterator & (wxLongToLongHashMap_wxImplementation_HashTable::iterator::*)()) &wxLongToLongHashMap_wxImplementation_HashTable::iterator::operator++, "C++: wxLongToLongHashMap_wxImplementation_HashTable::iterator::operator++() --> class wxLongToLongHashMap_wxImplementation_HashTable::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxLongToLongHashMap_wxImplementation_HashTable::iterator (wxLongToLongHashMap_wxImplementation_HashTable::iterator::*)(int)) &wxLongToLongHashMap_wxImplementation_HashTable::iterator::operator++, "C++: wxLongToLongHashMap_wxImplementation_HashTable::iterator::operator++(int) --> class wxLongToLongHashMap_wxImplementation_HashTable::iterator", pybind11::arg(""));
			cl.def("__mul__", (class wxLongToLongHashMap_wxImplementation_Pair & (wxLongToLongHashMap_wxImplementation_HashTable::iterator::*)() const) &wxLongToLongHashMap_wxImplementation_HashTable::iterator::operator*, "C++: wxLongToLongHashMap_wxImplementation_HashTable::iterator::operator*() const --> class wxLongToLongHashMap_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

		{ // wxLongToLongHashMap_wxImplementation_HashTable::const_iterator file: line:224
			auto & enclosing_class = cl;
			pybind11::class_<wxLongToLongHashMap_wxImplementation_HashTable::const_iterator, std::shared_ptr<wxLongToLongHashMap_wxImplementation_HashTable::const_iterator>, wxLongToLongHashMap_wxImplementation_HashTable::Iterator> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init( [](){ return new wxLongToLongHashMap_wxImplementation_HashTable::const_iterator(); } ) );
			cl.def( pybind11::init<class wxLongToLongHashMap_wxImplementation_HashTable::iterator>(), pybind11::arg("i") );

			cl.def( pybind11::init<struct wxLongToLongHashMap_wxImplementation_HashTable::Node *, const class wxLongToLongHashMap_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxLongToLongHashMap_wxImplementation_HashTable::const_iterator const &o){ return new wxLongToLongHashMap_wxImplementation_HashTable::const_iterator(o); } ) );
			cl.def("plus_plus", (class wxLongToLongHashMap_wxImplementation_HashTable::const_iterator & (wxLongToLongHashMap_wxImplementation_HashTable::const_iterator::*)()) &wxLongToLongHashMap_wxImplementation_HashTable::const_iterator::operator++, "C++: wxLongToLongHashMap_wxImplementation_HashTable::const_iterator::operator++() --> class wxLongToLongHashMap_wxImplementation_HashTable::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxLongToLongHashMap_wxImplementation_HashTable::const_iterator (wxLongToLongHashMap_wxImplementation_HashTable::const_iterator::*)(int)) &wxLongToLongHashMap_wxImplementation_HashTable::const_iterator::operator++, "C++: wxLongToLongHashMap_wxImplementation_HashTable::const_iterator::operator++(int) --> class wxLongToLongHashMap_wxImplementation_HashTable::const_iterator", pybind11::arg(""));
			cl.def("__mul__", (const class wxLongToLongHashMap_wxImplementation_Pair & (wxLongToLongHashMap_wxImplementation_HashTable::const_iterator::*)() const) &wxLongToLongHashMap_wxImplementation_HashTable::const_iterator::operator*, "C++: wxLongToLongHashMap_wxImplementation_HashTable::const_iterator::operator*() const --> const class wxLongToLongHashMap_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

	}
}

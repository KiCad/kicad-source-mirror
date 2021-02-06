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

void bind_unknown_unknown_21(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxLongToLongHashMap file: line:747
		pybind11::class_<wxLongToLongHashMap, std::shared_ptr<wxLongToLongHashMap>, wxLongToLongHashMap_wxImplementation_HashTable> cl(M(""), "wxLongToLongHashMap", "");
		cl.def( pybind11::init( [](){ return new wxLongToLongHashMap(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxLongToLongHashMap(a0); } ), "doc" , pybind11::arg("hint"));
		cl.def( pybind11::init( [](unsigned long const & a0, struct wxIntegerHash const & a1){ return new wxLongToLongHashMap(a0, a1); } ), "doc" , pybind11::arg("hint"), pybind11::arg("hf"));
		cl.def( pybind11::init<unsigned long, struct wxIntegerHash, struct wxIntegerEqual>(), pybind11::arg("hint"), pybind11::arg("hf"), pybind11::arg("eq") );

		cl.def( pybind11::init( [](wxLongToLongHashMap const &o){ return new wxLongToLongHashMap(o); } ) );
		cl.def("__getitem__", (long & (wxLongToLongHashMap::*)(const long &)) &wxLongToLongHashMap::operator[], "C++: wxLongToLongHashMap::operator[](const long &) --> long &", pybind11::return_value_policy::automatic, pybind11::arg("key"));
		cl.def("find", (class wxLongToLongHashMap_wxImplementation_HashTable::iterator (wxLongToLongHashMap::*)(const long &)) &wxLongToLongHashMap::find, "C++: wxLongToLongHashMap::find(const long &) --> class wxLongToLongHashMap_wxImplementation_HashTable::iterator", pybind11::arg("key"));
		cl.def("insert", (class wxLongToLongHashMap::Insert_Result (wxLongToLongHashMap::*)(const class wxLongToLongHashMap_wxImplementation_Pair &)) &wxLongToLongHashMap::insert, "C++: wxLongToLongHashMap::insert(const class wxLongToLongHashMap_wxImplementation_Pair &) --> class wxLongToLongHashMap::Insert_Result", pybind11::arg("v"));
		cl.def("erase", (unsigned long (wxLongToLongHashMap::*)(const long &)) &wxLongToLongHashMap::erase, "C++: wxLongToLongHashMap::erase(const long &) --> unsigned long", pybind11::arg("k"));
		cl.def("erase", (void (wxLongToLongHashMap::*)(const class wxLongToLongHashMap_wxImplementation_HashTable::iterator &)) &wxLongToLongHashMap::erase, "C++: wxLongToLongHashMap::erase(const class wxLongToLongHashMap_wxImplementation_HashTable::iterator &) --> void", pybind11::arg("it"));
		cl.def("count", (unsigned long (wxLongToLongHashMap::*)(const long &)) &wxLongToLongHashMap::count, "C++: wxLongToLongHashMap::count(const long &) --> unsigned long", pybind11::arg("key"));
		cl.def("assign", (class wxLongToLongHashMap & (wxLongToLongHashMap::*)(const class wxLongToLongHashMap &)) &wxLongToLongHashMap::operator=, "C++: wxLongToLongHashMap::operator=(const class wxLongToLongHashMap &) --> class wxLongToLongHashMap &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxLongToLongHashMap::Insert_Result file: line:645
			auto & enclosing_class = cl;
			pybind11::class_<wxLongToLongHashMap::Insert_Result, std::shared_ptr<wxLongToLongHashMap::Insert_Result>> cl(enclosing_class, "Insert_Result", "");
			cl.def( pybind11::init<const class wxLongToLongHashMap_wxImplementation_HashTable::iterator &, const bool &>(), pybind11::arg("f"), pybind11::arg("s") );

			cl.def( pybind11::init( [](wxLongToLongHashMap::Insert_Result const &o){ return new wxLongToLongHashMap::Insert_Result(o); } ) );
			cl.def_readwrite("first", &wxLongToLongHashMap::Insert_Result::first);
			cl.def_readwrite("second", &wxLongToLongHashMap::Insert_Result::second);
		}

	}
	{ // wxStringToStringHashMap_wxImplementation_Pair file: line:86
		pybind11::class_<wxStringToStringHashMap_wxImplementation_Pair, std::shared_ptr<wxStringToStringHashMap_wxImplementation_Pair>> cl(M(""), "wxStringToStringHashMap_wxImplementation_Pair", "");
		cl.def( pybind11::init<const class wxString &, const class wxString &>(), pybind11::arg("f"), pybind11::arg("s") );

		cl.def( pybind11::init( [](wxStringToStringHashMap_wxImplementation_Pair const &o){ return new wxStringToStringHashMap_wxImplementation_Pair(o); } ) );
		cl.def_readwrite("first", &wxStringToStringHashMap_wxImplementation_Pair::first);
		cl.def_readwrite("second", &wxStringToStringHashMap_wxImplementation_Pair::second);
		cl.def("assign", (class wxStringToStringHashMap_wxImplementation_Pair & (wxStringToStringHashMap_wxImplementation_Pair::*)(const class wxStringToStringHashMap_wxImplementation_Pair &)) &wxStringToStringHashMap_wxImplementation_Pair::operator=, "C++: wxStringToStringHashMap_wxImplementation_Pair::operator=(const class wxStringToStringHashMap_wxImplementation_Pair &) --> class wxStringToStringHashMap_wxImplementation_Pair &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxStringToStringHashMap_wxImplementation_KeyEx file: line:88
		pybind11::class_<wxStringToStringHashMap_wxImplementation_KeyEx, std::shared_ptr<wxStringToStringHashMap_wxImplementation_KeyEx>> cl(M(""), "wxStringToStringHashMap_wxImplementation_KeyEx", "");
		cl.def( pybind11::init( [](){ return new wxStringToStringHashMap_wxImplementation_KeyEx(); } ) );
		cl.def( pybind11::init( [](wxStringToStringHashMap_wxImplementation_KeyEx const &o){ return new wxStringToStringHashMap_wxImplementation_KeyEx(o); } ) );
		cl.def("__call__", (const class wxString & (wxStringToStringHashMap_wxImplementation_KeyEx::*)(const class wxStringToStringHashMap_wxImplementation_Pair &) const) &wxStringToStringHashMap_wxImplementation_KeyEx::operator(), "C++: wxStringToStringHashMap_wxImplementation_KeyEx::operator()(const class wxStringToStringHashMap_wxImplementation_Pair &) const --> const class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("pair"));
		cl.def("assign", (class wxStringToStringHashMap_wxImplementation_KeyEx & (wxStringToStringHashMap_wxImplementation_KeyEx::*)(const class wxStringToStringHashMap_wxImplementation_KeyEx &)) &wxStringToStringHashMap_wxImplementation_KeyEx::operator=, "C++: wxStringToStringHashMap_wxImplementation_KeyEx::operator=(const class wxStringToStringHashMap_wxImplementation_KeyEx &) --> class wxStringToStringHashMap_wxImplementation_KeyEx &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxStringToStringHashMap_wxImplementation_HashTable file: line:91
		pybind11::class_<wxStringToStringHashMap_wxImplementation_HashTable, std::shared_ptr<wxStringToStringHashMap_wxImplementation_HashTable>> cl(M(""), "wxStringToStringHashMap_wxImplementation_HashTable", "");
		cl.def( pybind11::init( [](){ return new wxStringToStringHashMap_wxImplementation_HashTable(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxStringToStringHashMap_wxImplementation_HashTable(a0); } ), "doc" , pybind11::arg("sz"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxStringHash & a1){ return new wxStringToStringHashMap_wxImplementation_HashTable(a0, a1); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxStringHash & a1, const struct wxStringEqual & a2){ return new wxStringToStringHashMap_wxImplementation_HashTable(a0, a1, a2); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"));
		cl.def( pybind11::init<unsigned long, const struct wxStringHash &, const struct wxStringEqual &, const class wxStringToStringHashMap_wxImplementation_KeyEx &>(), pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"), pybind11::arg("k_ex") );

		cl.def( pybind11::init( [](wxStringToStringHashMap_wxImplementation_HashTable const &o){ return new wxStringToStringHashMap_wxImplementation_HashTable(o); } ) );
		cl.def("assign", (const class wxStringToStringHashMap_wxImplementation_HashTable & (wxStringToStringHashMap_wxImplementation_HashTable::*)(const class wxStringToStringHashMap_wxImplementation_HashTable &)) &wxStringToStringHashMap_wxImplementation_HashTable::operator=, "C++: wxStringToStringHashMap_wxImplementation_HashTable::operator=(const class wxStringToStringHashMap_wxImplementation_HashTable &) --> const class wxStringToStringHashMap_wxImplementation_HashTable &", pybind11::return_value_policy::automatic, pybind11::arg("ht"));
		cl.def("hash_funct", (struct wxStringHash (wxStringToStringHashMap_wxImplementation_HashTable::*)()) &wxStringToStringHashMap_wxImplementation_HashTable::hash_funct, "C++: wxStringToStringHashMap_wxImplementation_HashTable::hash_funct() --> struct wxStringHash");
		cl.def("key_eq", (struct wxStringEqual (wxStringToStringHashMap_wxImplementation_HashTable::*)()) &wxStringToStringHashMap_wxImplementation_HashTable::key_eq, "C++: wxStringToStringHashMap_wxImplementation_HashTable::key_eq() --> struct wxStringEqual");
		cl.def("clear", (void (wxStringToStringHashMap_wxImplementation_HashTable::*)()) &wxStringToStringHashMap_wxImplementation_HashTable::clear, "C++: wxStringToStringHashMap_wxImplementation_HashTable::clear() --> void");
		cl.def("size", (unsigned long (wxStringToStringHashMap_wxImplementation_HashTable::*)() const) &wxStringToStringHashMap_wxImplementation_HashTable::size, "C++: wxStringToStringHashMap_wxImplementation_HashTable::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxStringToStringHashMap_wxImplementation_HashTable::*)() const) &wxStringToStringHashMap_wxImplementation_HashTable::max_size, "C++: wxStringToStringHashMap_wxImplementation_HashTable::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxStringToStringHashMap_wxImplementation_HashTable::*)() const) &wxStringToStringHashMap_wxImplementation_HashTable::empty, "C++: wxStringToStringHashMap_wxImplementation_HashTable::empty() const --> bool");
		cl.def("end", (class wxStringToStringHashMap_wxImplementation_HashTable::iterator (wxStringToStringHashMap_wxImplementation_HashTable::*)()) &wxStringToStringHashMap_wxImplementation_HashTable::end, "C++: wxStringToStringHashMap_wxImplementation_HashTable::end() --> class wxStringToStringHashMap_wxImplementation_HashTable::iterator");
		cl.def("begin", (class wxStringToStringHashMap_wxImplementation_HashTable::iterator (wxStringToStringHashMap_wxImplementation_HashTable::*)()) &wxStringToStringHashMap_wxImplementation_HashTable::begin, "C++: wxStringToStringHashMap_wxImplementation_HashTable::begin() --> class wxStringToStringHashMap_wxImplementation_HashTable::iterator");
		cl.def("erase", (unsigned long (wxStringToStringHashMap_wxImplementation_HashTable::*)(const class wxString &)) &wxStringToStringHashMap_wxImplementation_HashTable::erase, "C++: wxStringToStringHashMap_wxImplementation_HashTable::erase(const class wxString &) --> unsigned long", pybind11::arg("key"));

		{ // wxStringToStringHashMap_wxImplementation_HashTable::Node file: line:160
			auto & enclosing_class = cl;
			pybind11::class_<wxStringToStringHashMap_wxImplementation_HashTable::Node, std::shared_ptr<wxStringToStringHashMap_wxImplementation_HashTable::Node>, _wxHashTable_NodeBase> cl(enclosing_class, "Node", "");
			cl.def( pybind11::init<const class wxStringToStringHashMap_wxImplementation_Pair &>(), pybind11::arg("value") );

			cl.def( pybind11::init( [](wxStringToStringHashMap_wxImplementation_HashTable::Node const &o){ return new wxStringToStringHashMap_wxImplementation_HashTable::Node(o); } ) );
			cl.def_readwrite("m_value", &wxStringToStringHashMap_wxImplementation_HashTable::Node::m_value);
			cl.def("next", (struct wxStringToStringHashMap_wxImplementation_HashTable::Node * (wxStringToStringHashMap_wxImplementation_HashTable::Node::*)()) &wxStringToStringHashMap_wxImplementation_HashTable::Node::next, "C++: wxStringToStringHashMap_wxImplementation_HashTable::Node::next() --> struct wxStringToStringHashMap_wxImplementation_HashTable::Node *", pybind11::return_value_policy::automatic);
		}

		{ // wxStringToStringHashMap_wxImplementation_HashTable::Iterator file: line:179
			auto & enclosing_class = cl;
			pybind11::class_<wxStringToStringHashMap_wxImplementation_HashTable::Iterator, std::shared_ptr<wxStringToStringHashMap_wxImplementation_HashTable::Iterator>> cl(enclosing_class, "Iterator", "");
			cl.def( pybind11::init( [](){ return new wxStringToStringHashMap_wxImplementation_HashTable::Iterator(); } ) );
			cl.def( pybind11::init<struct wxStringToStringHashMap_wxImplementation_HashTable::Node *, const class wxStringToStringHashMap_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxStringToStringHashMap_wxImplementation_HashTable::Iterator const &o){ return new wxStringToStringHashMap_wxImplementation_HashTable::Iterator(o); } ) );
			cl.def("__eq__", (bool (wxStringToStringHashMap_wxImplementation_HashTable::Iterator::*)(const class wxStringToStringHashMap_wxImplementation_HashTable::Iterator &) const) &wxStringToStringHashMap_wxImplementation_HashTable::Iterator::operator==, "C++: wxStringToStringHashMap_wxImplementation_HashTable::Iterator::operator==(const class wxStringToStringHashMap_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxStringToStringHashMap_wxImplementation_HashTable::Iterator::*)(const class wxStringToStringHashMap_wxImplementation_HashTable::Iterator &) const) &wxStringToStringHashMap_wxImplementation_HashTable::Iterator::operator!=, "C++: wxStringToStringHashMap_wxImplementation_HashTable::Iterator::operator!=(const class wxStringToStringHashMap_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxStringToStringHashMap_wxImplementation_HashTable::iterator file: line:213
			auto & enclosing_class = cl;
			pybind11::class_<wxStringToStringHashMap_wxImplementation_HashTable::iterator, std::shared_ptr<wxStringToStringHashMap_wxImplementation_HashTable::iterator>, wxStringToStringHashMap_wxImplementation_HashTable::Iterator> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init( [](){ return new wxStringToStringHashMap_wxImplementation_HashTable::iterator(); } ) );
			cl.def( pybind11::init<struct wxStringToStringHashMap_wxImplementation_HashTable::Node *, class wxStringToStringHashMap_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxStringToStringHashMap_wxImplementation_HashTable::iterator const &o){ return new wxStringToStringHashMap_wxImplementation_HashTable::iterator(o); } ) );
			cl.def("plus_plus", (class wxStringToStringHashMap_wxImplementation_HashTable::iterator & (wxStringToStringHashMap_wxImplementation_HashTable::iterator::*)()) &wxStringToStringHashMap_wxImplementation_HashTable::iterator::operator++, "C++: wxStringToStringHashMap_wxImplementation_HashTable::iterator::operator++() --> class wxStringToStringHashMap_wxImplementation_HashTable::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxStringToStringHashMap_wxImplementation_HashTable::iterator (wxStringToStringHashMap_wxImplementation_HashTable::iterator::*)(int)) &wxStringToStringHashMap_wxImplementation_HashTable::iterator::operator++, "C++: wxStringToStringHashMap_wxImplementation_HashTable::iterator::operator++(int) --> class wxStringToStringHashMap_wxImplementation_HashTable::iterator", pybind11::arg(""));
			cl.def("__mul__", (class wxStringToStringHashMap_wxImplementation_Pair & (wxStringToStringHashMap_wxImplementation_HashTable::iterator::*)() const) &wxStringToStringHashMap_wxImplementation_HashTable::iterator::operator*, "C++: wxStringToStringHashMap_wxImplementation_HashTable::iterator::operator*() const --> class wxStringToStringHashMap_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

		{ // wxStringToStringHashMap_wxImplementation_HashTable::const_iterator file: line:224
			auto & enclosing_class = cl;
			pybind11::class_<wxStringToStringHashMap_wxImplementation_HashTable::const_iterator, std::shared_ptr<wxStringToStringHashMap_wxImplementation_HashTable::const_iterator>, wxStringToStringHashMap_wxImplementation_HashTable::Iterator> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init( [](){ return new wxStringToStringHashMap_wxImplementation_HashTable::const_iterator(); } ) );
			cl.def( pybind11::init<class wxStringToStringHashMap_wxImplementation_HashTable::iterator>(), pybind11::arg("i") );

			cl.def( pybind11::init<struct wxStringToStringHashMap_wxImplementation_HashTable::Node *, const class wxStringToStringHashMap_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxStringToStringHashMap_wxImplementation_HashTable::const_iterator const &o){ return new wxStringToStringHashMap_wxImplementation_HashTable::const_iterator(o); } ) );
			cl.def("plus_plus", (class wxStringToStringHashMap_wxImplementation_HashTable::const_iterator & (wxStringToStringHashMap_wxImplementation_HashTable::const_iterator::*)()) &wxStringToStringHashMap_wxImplementation_HashTable::const_iterator::operator++, "C++: wxStringToStringHashMap_wxImplementation_HashTable::const_iterator::operator++() --> class wxStringToStringHashMap_wxImplementation_HashTable::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxStringToStringHashMap_wxImplementation_HashTable::const_iterator (wxStringToStringHashMap_wxImplementation_HashTable::const_iterator::*)(int)) &wxStringToStringHashMap_wxImplementation_HashTable::const_iterator::operator++, "C++: wxStringToStringHashMap_wxImplementation_HashTable::const_iterator::operator++(int) --> class wxStringToStringHashMap_wxImplementation_HashTable::const_iterator", pybind11::arg(""));
			cl.def("__mul__", (const class wxStringToStringHashMap_wxImplementation_Pair & (wxStringToStringHashMap_wxImplementation_HashTable::const_iterator::*)() const) &wxStringToStringHashMap_wxImplementation_HashTable::const_iterator::operator*, "C++: wxStringToStringHashMap_wxImplementation_HashTable::const_iterator::operator*() const --> const class wxStringToStringHashMap_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

	}
}

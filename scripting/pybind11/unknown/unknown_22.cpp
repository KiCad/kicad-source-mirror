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

void bind_unknown_unknown_22(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxStringToStringHashMap file: line:749
		pybind11::class_<wxStringToStringHashMap, std::shared_ptr<wxStringToStringHashMap>, wxStringToStringHashMap_wxImplementation_HashTable> cl(M(""), "wxStringToStringHashMap", "");
		cl.def( pybind11::init( [](){ return new wxStringToStringHashMap(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxStringToStringHashMap(a0); } ), "doc" , pybind11::arg("hint"));
		cl.def( pybind11::init( [](unsigned long const & a0, struct wxStringHash const & a1){ return new wxStringToStringHashMap(a0, a1); } ), "doc" , pybind11::arg("hint"), pybind11::arg("hf"));
		cl.def( pybind11::init<unsigned long, struct wxStringHash, struct wxStringEqual>(), pybind11::arg("hint"), pybind11::arg("hf"), pybind11::arg("eq") );

		cl.def( pybind11::init( [](wxStringToStringHashMap const &o){ return new wxStringToStringHashMap(o); } ) );
		cl.def("__getitem__", (class wxString & (wxStringToStringHashMap::*)(const class wxString &)) &wxStringToStringHashMap::operator[], "C++: wxStringToStringHashMap::operator[](const class wxString &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("key"));
		cl.def("find", (class wxStringToStringHashMap_wxImplementation_HashTable::iterator (wxStringToStringHashMap::*)(const class wxString &)) &wxStringToStringHashMap::find, "C++: wxStringToStringHashMap::find(const class wxString &) --> class wxStringToStringHashMap_wxImplementation_HashTable::iterator", pybind11::arg("key"));
		cl.def("insert", (class wxStringToStringHashMap::Insert_Result (wxStringToStringHashMap::*)(const class wxStringToStringHashMap_wxImplementation_Pair &)) &wxStringToStringHashMap::insert, "C++: wxStringToStringHashMap::insert(const class wxStringToStringHashMap_wxImplementation_Pair &) --> class wxStringToStringHashMap::Insert_Result", pybind11::arg("v"));
		cl.def("erase", (unsigned long (wxStringToStringHashMap::*)(const class wxString &)) &wxStringToStringHashMap::erase, "C++: wxStringToStringHashMap::erase(const class wxString &) --> unsigned long", pybind11::arg("k"));
		cl.def("erase", (void (wxStringToStringHashMap::*)(const class wxStringToStringHashMap_wxImplementation_HashTable::iterator &)) &wxStringToStringHashMap::erase, "C++: wxStringToStringHashMap::erase(const class wxStringToStringHashMap_wxImplementation_HashTable::iterator &) --> void", pybind11::arg("it"));
		cl.def("count", (unsigned long (wxStringToStringHashMap::*)(const class wxString &)) &wxStringToStringHashMap::count, "C++: wxStringToStringHashMap::count(const class wxString &) --> unsigned long", pybind11::arg("key"));
		cl.def("assign", (class wxStringToStringHashMap & (wxStringToStringHashMap::*)(const class wxStringToStringHashMap &)) &wxStringToStringHashMap::operator=, "C++: wxStringToStringHashMap::operator=(const class wxStringToStringHashMap &) --> class wxStringToStringHashMap &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxStringToStringHashMap::Insert_Result file: line:645
			auto & enclosing_class = cl;
			pybind11::class_<wxStringToStringHashMap::Insert_Result, std::shared_ptr<wxStringToStringHashMap::Insert_Result>> cl(enclosing_class, "Insert_Result", "");
			cl.def( pybind11::init<const class wxStringToStringHashMap_wxImplementation_HashTable::iterator &, const bool &>(), pybind11::arg("f"), pybind11::arg("s") );

			cl.def( pybind11::init( [](wxStringToStringHashMap::Insert_Result const &o){ return new wxStringToStringHashMap::Insert_Result(o); } ) );
			cl.def_readwrite("first", &wxStringToStringHashMap::Insert_Result::first);
			cl.def_readwrite("second", &wxStringToStringHashMap::Insert_Result::second);
		}

	}
	{ // wxStringToNumHashMap_wxImplementation_Pair file: line:98
		pybind11::class_<wxStringToNumHashMap_wxImplementation_Pair, std::shared_ptr<wxStringToNumHashMap_wxImplementation_Pair>> cl(M(""), "wxStringToNumHashMap_wxImplementation_Pair", "");
		cl.def( pybind11::init<const class wxString &, const unsigned long &>(), pybind11::arg("f"), pybind11::arg("s") );

		cl.def( pybind11::init( [](wxStringToNumHashMap_wxImplementation_Pair const &o){ return new wxStringToNumHashMap_wxImplementation_Pair(o); } ) );
		cl.def_readwrite("first", &wxStringToNumHashMap_wxImplementation_Pair::first);
		cl.def_readwrite("second", &wxStringToNumHashMap_wxImplementation_Pair::second);
		cl.def("assign", (class wxStringToNumHashMap_wxImplementation_Pair & (wxStringToNumHashMap_wxImplementation_Pair::*)(const class wxStringToNumHashMap_wxImplementation_Pair &)) &wxStringToNumHashMap_wxImplementation_Pair::operator=, "C++: wxStringToNumHashMap_wxImplementation_Pair::operator=(const class wxStringToNumHashMap_wxImplementation_Pair &) --> class wxStringToNumHashMap_wxImplementation_Pair &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxStringToNumHashMap_wxImplementation_KeyEx file: line:100
		pybind11::class_<wxStringToNumHashMap_wxImplementation_KeyEx, std::shared_ptr<wxStringToNumHashMap_wxImplementation_KeyEx>> cl(M(""), "wxStringToNumHashMap_wxImplementation_KeyEx", "");
		cl.def( pybind11::init( [](){ return new wxStringToNumHashMap_wxImplementation_KeyEx(); } ) );
		cl.def( pybind11::init( [](wxStringToNumHashMap_wxImplementation_KeyEx const &o){ return new wxStringToNumHashMap_wxImplementation_KeyEx(o); } ) );
		cl.def("__call__", (const class wxString & (wxStringToNumHashMap_wxImplementation_KeyEx::*)(const class wxStringToNumHashMap_wxImplementation_Pair &) const) &wxStringToNumHashMap_wxImplementation_KeyEx::operator(), "C++: wxStringToNumHashMap_wxImplementation_KeyEx::operator()(const class wxStringToNumHashMap_wxImplementation_Pair &) const --> const class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("pair"));
		cl.def("assign", (class wxStringToNumHashMap_wxImplementation_KeyEx & (wxStringToNumHashMap_wxImplementation_KeyEx::*)(const class wxStringToNumHashMap_wxImplementation_KeyEx &)) &wxStringToNumHashMap_wxImplementation_KeyEx::operator=, "C++: wxStringToNumHashMap_wxImplementation_KeyEx::operator=(const class wxStringToNumHashMap_wxImplementation_KeyEx &) --> class wxStringToNumHashMap_wxImplementation_KeyEx &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxStringToNumHashMap_wxImplementation_HashTable file: line:103
		pybind11::class_<wxStringToNumHashMap_wxImplementation_HashTable, std::shared_ptr<wxStringToNumHashMap_wxImplementation_HashTable>> cl(M(""), "wxStringToNumHashMap_wxImplementation_HashTable", "");
		cl.def( pybind11::init( [](){ return new wxStringToNumHashMap_wxImplementation_HashTable(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxStringToNumHashMap_wxImplementation_HashTable(a0); } ), "doc" , pybind11::arg("sz"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxStringHash & a1){ return new wxStringToNumHashMap_wxImplementation_HashTable(a0, a1); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxStringHash & a1, const struct wxStringEqual & a2){ return new wxStringToNumHashMap_wxImplementation_HashTable(a0, a1, a2); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"));
		cl.def( pybind11::init<unsigned long, const struct wxStringHash &, const struct wxStringEqual &, const class wxStringToNumHashMap_wxImplementation_KeyEx &>(), pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"), pybind11::arg("k_ex") );

		cl.def( pybind11::init( [](wxStringToNumHashMap_wxImplementation_HashTable const &o){ return new wxStringToNumHashMap_wxImplementation_HashTable(o); } ) );
		cl.def("assign", (const class wxStringToNumHashMap_wxImplementation_HashTable & (wxStringToNumHashMap_wxImplementation_HashTable::*)(const class wxStringToNumHashMap_wxImplementation_HashTable &)) &wxStringToNumHashMap_wxImplementation_HashTable::operator=, "C++: wxStringToNumHashMap_wxImplementation_HashTable::operator=(const class wxStringToNumHashMap_wxImplementation_HashTable &) --> const class wxStringToNumHashMap_wxImplementation_HashTable &", pybind11::return_value_policy::automatic, pybind11::arg("ht"));
		cl.def("hash_funct", (struct wxStringHash (wxStringToNumHashMap_wxImplementation_HashTable::*)()) &wxStringToNumHashMap_wxImplementation_HashTable::hash_funct, "C++: wxStringToNumHashMap_wxImplementation_HashTable::hash_funct() --> struct wxStringHash");
		cl.def("key_eq", (struct wxStringEqual (wxStringToNumHashMap_wxImplementation_HashTable::*)()) &wxStringToNumHashMap_wxImplementation_HashTable::key_eq, "C++: wxStringToNumHashMap_wxImplementation_HashTable::key_eq() --> struct wxStringEqual");
		cl.def("clear", (void (wxStringToNumHashMap_wxImplementation_HashTable::*)()) &wxStringToNumHashMap_wxImplementation_HashTable::clear, "C++: wxStringToNumHashMap_wxImplementation_HashTable::clear() --> void");
		cl.def("size", (unsigned long (wxStringToNumHashMap_wxImplementation_HashTable::*)() const) &wxStringToNumHashMap_wxImplementation_HashTable::size, "C++: wxStringToNumHashMap_wxImplementation_HashTable::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxStringToNumHashMap_wxImplementation_HashTable::*)() const) &wxStringToNumHashMap_wxImplementation_HashTable::max_size, "C++: wxStringToNumHashMap_wxImplementation_HashTable::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxStringToNumHashMap_wxImplementation_HashTable::*)() const) &wxStringToNumHashMap_wxImplementation_HashTable::empty, "C++: wxStringToNumHashMap_wxImplementation_HashTable::empty() const --> bool");
		cl.def("end", (class wxStringToNumHashMap_wxImplementation_HashTable::iterator (wxStringToNumHashMap_wxImplementation_HashTable::*)()) &wxStringToNumHashMap_wxImplementation_HashTable::end, "C++: wxStringToNumHashMap_wxImplementation_HashTable::end() --> class wxStringToNumHashMap_wxImplementation_HashTable::iterator");
		cl.def("begin", (class wxStringToNumHashMap_wxImplementation_HashTable::iterator (wxStringToNumHashMap_wxImplementation_HashTable::*)()) &wxStringToNumHashMap_wxImplementation_HashTable::begin, "C++: wxStringToNumHashMap_wxImplementation_HashTable::begin() --> class wxStringToNumHashMap_wxImplementation_HashTable::iterator");
		cl.def("erase", (unsigned long (wxStringToNumHashMap_wxImplementation_HashTable::*)(const class wxString &)) &wxStringToNumHashMap_wxImplementation_HashTable::erase, "C++: wxStringToNumHashMap_wxImplementation_HashTable::erase(const class wxString &) --> unsigned long", pybind11::arg("key"));

		{ // wxStringToNumHashMap_wxImplementation_HashTable::Node file: line:160
			auto & enclosing_class = cl;
			pybind11::class_<wxStringToNumHashMap_wxImplementation_HashTable::Node, std::shared_ptr<wxStringToNumHashMap_wxImplementation_HashTable::Node>, _wxHashTable_NodeBase> cl(enclosing_class, "Node", "");
			cl.def( pybind11::init<const class wxStringToNumHashMap_wxImplementation_Pair &>(), pybind11::arg("value") );

			cl.def( pybind11::init( [](wxStringToNumHashMap_wxImplementation_HashTable::Node const &o){ return new wxStringToNumHashMap_wxImplementation_HashTable::Node(o); } ) );
			cl.def_readwrite("m_value", &wxStringToNumHashMap_wxImplementation_HashTable::Node::m_value);
			cl.def("next", (struct wxStringToNumHashMap_wxImplementation_HashTable::Node * (wxStringToNumHashMap_wxImplementation_HashTable::Node::*)()) &wxStringToNumHashMap_wxImplementation_HashTable::Node::next, "C++: wxStringToNumHashMap_wxImplementation_HashTable::Node::next() --> struct wxStringToNumHashMap_wxImplementation_HashTable::Node *", pybind11::return_value_policy::automatic);
		}

		{ // wxStringToNumHashMap_wxImplementation_HashTable::Iterator file: line:179
			auto & enclosing_class = cl;
			pybind11::class_<wxStringToNumHashMap_wxImplementation_HashTable::Iterator, std::shared_ptr<wxStringToNumHashMap_wxImplementation_HashTable::Iterator>> cl(enclosing_class, "Iterator", "");
			cl.def( pybind11::init( [](){ return new wxStringToNumHashMap_wxImplementation_HashTable::Iterator(); } ) );
			cl.def( pybind11::init<struct wxStringToNumHashMap_wxImplementation_HashTable::Node *, const class wxStringToNumHashMap_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxStringToNumHashMap_wxImplementation_HashTable::Iterator const &o){ return new wxStringToNumHashMap_wxImplementation_HashTable::Iterator(o); } ) );
			cl.def("__eq__", (bool (wxStringToNumHashMap_wxImplementation_HashTable::Iterator::*)(const class wxStringToNumHashMap_wxImplementation_HashTable::Iterator &) const) &wxStringToNumHashMap_wxImplementation_HashTable::Iterator::operator==, "C++: wxStringToNumHashMap_wxImplementation_HashTable::Iterator::operator==(const class wxStringToNumHashMap_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxStringToNumHashMap_wxImplementation_HashTable::Iterator::*)(const class wxStringToNumHashMap_wxImplementation_HashTable::Iterator &) const) &wxStringToNumHashMap_wxImplementation_HashTable::Iterator::operator!=, "C++: wxStringToNumHashMap_wxImplementation_HashTable::Iterator::operator!=(const class wxStringToNumHashMap_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxStringToNumHashMap_wxImplementation_HashTable::iterator file: line:213
			auto & enclosing_class = cl;
			pybind11::class_<wxStringToNumHashMap_wxImplementation_HashTable::iterator, std::shared_ptr<wxStringToNumHashMap_wxImplementation_HashTable::iterator>, wxStringToNumHashMap_wxImplementation_HashTable::Iterator> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init( [](){ return new wxStringToNumHashMap_wxImplementation_HashTable::iterator(); } ) );
			cl.def( pybind11::init<struct wxStringToNumHashMap_wxImplementation_HashTable::Node *, class wxStringToNumHashMap_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxStringToNumHashMap_wxImplementation_HashTable::iterator const &o){ return new wxStringToNumHashMap_wxImplementation_HashTable::iterator(o); } ) );
			cl.def("plus_plus", (class wxStringToNumHashMap_wxImplementation_HashTable::iterator & (wxStringToNumHashMap_wxImplementation_HashTable::iterator::*)()) &wxStringToNumHashMap_wxImplementation_HashTable::iterator::operator++, "C++: wxStringToNumHashMap_wxImplementation_HashTable::iterator::operator++() --> class wxStringToNumHashMap_wxImplementation_HashTable::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxStringToNumHashMap_wxImplementation_HashTable::iterator (wxStringToNumHashMap_wxImplementation_HashTable::iterator::*)(int)) &wxStringToNumHashMap_wxImplementation_HashTable::iterator::operator++, "C++: wxStringToNumHashMap_wxImplementation_HashTable::iterator::operator++(int) --> class wxStringToNumHashMap_wxImplementation_HashTable::iterator", pybind11::arg(""));
			cl.def("__mul__", (class wxStringToNumHashMap_wxImplementation_Pair & (wxStringToNumHashMap_wxImplementation_HashTable::iterator::*)() const) &wxStringToNumHashMap_wxImplementation_HashTable::iterator::operator*, "C++: wxStringToNumHashMap_wxImplementation_HashTable::iterator::operator*() const --> class wxStringToNumHashMap_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

		{ // wxStringToNumHashMap_wxImplementation_HashTable::const_iterator file: line:224
			auto & enclosing_class = cl;
			pybind11::class_<wxStringToNumHashMap_wxImplementation_HashTable::const_iterator, std::shared_ptr<wxStringToNumHashMap_wxImplementation_HashTable::const_iterator>, wxStringToNumHashMap_wxImplementation_HashTable::Iterator> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init( [](){ return new wxStringToNumHashMap_wxImplementation_HashTable::const_iterator(); } ) );
			cl.def( pybind11::init<class wxStringToNumHashMap_wxImplementation_HashTable::iterator>(), pybind11::arg("i") );

			cl.def( pybind11::init<struct wxStringToNumHashMap_wxImplementation_HashTable::Node *, const class wxStringToNumHashMap_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxStringToNumHashMap_wxImplementation_HashTable::const_iterator const &o){ return new wxStringToNumHashMap_wxImplementation_HashTable::const_iterator(o); } ) );
			cl.def("plus_plus", (class wxStringToNumHashMap_wxImplementation_HashTable::const_iterator & (wxStringToNumHashMap_wxImplementation_HashTable::const_iterator::*)()) &wxStringToNumHashMap_wxImplementation_HashTable::const_iterator::operator++, "C++: wxStringToNumHashMap_wxImplementation_HashTable::const_iterator::operator++() --> class wxStringToNumHashMap_wxImplementation_HashTable::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxStringToNumHashMap_wxImplementation_HashTable::const_iterator (wxStringToNumHashMap_wxImplementation_HashTable::const_iterator::*)(int)) &wxStringToNumHashMap_wxImplementation_HashTable::const_iterator::operator++, "C++: wxStringToNumHashMap_wxImplementation_HashTable::const_iterator::operator++(int) --> class wxStringToNumHashMap_wxImplementation_HashTable::const_iterator", pybind11::arg(""));
			cl.def("__mul__", (const class wxStringToNumHashMap_wxImplementation_Pair & (wxStringToNumHashMap_wxImplementation_HashTable::const_iterator::*)() const) &wxStringToNumHashMap_wxImplementation_HashTable::const_iterator::operator*, "C++: wxStringToNumHashMap_wxImplementation_HashTable::const_iterator::operator*() const --> const class wxStringToNumHashMap_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

	}
}

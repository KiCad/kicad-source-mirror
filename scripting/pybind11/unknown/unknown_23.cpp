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

void bind_unknown_unknown_23(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxStringToNumHashMap file: line:752
		pybind11::class_<wxStringToNumHashMap, std::shared_ptr<wxStringToNumHashMap>, wxStringToNumHashMap_wxImplementation_HashTable> cl(M(""), "wxStringToNumHashMap", "");
		cl.def( pybind11::init( [](){ return new wxStringToNumHashMap(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxStringToNumHashMap(a0); } ), "doc" , pybind11::arg("hint"));
		cl.def( pybind11::init( [](unsigned long const & a0, struct wxStringHash const & a1){ return new wxStringToNumHashMap(a0, a1); } ), "doc" , pybind11::arg("hint"), pybind11::arg("hf"));
		cl.def( pybind11::init<unsigned long, struct wxStringHash, struct wxStringEqual>(), pybind11::arg("hint"), pybind11::arg("hf"), pybind11::arg("eq") );

		cl.def( pybind11::init( [](wxStringToNumHashMap const &o){ return new wxStringToNumHashMap(o); } ) );
		cl.def("__getitem__", (unsigned long & (wxStringToNumHashMap::*)(const class wxString &)) &wxStringToNumHashMap::operator[], "C++: wxStringToNumHashMap::operator[](const class wxString &) --> unsigned long &", pybind11::return_value_policy::automatic, pybind11::arg("key"));
		cl.def("find", (class wxStringToNumHashMap_wxImplementation_HashTable::iterator (wxStringToNumHashMap::*)(const class wxString &)) &wxStringToNumHashMap::find, "C++: wxStringToNumHashMap::find(const class wxString &) --> class wxStringToNumHashMap_wxImplementation_HashTable::iterator", pybind11::arg("key"));
		cl.def("insert", (class wxStringToNumHashMap::Insert_Result (wxStringToNumHashMap::*)(const class wxStringToNumHashMap_wxImplementation_Pair &)) &wxStringToNumHashMap::insert, "C++: wxStringToNumHashMap::insert(const class wxStringToNumHashMap_wxImplementation_Pair &) --> class wxStringToNumHashMap::Insert_Result", pybind11::arg("v"));
		cl.def("erase", (unsigned long (wxStringToNumHashMap::*)(const class wxString &)) &wxStringToNumHashMap::erase, "C++: wxStringToNumHashMap::erase(const class wxString &) --> unsigned long", pybind11::arg("k"));
		cl.def("erase", (void (wxStringToNumHashMap::*)(const class wxStringToNumHashMap_wxImplementation_HashTable::iterator &)) &wxStringToNumHashMap::erase, "C++: wxStringToNumHashMap::erase(const class wxStringToNumHashMap_wxImplementation_HashTable::iterator &) --> void", pybind11::arg("it"));
		cl.def("count", (unsigned long (wxStringToNumHashMap::*)(const class wxString &)) &wxStringToNumHashMap::count, "C++: wxStringToNumHashMap::count(const class wxString &) --> unsigned long", pybind11::arg("key"));
		cl.def("assign", (class wxStringToNumHashMap & (wxStringToNumHashMap::*)(const class wxStringToNumHashMap &)) &wxStringToNumHashMap::operator=, "C++: wxStringToNumHashMap::operator=(const class wxStringToNumHashMap &) --> class wxStringToNumHashMap &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxStringToNumHashMap::Insert_Result file: line:645
			auto & enclosing_class = cl;
			pybind11::class_<wxStringToNumHashMap::Insert_Result, std::shared_ptr<wxStringToNumHashMap::Insert_Result>> cl(enclosing_class, "Insert_Result", "");
			cl.def( pybind11::init<const class wxStringToNumHashMap_wxImplementation_HashTable::iterator &, const bool &>(), pybind11::arg("f"), pybind11::arg("s") );

			cl.def( pybind11::init( [](wxStringToNumHashMap::Insert_Result const &o){ return new wxStringToNumHashMap::Insert_Result(o); } ) );
			cl.def_readwrite("first", &wxStringToNumHashMap::Insert_Result::first);
			cl.def_readwrite("second", &wxStringToNumHashMap::Insert_Result::second);
		}

	}
	{ // wxShadowObjectMethods_wxImplementation_Pair file: line:110
		pybind11::class_<wxShadowObjectMethods_wxImplementation_Pair, std::shared_ptr<wxShadowObjectMethods_wxImplementation_Pair>> cl(M(""), "wxShadowObjectMethods_wxImplementation_Pair", "");
		cl.def( pybind11::init( [](wxShadowObjectMethods_wxImplementation_Pair const &o){ return new wxShadowObjectMethods_wxImplementation_Pair(o); } ) );
		cl.def_readwrite("first", &wxShadowObjectMethods_wxImplementation_Pair::first);
		cl.def("assign", (class wxShadowObjectMethods_wxImplementation_Pair & (wxShadowObjectMethods_wxImplementation_Pair::*)(const class wxShadowObjectMethods_wxImplementation_Pair &)) &wxShadowObjectMethods_wxImplementation_Pair::operator=, "C++: wxShadowObjectMethods_wxImplementation_Pair::operator=(const class wxShadowObjectMethods_wxImplementation_Pair &) --> class wxShadowObjectMethods_wxImplementation_Pair &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxShadowObjectMethods_wxImplementation_KeyEx file: line:112
		pybind11::class_<wxShadowObjectMethods_wxImplementation_KeyEx, std::shared_ptr<wxShadowObjectMethods_wxImplementation_KeyEx>> cl(M(""), "wxShadowObjectMethods_wxImplementation_KeyEx", "");
		cl.def( pybind11::init( [](){ return new wxShadowObjectMethods_wxImplementation_KeyEx(); } ) );
		cl.def( pybind11::init( [](wxShadowObjectMethods_wxImplementation_KeyEx const &o){ return new wxShadowObjectMethods_wxImplementation_KeyEx(o); } ) );
		cl.def("__call__", (const class wxString & (wxShadowObjectMethods_wxImplementation_KeyEx::*)(const class wxShadowObjectMethods_wxImplementation_Pair &) const) &wxShadowObjectMethods_wxImplementation_KeyEx::operator(), "C++: wxShadowObjectMethods_wxImplementation_KeyEx::operator()(const class wxShadowObjectMethods_wxImplementation_Pair &) const --> const class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("pair"));
		cl.def("assign", (class wxShadowObjectMethods_wxImplementation_KeyEx & (wxShadowObjectMethods_wxImplementation_KeyEx::*)(const class wxShadowObjectMethods_wxImplementation_KeyEx &)) &wxShadowObjectMethods_wxImplementation_KeyEx::operator=, "C++: wxShadowObjectMethods_wxImplementation_KeyEx::operator=(const class wxShadowObjectMethods_wxImplementation_KeyEx &) --> class wxShadowObjectMethods_wxImplementation_KeyEx &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxShadowObjectMethods_wxImplementation_HashTable file: line:115
		pybind11::class_<wxShadowObjectMethods_wxImplementation_HashTable, std::shared_ptr<wxShadowObjectMethods_wxImplementation_HashTable>> cl(M(""), "wxShadowObjectMethods_wxImplementation_HashTable", "");
		cl.def( pybind11::init( [](){ return new wxShadowObjectMethods_wxImplementation_HashTable(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxShadowObjectMethods_wxImplementation_HashTable(a0); } ), "doc" , pybind11::arg("sz"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxStringHash & a1){ return new wxShadowObjectMethods_wxImplementation_HashTable(a0, a1); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxStringHash & a1, const struct wxStringEqual & a2){ return new wxShadowObjectMethods_wxImplementation_HashTable(a0, a1, a2); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"));
		cl.def( pybind11::init<unsigned long, const struct wxStringHash &, const struct wxStringEqual &, const class wxShadowObjectMethods_wxImplementation_KeyEx &>(), pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"), pybind11::arg("k_ex") );

		cl.def( pybind11::init( [](wxShadowObjectMethods_wxImplementation_HashTable const &o){ return new wxShadowObjectMethods_wxImplementation_HashTable(o); } ) );
		cl.def("assign", (const class wxShadowObjectMethods_wxImplementation_HashTable & (wxShadowObjectMethods_wxImplementation_HashTable::*)(const class wxShadowObjectMethods_wxImplementation_HashTable &)) &wxShadowObjectMethods_wxImplementation_HashTable::operator=, "C++: wxShadowObjectMethods_wxImplementation_HashTable::operator=(const class wxShadowObjectMethods_wxImplementation_HashTable &) --> const class wxShadowObjectMethods_wxImplementation_HashTable &", pybind11::return_value_policy::automatic, pybind11::arg("ht"));
		cl.def("hash_funct", (struct wxStringHash (wxShadowObjectMethods_wxImplementation_HashTable::*)()) &wxShadowObjectMethods_wxImplementation_HashTable::hash_funct, "C++: wxShadowObjectMethods_wxImplementation_HashTable::hash_funct() --> struct wxStringHash");
		cl.def("key_eq", (struct wxStringEqual (wxShadowObjectMethods_wxImplementation_HashTable::*)()) &wxShadowObjectMethods_wxImplementation_HashTable::key_eq, "C++: wxShadowObjectMethods_wxImplementation_HashTable::key_eq() --> struct wxStringEqual");
		cl.def("clear", (void (wxShadowObjectMethods_wxImplementation_HashTable::*)()) &wxShadowObjectMethods_wxImplementation_HashTable::clear, "C++: wxShadowObjectMethods_wxImplementation_HashTable::clear() --> void");
		cl.def("size", (unsigned long (wxShadowObjectMethods_wxImplementation_HashTable::*)() const) &wxShadowObjectMethods_wxImplementation_HashTable::size, "C++: wxShadowObjectMethods_wxImplementation_HashTable::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxShadowObjectMethods_wxImplementation_HashTable::*)() const) &wxShadowObjectMethods_wxImplementation_HashTable::max_size, "C++: wxShadowObjectMethods_wxImplementation_HashTable::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxShadowObjectMethods_wxImplementation_HashTable::*)() const) &wxShadowObjectMethods_wxImplementation_HashTable::empty, "C++: wxShadowObjectMethods_wxImplementation_HashTable::empty() const --> bool");
		cl.def("end", (class wxShadowObjectMethods_wxImplementation_HashTable::iterator (wxShadowObjectMethods_wxImplementation_HashTable::*)()) &wxShadowObjectMethods_wxImplementation_HashTable::end, "C++: wxShadowObjectMethods_wxImplementation_HashTable::end() --> class wxShadowObjectMethods_wxImplementation_HashTable::iterator");
		cl.def("begin", (class wxShadowObjectMethods_wxImplementation_HashTable::iterator (wxShadowObjectMethods_wxImplementation_HashTable::*)()) &wxShadowObjectMethods_wxImplementation_HashTable::begin, "C++: wxShadowObjectMethods_wxImplementation_HashTable::begin() --> class wxShadowObjectMethods_wxImplementation_HashTable::iterator");
		cl.def("erase", (unsigned long (wxShadowObjectMethods_wxImplementation_HashTable::*)(const class wxString &)) &wxShadowObjectMethods_wxImplementation_HashTable::erase, "C++: wxShadowObjectMethods_wxImplementation_HashTable::erase(const class wxString &) --> unsigned long", pybind11::arg("key"));

		{ // wxShadowObjectMethods_wxImplementation_HashTable::Node file: line:160
			auto & enclosing_class = cl;
			pybind11::class_<wxShadowObjectMethods_wxImplementation_HashTable::Node, std::shared_ptr<wxShadowObjectMethods_wxImplementation_HashTable::Node>, _wxHashTable_NodeBase> cl(enclosing_class, "Node", "");
			cl.def( pybind11::init<const class wxShadowObjectMethods_wxImplementation_Pair &>(), pybind11::arg("value") );

			cl.def( pybind11::init( [](wxShadowObjectMethods_wxImplementation_HashTable::Node const &o){ return new wxShadowObjectMethods_wxImplementation_HashTable::Node(o); } ) );
			cl.def_readwrite("m_value", &wxShadowObjectMethods_wxImplementation_HashTable::Node::m_value);
			cl.def("next", (struct wxShadowObjectMethods_wxImplementation_HashTable::Node * (wxShadowObjectMethods_wxImplementation_HashTable::Node::*)()) &wxShadowObjectMethods_wxImplementation_HashTable::Node::next, "C++: wxShadowObjectMethods_wxImplementation_HashTable::Node::next() --> struct wxShadowObjectMethods_wxImplementation_HashTable::Node *", pybind11::return_value_policy::automatic);
		}

		{ // wxShadowObjectMethods_wxImplementation_HashTable::Iterator file: line:179
			auto & enclosing_class = cl;
			pybind11::class_<wxShadowObjectMethods_wxImplementation_HashTable::Iterator, std::shared_ptr<wxShadowObjectMethods_wxImplementation_HashTable::Iterator>> cl(enclosing_class, "Iterator", "");
			cl.def( pybind11::init( [](){ return new wxShadowObjectMethods_wxImplementation_HashTable::Iterator(); } ) );
			cl.def( pybind11::init<struct wxShadowObjectMethods_wxImplementation_HashTable::Node *, const class wxShadowObjectMethods_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxShadowObjectMethods_wxImplementation_HashTable::Iterator const &o){ return new wxShadowObjectMethods_wxImplementation_HashTable::Iterator(o); } ) );
			cl.def("__eq__", (bool (wxShadowObjectMethods_wxImplementation_HashTable::Iterator::*)(const class wxShadowObjectMethods_wxImplementation_HashTable::Iterator &) const) &wxShadowObjectMethods_wxImplementation_HashTable::Iterator::operator==, "C++: wxShadowObjectMethods_wxImplementation_HashTable::Iterator::operator==(const class wxShadowObjectMethods_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxShadowObjectMethods_wxImplementation_HashTable::Iterator::*)(const class wxShadowObjectMethods_wxImplementation_HashTable::Iterator &) const) &wxShadowObjectMethods_wxImplementation_HashTable::Iterator::operator!=, "C++: wxShadowObjectMethods_wxImplementation_HashTable::Iterator::operator!=(const class wxShadowObjectMethods_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxShadowObjectMethods_wxImplementation_HashTable::iterator file: line:213
			auto & enclosing_class = cl;
			pybind11::class_<wxShadowObjectMethods_wxImplementation_HashTable::iterator, std::shared_ptr<wxShadowObjectMethods_wxImplementation_HashTable::iterator>, wxShadowObjectMethods_wxImplementation_HashTable::Iterator> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init( [](){ return new wxShadowObjectMethods_wxImplementation_HashTable::iterator(); } ) );
			cl.def( pybind11::init<struct wxShadowObjectMethods_wxImplementation_HashTable::Node *, class wxShadowObjectMethods_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxShadowObjectMethods_wxImplementation_HashTable::iterator const &o){ return new wxShadowObjectMethods_wxImplementation_HashTable::iterator(o); } ) );
			cl.def("plus_plus", (class wxShadowObjectMethods_wxImplementation_HashTable::iterator & (wxShadowObjectMethods_wxImplementation_HashTable::iterator::*)()) &wxShadowObjectMethods_wxImplementation_HashTable::iterator::operator++, "C++: wxShadowObjectMethods_wxImplementation_HashTable::iterator::operator++() --> class wxShadowObjectMethods_wxImplementation_HashTable::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxShadowObjectMethods_wxImplementation_HashTable::iterator (wxShadowObjectMethods_wxImplementation_HashTable::iterator::*)(int)) &wxShadowObjectMethods_wxImplementation_HashTable::iterator::operator++, "C++: wxShadowObjectMethods_wxImplementation_HashTable::iterator::operator++(int) --> class wxShadowObjectMethods_wxImplementation_HashTable::iterator", pybind11::arg(""));
			cl.def("__mul__", (class wxShadowObjectMethods_wxImplementation_Pair & (wxShadowObjectMethods_wxImplementation_HashTable::iterator::*)() const) &wxShadowObjectMethods_wxImplementation_HashTable::iterator::operator*, "C++: wxShadowObjectMethods_wxImplementation_HashTable::iterator::operator*() const --> class wxShadowObjectMethods_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

		{ // wxShadowObjectMethods_wxImplementation_HashTable::const_iterator file: line:224
			auto & enclosing_class = cl;
			pybind11::class_<wxShadowObjectMethods_wxImplementation_HashTable::const_iterator, std::shared_ptr<wxShadowObjectMethods_wxImplementation_HashTable::const_iterator>, wxShadowObjectMethods_wxImplementation_HashTable::Iterator> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init( [](){ return new wxShadowObjectMethods_wxImplementation_HashTable::const_iterator(); } ) );
			cl.def( pybind11::init<class wxShadowObjectMethods_wxImplementation_HashTable::iterator>(), pybind11::arg("i") );

			cl.def( pybind11::init<struct wxShadowObjectMethods_wxImplementation_HashTable::Node *, const class wxShadowObjectMethods_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxShadowObjectMethods_wxImplementation_HashTable::const_iterator const &o){ return new wxShadowObjectMethods_wxImplementation_HashTable::const_iterator(o); } ) );
			cl.def("plus_plus", (class wxShadowObjectMethods_wxImplementation_HashTable::const_iterator & (wxShadowObjectMethods_wxImplementation_HashTable::const_iterator::*)()) &wxShadowObjectMethods_wxImplementation_HashTable::const_iterator::operator++, "C++: wxShadowObjectMethods_wxImplementation_HashTable::const_iterator::operator++() --> class wxShadowObjectMethods_wxImplementation_HashTable::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxShadowObjectMethods_wxImplementation_HashTable::const_iterator (wxShadowObjectMethods_wxImplementation_HashTable::const_iterator::*)(int)) &wxShadowObjectMethods_wxImplementation_HashTable::const_iterator::operator++, "C++: wxShadowObjectMethods_wxImplementation_HashTable::const_iterator::operator++(int) --> class wxShadowObjectMethods_wxImplementation_HashTable::const_iterator", pybind11::arg(""));
			cl.def("__mul__", (const class wxShadowObjectMethods_wxImplementation_Pair & (wxShadowObjectMethods_wxImplementation_HashTable::const_iterator::*)() const) &wxShadowObjectMethods_wxImplementation_HashTable::const_iterator::operator*, "C++: wxShadowObjectMethods_wxImplementation_HashTable::const_iterator::operator*() const --> const class wxShadowObjectMethods_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

	}
}

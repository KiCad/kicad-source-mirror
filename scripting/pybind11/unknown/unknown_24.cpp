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

void bind_unknown_unknown_24(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxShadowObjectMethods file: line:21
		pybind11::class_<wxShadowObjectMethods, std::shared_ptr<wxShadowObjectMethods>, wxShadowObjectMethods_wxImplementation_HashTable> cl(M(""), "wxShadowObjectMethods", "");
		cl.def( pybind11::init( [](){ return new wxShadowObjectMethods(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxShadowObjectMethods(a0); } ), "doc" , pybind11::arg("hint"));
		cl.def( pybind11::init( [](unsigned long const & a0, struct wxStringHash const & a1){ return new wxShadowObjectMethods(a0, a1); } ), "doc" , pybind11::arg("hint"), pybind11::arg("hf"));
		cl.def( pybind11::init<unsigned long, struct wxStringHash, struct wxStringEqual>(), pybind11::arg("hint"), pybind11::arg("hf"), pybind11::arg("eq") );

		cl.def( pybind11::init( [](wxShadowObjectMethods const &o){ return new wxShadowObjectMethods(o); } ) );
		cl.def("find", (class wxShadowObjectMethods_wxImplementation_HashTable::iterator (wxShadowObjectMethods::*)(const class wxString &)) &wxShadowObjectMethods::find, "C++: wxShadowObjectMethods::find(const class wxString &) --> class wxShadowObjectMethods_wxImplementation_HashTable::iterator", pybind11::arg("key"));
		cl.def("insert", (class wxShadowObjectMethods::Insert_Result (wxShadowObjectMethods::*)(const class wxShadowObjectMethods_wxImplementation_Pair &)) &wxShadowObjectMethods::insert, "C++: wxShadowObjectMethods::insert(const class wxShadowObjectMethods_wxImplementation_Pair &) --> class wxShadowObjectMethods::Insert_Result", pybind11::arg("v"));
		cl.def("erase", (unsigned long (wxShadowObjectMethods::*)(const class wxString &)) &wxShadowObjectMethods::erase, "C++: wxShadowObjectMethods::erase(const class wxString &) --> unsigned long", pybind11::arg("k"));
		cl.def("erase", (void (wxShadowObjectMethods::*)(const class wxShadowObjectMethods_wxImplementation_HashTable::iterator &)) &wxShadowObjectMethods::erase, "C++: wxShadowObjectMethods::erase(const class wxShadowObjectMethods_wxImplementation_HashTable::iterator &) --> void", pybind11::arg("it"));
		cl.def("count", (unsigned long (wxShadowObjectMethods::*)(const class wxString &)) &wxShadowObjectMethods::count, "C++: wxShadowObjectMethods::count(const class wxString &) --> unsigned long", pybind11::arg("key"));
		cl.def("assign", (class wxShadowObjectMethods & (wxShadowObjectMethods::*)(const class wxShadowObjectMethods &)) &wxShadowObjectMethods::operator=, "C++: wxShadowObjectMethods::operator=(const class wxShadowObjectMethods &) --> class wxShadowObjectMethods &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxShadowObjectMethods::Insert_Result file: line:645
			auto & enclosing_class = cl;
			pybind11::class_<wxShadowObjectMethods::Insert_Result, std::shared_ptr<wxShadowObjectMethods::Insert_Result>> cl(enclosing_class, "Insert_Result", "");
			cl.def( pybind11::init<const class wxShadowObjectMethods_wxImplementation_HashTable::iterator &, const bool &>(), pybind11::arg("f"), pybind11::arg("s") );

			cl.def( pybind11::init( [](wxShadowObjectMethods::Insert_Result const &o){ return new wxShadowObjectMethods::Insert_Result(o); } ) );
			cl.def_readwrite("first", &wxShadowObjectMethods::Insert_Result::first);
			cl.def_readwrite("second", &wxShadowObjectMethods::Insert_Result::second);
		}

	}
	{ // wxShadowObjectFields_wxImplementation_Pair file: line:122
		pybind11::class_<wxShadowObjectFields_wxImplementation_Pair, std::shared_ptr<wxShadowObjectFields_wxImplementation_Pair>> cl(M(""), "wxShadowObjectFields_wxImplementation_Pair", "");
		cl.def( pybind11::init<const class wxString &, const void *const &>(), pybind11::arg("f"), pybind11::arg("s") );

		cl.def( pybind11::init( [](wxShadowObjectFields_wxImplementation_Pair const &o){ return new wxShadowObjectFields_wxImplementation_Pair(o); } ) );
		cl.def_readwrite("first", &wxShadowObjectFields_wxImplementation_Pair::first);
		cl.def("assign", (class wxShadowObjectFields_wxImplementation_Pair & (wxShadowObjectFields_wxImplementation_Pair::*)(const class wxShadowObjectFields_wxImplementation_Pair &)) &wxShadowObjectFields_wxImplementation_Pair::operator=, "C++: wxShadowObjectFields_wxImplementation_Pair::operator=(const class wxShadowObjectFields_wxImplementation_Pair &) --> class wxShadowObjectFields_wxImplementation_Pair &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxShadowObjectFields_wxImplementation_KeyEx file: line:124
		pybind11::class_<wxShadowObjectFields_wxImplementation_KeyEx, std::shared_ptr<wxShadowObjectFields_wxImplementation_KeyEx>> cl(M(""), "wxShadowObjectFields_wxImplementation_KeyEx", "");
		cl.def( pybind11::init( [](){ return new wxShadowObjectFields_wxImplementation_KeyEx(); } ) );
		cl.def( pybind11::init( [](wxShadowObjectFields_wxImplementation_KeyEx const &o){ return new wxShadowObjectFields_wxImplementation_KeyEx(o); } ) );
		cl.def("__call__", (const class wxString & (wxShadowObjectFields_wxImplementation_KeyEx::*)(const class wxShadowObjectFields_wxImplementation_Pair &) const) &wxShadowObjectFields_wxImplementation_KeyEx::operator(), "C++: wxShadowObjectFields_wxImplementation_KeyEx::operator()(const class wxShadowObjectFields_wxImplementation_Pair &) const --> const class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("pair"));
		cl.def("assign", (class wxShadowObjectFields_wxImplementation_KeyEx & (wxShadowObjectFields_wxImplementation_KeyEx::*)(const class wxShadowObjectFields_wxImplementation_KeyEx &)) &wxShadowObjectFields_wxImplementation_KeyEx::operator=, "C++: wxShadowObjectFields_wxImplementation_KeyEx::operator=(const class wxShadowObjectFields_wxImplementation_KeyEx &) --> class wxShadowObjectFields_wxImplementation_KeyEx &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxShadowObjectFields_wxImplementation_HashTable file: line:127
		pybind11::class_<wxShadowObjectFields_wxImplementation_HashTable, std::shared_ptr<wxShadowObjectFields_wxImplementation_HashTable>> cl(M(""), "wxShadowObjectFields_wxImplementation_HashTable", "");
		cl.def( pybind11::init( [](){ return new wxShadowObjectFields_wxImplementation_HashTable(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxShadowObjectFields_wxImplementation_HashTable(a0); } ), "doc" , pybind11::arg("sz"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxStringHash & a1){ return new wxShadowObjectFields_wxImplementation_HashTable(a0, a1); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxStringHash & a1, const struct wxStringEqual & a2){ return new wxShadowObjectFields_wxImplementation_HashTable(a0, a1, a2); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"));
		cl.def( pybind11::init<unsigned long, const struct wxStringHash &, const struct wxStringEqual &, const class wxShadowObjectFields_wxImplementation_KeyEx &>(), pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"), pybind11::arg("k_ex") );

		cl.def( pybind11::init( [](wxShadowObjectFields_wxImplementation_HashTable const &o){ return new wxShadowObjectFields_wxImplementation_HashTable(o); } ) );
		cl.def("assign", (const class wxShadowObjectFields_wxImplementation_HashTable & (wxShadowObjectFields_wxImplementation_HashTable::*)(const class wxShadowObjectFields_wxImplementation_HashTable &)) &wxShadowObjectFields_wxImplementation_HashTable::operator=, "C++: wxShadowObjectFields_wxImplementation_HashTable::operator=(const class wxShadowObjectFields_wxImplementation_HashTable &) --> const class wxShadowObjectFields_wxImplementation_HashTable &", pybind11::return_value_policy::automatic, pybind11::arg("ht"));
		cl.def("hash_funct", (struct wxStringHash (wxShadowObjectFields_wxImplementation_HashTable::*)()) &wxShadowObjectFields_wxImplementation_HashTable::hash_funct, "C++: wxShadowObjectFields_wxImplementation_HashTable::hash_funct() --> struct wxStringHash");
		cl.def("key_eq", (struct wxStringEqual (wxShadowObjectFields_wxImplementation_HashTable::*)()) &wxShadowObjectFields_wxImplementation_HashTable::key_eq, "C++: wxShadowObjectFields_wxImplementation_HashTable::key_eq() --> struct wxStringEqual");
		cl.def("clear", (void (wxShadowObjectFields_wxImplementation_HashTable::*)()) &wxShadowObjectFields_wxImplementation_HashTable::clear, "C++: wxShadowObjectFields_wxImplementation_HashTable::clear() --> void");
		cl.def("size", (unsigned long (wxShadowObjectFields_wxImplementation_HashTable::*)() const) &wxShadowObjectFields_wxImplementation_HashTable::size, "C++: wxShadowObjectFields_wxImplementation_HashTable::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxShadowObjectFields_wxImplementation_HashTable::*)() const) &wxShadowObjectFields_wxImplementation_HashTable::max_size, "C++: wxShadowObjectFields_wxImplementation_HashTable::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxShadowObjectFields_wxImplementation_HashTable::*)() const) &wxShadowObjectFields_wxImplementation_HashTable::empty, "C++: wxShadowObjectFields_wxImplementation_HashTable::empty() const --> bool");
		cl.def("end", (class wxShadowObjectFields_wxImplementation_HashTable::iterator (wxShadowObjectFields_wxImplementation_HashTable::*)()) &wxShadowObjectFields_wxImplementation_HashTable::end, "C++: wxShadowObjectFields_wxImplementation_HashTable::end() --> class wxShadowObjectFields_wxImplementation_HashTable::iterator");
		cl.def("begin", (class wxShadowObjectFields_wxImplementation_HashTable::iterator (wxShadowObjectFields_wxImplementation_HashTable::*)()) &wxShadowObjectFields_wxImplementation_HashTable::begin, "C++: wxShadowObjectFields_wxImplementation_HashTable::begin() --> class wxShadowObjectFields_wxImplementation_HashTable::iterator");
		cl.def("erase", (unsigned long (wxShadowObjectFields_wxImplementation_HashTable::*)(const class wxString &)) &wxShadowObjectFields_wxImplementation_HashTable::erase, "C++: wxShadowObjectFields_wxImplementation_HashTable::erase(const class wxString &) --> unsigned long", pybind11::arg("key"));

		{ // wxShadowObjectFields_wxImplementation_HashTable::Node file: line:160
			auto & enclosing_class = cl;
			pybind11::class_<wxShadowObjectFields_wxImplementation_HashTable::Node, std::shared_ptr<wxShadowObjectFields_wxImplementation_HashTable::Node>, _wxHashTable_NodeBase> cl(enclosing_class, "Node", "");
			cl.def( pybind11::init<const class wxShadowObjectFields_wxImplementation_Pair &>(), pybind11::arg("value") );

			cl.def( pybind11::init( [](wxShadowObjectFields_wxImplementation_HashTable::Node const &o){ return new wxShadowObjectFields_wxImplementation_HashTable::Node(o); } ) );
			cl.def_readwrite("m_value", &wxShadowObjectFields_wxImplementation_HashTable::Node::m_value);
			cl.def("next", (struct wxShadowObjectFields_wxImplementation_HashTable::Node * (wxShadowObjectFields_wxImplementation_HashTable::Node::*)()) &wxShadowObjectFields_wxImplementation_HashTable::Node::next, "C++: wxShadowObjectFields_wxImplementation_HashTable::Node::next() --> struct wxShadowObjectFields_wxImplementation_HashTable::Node *", pybind11::return_value_policy::automatic);
		}

		{ // wxShadowObjectFields_wxImplementation_HashTable::Iterator file: line:179
			auto & enclosing_class = cl;
			pybind11::class_<wxShadowObjectFields_wxImplementation_HashTable::Iterator, std::shared_ptr<wxShadowObjectFields_wxImplementation_HashTable::Iterator>> cl(enclosing_class, "Iterator", "");
			cl.def( pybind11::init( [](){ return new wxShadowObjectFields_wxImplementation_HashTable::Iterator(); } ) );
			cl.def( pybind11::init<struct wxShadowObjectFields_wxImplementation_HashTable::Node *, const class wxShadowObjectFields_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxShadowObjectFields_wxImplementation_HashTable::Iterator const &o){ return new wxShadowObjectFields_wxImplementation_HashTable::Iterator(o); } ) );
			cl.def("__eq__", (bool (wxShadowObjectFields_wxImplementation_HashTable::Iterator::*)(const class wxShadowObjectFields_wxImplementation_HashTable::Iterator &) const) &wxShadowObjectFields_wxImplementation_HashTable::Iterator::operator==, "C++: wxShadowObjectFields_wxImplementation_HashTable::Iterator::operator==(const class wxShadowObjectFields_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxShadowObjectFields_wxImplementation_HashTable::Iterator::*)(const class wxShadowObjectFields_wxImplementation_HashTable::Iterator &) const) &wxShadowObjectFields_wxImplementation_HashTable::Iterator::operator!=, "C++: wxShadowObjectFields_wxImplementation_HashTable::Iterator::operator!=(const class wxShadowObjectFields_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxShadowObjectFields_wxImplementation_HashTable::iterator file: line:213
			auto & enclosing_class = cl;
			pybind11::class_<wxShadowObjectFields_wxImplementation_HashTable::iterator, std::shared_ptr<wxShadowObjectFields_wxImplementation_HashTable::iterator>, wxShadowObjectFields_wxImplementation_HashTable::Iterator> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init( [](){ return new wxShadowObjectFields_wxImplementation_HashTable::iterator(); } ) );
			cl.def( pybind11::init<struct wxShadowObjectFields_wxImplementation_HashTable::Node *, class wxShadowObjectFields_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxShadowObjectFields_wxImplementation_HashTable::iterator const &o){ return new wxShadowObjectFields_wxImplementation_HashTable::iterator(o); } ) );
			cl.def("plus_plus", (class wxShadowObjectFields_wxImplementation_HashTable::iterator & (wxShadowObjectFields_wxImplementation_HashTable::iterator::*)()) &wxShadowObjectFields_wxImplementation_HashTable::iterator::operator++, "C++: wxShadowObjectFields_wxImplementation_HashTable::iterator::operator++() --> class wxShadowObjectFields_wxImplementation_HashTable::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxShadowObjectFields_wxImplementation_HashTable::iterator (wxShadowObjectFields_wxImplementation_HashTable::iterator::*)(int)) &wxShadowObjectFields_wxImplementation_HashTable::iterator::operator++, "C++: wxShadowObjectFields_wxImplementation_HashTable::iterator::operator++(int) --> class wxShadowObjectFields_wxImplementation_HashTable::iterator", pybind11::arg(""));
			cl.def("__mul__", (class wxShadowObjectFields_wxImplementation_Pair & (wxShadowObjectFields_wxImplementation_HashTable::iterator::*)() const) &wxShadowObjectFields_wxImplementation_HashTable::iterator::operator*, "C++: wxShadowObjectFields_wxImplementation_HashTable::iterator::operator*() const --> class wxShadowObjectFields_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

		{ // wxShadowObjectFields_wxImplementation_HashTable::const_iterator file: line:224
			auto & enclosing_class = cl;
			pybind11::class_<wxShadowObjectFields_wxImplementation_HashTable::const_iterator, std::shared_ptr<wxShadowObjectFields_wxImplementation_HashTable::const_iterator>, wxShadowObjectFields_wxImplementation_HashTable::Iterator> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init( [](){ return new wxShadowObjectFields_wxImplementation_HashTable::const_iterator(); } ) );
			cl.def( pybind11::init<class wxShadowObjectFields_wxImplementation_HashTable::iterator>(), pybind11::arg("i") );

			cl.def( pybind11::init<struct wxShadowObjectFields_wxImplementation_HashTable::Node *, const class wxShadowObjectFields_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxShadowObjectFields_wxImplementation_HashTable::const_iterator const &o){ return new wxShadowObjectFields_wxImplementation_HashTable::const_iterator(o); } ) );
			cl.def("plus_plus", (class wxShadowObjectFields_wxImplementation_HashTable::const_iterator & (wxShadowObjectFields_wxImplementation_HashTable::const_iterator::*)()) &wxShadowObjectFields_wxImplementation_HashTable::const_iterator::operator++, "C++: wxShadowObjectFields_wxImplementation_HashTable::const_iterator::operator++() --> class wxShadowObjectFields_wxImplementation_HashTable::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxShadowObjectFields_wxImplementation_HashTable::const_iterator (wxShadowObjectFields_wxImplementation_HashTable::const_iterator::*)(int)) &wxShadowObjectFields_wxImplementation_HashTable::const_iterator::operator++, "C++: wxShadowObjectFields_wxImplementation_HashTable::const_iterator::operator++(int) --> class wxShadowObjectFields_wxImplementation_HashTable::const_iterator", pybind11::arg(""));
			cl.def("__mul__", (const class wxShadowObjectFields_wxImplementation_Pair & (wxShadowObjectFields_wxImplementation_HashTable::const_iterator::*)() const) &wxShadowObjectFields_wxImplementation_HashTable::const_iterator::operator*, "C++: wxShadowObjectFields_wxImplementation_HashTable::const_iterator::operator*() const --> const class wxShadowObjectFields_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

	}
}

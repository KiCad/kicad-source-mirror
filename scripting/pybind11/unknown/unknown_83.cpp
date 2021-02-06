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

void bind_unknown_unknown_83(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxPGHashMapS2S_wxImplementation_KeyEx file: line:3
		pybind11::class_<wxPGHashMapS2S_wxImplementation_KeyEx, std::shared_ptr<wxPGHashMapS2S_wxImplementation_KeyEx>> cl(M(""), "wxPGHashMapS2S_wxImplementation_KeyEx", "");
		cl.def( pybind11::init( [](){ return new wxPGHashMapS2S_wxImplementation_KeyEx(); } ) );
		cl.def( pybind11::init( [](wxPGHashMapS2S_wxImplementation_KeyEx const &o){ return new wxPGHashMapS2S_wxImplementation_KeyEx(o); } ) );
		cl.def("__call__", (const class wxString & (wxPGHashMapS2S_wxImplementation_KeyEx::*)(const class wxPGHashMapS2S_wxImplementation_Pair &) const) &wxPGHashMapS2S_wxImplementation_KeyEx::operator(), "C++: wxPGHashMapS2S_wxImplementation_KeyEx::operator()(const class wxPGHashMapS2S_wxImplementation_Pair &) const --> const class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("pair"));
		cl.def("assign", (class wxPGHashMapS2S_wxImplementation_KeyEx & (wxPGHashMapS2S_wxImplementation_KeyEx::*)(const class wxPGHashMapS2S_wxImplementation_KeyEx &)) &wxPGHashMapS2S_wxImplementation_KeyEx::operator=, "C++: wxPGHashMapS2S_wxImplementation_KeyEx::operator=(const class wxPGHashMapS2S_wxImplementation_KeyEx &) --> class wxPGHashMapS2S_wxImplementation_KeyEx &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxPGHashMapS2S_wxImplementation_HashTable file: line:6
		pybind11::class_<wxPGHashMapS2S_wxImplementation_HashTable, std::shared_ptr<wxPGHashMapS2S_wxImplementation_HashTable>> cl(M(""), "wxPGHashMapS2S_wxImplementation_HashTable", "");
		cl.def( pybind11::init( [](){ return new wxPGHashMapS2S_wxImplementation_HashTable(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxPGHashMapS2S_wxImplementation_HashTable(a0); } ), "doc" , pybind11::arg("sz"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxStringHash & a1){ return new wxPGHashMapS2S_wxImplementation_HashTable(a0, a1); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxStringHash & a1, const struct wxStringEqual & a2){ return new wxPGHashMapS2S_wxImplementation_HashTable(a0, a1, a2); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"));
		cl.def( pybind11::init<unsigned long, const struct wxStringHash &, const struct wxStringEqual &, const class wxPGHashMapS2S_wxImplementation_KeyEx &>(), pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"), pybind11::arg("k_ex") );

		cl.def( pybind11::init( [](wxPGHashMapS2S_wxImplementation_HashTable const &o){ return new wxPGHashMapS2S_wxImplementation_HashTable(o); } ) );
		cl.def("assign", (const class wxPGHashMapS2S_wxImplementation_HashTable & (wxPGHashMapS2S_wxImplementation_HashTable::*)(const class wxPGHashMapS2S_wxImplementation_HashTable &)) &wxPGHashMapS2S_wxImplementation_HashTable::operator=, "C++: wxPGHashMapS2S_wxImplementation_HashTable::operator=(const class wxPGHashMapS2S_wxImplementation_HashTable &) --> const class wxPGHashMapS2S_wxImplementation_HashTable &", pybind11::return_value_policy::automatic, pybind11::arg("ht"));
		cl.def("hash_funct", (struct wxStringHash (wxPGHashMapS2S_wxImplementation_HashTable::*)()) &wxPGHashMapS2S_wxImplementation_HashTable::hash_funct, "C++: wxPGHashMapS2S_wxImplementation_HashTable::hash_funct() --> struct wxStringHash");
		cl.def("key_eq", (struct wxStringEqual (wxPGHashMapS2S_wxImplementation_HashTable::*)()) &wxPGHashMapS2S_wxImplementation_HashTable::key_eq, "C++: wxPGHashMapS2S_wxImplementation_HashTable::key_eq() --> struct wxStringEqual");
		cl.def("clear", (void (wxPGHashMapS2S_wxImplementation_HashTable::*)()) &wxPGHashMapS2S_wxImplementation_HashTable::clear, "C++: wxPGHashMapS2S_wxImplementation_HashTable::clear() --> void");
		cl.def("size", (unsigned long (wxPGHashMapS2S_wxImplementation_HashTable::*)() const) &wxPGHashMapS2S_wxImplementation_HashTable::size, "C++: wxPGHashMapS2S_wxImplementation_HashTable::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxPGHashMapS2S_wxImplementation_HashTable::*)() const) &wxPGHashMapS2S_wxImplementation_HashTable::max_size, "C++: wxPGHashMapS2S_wxImplementation_HashTable::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxPGHashMapS2S_wxImplementation_HashTable::*)() const) &wxPGHashMapS2S_wxImplementation_HashTable::empty, "C++: wxPGHashMapS2S_wxImplementation_HashTable::empty() const --> bool");
		cl.def("end", (class wxPGHashMapS2S_wxImplementation_HashTable::iterator (wxPGHashMapS2S_wxImplementation_HashTable::*)()) &wxPGHashMapS2S_wxImplementation_HashTable::end, "C++: wxPGHashMapS2S_wxImplementation_HashTable::end() --> class wxPGHashMapS2S_wxImplementation_HashTable::iterator");
		cl.def("begin", (class wxPGHashMapS2S_wxImplementation_HashTable::iterator (wxPGHashMapS2S_wxImplementation_HashTable::*)()) &wxPGHashMapS2S_wxImplementation_HashTable::begin, "C++: wxPGHashMapS2S_wxImplementation_HashTable::begin() --> class wxPGHashMapS2S_wxImplementation_HashTable::iterator");
		cl.def("erase", (unsigned long (wxPGHashMapS2S_wxImplementation_HashTable::*)(const class wxString &)) &wxPGHashMapS2S_wxImplementation_HashTable::erase, "C++: wxPGHashMapS2S_wxImplementation_HashTable::erase(const class wxString &) --> unsigned long", pybind11::arg("key"));

		{ // wxPGHashMapS2S_wxImplementation_HashTable::Node file: line:160
			auto & enclosing_class = cl;
			pybind11::class_<wxPGHashMapS2S_wxImplementation_HashTable::Node, std::shared_ptr<wxPGHashMapS2S_wxImplementation_HashTable::Node>, _wxHashTable_NodeBase> cl(enclosing_class, "Node", "");
			cl.def( pybind11::init<const class wxPGHashMapS2S_wxImplementation_Pair &>(), pybind11::arg("value") );

			cl.def( pybind11::init( [](wxPGHashMapS2S_wxImplementation_HashTable::Node const &o){ return new wxPGHashMapS2S_wxImplementation_HashTable::Node(o); } ) );
			cl.def_readwrite("m_value", &wxPGHashMapS2S_wxImplementation_HashTable::Node::m_value);
			cl.def("next", (struct wxPGHashMapS2S_wxImplementation_HashTable::Node * (wxPGHashMapS2S_wxImplementation_HashTable::Node::*)()) &wxPGHashMapS2S_wxImplementation_HashTable::Node::next, "C++: wxPGHashMapS2S_wxImplementation_HashTable::Node::next() --> struct wxPGHashMapS2S_wxImplementation_HashTable::Node *", pybind11::return_value_policy::automatic);
		}

		{ // wxPGHashMapS2S_wxImplementation_HashTable::Iterator file: line:179
			auto & enclosing_class = cl;
			pybind11::class_<wxPGHashMapS2S_wxImplementation_HashTable::Iterator, std::shared_ptr<wxPGHashMapS2S_wxImplementation_HashTable::Iterator>> cl(enclosing_class, "Iterator", "");
			cl.def( pybind11::init( [](){ return new wxPGHashMapS2S_wxImplementation_HashTable::Iterator(); } ) );
			cl.def( pybind11::init<struct wxPGHashMapS2S_wxImplementation_HashTable::Node *, const class wxPGHashMapS2S_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxPGHashMapS2S_wxImplementation_HashTable::Iterator const &o){ return new wxPGHashMapS2S_wxImplementation_HashTable::Iterator(o); } ) );
			cl.def("__eq__", (bool (wxPGHashMapS2S_wxImplementation_HashTable::Iterator::*)(const class wxPGHashMapS2S_wxImplementation_HashTable::Iterator &) const) &wxPGHashMapS2S_wxImplementation_HashTable::Iterator::operator==, "C++: wxPGHashMapS2S_wxImplementation_HashTable::Iterator::operator==(const class wxPGHashMapS2S_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxPGHashMapS2S_wxImplementation_HashTable::Iterator::*)(const class wxPGHashMapS2S_wxImplementation_HashTable::Iterator &) const) &wxPGHashMapS2S_wxImplementation_HashTable::Iterator::operator!=, "C++: wxPGHashMapS2S_wxImplementation_HashTable::Iterator::operator!=(const class wxPGHashMapS2S_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxPGHashMapS2S_wxImplementation_HashTable::iterator file: line:213
			auto & enclosing_class = cl;
			pybind11::class_<wxPGHashMapS2S_wxImplementation_HashTable::iterator, std::shared_ptr<wxPGHashMapS2S_wxImplementation_HashTable::iterator>, wxPGHashMapS2S_wxImplementation_HashTable::Iterator> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init( [](){ return new wxPGHashMapS2S_wxImplementation_HashTable::iterator(); } ) );
			cl.def( pybind11::init<struct wxPGHashMapS2S_wxImplementation_HashTable::Node *, class wxPGHashMapS2S_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxPGHashMapS2S_wxImplementation_HashTable::iterator const &o){ return new wxPGHashMapS2S_wxImplementation_HashTable::iterator(o); } ) );
			cl.def("plus_plus", (class wxPGHashMapS2S_wxImplementation_HashTable::iterator & (wxPGHashMapS2S_wxImplementation_HashTable::iterator::*)()) &wxPGHashMapS2S_wxImplementation_HashTable::iterator::operator++, "C++: wxPGHashMapS2S_wxImplementation_HashTable::iterator::operator++() --> class wxPGHashMapS2S_wxImplementation_HashTable::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxPGHashMapS2S_wxImplementation_HashTable::iterator (wxPGHashMapS2S_wxImplementation_HashTable::iterator::*)(int)) &wxPGHashMapS2S_wxImplementation_HashTable::iterator::operator++, "C++: wxPGHashMapS2S_wxImplementation_HashTable::iterator::operator++(int) --> class wxPGHashMapS2S_wxImplementation_HashTable::iterator", pybind11::arg(""));
			cl.def("__mul__", (class wxPGHashMapS2S_wxImplementation_Pair & (wxPGHashMapS2S_wxImplementation_HashTable::iterator::*)() const) &wxPGHashMapS2S_wxImplementation_HashTable::iterator::operator*, "C++: wxPGHashMapS2S_wxImplementation_HashTable::iterator::operator*() const --> class wxPGHashMapS2S_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

		{ // wxPGHashMapS2S_wxImplementation_HashTable::const_iterator file: line:224
			auto & enclosing_class = cl;
			pybind11::class_<wxPGHashMapS2S_wxImplementation_HashTable::const_iterator, std::shared_ptr<wxPGHashMapS2S_wxImplementation_HashTable::const_iterator>, wxPGHashMapS2S_wxImplementation_HashTable::Iterator> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init( [](){ return new wxPGHashMapS2S_wxImplementation_HashTable::const_iterator(); } ) );
			cl.def( pybind11::init<class wxPGHashMapS2S_wxImplementation_HashTable::iterator>(), pybind11::arg("i") );

			cl.def( pybind11::init<struct wxPGHashMapS2S_wxImplementation_HashTable::Node *, const class wxPGHashMapS2S_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxPGHashMapS2S_wxImplementation_HashTable::const_iterator const &o){ return new wxPGHashMapS2S_wxImplementation_HashTable::const_iterator(o); } ) );
			cl.def("plus_plus", (class wxPGHashMapS2S_wxImplementation_HashTable::const_iterator & (wxPGHashMapS2S_wxImplementation_HashTable::const_iterator::*)()) &wxPGHashMapS2S_wxImplementation_HashTable::const_iterator::operator++, "C++: wxPGHashMapS2S_wxImplementation_HashTable::const_iterator::operator++() --> class wxPGHashMapS2S_wxImplementation_HashTable::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxPGHashMapS2S_wxImplementation_HashTable::const_iterator (wxPGHashMapS2S_wxImplementation_HashTable::const_iterator::*)(int)) &wxPGHashMapS2S_wxImplementation_HashTable::const_iterator::operator++, "C++: wxPGHashMapS2S_wxImplementation_HashTable::const_iterator::operator++(int) --> class wxPGHashMapS2S_wxImplementation_HashTable::const_iterator", pybind11::arg(""));
			cl.def("__mul__", (const class wxPGHashMapS2S_wxImplementation_Pair & (wxPGHashMapS2S_wxImplementation_HashTable::const_iterator::*)() const) &wxPGHashMapS2S_wxImplementation_HashTable::const_iterator::operator*, "C++: wxPGHashMapS2S_wxImplementation_HashTable::const_iterator::operator*() const --> const class wxPGHashMapS2S_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

	}
	{ // wxPGHashMapS2S file: line:304
		pybind11::class_<wxPGHashMapS2S, std::shared_ptr<wxPGHashMapS2S>, wxPGHashMapS2S_wxImplementation_HashTable> cl(M(""), "wxPGHashMapS2S", "");
		cl.def( pybind11::init( [](){ return new wxPGHashMapS2S(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxPGHashMapS2S(a0); } ), "doc" , pybind11::arg("hint"));
		cl.def( pybind11::init( [](unsigned long const & a0, struct wxStringHash const & a1){ return new wxPGHashMapS2S(a0, a1); } ), "doc" , pybind11::arg("hint"), pybind11::arg("hf"));
		cl.def( pybind11::init<unsigned long, struct wxStringHash, struct wxStringEqual>(), pybind11::arg("hint"), pybind11::arg("hf"), pybind11::arg("eq") );

		cl.def( pybind11::init( [](wxPGHashMapS2S const &o){ return new wxPGHashMapS2S(o); } ) );
		cl.def("__getitem__", (class wxString & (wxPGHashMapS2S::*)(const class wxString &)) &wxPGHashMapS2S::operator[], "C++: wxPGHashMapS2S::operator[](const class wxString &) --> class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("key"));
		cl.def("find", (class wxPGHashMapS2S_wxImplementation_HashTable::iterator (wxPGHashMapS2S::*)(const class wxString &)) &wxPGHashMapS2S::find, "C++: wxPGHashMapS2S::find(const class wxString &) --> class wxPGHashMapS2S_wxImplementation_HashTable::iterator", pybind11::arg("key"));
		cl.def("insert", (class wxPGHashMapS2S::Insert_Result (wxPGHashMapS2S::*)(const class wxPGHashMapS2S_wxImplementation_Pair &)) &wxPGHashMapS2S::insert, "C++: wxPGHashMapS2S::insert(const class wxPGHashMapS2S_wxImplementation_Pair &) --> class wxPGHashMapS2S::Insert_Result", pybind11::arg("v"));
		cl.def("erase", (unsigned long (wxPGHashMapS2S::*)(const class wxString &)) &wxPGHashMapS2S::erase, "C++: wxPGHashMapS2S::erase(const class wxString &) --> unsigned long", pybind11::arg("k"));
		cl.def("erase", (void (wxPGHashMapS2S::*)(const class wxPGHashMapS2S_wxImplementation_HashTable::iterator &)) &wxPGHashMapS2S::erase, "C++: wxPGHashMapS2S::erase(const class wxPGHashMapS2S_wxImplementation_HashTable::iterator &) --> void", pybind11::arg("it"));
		cl.def("count", (unsigned long (wxPGHashMapS2S::*)(const class wxString &)) &wxPGHashMapS2S::count, "C++: wxPGHashMapS2S::count(const class wxString &) --> unsigned long", pybind11::arg("key"));
		cl.def("assign", (class wxPGHashMapS2S & (wxPGHashMapS2S::*)(const class wxPGHashMapS2S &)) &wxPGHashMapS2S::operator=, "C++: wxPGHashMapS2S::operator=(const class wxPGHashMapS2S &) --> class wxPGHashMapS2S &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxPGHashMapS2S::Insert_Result file: line:645
			auto & enclosing_class = cl;
			pybind11::class_<wxPGHashMapS2S::Insert_Result, std::shared_ptr<wxPGHashMapS2S::Insert_Result>> cl(enclosing_class, "Insert_Result", "");
			cl.def( pybind11::init<const class wxPGHashMapS2S_wxImplementation_HashTable::iterator &, const bool &>(), pybind11::arg("f"), pybind11::arg("s") );

			cl.def( pybind11::init( [](wxPGHashMapS2S::Insert_Result const &o){ return new wxPGHashMapS2S::Insert_Result(o); } ) );
			cl.def_readwrite("first", &wxPGHashMapS2S::Insert_Result::first);
			cl.def_readwrite("second", &wxPGHashMapS2S::Insert_Result::second);
		}

	}
	{ // wxPGHashMapP2P_wxImplementation_Pair file: line:13
		pybind11::class_<wxPGHashMapP2P_wxImplementation_Pair, std::shared_ptr<wxPGHashMapP2P_wxImplementation_Pair>> cl(M(""), "wxPGHashMapP2P_wxImplementation_Pair", "");
		cl.def( pybind11::init<const void *const &, const void *const &>(), pybind11::arg("f"), pybind11::arg("s") );

		cl.def( pybind11::init( [](wxPGHashMapP2P_wxImplementation_Pair const &o){ return new wxPGHashMapP2P_wxImplementation_Pair(o); } ) );
	}
	{ // wxPGHashMapP2P_wxImplementation_KeyEx file: line:15
		pybind11::class_<wxPGHashMapP2P_wxImplementation_KeyEx, std::shared_ptr<wxPGHashMapP2P_wxImplementation_KeyEx>> cl(M(""), "wxPGHashMapP2P_wxImplementation_KeyEx", "");
		cl.def( pybind11::init( [](){ return new wxPGHashMapP2P_wxImplementation_KeyEx(); } ) );
		cl.def( pybind11::init( [](wxPGHashMapP2P_wxImplementation_KeyEx const &o){ return new wxPGHashMapP2P_wxImplementation_KeyEx(o); } ) );
		cl.def("__call__", (void *const & (wxPGHashMapP2P_wxImplementation_KeyEx::*)(const class wxPGHashMapP2P_wxImplementation_Pair &) const) &wxPGHashMapP2P_wxImplementation_KeyEx::operator(), "C++: wxPGHashMapP2P_wxImplementation_KeyEx::operator()(const class wxPGHashMapP2P_wxImplementation_Pair &) const --> void *const &", pybind11::return_value_policy::automatic, pybind11::arg("pair"));
		cl.def("assign", (class wxPGHashMapP2P_wxImplementation_KeyEx & (wxPGHashMapP2P_wxImplementation_KeyEx::*)(const class wxPGHashMapP2P_wxImplementation_KeyEx &)) &wxPGHashMapP2P_wxImplementation_KeyEx::operator=, "C++: wxPGHashMapP2P_wxImplementation_KeyEx::operator=(const class wxPGHashMapP2P_wxImplementation_KeyEx &) --> class wxPGHashMapP2P_wxImplementation_KeyEx &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}

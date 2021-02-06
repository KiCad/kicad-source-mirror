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

void bind_unknown_unknown_84(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxPGHashMapP2P_wxImplementation_HashTable file: line:18
		pybind11::class_<wxPGHashMapP2P_wxImplementation_HashTable, std::shared_ptr<wxPGHashMapP2P_wxImplementation_HashTable>> cl(M(""), "wxPGHashMapP2P_wxImplementation_HashTable", "");
		cl.def( pybind11::init( [](){ return new wxPGHashMapP2P_wxImplementation_HashTable(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxPGHashMapP2P_wxImplementation_HashTable(a0); } ), "doc" , pybind11::arg("sz"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxPointerHash & a1){ return new wxPGHashMapP2P_wxImplementation_HashTable(a0, a1); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxPointerHash & a1, const struct wxPointerEqual & a2){ return new wxPGHashMapP2P_wxImplementation_HashTable(a0, a1, a2); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"));
		cl.def( pybind11::init<unsigned long, const struct wxPointerHash &, const struct wxPointerEqual &, const class wxPGHashMapP2P_wxImplementation_KeyEx &>(), pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"), pybind11::arg("k_ex") );

		cl.def( pybind11::init( [](wxPGHashMapP2P_wxImplementation_HashTable const &o){ return new wxPGHashMapP2P_wxImplementation_HashTable(o); } ) );
		cl.def("assign", (const class wxPGHashMapP2P_wxImplementation_HashTable & (wxPGHashMapP2P_wxImplementation_HashTable::*)(const class wxPGHashMapP2P_wxImplementation_HashTable &)) &wxPGHashMapP2P_wxImplementation_HashTable::operator=, "C++: wxPGHashMapP2P_wxImplementation_HashTable::operator=(const class wxPGHashMapP2P_wxImplementation_HashTable &) --> const class wxPGHashMapP2P_wxImplementation_HashTable &", pybind11::return_value_policy::automatic, pybind11::arg("ht"));
		cl.def("hash_funct", (struct wxPointerHash (wxPGHashMapP2P_wxImplementation_HashTable::*)()) &wxPGHashMapP2P_wxImplementation_HashTable::hash_funct, "C++: wxPGHashMapP2P_wxImplementation_HashTable::hash_funct() --> struct wxPointerHash");
		cl.def("key_eq", (struct wxPointerEqual (wxPGHashMapP2P_wxImplementation_HashTable::*)()) &wxPGHashMapP2P_wxImplementation_HashTable::key_eq, "C++: wxPGHashMapP2P_wxImplementation_HashTable::key_eq() --> struct wxPointerEqual");
		cl.def("clear", (void (wxPGHashMapP2P_wxImplementation_HashTable::*)()) &wxPGHashMapP2P_wxImplementation_HashTable::clear, "C++: wxPGHashMapP2P_wxImplementation_HashTable::clear() --> void");
		cl.def("size", (unsigned long (wxPGHashMapP2P_wxImplementation_HashTable::*)() const) &wxPGHashMapP2P_wxImplementation_HashTable::size, "C++: wxPGHashMapP2P_wxImplementation_HashTable::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxPGHashMapP2P_wxImplementation_HashTable::*)() const) &wxPGHashMapP2P_wxImplementation_HashTable::max_size, "C++: wxPGHashMapP2P_wxImplementation_HashTable::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxPGHashMapP2P_wxImplementation_HashTable::*)() const) &wxPGHashMapP2P_wxImplementation_HashTable::empty, "C++: wxPGHashMapP2P_wxImplementation_HashTable::empty() const --> bool");
		cl.def("end", (class wxPGHashMapP2P_wxImplementation_HashTable::iterator (wxPGHashMapP2P_wxImplementation_HashTable::*)()) &wxPGHashMapP2P_wxImplementation_HashTable::end, "C++: wxPGHashMapP2P_wxImplementation_HashTable::end() --> class wxPGHashMapP2P_wxImplementation_HashTable::iterator");
		cl.def("begin", (class wxPGHashMapP2P_wxImplementation_HashTable::iterator (wxPGHashMapP2P_wxImplementation_HashTable::*)()) &wxPGHashMapP2P_wxImplementation_HashTable::begin, "C++: wxPGHashMapP2P_wxImplementation_HashTable::begin() --> class wxPGHashMapP2P_wxImplementation_HashTable::iterator");
		cl.def("erase", (unsigned long (wxPGHashMapP2P_wxImplementation_HashTable::*)(const void *const &)) &wxPGHashMapP2P_wxImplementation_HashTable::erase, "C++: wxPGHashMapP2P_wxImplementation_HashTable::erase(const void *const &) --> unsigned long", pybind11::arg("key"));

		{ // wxPGHashMapP2P_wxImplementation_HashTable::Node file: line:160
			auto & enclosing_class = cl;
			pybind11::class_<wxPGHashMapP2P_wxImplementation_HashTable::Node, std::shared_ptr<wxPGHashMapP2P_wxImplementation_HashTable::Node>, _wxHashTable_NodeBase> cl(enclosing_class, "Node", "");
			cl.def( pybind11::init<const class wxPGHashMapP2P_wxImplementation_Pair &>(), pybind11::arg("value") );

			cl.def( pybind11::init( [](wxPGHashMapP2P_wxImplementation_HashTable::Node const &o){ return new wxPGHashMapP2P_wxImplementation_HashTable::Node(o); } ) );
			cl.def_readwrite("m_value", &wxPGHashMapP2P_wxImplementation_HashTable::Node::m_value);
			cl.def("next", (struct wxPGHashMapP2P_wxImplementation_HashTable::Node * (wxPGHashMapP2P_wxImplementation_HashTable::Node::*)()) &wxPGHashMapP2P_wxImplementation_HashTable::Node::next, "C++: wxPGHashMapP2P_wxImplementation_HashTable::Node::next() --> struct wxPGHashMapP2P_wxImplementation_HashTable::Node *", pybind11::return_value_policy::automatic);
		}

		{ // wxPGHashMapP2P_wxImplementation_HashTable::Iterator file: line:179
			auto & enclosing_class = cl;
			pybind11::class_<wxPGHashMapP2P_wxImplementation_HashTable::Iterator, std::shared_ptr<wxPGHashMapP2P_wxImplementation_HashTable::Iterator>> cl(enclosing_class, "Iterator", "");
			cl.def( pybind11::init( [](){ return new wxPGHashMapP2P_wxImplementation_HashTable::Iterator(); } ) );
			cl.def( pybind11::init<struct wxPGHashMapP2P_wxImplementation_HashTable::Node *, const class wxPGHashMapP2P_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxPGHashMapP2P_wxImplementation_HashTable::Iterator const &o){ return new wxPGHashMapP2P_wxImplementation_HashTable::Iterator(o); } ) );
			cl.def("__eq__", (bool (wxPGHashMapP2P_wxImplementation_HashTable::Iterator::*)(const class wxPGHashMapP2P_wxImplementation_HashTable::Iterator &) const) &wxPGHashMapP2P_wxImplementation_HashTable::Iterator::operator==, "C++: wxPGHashMapP2P_wxImplementation_HashTable::Iterator::operator==(const class wxPGHashMapP2P_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxPGHashMapP2P_wxImplementation_HashTable::Iterator::*)(const class wxPGHashMapP2P_wxImplementation_HashTable::Iterator &) const) &wxPGHashMapP2P_wxImplementation_HashTable::Iterator::operator!=, "C++: wxPGHashMapP2P_wxImplementation_HashTable::Iterator::operator!=(const class wxPGHashMapP2P_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxPGHashMapP2P_wxImplementation_HashTable::iterator file: line:213
			auto & enclosing_class = cl;
			pybind11::class_<wxPGHashMapP2P_wxImplementation_HashTable::iterator, std::shared_ptr<wxPGHashMapP2P_wxImplementation_HashTable::iterator>, wxPGHashMapP2P_wxImplementation_HashTable::Iterator> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init( [](){ return new wxPGHashMapP2P_wxImplementation_HashTable::iterator(); } ) );
			cl.def( pybind11::init<struct wxPGHashMapP2P_wxImplementation_HashTable::Node *, class wxPGHashMapP2P_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxPGHashMapP2P_wxImplementation_HashTable::iterator const &o){ return new wxPGHashMapP2P_wxImplementation_HashTable::iterator(o); } ) );
			cl.def("plus_plus", (class wxPGHashMapP2P_wxImplementation_HashTable::iterator & (wxPGHashMapP2P_wxImplementation_HashTable::iterator::*)()) &wxPGHashMapP2P_wxImplementation_HashTable::iterator::operator++, "C++: wxPGHashMapP2P_wxImplementation_HashTable::iterator::operator++() --> class wxPGHashMapP2P_wxImplementation_HashTable::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxPGHashMapP2P_wxImplementation_HashTable::iterator (wxPGHashMapP2P_wxImplementation_HashTable::iterator::*)(int)) &wxPGHashMapP2P_wxImplementation_HashTable::iterator::operator++, "C++: wxPGHashMapP2P_wxImplementation_HashTable::iterator::operator++(int) --> class wxPGHashMapP2P_wxImplementation_HashTable::iterator", pybind11::arg(""));
			cl.def("__mul__", (class wxPGHashMapP2P_wxImplementation_Pair & (wxPGHashMapP2P_wxImplementation_HashTable::iterator::*)() const) &wxPGHashMapP2P_wxImplementation_HashTable::iterator::operator*, "C++: wxPGHashMapP2P_wxImplementation_HashTable::iterator::operator*() const --> class wxPGHashMapP2P_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

		{ // wxPGHashMapP2P_wxImplementation_HashTable::const_iterator file: line:224
			auto & enclosing_class = cl;
			pybind11::class_<wxPGHashMapP2P_wxImplementation_HashTable::const_iterator, std::shared_ptr<wxPGHashMapP2P_wxImplementation_HashTable::const_iterator>, wxPGHashMapP2P_wxImplementation_HashTable::Iterator> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init( [](){ return new wxPGHashMapP2P_wxImplementation_HashTable::const_iterator(); } ) );
			cl.def( pybind11::init<class wxPGHashMapP2P_wxImplementation_HashTable::iterator>(), pybind11::arg("i") );

			cl.def( pybind11::init<struct wxPGHashMapP2P_wxImplementation_HashTable::Node *, const class wxPGHashMapP2P_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxPGHashMapP2P_wxImplementation_HashTable::const_iterator const &o){ return new wxPGHashMapP2P_wxImplementation_HashTable::const_iterator(o); } ) );
			cl.def("plus_plus", (class wxPGHashMapP2P_wxImplementation_HashTable::const_iterator & (wxPGHashMapP2P_wxImplementation_HashTable::const_iterator::*)()) &wxPGHashMapP2P_wxImplementation_HashTable::const_iterator::operator++, "C++: wxPGHashMapP2P_wxImplementation_HashTable::const_iterator::operator++() --> class wxPGHashMapP2P_wxImplementation_HashTable::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxPGHashMapP2P_wxImplementation_HashTable::const_iterator (wxPGHashMapP2P_wxImplementation_HashTable::const_iterator::*)(int)) &wxPGHashMapP2P_wxImplementation_HashTable::const_iterator::operator++, "C++: wxPGHashMapP2P_wxImplementation_HashTable::const_iterator::operator++(int) --> class wxPGHashMapP2P_wxImplementation_HashTable::const_iterator", pybind11::arg(""));
			cl.def("__mul__", (const class wxPGHashMapP2P_wxImplementation_Pair & (wxPGHashMapP2P_wxImplementation_HashTable::const_iterator::*)() const) &wxPGHashMapP2P_wxImplementation_HashTable::const_iterator::operator*, "C++: wxPGHashMapP2P_wxImplementation_HashTable::const_iterator::operator*() const --> const class wxPGHashMapP2P_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

	}
	{ // wxPGHashMapP2P file: line:308
		pybind11::class_<wxPGHashMapP2P, std::shared_ptr<wxPGHashMapP2P>, wxPGHashMapP2P_wxImplementation_HashTable> cl(M(""), "wxPGHashMapP2P", "");
		cl.def( pybind11::init( [](){ return new wxPGHashMapP2P(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxPGHashMapP2P(a0); } ), "doc" , pybind11::arg("hint"));
		cl.def( pybind11::init( [](unsigned long const & a0, struct wxPointerHash const & a1){ return new wxPGHashMapP2P(a0, a1); } ), "doc" , pybind11::arg("hint"), pybind11::arg("hf"));
		cl.def( pybind11::init<unsigned long, struct wxPointerHash, struct wxPointerEqual>(), pybind11::arg("hint"), pybind11::arg("hf"), pybind11::arg("eq") );

		cl.def( pybind11::init( [](wxPGHashMapP2P const &o){ return new wxPGHashMapP2P(o); } ) );
		cl.def("__getitem__", (void *& (wxPGHashMapP2P::*)(const void *const &)) &wxPGHashMapP2P::operator[], "C++: wxPGHashMapP2P::operator[](const void *const &) --> void *&", pybind11::return_value_policy::automatic, pybind11::arg("key"));
		cl.def("find", (class wxPGHashMapP2P_wxImplementation_HashTable::iterator (wxPGHashMapP2P::*)(const void *const &)) &wxPGHashMapP2P::find, "C++: wxPGHashMapP2P::find(const void *const &) --> class wxPGHashMapP2P_wxImplementation_HashTable::iterator", pybind11::arg("key"));
		cl.def("insert", (class wxPGHashMapP2P::Insert_Result (wxPGHashMapP2P::*)(const class wxPGHashMapP2P_wxImplementation_Pair &)) &wxPGHashMapP2P::insert, "C++: wxPGHashMapP2P::insert(const class wxPGHashMapP2P_wxImplementation_Pair &) --> class wxPGHashMapP2P::Insert_Result", pybind11::arg("v"));
		cl.def("erase", (unsigned long (wxPGHashMapP2P::*)(void *const &)) &wxPGHashMapP2P::erase, "C++: wxPGHashMapP2P::erase(void *const &) --> unsigned long", pybind11::arg("k"));
		cl.def("erase", (void (wxPGHashMapP2P::*)(const class wxPGHashMapP2P_wxImplementation_HashTable::iterator &)) &wxPGHashMapP2P::erase, "C++: wxPGHashMapP2P::erase(const class wxPGHashMapP2P_wxImplementation_HashTable::iterator &) --> void", pybind11::arg("it"));
		cl.def("count", (unsigned long (wxPGHashMapP2P::*)(const void *const &)) &wxPGHashMapP2P::count, "C++: wxPGHashMapP2P::count(const void *const &) --> unsigned long", pybind11::arg("key"));
		cl.def("assign", (class wxPGHashMapP2P & (wxPGHashMapP2P::*)(const class wxPGHashMapP2P &)) &wxPGHashMapP2P::operator=, "C++: wxPGHashMapP2P::operator=(const class wxPGHashMapP2P &) --> class wxPGHashMapP2P &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxPGHashMapP2P::Insert_Result file: line:645
			auto & enclosing_class = cl;
			pybind11::class_<wxPGHashMapP2P::Insert_Result, std::shared_ptr<wxPGHashMapP2P::Insert_Result>> cl(enclosing_class, "Insert_Result", "");
			cl.def( pybind11::init<const class wxPGHashMapP2P_wxImplementation_HashTable::iterator &, const bool &>(), pybind11::arg("f"), pybind11::arg("s") );

			cl.def( pybind11::init( [](wxPGHashMapP2P::Insert_Result const &o){ return new wxPGHashMapP2P::Insert_Result(o); } ) );
			cl.def_readwrite("first", &wxPGHashMapP2P::Insert_Result::first);
			cl.def_readwrite("second", &wxPGHashMapP2P::Insert_Result::second);
		}

	}
	{ // wxPGHashMapI2I_wxImplementation_Pair file: line:25
		pybind11::class_<wxPGHashMapI2I_wxImplementation_Pair, std::shared_ptr<wxPGHashMapI2I_wxImplementation_Pair>> cl(M(""), "wxPGHashMapI2I_wxImplementation_Pair", "");
		cl.def( pybind11::init<const int &, const int &>(), pybind11::arg("f"), pybind11::arg("s") );

		cl.def( pybind11::init( [](wxPGHashMapI2I_wxImplementation_Pair const &o){ return new wxPGHashMapI2I_wxImplementation_Pair(o); } ) );
		cl.def_readwrite("first", &wxPGHashMapI2I_wxImplementation_Pair::first);
		cl.def_readwrite("second", &wxPGHashMapI2I_wxImplementation_Pair::second);
	}
	{ // wxPGHashMapI2I_wxImplementation_KeyEx file: line:27
		pybind11::class_<wxPGHashMapI2I_wxImplementation_KeyEx, std::shared_ptr<wxPGHashMapI2I_wxImplementation_KeyEx>> cl(M(""), "wxPGHashMapI2I_wxImplementation_KeyEx", "");
		cl.def( pybind11::init( [](){ return new wxPGHashMapI2I_wxImplementation_KeyEx(); } ) );
		cl.def( pybind11::init( [](wxPGHashMapI2I_wxImplementation_KeyEx const &o){ return new wxPGHashMapI2I_wxImplementation_KeyEx(o); } ) );
		cl.def("__call__", (const int & (wxPGHashMapI2I_wxImplementation_KeyEx::*)(const class wxPGHashMapI2I_wxImplementation_Pair &) const) &wxPGHashMapI2I_wxImplementation_KeyEx::operator(), "C++: wxPGHashMapI2I_wxImplementation_KeyEx::operator()(const class wxPGHashMapI2I_wxImplementation_Pair &) const --> const int &", pybind11::return_value_policy::automatic, pybind11::arg("pair"));
		cl.def("assign", (class wxPGHashMapI2I_wxImplementation_KeyEx & (wxPGHashMapI2I_wxImplementation_KeyEx::*)(const class wxPGHashMapI2I_wxImplementation_KeyEx &)) &wxPGHashMapI2I_wxImplementation_KeyEx::operator=, "C++: wxPGHashMapI2I_wxImplementation_KeyEx::operator=(const class wxPGHashMapI2I_wxImplementation_KeyEx &) --> class wxPGHashMapI2I_wxImplementation_KeyEx &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxPGHashMapI2I_wxImplementation_HashTable file: line:30
		pybind11::class_<wxPGHashMapI2I_wxImplementation_HashTable, std::shared_ptr<wxPGHashMapI2I_wxImplementation_HashTable>> cl(M(""), "wxPGHashMapI2I_wxImplementation_HashTable", "");
		cl.def( pybind11::init( [](){ return new wxPGHashMapI2I_wxImplementation_HashTable(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxPGHashMapI2I_wxImplementation_HashTable(a0); } ), "doc" , pybind11::arg("sz"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxIntegerHash & a1){ return new wxPGHashMapI2I_wxImplementation_HashTable(a0, a1); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxIntegerHash & a1, const struct wxIntegerEqual & a2){ return new wxPGHashMapI2I_wxImplementation_HashTable(a0, a1, a2); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"));
		cl.def( pybind11::init<unsigned long, const struct wxIntegerHash &, const struct wxIntegerEqual &, const class wxPGHashMapI2I_wxImplementation_KeyEx &>(), pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"), pybind11::arg("k_ex") );

		cl.def( pybind11::init( [](wxPGHashMapI2I_wxImplementation_HashTable const &o){ return new wxPGHashMapI2I_wxImplementation_HashTable(o); } ) );
		cl.def("assign", (const class wxPGHashMapI2I_wxImplementation_HashTable & (wxPGHashMapI2I_wxImplementation_HashTable::*)(const class wxPGHashMapI2I_wxImplementation_HashTable &)) &wxPGHashMapI2I_wxImplementation_HashTable::operator=, "C++: wxPGHashMapI2I_wxImplementation_HashTable::operator=(const class wxPGHashMapI2I_wxImplementation_HashTable &) --> const class wxPGHashMapI2I_wxImplementation_HashTable &", pybind11::return_value_policy::automatic, pybind11::arg("ht"));
		cl.def("hash_funct", (struct wxIntegerHash (wxPGHashMapI2I_wxImplementation_HashTable::*)()) &wxPGHashMapI2I_wxImplementation_HashTable::hash_funct, "C++: wxPGHashMapI2I_wxImplementation_HashTable::hash_funct() --> struct wxIntegerHash");
		cl.def("key_eq", (struct wxIntegerEqual (wxPGHashMapI2I_wxImplementation_HashTable::*)()) &wxPGHashMapI2I_wxImplementation_HashTable::key_eq, "C++: wxPGHashMapI2I_wxImplementation_HashTable::key_eq() --> struct wxIntegerEqual");
		cl.def("clear", (void (wxPGHashMapI2I_wxImplementation_HashTable::*)()) &wxPGHashMapI2I_wxImplementation_HashTable::clear, "C++: wxPGHashMapI2I_wxImplementation_HashTable::clear() --> void");
		cl.def("size", (unsigned long (wxPGHashMapI2I_wxImplementation_HashTable::*)() const) &wxPGHashMapI2I_wxImplementation_HashTable::size, "C++: wxPGHashMapI2I_wxImplementation_HashTable::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxPGHashMapI2I_wxImplementation_HashTable::*)() const) &wxPGHashMapI2I_wxImplementation_HashTable::max_size, "C++: wxPGHashMapI2I_wxImplementation_HashTable::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxPGHashMapI2I_wxImplementation_HashTable::*)() const) &wxPGHashMapI2I_wxImplementation_HashTable::empty, "C++: wxPGHashMapI2I_wxImplementation_HashTable::empty() const --> bool");
		cl.def("end", (class wxPGHashMapI2I_wxImplementation_HashTable::iterator (wxPGHashMapI2I_wxImplementation_HashTable::*)()) &wxPGHashMapI2I_wxImplementation_HashTable::end, "C++: wxPGHashMapI2I_wxImplementation_HashTable::end() --> class wxPGHashMapI2I_wxImplementation_HashTable::iterator");
		cl.def("begin", (class wxPGHashMapI2I_wxImplementation_HashTable::iterator (wxPGHashMapI2I_wxImplementation_HashTable::*)()) &wxPGHashMapI2I_wxImplementation_HashTable::begin, "C++: wxPGHashMapI2I_wxImplementation_HashTable::begin() --> class wxPGHashMapI2I_wxImplementation_HashTable::iterator");
		cl.def("erase", (unsigned long (wxPGHashMapI2I_wxImplementation_HashTable::*)(const int &)) &wxPGHashMapI2I_wxImplementation_HashTable::erase, "C++: wxPGHashMapI2I_wxImplementation_HashTable::erase(const int &) --> unsigned long", pybind11::arg("key"));

		{ // wxPGHashMapI2I_wxImplementation_HashTable::Node file: line:160
			auto & enclosing_class = cl;
			pybind11::class_<wxPGHashMapI2I_wxImplementation_HashTable::Node, std::shared_ptr<wxPGHashMapI2I_wxImplementation_HashTable::Node>, _wxHashTable_NodeBase> cl(enclosing_class, "Node", "");
			cl.def( pybind11::init<const class wxPGHashMapI2I_wxImplementation_Pair &>(), pybind11::arg("value") );

			cl.def( pybind11::init( [](wxPGHashMapI2I_wxImplementation_HashTable::Node const &o){ return new wxPGHashMapI2I_wxImplementation_HashTable::Node(o); } ) );
			cl.def_readwrite("m_value", &wxPGHashMapI2I_wxImplementation_HashTable::Node::m_value);
			cl.def("next", (struct wxPGHashMapI2I_wxImplementation_HashTable::Node * (wxPGHashMapI2I_wxImplementation_HashTable::Node::*)()) &wxPGHashMapI2I_wxImplementation_HashTable::Node::next, "C++: wxPGHashMapI2I_wxImplementation_HashTable::Node::next() --> struct wxPGHashMapI2I_wxImplementation_HashTable::Node *", pybind11::return_value_policy::automatic);
		}

		{ // wxPGHashMapI2I_wxImplementation_HashTable::Iterator file: line:179
			auto & enclosing_class = cl;
			pybind11::class_<wxPGHashMapI2I_wxImplementation_HashTable::Iterator, std::shared_ptr<wxPGHashMapI2I_wxImplementation_HashTable::Iterator>> cl(enclosing_class, "Iterator", "");
			cl.def( pybind11::init( [](){ return new wxPGHashMapI2I_wxImplementation_HashTable::Iterator(); } ) );
			cl.def( pybind11::init<struct wxPGHashMapI2I_wxImplementation_HashTable::Node *, const class wxPGHashMapI2I_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxPGHashMapI2I_wxImplementation_HashTable::Iterator const &o){ return new wxPGHashMapI2I_wxImplementation_HashTable::Iterator(o); } ) );
			cl.def("__eq__", (bool (wxPGHashMapI2I_wxImplementation_HashTable::Iterator::*)(const class wxPGHashMapI2I_wxImplementation_HashTable::Iterator &) const) &wxPGHashMapI2I_wxImplementation_HashTable::Iterator::operator==, "C++: wxPGHashMapI2I_wxImplementation_HashTable::Iterator::operator==(const class wxPGHashMapI2I_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxPGHashMapI2I_wxImplementation_HashTable::Iterator::*)(const class wxPGHashMapI2I_wxImplementation_HashTable::Iterator &) const) &wxPGHashMapI2I_wxImplementation_HashTable::Iterator::operator!=, "C++: wxPGHashMapI2I_wxImplementation_HashTable::Iterator::operator!=(const class wxPGHashMapI2I_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxPGHashMapI2I_wxImplementation_HashTable::iterator file: line:213
			auto & enclosing_class = cl;
			pybind11::class_<wxPGHashMapI2I_wxImplementation_HashTable::iterator, std::shared_ptr<wxPGHashMapI2I_wxImplementation_HashTable::iterator>, wxPGHashMapI2I_wxImplementation_HashTable::Iterator> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init( [](){ return new wxPGHashMapI2I_wxImplementation_HashTable::iterator(); } ) );
			cl.def( pybind11::init<struct wxPGHashMapI2I_wxImplementation_HashTable::Node *, class wxPGHashMapI2I_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxPGHashMapI2I_wxImplementation_HashTable::iterator const &o){ return new wxPGHashMapI2I_wxImplementation_HashTable::iterator(o); } ) );
			cl.def("plus_plus", (class wxPGHashMapI2I_wxImplementation_HashTable::iterator & (wxPGHashMapI2I_wxImplementation_HashTable::iterator::*)()) &wxPGHashMapI2I_wxImplementation_HashTable::iterator::operator++, "C++: wxPGHashMapI2I_wxImplementation_HashTable::iterator::operator++() --> class wxPGHashMapI2I_wxImplementation_HashTable::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxPGHashMapI2I_wxImplementation_HashTable::iterator (wxPGHashMapI2I_wxImplementation_HashTable::iterator::*)(int)) &wxPGHashMapI2I_wxImplementation_HashTable::iterator::operator++, "C++: wxPGHashMapI2I_wxImplementation_HashTable::iterator::operator++(int) --> class wxPGHashMapI2I_wxImplementation_HashTable::iterator", pybind11::arg(""));
			cl.def("__mul__", (class wxPGHashMapI2I_wxImplementation_Pair & (wxPGHashMapI2I_wxImplementation_HashTable::iterator::*)() const) &wxPGHashMapI2I_wxImplementation_HashTable::iterator::operator*, "C++: wxPGHashMapI2I_wxImplementation_HashTable::iterator::operator*() const --> class wxPGHashMapI2I_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

		{ // wxPGHashMapI2I_wxImplementation_HashTable::const_iterator file: line:224
			auto & enclosing_class = cl;
			pybind11::class_<wxPGHashMapI2I_wxImplementation_HashTable::const_iterator, std::shared_ptr<wxPGHashMapI2I_wxImplementation_HashTable::const_iterator>, wxPGHashMapI2I_wxImplementation_HashTable::Iterator> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init( [](){ return new wxPGHashMapI2I_wxImplementation_HashTable::const_iterator(); } ) );
			cl.def( pybind11::init<class wxPGHashMapI2I_wxImplementation_HashTable::iterator>(), pybind11::arg("i") );

			cl.def( pybind11::init<struct wxPGHashMapI2I_wxImplementation_HashTable::Node *, const class wxPGHashMapI2I_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxPGHashMapI2I_wxImplementation_HashTable::const_iterator const &o){ return new wxPGHashMapI2I_wxImplementation_HashTable::const_iterator(o); } ) );
			cl.def("plus_plus", (class wxPGHashMapI2I_wxImplementation_HashTable::const_iterator & (wxPGHashMapI2I_wxImplementation_HashTable::const_iterator::*)()) &wxPGHashMapI2I_wxImplementation_HashTable::const_iterator::operator++, "C++: wxPGHashMapI2I_wxImplementation_HashTable::const_iterator::operator++() --> class wxPGHashMapI2I_wxImplementation_HashTable::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxPGHashMapI2I_wxImplementation_HashTable::const_iterator (wxPGHashMapI2I_wxImplementation_HashTable::const_iterator::*)(int)) &wxPGHashMapI2I_wxImplementation_HashTable::const_iterator::operator++, "C++: wxPGHashMapI2I_wxImplementation_HashTable::const_iterator::operator++(int) --> class wxPGHashMapI2I_wxImplementation_HashTable::const_iterator", pybind11::arg(""));
			cl.def("__mul__", (const class wxPGHashMapI2I_wxImplementation_Pair & (wxPGHashMapI2I_wxImplementation_HashTable::const_iterator::*)() const) &wxPGHashMapI2I_wxImplementation_HashTable::const_iterator::operator*, "C++: wxPGHashMapI2I_wxImplementation_HashTable::const_iterator::operator*() const --> const class wxPGHashMapI2I_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

	}
}

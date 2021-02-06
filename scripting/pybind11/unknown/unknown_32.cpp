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

void bind_unknown_unknown_32(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxRect file: line:696
		pybind11::class_<wxRect, std::shared_ptr<wxRect>> cl(M(""), "wxRect", "");
		cl.def( pybind11::init( [](){ return new wxRect(); } ) );
		cl.def( pybind11::init<int, int, int, int>(), pybind11::arg("xx"), pybind11::arg("yy"), pybind11::arg("ww"), pybind11::arg("hh") );

		cl.def( pybind11::init<const class wxPoint &, const class wxPoint &>(), pybind11::arg("topLeft"), pybind11::arg("bottomRight") );

		cl.def( pybind11::init<const class wxPoint &, const class wxSize &>(), pybind11::arg("pt"), pybind11::arg("size") );

		cl.def( pybind11::init<const class wxSize &>(), pybind11::arg("size") );

		cl.def( pybind11::init( [](wxRect const &o){ return new wxRect(o); } ) );
		cl.def_readwrite("x", &wxRect::x);
		cl.def_readwrite("y", &wxRect::y);
		cl.def_readwrite("width", &wxRect::width);
		cl.def_readwrite("height", &wxRect::height);
		cl.def("GetX", (int (wxRect::*)() const) &wxRect::GetX, "C++: wxRect::GetX() const --> int");
		cl.def("SetX", (void (wxRect::*)(int)) &wxRect::SetX, "C++: wxRect::SetX(int) --> void", pybind11::arg("xx"));
		cl.def("GetY", (int (wxRect::*)() const) &wxRect::GetY, "C++: wxRect::GetY() const --> int");
		cl.def("SetY", (void (wxRect::*)(int)) &wxRect::SetY, "C++: wxRect::SetY(int) --> void", pybind11::arg("yy"));
		cl.def("GetWidth", (int (wxRect::*)() const) &wxRect::GetWidth, "C++: wxRect::GetWidth() const --> int");
		cl.def("SetWidth", (void (wxRect::*)(int)) &wxRect::SetWidth, "C++: wxRect::SetWidth(int) --> void", pybind11::arg("w"));
		cl.def("GetHeight", (int (wxRect::*)() const) &wxRect::GetHeight, "C++: wxRect::GetHeight() const --> int");
		cl.def("SetHeight", (void (wxRect::*)(int)) &wxRect::SetHeight, "C++: wxRect::SetHeight(int) --> void", pybind11::arg("h"));
		cl.def("GetPosition", (class wxPoint (wxRect::*)() const) &wxRect::GetPosition, "C++: wxRect::GetPosition() const --> class wxPoint");
		cl.def("SetPosition", (void (wxRect::*)(const class wxPoint &)) &wxRect::SetPosition, "C++: wxRect::SetPosition(const class wxPoint &) --> void", pybind11::arg("p"));
		cl.def("GetSize", (class wxSize (wxRect::*)() const) &wxRect::GetSize, "C++: wxRect::GetSize() const --> class wxSize");
		cl.def("SetSize", (void (wxRect::*)(const class wxSize &)) &wxRect::SetSize, "C++: wxRect::SetSize(const class wxSize &) --> void", pybind11::arg("s"));
		cl.def("IsEmpty", (bool (wxRect::*)() const) &wxRect::IsEmpty, "C++: wxRect::IsEmpty() const --> bool");
		cl.def("GetLeft", (int (wxRect::*)() const) &wxRect::GetLeft, "C++: wxRect::GetLeft() const --> int");
		cl.def("GetTop", (int (wxRect::*)() const) &wxRect::GetTop, "C++: wxRect::GetTop() const --> int");
		cl.def("GetBottom", (int (wxRect::*)() const) &wxRect::GetBottom, "C++: wxRect::GetBottom() const --> int");
		cl.def("GetRight", (int (wxRect::*)() const) &wxRect::GetRight, "C++: wxRect::GetRight() const --> int");
		cl.def("SetLeft", (void (wxRect::*)(int)) &wxRect::SetLeft, "C++: wxRect::SetLeft(int) --> void", pybind11::arg("left"));
		cl.def("SetRight", (void (wxRect::*)(int)) &wxRect::SetRight, "C++: wxRect::SetRight(int) --> void", pybind11::arg("right"));
		cl.def("SetTop", (void (wxRect::*)(int)) &wxRect::SetTop, "C++: wxRect::SetTop(int) --> void", pybind11::arg("top"));
		cl.def("SetBottom", (void (wxRect::*)(int)) &wxRect::SetBottom, "C++: wxRect::SetBottom(int) --> void", pybind11::arg("bottom"));
		cl.def("GetTopLeft", (class wxPoint (wxRect::*)() const) &wxRect::GetTopLeft, "C++: wxRect::GetTopLeft() const --> class wxPoint");
		cl.def("GetLeftTop", (class wxPoint (wxRect::*)() const) &wxRect::GetLeftTop, "C++: wxRect::GetLeftTop() const --> class wxPoint");
		cl.def("SetTopLeft", (void (wxRect::*)(const class wxPoint &)) &wxRect::SetTopLeft, "C++: wxRect::SetTopLeft(const class wxPoint &) --> void", pybind11::arg("p"));
		cl.def("SetLeftTop", (void (wxRect::*)(const class wxPoint &)) &wxRect::SetLeftTop, "C++: wxRect::SetLeftTop(const class wxPoint &) --> void", pybind11::arg("p"));
		cl.def("GetBottomRight", (class wxPoint (wxRect::*)() const) &wxRect::GetBottomRight, "C++: wxRect::GetBottomRight() const --> class wxPoint");
		cl.def("GetRightBottom", (class wxPoint (wxRect::*)() const) &wxRect::GetRightBottom, "C++: wxRect::GetRightBottom() const --> class wxPoint");
		cl.def("SetBottomRight", (void (wxRect::*)(const class wxPoint &)) &wxRect::SetBottomRight, "C++: wxRect::SetBottomRight(const class wxPoint &) --> void", pybind11::arg("p"));
		cl.def("SetRightBottom", (void (wxRect::*)(const class wxPoint &)) &wxRect::SetRightBottom, "C++: wxRect::SetRightBottom(const class wxPoint &) --> void", pybind11::arg("p"));
		cl.def("GetTopRight", (class wxPoint (wxRect::*)() const) &wxRect::GetTopRight, "C++: wxRect::GetTopRight() const --> class wxPoint");
		cl.def("GetRightTop", (class wxPoint (wxRect::*)() const) &wxRect::GetRightTop, "C++: wxRect::GetRightTop() const --> class wxPoint");
		cl.def("SetTopRight", (void (wxRect::*)(const class wxPoint &)) &wxRect::SetTopRight, "C++: wxRect::SetTopRight(const class wxPoint &) --> void", pybind11::arg("p"));
		cl.def("SetRightTop", (void (wxRect::*)(const class wxPoint &)) &wxRect::SetRightTop, "C++: wxRect::SetRightTop(const class wxPoint &) --> void", pybind11::arg("p"));
		cl.def("GetBottomLeft", (class wxPoint (wxRect::*)() const) &wxRect::GetBottomLeft, "C++: wxRect::GetBottomLeft() const --> class wxPoint");
		cl.def("GetLeftBottom", (class wxPoint (wxRect::*)() const) &wxRect::GetLeftBottom, "C++: wxRect::GetLeftBottom() const --> class wxPoint");
		cl.def("SetBottomLeft", (void (wxRect::*)(const class wxPoint &)) &wxRect::SetBottomLeft, "C++: wxRect::SetBottomLeft(const class wxPoint &) --> void", pybind11::arg("p"));
		cl.def("SetLeftBottom", (void (wxRect::*)(const class wxPoint &)) &wxRect::SetLeftBottom, "C++: wxRect::SetLeftBottom(const class wxPoint &) --> void", pybind11::arg("p"));
		cl.def("Inflate", (class wxRect & (wxRect::*)(int, int)) &wxRect::Inflate, "C++: wxRect::Inflate(int, int) --> class wxRect &", pybind11::return_value_policy::automatic, pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("Inflate", (class wxRect & (wxRect::*)(const class wxSize &)) &wxRect::Inflate, "C++: wxRect::Inflate(const class wxSize &) --> class wxRect &", pybind11::return_value_policy::automatic, pybind11::arg("d"));
		cl.def("Inflate", (class wxRect & (wxRect::*)(int)) &wxRect::Inflate, "C++: wxRect::Inflate(int) --> class wxRect &", pybind11::return_value_policy::automatic, pybind11::arg("d"));
		cl.def("Deflate", (class wxRect & (wxRect::*)(int, int)) &wxRect::Deflate, "C++: wxRect::Deflate(int, int) --> class wxRect &", pybind11::return_value_policy::automatic, pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("Deflate", (class wxRect & (wxRect::*)(const class wxSize &)) &wxRect::Deflate, "C++: wxRect::Deflate(const class wxSize &) --> class wxRect &", pybind11::return_value_policy::automatic, pybind11::arg("d"));
		cl.def("Deflate", (class wxRect & (wxRect::*)(int)) &wxRect::Deflate, "C++: wxRect::Deflate(int) --> class wxRect &", pybind11::return_value_policy::automatic, pybind11::arg("d"));
		cl.def("Offset", (void (wxRect::*)(int, int)) &wxRect::Offset, "C++: wxRect::Offset(int, int) --> void", pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("Offset", (void (wxRect::*)(const class wxPoint &)) &wxRect::Offset, "C++: wxRect::Offset(const class wxPoint &) --> void", pybind11::arg("pt"));
		cl.def("Intersect", (class wxRect & (wxRect::*)(const class wxRect &)) &wxRect::Intersect, "C++: wxRect::Intersect(const class wxRect &) --> class wxRect &", pybind11::return_value_policy::automatic, pybind11::arg("rect"));
		cl.def("Union", (class wxRect & (wxRect::*)(const class wxRect &)) &wxRect::Union, "C++: wxRect::Union(const class wxRect &) --> class wxRect &", pybind11::return_value_policy::automatic, pybind11::arg("rect"));
		cl.def("Contains", (bool (wxRect::*)(int, int) const) &wxRect::Contains, "C++: wxRect::Contains(int, int) const --> bool", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("Contains", (bool (wxRect::*)(const class wxPoint &) const) &wxRect::Contains, "C++: wxRect::Contains(const class wxPoint &) const --> bool", pybind11::arg("pt"));
		cl.def("Contains", (bool (wxRect::*)(const class wxRect &) const) &wxRect::Contains, "C++: wxRect::Contains(const class wxRect &) const --> bool", pybind11::arg("rect"));
		cl.def("Intersects", (bool (wxRect::*)(const class wxRect &) const) &wxRect::Intersects, "C++: wxRect::Intersects(const class wxRect &) const --> bool", pybind11::arg("rect"));
		cl.def("__iadd__", (class wxRect & (wxRect::*)(const class wxRect &)) &wxRect::operator+=, "C++: wxRect::operator+=(const class wxRect &) --> class wxRect &", pybind11::return_value_policy::automatic, pybind11::arg("rect"));
		cl.def("__imul__", (class wxRect & (wxRect::*)(const class wxRect &)) &wxRect::operator*=, "C++: wxRect::operator*=(const class wxRect &) --> class wxRect &", pybind11::return_value_policy::automatic, pybind11::arg("rect"));
		cl.def("CentreIn", [](wxRect const &o, const class wxRect & a0) -> wxRect { return o.CentreIn(a0); }, "", pybind11::arg("r"));
		cl.def("CentreIn", (class wxRect (wxRect::*)(const class wxRect &, int) const) &wxRect::CentreIn, "C++: wxRect::CentreIn(const class wxRect &, int) const --> class wxRect", pybind11::arg("r"), pybind11::arg("dir"));
		cl.def("CenterIn", [](wxRect const &o, const class wxRect & a0) -> wxRect { return o.CenterIn(a0); }, "", pybind11::arg("r"));
		cl.def("CenterIn", (class wxRect (wxRect::*)(const class wxRect &, int) const) &wxRect::CenterIn, "C++: wxRect::CenterIn(const class wxRect &, int) const --> class wxRect", pybind11::arg("r"), pybind11::arg("dir"));
		cl.def("assign", (class wxRect & (wxRect::*)(const class wxRect &)) &wxRect::operator=, "C++: wxRect::operator=(const class wxRect &) --> class wxRect &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxGDIObjListBase file: line:885
		pybind11::class_<wxGDIObjListBase, std::shared_ptr<wxGDIObjListBase>> cl(M(""), "wxGDIObjListBase", "");
		cl.def( pybind11::init( [](){ return new wxGDIObjListBase(); } ) );
		cl.def( pybind11::init( [](wxGDIObjListBase const &o){ return new wxGDIObjListBase(o); } ) );
		cl.def("assign", (class wxGDIObjListBase & (wxGDIObjListBase::*)(const class wxGDIObjListBase &)) &wxGDIObjListBase::operator=, "C++: wxGDIObjListBase::operator=(const class wxGDIObjListBase &) --> class wxGDIObjListBase &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxStringToColourHashMap_wxImplementation_Pair file: line:76
		pybind11::class_<wxStringToColourHashMap_wxImplementation_Pair, std::shared_ptr<wxStringToColourHashMap_wxImplementation_Pair>> cl(M(""), "wxStringToColourHashMap_wxImplementation_Pair", "");
		cl.def( pybind11::init<const class wxString &, const class wxColour *const &>(), pybind11::arg("f"), pybind11::arg("s") );

		cl.def( pybind11::init( [](wxStringToColourHashMap_wxImplementation_Pair const &o){ return new wxStringToColourHashMap_wxImplementation_Pair(o); } ) );
		cl.def_readwrite("first", &wxStringToColourHashMap_wxImplementation_Pair::first);
		cl.def("assign", (class wxStringToColourHashMap_wxImplementation_Pair & (wxStringToColourHashMap_wxImplementation_Pair::*)(const class wxStringToColourHashMap_wxImplementation_Pair &)) &wxStringToColourHashMap_wxImplementation_Pair::operator=, "C++: wxStringToColourHashMap_wxImplementation_Pair::operator=(const class wxStringToColourHashMap_wxImplementation_Pair &) --> class wxStringToColourHashMap_wxImplementation_Pair &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxStringToColourHashMap_wxImplementation_KeyEx file: line:78
		pybind11::class_<wxStringToColourHashMap_wxImplementation_KeyEx, std::shared_ptr<wxStringToColourHashMap_wxImplementation_KeyEx>> cl(M(""), "wxStringToColourHashMap_wxImplementation_KeyEx", "");
		cl.def( pybind11::init( [](){ return new wxStringToColourHashMap_wxImplementation_KeyEx(); } ) );
		cl.def( pybind11::init( [](wxStringToColourHashMap_wxImplementation_KeyEx const &o){ return new wxStringToColourHashMap_wxImplementation_KeyEx(o); } ) );
		cl.def("__call__", (const class wxString & (wxStringToColourHashMap_wxImplementation_KeyEx::*)(const class wxStringToColourHashMap_wxImplementation_Pair &) const) &wxStringToColourHashMap_wxImplementation_KeyEx::operator(), "C++: wxStringToColourHashMap_wxImplementation_KeyEx::operator()(const class wxStringToColourHashMap_wxImplementation_Pair &) const --> const class wxString &", pybind11::return_value_policy::automatic, pybind11::arg("pair"));
		cl.def("assign", (class wxStringToColourHashMap_wxImplementation_KeyEx & (wxStringToColourHashMap_wxImplementation_KeyEx::*)(const class wxStringToColourHashMap_wxImplementation_KeyEx &)) &wxStringToColourHashMap_wxImplementation_KeyEx::operator=, "C++: wxStringToColourHashMap_wxImplementation_KeyEx::operator=(const class wxStringToColourHashMap_wxImplementation_KeyEx &) --> class wxStringToColourHashMap_wxImplementation_KeyEx &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxStringToColourHashMap_wxImplementation_HashTable file: line:81
		pybind11::class_<wxStringToColourHashMap_wxImplementation_HashTable, std::shared_ptr<wxStringToColourHashMap_wxImplementation_HashTable>> cl(M(""), "wxStringToColourHashMap_wxImplementation_HashTable", "");
		cl.def( pybind11::init( [](){ return new wxStringToColourHashMap_wxImplementation_HashTable(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxStringToColourHashMap_wxImplementation_HashTable(a0); } ), "doc" , pybind11::arg("sz"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxStringHash & a1){ return new wxStringToColourHashMap_wxImplementation_HashTable(a0, a1); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxStringHash & a1, const struct wxStringEqual & a2){ return new wxStringToColourHashMap_wxImplementation_HashTable(a0, a1, a2); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"));
		cl.def( pybind11::init<unsigned long, const struct wxStringHash &, const struct wxStringEqual &, const class wxStringToColourHashMap_wxImplementation_KeyEx &>(), pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"), pybind11::arg("k_ex") );

		cl.def( pybind11::init( [](wxStringToColourHashMap_wxImplementation_HashTable const &o){ return new wxStringToColourHashMap_wxImplementation_HashTable(o); } ) );
		cl.def("assign", (const class wxStringToColourHashMap_wxImplementation_HashTable & (wxStringToColourHashMap_wxImplementation_HashTable::*)(const class wxStringToColourHashMap_wxImplementation_HashTable &)) &wxStringToColourHashMap_wxImplementation_HashTable::operator=, "C++: wxStringToColourHashMap_wxImplementation_HashTable::operator=(const class wxStringToColourHashMap_wxImplementation_HashTable &) --> const class wxStringToColourHashMap_wxImplementation_HashTable &", pybind11::return_value_policy::automatic, pybind11::arg("ht"));
		cl.def("hash_funct", (struct wxStringHash (wxStringToColourHashMap_wxImplementation_HashTable::*)()) &wxStringToColourHashMap_wxImplementation_HashTable::hash_funct, "C++: wxStringToColourHashMap_wxImplementation_HashTable::hash_funct() --> struct wxStringHash");
		cl.def("key_eq", (struct wxStringEqual (wxStringToColourHashMap_wxImplementation_HashTable::*)()) &wxStringToColourHashMap_wxImplementation_HashTable::key_eq, "C++: wxStringToColourHashMap_wxImplementation_HashTable::key_eq() --> struct wxStringEqual");
		cl.def("clear", (void (wxStringToColourHashMap_wxImplementation_HashTable::*)()) &wxStringToColourHashMap_wxImplementation_HashTable::clear, "C++: wxStringToColourHashMap_wxImplementation_HashTable::clear() --> void");
		cl.def("size", (unsigned long (wxStringToColourHashMap_wxImplementation_HashTable::*)() const) &wxStringToColourHashMap_wxImplementation_HashTable::size, "C++: wxStringToColourHashMap_wxImplementation_HashTable::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxStringToColourHashMap_wxImplementation_HashTable::*)() const) &wxStringToColourHashMap_wxImplementation_HashTable::max_size, "C++: wxStringToColourHashMap_wxImplementation_HashTable::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxStringToColourHashMap_wxImplementation_HashTable::*)() const) &wxStringToColourHashMap_wxImplementation_HashTable::empty, "C++: wxStringToColourHashMap_wxImplementation_HashTable::empty() const --> bool");
		cl.def("end", (class wxStringToColourHashMap_wxImplementation_HashTable::iterator (wxStringToColourHashMap_wxImplementation_HashTable::*)()) &wxStringToColourHashMap_wxImplementation_HashTable::end, "C++: wxStringToColourHashMap_wxImplementation_HashTable::end() --> class wxStringToColourHashMap_wxImplementation_HashTable::iterator");
		cl.def("begin", (class wxStringToColourHashMap_wxImplementation_HashTable::iterator (wxStringToColourHashMap_wxImplementation_HashTable::*)()) &wxStringToColourHashMap_wxImplementation_HashTable::begin, "C++: wxStringToColourHashMap_wxImplementation_HashTable::begin() --> class wxStringToColourHashMap_wxImplementation_HashTable::iterator");
		cl.def("erase", (unsigned long (wxStringToColourHashMap_wxImplementation_HashTable::*)(const class wxString &)) &wxStringToColourHashMap_wxImplementation_HashTable::erase, "C++: wxStringToColourHashMap_wxImplementation_HashTable::erase(const class wxString &) --> unsigned long", pybind11::arg("key"));

		{ // wxStringToColourHashMap_wxImplementation_HashTable::Node file: line:160
			auto & enclosing_class = cl;
			pybind11::class_<wxStringToColourHashMap_wxImplementation_HashTable::Node, std::shared_ptr<wxStringToColourHashMap_wxImplementation_HashTable::Node>, _wxHashTable_NodeBase> cl(enclosing_class, "Node", "");
			cl.def( pybind11::init<const class wxStringToColourHashMap_wxImplementation_Pair &>(), pybind11::arg("value") );

			cl.def( pybind11::init( [](wxStringToColourHashMap_wxImplementation_HashTable::Node const &o){ return new wxStringToColourHashMap_wxImplementation_HashTable::Node(o); } ) );
			cl.def_readwrite("m_value", &wxStringToColourHashMap_wxImplementation_HashTable::Node::m_value);
			cl.def("next", (struct wxStringToColourHashMap_wxImplementation_HashTable::Node * (wxStringToColourHashMap_wxImplementation_HashTable::Node::*)()) &wxStringToColourHashMap_wxImplementation_HashTable::Node::next, "C++: wxStringToColourHashMap_wxImplementation_HashTable::Node::next() --> struct wxStringToColourHashMap_wxImplementation_HashTable::Node *", pybind11::return_value_policy::automatic);
		}

		{ // wxStringToColourHashMap_wxImplementation_HashTable::Iterator file: line:179
			auto & enclosing_class = cl;
			pybind11::class_<wxStringToColourHashMap_wxImplementation_HashTable::Iterator, std::shared_ptr<wxStringToColourHashMap_wxImplementation_HashTable::Iterator>> cl(enclosing_class, "Iterator", "");
			cl.def( pybind11::init( [](){ return new wxStringToColourHashMap_wxImplementation_HashTable::Iterator(); } ) );
			cl.def( pybind11::init<struct wxStringToColourHashMap_wxImplementation_HashTable::Node *, const class wxStringToColourHashMap_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxStringToColourHashMap_wxImplementation_HashTable::Iterator const &o){ return new wxStringToColourHashMap_wxImplementation_HashTable::Iterator(o); } ) );
			cl.def("__eq__", (bool (wxStringToColourHashMap_wxImplementation_HashTable::Iterator::*)(const class wxStringToColourHashMap_wxImplementation_HashTable::Iterator &) const) &wxStringToColourHashMap_wxImplementation_HashTable::Iterator::operator==, "C++: wxStringToColourHashMap_wxImplementation_HashTable::Iterator::operator==(const class wxStringToColourHashMap_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxStringToColourHashMap_wxImplementation_HashTable::Iterator::*)(const class wxStringToColourHashMap_wxImplementation_HashTable::Iterator &) const) &wxStringToColourHashMap_wxImplementation_HashTable::Iterator::operator!=, "C++: wxStringToColourHashMap_wxImplementation_HashTable::Iterator::operator!=(const class wxStringToColourHashMap_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxStringToColourHashMap_wxImplementation_HashTable::iterator file: line:213
			auto & enclosing_class = cl;
			pybind11::class_<wxStringToColourHashMap_wxImplementation_HashTable::iterator, std::shared_ptr<wxStringToColourHashMap_wxImplementation_HashTable::iterator>, wxStringToColourHashMap_wxImplementation_HashTable::Iterator> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init( [](){ return new wxStringToColourHashMap_wxImplementation_HashTable::iterator(); } ) );
			cl.def( pybind11::init<struct wxStringToColourHashMap_wxImplementation_HashTable::Node *, class wxStringToColourHashMap_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxStringToColourHashMap_wxImplementation_HashTable::iterator const &o){ return new wxStringToColourHashMap_wxImplementation_HashTable::iterator(o); } ) );
			cl.def("plus_plus", (class wxStringToColourHashMap_wxImplementation_HashTable::iterator & (wxStringToColourHashMap_wxImplementation_HashTable::iterator::*)()) &wxStringToColourHashMap_wxImplementation_HashTable::iterator::operator++, "C++: wxStringToColourHashMap_wxImplementation_HashTable::iterator::operator++() --> class wxStringToColourHashMap_wxImplementation_HashTable::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxStringToColourHashMap_wxImplementation_HashTable::iterator (wxStringToColourHashMap_wxImplementation_HashTable::iterator::*)(int)) &wxStringToColourHashMap_wxImplementation_HashTable::iterator::operator++, "C++: wxStringToColourHashMap_wxImplementation_HashTable::iterator::operator++(int) --> class wxStringToColourHashMap_wxImplementation_HashTable::iterator", pybind11::arg(""));
			cl.def("__mul__", (class wxStringToColourHashMap_wxImplementation_Pair & (wxStringToColourHashMap_wxImplementation_HashTable::iterator::*)() const) &wxStringToColourHashMap_wxImplementation_HashTable::iterator::operator*, "C++: wxStringToColourHashMap_wxImplementation_HashTable::iterator::operator*() const --> class wxStringToColourHashMap_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

		{ // wxStringToColourHashMap_wxImplementation_HashTable::const_iterator file: line:224
			auto & enclosing_class = cl;
			pybind11::class_<wxStringToColourHashMap_wxImplementation_HashTable::const_iterator, std::shared_ptr<wxStringToColourHashMap_wxImplementation_HashTable::const_iterator>, wxStringToColourHashMap_wxImplementation_HashTable::Iterator> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init( [](){ return new wxStringToColourHashMap_wxImplementation_HashTable::const_iterator(); } ) );
			cl.def( pybind11::init<class wxStringToColourHashMap_wxImplementation_HashTable::iterator>(), pybind11::arg("i") );

			cl.def( pybind11::init<struct wxStringToColourHashMap_wxImplementation_HashTable::Node *, const class wxStringToColourHashMap_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxStringToColourHashMap_wxImplementation_HashTable::const_iterator const &o){ return new wxStringToColourHashMap_wxImplementation_HashTable::const_iterator(o); } ) );
			cl.def("plus_plus", (class wxStringToColourHashMap_wxImplementation_HashTable::const_iterator & (wxStringToColourHashMap_wxImplementation_HashTable::const_iterator::*)()) &wxStringToColourHashMap_wxImplementation_HashTable::const_iterator::operator++, "C++: wxStringToColourHashMap_wxImplementation_HashTable::const_iterator::operator++() --> class wxStringToColourHashMap_wxImplementation_HashTable::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxStringToColourHashMap_wxImplementation_HashTable::const_iterator (wxStringToColourHashMap_wxImplementation_HashTable::const_iterator::*)(int)) &wxStringToColourHashMap_wxImplementation_HashTable::const_iterator::operator++, "C++: wxStringToColourHashMap_wxImplementation_HashTable::const_iterator::operator++(int) --> class wxStringToColourHashMap_wxImplementation_HashTable::const_iterator", pybind11::arg(""));
			cl.def("__mul__", (const class wxStringToColourHashMap_wxImplementation_Pair & (wxStringToColourHashMap_wxImplementation_HashTable::const_iterator::*)() const) &wxStringToColourHashMap_wxImplementation_HashTable::const_iterator::operator*, "C++: wxStringToColourHashMap_wxImplementation_HashTable::const_iterator::operator*() const --> const class wxStringToColourHashMap_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

	}
}

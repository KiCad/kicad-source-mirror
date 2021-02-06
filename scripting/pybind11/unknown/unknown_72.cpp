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

// wxImageHandler file: line:104
struct PyCallBack_wxImageHandler : public wxImageHandler {
	using wxImageHandler::wxImageHandler;

	bool LoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImageHandler *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxImageHandler::LoadFile(a0, a1, a2, a3);
	}
	bool SaveFile(class wxImage * a0, class wxOutputStream & a1, bool a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImageHandler *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxImageHandler::SaveFile(a0, a1, a2);
	}
	int DoGetImageCount(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImageHandler *>(this), "DoGetImageCount");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxImageHandler::DoGetImageCount(a0);
	}
	bool DoCanRead(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImageHandler *>(this), "DoCanRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxImageHandler::DoCanRead\"");
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImageHandler *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxImageHandler::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImageHandler *>(this), "CreateRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxObject::CreateRefData();
	}
	class wxRefCounter * CloneRefData(const class wxRefCounter * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImageHandler *>(this), "CloneRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxObject::CloneRefData(a0);
	}
};

void bind_unknown_unknown_72(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxImageHandler file: line:104
		pybind11::class_<wxImageHandler, std::shared_ptr<wxImageHandler>, PyCallBack_wxImageHandler, wxObject> cl(M(""), "wxImageHandler", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxImageHandler(); } ) );
		cl.def(pybind11::init<PyCallBack_wxImageHandler const &>());
		cl.def("LoadFile", [](wxImageHandler &o, class wxImage * a0, class wxInputStream & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg(""), pybind11::arg(""));
		cl.def("LoadFile", [](wxImageHandler &o, class wxImage * a0, class wxInputStream & a1, bool const & a2) -> bool { return o.LoadFile(a0, a1, a2); }, "", pybind11::arg(""), pybind11::arg(""), pybind11::arg(""));
		cl.def("LoadFile", (bool (wxImageHandler::*)(class wxImage *, class wxInputStream &, bool, int)) &wxImageHandler::LoadFile, "C++: wxImageHandler::LoadFile(class wxImage *, class wxInputStream &, bool, int) --> bool", pybind11::arg(""), pybind11::arg(""), pybind11::arg(""), pybind11::arg(""));
		cl.def("SaveFile", [](wxImageHandler &o, class wxImage * a0, class wxOutputStream & a1) -> bool { return o.SaveFile(a0, a1); }, "", pybind11::arg(""), pybind11::arg(""));
		cl.def("SaveFile", (bool (wxImageHandler::*)(class wxImage *, class wxOutputStream &, bool)) &wxImageHandler::SaveFile, "C++: wxImageHandler::SaveFile(class wxImage *, class wxOutputStream &, bool) --> bool", pybind11::arg(""), pybind11::arg(""), pybind11::arg(""));
		cl.def("GetImageCount", (int (wxImageHandler::*)(class wxInputStream &)) &wxImageHandler::GetImageCount, "C++: wxImageHandler::GetImageCount(class wxInputStream &) --> int", pybind11::arg("stream"));
		cl.def("CanRead", (bool (wxImageHandler::*)(class wxInputStream &)) &wxImageHandler::CanRead, "C++: wxImageHandler::CanRead(class wxInputStream &) --> bool", pybind11::arg("stream"));
		cl.def("CanRead", (bool (wxImageHandler::*)(const class wxString &)) &wxImageHandler::CanRead, "C++: wxImageHandler::CanRead(const class wxString &) --> bool", pybind11::arg("name"));
		cl.def("SetName", (void (wxImageHandler::*)(const class wxString &)) &wxImageHandler::SetName, "C++: wxImageHandler::SetName(const class wxString &) --> void", pybind11::arg("name"));
		cl.def("SetExtension", (void (wxImageHandler::*)(const class wxString &)) &wxImageHandler::SetExtension, "C++: wxImageHandler::SetExtension(const class wxString &) --> void", pybind11::arg("ext"));
		cl.def("SetAltExtensions", (void (wxImageHandler::*)(const class wxArrayString &)) &wxImageHandler::SetAltExtensions, "C++: wxImageHandler::SetAltExtensions(const class wxArrayString &) --> void", pybind11::arg("exts"));
		cl.def("SetType", (void (wxImageHandler::*)(enum wxBitmapType)) &wxImageHandler::SetType, "C++: wxImageHandler::SetType(enum wxBitmapType) --> void", pybind11::arg("type"));
		cl.def("SetMimeType", (void (wxImageHandler::*)(const class wxString &)) &wxImageHandler::SetMimeType, "C++: wxImageHandler::SetMimeType(const class wxString &) --> void", pybind11::arg("type"));
		cl.def("GetName", (const class wxString & (wxImageHandler::*)() const) &wxImageHandler::GetName, "C++: wxImageHandler::GetName() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetExtension", (const class wxString & (wxImageHandler::*)() const) &wxImageHandler::GetExtension, "C++: wxImageHandler::GetExtension() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetAltExtensions", (const class wxArrayString & (wxImageHandler::*)() const) &wxImageHandler::GetAltExtensions, "C++: wxImageHandler::GetAltExtensions() const --> const class wxArrayString &", pybind11::return_value_policy::automatic);
		cl.def("GetType", (enum wxBitmapType (wxImageHandler::*)() const) &wxImageHandler::GetType, "C++: wxImageHandler::GetType() const --> enum wxBitmapType");
		cl.def("GetMimeType", (const class wxString & (wxImageHandler::*)() const) &wxImageHandler::GetMimeType, "C++: wxImageHandler::GetMimeType() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("SetType", (void (wxImageHandler::*)(long)) &wxImageHandler::SetType, "C++: wxImageHandler::SetType(long) --> void", pybind11::arg("type"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxImageHandler::*)() const) &wxImageHandler::GetClassInfo, "C++: wxImageHandler::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxImageHandler & (wxImageHandler::*)(const class wxImageHandler &)) &wxImageHandler::operator=, "C++: wxImageHandler::operator=(const class wxImageHandler &) --> class wxImageHandler &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxImageHistogramEntry file: line:181
		pybind11::class_<wxImageHistogramEntry, std::shared_ptr<wxImageHistogramEntry>> cl(M(""), "wxImageHistogramEntry", "");
		cl.def( pybind11::init( [](){ return new wxImageHistogramEntry(); } ) );
		cl.def( pybind11::init( [](wxImageHistogramEntry const &o){ return new wxImageHistogramEntry(o); } ) );
		cl.def_readwrite("index", &wxImageHistogramEntry::index);
		cl.def_readwrite("value", &wxImageHistogramEntry::value);
	}
	{ // wxImageHistogramBase_wxImplementation_Pair file: line:166
		pybind11::class_<wxImageHistogramBase_wxImplementation_Pair, std::shared_ptr<wxImageHistogramBase_wxImplementation_Pair>> cl(M(""), "wxImageHistogramBase_wxImplementation_Pair", "");
		cl.def( pybind11::init<const unsigned long &, const class wxImageHistogramEntry &>(), pybind11::arg("f"), pybind11::arg("s") );

		cl.def( pybind11::init( [](wxImageHistogramBase_wxImplementation_Pair const &o){ return new wxImageHistogramBase_wxImplementation_Pair(o); } ) );
		cl.def_readwrite("first", &wxImageHistogramBase_wxImplementation_Pair::first);
		cl.def_readwrite("second", &wxImageHistogramBase_wxImplementation_Pair::second);
	}
	{ // wxImageHistogramBase_wxImplementation_KeyEx file: line:168
		pybind11::class_<wxImageHistogramBase_wxImplementation_KeyEx, std::shared_ptr<wxImageHistogramBase_wxImplementation_KeyEx>> cl(M(""), "wxImageHistogramBase_wxImplementation_KeyEx", "");
		cl.def( pybind11::init( [](){ return new wxImageHistogramBase_wxImplementation_KeyEx(); } ) );
		cl.def( pybind11::init( [](wxImageHistogramBase_wxImplementation_KeyEx const &o){ return new wxImageHistogramBase_wxImplementation_KeyEx(o); } ) );
		cl.def("__call__", (const unsigned long & (wxImageHistogramBase_wxImplementation_KeyEx::*)(const class wxImageHistogramBase_wxImplementation_Pair &) const) &wxImageHistogramBase_wxImplementation_KeyEx::operator(), "C++: wxImageHistogramBase_wxImplementation_KeyEx::operator()(const class wxImageHistogramBase_wxImplementation_Pair &) const --> const unsigned long &", pybind11::return_value_policy::automatic, pybind11::arg("pair"));
		cl.def("assign", (class wxImageHistogramBase_wxImplementation_KeyEx & (wxImageHistogramBase_wxImplementation_KeyEx::*)(const class wxImageHistogramBase_wxImplementation_KeyEx &)) &wxImageHistogramBase_wxImplementation_KeyEx::operator=, "C++: wxImageHistogramBase_wxImplementation_KeyEx::operator=(const class wxImageHistogramBase_wxImplementation_KeyEx &) --> class wxImageHistogramBase_wxImplementation_KeyEx &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxImageHistogramBase_wxImplementation_HashTable file: line:2
		pybind11::class_<wxImageHistogramBase_wxImplementation_HashTable, std::shared_ptr<wxImageHistogramBase_wxImplementation_HashTable>> cl(M(""), "wxImageHistogramBase_wxImplementation_HashTable", "");
		cl.def( pybind11::init( [](){ return new wxImageHistogramBase_wxImplementation_HashTable(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxImageHistogramBase_wxImplementation_HashTable(a0); } ), "doc" , pybind11::arg("sz"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxIntegerHash & a1){ return new wxImageHistogramBase_wxImplementation_HashTable(a0, a1); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"));
		cl.def( pybind11::init( [](unsigned long const & a0, const struct wxIntegerHash & a1, const struct wxIntegerEqual & a2){ return new wxImageHistogramBase_wxImplementation_HashTable(a0, a1, a2); } ), "doc" , pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"));
		cl.def( pybind11::init<unsigned long, const struct wxIntegerHash &, const struct wxIntegerEqual &, const class wxImageHistogramBase_wxImplementation_KeyEx &>(), pybind11::arg("sz"), pybind11::arg("hfun"), pybind11::arg("k_eq"), pybind11::arg("k_ex") );

		cl.def( pybind11::init( [](wxImageHistogramBase_wxImplementation_HashTable const &o){ return new wxImageHistogramBase_wxImplementation_HashTable(o); } ) );
		cl.def("assign", (const class wxImageHistogramBase_wxImplementation_HashTable & (wxImageHistogramBase_wxImplementation_HashTable::*)(const class wxImageHistogramBase_wxImplementation_HashTable &)) &wxImageHistogramBase_wxImplementation_HashTable::operator=, "C++: wxImageHistogramBase_wxImplementation_HashTable::operator=(const class wxImageHistogramBase_wxImplementation_HashTable &) --> const class wxImageHistogramBase_wxImplementation_HashTable &", pybind11::return_value_policy::automatic, pybind11::arg("ht"));
		cl.def("hash_funct", (struct wxIntegerHash (wxImageHistogramBase_wxImplementation_HashTable::*)()) &wxImageHistogramBase_wxImplementation_HashTable::hash_funct, "C++: wxImageHistogramBase_wxImplementation_HashTable::hash_funct() --> struct wxIntegerHash");
		cl.def("key_eq", (struct wxIntegerEqual (wxImageHistogramBase_wxImplementation_HashTable::*)()) &wxImageHistogramBase_wxImplementation_HashTable::key_eq, "C++: wxImageHistogramBase_wxImplementation_HashTable::key_eq() --> struct wxIntegerEqual");
		cl.def("clear", (void (wxImageHistogramBase_wxImplementation_HashTable::*)()) &wxImageHistogramBase_wxImplementation_HashTable::clear, "C++: wxImageHistogramBase_wxImplementation_HashTable::clear() --> void");
		cl.def("size", (unsigned long (wxImageHistogramBase_wxImplementation_HashTable::*)() const) &wxImageHistogramBase_wxImplementation_HashTable::size, "C++: wxImageHistogramBase_wxImplementation_HashTable::size() const --> unsigned long");
		cl.def("max_size", (unsigned long (wxImageHistogramBase_wxImplementation_HashTable::*)() const) &wxImageHistogramBase_wxImplementation_HashTable::max_size, "C++: wxImageHistogramBase_wxImplementation_HashTable::max_size() const --> unsigned long");
		cl.def("empty", (bool (wxImageHistogramBase_wxImplementation_HashTable::*)() const) &wxImageHistogramBase_wxImplementation_HashTable::empty, "C++: wxImageHistogramBase_wxImplementation_HashTable::empty() const --> bool");
		cl.def("end", (class wxImageHistogramBase_wxImplementation_HashTable::iterator (wxImageHistogramBase_wxImplementation_HashTable::*)()) &wxImageHistogramBase_wxImplementation_HashTable::end, "C++: wxImageHistogramBase_wxImplementation_HashTable::end() --> class wxImageHistogramBase_wxImplementation_HashTable::iterator");
		cl.def("begin", (class wxImageHistogramBase_wxImplementation_HashTable::iterator (wxImageHistogramBase_wxImplementation_HashTable::*)()) &wxImageHistogramBase_wxImplementation_HashTable::begin, "C++: wxImageHistogramBase_wxImplementation_HashTable::begin() --> class wxImageHistogramBase_wxImplementation_HashTable::iterator");
		cl.def("erase", (unsigned long (wxImageHistogramBase_wxImplementation_HashTable::*)(const unsigned long &)) &wxImageHistogramBase_wxImplementation_HashTable::erase, "C++: wxImageHistogramBase_wxImplementation_HashTable::erase(const unsigned long &) --> unsigned long", pybind11::arg("key"));

		{ // wxImageHistogramBase_wxImplementation_HashTable::Node file: line:160
			auto & enclosing_class = cl;
			pybind11::class_<wxImageHistogramBase_wxImplementation_HashTable::Node, std::shared_ptr<wxImageHistogramBase_wxImplementation_HashTable::Node>, _wxHashTable_NodeBase> cl(enclosing_class, "Node", "");
			cl.def( pybind11::init<const class wxImageHistogramBase_wxImplementation_Pair &>(), pybind11::arg("value") );

			cl.def( pybind11::init( [](wxImageHistogramBase_wxImplementation_HashTable::Node const &o){ return new wxImageHistogramBase_wxImplementation_HashTable::Node(o); } ) );
			cl.def_readwrite("m_value", &wxImageHistogramBase_wxImplementation_HashTable::Node::m_value);
			cl.def("next", (struct wxImageHistogramBase_wxImplementation_HashTable::Node * (wxImageHistogramBase_wxImplementation_HashTable::Node::*)()) &wxImageHistogramBase_wxImplementation_HashTable::Node::next, "C++: wxImageHistogramBase_wxImplementation_HashTable::Node::next() --> struct wxImageHistogramBase_wxImplementation_HashTable::Node *", pybind11::return_value_policy::automatic);
		}

		{ // wxImageHistogramBase_wxImplementation_HashTable::Iterator file: line:179
			auto & enclosing_class = cl;
			pybind11::class_<wxImageHistogramBase_wxImplementation_HashTable::Iterator, std::shared_ptr<wxImageHistogramBase_wxImplementation_HashTable::Iterator>> cl(enclosing_class, "Iterator", "");
			cl.def( pybind11::init( [](){ return new wxImageHistogramBase_wxImplementation_HashTable::Iterator(); } ) );
			cl.def( pybind11::init<struct wxImageHistogramBase_wxImplementation_HashTable::Node *, const class wxImageHistogramBase_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxImageHistogramBase_wxImplementation_HashTable::Iterator const &o){ return new wxImageHistogramBase_wxImplementation_HashTable::Iterator(o); } ) );
			cl.def("__eq__", (bool (wxImageHistogramBase_wxImplementation_HashTable::Iterator::*)(const class wxImageHistogramBase_wxImplementation_HashTable::Iterator &) const) &wxImageHistogramBase_wxImplementation_HashTable::Iterator::operator==, "C++: wxImageHistogramBase_wxImplementation_HashTable::Iterator::operator==(const class wxImageHistogramBase_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxImageHistogramBase_wxImplementation_HashTable::Iterator::*)(const class wxImageHistogramBase_wxImplementation_HashTable::Iterator &) const) &wxImageHistogramBase_wxImplementation_HashTable::Iterator::operator!=, "C++: wxImageHistogramBase_wxImplementation_HashTable::Iterator::operator!=(const class wxImageHistogramBase_wxImplementation_HashTable::Iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxImageHistogramBase_wxImplementation_HashTable::iterator file: line:213
			auto & enclosing_class = cl;
			pybind11::class_<wxImageHistogramBase_wxImplementation_HashTable::iterator, std::shared_ptr<wxImageHistogramBase_wxImplementation_HashTable::iterator>, wxImageHistogramBase_wxImplementation_HashTable::Iterator> cl(enclosing_class, "iterator", "");
			cl.def( pybind11::init( [](){ return new wxImageHistogramBase_wxImplementation_HashTable::iterator(); } ) );
			cl.def( pybind11::init<struct wxImageHistogramBase_wxImplementation_HashTable::Node *, class wxImageHistogramBase_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxImageHistogramBase_wxImplementation_HashTable::iterator const &o){ return new wxImageHistogramBase_wxImplementation_HashTable::iterator(o); } ) );
			cl.def("plus_plus", (class wxImageHistogramBase_wxImplementation_HashTable::iterator & (wxImageHistogramBase_wxImplementation_HashTable::iterator::*)()) &wxImageHistogramBase_wxImplementation_HashTable::iterator::operator++, "C++: wxImageHistogramBase_wxImplementation_HashTable::iterator::operator++() --> class wxImageHistogramBase_wxImplementation_HashTable::iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxImageHistogramBase_wxImplementation_HashTable::iterator (wxImageHistogramBase_wxImplementation_HashTable::iterator::*)(int)) &wxImageHistogramBase_wxImplementation_HashTable::iterator::operator++, "C++: wxImageHistogramBase_wxImplementation_HashTable::iterator::operator++(int) --> class wxImageHistogramBase_wxImplementation_HashTable::iterator", pybind11::arg(""));
			cl.def("__mul__", (class wxImageHistogramBase_wxImplementation_Pair & (wxImageHistogramBase_wxImplementation_HashTable::iterator::*)() const) &wxImageHistogramBase_wxImplementation_HashTable::iterator::operator*, "C++: wxImageHistogramBase_wxImplementation_HashTable::iterator::operator*() const --> class wxImageHistogramBase_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

		{ // wxImageHistogramBase_wxImplementation_HashTable::const_iterator file: line:224
			auto & enclosing_class = cl;
			pybind11::class_<wxImageHistogramBase_wxImplementation_HashTable::const_iterator, std::shared_ptr<wxImageHistogramBase_wxImplementation_HashTable::const_iterator>, wxImageHistogramBase_wxImplementation_HashTable::Iterator> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init( [](){ return new wxImageHistogramBase_wxImplementation_HashTable::const_iterator(); } ) );
			cl.def( pybind11::init<class wxImageHistogramBase_wxImplementation_HashTable::iterator>(), pybind11::arg("i") );

			cl.def( pybind11::init<struct wxImageHistogramBase_wxImplementation_HashTable::Node *, const class wxImageHistogramBase_wxImplementation_HashTable *>(), pybind11::arg("node"), pybind11::arg("ht") );

			cl.def( pybind11::init( [](wxImageHistogramBase_wxImplementation_HashTable::const_iterator const &o){ return new wxImageHistogramBase_wxImplementation_HashTable::const_iterator(o); } ) );
			cl.def("plus_plus", (class wxImageHistogramBase_wxImplementation_HashTable::const_iterator & (wxImageHistogramBase_wxImplementation_HashTable::const_iterator::*)()) &wxImageHistogramBase_wxImplementation_HashTable::const_iterator::operator++, "C++: wxImageHistogramBase_wxImplementation_HashTable::const_iterator::operator++() --> class wxImageHistogramBase_wxImplementation_HashTable::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxImageHistogramBase_wxImplementation_HashTable::const_iterator (wxImageHistogramBase_wxImplementation_HashTable::const_iterator::*)(int)) &wxImageHistogramBase_wxImplementation_HashTable::const_iterator::operator++, "C++: wxImageHistogramBase_wxImplementation_HashTable::const_iterator::operator++(int) --> class wxImageHistogramBase_wxImplementation_HashTable::const_iterator", pybind11::arg(""));
			cl.def("__mul__", (const class wxImageHistogramBase_wxImplementation_Pair & (wxImageHistogramBase_wxImplementation_HashTable::const_iterator::*)() const) &wxImageHistogramBase_wxImplementation_HashTable::const_iterator::operator*, "C++: wxImageHistogramBase_wxImplementation_HashTable::const_iterator::operator*() const --> const class wxImageHistogramBase_wxImplementation_Pair &", pybind11::return_value_policy::automatic);
		}

	}
}

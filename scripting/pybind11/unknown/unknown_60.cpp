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

// wxClipboardTextEvent file: line:3095
struct PyCallBack_wxClipboardTextEvent : public wxClipboardTextEvent {
	using wxClipboardTextEvent::wxClipboardTextEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxClipboardTextEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxClipboardTextEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxClipboardTextEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxClipboardTextEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxClipboardTextEvent *>(this), "GetEventCategory");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxEventCategory>::value) {
				static pybind11::detail::override_caster_t<enum wxEventCategory> caster;
				return pybind11::detail::cast_ref<enum wxEventCategory>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxEventCategory>(std::move(o));
		}
		return wxCommandEvent::GetEventCategory();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxClipboardTextEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxClipboardTextEvent *>(this), "CloneRefData");
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

// wxContextMenuEvent file: line:3120
struct PyCallBack_wxContextMenuEvent : public wxContextMenuEvent {
	using wxContextMenuEvent::wxContextMenuEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxContextMenuEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxContextMenuEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxContextMenuEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxContextMenuEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxContextMenuEvent *>(this), "GetEventCategory");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxEventCategory>::value) {
				static pybind11::detail::override_caster_t<enum wxEventCategory> caster;
				return pybind11::detail::cast_ref<enum wxEventCategory>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxEventCategory>(std::move(o));
		}
		return wxCommandEvent::GetEventCategory();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxContextMenuEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxContextMenuEvent *>(this), "CloneRefData");
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

void bind_unknown_unknown_60(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxClipboardTextEvent file: line:3095
		pybind11::class_<wxClipboardTextEvent, std::shared_ptr<wxClipboardTextEvent>, PyCallBack_wxClipboardTextEvent, wxCommandEvent> cl(M(""), "wxClipboardTextEvent", "");
		cl.def( pybind11::init( [](){ return new wxClipboardTextEvent(); }, [](){ return new PyCallBack_wxClipboardTextEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxClipboardTextEvent(a0); }, [](int const & a0){ return new PyCallBack_wxClipboardTextEvent(a0); } ), "doc");
		cl.def( pybind11::init<int, int>(), pybind11::arg("type"), pybind11::arg("winid") );

		cl.def( pybind11::init( [](PyCallBack_wxClipboardTextEvent const &o){ return new PyCallBack_wxClipboardTextEvent(o); } ) );
		cl.def( pybind11::init( [](wxClipboardTextEvent const &o){ return new wxClipboardTextEvent(o); } ) );
		cl.def("Clone", (class wxEvent * (wxClipboardTextEvent::*)() const) &wxClipboardTextEvent::Clone, "C++: wxClipboardTextEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxClipboardTextEvent::*)() const) &wxClipboardTextEvent::GetClassInfo, "C++: wxClipboardTextEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxClipboardTextEvent::wxCreateObject, "C++: wxClipboardTextEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxContextMenuEvent file: line:3120
		pybind11::class_<wxContextMenuEvent, std::shared_ptr<wxContextMenuEvent>, PyCallBack_wxContextMenuEvent, wxCommandEvent> cl(M(""), "wxContextMenuEvent", "");
		cl.def( pybind11::init( [](){ return new wxContextMenuEvent(); }, [](){ return new PyCallBack_wxContextMenuEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxContextMenuEvent(a0); }, [](int const & a0){ return new PyCallBack_wxContextMenuEvent(a0); } ), "doc");
		cl.def( pybind11::init( [](int const & a0, int const & a1){ return new wxContextMenuEvent(a0, a1); }, [](int const & a0, int const & a1){ return new PyCallBack_wxContextMenuEvent(a0, a1); } ), "doc");
		cl.def( pybind11::init<int, int, const class wxPoint &>(), pybind11::arg("type"), pybind11::arg("winid"), pybind11::arg("pt") );

		cl.def( pybind11::init( [](PyCallBack_wxContextMenuEvent const &o){ return new PyCallBack_wxContextMenuEvent(o); } ) );
		cl.def( pybind11::init( [](wxContextMenuEvent const &o){ return new wxContextMenuEvent(o); } ) );
		cl.def("GetPosition", (const class wxPoint & (wxContextMenuEvent::*)() const) &wxContextMenuEvent::GetPosition, "C++: wxContextMenuEvent::GetPosition() const --> const class wxPoint &", pybind11::return_value_policy::automatic);
		cl.def("SetPosition", (void (wxContextMenuEvent::*)(const class wxPoint &)) &wxContextMenuEvent::SetPosition, "C++: wxContextMenuEvent::SetPosition(const class wxPoint &) --> void", pybind11::arg("pos"));
		cl.def("Clone", (class wxEvent * (wxContextMenuEvent::*)() const) &wxContextMenuEvent::Clone, "C++: wxContextMenuEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxContextMenuEvent::*)() const) &wxContextMenuEvent::GetClassInfo, "C++: wxContextMenuEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxContextMenuEvent::wxCreateObject, "C++: wxContextMenuEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxEventTableEntryBase file: line:3168
		pybind11::class_<wxEventTableEntryBase, std::shared_ptr<wxEventTableEntryBase>> cl(M(""), "wxEventTableEntryBase", "");
		cl.def( pybind11::init<int, int, class wxEventFunctor *, class wxObject *>(), pybind11::arg("winid"), pybind11::arg("idLast"), pybind11::arg("fn"), pybind11::arg("data") );

		cl.def( pybind11::init( [](wxEventTableEntryBase const &o){ return new wxEventTableEntryBase(o); } ) );
		cl.def_readwrite("m_id", &wxEventTableEntryBase::m_id);
		cl.def_readwrite("m_lastId", &wxEventTableEntryBase::m_lastId);
	}
	{ // wxEventTableEntry file: line:3217
		pybind11::class_<wxEventTableEntry, std::shared_ptr<wxEventTableEntry>, wxEventTableEntryBase> cl(M(""), "wxEventTableEntry", "");
		cl.def( pybind11::init<const int &, int, int, class wxEventFunctor *, class wxObject *>(), pybind11::arg("evType"), pybind11::arg("winid"), pybind11::arg("idLast"), pybind11::arg("fn"), pybind11::arg("data") );

		cl.def( pybind11::init( [](wxEventTableEntry const &o){ return new wxEventTableEntry(o); } ) );
	}
	{ // wxDynamicEventTableEntry file: line:3238
		pybind11::class_<wxDynamicEventTableEntry, std::shared_ptr<wxDynamicEventTableEntry>, wxEventTableEntryBase> cl(M(""), "wxDynamicEventTableEntry", "");
		cl.def( pybind11::init<int, int, int, class wxEventFunctor *, class wxObject *>(), pybind11::arg("evType"), pybind11::arg("winid"), pybind11::arg("idLast"), pybind11::arg("fn"), pybind11::arg("data") );

		cl.def( pybind11::init( [](wxDynamicEventTableEntry const &o){ return new wxDynamicEventTableEntry(o); } ) );
		cl.def_readwrite("m_eventType", &wxDynamicEventTableEntry::m_eventType);
	}
	{ // wxEventTable file: line:3259
		pybind11::class_<wxEventTable, std::shared_ptr<wxEventTable>> cl(M(""), "wxEventTable", "");
		cl.def( pybind11::init( [](){ return new wxEventTable(); } ) );
	}
	{ // wxAssert_wxEventTableEntryPointerArray file: line:91
		pybind11::class_<wxAssert_wxEventTableEntryPointerArray, std::shared_ptr<wxAssert_wxEventTableEntryPointerArray>> cl(M(""), "wxAssert_wxEventTableEntryPointerArray", "");
		cl.def( pybind11::init( [](){ return new wxAssert_wxEventTableEntryPointerArray(); } ) );
		cl.def_readwrite("TypeTooBigToBeStoredInwxBaseArrayPtrVoid", &wxAssert_wxEventTableEntryPointerArray::TypeTooBigToBeStoredInwxBaseArrayPtrVoid);
	}
	{ // wxEventTableEntryPointerArray file: line:3269
		pybind11::class_<wxEventTableEntryPointerArray, std::shared_ptr<wxEventTableEntryPointerArray>, wxBaseArrayPtrVoid> cl(M(""), "wxEventTableEntryPointerArray", "");
		cl.def( pybind11::init( [](){ return new wxEventTableEntryPointerArray(); } ) );
		cl.def( pybind11::init<unsigned long>(), pybind11::arg("n") );

		cl.def( pybind11::init<unsigned long, const struct wxEventTableEntry *const &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init( [](wxEventTableEntryPointerArray const &o){ return new wxEventTableEntryPointerArray(o); } ) );
		cl.def("__getitem__", (const struct wxEventTableEntry *& (wxEventTableEntryPointerArray::*)(unsigned long) const) &wxEventTableEntryPointerArray::operator[], "C++: wxEventTableEntryPointerArray::operator[](unsigned long) const --> const struct wxEventTableEntry *&", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Item", (const struct wxEventTableEntry *& (wxEventTableEntryPointerArray::*)(unsigned long) const) &wxEventTableEntryPointerArray::Item, "C++: wxEventTableEntryPointerArray::Item(unsigned long) const --> const struct wxEventTableEntry *&", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Last", (const struct wxEventTableEntry *& (wxEventTableEntryPointerArray::*)() const) &wxEventTableEntryPointerArray::Last, "C++: wxEventTableEntryPointerArray::Last() const --> const struct wxEventTableEntry *&", pybind11::return_value_policy::automatic);
		cl.def("Index", [](wxEventTableEntryPointerArray const &o, const struct wxEventTableEntry * a0) -> int { return o.Index(a0); }, "", pybind11::arg("lItem"));
		cl.def("Index", (int (wxEventTableEntryPointerArray::*)(const struct wxEventTableEntry *, bool) const) &wxEventTableEntryPointerArray::Index, "C++: wxEventTableEntryPointerArray::Index(const struct wxEventTableEntry *, bool) const --> int", pybind11::arg("lItem"), pybind11::arg("bFromEnd"));
		cl.def("Add", [](wxEventTableEntryPointerArray &o, const struct wxEventTableEntry * a0) -> void { return o.Add(a0); }, "", pybind11::arg("lItem"));
		cl.def("Add", (void (wxEventTableEntryPointerArray::*)(const struct wxEventTableEntry *, unsigned long)) &wxEventTableEntryPointerArray::Add, "C++: wxEventTableEntryPointerArray::Add(const struct wxEventTableEntry *, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("nInsert"));
		cl.def("Insert", [](wxEventTableEntryPointerArray &o, const struct wxEventTableEntry * a0, unsigned long const & a1) -> void { return o.Insert(a0, a1); }, "", pybind11::arg("lItem"), pybind11::arg("uiIndex"));
		cl.def("Insert", (void (wxEventTableEntryPointerArray::*)(const struct wxEventTableEntry *, unsigned long, unsigned long)) &wxEventTableEntryPointerArray::Insert, "C++: wxEventTableEntryPointerArray::Insert(const struct wxEventTableEntry *, unsigned long, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("uiIndex"), pybind11::arg("nInsert"));
		cl.def("RemoveAt", [](wxEventTableEntryPointerArray &o, unsigned long const & a0) -> void { return o.RemoveAt(a0); }, "", pybind11::arg("uiIndex"));
		cl.def("RemoveAt", (void (wxEventTableEntryPointerArray::*)(unsigned long, unsigned long)) &wxEventTableEntryPointerArray::RemoveAt, "C++: wxEventTableEntryPointerArray::RemoveAt(unsigned long, unsigned long) --> void", pybind11::arg("uiIndex"), pybind11::arg("nRemove"));
		cl.def("Remove", (void (wxEventTableEntryPointerArray::*)(const struct wxEventTableEntry *)) &wxEventTableEntryPointerArray::Remove, "C++: wxEventTableEntryPointerArray::Remove(const struct wxEventTableEntry *) --> void", pybind11::arg("lItem"));
		cl.def("assign", (void (wxEventTableEntryPointerArray::*)(unsigned long, const struct wxEventTableEntry *const &)) &wxEventTableEntryPointerArray::assign, "C++: wxEventTableEntryPointerArray::assign(unsigned long, const struct wxEventTableEntry *const &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("back", (const struct wxEventTableEntry *& (wxEventTableEntryPointerArray::*)()) &wxEventTableEntryPointerArray::back, "C++: wxEventTableEntryPointerArray::back() --> const struct wxEventTableEntry *&", pybind11::return_value_policy::automatic);
		cl.def("begin", (const struct wxEventTableEntry ** (wxEventTableEntryPointerArray::*)()) &wxEventTableEntryPointerArray::begin, "C++: wxEventTableEntryPointerArray::begin() --> const struct wxEventTableEntry **", pybind11::return_value_policy::automatic);
		cl.def("capacity", (unsigned long (wxEventTableEntryPointerArray::*)() const) &wxEventTableEntryPointerArray::capacity, "C++: wxEventTableEntryPointerArray::capacity() const --> unsigned long");
		cl.def("end", (const struct wxEventTableEntry ** (wxEventTableEntryPointerArray::*)()) &wxEventTableEntryPointerArray::end, "C++: wxEventTableEntryPointerArray::end() --> const struct wxEventTableEntry **", pybind11::return_value_policy::automatic);
		cl.def("front", (const struct wxEventTableEntry *& (wxEventTableEntryPointerArray::*)()) &wxEventTableEntryPointerArray::front, "C++: wxEventTableEntryPointerArray::front() --> const struct wxEventTableEntry *&", pybind11::return_value_policy::automatic);
		cl.def("pop_back", (void (wxEventTableEntryPointerArray::*)()) &wxEventTableEntryPointerArray::pop_back, "C++: wxEventTableEntryPointerArray::pop_back() --> void");
		cl.def("push_back", (void (wxEventTableEntryPointerArray::*)(const struct wxEventTableEntry *const &)) &wxEventTableEntryPointerArray::push_back, "C++: wxEventTableEntryPointerArray::push_back(const struct wxEventTableEntry *const &) --> void", pybind11::arg("v"));
		cl.def("rbegin", (class wxEventTableEntryPointerArray::reverse_iterator (wxEventTableEntryPointerArray::*)()) &wxEventTableEntryPointerArray::rbegin, "C++: wxEventTableEntryPointerArray::rbegin() --> class wxEventTableEntryPointerArray::reverse_iterator");
		cl.def("rend", (class wxEventTableEntryPointerArray::reverse_iterator (wxEventTableEntryPointerArray::*)()) &wxEventTableEntryPointerArray::rend, "C++: wxEventTableEntryPointerArray::rend() --> class wxEventTableEntryPointerArray::reverse_iterator");
		cl.def("reserve", (void (wxEventTableEntryPointerArray::*)(unsigned long)) &wxEventTableEntryPointerArray::reserve, "C++: wxEventTableEntryPointerArray::reserve(unsigned long) --> void", pybind11::arg("n"));
		cl.def("resize", [](wxEventTableEntryPointerArray &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxEventTableEntryPointerArray::*)(unsigned long, const struct wxEventTableEntry *)) &wxEventTableEntryPointerArray::resize, "C++: wxEventTableEntryPointerArray::resize(unsigned long, const struct wxEventTableEntry *) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("swap", (void (wxEventTableEntryPointerArray::*)(class wxEventTableEntryPointerArray &)) &wxEventTableEntryPointerArray::swap, "C++: wxEventTableEntryPointerArray::swap(class wxEventTableEntryPointerArray &) --> void", pybind11::arg("other"));
		cl.def("assign", (class wxEventTableEntryPointerArray & (wxEventTableEntryPointerArray::*)(const class wxEventTableEntryPointerArray &)) &wxEventTableEntryPointerArray::operator=, "C++: wxEventTableEntryPointerArray::operator=(const class wxEventTableEntryPointerArray &) --> class wxEventTableEntryPointerArray &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxEventTableEntryPointerArray::reverse_iterator file: line:400
			auto & enclosing_class = cl;
			pybind11::class_<wxEventTableEntryPointerArray::reverse_iterator, std::shared_ptr<wxEventTableEntryPointerArray::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxEventTableEntryPointerArray::reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxEventTableEntryPointerArray::reverse_iterator const &o){ return new wxEventTableEntryPointerArray::reverse_iterator(o); } ) );
			cl.def("__mul__", (const struct wxEventTableEntry *& (wxEventTableEntryPointerArray::reverse_iterator::*)() const) &wxEventTableEntryPointerArray::reverse_iterator::operator*, "C++: wxEventTableEntryPointerArray::reverse_iterator::operator*() const --> const struct wxEventTableEntry *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxEventTableEntryPointerArray::reverse_iterator & (wxEventTableEntryPointerArray::reverse_iterator::*)()) &wxEventTableEntryPointerArray::reverse_iterator::operator++, "C++: wxEventTableEntryPointerArray::reverse_iterator::operator++() --> class wxEventTableEntryPointerArray::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxEventTableEntryPointerArray::reverse_iterator (wxEventTableEntryPointerArray::reverse_iterator::*)(int)) &wxEventTableEntryPointerArray::reverse_iterator::operator++, "C++: wxEventTableEntryPointerArray::reverse_iterator::operator++(int) --> const class wxEventTableEntryPointerArray::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxEventTableEntryPointerArray::reverse_iterator & (wxEventTableEntryPointerArray::reverse_iterator::*)()) &wxEventTableEntryPointerArray::reverse_iterator::operator--, "C++: wxEventTableEntryPointerArray::reverse_iterator::operator--() --> class wxEventTableEntryPointerArray::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxEventTableEntryPointerArray::reverse_iterator (wxEventTableEntryPointerArray::reverse_iterator::*)(int)) &wxEventTableEntryPointerArray::reverse_iterator::operator--, "C++: wxEventTableEntryPointerArray::reverse_iterator::operator--(int) --> const class wxEventTableEntryPointerArray::reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxEventTableEntryPointerArray::reverse_iterator::*)(const class wxEventTableEntryPointerArray::reverse_iterator &) const) &wxEventTableEntryPointerArray::reverse_iterator::operator==, "C++: wxEventTableEntryPointerArray::reverse_iterator::operator==(const class wxEventTableEntryPointerArray::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxEventTableEntryPointerArray::reverse_iterator::*)(const class wxEventTableEntryPointerArray::reverse_iterator &) const) &wxEventTableEntryPointerArray::reverse_iterator::operator!=, "C++: wxEventTableEntryPointerArray::reverse_iterator::operator!=(const class wxEventTableEntryPointerArray::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxEventTableEntryPointerArray::const_reverse_iterator file: line:432
			auto & enclosing_class = cl;
			pybind11::class_<wxEventTableEntryPointerArray::const_reverse_iterator, std::shared_ptr<wxEventTableEntryPointerArray::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxEventTableEntryPointerArray::const_reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxEventTableEntryPointerArray::const_reverse_iterator const &o){ return new wxEventTableEntryPointerArray::const_reverse_iterator(o); } ) );
			cl.def( pybind11::init<const class wxEventTableEntryPointerArray::reverse_iterator &>(), pybind11::arg("it") );

			cl.def("__mul__", (const struct wxEventTableEntry *const & (wxEventTableEntryPointerArray::const_reverse_iterator::*)() const) &wxEventTableEntryPointerArray::const_reverse_iterator::operator*, "C++: wxEventTableEntryPointerArray::const_reverse_iterator::operator*() const --> const struct wxEventTableEntry *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxEventTableEntryPointerArray::const_reverse_iterator & (wxEventTableEntryPointerArray::const_reverse_iterator::*)()) &wxEventTableEntryPointerArray::const_reverse_iterator::operator++, "C++: wxEventTableEntryPointerArray::const_reverse_iterator::operator++() --> class wxEventTableEntryPointerArray::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxEventTableEntryPointerArray::const_reverse_iterator (wxEventTableEntryPointerArray::const_reverse_iterator::*)(int)) &wxEventTableEntryPointerArray::const_reverse_iterator::operator++, "C++: wxEventTableEntryPointerArray::const_reverse_iterator::operator++(int) --> const class wxEventTableEntryPointerArray::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxEventTableEntryPointerArray::const_reverse_iterator & (wxEventTableEntryPointerArray::const_reverse_iterator::*)()) &wxEventTableEntryPointerArray::const_reverse_iterator::operator--, "C++: wxEventTableEntryPointerArray::const_reverse_iterator::operator--() --> class wxEventTableEntryPointerArray::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxEventTableEntryPointerArray::const_reverse_iterator (wxEventTableEntryPointerArray::const_reverse_iterator::*)(int)) &wxEventTableEntryPointerArray::const_reverse_iterator::operator--, "C++: wxEventTableEntryPointerArray::const_reverse_iterator::operator--(int) --> const class wxEventTableEntryPointerArray::const_reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxEventTableEntryPointerArray::const_reverse_iterator::*)(const class wxEventTableEntryPointerArray::const_reverse_iterator &) const) &wxEventTableEntryPointerArray::const_reverse_iterator::operator==, "C++: wxEventTableEntryPointerArray::const_reverse_iterator::operator==(const class wxEventTableEntryPointerArray::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxEventTableEntryPointerArray::const_reverse_iterator::*)(const class wxEventTableEntryPointerArray::const_reverse_iterator &) const) &wxEventTableEntryPointerArray::const_reverse_iterator::operator!=, "C++: wxEventTableEntryPointerArray::const_reverse_iterator::operator!=(const class wxEventTableEntryPointerArray::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
}

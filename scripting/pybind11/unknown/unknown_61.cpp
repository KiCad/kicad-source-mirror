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

// wxEvtHandler file: line:3333
struct PyCallBack_wxEvtHandler : public wxEvtHandler {
	using wxEvtHandler::wxEvtHandler;

	void SetNextHandler(class wxEvtHandler * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "SetNextHandler");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEvtHandler::SetNextHandler(a0);
	}
	void SetPreviousHandler(class wxEvtHandler * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "SetPreviousHandler");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEvtHandler::SetPreviousHandler(a0);
	}
	bool ProcessEvent(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "ProcessEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEvtHandler::ProcessEvent(a0);
	}
	void QueueEvent(class wxEvent * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "QueueEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEvtHandler::QueueEvent(a0);
	}
	void AddPendingEvent(const class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "AddPendingEvent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEvtHandler::AddPendingEvent(a0);
	}
	bool SearchEventTable(struct wxEventTable & a0, class wxEvent & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "SearchEventTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEvtHandler::SearchEventTable(a0, a1);
	}
	bool TryBefore(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "TryBefore");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEvtHandler::TryBefore(a0);
	}
	bool TryAfter(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "TryAfter");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEvtHandler::TryAfter(a0);
	}
	bool TryValidator(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "TryValidator");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEvtHandler::TryValidator(a0);
	}
	bool TryParent(class wxEvent & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "TryParent");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxEvtHandler::TryParent(a0);
	}
	const struct wxEventTable * GetEventTable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "GetEventTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const struct wxEventTable *>::value) {
				static pybind11::detail::override_caster_t<const struct wxEventTable *> caster;
				return pybind11::detail::cast_ref<const struct wxEventTable *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const struct wxEventTable *>(std::move(o));
		}
		return wxEvtHandler::GetEventTable();
	}
	class wxEventHashTable & GetEventHashTable() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "GetEventHashTable");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEventHashTable &>::value) {
				static pybind11::detail::override_caster_t<class wxEventHashTable &> caster;
				return pybind11::detail::cast_ref<class wxEventHashTable &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEventHashTable &>(std::move(o));
		}
		return wxEvtHandler::GetEventHashTable();
	}
	void DoSetClientObject(class wxClientData * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "DoSetClientObject");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEvtHandler::DoSetClientObject(a0);
	}
	class wxClientData * DoGetClientObject() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "DoGetClientObject");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClientData *>::value) {
				static pybind11::detail::override_caster_t<class wxClientData *> caster;
				return pybind11::detail::cast_ref<class wxClientData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClientData *>(std::move(o));
		}
		return wxEvtHandler::DoGetClientObject();
	}
	void DoSetClientData(void * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "DoSetClientData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxEvtHandler::DoSetClientData(a0);
	}
	void * DoGetClientData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "DoGetClientData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void *>::value) {
				static pybind11::detail::override_caster_t<void *> caster;
				return pybind11::detail::cast_ref<void *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void *>(std::move(o));
		}
		return wxEvtHandler::DoGetClientData();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxEvtHandler::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEvtHandler *>(this), "CloneRefData");
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

void bind_unknown_unknown_61(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxEventHashTable file: line:3271
		pybind11::class_<wxEventHashTable, std::shared_ptr<wxEventHashTable>> cl(M(""), "wxEventHashTable", "");
		cl.def( pybind11::init<const struct wxEventTable &>(), pybind11::arg("table") );

		cl.def("HandleEvent", (bool (wxEventHashTable::*)(class wxEvent &, class wxEvtHandler *)) &wxEventHashTable::HandleEvent, "C++: wxEventHashTable::HandleEvent(class wxEvent &, class wxEvtHandler *) --> bool", pybind11::arg("event"), pybind11::arg("self"));
		cl.def("Clear", (void (wxEventHashTable::*)()) &wxEventHashTable::Clear, "C++: wxEventHashTable::Clear() --> void");
	}
	{ // wxEvtHandler file: line:3333
		pybind11::class_<wxEvtHandler, std::shared_ptr<wxEvtHandler>, PyCallBack_wxEvtHandler, wxObject, wxTrackable> cl(M(""), "wxEvtHandler", "");
		cl.def( pybind11::init( [](){ return new wxEvtHandler(); }, [](){ return new PyCallBack_wxEvtHandler(); } ) );
		cl.def("GetNextHandler", (class wxEvtHandler * (wxEvtHandler::*)() const) &wxEvtHandler::GetNextHandler, "C++: wxEvtHandler::GetNextHandler() const --> class wxEvtHandler *", pybind11::return_value_policy::automatic);
		cl.def("GetPreviousHandler", (class wxEvtHandler * (wxEvtHandler::*)() const) &wxEvtHandler::GetPreviousHandler, "C++: wxEvtHandler::GetPreviousHandler() const --> class wxEvtHandler *", pybind11::return_value_policy::automatic);
		cl.def("SetNextHandler", (void (wxEvtHandler::*)(class wxEvtHandler *)) &wxEvtHandler::SetNextHandler, "C++: wxEvtHandler::SetNextHandler(class wxEvtHandler *) --> void", pybind11::arg("handler"));
		cl.def("SetPreviousHandler", (void (wxEvtHandler::*)(class wxEvtHandler *)) &wxEvtHandler::SetPreviousHandler, "C++: wxEvtHandler::SetPreviousHandler(class wxEvtHandler *) --> void", pybind11::arg("handler"));
		cl.def("SetEvtHandlerEnabled", (void (wxEvtHandler::*)(bool)) &wxEvtHandler::SetEvtHandlerEnabled, "C++: wxEvtHandler::SetEvtHandlerEnabled(bool) --> void", pybind11::arg("enabled"));
		cl.def("GetEvtHandlerEnabled", (bool (wxEvtHandler::*)() const) &wxEvtHandler::GetEvtHandlerEnabled, "C++: wxEvtHandler::GetEvtHandlerEnabled() const --> bool");
		cl.def("Unlink", (void (wxEvtHandler::*)()) &wxEvtHandler::Unlink, "C++: wxEvtHandler::Unlink() --> void");
		cl.def("IsUnlinked", (bool (wxEvtHandler::*)() const) &wxEvtHandler::IsUnlinked, "C++: wxEvtHandler::IsUnlinked() const --> bool");
		cl.def("ProcessEvent", (bool (wxEvtHandler::*)(class wxEvent &)) &wxEvtHandler::ProcessEvent, "C++: wxEvtHandler::ProcessEvent(class wxEvent &) --> bool", pybind11::arg("event"));
		cl.def("SafelyProcessEvent", (bool (wxEvtHandler::*)(class wxEvent &)) &wxEvtHandler::SafelyProcessEvent, "C++: wxEvtHandler::SafelyProcessEvent(class wxEvent &) --> bool", pybind11::arg("event"));
		cl.def("ProcessEventLocally", (bool (wxEvtHandler::*)(class wxEvent &)) &wxEvtHandler::ProcessEventLocally, "C++: wxEvtHandler::ProcessEventLocally(class wxEvent &) --> bool", pybind11::arg("event"));
		cl.def("QueueEvent", (void (wxEvtHandler::*)(class wxEvent *)) &wxEvtHandler::QueueEvent, "C++: wxEvtHandler::QueueEvent(class wxEvent *) --> void", pybind11::arg("event"));
		cl.def("AddPendingEvent", (void (wxEvtHandler::*)(const class wxEvent &)) &wxEvtHandler::AddPendingEvent, "C++: wxEvtHandler::AddPendingEvent(const class wxEvent &) --> void", pybind11::arg("event"));
		cl.def("ProcessPendingEvents", (void (wxEvtHandler::*)()) &wxEvtHandler::ProcessPendingEvents, "C++: wxEvtHandler::ProcessPendingEvents() --> void");
		cl.def("DeletePendingEvents", (void (wxEvtHandler::*)()) &wxEvtHandler::DeletePendingEvents, "C++: wxEvtHandler::DeletePendingEvents() --> void");
		cl.def("ProcessThreadEvent", (bool (wxEvtHandler::*)(const class wxEvent &)) &wxEvtHandler::ProcessThreadEvent, "C++: wxEvtHandler::ProcessThreadEvent(const class wxEvent &) --> bool", pybind11::arg("event"));
		cl.def("Connect", [](wxEvtHandler &o, int const & a0, int const & a1, int const & a2, void (class wxEvtHandler::*)(class wxEvent &) const & a3) -> void { return o.Connect(a0, a1, a2, a3); }, "", pybind11::arg("winid"), pybind11::arg("lastId"), pybind11::arg("eventType"), pybind11::arg("func"));
		cl.def("Connect", [](wxEvtHandler &o, int const & a0, int const & a1, int const & a2, void (class wxEvtHandler::*)(class wxEvent &) const & a3, class wxObject * a4) -> void { return o.Connect(a0, a1, a2, a3, a4); }, "", pybind11::arg("winid"), pybind11::arg("lastId"), pybind11::arg("eventType"), pybind11::arg("func"), pybind11::arg("userData"));
		cl.def("Connect", (void (wxEvtHandler::*)(int, int, int, void (class wxEvtHandler::*)(class wxEvent &), class wxObject *, class wxEvtHandler *)) &wxEvtHandler::Connect, "C++: wxEvtHandler::Connect(int, int, int, void (class wxEvtHandler::*)(class wxEvent &), class wxObject *, class wxEvtHandler *) --> void", pybind11::arg("winid"), pybind11::arg("lastId"), pybind11::arg("eventType"), pybind11::arg("func"), pybind11::arg("userData"), pybind11::arg("eventSink"));
		cl.def("Connect", [](wxEvtHandler &o, int const & a0, int const & a1, void (class wxEvtHandler::*)(class wxEvent &) const & a2) -> void { return o.Connect(a0, a1, a2); }, "", pybind11::arg("winid"), pybind11::arg("eventType"), pybind11::arg("func"));
		cl.def("Connect", [](wxEvtHandler &o, int const & a0, int const & a1, void (class wxEvtHandler::*)(class wxEvent &) const & a2, class wxObject * a3) -> void { return o.Connect(a0, a1, a2, a3); }, "", pybind11::arg("winid"), pybind11::arg("eventType"), pybind11::arg("func"), pybind11::arg("userData"));
		cl.def("Connect", (void (wxEvtHandler::*)(int, int, void (class wxEvtHandler::*)(class wxEvent &), class wxObject *, class wxEvtHandler *)) &wxEvtHandler::Connect, "C++: wxEvtHandler::Connect(int, int, void (class wxEvtHandler::*)(class wxEvent &), class wxObject *, class wxEvtHandler *) --> void", pybind11::arg("winid"), pybind11::arg("eventType"), pybind11::arg("func"), pybind11::arg("userData"), pybind11::arg("eventSink"));
		cl.def("Connect", [](wxEvtHandler &o, int const & a0, void (class wxEvtHandler::*)(class wxEvent &) const & a1) -> void { return o.Connect(a0, a1); }, "", pybind11::arg("eventType"), pybind11::arg("func"));
		cl.def("Connect", [](wxEvtHandler &o, int const & a0, void (class wxEvtHandler::*)(class wxEvent &) const & a1, class wxObject * a2) -> void { return o.Connect(a0, a1, a2); }, "", pybind11::arg("eventType"), pybind11::arg("func"), pybind11::arg("userData"));
		cl.def("Connect", (void (wxEvtHandler::*)(int, void (class wxEvtHandler::*)(class wxEvent &), class wxObject *, class wxEvtHandler *)) &wxEvtHandler::Connect, "C++: wxEvtHandler::Connect(int, void (class wxEvtHandler::*)(class wxEvent &), class wxObject *, class wxEvtHandler *) --> void", pybind11::arg("eventType"), pybind11::arg("func"), pybind11::arg("userData"), pybind11::arg("eventSink"));
		cl.def("Disconnect", [](wxEvtHandler &o, int const & a0, int const & a1, int const & a2) -> bool { return o.Disconnect(a0, a1, a2); }, "", pybind11::arg("winid"), pybind11::arg("lastId"), pybind11::arg("eventType"));
		cl.def("Disconnect", [](wxEvtHandler &o, int const & a0, int const & a1, int const & a2, void (class wxEvtHandler::*)(class wxEvent &) const & a3) -> bool { return o.Disconnect(a0, a1, a2, a3); }, "", pybind11::arg("winid"), pybind11::arg("lastId"), pybind11::arg("eventType"), pybind11::arg("func"));
		cl.def("Disconnect", [](wxEvtHandler &o, int const & a0, int const & a1, int const & a2, void (class wxEvtHandler::*)(class wxEvent &) const & a3, class wxObject * a4) -> bool { return o.Disconnect(a0, a1, a2, a3, a4); }, "", pybind11::arg("winid"), pybind11::arg("lastId"), pybind11::arg("eventType"), pybind11::arg("func"), pybind11::arg("userData"));
		cl.def("Disconnect", (bool (wxEvtHandler::*)(int, int, int, void (class wxEvtHandler::*)(class wxEvent &), class wxObject *, class wxEvtHandler *)) &wxEvtHandler::Disconnect, "C++: wxEvtHandler::Disconnect(int, int, int, void (class wxEvtHandler::*)(class wxEvent &), class wxObject *, class wxEvtHandler *) --> bool", pybind11::arg("winid"), pybind11::arg("lastId"), pybind11::arg("eventType"), pybind11::arg("func"), pybind11::arg("userData"), pybind11::arg("eventSink"));
		cl.def("Disconnect", [](wxEvtHandler &o) -> bool { return o.Disconnect(); }, "");
		cl.def("Disconnect", [](wxEvtHandler &o, int const & a0) -> bool { return o.Disconnect(a0); }, "", pybind11::arg("winid"));
		cl.def("Disconnect", [](wxEvtHandler &o, int const & a0, int const & a1) -> bool { return o.Disconnect(a0, a1); }, "", pybind11::arg("winid"), pybind11::arg("eventType"));
		cl.def("Disconnect", [](wxEvtHandler &o, int const & a0, int const & a1, void (class wxEvtHandler::*)(class wxEvent &) const & a2) -> bool { return o.Disconnect(a0, a1, a2); }, "", pybind11::arg("winid"), pybind11::arg("eventType"), pybind11::arg("func"));
		cl.def("Disconnect", [](wxEvtHandler &o, int const & a0, int const & a1, void (class wxEvtHandler::*)(class wxEvent &) const & a2, class wxObject * a3) -> bool { return o.Disconnect(a0, a1, a2, a3); }, "", pybind11::arg("winid"), pybind11::arg("eventType"), pybind11::arg("func"), pybind11::arg("userData"));
		cl.def("Disconnect", (bool (wxEvtHandler::*)(int, int, void (class wxEvtHandler::*)(class wxEvent &), class wxObject *, class wxEvtHandler *)) &wxEvtHandler::Disconnect, "C++: wxEvtHandler::Disconnect(int, int, void (class wxEvtHandler::*)(class wxEvent &), class wxObject *, class wxEvtHandler *) --> bool", pybind11::arg("winid"), pybind11::arg("eventType"), pybind11::arg("func"), pybind11::arg("userData"), pybind11::arg("eventSink"));
		cl.def("Disconnect", [](wxEvtHandler &o, int const & a0, void (class wxEvtHandler::*)(class wxEvent &) const & a1) -> bool { return o.Disconnect(a0, a1); }, "", pybind11::arg("eventType"), pybind11::arg("func"));
		cl.def("Disconnect", [](wxEvtHandler &o, int const & a0, void (class wxEvtHandler::*)(class wxEvent &) const & a1, class wxObject * a2) -> bool { return o.Disconnect(a0, a1, a2); }, "", pybind11::arg("eventType"), pybind11::arg("func"), pybind11::arg("userData"));
		cl.def("Disconnect", (bool (wxEvtHandler::*)(int, void (class wxEvtHandler::*)(class wxEvent &), class wxObject *, class wxEvtHandler *)) &wxEvtHandler::Disconnect, "C++: wxEvtHandler::Disconnect(int, void (class wxEvtHandler::*)(class wxEvent &), class wxObject *, class wxEvtHandler *) --> bool", pybind11::arg("eventType"), pybind11::arg("func"), pybind11::arg("userData"), pybind11::arg("eventSink"));
		cl.def("GetDynamicEventTable", (class wxList * (wxEvtHandler::*)() const) &wxEvtHandler::GetDynamicEventTable, "C++: wxEvtHandler::GetDynamicEventTable() const --> class wxList *", pybind11::return_value_policy::automatic);
		cl.def("SetClientObject", (void (wxEvtHandler::*)(class wxClientData *)) &wxEvtHandler::SetClientObject, "C++: wxEvtHandler::SetClientObject(class wxClientData *) --> void", pybind11::arg("data"));
		cl.def("GetClientObject", (class wxClientData * (wxEvtHandler::*)() const) &wxEvtHandler::GetClientObject, "C++: wxEvtHandler::GetClientObject() const --> class wxClientData *", pybind11::return_value_policy::automatic);
		cl.def("SetClientData", (void (wxEvtHandler::*)(void *)) &wxEvtHandler::SetClientData, "C++: wxEvtHandler::SetClientData(void *) --> void", pybind11::arg("data"));
		cl.def("GetClientData", (void * (wxEvtHandler::*)() const) &wxEvtHandler::GetClientData, "C++: wxEvtHandler::GetClientData() const --> void *", pybind11::return_value_policy::automatic);
		cl.def_static("ProcessEventIfMatchesId", (bool (*)(const struct wxEventTableEntryBase &, class wxEvtHandler *, class wxEvent &)) &wxEvtHandler::ProcessEventIfMatchesId, "C++: wxEvtHandler::ProcessEventIfMatchesId(const struct wxEventTableEntryBase &, class wxEvtHandler *, class wxEvent &) --> bool", pybind11::arg("tableEntry"), pybind11::arg("handler"), pybind11::arg("event"));
		cl.def("SearchEventTable", (bool (wxEvtHandler::*)(struct wxEventTable &, class wxEvent &)) &wxEvtHandler::SearchEventTable, "C++: wxEvtHandler::SearchEventTable(struct wxEventTable &, class wxEvent &) --> bool", pybind11::arg("table"), pybind11::arg("event"));
		cl.def("SearchDynamicEventTable", (bool (wxEvtHandler::*)(class wxEvent &)) &wxEvtHandler::SearchDynamicEventTable, "C++: wxEvtHandler::SearchDynamicEventTable(class wxEvent &) --> bool", pybind11::arg("event"));
		cl.def("ClearEventHashTable", (void (wxEvtHandler::*)()) &wxEvtHandler::ClearEventHashTable, "C++: wxEvtHandler::ClearEventHashTable() --> void");
		cl.def("OnSinkDestroyed", (void (wxEvtHandler::*)(class wxEvtHandler *)) &wxEvtHandler::OnSinkDestroyed, "C++: wxEvtHandler::OnSinkDestroyed(class wxEvtHandler *) --> void", pybind11::arg("sink"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxEvtHandler::*)() const) &wxEvtHandler::GetClassInfo, "C++: wxEvtHandler::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxEvtHandler::wxCreateObject, "C++: wxEvtHandler::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
	{ // wxAssert_wxEvtHandlerArray file: line:100
		pybind11::class_<wxAssert_wxEvtHandlerArray, std::shared_ptr<wxAssert_wxEvtHandlerArray>> cl(M(""), "wxAssert_wxEvtHandlerArray", "");
		cl.def( pybind11::init( [](){ return new wxAssert_wxEvtHandlerArray(); } ) );
		cl.def_readwrite("TypeTooBigToBeStoredInwxBaseArrayPtrVoid", &wxAssert_wxEvtHandlerArray::TypeTooBigToBeStoredInwxBaseArrayPtrVoid);
	}
	{ // wxEvtHandlerArray file: line:3768
		pybind11::class_<wxEvtHandlerArray, std::shared_ptr<wxEvtHandlerArray>, wxBaseArrayPtrVoid> cl(M(""), "wxEvtHandlerArray", "");
		cl.def( pybind11::init( [](){ return new wxEvtHandlerArray(); } ) );
		cl.def( pybind11::init<unsigned long>(), pybind11::arg("n") );

		cl.def( pybind11::init<unsigned long, class wxEvtHandler *const &>(), pybind11::arg("n"), pybind11::arg("v") );

		cl.def( pybind11::init( [](wxEvtHandlerArray const &o){ return new wxEvtHandlerArray(o); } ) );
		cl.def("__getitem__", (class wxEvtHandler *& (wxEvtHandlerArray::*)(unsigned long) const) &wxEvtHandlerArray::operator[], "C++: wxEvtHandlerArray::operator[](unsigned long) const --> class wxEvtHandler *&", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Item", (class wxEvtHandler *& (wxEvtHandlerArray::*)(unsigned long) const) &wxEvtHandlerArray::Item, "C++: wxEvtHandlerArray::Item(unsigned long) const --> class wxEvtHandler *&", pybind11::return_value_policy::automatic, pybind11::arg("uiIndex"));
		cl.def("Last", (class wxEvtHandler *& (wxEvtHandlerArray::*)() const) &wxEvtHandlerArray::Last, "C++: wxEvtHandlerArray::Last() const --> class wxEvtHandler *&", pybind11::return_value_policy::automatic);
		cl.def("Index", [](wxEvtHandlerArray const &o, class wxEvtHandler * a0) -> int { return o.Index(a0); }, "", pybind11::arg("lItem"));
		cl.def("Index", (int (wxEvtHandlerArray::*)(class wxEvtHandler *, bool) const) &wxEvtHandlerArray::Index, "C++: wxEvtHandlerArray::Index(class wxEvtHandler *, bool) const --> int", pybind11::arg("lItem"), pybind11::arg("bFromEnd"));
		cl.def("Add", [](wxEvtHandlerArray &o, class wxEvtHandler * a0) -> void { return o.Add(a0); }, "", pybind11::arg("lItem"));
		cl.def("Add", (void (wxEvtHandlerArray::*)(class wxEvtHandler *, unsigned long)) &wxEvtHandlerArray::Add, "C++: wxEvtHandlerArray::Add(class wxEvtHandler *, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("nInsert"));
		cl.def("Insert", [](wxEvtHandlerArray &o, class wxEvtHandler * a0, unsigned long const & a1) -> void { return o.Insert(a0, a1); }, "", pybind11::arg("lItem"), pybind11::arg("uiIndex"));
		cl.def("Insert", (void (wxEvtHandlerArray::*)(class wxEvtHandler *, unsigned long, unsigned long)) &wxEvtHandlerArray::Insert, "C++: wxEvtHandlerArray::Insert(class wxEvtHandler *, unsigned long, unsigned long) --> void", pybind11::arg("lItem"), pybind11::arg("uiIndex"), pybind11::arg("nInsert"));
		cl.def("RemoveAt", [](wxEvtHandlerArray &o, unsigned long const & a0) -> void { return o.RemoveAt(a0); }, "", pybind11::arg("uiIndex"));
		cl.def("RemoveAt", (void (wxEvtHandlerArray::*)(unsigned long, unsigned long)) &wxEvtHandlerArray::RemoveAt, "C++: wxEvtHandlerArray::RemoveAt(unsigned long, unsigned long) --> void", pybind11::arg("uiIndex"), pybind11::arg("nRemove"));
		cl.def("Remove", (void (wxEvtHandlerArray::*)(class wxEvtHandler *)) &wxEvtHandlerArray::Remove, "C++: wxEvtHandlerArray::Remove(class wxEvtHandler *) --> void", pybind11::arg("lItem"));
		cl.def("assign", (void (wxEvtHandlerArray::*)(unsigned long, class wxEvtHandler *const &)) &wxEvtHandlerArray::assign, "C++: wxEvtHandlerArray::assign(unsigned long, class wxEvtHandler *const &) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("back", (class wxEvtHandler *& (wxEvtHandlerArray::*)()) &wxEvtHandlerArray::back, "C++: wxEvtHandlerArray::back() --> class wxEvtHandler *&", pybind11::return_value_policy::automatic);
		cl.def("begin", (class wxEvtHandler ** (wxEvtHandlerArray::*)()) &wxEvtHandlerArray::begin, "C++: wxEvtHandlerArray::begin() --> class wxEvtHandler **", pybind11::return_value_policy::automatic);
		cl.def("capacity", (unsigned long (wxEvtHandlerArray::*)() const) &wxEvtHandlerArray::capacity, "C++: wxEvtHandlerArray::capacity() const --> unsigned long");
		cl.def("end", (class wxEvtHandler ** (wxEvtHandlerArray::*)()) &wxEvtHandlerArray::end, "C++: wxEvtHandlerArray::end() --> class wxEvtHandler **", pybind11::return_value_policy::automatic);
		cl.def("front", (class wxEvtHandler *& (wxEvtHandlerArray::*)()) &wxEvtHandlerArray::front, "C++: wxEvtHandlerArray::front() --> class wxEvtHandler *&", pybind11::return_value_policy::automatic);
		cl.def("pop_back", (void (wxEvtHandlerArray::*)()) &wxEvtHandlerArray::pop_back, "C++: wxEvtHandlerArray::pop_back() --> void");
		cl.def("push_back", (void (wxEvtHandlerArray::*)(class wxEvtHandler *const &)) &wxEvtHandlerArray::push_back, "C++: wxEvtHandlerArray::push_back(class wxEvtHandler *const &) --> void", pybind11::arg("v"));
		cl.def("rbegin", (class wxEvtHandlerArray::reverse_iterator (wxEvtHandlerArray::*)()) &wxEvtHandlerArray::rbegin, "C++: wxEvtHandlerArray::rbegin() --> class wxEvtHandlerArray::reverse_iterator");
		cl.def("rend", (class wxEvtHandlerArray::reverse_iterator (wxEvtHandlerArray::*)()) &wxEvtHandlerArray::rend, "C++: wxEvtHandlerArray::rend() --> class wxEvtHandlerArray::reverse_iterator");
		cl.def("reserve", (void (wxEvtHandlerArray::*)(unsigned long)) &wxEvtHandlerArray::reserve, "C++: wxEvtHandlerArray::reserve(unsigned long) --> void", pybind11::arg("n"));
		cl.def("resize", [](wxEvtHandlerArray &o, unsigned long const & a0) -> void { return o.resize(a0); }, "", pybind11::arg("n"));
		cl.def("resize", (void (wxEvtHandlerArray::*)(unsigned long, class wxEvtHandler *)) &wxEvtHandlerArray::resize, "C++: wxEvtHandlerArray::resize(unsigned long, class wxEvtHandler *) --> void", pybind11::arg("n"), pybind11::arg("v"));
		cl.def("swap", (void (wxEvtHandlerArray::*)(class wxEvtHandlerArray &)) &wxEvtHandlerArray::swap, "C++: wxEvtHandlerArray::swap(class wxEvtHandlerArray &) --> void", pybind11::arg("other"));
		cl.def("assign", (class wxEvtHandlerArray & (wxEvtHandlerArray::*)(const class wxEvtHandlerArray &)) &wxEvtHandlerArray::operator=, "C++: wxEvtHandlerArray::operator=(const class wxEvtHandlerArray &) --> class wxEvtHandlerArray &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxEvtHandlerArray::reverse_iterator file: line:400
			auto & enclosing_class = cl;
			pybind11::class_<wxEvtHandlerArray::reverse_iterator, std::shared_ptr<wxEvtHandlerArray::reverse_iterator>> cl(enclosing_class, "reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxEvtHandlerArray::reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxEvtHandlerArray::reverse_iterator const &o){ return new wxEvtHandlerArray::reverse_iterator(o); } ) );
			cl.def("__mul__", (class wxEvtHandler *& (wxEvtHandlerArray::reverse_iterator::*)() const) &wxEvtHandlerArray::reverse_iterator::operator*, "C++: wxEvtHandlerArray::reverse_iterator::operator*() const --> class wxEvtHandler *&", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxEvtHandlerArray::reverse_iterator & (wxEvtHandlerArray::reverse_iterator::*)()) &wxEvtHandlerArray::reverse_iterator::operator++, "C++: wxEvtHandlerArray::reverse_iterator::operator++() --> class wxEvtHandlerArray::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxEvtHandlerArray::reverse_iterator (wxEvtHandlerArray::reverse_iterator::*)(int)) &wxEvtHandlerArray::reverse_iterator::operator++, "C++: wxEvtHandlerArray::reverse_iterator::operator++(int) --> const class wxEvtHandlerArray::reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxEvtHandlerArray::reverse_iterator & (wxEvtHandlerArray::reverse_iterator::*)()) &wxEvtHandlerArray::reverse_iterator::operator--, "C++: wxEvtHandlerArray::reverse_iterator::operator--() --> class wxEvtHandlerArray::reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxEvtHandlerArray::reverse_iterator (wxEvtHandlerArray::reverse_iterator::*)(int)) &wxEvtHandlerArray::reverse_iterator::operator--, "C++: wxEvtHandlerArray::reverse_iterator::operator--(int) --> const class wxEvtHandlerArray::reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxEvtHandlerArray::reverse_iterator::*)(const class wxEvtHandlerArray::reverse_iterator &) const) &wxEvtHandlerArray::reverse_iterator::operator==, "C++: wxEvtHandlerArray::reverse_iterator::operator==(const class wxEvtHandlerArray::reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxEvtHandlerArray::reverse_iterator::*)(const class wxEvtHandlerArray::reverse_iterator &) const) &wxEvtHandlerArray::reverse_iterator::operator!=, "C++: wxEvtHandlerArray::reverse_iterator::operator!=(const class wxEvtHandlerArray::reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

		{ // wxEvtHandlerArray::const_reverse_iterator file: line:432
			auto & enclosing_class = cl;
			pybind11::class_<wxEvtHandlerArray::const_reverse_iterator, std::shared_ptr<wxEvtHandlerArray::const_reverse_iterator>> cl(enclosing_class, "const_reverse_iterator", "");
			cl.def( pybind11::init( [](){ return new wxEvtHandlerArray::const_reverse_iterator(); } ) );
			cl.def( pybind11::init( [](wxEvtHandlerArray::const_reverse_iterator const &o){ return new wxEvtHandlerArray::const_reverse_iterator(o); } ) );
			cl.def( pybind11::init<const class wxEvtHandlerArray::reverse_iterator &>(), pybind11::arg("it") );

			cl.def("__mul__", (class wxEvtHandler *const & (wxEvtHandlerArray::const_reverse_iterator::*)() const) &wxEvtHandlerArray::const_reverse_iterator::operator*, "C++: wxEvtHandlerArray::const_reverse_iterator::operator*() const --> class wxEvtHandler *const &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxEvtHandlerArray::const_reverse_iterator & (wxEvtHandlerArray::const_reverse_iterator::*)()) &wxEvtHandlerArray::const_reverse_iterator::operator++, "C++: wxEvtHandlerArray::const_reverse_iterator::operator++() --> class wxEvtHandlerArray::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxEvtHandlerArray::const_reverse_iterator (wxEvtHandlerArray::const_reverse_iterator::*)(int)) &wxEvtHandlerArray::const_reverse_iterator::operator++, "C++: wxEvtHandlerArray::const_reverse_iterator::operator++(int) --> const class wxEvtHandlerArray::const_reverse_iterator", pybind11::arg(""));
			cl.def("minus_minus", (class wxEvtHandlerArray::const_reverse_iterator & (wxEvtHandlerArray::const_reverse_iterator::*)()) &wxEvtHandlerArray::const_reverse_iterator::operator--, "C++: wxEvtHandlerArray::const_reverse_iterator::operator--() --> class wxEvtHandlerArray::const_reverse_iterator &", pybind11::return_value_policy::automatic);
			cl.def("minus_minus", (const class wxEvtHandlerArray::const_reverse_iterator (wxEvtHandlerArray::const_reverse_iterator::*)(int)) &wxEvtHandlerArray::const_reverse_iterator::operator--, "C++: wxEvtHandlerArray::const_reverse_iterator::operator--(int) --> const class wxEvtHandlerArray::const_reverse_iterator", pybind11::arg(""));
			cl.def("__eq__", (bool (wxEvtHandlerArray::const_reverse_iterator::*)(const class wxEvtHandlerArray::const_reverse_iterator &) const) &wxEvtHandlerArray::const_reverse_iterator::operator==, "C++: wxEvtHandlerArray::const_reverse_iterator::operator==(const class wxEvtHandlerArray::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__ne__", (bool (wxEvtHandlerArray::const_reverse_iterator::*)(const class wxEvtHandlerArray::const_reverse_iterator &) const) &wxEvtHandlerArray::const_reverse_iterator::operator!=, "C++: wxEvtHandlerArray::const_reverse_iterator::operator!=(const class wxEvtHandlerArray::const_reverse_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
}

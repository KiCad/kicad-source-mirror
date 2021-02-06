#include <sstream> // __str__
#include <wx/dc.h> // wxDC

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxEventFunctor file: line:211
struct PyCallBack_wxEventFunctor : public wxEventFunctor {
	using wxEventFunctor::wxEventFunctor;

	void operator()(class wxEvtHandler * a0, class wxEvent & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventFunctor *>(this), "__call__");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxEventFunctor::__call__\"");
	}
	bool IsMatching(const class wxEventFunctor & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventFunctor *>(this), "IsMatching");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxEventFunctor::IsMatching\"");
	}
	class wxEvtHandler * GetEvtHandler() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventFunctor *>(this), "GetEvtHandler");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvtHandler *>::value) {
				static pybind11::detail::override_caster_t<class wxEvtHandler *> caster;
				return pybind11::detail::cast_ref<class wxEvtHandler *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvtHandler *>(std::move(o));
		}
		return wxEventFunctor::GetEvtHandler();
	}
	void (class wxEvtHandler::*)(class wxEvent &) GetEvtMethod() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxEventFunctor *>(this), "GetEvtMethod");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void (class wxEvtHandler::*)(class wxEvent &)>::value) {
				static pybind11::detail::override_caster_t<void (class wxEvtHandler::*)(class wxEvent &)> caster;
				return pybind11::detail::cast_ref<void (class wxEvtHandler::*)(class wxEvent &)>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void (class wxEvtHandler::*)(class wxEvent &)>(std::move(o));
		}
		return wxEventFunctor::GetEvtMethod();
	}
};

// wxObjectEventFunctor file: line:240
struct PyCallBack_wxObjectEventFunctor : public wxObjectEventFunctor {
	using wxObjectEventFunctor::wxObjectEventFunctor;

	void operator()(class wxEvtHandler * a0, class wxEvent & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxObjectEventFunctor *>(this), "__call__");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxObjectEventFunctor::operator()(a0, a1);
	}
	bool IsMatching(const class wxEventFunctor & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxObjectEventFunctor *>(this), "IsMatching");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxObjectEventFunctor::IsMatching(a0);
	}
	class wxEvtHandler * GetEvtHandler() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxObjectEventFunctor *>(this), "GetEvtHandler");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvtHandler *>::value) {
				static pybind11::detail::override_caster_t<class wxEvtHandler *> caster;
				return pybind11::detail::cast_ref<class wxEvtHandler *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvtHandler *>(std::move(o));
		}
		return wxObjectEventFunctor::GetEvtHandler();
	}
	void (class wxEvtHandler::*)(class wxEvent &) GetEvtMethod() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxObjectEventFunctor *>(this), "GetEvtMethod");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void (class wxEvtHandler::*)(class wxEvent &)>::value) {
				static pybind11::detail::override_caster_t<void (class wxEvtHandler::*)(class wxEvent &)> caster;
				return pybind11::detail::cast_ref<void (class wxEvtHandler::*)(class wxEvent &)>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void (class wxEvtHandler::*)(class wxEvent &)>(std::move(o));
		}
		return wxObjectEventFunctor::GetEvtMethod();
	}
};

void bind_unknown_unknown_54(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxNewEventType() file: line:97
	M("").def("wxNewEventType", (int (*)()) &wxNewEventType, "C++: wxNewEventType() --> int");

	{ // wxEventFunctor file: line:211
		pybind11::class_<wxEventFunctor, std::shared_ptr<wxEventFunctor>, PyCallBack_wxEventFunctor> cl(M(""), "wxEventFunctor", "");
		cl.def(pybind11::init<PyCallBack_wxEventFunctor const &>());
		cl.def( pybind11::init( [](){ return new PyCallBack_wxEventFunctor(); } ) );
		cl.def("__call__", (void (wxEventFunctor::*)(class wxEvtHandler *, class wxEvent &)) &wxEventFunctor::operator(), "C++: wxEventFunctor::operator()(class wxEvtHandler *, class wxEvent &) --> void", pybind11::arg(""), pybind11::arg(""));
		cl.def("IsMatching", (bool (wxEventFunctor::*)(const class wxEventFunctor &) const) &wxEventFunctor::IsMatching, "C++: wxEventFunctor::IsMatching(const class wxEventFunctor &) const --> bool", pybind11::arg("functor"));
		cl.def("GetEvtHandler", (class wxEvtHandler * (wxEventFunctor::*)() const) &wxEventFunctor::GetEvtHandler, "C++: wxEventFunctor::GetEvtHandler() const --> class wxEvtHandler *", pybind11::return_value_policy::automatic);
		cl.def("GetEvtMethod", (void (class wxEvtHandler::*)(class wxEvent &) (wxEventFunctor::*)() const) &wxEventFunctor::GetEvtMethod, "C++: wxEventFunctor::GetEvtMethod() const --> void (class wxEvtHandler::*)(class wxEvent &)");
		cl.def("assign", (class wxEventFunctor & (wxEventFunctor::*)(const class wxEventFunctor &)) &wxEventFunctor::operator=, "C++: wxEventFunctor::operator=(const class wxEventFunctor &) --> class wxEventFunctor &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxObjectEventFunctor file: line:240
		pybind11::class_<wxObjectEventFunctor, std::shared_ptr<wxObjectEventFunctor>, PyCallBack_wxObjectEventFunctor, wxEventFunctor> cl(M(""), "wxObjectEventFunctor", "");
		cl.def( pybind11::init<void (class wxEvtHandler::*)(class wxEvent &), class wxEvtHandler *>(), pybind11::arg("method"), pybind11::arg("handler") );

		cl.def( pybind11::init( [](PyCallBack_wxObjectEventFunctor const &o){ return new PyCallBack_wxObjectEventFunctor(o); } ) );
		cl.def( pybind11::init( [](wxObjectEventFunctor const &o){ return new wxObjectEventFunctor(o); } ) );
		cl.def("__call__", (void (wxObjectEventFunctor::*)(class wxEvtHandler *, class wxEvent &)) &wxObjectEventFunctor::operator(), "C++: wxObjectEventFunctor::operator()(class wxEvtHandler *, class wxEvent &) --> void", pybind11::arg("handler"), pybind11::arg("event"));
		cl.def("IsMatching", (bool (wxObjectEventFunctor::*)(const class wxEventFunctor &) const) &wxObjectEventFunctor::IsMatching, "C++: wxObjectEventFunctor::IsMatching(const class wxEventFunctor &) const --> bool", pybind11::arg("functor"));
		cl.def("GetEvtHandler", (class wxEvtHandler * (wxObjectEventFunctor::*)() const) &wxObjectEventFunctor::GetEvtHandler, "C++: wxObjectEventFunctor::GetEvtHandler() const --> class wxEvtHandler *", pybind11::return_value_policy::automatic);
		cl.def("GetEvtMethod", (void (class wxEvtHandler::*)(class wxEvent &) (wxObjectEventFunctor::*)() const) &wxObjectEventFunctor::GetEvtMethod, "C++: wxObjectEventFunctor::GetEvtMethod() const --> void (class wxEvtHandler::*)(class wxEvent &)");
		cl.def("assign", (class wxObjectEventFunctor & (wxObjectEventFunctor::*)(const class wxObjectEventFunctor &)) &wxObjectEventFunctor::operator=, "C++: wxObjectEventFunctor::operator=(const class wxObjectEventFunctor &) --> class wxObjectEventFunctor &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// wxNewEventFunctor(const int &, void (class wxEvtHandler::*)(class wxEvent &), class wxEvtHandler *) file: line:287
	M("").def("wxNewEventFunctor", (class wxObjectEventFunctor * (*)(const int &, void (class wxEvtHandler::*)(class wxEvent &), class wxEvtHandler *)) &wxNewEventFunctor, "C++: wxNewEventFunctor(const int &, void (class wxEvtHandler::*)(class wxEvent &), class wxEvtHandler *) --> class wxObjectEventFunctor *", pybind11::return_value_policy::automatic, pybind11::arg(""), pybind11::arg("method"), pybind11::arg("handler"));

	// wxNewEventTableFunctor(const int &, void (class wxEvtHandler::*)(class wxEvent &)) file: line:296
	M("").def("wxNewEventTableFunctor", (class wxObjectEventFunctor * (*)(const int &, void (class wxEvtHandler::*)(class wxEvent &))) &wxNewEventTableFunctor, "C++: wxNewEventTableFunctor(const int &, void (class wxEvtHandler::*)(class wxEvent &)) --> class wxObjectEventFunctor *", pybind11::return_value_policy::automatic, pybind11::arg(""), pybind11::arg("method"));

	// wxMakeEventFunctor(const int &, void (class wxEvtHandler::*)(class wxEvent &), class wxEvtHandler *) file: line:303
	M("").def("wxMakeEventFunctor", (class wxObjectEventFunctor (*)(const int &, void (class wxEvtHandler::*)(class wxEvent &), class wxEvtHandler *)) &wxMakeEventFunctor, "C++: wxMakeEventFunctor(const int &, void (class wxEvtHandler::*)(class wxEvent &), class wxEvtHandler *) --> class wxObjectEventFunctor", pybind11::arg(""), pybind11::arg("method"), pybind11::arg("handler"));

}

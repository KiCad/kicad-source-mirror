#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/process.h> // wxProcessEvent

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxProcessEvent file:wx/process.h line:152
struct PyCallBack_wxProcessEvent : public wxProcessEvent {
	using wxProcessEvent::wxProcessEvent;

	class wxEvent * Clone() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcessEvent *>(this), "Clone");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxEvent *>::value) {
				static pybind11::detail::override_caster_t<class wxEvent *> caster;
				return pybind11::detail::cast_ref<class wxEvent *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxEvent *>(std::move(o));
		}
		return wxProcessEvent::Clone();
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcessEvent *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxProcessEvent::GetClassInfo();
	}
	enum wxEventCategory GetEventCategory() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcessEvent *>(this), "GetEventCategory");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxEventCategory>::value) {
				static pybind11::detail::override_caster_t<enum wxEventCategory> caster;
				return pybind11::detail::cast_ref<enum wxEventCategory>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxEventCategory>(std::move(o));
		}
		return wxEvent::GetEventCategory();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcessEvent *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcessEvent *>(this), "CloneRefData");
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

void bind_wx_process(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxProcessEvent file:wx/process.h line:152
		pybind11::class_<wxProcessEvent, std::shared_ptr<wxProcessEvent>, PyCallBack_wxProcessEvent, wxEvent> cl(M(""), "wxProcessEvent", "");
		cl.def( pybind11::init( [](){ return new wxProcessEvent(); }, [](){ return new PyCallBack_wxProcessEvent(); } ), "doc");
		cl.def( pybind11::init( [](int const & a0){ return new wxProcessEvent(a0); }, [](int const & a0){ return new PyCallBack_wxProcessEvent(a0); } ), "doc");
		cl.def( pybind11::init( [](int const & a0, int const & a1){ return new wxProcessEvent(a0, a1); }, [](int const & a0, int const & a1){ return new PyCallBack_wxProcessEvent(a0, a1); } ), "doc");
		cl.def( pybind11::init<int, int, int>(), pybind11::arg("nId"), pybind11::arg("pid"), pybind11::arg("exitcode") );

		cl.def( pybind11::init( [](PyCallBack_wxProcessEvent const &o){ return new PyCallBack_wxProcessEvent(o); } ) );
		cl.def( pybind11::init( [](wxProcessEvent const &o){ return new wxProcessEvent(o); } ) );
		cl.def_readwrite("m_pid", &wxProcessEvent::m_pid);
		cl.def_readwrite("m_exitcode", &wxProcessEvent::m_exitcode);
		cl.def("GetPid", (int (wxProcessEvent::*)()) &wxProcessEvent::GetPid, "C++: wxProcessEvent::GetPid() --> int");
		cl.def("GetExitCode", (int (wxProcessEvent::*)()) &wxProcessEvent::GetExitCode, "C++: wxProcessEvent::GetExitCode() --> int");
		cl.def("Clone", (class wxEvent * (wxProcessEvent::*)() const) &wxProcessEvent::Clone, "C++: wxProcessEvent::Clone() const --> class wxEvent *", pybind11::return_value_policy::automatic);
		cl.def("GetClassInfo", (class wxClassInfo * (wxProcessEvent::*)() const) &wxProcessEvent::GetClassInfo, "C++: wxProcessEvent::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxProcessEvent::wxCreateObject, "C++: wxProcessEvent::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
}

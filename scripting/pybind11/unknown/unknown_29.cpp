#include <bits/iscanonical.h> // __iscanonicall
#include <bits/iscanonical.h> // iscanonical
#include <math.h> // __iseqsig_type
#include <math.h> // issignaling
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

// wxStringList file: line:1244
struct PyCallBack_wxStringList : public wxStringList {
	using wxStringList::wxStringList;

	class wxStringListNode * Find(const class wxListKey & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStringList *>(this), "Find");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxStringListNode *>::value) {
				static pybind11::detail::override_caster_t<class wxStringListNode *> caster;
				return pybind11::detail::cast_ref<class wxStringListNode *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxStringListNode *>(std::move(o));
		}
		return wxStringListBase::Find(a0);
	}
	class wxNodeBase * CreateNode(class wxNodeBase * a0, class wxNodeBase * a1, void * a2, const class wxListKey & a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxStringList *>(this), "CreateNode");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxNodeBase *>::value) {
				static pybind11::detail::override_caster_t<class wxNodeBase *> caster;
				return pybind11::detail::cast_ref<class wxNodeBase *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxNodeBase *>(std::move(o));
		}
		return wxStringListBase::CreateNode(a0, a1, a2, a3);
	}
};

void bind_unknown_unknown_29(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxStringList file: line:1244
		pybind11::class_<wxStringList, std::shared_ptr<wxStringList>, PyCallBack_wxStringList, wxStringListBase> cl(M(""), "wxStringList", "");
		cl.def( pybind11::init( [](){ return new wxStringList(); }, [](){ return new PyCallBack_wxStringList(); } ) );
		cl.def( pybind11::init( [](const wchar_t * a0){ return new wxStringList(a0); }, [](const wchar_t * a0){ return new PyCallBack_wxStringList(a0); } ), "doc");
		cl.def( pybind11::init( [](PyCallBack_wxStringList const &o){ return new PyCallBack_wxStringList(o); } ) );
		cl.def( pybind11::init( [](wxStringList const &o){ return new wxStringList(o); } ) );
		cl.def("assign", (class wxStringList & (wxStringList::*)(const class wxStringList &)) &wxStringList::operator=, "C++: wxStringList::operator=(const class wxStringList &) --> class wxStringList &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		cl.def("Add", (class wxObjectListNode * (wxStringList::*)(const wchar_t *)) &wxStringList::Add, "C++: wxStringList::Add(const wchar_t *) --> class wxObjectListNode *", pybind11::return_value_policy::automatic, pybind11::arg("s"));
		cl.def("Prepend", (class wxObjectListNode * (wxStringList::*)(const wchar_t *)) &wxStringList::Prepend, "C++: wxStringList::Prepend(const wchar_t *) --> class wxObjectListNode *", pybind11::return_value_policy::automatic, pybind11::arg("s"));
		cl.def("Delete", (bool (wxStringList::*)(const wchar_t *)) &wxStringList::Delete, "C++: wxStringList::Delete(const wchar_t *) --> bool", pybind11::arg("s"));
		cl.def("Member", (bool (wxStringList::*)(const wchar_t *) const) &wxStringList::Member, "C++: wxStringList::Member(const wchar_t *) const --> bool", pybind11::arg("s"));
		cl.def("Sort", (void (wxStringList::*)()) &wxStringList::Sort, "C++: wxStringList::Sort() --> void");
	}
}

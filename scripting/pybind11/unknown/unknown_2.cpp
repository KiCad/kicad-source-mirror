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

void bind_unknown_unknown_2(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxPrivate::UntypedBufferData file: line:30
		pybind11::class_<wxPrivate::UntypedBufferData, std::shared_ptr<wxPrivate::UntypedBufferData>> cl(M("wxPrivate"), "UntypedBufferData", "");
		cl.def( pybind11::init( [](void * a0, unsigned long const & a1){ return new wxPrivate::UntypedBufferData(a0, a1); } ), "doc" , pybind11::arg("str"), pybind11::arg("len"));
		cl.def( pybind11::init<void *, unsigned long, enum wxPrivate::UntypedBufferData::Kind>(), pybind11::arg("str"), pybind11::arg("len"), pybind11::arg("kind") );


		pybind11::enum_<wxPrivate::UntypedBufferData::Kind>(cl, "Kind", pybind11::arithmetic(), "")
			.value("Owned", wxPrivate::UntypedBufferData::Owned)
			.value("NonOwned", wxPrivate::UntypedBufferData::NonOwned)
			.export_values();

		cl.def_readwrite("m_length", &wxPrivate::UntypedBufferData::m_length);
		cl.def_readwrite("m_ref", &wxPrivate::UntypedBufferData::m_ref);
		cl.def_readwrite("m_owned", &wxPrivate::UntypedBufferData::m_owned);
	}
	// wxPrivate::GetUntypedNullData() file: line:57
	M("wxPrivate").def("GetUntypedNullData", (struct wxPrivate::UntypedBufferData * (*)()) &wxPrivate::GetUntypedNullData, "C++: wxPrivate::GetUntypedNullData() --> struct wxPrivate::UntypedBufferData *", pybind11::return_value_policy::automatic);

}

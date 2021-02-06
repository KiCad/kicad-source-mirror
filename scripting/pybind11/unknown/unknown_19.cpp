#include <iterator> // __gnu_cxx::__normal_iterator
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

// wxMessageOutputBest file: line:133
struct PyCallBack_wxMessageOutputBest : public wxMessageOutputBest {
	using wxMessageOutputBest::wxMessageOutputBest;

	void Output(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMessageOutputBest *>(this), "Output");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxMessageOutputBest::Output(a0);
	}
	void DoPrintfWchar(const wchar_t * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMessageOutputBest *>(this), "DoPrintfWchar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxMessageOutput::DoPrintfWchar(a0);
	}
};

// wxMessageOutputMessageBox file: line:151
struct PyCallBack_wxMessageOutputMessageBox : public wxMessageOutputMessageBox {
	using wxMessageOutputMessageBox::wxMessageOutputMessageBox;

	void Output(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMessageOutputMessageBox *>(this), "Output");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxMessageOutputMessageBox::Output(a0);
	}
	void DoPrintfWchar(const wchar_t * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMessageOutputMessageBox *>(this), "DoPrintfWchar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxMessageOutput::DoPrintfWchar(a0);
	}
};

// wxMessageOutputDebug file: line:165
struct PyCallBack_wxMessageOutputDebug : public wxMessageOutputDebug {
	using wxMessageOutputDebug::wxMessageOutputDebug;

	void Output(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMessageOutputDebug *>(this), "Output");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxMessageOutputDebug::Output(a0);
	}
	void DoPrintfWchar(const wchar_t * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMessageOutputDebug *>(this), "DoPrintfWchar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxMessageOutput::DoPrintfWchar(a0);
	}
};

// wxMessageOutputLog file: line:177
struct PyCallBack_wxMessageOutputLog : public wxMessageOutputLog {
	using wxMessageOutputLog::wxMessageOutputLog;

	void Output(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMessageOutputLog *>(this), "Output");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxMessageOutputLog::Output(a0);
	}
	void DoPrintfWchar(const wchar_t * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMessageOutputLog *>(this), "DoPrintfWchar");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxMessageOutput::DoPrintfWchar(a0);
	}
};

// wxObject file: line:355
struct PyCallBack_wxObject : public wxObject {
	using wxObject::wxObject;

	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxObject *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxObject::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxObject *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxObject *>(this), "CloneRefData");
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

void bind_unknown_unknown_19(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxMessageOutputFlags file: line:127
	pybind11::enum_<wxMessageOutputFlags>(M(""), "wxMessageOutputFlags", pybind11::arithmetic(), "")
		.value("wxMSGOUT_PREFER_STDERR", wxMSGOUT_PREFER_STDERR)
		.value("wxMSGOUT_PREFER_MSGBOX", wxMSGOUT_PREFER_MSGBOX)
		.export_values();

;

	{ // wxMessageOutputBest file: line:133
		pybind11::class_<wxMessageOutputBest, std::shared_ptr<wxMessageOutputBest>, PyCallBack_wxMessageOutputBest, wxMessageOutputStderr> cl(M(""), "wxMessageOutputBest", "");
		cl.def( pybind11::init( [](){ return new wxMessageOutputBest(); }, [](){ return new PyCallBack_wxMessageOutputBest(); } ), "doc");
		cl.def( pybind11::init<enum wxMessageOutputFlags>(), pybind11::arg("flags") );

		cl.def("Output", (void (wxMessageOutputBest::*)(const class wxString &)) &wxMessageOutputBest::Output, "C++: wxMessageOutputBest::Output(const class wxString &) --> void", pybind11::arg("str"));
		cl.def("assign", (class wxMessageOutputBest & (wxMessageOutputBest::*)(const class wxMessageOutputBest &)) &wxMessageOutputBest::operator=, "C++: wxMessageOutputBest::operator=(const class wxMessageOutputBest &) --> class wxMessageOutputBest &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxMessageOutputMessageBox file: line:151
		pybind11::class_<wxMessageOutputMessageBox, std::shared_ptr<wxMessageOutputMessageBox>, PyCallBack_wxMessageOutputMessageBox, wxMessageOutput> cl(M(""), "wxMessageOutputMessageBox", "");
		cl.def( pybind11::init( [](){ return new wxMessageOutputMessageBox(); }, [](){ return new PyCallBack_wxMessageOutputMessageBox(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxMessageOutputMessageBox const &o){ return new PyCallBack_wxMessageOutputMessageBox(o); } ) );
		cl.def( pybind11::init( [](wxMessageOutputMessageBox const &o){ return new wxMessageOutputMessageBox(o); } ) );
		cl.def("Output", (void (wxMessageOutputMessageBox::*)(const class wxString &)) &wxMessageOutputMessageBox::Output, "C++: wxMessageOutputMessageBox::Output(const class wxString &) --> void", pybind11::arg("str"));
		cl.def("assign", (class wxMessageOutputMessageBox & (wxMessageOutputMessageBox::*)(const class wxMessageOutputMessageBox &)) &wxMessageOutputMessageBox::operator=, "C++: wxMessageOutputMessageBox::operator=(const class wxMessageOutputMessageBox &) --> class wxMessageOutputMessageBox &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxMessageOutputDebug file: line:165
		pybind11::class_<wxMessageOutputDebug, std::shared_ptr<wxMessageOutputDebug>, PyCallBack_wxMessageOutputDebug, wxMessageOutputStderr> cl(M(""), "wxMessageOutputDebug", "");
		cl.def( pybind11::init( [](){ return new wxMessageOutputDebug(); }, [](){ return new PyCallBack_wxMessageOutputDebug(); } ) );
		cl.def("Output", (void (wxMessageOutputDebug::*)(const class wxString &)) &wxMessageOutputDebug::Output, "C++: wxMessageOutputDebug::Output(const class wxString &) --> void", pybind11::arg("str"));
		cl.def("assign", (class wxMessageOutputDebug & (wxMessageOutputDebug::*)(const class wxMessageOutputDebug &)) &wxMessageOutputDebug::operator=, "C++: wxMessageOutputDebug::operator=(const class wxMessageOutputDebug &) --> class wxMessageOutputDebug &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxMessageOutputLog file: line:177
		pybind11::class_<wxMessageOutputLog, std::shared_ptr<wxMessageOutputLog>, PyCallBack_wxMessageOutputLog, wxMessageOutput> cl(M(""), "wxMessageOutputLog", "");
		cl.def( pybind11::init( [](){ return new wxMessageOutputLog(); }, [](){ return new PyCallBack_wxMessageOutputLog(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxMessageOutputLog const &o){ return new PyCallBack_wxMessageOutputLog(o); } ) );
		cl.def( pybind11::init( [](wxMessageOutputLog const &o){ return new wxMessageOutputLog(o); } ) );
		cl.def("Output", (void (wxMessageOutputLog::*)(const class wxString &)) &wxMessageOutputLog::Output, "C++: wxMessageOutputLog::Output(const class wxString &) --> void", pybind11::arg("str"));
		cl.def("assign", (class wxMessageOutputLog & (wxMessageOutputLog::*)(const class wxMessageOutputLog &)) &wxMessageOutputLog::operator=, "C++: wxMessageOutputLog::operator=(const class wxMessageOutputLog &) --> class wxMessageOutputLog &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxClassInfo file: line:41
		pybind11::class_<wxClassInfo, std::shared_ptr<wxClassInfo>> cl(M(""), "wxClassInfo", "");
		cl.def("CreateObject", (class wxObject * (wxClassInfo::*)() const) &wxClassInfo::CreateObject, "C++: wxClassInfo::CreateObject() const --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("IsDynamic", (bool (wxClassInfo::*)() const) &wxClassInfo::IsDynamic, "C++: wxClassInfo::IsDynamic() const --> bool");
		cl.def("GetClassName", (const wchar_t * (wxClassInfo::*)() const) &wxClassInfo::GetClassName, "C++: wxClassInfo::GetClassName() const --> const wchar_t *", pybind11::return_value_policy::automatic);
		cl.def("GetBaseClassName1", (const wchar_t * (wxClassInfo::*)() const) &wxClassInfo::GetBaseClassName1, "C++: wxClassInfo::GetBaseClassName1() const --> const wchar_t *", pybind11::return_value_policy::automatic);
		cl.def("GetBaseClassName2", (const wchar_t * (wxClassInfo::*)() const) &wxClassInfo::GetBaseClassName2, "C++: wxClassInfo::GetBaseClassName2() const --> const wchar_t *", pybind11::return_value_policy::automatic);
		cl.def("GetBaseClass1", (const class wxClassInfo * (wxClassInfo::*)() const) &wxClassInfo::GetBaseClass1, "C++: wxClassInfo::GetBaseClass1() const --> const class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def("GetBaseClass2", (const class wxClassInfo * (wxClassInfo::*)() const) &wxClassInfo::GetBaseClass2, "C++: wxClassInfo::GetBaseClass2() const --> const class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def("GetSize", (int (wxClassInfo::*)() const) &wxClassInfo::GetSize, "C++: wxClassInfo::GetSize() const --> int");
		cl.def_static("GetFirst", (const class wxClassInfo * (*)()) &wxClassInfo::GetFirst, "C++: wxClassInfo::GetFirst() --> const class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def("GetNext", (const class wxClassInfo * (wxClassInfo::*)() const) &wxClassInfo::GetNext, "C++: wxClassInfo::GetNext() const --> const class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("FindClass", (class wxClassInfo * (*)(const class wxString &)) &wxClassInfo::FindClass, "C++: wxClassInfo::FindClass(const class wxString &) --> class wxClassInfo *", pybind11::return_value_policy::automatic, pybind11::arg("className"));
		cl.def("IsKindOf", (bool (wxClassInfo::*)(const class wxClassInfo *) const) &wxClassInfo::IsKindOf, "C++: wxClassInfo::IsKindOf(const class wxClassInfo *) const --> bool", pybind11::arg("info"));
		cl.def_static("begin_classinfo", (class wxClassInfo::const_iterator (*)()) &wxClassInfo::begin_classinfo, "C++: wxClassInfo::begin_classinfo() --> class wxClassInfo::const_iterator");
		cl.def_static("end_classinfo", (class wxClassInfo::const_iterator (*)()) &wxClassInfo::end_classinfo, "C++: wxClassInfo::end_classinfo() --> class wxClassInfo::const_iterator");

		{ // wxClassInfo::const_iterator file: line:22
			auto & enclosing_class = cl;
			pybind11::class_<wxClassInfo::const_iterator, std::shared_ptr<wxClassInfo::const_iterator>> cl(enclosing_class, "const_iterator", "");
			cl.def( pybind11::init( [](){ return new wxClassInfo::const_iterator(); } ) );
			cl.def("__mul__", (const class wxClassInfo * (wxClassInfo::const_iterator::*)() const) &wxClassInfo::const_iterator::operator*, "C++: wxClassInfo::const_iterator::operator*() const --> const class wxClassInfo *", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (class wxClassInfo::const_iterator & (wxClassInfo::const_iterator::*)()) &wxClassInfo::const_iterator::operator++, "C++: wxClassInfo::const_iterator::operator++() --> class wxClassInfo::const_iterator &", pybind11::return_value_policy::automatic);
			cl.def("plus_plus", (const class wxClassInfo::const_iterator (wxClassInfo::const_iterator::*)(int)) &wxClassInfo::const_iterator::operator++, "C++: wxClassInfo::const_iterator::operator++(int) --> const class wxClassInfo::const_iterator", pybind11::arg(""));
			cl.def("__ne__", (bool (wxClassInfo::const_iterator::*)(const class wxClassInfo::const_iterator &) const) &wxClassInfo::const_iterator::operator!=, "C++: wxClassInfo::const_iterator::operator!=(const class wxClassInfo::const_iterator &) const --> bool", pybind11::arg("it"));
			cl.def("__eq__", (bool (wxClassInfo::const_iterator::*)(const class wxClassInfo::const_iterator &) const) &wxClassInfo::const_iterator::operator==, "C++: wxClassInfo::const_iterator::operator==(const class wxClassInfo::const_iterator &) const --> bool", pybind11::arg("it"));
		}

	}
	// wxCreateDynamicObject(const class wxString &) file: line:44
	M("").def("wxCreateDynamicObject", (class wxObject * (*)(const class wxString &)) &wxCreateDynamicObject, "C++: wxCreateDynamicObject(const class wxString &) --> class wxObject *", pybind11::return_value_policy::automatic, pybind11::arg("name"));

	{ // wxRefCounter file: line:241
		pybind11::class_<wxRefCounter, wxRefCounter*> cl(M(""), "wxRefCounter", "");
		cl.def( pybind11::init( [](){ return new wxRefCounter(); } ) );
		cl.def("GetRefCount", (int (wxRefCounter::*)() const) &wxRefCounter::GetRefCount, "C++: wxRefCounter::GetRefCount() const --> int");
		cl.def("IncRef", (void (wxRefCounter::*)()) &wxRefCounter::IncRef, "C++: wxRefCounter::IncRef() --> void");
		cl.def("DecRef", (void (wxRefCounter::*)()) &wxRefCounter::DecRef, "C++: wxRefCounter::DecRef() --> void");
	}
	{ // wxObject file: line:355
		pybind11::class_<wxObject, std::shared_ptr<wxObject>, PyCallBack_wxObject> cl(M(""), "wxObject", "");
		cl.def( pybind11::init( [](){ return new wxObject(); }, [](){ return new PyCallBack_wxObject(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxObject const &o){ return new PyCallBack_wxObject(o); } ) );
		cl.def( pybind11::init( [](wxObject const &o){ return new wxObject(o); } ) );
		cl.def("GetClassInfo", (class wxClassInfo * (wxObject::*)() const) &wxObject::GetClassInfo, "C++: wxObject::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxObject & (wxObject::*)(const class wxObject &)) &wxObject::operator=, "C++: wxObject::operator=(const class wxObject &) --> class wxObject &", pybind11::return_value_policy::automatic, pybind11::arg("other"));
		cl.def("IsKindOf", (bool (wxObject::*)(const class wxClassInfo *) const) &wxObject::IsKindOf, "C++: wxObject::IsKindOf(const class wxClassInfo *) const --> bool", pybind11::arg("info"));
		cl.def("GetRefData", (class wxRefCounter * (wxObject::*)() const) &wxObject::GetRefData, "C++: wxObject::GetRefData() const --> class wxRefCounter *", pybind11::return_value_policy::automatic);
		cl.def("SetRefData", (void (wxObject::*)(class wxRefCounter *)) &wxObject::SetRefData, "C++: wxObject::SetRefData(class wxRefCounter *) --> void", pybind11::arg("data"));
		cl.def("Ref", (void (wxObject::*)(const class wxObject &)) &wxObject::Ref, "C++: wxObject::Ref(const class wxObject &) --> void", pybind11::arg("clone"));
		cl.def("UnRef", (void (wxObject::*)()) &wxObject::UnRef, "C++: wxObject::UnRef() --> void");
		cl.def("UnShare", (void (wxObject::*)()) &wxObject::UnShare, "C++: wxObject::UnShare() --> void");
		cl.def("IsSameAs", (bool (wxObject::*)(const class wxObject &) const) &wxObject::IsSameAs, "C++: wxObject::IsSameAs(const class wxObject &) const --> bool", pybind11::arg("o"));
	}
	// wxCheckDynamicCast(class wxObject *, class wxClassInfo *) file: line:448
	M("").def("wxCheckDynamicCast", (class wxObject * (*)(class wxObject *, class wxClassInfo *)) &wxCheckDynamicCast, "C++: wxCheckDynamicCast(class wxObject *, class wxClassInfo *) --> class wxObject *", pybind11::return_value_policy::automatic, pybind11::arg("obj"), pybind11::arg("classInfo"));

	{ // _wxHashTable_NodeBase file: line:72
		pybind11::class_<_wxHashTable_NodeBase, std::shared_ptr<_wxHashTable_NodeBase>> cl(M(""), "_wxHashTable_NodeBase", "");
		cl.def( pybind11::init( [](){ return new _wxHashTable_NodeBase(); } ) );
		cl.def( pybind11::init( [](_wxHashTable_NodeBase const &o){ return new _wxHashTable_NodeBase(o); } ) );
	}
	{ // _wxHashTableBase2 file: line:84
		pybind11::class_<_wxHashTableBase2, std::shared_ptr<_wxHashTableBase2>> cl(M(""), "_wxHashTableBase2", "");
		cl.def( pybind11::init( [](){ return new _wxHashTableBase2(); } ) );
		cl.def( pybind11::init( [](_wxHashTableBase2 const &o){ return new _wxHashTableBase2(o); } ) );
	}
	// never_grow(unsigned long, unsigned long) file: line:465
	M("").def("never_grow", (bool (*)(unsigned long, unsigned long)) &never_grow, "C++: never_grow(unsigned long, unsigned long) --> bool", pybind11::arg(""), pybind11::arg(""));

	// never_shrink(unsigned long, unsigned long) file: line:466
	M("").def("never_shrink", (bool (*)(unsigned long, unsigned long)) &never_shrink, "C++: never_shrink(unsigned long, unsigned long) --> bool", pybind11::arg(""), pybind11::arg(""));

	// grow_lf70(unsigned long, unsigned long) file: line:467
	M("").def("grow_lf70", (bool (*)(unsigned long, unsigned long)) &grow_lf70, "C++: grow_lf70(unsigned long, unsigned long) --> bool", pybind11::arg("buckets"), pybind11::arg("items"));

	{ // wxIntegerHash file: line:530
		pybind11::class_<wxIntegerHash, std::shared_ptr<wxIntegerHash>> cl(M(""), "wxIntegerHash", "");
		cl.def( pybind11::init( [](){ return new wxIntegerHash(); } ) );
		cl.def( pybind11::init( [](wxIntegerHash const &o){ return new wxIntegerHash(o); } ) );
		cl.def("__call__", (unsigned long (wxIntegerHash::*)(long) const) &wxIntegerHash::operator(), "C++: wxIntegerHash::operator()(long) const --> unsigned long", pybind11::arg("x"));
		cl.def("__call__", (unsigned long (wxIntegerHash::*)(unsigned long) const) &wxIntegerHash::operator(), "C++: wxIntegerHash::operator()(unsigned long) const --> unsigned long", pybind11::arg("x"));
		cl.def("__call__", (unsigned long (wxIntegerHash::*)(int) const) &wxIntegerHash::operator(), "C++: wxIntegerHash::operator()(int) const --> unsigned long", pybind11::arg("x"));
		cl.def("__call__", (unsigned long (wxIntegerHash::*)(unsigned int) const) &wxIntegerHash::operator(), "C++: wxIntegerHash::operator()(unsigned int) const --> unsigned long", pybind11::arg("x"));
		cl.def("__call__", (unsigned long (wxIntegerHash::*)(short) const) &wxIntegerHash::operator(), "C++: wxIntegerHash::operator()(short) const --> unsigned long", pybind11::arg("x"));
		cl.def("__call__", (unsigned long (wxIntegerHash::*)(unsigned short) const) &wxIntegerHash::operator(), "C++: wxIntegerHash::operator()(unsigned short) const --> unsigned long", pybind11::arg("x"));
		cl.def("__call__", (unsigned long long (wxIntegerHash::*)(long long) const) &wxIntegerHash::operator(), "C++: wxIntegerHash::operator()(long long) const --> unsigned long long", pybind11::arg("x"));
		cl.def("__call__", (unsigned long long (wxIntegerHash::*)(unsigned long long) const) &wxIntegerHash::operator(), "C++: wxIntegerHash::operator()(unsigned long long) const --> unsigned long long", pybind11::arg("x"));
		cl.def("assign", (struct wxIntegerHash & (wxIntegerHash::*)(const struct wxIntegerHash &)) &wxIntegerHash::operator=, "C++: wxIntegerHash::operator=(const struct wxIntegerHash &) --> struct wxIntegerHash &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxIntegerEqual file: line:549
		pybind11::class_<wxIntegerEqual, std::shared_ptr<wxIntegerEqual>> cl(M(""), "wxIntegerEqual", "");
		cl.def( pybind11::init( [](){ return new wxIntegerEqual(); } ) );
		cl.def( pybind11::init( [](wxIntegerEqual const &o){ return new wxIntegerEqual(o); } ) );
		cl.def("__call__", (bool (wxIntegerEqual::*)(long, long) const) &wxIntegerEqual::operator(), "C++: wxIntegerEqual::operator()(long, long) const --> bool", pybind11::arg("a"), pybind11::arg("b"));
		cl.def("__call__", (bool (wxIntegerEqual::*)(unsigned long, unsigned long) const) &wxIntegerEqual::operator(), "C++: wxIntegerEqual::operator()(unsigned long, unsigned long) const --> bool", pybind11::arg("a"), pybind11::arg("b"));
		cl.def("__call__", (bool (wxIntegerEqual::*)(int, int) const) &wxIntegerEqual::operator(), "C++: wxIntegerEqual::operator()(int, int) const --> bool", pybind11::arg("a"), pybind11::arg("b"));
		cl.def("__call__", (bool (wxIntegerEqual::*)(unsigned int, unsigned int) const) &wxIntegerEqual::operator(), "C++: wxIntegerEqual::operator()(unsigned int, unsigned int) const --> bool", pybind11::arg("a"), pybind11::arg("b"));
		cl.def("__call__", (bool (wxIntegerEqual::*)(short, short) const) &wxIntegerEqual::operator(), "C++: wxIntegerEqual::operator()(short, short) const --> bool", pybind11::arg("a"), pybind11::arg("b"));
		cl.def("__call__", (bool (wxIntegerEqual::*)(unsigned short, unsigned short) const) &wxIntegerEqual::operator(), "C++: wxIntegerEqual::operator()(unsigned short, unsigned short) const --> bool", pybind11::arg("a"), pybind11::arg("b"));
		cl.def("__call__", (bool (wxIntegerEqual::*)(long long, long long) const) &wxIntegerEqual::operator(), "C++: wxIntegerEqual::operator()(long long, long long) const --> bool", pybind11::arg("a"), pybind11::arg("b"));
		cl.def("__call__", (bool (wxIntegerEqual::*)(unsigned long long, unsigned long long) const) &wxIntegerEqual::operator(), "C++: wxIntegerEqual::operator()(unsigned long long, unsigned long long) const --> bool", pybind11::arg("a"), pybind11::arg("b"));
		cl.def("assign", (struct wxIntegerEqual & (wxIntegerEqual::*)(const struct wxIntegerEqual &)) &wxIntegerEqual::operator=, "C++: wxIntegerEqual::operator=(const struct wxIntegerEqual &) --> struct wxIntegerEqual &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}

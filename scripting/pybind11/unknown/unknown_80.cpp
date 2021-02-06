#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/confbase.h> // 
#include <wx/confbase.h> // wxConfigBase

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxConfigBase file:wx/confbase.h line:83
struct PyCallBack_wxConfigBase : public wxConfigBase {
	using wxConfigBase::wxConfigBase;

	void SetPath(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "SetPath");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::SetPath\"");
	}
	const class wxString & GetPath() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "GetPath");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxString &>::value) {
				static pybind11::detail::override_caster_t<const class wxString &> caster;
				return pybind11::detail::cast_ref<const class wxString &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxString &>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::GetPath\"");
	}
	bool GetFirstGroup(class wxString & a0, long & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "GetFirstGroup");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::GetFirstGroup\"");
	}
	bool GetNextGroup(class wxString & a0, long & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "GetNextGroup");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::GetNextGroup\"");
	}
	bool GetFirstEntry(class wxString & a0, long & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "GetFirstEntry");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::GetFirstEntry\"");
	}
	bool GetNextEntry(class wxString & a0, long & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "GetNextEntry");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::GetNextEntry\"");
	}
	unsigned long GetNumberOfEntries(bool a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "GetNumberOfEntries");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::GetNumberOfEntries\"");
	}
	unsigned long GetNumberOfGroups(bool a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "GetNumberOfGroups");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::GetNumberOfGroups\"");
	}
	bool HasGroup(const class wxString & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "HasGroup");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::HasGroup\"");
	}
	bool HasEntry(const class wxString & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "HasEntry");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::HasEntry\"");
	}
	enum wxConfigBase::EntryType GetEntryType(const class wxString & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "GetEntryType");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxConfigBase::EntryType>::value) {
				static pybind11::detail::override_caster_t<enum wxConfigBase::EntryType> caster;
				return pybind11::detail::cast_ref<enum wxConfigBase::EntryType>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxConfigBase::EntryType>(std::move(o));
		}
		return wxConfigBase::GetEntryType(a0);
	}
	bool Flush(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "Flush");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::Flush\"");
	}
	bool RenameEntry(const class wxString & a0, const class wxString & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "RenameEntry");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::RenameEntry\"");
	}
	bool RenameGroup(const class wxString & a0, const class wxString & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "RenameGroup");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::RenameGroup\"");
	}
	bool DeleteEntry(const class wxString & a0, bool a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "DeleteEntry");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::DeleteEntry\"");
	}
	bool DeleteGroup(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "DeleteGroup");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::DeleteGroup\"");
	}
	bool DeleteAll() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "DeleteAll");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::DeleteAll\"");
	}
	bool DoReadString(const class wxString & a0, class wxString * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "DoReadString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::DoReadString\"");
	}
	bool DoReadLong(const class wxString & a0, long * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "DoReadLong");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::DoReadLong\"");
	}
	bool DoReadDouble(const class wxString & a0, double * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "DoReadDouble");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxConfigBase::DoReadDouble(a0, a1);
	}
	bool DoReadBool(const class wxString & a0, bool * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "DoReadBool");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxConfigBase::DoReadBool(a0, a1);
	}
	bool DoReadBinary(const class wxString & a0, class wxMemoryBuffer * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "DoReadBinary");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::DoReadBinary\"");
	}
	bool DoWriteString(const class wxString & a0, const class wxString & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "DoWriteString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::DoWriteString\"");
	}
	bool DoWriteLong(const class wxString & a0, long a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "DoWriteLong");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::DoWriteLong\"");
	}
	bool DoWriteDouble(const class wxString & a0, double a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "DoWriteDouble");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxConfigBase::DoWriteDouble(a0, a1);
	}
	bool DoWriteBool(const class wxString & a0, bool a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "DoWriteBool");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxConfigBase::DoWriteBool(a0, a1);
	}
	bool DoWriteBinary(const class wxString & a0, const class wxMemoryBuffer & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "DoWriteBinary");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxConfigBase::DoWriteBinary\"");
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxConfigBase::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxConfigBase *>(this), "CloneRefData");
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

void bind_unknown_unknown_80(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxBase64EncodedSize(unsigned long) file: line:23
	M("").def("wxBase64EncodedSize", (unsigned long (*)(unsigned long)) &wxBase64EncodedSize, "C++: wxBase64EncodedSize(unsigned long) --> unsigned long", pybind11::arg("len"));

	// wxBase64Encode(char *, unsigned long, const void *, unsigned long) file: line:33
	M("").def("wxBase64Encode", (unsigned long (*)(char *, unsigned long, const void *, unsigned long)) &wxBase64Encode, "C++: wxBase64Encode(char *, unsigned long, const void *, unsigned long) --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));

	// wxBase64Encode(const void *, unsigned long) file: line:37
	M("").def("wxBase64Encode", (class wxString (*)(const void *, unsigned long)) &wxBase64Encode, "C++: wxBase64Encode(const void *, unsigned long) --> class wxString", pybind11::arg("src"), pybind11::arg("srcLen"));

	// wxBase64Encode(const class wxMemoryBuffer &) file: line:46
	M("").def("wxBase64Encode", (class wxString (*)(const class wxMemoryBuffer &)) &wxBase64Encode, "C++: wxBase64Encode(const class wxMemoryBuffer &) --> class wxString", pybind11::arg("buf"));

	// wxBase64DecodeMode file: line:57
	pybind11::enum_<wxBase64DecodeMode>(M(""), "wxBase64DecodeMode", pybind11::arithmetic(), "")
		.value("wxBase64DecodeMode_Strict", wxBase64DecodeMode_Strict)
		.value("wxBase64DecodeMode_SkipWS", wxBase64DecodeMode_SkipWS)
		.value("wxBase64DecodeMode_Relaxed", wxBase64DecodeMode_Relaxed)
		.export_values();

;

	// wxBase64DecodedSize(unsigned long) file: line:71
	M("").def("wxBase64DecodedSize", (unsigned long (*)(unsigned long)) &wxBase64DecodedSize, "C++: wxBase64DecodedSize(unsigned long) --> unsigned long", pybind11::arg("srcLen"));

	// wxBase64Decode(void *, unsigned long, const char *, unsigned long, enum wxBase64DecodeMode, unsigned long *) file: line:86
	M("").def("wxBase64Decode", [](void * a0, unsigned long const & a1, const char * a2) -> unsigned long { return wxBase64Decode(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
	M("").def("wxBase64Decode", [](void * a0, unsigned long const & a1, const char * a2, unsigned long const & a3) -> unsigned long { return wxBase64Decode(a0, a1, a2, a3); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"));
	M("").def("wxBase64Decode", [](void * a0, unsigned long const & a1, const char * a2, unsigned long const & a3, enum wxBase64DecodeMode const & a4) -> unsigned long { return wxBase64Decode(a0, a1, a2, a3, a4); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"), pybind11::arg("mode"));
	M("").def("wxBase64Decode", (unsigned long (*)(void *, unsigned long, const char *, unsigned long, enum wxBase64DecodeMode, unsigned long *)) &wxBase64Decode, "C++: wxBase64Decode(void *, unsigned long, const char *, unsigned long, enum wxBase64DecodeMode, unsigned long *) --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("srcLen"), pybind11::arg("mode"), pybind11::arg("posErr"));

	// wxBase64Decode(void *, unsigned long, const class wxString &, enum wxBase64DecodeMode, unsigned long *) file: line:92
	M("").def("wxBase64Decode", [](void * a0, unsigned long const & a1, const class wxString & a2) -> unsigned long { return wxBase64Decode(a0, a1, a2); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"));
	M("").def("wxBase64Decode", [](void * a0, unsigned long const & a1, const class wxString & a2, enum wxBase64DecodeMode const & a3) -> unsigned long { return wxBase64Decode(a0, a1, a2, a3); }, "", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("mode"));
	M("").def("wxBase64Decode", (unsigned long (*)(void *, unsigned long, const class wxString &, enum wxBase64DecodeMode, unsigned long *)) &wxBase64Decode, "C++: wxBase64Decode(void *, unsigned long, const class wxString &, enum wxBase64DecodeMode, unsigned long *) --> unsigned long", pybind11::arg("dst"), pybind11::arg("dstLen"), pybind11::arg("src"), pybind11::arg("mode"), pybind11::arg("posErr"));

	// wxBase64Decode(const char *, unsigned long, enum wxBase64DecodeMode, unsigned long *) file: line:105
	M("").def("wxBase64Decode", [](const char * a0) -> wxMemoryBuffer { return wxBase64Decode(a0); }, "", pybind11::arg("src"));
	M("").def("wxBase64Decode", [](const char * a0, unsigned long const & a1) -> wxMemoryBuffer { return wxBase64Decode(a0, a1); }, "", pybind11::arg("src"), pybind11::arg("srcLen"));
	M("").def("wxBase64Decode", [](const char * a0, unsigned long const & a1, enum wxBase64DecodeMode const & a2) -> wxMemoryBuffer { return wxBase64Decode(a0, a1, a2); }, "", pybind11::arg("src"), pybind11::arg("srcLen"), pybind11::arg("mode"));
	M("").def("wxBase64Decode", (class wxMemoryBuffer (*)(const char *, unsigned long, enum wxBase64DecodeMode, unsigned long *)) &wxBase64Decode, "C++: wxBase64Decode(const char *, unsigned long, enum wxBase64DecodeMode, unsigned long *) --> class wxMemoryBuffer", pybind11::arg("src"), pybind11::arg("srcLen"), pybind11::arg("mode"), pybind11::arg("posErr"));

	// wxBase64Decode(const class wxString &, enum wxBase64DecodeMode, unsigned long *) file: line:110
	M("").def("wxBase64Decode", [](const class wxString & a0) -> wxMemoryBuffer { return wxBase64Decode(a0); }, "", pybind11::arg("src"));
	M("").def("wxBase64Decode", [](const class wxString & a0, enum wxBase64DecodeMode const & a1) -> wxMemoryBuffer { return wxBase64Decode(a0, a1); }, "", pybind11::arg("src"), pybind11::arg("mode"));
	M("").def("wxBase64Decode", (class wxMemoryBuffer (*)(const class wxString &, enum wxBase64DecodeMode, unsigned long *)) &wxBase64Decode, "C++: wxBase64Decode(const class wxString &, enum wxBase64DecodeMode, unsigned long *) --> class wxMemoryBuffer", pybind11::arg("src"), pybind11::arg("mode"), pybind11::arg("posErr"));

	{ // wxConfigBase file:wx/confbase.h line:83
		pybind11::class_<wxConfigBase, std::shared_ptr<wxConfigBase>, PyCallBack_wxConfigBase, wxObject> cl(M(""), "wxConfigBase", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxConfigBase(); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0){ return new PyCallBack_wxConfigBase(a0); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, const class wxString & a1){ return new PyCallBack_wxConfigBase(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, const class wxString & a1, const class wxString & a2){ return new PyCallBack_wxConfigBase(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, const class wxString & a1, const class wxString & a2, const class wxString & a3){ return new PyCallBack_wxConfigBase(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init<const class wxString &, const class wxString &, const class wxString &, const class wxString &, long>(), pybind11::arg("appName"), pybind11::arg("vendorName"), pybind11::arg("localFilename"), pybind11::arg("globalFilename"), pybind11::arg("style") );

		cl.def(pybind11::init<PyCallBack_wxConfigBase const &>());

		pybind11::enum_<wxConfigBase::EntryType>(cl, "EntryType", pybind11::arithmetic(), "")
			.value("Type_Unknown", wxConfigBase::Type_Unknown)
			.value("Type_String", wxConfigBase::Type_String)
			.value("Type_Boolean", wxConfigBase::Type_Boolean)
			.value("Type_Integer", wxConfigBase::Type_Integer)
			.value("Type_Float", wxConfigBase::Type_Float)
			.export_values();

		cl.def_static("Set", (class wxConfigBase * (*)(class wxConfigBase *)) &wxConfigBase::Set, "C++: wxConfigBase::Set(class wxConfigBase *) --> class wxConfigBase *", pybind11::return_value_policy::automatic, pybind11::arg("pConfig"));
		cl.def_static("Get", []() -> wxConfigBase * { return wxConfigBase::Get(); }, "", pybind11::return_value_policy::automatic);
		cl.def_static("Get", (class wxConfigBase * (*)(bool)) &wxConfigBase::Get, "C++: wxConfigBase::Get(bool) --> class wxConfigBase *", pybind11::return_value_policy::automatic, pybind11::arg("createOnDemand"));
		cl.def_static("Create", (class wxConfigBase * (*)()) &wxConfigBase::Create, "C++: wxConfigBase::Create() --> class wxConfigBase *", pybind11::return_value_policy::automatic);
		cl.def_static("DontCreateOnDemand", (void (*)()) &wxConfigBase::DontCreateOnDemand, "C++: wxConfigBase::DontCreateOnDemand() --> void");
		cl.def("SetPath", (void (wxConfigBase::*)(const class wxString &)) &wxConfigBase::SetPath, "C++: wxConfigBase::SetPath(const class wxString &) --> void", pybind11::arg("strPath"));
		cl.def("GetPath", (const class wxString & (wxConfigBase::*)() const) &wxConfigBase::GetPath, "C++: wxConfigBase::GetPath() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetFirstGroup", (bool (wxConfigBase::*)(class wxString &, long &) const) &wxConfigBase::GetFirstGroup, "C++: wxConfigBase::GetFirstGroup(class wxString &, long &) const --> bool", pybind11::arg("str"), pybind11::arg("lIndex"));
		cl.def("GetNextGroup", (bool (wxConfigBase::*)(class wxString &, long &) const) &wxConfigBase::GetNextGroup, "C++: wxConfigBase::GetNextGroup(class wxString &, long &) const --> bool", pybind11::arg("str"), pybind11::arg("lIndex"));
		cl.def("GetFirstEntry", (bool (wxConfigBase::*)(class wxString &, long &) const) &wxConfigBase::GetFirstEntry, "C++: wxConfigBase::GetFirstEntry(class wxString &, long &) const --> bool", pybind11::arg("str"), pybind11::arg("lIndex"));
		cl.def("GetNextEntry", (bool (wxConfigBase::*)(class wxString &, long &) const) &wxConfigBase::GetNextEntry, "C++: wxConfigBase::GetNextEntry(class wxString &, long &) const --> bool", pybind11::arg("str"), pybind11::arg("lIndex"));
		cl.def("GetNumberOfEntries", [](wxConfigBase const &o) -> unsigned long { return o.GetNumberOfEntries(); }, "");
		cl.def("GetNumberOfEntries", (unsigned long (wxConfigBase::*)(bool) const) &wxConfigBase::GetNumberOfEntries, "C++: wxConfigBase::GetNumberOfEntries(bool) const --> unsigned long", pybind11::arg("bRecursive"));
		cl.def("GetNumberOfGroups", [](wxConfigBase const &o) -> unsigned long { return o.GetNumberOfGroups(); }, "");
		cl.def("GetNumberOfGroups", (unsigned long (wxConfigBase::*)(bool) const) &wxConfigBase::GetNumberOfGroups, "C++: wxConfigBase::GetNumberOfGroups(bool) const --> unsigned long", pybind11::arg("bRecursive"));
		cl.def("HasGroup", (bool (wxConfigBase::*)(const class wxString &) const) &wxConfigBase::HasGroup, "C++: wxConfigBase::HasGroup(const class wxString &) const --> bool", pybind11::arg("strName"));
		cl.def("HasEntry", (bool (wxConfigBase::*)(const class wxString &) const) &wxConfigBase::HasEntry, "C++: wxConfigBase::HasEntry(const class wxString &) const --> bool", pybind11::arg("strName"));
		cl.def("Exists", (bool (wxConfigBase::*)(const class wxString &) const) &wxConfigBase::Exists, "C++: wxConfigBase::Exists(const class wxString &) const --> bool", pybind11::arg("strName"));
		cl.def("GetEntryType", (enum wxConfigBase::EntryType (wxConfigBase::*)(const class wxString &) const) &wxConfigBase::GetEntryType, "C++: wxConfigBase::GetEntryType(const class wxString &) const --> enum wxConfigBase::EntryType", pybind11::arg("name"));
		cl.def("Read", (bool (wxConfigBase::*)(const class wxString &, class wxString *) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, class wxString *) const --> bool", pybind11::arg("key"), pybind11::arg("pStr"));
		cl.def("Read", (bool (wxConfigBase::*)(const class wxString &, class wxString *, const class wxString &) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, class wxString *, const class wxString &) const --> bool", pybind11::arg("key"), pybind11::arg("pStr"), pybind11::arg("defVal"));
		cl.def("Read", (bool (wxConfigBase::*)(const class wxString &, long *) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, long *) const --> bool", pybind11::arg("key"), pybind11::arg("pl"));
		cl.def("Read", (bool (wxConfigBase::*)(const class wxString &, long *, long) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, long *, long) const --> bool", pybind11::arg("key"), pybind11::arg("pl"), pybind11::arg("defVal"));
		cl.def("Read", (bool (wxConfigBase::*)(const class wxString &, int *) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, int *) const --> bool", pybind11::arg("key"), pybind11::arg("pi"));
		cl.def("Read", (bool (wxConfigBase::*)(const class wxString &, int *, int) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, int *, int) const --> bool", pybind11::arg("key"), pybind11::arg("pi"), pybind11::arg("defVal"));
		cl.def("Read", (bool (wxConfigBase::*)(const class wxString &, double *) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, double *) const --> bool", pybind11::arg("key"), pybind11::arg("val"));
		cl.def("Read", (bool (wxConfigBase::*)(const class wxString &, double *, double) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, double *, double) const --> bool", pybind11::arg("key"), pybind11::arg("val"), pybind11::arg("defVal"));
		cl.def("Read", (bool (wxConfigBase::*)(const class wxString &, float *) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, float *) const --> bool", pybind11::arg("key"), pybind11::arg("val"));
		cl.def("Read", (bool (wxConfigBase::*)(const class wxString &, float *, float) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, float *, float) const --> bool", pybind11::arg("key"), pybind11::arg("val"), pybind11::arg("defVal"));
		cl.def("Read", (bool (wxConfigBase::*)(const class wxString &, bool *) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, bool *) const --> bool", pybind11::arg("key"), pybind11::arg("val"));
		cl.def("Read", (bool (wxConfigBase::*)(const class wxString &, bool *, bool) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, bool *, bool) const --> bool", pybind11::arg("key"), pybind11::arg("val"), pybind11::arg("defVal"));
		cl.def("Read", (bool (wxConfigBase::*)(const class wxString &, class wxMemoryBuffer *) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, class wxMemoryBuffer *) const --> bool", pybind11::arg("key"), pybind11::arg("data"));
		cl.def("Read", [](wxConfigBase const &o, const class wxString & a0) -> wxString { return o.Read(a0); }, "", pybind11::arg("key"));
		cl.def("Read", (class wxString (wxConfigBase::*)(const class wxString &, const class wxString &) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, const class wxString &) const --> class wxString", pybind11::arg("key"), pybind11::arg("defVal"));
		cl.def("Read", (class wxString (wxConfigBase::*)(const class wxString &, const char *) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, const char *) const --> class wxString", pybind11::arg("key"), pybind11::arg("defVal"));
		cl.def("Read", (class wxString (wxConfigBase::*)(const class wxString &, const wchar_t *) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, const wchar_t *) const --> class wxString", pybind11::arg("key"), pybind11::arg("defVal"));
		cl.def("ReadLong", (long (wxConfigBase::*)(const class wxString &, long) const) &wxConfigBase::ReadLong, "C++: wxConfigBase::ReadLong(const class wxString &, long) const --> long", pybind11::arg("key"), pybind11::arg("defVal"));
		cl.def("ReadDouble", (double (wxConfigBase::*)(const class wxString &, double) const) &wxConfigBase::ReadDouble, "C++: wxConfigBase::ReadDouble(const class wxString &, double) const --> double", pybind11::arg("key"), pybind11::arg("defVal"));
		cl.def("ReadBool", (bool (wxConfigBase::*)(const class wxString &, bool) const) &wxConfigBase::ReadBool, "C++: wxConfigBase::ReadBool(const class wxString &, bool) const --> bool", pybind11::arg("key"), pybind11::arg("defVal"));
		cl.def("Read", (long (wxConfigBase::*)(const class wxString &, long) const) &wxConfigBase::Read, "C++: wxConfigBase::Read(const class wxString &, long) const --> long", pybind11::arg("key"), pybind11::arg("defVal"));
		cl.def("Write", (bool (wxConfigBase::*)(const class wxString &, const class wxString &)) &wxConfigBase::Write, "C++: wxConfigBase::Write(const class wxString &, const class wxString &) --> bool", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("Write", (bool (wxConfigBase::*)(const class wxString &, long)) &wxConfigBase::Write, "C++: wxConfigBase::Write(const class wxString &, long) --> bool", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("Write", (bool (wxConfigBase::*)(const class wxString &, double)) &wxConfigBase::Write, "C++: wxConfigBase::Write(const class wxString &, double) --> bool", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("Write", (bool (wxConfigBase::*)(const class wxString &, bool)) &wxConfigBase::Write, "C++: wxConfigBase::Write(const class wxString &, bool) --> bool", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("Write", (bool (wxConfigBase::*)(const class wxString &, const class wxMemoryBuffer &)) &wxConfigBase::Write, "C++: wxConfigBase::Write(const class wxString &, const class wxMemoryBuffer &) --> bool", pybind11::arg("key"), pybind11::arg("buf"));
		cl.def("Write", (bool (wxConfigBase::*)(const class wxString &, const char *)) &wxConfigBase::Write, "C++: wxConfigBase::Write(const class wxString &, const char *) --> bool", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("Write", (bool (wxConfigBase::*)(const class wxString &, const unsigned char *)) &wxConfigBase::Write, "C++: wxConfigBase::Write(const class wxString &, const unsigned char *) --> bool", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("Write", (bool (wxConfigBase::*)(const class wxString &, const wchar_t *)) &wxConfigBase::Write, "C++: wxConfigBase::Write(const class wxString &, const wchar_t *) --> bool", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("Write", (bool (wxConfigBase::*)(const class wxString &, char)) &wxConfigBase::Write, "C++: wxConfigBase::Write(const class wxString &, char) --> bool", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("Write", (bool (wxConfigBase::*)(const class wxString &, unsigned char)) &wxConfigBase::Write, "C++: wxConfigBase::Write(const class wxString &, unsigned char) --> bool", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("Write", (bool (wxConfigBase::*)(const class wxString &, short)) &wxConfigBase::Write, "C++: wxConfigBase::Write(const class wxString &, short) --> bool", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("Write", (bool (wxConfigBase::*)(const class wxString &, unsigned short)) &wxConfigBase::Write, "C++: wxConfigBase::Write(const class wxString &, unsigned short) --> bool", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("Write", (bool (wxConfigBase::*)(const class wxString &, unsigned int)) &wxConfigBase::Write, "C++: wxConfigBase::Write(const class wxString &, unsigned int) --> bool", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("Write", (bool (wxConfigBase::*)(const class wxString &, int)) &wxConfigBase::Write, "C++: wxConfigBase::Write(const class wxString &, int) --> bool", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("Write", (bool (wxConfigBase::*)(const class wxString &, unsigned long)) &wxConfigBase::Write, "C++: wxConfigBase::Write(const class wxString &, unsigned long) --> bool", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("Write", (bool (wxConfigBase::*)(const class wxString &, float)) &wxConfigBase::Write, "C++: wxConfigBase::Write(const class wxString &, float) --> bool", pybind11::arg("key"), pybind11::arg("value"));
		cl.def("Flush", [](wxConfigBase &o) -> bool { return o.Flush(); }, "");
		cl.def("Flush", (bool (wxConfigBase::*)(bool)) &wxConfigBase::Flush, "C++: wxConfigBase::Flush(bool) --> bool", pybind11::arg("bCurrentOnly"));
		cl.def("RenameEntry", (bool (wxConfigBase::*)(const class wxString &, const class wxString &)) &wxConfigBase::RenameEntry, "C++: wxConfigBase::RenameEntry(const class wxString &, const class wxString &) --> bool", pybind11::arg("oldName"), pybind11::arg("newName"));
		cl.def("RenameGroup", (bool (wxConfigBase::*)(const class wxString &, const class wxString &)) &wxConfigBase::RenameGroup, "C++: wxConfigBase::RenameGroup(const class wxString &, const class wxString &) --> bool", pybind11::arg("oldName"), pybind11::arg("newName"));
		cl.def("DeleteEntry", [](wxConfigBase &o, const class wxString & a0) -> bool { return o.DeleteEntry(a0); }, "", pybind11::arg("key"));
		cl.def("DeleteEntry", (bool (wxConfigBase::*)(const class wxString &, bool)) &wxConfigBase::DeleteEntry, "C++: wxConfigBase::DeleteEntry(const class wxString &, bool) --> bool", pybind11::arg("key"), pybind11::arg("bDeleteGroupIfEmpty"));
		cl.def("DeleteGroup", (bool (wxConfigBase::*)(const class wxString &)) &wxConfigBase::DeleteGroup, "C++: wxConfigBase::DeleteGroup(const class wxString &) --> bool", pybind11::arg("key"));
		cl.def("DeleteAll", (bool (wxConfigBase::*)()) &wxConfigBase::DeleteAll, "C++: wxConfigBase::DeleteAll() --> bool");
		cl.def("IsExpandingEnvVars", (bool (wxConfigBase::*)() const) &wxConfigBase::IsExpandingEnvVars, "C++: wxConfigBase::IsExpandingEnvVars() const --> bool");
		cl.def("SetExpandEnvVars", [](wxConfigBase &o) -> void { return o.SetExpandEnvVars(); }, "");
		cl.def("SetExpandEnvVars", (void (wxConfigBase::*)(bool)) &wxConfigBase::SetExpandEnvVars, "C++: wxConfigBase::SetExpandEnvVars(bool) --> void", pybind11::arg("bDoIt"));
		cl.def("SetRecordDefaults", [](wxConfigBase &o) -> void { return o.SetRecordDefaults(); }, "");
		cl.def("SetRecordDefaults", (void (wxConfigBase::*)(bool)) &wxConfigBase::SetRecordDefaults, "C++: wxConfigBase::SetRecordDefaults(bool) --> void", pybind11::arg("bDoIt"));
		cl.def("IsRecordingDefaults", (bool (wxConfigBase::*)() const) &wxConfigBase::IsRecordingDefaults, "C++: wxConfigBase::IsRecordingDefaults() const --> bool");
		cl.def("ExpandEnvVars", (class wxString (wxConfigBase::*)(const class wxString &) const) &wxConfigBase::ExpandEnvVars, "C++: wxConfigBase::ExpandEnvVars(const class wxString &) const --> class wxString", pybind11::arg("str"));
		cl.def("GetAppName", (class wxString (wxConfigBase::*)() const) &wxConfigBase::GetAppName, "C++: wxConfigBase::GetAppName() const --> class wxString");
		cl.def("GetVendorName", (class wxString (wxConfigBase::*)() const) &wxConfigBase::GetVendorName, "C++: wxConfigBase::GetVendorName() const --> class wxString");
		cl.def("SetAppName", (void (wxConfigBase::*)(const class wxString &)) &wxConfigBase::SetAppName, "C++: wxConfigBase::SetAppName(const class wxString &) --> void", pybind11::arg("appName"));
		cl.def("SetVendorName", (void (wxConfigBase::*)(const class wxString &)) &wxConfigBase::SetVendorName, "C++: wxConfigBase::SetVendorName(const class wxString &) --> void", pybind11::arg("vendorName"));
		cl.def("SetStyle", (void (wxConfigBase::*)(long)) &wxConfigBase::SetStyle, "C++: wxConfigBase::SetStyle(long) --> void", pybind11::arg("style"));
		cl.def("GetStyle", (long (wxConfigBase::*)() const) &wxConfigBase::GetStyle, "C++: wxConfigBase::GetStyle() const --> long");
		cl.def("GetClassInfo", (class wxClassInfo * (wxConfigBase::*)() const) &wxConfigBase::GetClassInfo, "C++: wxConfigBase::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxConfigBase & (wxConfigBase::*)(const class wxConfigBase &)) &wxConfigBase::operator=, "C++: wxConfigBase::operator=(const class wxConfigBase &) --> class wxConfigBase &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}

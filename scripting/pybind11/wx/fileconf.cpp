#include <iterator> // __gnu_cxx::__normal_iterator
#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/confbase.h> // 
#include <wx/confbase.h> // wxConfigBase
#include <wx/dir.h> // wxDir
#include <wx/dir.h> // wxDirFlags
#include <wx/dir.h> // wxDirTraverseResult
#include <wx/dir.h> // wxDirTraverser
#include <wx/fileconf.h> // wxFileConfig
#include <wx/process.h> // wxProcess

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxFileConfig file:wx/fileconf.h line:101
struct PyCallBack_wxFileConfig : public wxFileConfig {
	using wxFileConfig::wxFileConfig;

	void SetPath(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "SetPath");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxFileConfig::SetPath(a0);
	}
	const class wxString & GetPath() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "GetPath");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<const class wxString &>::value) {
				static pybind11::detail::override_caster_t<const class wxString &> caster;
				return pybind11::detail::cast_ref<const class wxString &>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<const class wxString &>(std::move(o));
		}
		return wxFileConfig::GetPath();
	}
	bool GetFirstGroup(class wxString & a0, long & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "GetFirstGroup");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::GetFirstGroup(a0, a1);
	}
	bool GetNextGroup(class wxString & a0, long & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "GetNextGroup");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::GetNextGroup(a0, a1);
	}
	bool GetFirstEntry(class wxString & a0, long & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "GetFirstEntry");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::GetFirstEntry(a0, a1);
	}
	bool GetNextEntry(class wxString & a0, long & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "GetNextEntry");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::GetNextEntry(a0, a1);
	}
	unsigned long GetNumberOfEntries(bool a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "GetNumberOfEntries");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxFileConfig::GetNumberOfEntries(a0);
	}
	unsigned long GetNumberOfGroups(bool a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "GetNumberOfGroups");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<unsigned long>::value) {
				static pybind11::detail::override_caster_t<unsigned long> caster;
				return pybind11::detail::cast_ref<unsigned long>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<unsigned long>(std::move(o));
		}
		return wxFileConfig::GetNumberOfGroups(a0);
	}
	bool HasGroup(const class wxString & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "HasGroup");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::HasGroup(a0);
	}
	bool HasEntry(const class wxString & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "HasEntry");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::HasEntry(a0);
	}
	bool Flush(bool a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "Flush");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::Flush(a0);
	}
	bool RenameEntry(const class wxString & a0, const class wxString & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "RenameEntry");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::RenameEntry(a0, a1);
	}
	bool RenameGroup(const class wxString & a0, const class wxString & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "RenameGroup");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::RenameGroup(a0, a1);
	}
	bool DeleteEntry(const class wxString & a0, bool a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "DeleteEntry");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::DeleteEntry(a0, a1);
	}
	bool DeleteGroup(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "DeleteGroup");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::DeleteGroup(a0);
	}
	bool DeleteAll() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "DeleteAll");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::DeleteAll();
	}
	bool Save(class wxOutputStream & a0, const class wxMBConv & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "Save");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::Save(a0, a1);
	}
	bool DoReadString(const class wxString & a0, class wxString * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "DoReadString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::DoReadString(a0, a1);
	}
	bool DoReadLong(const class wxString & a0, long * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "DoReadLong");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::DoReadLong(a0, a1);
	}
	bool DoReadBinary(const class wxString & a0, class wxMemoryBuffer * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "DoReadBinary");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::DoReadBinary(a0, a1);
	}
	bool DoWriteString(const class wxString & a0, const class wxString & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "DoWriteString");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::DoWriteString(a0, a1);
	}
	bool DoWriteLong(const class wxString & a0, long a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "DoWriteLong");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::DoWriteLong(a0, a1);
	}
	bool DoWriteBinary(const class wxString & a0, const class wxMemoryBuffer & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "DoWriteBinary");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxFileConfig::DoWriteBinary(a0, a1);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxFileConfig::GetClassInfo();
	}
	enum wxConfigBase::EntryType GetEntryType(const class wxString & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "GetEntryType");
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
	bool DoReadDouble(const class wxString & a0, double * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "DoReadDouble");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "DoReadBool");
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
	bool DoWriteDouble(const class wxString & a0, double a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "DoWriteDouble");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "DoWriteBool");
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
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxFileConfig *>(this), "CloneRefData");
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

// wxDirTraverser file:wx/dir.h line:52
struct PyCallBack_wxDirTraverser : public wxDirTraverser {
	using wxDirTraverser::wxDirTraverser;

	enum wxDirTraverseResult OnFile(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDirTraverser *>(this), "OnFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxDirTraverseResult>::value) {
				static pybind11::detail::override_caster_t<enum wxDirTraverseResult> caster;
				return pybind11::detail::cast_ref<enum wxDirTraverseResult>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxDirTraverseResult>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxDirTraverser::OnFile\"");
	}
	enum wxDirTraverseResult OnDir(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDirTraverser *>(this), "OnDir");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxDirTraverseResult>::value) {
				static pybind11::detail::override_caster_t<enum wxDirTraverseResult> caster;
				return pybind11::detail::cast_ref<enum wxDirTraverseResult>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxDirTraverseResult>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxDirTraverser::OnDir\"");
	}
	enum wxDirTraverseResult OnOpenError(const class wxString & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxDirTraverser *>(this), "OnOpenError");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<enum wxDirTraverseResult>::value) {
				static pybind11::detail::override_caster_t<enum wxDirTraverseResult> caster;
				return pybind11::detail::cast_ref<enum wxDirTraverseResult>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<enum wxDirTraverseResult>(std::move(o));
		}
		return wxDirTraverser::OnOpenError(a0);
	}
};

// wxProcess file:wx/process.h line:37
struct PyCallBack_wxProcess : public wxProcess {
	using wxProcess::wxProcess;

	void OnTerminate(int a0, int a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "OnTerminate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxProcess::OnTerminate(a0, a1);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxProcess::GetClassInfo();
	}
	void SetNextHandler(class wxEvtHandler * a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "SetNextHandler");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "SetPreviousHandler");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "ProcessEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "QueueEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "AddPendingEvent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "SearchEventTable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "TryBefore");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "TryAfter");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "TryValidator");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "TryParent");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "GetEventTable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "GetEventHashTable");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "DoSetClientObject");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "DoGetClientObject");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "DoSetClientData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "DoGetClientData");
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
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxProcess *>(this), "CloneRefData");
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

void bind_wx_fileconf(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxFileConfig file:wx/fileconf.h line:101
		pybind11::class_<wxFileConfig, std::shared_ptr<wxFileConfig>, PyCallBack_wxFileConfig, wxConfigBase> cl(M(""), "wxFileConfig", "");
		cl.def( pybind11::init( [](){ return new wxFileConfig(); }, [](){ return new PyCallBack_wxFileConfig(); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0){ return new wxFileConfig(a0); }, [](const class wxString & a0){ return new PyCallBack_wxFileConfig(a0); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, const class wxString & a1){ return new wxFileConfig(a0, a1); }, [](const class wxString & a0, const class wxString & a1){ return new PyCallBack_wxFileConfig(a0, a1); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, const class wxString & a1, const class wxString & a2){ return new wxFileConfig(a0, a1, a2); }, [](const class wxString & a0, const class wxString & a1, const class wxString & a2){ return new PyCallBack_wxFileConfig(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, const class wxString & a1, const class wxString & a2, const class wxString & a3){ return new wxFileConfig(a0, a1, a2, a3); }, [](const class wxString & a0, const class wxString & a1, const class wxString & a2, const class wxString & a3){ return new PyCallBack_wxFileConfig(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, const class wxString & a1, const class wxString & a2, const class wxString & a3, long const & a4){ return new wxFileConfig(a0, a1, a2, a3, a4); }, [](const class wxString & a0, const class wxString & a1, const class wxString & a2, const class wxString & a3, long const & a4){ return new PyCallBack_wxFileConfig(a0, a1, a2, a3, a4); } ), "doc");
		cl.def( pybind11::init<const class wxString &, const class wxString &, const class wxString &, const class wxString &, long, const class wxMBConv &>(), pybind11::arg("appName"), pybind11::arg("vendorName"), pybind11::arg("localFilename"), pybind11::arg("globalFilename"), pybind11::arg("style"), pybind11::arg("conv") );

		cl.def( pybind11::init( [](class wxInputStream & a0){ return new wxFileConfig(a0); }, [](class wxInputStream & a0){ return new PyCallBack_wxFileConfig(a0); } ), "doc");
		cl.def( pybind11::init<class wxInputStream &, const class wxMBConv &>(), pybind11::arg("inStream"), pybind11::arg("conv") );

		cl.def_static("GetGlobalFile", (class wxFileName (*)(const class wxString &)) &wxFileConfig::GetGlobalFile, "C++: wxFileConfig::GetGlobalFile(const class wxString &) --> class wxFileName", pybind11::arg("szFile"));
		cl.def_static("GetLocalFile", [](const class wxString & a0) -> wxFileName { return wxFileConfig::GetLocalFile(a0); }, "", pybind11::arg("szFile"));
		cl.def_static("GetLocalFile", (class wxFileName (*)(const class wxString &, int)) &wxFileConfig::GetLocalFile, "C++: wxFileConfig::GetLocalFile(const class wxString &, int) --> class wxFileName", pybind11::arg("szFile"), pybind11::arg("style"));
		cl.def_static("GetGlobalFileName", (class wxString (*)(const class wxString &)) &wxFileConfig::GetGlobalFileName, "C++: wxFileConfig::GetGlobalFileName(const class wxString &) --> class wxString", pybind11::arg("szFile"));
		cl.def_static("GetLocalFileName", [](const class wxString & a0) -> wxString { return wxFileConfig::GetLocalFileName(a0); }, "", pybind11::arg("szFile"));
		cl.def_static("GetLocalFileName", (class wxString (*)(const class wxString &, int)) &wxFileConfig::GetLocalFileName, "C++: wxFileConfig::GetLocalFileName(const class wxString &, int) --> class wxString", pybind11::arg("szFile"), pybind11::arg("style"));
		cl.def("SetUmask", (void (wxFileConfig::*)(int)) &wxFileConfig::SetUmask, "C++: wxFileConfig::SetUmask(int) --> void", pybind11::arg("mode"));
		cl.def("SetPath", (void (wxFileConfig::*)(const class wxString &)) &wxFileConfig::SetPath, "C++: wxFileConfig::SetPath(const class wxString &) --> void", pybind11::arg("strPath"));
		cl.def("GetPath", (const class wxString & (wxFileConfig::*)() const) &wxFileConfig::GetPath, "C++: wxFileConfig::GetPath() const --> const class wxString &", pybind11::return_value_policy::automatic);
		cl.def("GetFirstGroup", (bool (wxFileConfig::*)(class wxString &, long &) const) &wxFileConfig::GetFirstGroup, "C++: wxFileConfig::GetFirstGroup(class wxString &, long &) const --> bool", pybind11::arg("str"), pybind11::arg("lIndex"));
		cl.def("GetNextGroup", (bool (wxFileConfig::*)(class wxString &, long &) const) &wxFileConfig::GetNextGroup, "C++: wxFileConfig::GetNextGroup(class wxString &, long &) const --> bool", pybind11::arg("str"), pybind11::arg("lIndex"));
		cl.def("GetFirstEntry", (bool (wxFileConfig::*)(class wxString &, long &) const) &wxFileConfig::GetFirstEntry, "C++: wxFileConfig::GetFirstEntry(class wxString &, long &) const --> bool", pybind11::arg("str"), pybind11::arg("lIndex"));
		cl.def("GetNextEntry", (bool (wxFileConfig::*)(class wxString &, long &) const) &wxFileConfig::GetNextEntry, "C++: wxFileConfig::GetNextEntry(class wxString &, long &) const --> bool", pybind11::arg("str"), pybind11::arg("lIndex"));
		cl.def("GetNumberOfEntries", [](wxFileConfig const &o) -> unsigned long { return o.GetNumberOfEntries(); }, "");
		cl.def("GetNumberOfEntries", (unsigned long (wxFileConfig::*)(bool) const) &wxFileConfig::GetNumberOfEntries, "C++: wxFileConfig::GetNumberOfEntries(bool) const --> unsigned long", pybind11::arg("bRecursive"));
		cl.def("GetNumberOfGroups", [](wxFileConfig const &o) -> unsigned long { return o.GetNumberOfGroups(); }, "");
		cl.def("GetNumberOfGroups", (unsigned long (wxFileConfig::*)(bool) const) &wxFileConfig::GetNumberOfGroups, "C++: wxFileConfig::GetNumberOfGroups(bool) const --> unsigned long", pybind11::arg("bRecursive"));
		cl.def("HasGroup", (bool (wxFileConfig::*)(const class wxString &) const) &wxFileConfig::HasGroup, "C++: wxFileConfig::HasGroup(const class wxString &) const --> bool", pybind11::arg("strName"));
		cl.def("HasEntry", (bool (wxFileConfig::*)(const class wxString &) const) &wxFileConfig::HasEntry, "C++: wxFileConfig::HasEntry(const class wxString &) const --> bool", pybind11::arg("strName"));
		cl.def("Flush", [](wxFileConfig &o) -> bool { return o.Flush(); }, "");
		cl.def("Flush", (bool (wxFileConfig::*)(bool)) &wxFileConfig::Flush, "C++: wxFileConfig::Flush(bool) --> bool", pybind11::arg("bCurrentOnly"));
		cl.def("RenameEntry", (bool (wxFileConfig::*)(const class wxString &, const class wxString &)) &wxFileConfig::RenameEntry, "C++: wxFileConfig::RenameEntry(const class wxString &, const class wxString &) --> bool", pybind11::arg("oldName"), pybind11::arg("newName"));
		cl.def("RenameGroup", (bool (wxFileConfig::*)(const class wxString &, const class wxString &)) &wxFileConfig::RenameGroup, "C++: wxFileConfig::RenameGroup(const class wxString &, const class wxString &) --> bool", pybind11::arg("oldName"), pybind11::arg("newName"));
		cl.def("DeleteEntry", [](wxFileConfig &o, const class wxString & a0) -> bool { return o.DeleteEntry(a0); }, "", pybind11::arg("key"));
		cl.def("DeleteEntry", (bool (wxFileConfig::*)(const class wxString &, bool)) &wxFileConfig::DeleteEntry, "C++: wxFileConfig::DeleteEntry(const class wxString &, bool) --> bool", pybind11::arg("key"), pybind11::arg("bGroupIfEmptyAlso"));
		cl.def("DeleteGroup", (bool (wxFileConfig::*)(const class wxString &)) &wxFileConfig::DeleteGroup, "C++: wxFileConfig::DeleteGroup(const class wxString &) --> bool", pybind11::arg("szKey"));
		cl.def("DeleteAll", (bool (wxFileConfig::*)()) &wxFileConfig::DeleteAll, "C++: wxFileConfig::DeleteAll() --> bool");
		cl.def("Save", [](wxFileConfig &o, class wxOutputStream & a0) -> bool { return o.Save(a0); }, "", pybind11::arg("os"));
		cl.def("Save", (bool (wxFileConfig::*)(class wxOutputStream &, const class wxMBConv &)) &wxFileConfig::Save, "C++: wxFileConfig::Save(class wxOutputStream &, const class wxMBConv &) --> bool", pybind11::arg("os"), pybind11::arg("conv"));
		cl.def("LineListIsEmpty", (bool (wxFileConfig::*)()) &wxFileConfig::LineListIsEmpty, "C++: wxFileConfig::LineListIsEmpty() --> bool");
		cl.def("GetClassInfo", (class wxClassInfo * (wxFileConfig::*)() const) &wxFileConfig::GetClassInfo, "C++: wxFileConfig::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
	}
	// wxDirFlags file:wx/dir.h line:28
	pybind11::enum_<wxDirFlags>(M(""), "wxDirFlags", pybind11::arithmetic(), "")
		.value("wxDIR_FILES", wxDIR_FILES)
		.value("wxDIR_DIRS", wxDIR_DIRS)
		.value("wxDIR_HIDDEN", wxDIR_HIDDEN)
		.value("wxDIR_DOTDOT", wxDIR_DOTDOT)
		.value("wxDIR_NO_FOLLOW", wxDIR_NO_FOLLOW)
		.value("wxDIR_DEFAULT", wxDIR_DEFAULT)
		.export_values();

;

	// wxDirTraverseResult file:wx/dir.h line:41
	pybind11::enum_<wxDirTraverseResult>(M(""), "wxDirTraverseResult", pybind11::arithmetic(), "")
		.value("wxDIR_IGNORE", wxDIR_IGNORE)
		.value("wxDIR_STOP", wxDIR_STOP)
		.value("wxDIR_CONTINUE", wxDIR_CONTINUE)
		.export_values();

;

	{ // wxDirTraverser file:wx/dir.h line:52
		pybind11::class_<wxDirTraverser, std::shared_ptr<wxDirTraverser>, PyCallBack_wxDirTraverser> cl(M(""), "wxDirTraverser", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxDirTraverser(); } ) );
		cl.def("OnFile", (enum wxDirTraverseResult (wxDirTraverser::*)(const class wxString &)) &wxDirTraverser::OnFile, "C++: wxDirTraverser::OnFile(const class wxString &) --> enum wxDirTraverseResult", pybind11::arg("filename"));
		cl.def("OnDir", (enum wxDirTraverseResult (wxDirTraverser::*)(const class wxString &)) &wxDirTraverser::OnDir, "C++: wxDirTraverser::OnDir(const class wxString &) --> enum wxDirTraverseResult", pybind11::arg("dirname"));
		cl.def("OnOpenError", (enum wxDirTraverseResult (wxDirTraverser::*)(const class wxString &)) &wxDirTraverser::OnOpenError, "C++: wxDirTraverser::OnOpenError(const class wxString &) --> enum wxDirTraverseResult", pybind11::arg("dirname"));
		cl.def("assign", (class wxDirTraverser & (wxDirTraverser::*)(const class wxDirTraverser &)) &wxDirTraverser::operator=, "C++: wxDirTraverser::operator=(const class wxDirTraverser &) --> class wxDirTraverser &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxDir file:wx/dir.h line:86
		pybind11::class_<wxDir, std::shared_ptr<wxDir>> cl(M(""), "wxDir", "");
		cl.def( pybind11::init( [](){ return new wxDir(); } ) );
		cl.def( pybind11::init<const class wxString &>(), pybind11::arg("dir") );

		cl.def("Open", (bool (wxDir::*)(const class wxString &)) &wxDir::Open, "C++: wxDir::Open(const class wxString &) --> bool", pybind11::arg("dir"));
		cl.def("Close", (void (wxDir::*)()) &wxDir::Close, "C++: wxDir::Close() --> void");
		cl.def("IsOpened", (bool (wxDir::*)() const) &wxDir::IsOpened, "C++: wxDir::IsOpened() const --> bool");
		cl.def("GetName", (class wxString (wxDir::*)() const) &wxDir::GetName, "C++: wxDir::GetName() const --> class wxString");
		cl.def("GetNameWithSep", (class wxString (wxDir::*)() const) &wxDir::GetNameWithSep, "C++: wxDir::GetNameWithSep() const --> class wxString");
		cl.def("GetFirst", [](wxDir const &o, class wxString * a0) -> bool { return o.GetFirst(a0); }, "", pybind11::arg("filename"));
		cl.def("GetFirst", [](wxDir const &o, class wxString * a0, const class wxString & a1) -> bool { return o.GetFirst(a0, a1); }, "", pybind11::arg("filename"), pybind11::arg("filespec"));
		cl.def("GetFirst", (bool (wxDir::*)(class wxString *, const class wxString &, int) const) &wxDir::GetFirst, "C++: wxDir::GetFirst(class wxString *, const class wxString &, int) const --> bool", pybind11::arg("filename"), pybind11::arg("filespec"), pybind11::arg("flags"));
		cl.def("GetNext", (bool (wxDir::*)(class wxString *) const) &wxDir::GetNext, "C++: wxDir::GetNext(class wxString *) const --> bool", pybind11::arg("filename"));
		cl.def("HasFiles", [](wxDir const &o) -> bool { return o.HasFiles(); }, "");
		cl.def("HasFiles", (bool (wxDir::*)(const class wxString &) const) &wxDir::HasFiles, "C++: wxDir::HasFiles(const class wxString &) const --> bool", pybind11::arg("spec"));
		cl.def("HasSubDirs", [](wxDir const &o) -> bool { return o.HasSubDirs(); }, "");
		cl.def("HasSubDirs", (bool (wxDir::*)(const class wxString &) const) &wxDir::HasSubDirs, "C++: wxDir::HasSubDirs(const class wxString &) const --> bool", pybind11::arg("spec"));
		cl.def("Traverse", [](wxDir const &o, class wxDirTraverser & a0) -> unsigned long { return o.Traverse(a0); }, "", pybind11::arg("sink"));
		cl.def("Traverse", [](wxDir const &o, class wxDirTraverser & a0, const class wxString & a1) -> unsigned long { return o.Traverse(a0, a1); }, "", pybind11::arg("sink"), pybind11::arg("filespec"));
		cl.def("Traverse", (unsigned long (wxDir::*)(class wxDirTraverser &, const class wxString &, int) const) &wxDir::Traverse, "C++: wxDir::Traverse(class wxDirTraverser &, const class wxString &, int) const --> unsigned long", pybind11::arg("sink"), pybind11::arg("filespec"), pybind11::arg("flags"));
		cl.def_static("GetAllFiles", [](const class wxString & a0, class wxArrayString * a1) -> unsigned long { return wxDir::GetAllFiles(a0, a1); }, "", pybind11::arg("dirname"), pybind11::arg("files"));
		cl.def_static("GetAllFiles", [](const class wxString & a0, class wxArrayString * a1, const class wxString & a2) -> unsigned long { return wxDir::GetAllFiles(a0, a1, a2); }, "", pybind11::arg("dirname"), pybind11::arg("files"), pybind11::arg("filespec"));
		cl.def_static("GetAllFiles", (unsigned long (*)(const class wxString &, class wxArrayString *, const class wxString &, int)) &wxDir::GetAllFiles, "C++: wxDir::GetAllFiles(const class wxString &, class wxArrayString *, const class wxString &, int) --> unsigned long", pybind11::arg("dirname"), pybind11::arg("files"), pybind11::arg("filespec"), pybind11::arg("flags"));
		cl.def_static("FindFirst", [](const class wxString & a0, const class wxString & a1) -> wxString { return wxDir::FindFirst(a0, a1); }, "", pybind11::arg("dirname"), pybind11::arg("filespec"));
		cl.def_static("FindFirst", (class wxString (*)(const class wxString &, const class wxString &, int)) &wxDir::FindFirst, "C++: wxDir::FindFirst(const class wxString &, const class wxString &, int) --> class wxString", pybind11::arg("dirname"), pybind11::arg("filespec"), pybind11::arg("flags"));
		cl.def_static("GetTotalSize", [](const class wxString & a0) -> wxULongLongNative { return wxDir::GetTotalSize(a0); }, "", pybind11::arg("dir"));
		cl.def_static("GetTotalSize", (class wxULongLongNative (*)(const class wxString &, class wxArrayString *)) &wxDir::GetTotalSize, "C++: wxDir::GetTotalSize(const class wxString &, class wxArrayString *) --> class wxULongLongNative", pybind11::arg("dir"), pybind11::arg("filesSkipped"));
		cl.def_static("Exists", (bool (*)(const class wxString &)) &wxDir::Exists, "C++: wxDir::Exists(const class wxString &) --> bool", pybind11::arg("dir"));
		cl.def_static("Make", [](const class wxString & a0) -> bool { return wxDir::Make(a0); }, "", pybind11::arg("dir"));
		cl.def_static("Make", [](const class wxString & a0, int const & a1) -> bool { return wxDir::Make(a0, a1); }, "", pybind11::arg("dir"), pybind11::arg("perm"));
		cl.def_static("Make", (bool (*)(const class wxString &, int, int)) &wxDir::Make, "C++: wxDir::Make(const class wxString &, int, int) --> bool", pybind11::arg("dir"), pybind11::arg("perm"), pybind11::arg("flags"));
		cl.def_static("Remove", [](const class wxString & a0) -> bool { return wxDir::Remove(a0); }, "", pybind11::arg("dir"));
		cl.def_static("Remove", (bool (*)(const class wxString &, int)) &wxDir::Remove, "C++: wxDir::Remove(const class wxString &, int) --> bool", pybind11::arg("dir"), pybind11::arg("flags"));
	}
	{ // wxProcess file:wx/process.h line:37
		pybind11::class_<wxProcess, std::shared_ptr<wxProcess>, PyCallBack_wxProcess, wxEvtHandler> cl(M(""), "wxProcess", "");
		cl.def( pybind11::init( [](){ return new wxProcess(); }, [](){ return new PyCallBack_wxProcess(); } ), "doc");
		cl.def( pybind11::init( [](class wxEvtHandler * a0){ return new wxProcess(a0); }, [](class wxEvtHandler * a0){ return new PyCallBack_wxProcess(a0); } ), "doc");
		cl.def( pybind11::init<class wxEvtHandler *, int>(), pybind11::arg("parent"), pybind11::arg("nId") );

		cl.def( pybind11::init<int>(), pybind11::arg("flags") );

		cl.def_static("Kill", [](int const & a0) -> wxKillError { return wxProcess::Kill(a0); }, "", pybind11::arg("pid"));
		cl.def_static("Kill", [](int const & a0, enum wxSignal const & a1) -> wxKillError { return wxProcess::Kill(a0, a1); }, "", pybind11::arg("pid"), pybind11::arg("sig"));
		cl.def_static("Kill", (enum wxKillError (*)(int, enum wxSignal, int)) &wxProcess::Kill, "C++: wxProcess::Kill(int, enum wxSignal, int) --> enum wxKillError", pybind11::arg("pid"), pybind11::arg("sig"), pybind11::arg("flags"));
		cl.def_static("Exists", (bool (*)(int)) &wxProcess::Exists, "C++: wxProcess::Exists(int) --> bool", pybind11::arg("pid"));
		cl.def_static("Open", [](const class wxString & a0) -> wxProcess * { return wxProcess::Open(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("cmd"));
		cl.def_static("Open", (class wxProcess * (*)(const class wxString &, int)) &wxProcess::Open, "C++: wxProcess::Open(const class wxString &, int) --> class wxProcess *", pybind11::return_value_policy::automatic, pybind11::arg("cmd"), pybind11::arg("flags"));
		cl.def("GetPid", (long (wxProcess::*)() const) &wxProcess::GetPid, "C++: wxProcess::GetPid() const --> long");
		cl.def("OnTerminate", (void (wxProcess::*)(int, int)) &wxProcess::OnTerminate, "C++: wxProcess::OnTerminate(int, int) --> void", pybind11::arg("pid"), pybind11::arg("status"));
		cl.def("Redirect", (void (wxProcess::*)()) &wxProcess::Redirect, "C++: wxProcess::Redirect() --> void");
		cl.def("IsRedirected", (bool (wxProcess::*)() const) &wxProcess::IsRedirected, "C++: wxProcess::IsRedirected() const --> bool");
		cl.def("Detach", (void (wxProcess::*)()) &wxProcess::Detach, "C++: wxProcess::Detach() --> void");
		cl.def("GetInputStream", (class wxInputStream * (wxProcess::*)() const) &wxProcess::GetInputStream, "C++: wxProcess::GetInputStream() const --> class wxInputStream *", pybind11::return_value_policy::automatic);
		cl.def("GetErrorStream", (class wxInputStream * (wxProcess::*)() const) &wxProcess::GetErrorStream, "C++: wxProcess::GetErrorStream() const --> class wxInputStream *", pybind11::return_value_policy::automatic);
		cl.def("GetOutputStream", (class wxOutputStream * (wxProcess::*)() const) &wxProcess::GetOutputStream, "C++: wxProcess::GetOutputStream() const --> class wxOutputStream *", pybind11::return_value_policy::automatic);
		cl.def("CloseOutput", (void (wxProcess::*)()) &wxProcess::CloseOutput, "C++: wxProcess::CloseOutput() --> void");
		cl.def("IsInputOpened", (bool (wxProcess::*)() const) &wxProcess::IsInputOpened, "C++: wxProcess::IsInputOpened() const --> bool");
		cl.def("IsInputAvailable", (bool (wxProcess::*)() const) &wxProcess::IsInputAvailable, "C++: wxProcess::IsInputAvailable() const --> bool");
		cl.def("IsErrorAvailable", (bool (wxProcess::*)() const) &wxProcess::IsErrorAvailable, "C++: wxProcess::IsErrorAvailable() const --> bool");
		cl.def("SetPipeStreams", (void (wxProcess::*)(class wxInputStream *, class wxOutputStream *, class wxInputStream *)) &wxProcess::SetPipeStreams, "C++: wxProcess::SetPipeStreams(class wxInputStream *, class wxOutputStream *, class wxInputStream *) --> void", pybind11::arg("outStream"), pybind11::arg("inStream"), pybind11::arg("errStream"));
		cl.def("SetPriority", (void (wxProcess::*)(unsigned int)) &wxProcess::SetPriority, "C++: wxProcess::SetPriority(unsigned int) --> void", pybind11::arg("priority"));
		cl.def("GetPriority", (unsigned int (wxProcess::*)() const) &wxProcess::GetPriority, "C++: wxProcess::GetPriority() const --> unsigned int");
		cl.def("SetPid", (void (wxProcess::*)(long)) &wxProcess::SetPid, "C++: wxProcess::SetPid(long) --> void", pybind11::arg("pid"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxProcess::*)() const) &wxProcess::GetClassInfo, "C++: wxProcess::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxProcess::wxCreateObject, "C++: wxProcess::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
	}
}

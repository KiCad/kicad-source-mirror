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

// wxBMPHandler file: line:42
struct PyCallBack_wxBMPHandler : public wxBMPHandler {
	using wxBMPHandler::wxBMPHandler;

	bool SaveFile(class wxImage * a0, class wxOutputStream & a1, bool a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBMPHandler *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxBMPHandler::SaveFile(a0, a1, a2);
	}
	bool LoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBMPHandler *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxBMPHandler::LoadFile(a0, a1, a2, a3);
	}
	bool DoCanRead(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBMPHandler *>(this), "DoCanRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxBMPHandler::DoCanRead(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBMPHandler *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxBMPHandler::GetClassInfo();
	}
	int DoGetImageCount(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBMPHandler *>(this), "DoGetImageCount");
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
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBMPHandler *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBMPHandler *>(this), "CloneRefData");
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

// wxICOHandler file: line:76
struct PyCallBack_wxICOHandler : public wxICOHandler {
	using wxICOHandler::wxICOHandler;

	bool SaveFile(class wxImage * a0, class wxOutputStream & a1, bool a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxICOHandler *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxICOHandler::SaveFile(a0, a1, a2);
	}
	bool LoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxICOHandler *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxICOHandler::LoadFile(a0, a1, a2, a3);
	}
	bool DoLoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxICOHandler *>(this), "DoLoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxICOHandler::DoLoadFile(a0, a1, a2, a3);
	}
	int DoGetImageCount(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxICOHandler *>(this), "DoGetImageCount");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxICOHandler::DoGetImageCount(a0);
	}
	bool DoCanRead(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxICOHandler *>(this), "DoCanRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxICOHandler::DoCanRead(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxICOHandler *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxICOHandler::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxICOHandler *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxICOHandler *>(this), "CloneRefData");
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

// wxCURHandler file: line:106
struct PyCallBack_wxCURHandler : public wxCURHandler {
	using wxCURHandler::wxCURHandler;

	bool DoCanRead(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCURHandler *>(this), "DoCanRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxCURHandler::DoCanRead(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCURHandler *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxCURHandler::GetClassInfo();
	}
	bool SaveFile(class wxImage * a0, class wxOutputStream & a1, bool a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCURHandler *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxICOHandler::SaveFile(a0, a1, a2);
	}
	bool LoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCURHandler *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxICOHandler::LoadFile(a0, a1, a2, a3);
	}
	bool DoLoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCURHandler *>(this), "DoLoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxICOHandler::DoLoadFile(a0, a1, a2, a3);
	}
	int DoGetImageCount(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCURHandler *>(this), "DoGetImageCount");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxICOHandler::DoGetImageCount(a0);
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCURHandler *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxCURHandler *>(this), "CloneRefData");
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

// wxANIHandler file: line:133
struct PyCallBack_wxANIHandler : public wxANIHandler {
	using wxANIHandler::wxANIHandler;

	bool SaveFile(class wxImage * a0, class wxOutputStream & a1, bool a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxANIHandler *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxANIHandler::SaveFile(a0, a1, a2);
	}
	bool LoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxANIHandler *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxANIHandler::LoadFile(a0, a1, a2, a3);
	}
	int DoGetImageCount(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxANIHandler *>(this), "DoGetImageCount");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxANIHandler::DoGetImageCount(a0);
	}
	bool DoCanRead(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxANIHandler *>(this), "DoCanRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxANIHandler::DoCanRead(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxANIHandler *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxANIHandler::GetClassInfo();
	}
	bool DoLoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxANIHandler *>(this), "DoLoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxICOHandler::DoLoadFile(a0, a1, a2, a3);
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxANIHandler *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxANIHandler *>(this), "CloneRefData");
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

// wxPNGHandler file: line:39
struct PyCallBack_wxPNGHandler : public wxPNGHandler {
	using wxPNGHandler::wxPNGHandler;

	bool LoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPNGHandler *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxPNGHandler::LoadFile(a0, a1, a2, a3);
	}
	bool SaveFile(class wxImage * a0, class wxOutputStream & a1, bool a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPNGHandler *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxPNGHandler::SaveFile(a0, a1, a2);
	}
	bool DoCanRead(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPNGHandler *>(this), "DoCanRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxPNGHandler::DoCanRead(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPNGHandler *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxPNGHandler::GetClassInfo();
	}
	int DoGetImageCount(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPNGHandler *>(this), "DoGetImageCount");
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
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPNGHandler *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPNGHandler *>(this), "CloneRefData");
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

// wxGIFHandler file: line:27
struct PyCallBack_wxGIFHandler : public wxGIFHandler {
	using wxGIFHandler::wxGIFHandler;

	bool LoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxGIFHandler *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxGIFHandler::LoadFile(a0, a1, a2, a3);
	}
	bool SaveFile(class wxImage * a0, class wxOutputStream & a1, bool a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxGIFHandler *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxGIFHandler::SaveFile(a0, a1, a2);
	}
	int DoGetImageCount(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxGIFHandler *>(this), "DoGetImageCount");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxGIFHandler::DoGetImageCount(a0);
	}
	bool DoCanRead(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxGIFHandler *>(this), "DoCanRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxGIFHandler::DoCanRead(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxGIFHandler *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxGIFHandler::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxGIFHandler *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxGIFHandler *>(this), "CloneRefData");
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

// wxPCXHandler file: line:20
struct PyCallBack_wxPCXHandler : public wxPCXHandler {
	using wxPCXHandler::wxPCXHandler;

	bool LoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPCXHandler *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxPCXHandler::LoadFile(a0, a1, a2, a3);
	}
	bool SaveFile(class wxImage * a0, class wxOutputStream & a1, bool a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPCXHandler *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxPCXHandler::SaveFile(a0, a1, a2);
	}
	bool DoCanRead(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPCXHandler *>(this), "DoCanRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxPCXHandler::DoCanRead(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPCXHandler *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxPCXHandler::GetClassInfo();
	}
	int DoGetImageCount(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPCXHandler *>(this), "DoGetImageCount");
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
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPCXHandler *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPCXHandler *>(this), "CloneRefData");
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

// wxJPEGHandler file: line:23
struct PyCallBack_wxJPEGHandler : public wxJPEGHandler {
	using wxJPEGHandler::wxJPEGHandler;

	bool LoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxJPEGHandler *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxJPEGHandler::LoadFile(a0, a1, a2, a3);
	}
	bool SaveFile(class wxImage * a0, class wxOutputStream & a1, bool a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxJPEGHandler *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxJPEGHandler::SaveFile(a0, a1, a2);
	}
	bool DoCanRead(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxJPEGHandler *>(this), "DoCanRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxJPEGHandler::DoCanRead(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxJPEGHandler *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxJPEGHandler::GetClassInfo();
	}
	int DoGetImageCount(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxJPEGHandler *>(this), "DoGetImageCount");
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
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxJPEGHandler *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxJPEGHandler *>(this), "CloneRefData");
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

void bind_unknown_unknown_74(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// wxInitAllImageHandlers() file: line:629
	M("").def("wxInitAllImageHandlers", (void (*)()) &wxInitAllImageHandlers, "C++: wxInitAllImageHandlers() --> void");

	{ // wxBMPHandler file: line:42
		pybind11::class_<wxBMPHandler, std::shared_ptr<wxBMPHandler>, PyCallBack_wxBMPHandler, wxImageHandler> cl(M(""), "wxBMPHandler", "");
		cl.def( pybind11::init( [](){ return new wxBMPHandler(); }, [](){ return new PyCallBack_wxBMPHandler(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxBMPHandler const &o){ return new PyCallBack_wxBMPHandler(o); } ) );
		cl.def( pybind11::init( [](wxBMPHandler const &o){ return new wxBMPHandler(o); } ) );
		cl.def("SaveFile", [](wxBMPHandler &o, class wxImage * a0, class wxOutputStream & a1) -> bool { return o.SaveFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("SaveFile", (bool (wxBMPHandler::*)(class wxImage *, class wxOutputStream &, bool)) &wxBMPHandler::SaveFile, "C++: wxBMPHandler::SaveFile(class wxImage *, class wxOutputStream &, bool) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("LoadFile", [](wxBMPHandler &o, class wxImage * a0, class wxInputStream & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("LoadFile", [](wxBMPHandler &o, class wxImage * a0, class wxInputStream & a1, bool const & a2) -> bool { return o.LoadFile(a0, a1, a2); }, "", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("LoadFile", (bool (wxBMPHandler::*)(class wxImage *, class wxInputStream &, bool, int)) &wxBMPHandler::LoadFile, "C++: wxBMPHandler::LoadFile(class wxImage *, class wxInputStream &, bool, int) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"), pybind11::arg("index"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxBMPHandler::*)() const) &wxBMPHandler::GetClassInfo, "C++: wxBMPHandler::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxBMPHandler::wxCreateObject, "C++: wxBMPHandler::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxBMPHandler & (wxBMPHandler::*)(const class wxBMPHandler &)) &wxBMPHandler::operator=, "C++: wxBMPHandler::operator=(const class wxBMPHandler &) --> class wxBMPHandler &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxICOHandler file: line:76
		pybind11::class_<wxICOHandler, std::shared_ptr<wxICOHandler>, PyCallBack_wxICOHandler, wxBMPHandler> cl(M(""), "wxICOHandler", "");
		cl.def( pybind11::init( [](){ return new wxICOHandler(); }, [](){ return new PyCallBack_wxICOHandler(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxICOHandler const &o){ return new PyCallBack_wxICOHandler(o); } ) );
		cl.def( pybind11::init( [](wxICOHandler const &o){ return new wxICOHandler(o); } ) );
		cl.def("SaveFile", [](wxICOHandler &o, class wxImage * a0, class wxOutputStream & a1) -> bool { return o.SaveFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("SaveFile", (bool (wxICOHandler::*)(class wxImage *, class wxOutputStream &, bool)) &wxICOHandler::SaveFile, "C++: wxICOHandler::SaveFile(class wxImage *, class wxOutputStream &, bool) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("LoadFile", [](wxICOHandler &o, class wxImage * a0, class wxInputStream & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("LoadFile", [](wxICOHandler &o, class wxImage * a0, class wxInputStream & a1, bool const & a2) -> bool { return o.LoadFile(a0, a1, a2); }, "", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("LoadFile", (bool (wxICOHandler::*)(class wxImage *, class wxInputStream &, bool, int)) &wxICOHandler::LoadFile, "C++: wxICOHandler::LoadFile(class wxImage *, class wxInputStream &, bool, int) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"), pybind11::arg("index"));
		cl.def("DoLoadFile", (bool (wxICOHandler::*)(class wxImage *, class wxInputStream &, bool, int)) &wxICOHandler::DoLoadFile, "C++: wxICOHandler::DoLoadFile(class wxImage *, class wxInputStream &, bool, int) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"), pybind11::arg("index"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxICOHandler::*)() const) &wxICOHandler::GetClassInfo, "C++: wxICOHandler::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxICOHandler::wxCreateObject, "C++: wxICOHandler::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxICOHandler & (wxICOHandler::*)(const class wxICOHandler &)) &wxICOHandler::operator=, "C++: wxICOHandler::operator=(const class wxICOHandler &) --> class wxICOHandler &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxCURHandler file: line:106
		pybind11::class_<wxCURHandler, std::shared_ptr<wxCURHandler>, PyCallBack_wxCURHandler, wxICOHandler> cl(M(""), "wxCURHandler", "");
		cl.def( pybind11::init( [](){ return new wxCURHandler(); }, [](){ return new PyCallBack_wxCURHandler(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxCURHandler const &o){ return new PyCallBack_wxCURHandler(o); } ) );
		cl.def( pybind11::init( [](wxCURHandler const &o){ return new wxCURHandler(o); } ) );
		cl.def("GetClassInfo", (class wxClassInfo * (wxCURHandler::*)() const) &wxCURHandler::GetClassInfo, "C++: wxCURHandler::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxCURHandler::wxCreateObject, "C++: wxCURHandler::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxCURHandler & (wxCURHandler::*)(const class wxCURHandler &)) &wxCURHandler::operator=, "C++: wxCURHandler::operator=(const class wxCURHandler &) --> class wxCURHandler &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxANIHandler file: line:133
		pybind11::class_<wxANIHandler, std::shared_ptr<wxANIHandler>, PyCallBack_wxANIHandler, wxCURHandler> cl(M(""), "wxANIHandler", "");
		cl.def( pybind11::init( [](){ return new wxANIHandler(); }, [](){ return new PyCallBack_wxANIHandler(); } ) );
		cl.def("SaveFile", (bool (wxANIHandler::*)(class wxImage *, class wxOutputStream &, bool)) &wxANIHandler::SaveFile, "C++: wxANIHandler::SaveFile(class wxImage *, class wxOutputStream &, bool) --> bool", pybind11::arg(""), pybind11::arg(""), pybind11::arg(""));
		cl.def("LoadFile", [](wxANIHandler &o, class wxImage * a0, class wxInputStream & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("LoadFile", [](wxANIHandler &o, class wxImage * a0, class wxInputStream & a1, bool const & a2) -> bool { return o.LoadFile(a0, a1, a2); }, "", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("LoadFile", (bool (wxANIHandler::*)(class wxImage *, class wxInputStream &, bool, int)) &wxANIHandler::LoadFile, "C++: wxANIHandler::LoadFile(class wxImage *, class wxInputStream &, bool, int) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"), pybind11::arg("index"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxANIHandler::*)() const) &wxANIHandler::GetClassInfo, "C++: wxANIHandler::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxANIHandler::wxCreateObject, "C++: wxANIHandler::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxANIHandler & (wxANIHandler::*)(const class wxANIHandler &)) &wxANIHandler::operator=, "C++: wxANIHandler::operator=(const class wxANIHandler &) --> class wxANIHandler &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxPNGHandler file: line:39
		pybind11::class_<wxPNGHandler, std::shared_ptr<wxPNGHandler>, PyCallBack_wxPNGHandler, wxImageHandler> cl(M(""), "wxPNGHandler", "");
		cl.def( pybind11::init( [](){ return new wxPNGHandler(); }, [](){ return new PyCallBack_wxPNGHandler(); } ) );
		cl.def_static("GetLibraryVersionInfo", (class wxVersionInfo (*)()) &wxPNGHandler::GetLibraryVersionInfo, "C++: wxPNGHandler::GetLibraryVersionInfo() --> class wxVersionInfo");
		cl.def("LoadFile", [](wxPNGHandler &o, class wxImage * a0, class wxInputStream & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("LoadFile", [](wxPNGHandler &o, class wxImage * a0, class wxInputStream & a1, bool const & a2) -> bool { return o.LoadFile(a0, a1, a2); }, "", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("LoadFile", (bool (wxPNGHandler::*)(class wxImage *, class wxInputStream &, bool, int)) &wxPNGHandler::LoadFile, "C++: wxPNGHandler::LoadFile(class wxImage *, class wxInputStream &, bool, int) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"), pybind11::arg("index"));
		cl.def("SaveFile", [](wxPNGHandler &o, class wxImage * a0, class wxOutputStream & a1) -> bool { return o.SaveFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("SaveFile", (bool (wxPNGHandler::*)(class wxImage *, class wxOutputStream &, bool)) &wxPNGHandler::SaveFile, "C++: wxPNGHandler::SaveFile(class wxImage *, class wxOutputStream &, bool) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxPNGHandler::*)() const) &wxPNGHandler::GetClassInfo, "C++: wxPNGHandler::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxPNGHandler::wxCreateObject, "C++: wxPNGHandler::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxPNGHandler & (wxPNGHandler::*)(const class wxPNGHandler &)) &wxPNGHandler::operator=, "C++: wxPNGHandler::operator=(const class wxPNGHandler &) --> class wxPNGHandler &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxGIFHandler file: line:27
		pybind11::class_<wxGIFHandler, std::shared_ptr<wxGIFHandler>, PyCallBack_wxGIFHandler, wxImageHandler> cl(M(""), "wxGIFHandler", "");
		cl.def( pybind11::init( [](){ return new wxGIFHandler(); }, [](){ return new PyCallBack_wxGIFHandler(); } ) );
		cl.def("LoadFile", [](wxGIFHandler &o, class wxImage * a0, class wxInputStream & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("LoadFile", [](wxGIFHandler &o, class wxImage * a0, class wxInputStream & a1, bool const & a2) -> bool { return o.LoadFile(a0, a1, a2); }, "", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("LoadFile", (bool (wxGIFHandler::*)(class wxImage *, class wxInputStream &, bool, int)) &wxGIFHandler::LoadFile, "C++: wxGIFHandler::LoadFile(class wxImage *, class wxInputStream &, bool, int) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"), pybind11::arg("index"));
		cl.def("SaveFile", [](wxGIFHandler &o, class wxImage * a0, class wxOutputStream & a1) -> bool { return o.SaveFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("SaveFile", (bool (wxGIFHandler::*)(class wxImage *, class wxOutputStream &, bool)) &wxGIFHandler::SaveFile, "C++: wxGIFHandler::SaveFile(class wxImage *, class wxOutputStream &, bool) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxGIFHandler::*)() const) &wxGIFHandler::GetClassInfo, "C++: wxGIFHandler::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxGIFHandler::wxCreateObject, "C++: wxGIFHandler::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxGIFHandler & (wxGIFHandler::*)(const class wxGIFHandler &)) &wxGIFHandler::operator=, "C++: wxGIFHandler::operator=(const class wxGIFHandler &) --> class wxGIFHandler &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxPCXHandler file: line:20
		pybind11::class_<wxPCXHandler, std::shared_ptr<wxPCXHandler>, PyCallBack_wxPCXHandler, wxImageHandler> cl(M(""), "wxPCXHandler", "");
		cl.def( pybind11::init( [](){ return new wxPCXHandler(); }, [](){ return new PyCallBack_wxPCXHandler(); } ) );
		cl.def("LoadFile", [](wxPCXHandler &o, class wxImage * a0, class wxInputStream & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("LoadFile", [](wxPCXHandler &o, class wxImage * a0, class wxInputStream & a1, bool const & a2) -> bool { return o.LoadFile(a0, a1, a2); }, "", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("LoadFile", (bool (wxPCXHandler::*)(class wxImage *, class wxInputStream &, bool, int)) &wxPCXHandler::LoadFile, "C++: wxPCXHandler::LoadFile(class wxImage *, class wxInputStream &, bool, int) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"), pybind11::arg("index"));
		cl.def("SaveFile", [](wxPCXHandler &o, class wxImage * a0, class wxOutputStream & a1) -> bool { return o.SaveFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("SaveFile", (bool (wxPCXHandler::*)(class wxImage *, class wxOutputStream &, bool)) &wxPCXHandler::SaveFile, "C++: wxPCXHandler::SaveFile(class wxImage *, class wxOutputStream &, bool) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxPCXHandler::*)() const) &wxPCXHandler::GetClassInfo, "C++: wxPCXHandler::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxPCXHandler::wxCreateObject, "C++: wxPCXHandler::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxPCXHandler & (wxPCXHandler::*)(const class wxPCXHandler &)) &wxPCXHandler::operator=, "C++: wxPCXHandler::operator=(const class wxPCXHandler &) --> class wxPCXHandler &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxJPEGHandler file: line:23
		pybind11::class_<wxJPEGHandler, std::shared_ptr<wxJPEGHandler>, PyCallBack_wxJPEGHandler, wxImageHandler> cl(M(""), "wxJPEGHandler", "");
		cl.def( pybind11::init( [](){ return new wxJPEGHandler(); }, [](){ return new PyCallBack_wxJPEGHandler(); } ) );
		cl.def_static("GetLibraryVersionInfo", (class wxVersionInfo (*)()) &wxJPEGHandler::GetLibraryVersionInfo, "C++: wxJPEGHandler::GetLibraryVersionInfo() --> class wxVersionInfo");
		cl.def("LoadFile", [](wxJPEGHandler &o, class wxImage * a0, class wxInputStream & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("LoadFile", [](wxJPEGHandler &o, class wxImage * a0, class wxInputStream & a1, bool const & a2) -> bool { return o.LoadFile(a0, a1, a2); }, "", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("LoadFile", (bool (wxJPEGHandler::*)(class wxImage *, class wxInputStream &, bool, int)) &wxJPEGHandler::LoadFile, "C++: wxJPEGHandler::LoadFile(class wxImage *, class wxInputStream &, bool, int) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"), pybind11::arg("index"));
		cl.def("SaveFile", [](wxJPEGHandler &o, class wxImage * a0, class wxOutputStream & a1) -> bool { return o.SaveFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("SaveFile", (bool (wxJPEGHandler::*)(class wxImage *, class wxOutputStream &, bool)) &wxJPEGHandler::SaveFile, "C++: wxJPEGHandler::SaveFile(class wxImage *, class wxOutputStream &, bool) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxJPEGHandler::*)() const) &wxJPEGHandler::GetClassInfo, "C++: wxJPEGHandler::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxJPEGHandler::wxCreateObject, "C++: wxJPEGHandler::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxJPEGHandler & (wxJPEGHandler::*)(const class wxJPEGHandler &)) &wxJPEGHandler::operator=, "C++: wxJPEGHandler::operator=(const class wxJPEGHandler &) --> class wxJPEGHandler &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}

#include <memory> // std::allocator
#include <sstream> // __str__
#include <string> // std::basic_string
#include <string> // std::char_traits
#include <wx/dc.h> // wxDC
#include <wx/dc.h> // wxDCImpl
#include <wx/dc.h> // wxDrawObject
#include <wx/dc.h> // wxFloodFillStyle
#include <wx/dc.h> // wxFontMetrics
#include <wx/dc.h> // wxMappingMode
#include <wx/dc.h> // wxRasterOperationMode

#include <pybind11/pybind11.h>
#include <functional>
#include <string>

#ifndef BINDER_PYBIND11_TYPE_CASTER
	#define BINDER_PYBIND11_TYPE_CASTER
	PYBIND11_DECLARE_HOLDER_TYPE(T, std::shared_ptr<T>);
	PYBIND11_DECLARE_HOLDER_TYPE(T, T*);
	PYBIND11_MAKE_OPAQUE(std::shared_ptr<void>);
#endif

// wxTGAHandler file: line:20
struct PyCallBack_wxTGAHandler : public wxTGAHandler {
	using wxTGAHandler::wxTGAHandler;

	bool LoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTGAHandler *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTGAHandler::LoadFile(a0, a1, a2, a3);
	}
	bool SaveFile(class wxImage * a0, class wxOutputStream & a1, bool a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTGAHandler *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTGAHandler::SaveFile(a0, a1, a2);
	}
	bool DoCanRead(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTGAHandler *>(this), "DoCanRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTGAHandler::DoCanRead(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTGAHandler *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxTGAHandler::GetClassInfo();
	}
	int DoGetImageCount(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTGAHandler *>(this), "DoGetImageCount");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTGAHandler *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTGAHandler *>(this), "CloneRefData");
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

// wxTIFFHandler file: line:36
struct PyCallBack_wxTIFFHandler : public wxTIFFHandler {
	using wxTIFFHandler::wxTIFFHandler;

	bool LoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTIFFHandler *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTIFFHandler::LoadFile(a0, a1, a2, a3);
	}
	bool SaveFile(class wxImage * a0, class wxOutputStream & a1, bool a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTIFFHandler *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTIFFHandler::SaveFile(a0, a1, a2);
	}
	int DoGetImageCount(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTIFFHandler *>(this), "DoGetImageCount");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxTIFFHandler::DoGetImageCount(a0);
	}
	bool DoCanRead(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTIFFHandler *>(this), "DoCanRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxTIFFHandler::DoCanRead(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTIFFHandler *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxTIFFHandler::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTIFFHandler *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTIFFHandler *>(this), "CloneRefData");
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

// wxPNMHandler file: line:19
struct PyCallBack_wxPNMHandler : public wxPNMHandler {
	using wxPNMHandler::wxPNMHandler;

	bool LoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPNMHandler *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxPNMHandler::LoadFile(a0, a1, a2, a3);
	}
	bool SaveFile(class wxImage * a0, class wxOutputStream & a1, bool a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPNMHandler *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxPNMHandler::SaveFile(a0, a1, a2);
	}
	bool DoCanRead(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPNMHandler *>(this), "DoCanRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxPNMHandler::DoCanRead(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPNMHandler *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxPNMHandler::GetClassInfo();
	}
	int DoGetImageCount(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPNMHandler *>(this), "DoGetImageCount");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPNMHandler *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxPNMHandler *>(this), "CloneRefData");
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

// wxXPMHandler file: line:20
struct PyCallBack_wxXPMHandler : public wxXPMHandler {
	using wxXPMHandler::wxXPMHandler;

	bool LoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxXPMHandler *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxXPMHandler::LoadFile(a0, a1, a2, a3);
	}
	bool SaveFile(class wxImage * a0, class wxOutputStream & a1, bool a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxXPMHandler *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxXPMHandler::SaveFile(a0, a1, a2);
	}
	bool DoCanRead(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxXPMHandler *>(this), "DoCanRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxXPMHandler::DoCanRead(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxXPMHandler *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxXPMHandler::GetClassInfo();
	}
	int DoGetImageCount(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxXPMHandler *>(this), "DoGetImageCount");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxXPMHandler *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxXPMHandler *>(this), "CloneRefData");
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

// wxIFFHandler file: line:20
struct PyCallBack_wxIFFHandler : public wxIFFHandler {
	using wxIFFHandler::wxIFFHandler;

	bool LoadFile(class wxImage * a0, class wxInputStream & a1, bool a2, int a3) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIFFHandler *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2, a3);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxIFFHandler::LoadFile(a0, a1, a2, a3);
	}
	bool SaveFile(class wxImage * a0, class wxOutputStream & a1, bool a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIFFHandler *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxIFFHandler::SaveFile(a0, a1, a2);
	}
	bool DoCanRead(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIFFHandler *>(this), "DoCanRead");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxIFFHandler::DoCanRead(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIFFHandler *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxIFFHandler::GetClassInfo();
	}
	int DoGetImageCount(class wxInputStream & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIFFHandler *>(this), "DoGetImageCount");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIFFHandler *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxIFFHandler *>(this), "CloneRefData");
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

// wxMaskBase file: line:45
struct PyCallBack_wxMaskBase : public wxMaskBase {
	using wxMaskBase::wxMaskBase;

	void FreeData() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMaskBase *>(this), "FreeData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxMaskBase::FreeData\"");
	}
	bool InitFromColour(const class wxBitmap & a0, const class wxColour & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMaskBase *>(this), "InitFromColour");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxMaskBase::InitFromColour\"");
	}
	bool InitFromMonoBitmap(const class wxBitmap & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMaskBase *>(this), "InitFromMonoBitmap");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxMaskBase::InitFromMonoBitmap\"");
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMaskBase *>(this), "GetClassInfo");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMaskBase *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMaskBase *>(this), "CloneRefData");
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

// wxMask file: line:23
struct PyCallBack_wxMask : public wxMask {
	using wxMask::wxMask;

	void FreeData() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMask *>(this), "FreeData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxMask::FreeData();
	}
	bool InitFromColour(const class wxBitmap & a0, const class wxColour & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMask *>(this), "InitFromColour");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxMask::InitFromColour(a0, a1);
	}
	bool InitFromMonoBitmap(const class wxBitmap & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMask *>(this), "InitFromMonoBitmap");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxMask::InitFromMonoBitmap(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMask *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxMask::GetClassInfo();
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMask *>(this), "CreateRefData");
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
		pybind11::function overload = pybind11::get_overload(static_cast<const wxMask *>(this), "CloneRefData");
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

// wxBitmap file: line:64
struct PyCallBack_wxBitmap : public wxBitmap {
	using wxBitmap::wxBitmap;

	int GetHeight() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBitmap *>(this), "GetHeight");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxBitmap::GetHeight();
	}
	int GetWidth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBitmap *>(this), "GetWidth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxBitmap::GetWidth();
	}
	int GetDepth() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBitmap *>(this), "GetDepth");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<int>::value) {
				static pybind11::detail::override_caster_t<int> caster;
				return pybind11::detail::cast_ref<int>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<int>(std::move(o));
		}
		return wxBitmap::GetDepth();
	}
	bool CopyFromIcon(const class wxIcon & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBitmap *>(this), "CopyFromIcon");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxBitmap::CopyFromIcon(a0);
	}
	class wxGDIRefData * CreateGDIRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBitmap *>(this), "CreateGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxBitmap::CreateGDIRefData();
	}
	class wxGDIRefData * CloneGDIRefData(const class wxGDIRefData * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBitmap *>(this), "CloneGDIRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxGDIRefData *>::value) {
				static pybind11::detail::override_caster_t<class wxGDIRefData *> caster;
				return pybind11::detail::cast_ref<class wxGDIRefData *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxGDIRefData *>(std::move(o));
		}
		return wxBitmap::CloneGDIRefData(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxBitmap *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxBitmap::GetClassInfo();
	}
};

void bind_unknown_unknown_75(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxTGAHandler file: line:20
		pybind11::class_<wxTGAHandler, std::shared_ptr<wxTGAHandler>, PyCallBack_wxTGAHandler, wxImageHandler> cl(M(""), "wxTGAHandler", "");
		cl.def( pybind11::init( [](){ return new wxTGAHandler(); }, [](){ return new PyCallBack_wxTGAHandler(); } ) );
		cl.def("LoadFile", [](wxTGAHandler &o, class wxImage * a0, class wxInputStream & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("LoadFile", [](wxTGAHandler &o, class wxImage * a0, class wxInputStream & a1, bool const & a2) -> bool { return o.LoadFile(a0, a1, a2); }, "", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("LoadFile", (bool (wxTGAHandler::*)(class wxImage *, class wxInputStream &, bool, int)) &wxTGAHandler::LoadFile, "C++: wxTGAHandler::LoadFile(class wxImage *, class wxInputStream &, bool, int) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"), pybind11::arg("index"));
		cl.def("SaveFile", [](wxTGAHandler &o, class wxImage * a0, class wxOutputStream & a1) -> bool { return o.SaveFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("SaveFile", (bool (wxTGAHandler::*)(class wxImage *, class wxOutputStream &, bool)) &wxTGAHandler::SaveFile, "C++: wxTGAHandler::SaveFile(class wxImage *, class wxOutputStream &, bool) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxTGAHandler::*)() const) &wxTGAHandler::GetClassInfo, "C++: wxTGAHandler::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxTGAHandler::wxCreateObject, "C++: wxTGAHandler::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxTGAHandler & (wxTGAHandler::*)(const class wxTGAHandler &)) &wxTGAHandler::operator=, "C++: wxTGAHandler::operator=(const class wxTGAHandler &) --> class wxTGAHandler &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxTIFFHandler file: line:36
		pybind11::class_<wxTIFFHandler, std::shared_ptr<wxTIFFHandler>, PyCallBack_wxTIFFHandler, wxImageHandler> cl(M(""), "wxTIFFHandler", "");
		cl.def( pybind11::init( [](){ return new wxTIFFHandler(); }, [](){ return new PyCallBack_wxTIFFHandler(); } ) );
		cl.def_static("GetLibraryVersionInfo", (class wxVersionInfo (*)()) &wxTIFFHandler::GetLibraryVersionInfo, "C++: wxTIFFHandler::GetLibraryVersionInfo() --> class wxVersionInfo");
		cl.def("LoadFile", [](wxTIFFHandler &o, class wxImage * a0, class wxInputStream & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("LoadFile", [](wxTIFFHandler &o, class wxImage * a0, class wxInputStream & a1, bool const & a2) -> bool { return o.LoadFile(a0, a1, a2); }, "", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("LoadFile", (bool (wxTIFFHandler::*)(class wxImage *, class wxInputStream &, bool, int)) &wxTIFFHandler::LoadFile, "C++: wxTIFFHandler::LoadFile(class wxImage *, class wxInputStream &, bool, int) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"), pybind11::arg("index"));
		cl.def("SaveFile", [](wxTIFFHandler &o, class wxImage * a0, class wxOutputStream & a1) -> bool { return o.SaveFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("SaveFile", (bool (wxTIFFHandler::*)(class wxImage *, class wxOutputStream &, bool)) &wxTIFFHandler::SaveFile, "C++: wxTIFFHandler::SaveFile(class wxImage *, class wxOutputStream &, bool) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxTIFFHandler::*)() const) &wxTIFFHandler::GetClassInfo, "C++: wxTIFFHandler::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxTIFFHandler::wxCreateObject, "C++: wxTIFFHandler::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxTIFFHandler & (wxTIFFHandler::*)(const class wxTIFFHandler &)) &wxTIFFHandler::operator=, "C++: wxTIFFHandler::operator=(const class wxTIFFHandler &) --> class wxTIFFHandler &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxPNMHandler file: line:19
		pybind11::class_<wxPNMHandler, std::shared_ptr<wxPNMHandler>, PyCallBack_wxPNMHandler, wxImageHandler> cl(M(""), "wxPNMHandler", "");
		cl.def( pybind11::init( [](){ return new wxPNMHandler(); }, [](){ return new PyCallBack_wxPNMHandler(); } ) );
		cl.def("LoadFile", [](wxPNMHandler &o, class wxImage * a0, class wxInputStream & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("LoadFile", [](wxPNMHandler &o, class wxImage * a0, class wxInputStream & a1, bool const & a2) -> bool { return o.LoadFile(a0, a1, a2); }, "", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("LoadFile", (bool (wxPNMHandler::*)(class wxImage *, class wxInputStream &, bool, int)) &wxPNMHandler::LoadFile, "C++: wxPNMHandler::LoadFile(class wxImage *, class wxInputStream &, bool, int) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"), pybind11::arg("index"));
		cl.def("SaveFile", [](wxPNMHandler &o, class wxImage * a0, class wxOutputStream & a1) -> bool { return o.SaveFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("SaveFile", (bool (wxPNMHandler::*)(class wxImage *, class wxOutputStream &, bool)) &wxPNMHandler::SaveFile, "C++: wxPNMHandler::SaveFile(class wxImage *, class wxOutputStream &, bool) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxPNMHandler::*)() const) &wxPNMHandler::GetClassInfo, "C++: wxPNMHandler::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxPNMHandler::wxCreateObject, "C++: wxPNMHandler::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxPNMHandler & (wxPNMHandler::*)(const class wxPNMHandler &)) &wxPNMHandler::operator=, "C++: wxPNMHandler::operator=(const class wxPNMHandler &) --> class wxPNMHandler &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxXPMHandler file: line:20
		pybind11::class_<wxXPMHandler, std::shared_ptr<wxXPMHandler>, PyCallBack_wxXPMHandler, wxImageHandler> cl(M(""), "wxXPMHandler", "");
		cl.def( pybind11::init( [](){ return new wxXPMHandler(); }, [](){ return new PyCallBack_wxXPMHandler(); } ) );
		cl.def("LoadFile", [](wxXPMHandler &o, class wxImage * a0, class wxInputStream & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("LoadFile", [](wxXPMHandler &o, class wxImage * a0, class wxInputStream & a1, bool const & a2) -> bool { return o.LoadFile(a0, a1, a2); }, "", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("LoadFile", (bool (wxXPMHandler::*)(class wxImage *, class wxInputStream &, bool, int)) &wxXPMHandler::LoadFile, "C++: wxXPMHandler::LoadFile(class wxImage *, class wxInputStream &, bool, int) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"), pybind11::arg("index"));
		cl.def("SaveFile", [](wxXPMHandler &o, class wxImage * a0, class wxOutputStream & a1) -> bool { return o.SaveFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("SaveFile", (bool (wxXPMHandler::*)(class wxImage *, class wxOutputStream &, bool)) &wxXPMHandler::SaveFile, "C++: wxXPMHandler::SaveFile(class wxImage *, class wxOutputStream &, bool) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxXPMHandler::*)() const) &wxXPMHandler::GetClassInfo, "C++: wxXPMHandler::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxXPMHandler::wxCreateObject, "C++: wxXPMHandler::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxXPMHandler & (wxXPMHandler::*)(const class wxXPMHandler &)) &wxXPMHandler::operator=, "C++: wxXPMHandler::operator=(const class wxXPMHandler &) --> class wxXPMHandler &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxIFFHandler file: line:20
		pybind11::class_<wxIFFHandler, std::shared_ptr<wxIFFHandler>, PyCallBack_wxIFFHandler, wxImageHandler> cl(M(""), "wxIFFHandler", "");
		cl.def( pybind11::init( [](){ return new wxIFFHandler(); }, [](){ return new PyCallBack_wxIFFHandler(); } ) );
		cl.def("LoadFile", [](wxIFFHandler &o, class wxImage * a0, class wxInputStream & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("LoadFile", [](wxIFFHandler &o, class wxImage * a0, class wxInputStream & a1, bool const & a2) -> bool { return o.LoadFile(a0, a1, a2); }, "", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("LoadFile", (bool (wxIFFHandler::*)(class wxImage *, class wxInputStream &, bool, int)) &wxIFFHandler::LoadFile, "C++: wxIFFHandler::LoadFile(class wxImage *, class wxInputStream &, bool, int) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"), pybind11::arg("index"));
		cl.def("SaveFile", [](wxIFFHandler &o, class wxImage * a0, class wxOutputStream & a1) -> bool { return o.SaveFile(a0, a1); }, "", pybind11::arg("image"), pybind11::arg("stream"));
		cl.def("SaveFile", (bool (wxIFFHandler::*)(class wxImage *, class wxOutputStream &, bool)) &wxIFFHandler::SaveFile, "C++: wxIFFHandler::SaveFile(class wxImage *, class wxOutputStream &, bool) --> bool", pybind11::arg("image"), pybind11::arg("stream"), pybind11::arg("verbose"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxIFFHandler::*)() const) &wxIFFHandler::GetClassInfo, "C++: wxIFFHandler::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxIFFHandler::wxCreateObject, "C++: wxIFFHandler::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxIFFHandler & (wxIFFHandler::*)(const class wxIFFHandler &)) &wxIFFHandler::operator=, "C++: wxIFFHandler::operator=(const class wxIFFHandler &) --> class wxIFFHandler &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxMaskBase file: line:45
		pybind11::class_<wxMaskBase, std::shared_ptr<wxMaskBase>, PyCallBack_wxMaskBase, wxObject> cl(M(""), "wxMaskBase", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxMaskBase(); } ) );
		cl.def(pybind11::init<PyCallBack_wxMaskBase const &>());
		cl.def("Create", (bool (wxMaskBase::*)(const class wxBitmap &, const class wxColour &)) &wxMaskBase::Create, "C++: wxMaskBase::Create(const class wxBitmap &, const class wxColour &) --> bool", pybind11::arg("bitmap"), pybind11::arg("colour"));
		cl.def("Create", (bool (wxMaskBase::*)(const class wxBitmap &, int)) &wxMaskBase::Create, "C++: wxMaskBase::Create(const class wxBitmap &, int) --> bool", pybind11::arg("bitmap"), pybind11::arg("paletteIndex"));
		cl.def("Create", (bool (wxMaskBase::*)(const class wxBitmap &)) &wxMaskBase::Create, "C++: wxMaskBase::Create(const class wxBitmap &) --> bool", pybind11::arg("bitmap"));
		cl.def("assign", (class wxMaskBase & (wxMaskBase::*)(const class wxMaskBase &)) &wxMaskBase::operator=, "C++: wxMaskBase::operator=(const class wxMaskBase &) --> class wxMaskBase &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxBitmapHelpers file: line:95
		pybind11::class_<wxBitmapHelpers, std::shared_ptr<wxBitmapHelpers>> cl(M(""), "wxBitmapHelpers", "");
		cl.def( pybind11::init( [](){ return new wxBitmapHelpers(); } ) );
		cl.def_static("NewFromPNGData", (class wxBitmap (*)(const void *, unsigned long)) &wxBitmapHelpers::NewFromPNGData, "C++: wxBitmapHelpers::NewFromPNGData(const void *, unsigned long) --> class wxBitmap", pybind11::arg("data"), pybind11::arg("size"));
	}
	{ // wxMask file: line:23
		pybind11::class_<wxMask, std::shared_ptr<wxMask>, PyCallBack_wxMask, wxMaskBase> cl(M(""), "wxMask", "");
		cl.def( pybind11::init( [](){ return new wxMask(); }, [](){ return new PyCallBack_wxMask(); } ) );
		cl.def( pybind11::init( [](PyCallBack_wxMask const &o){ return new PyCallBack_wxMask(o); } ) );
		cl.def( pybind11::init( [](wxMask const &o){ return new wxMask(o); } ) );
		cl.def( pybind11::init<const class wxBitmap &, const class wxColour &>(), pybind11::arg("bitmap"), pybind11::arg("colour") );

		cl.def( pybind11::init<const class wxBitmap &, int>(), pybind11::arg("bitmap"), pybind11::arg("paletteIndex") );

		cl.def( pybind11::init<const class wxBitmap &>(), pybind11::arg("bitmap") );

		cl.def("GetBitmap", (class wxBitmap (wxMask::*)() const) &wxMask::GetBitmap, "C++: wxMask::GetBitmap() const --> class wxBitmap");
		cl.def("GetClassInfo", (class wxClassInfo * (wxMask::*)() const) &wxMask::GetClassInfo, "C++: wxMask::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxMask::wxCreateObject, "C++: wxMask::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxMask & (wxMask::*)(const class wxMask &)) &wxMask::operator=, "C++: wxMask::operator=(const class wxMask &) --> class wxMask &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxBitmap file: line:64
		pybind11::class_<wxBitmap, std::shared_ptr<wxBitmap>, PyCallBack_wxBitmap> cl(M(""), "wxBitmap", "");
		cl.def( pybind11::init( [](){ return new wxBitmap(); }, [](){ return new PyCallBack_wxBitmap(); } ) );
		cl.def( pybind11::init( [](int const & a0, int const & a1){ return new wxBitmap(a0, a1); }, [](int const & a0, int const & a1){ return new PyCallBack_wxBitmap(a0, a1); } ), "doc");
		cl.def( pybind11::init<int, int, int>(), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("depth") );

		cl.def( pybind11::init( [](const class wxSize & a0){ return new wxBitmap(a0); }, [](const class wxSize & a0){ return new PyCallBack_wxBitmap(a0); } ), "doc");
		cl.def( pybind11::init<const class wxSize &, int>(), pybind11::arg("sz"), pybind11::arg("depth") );

		cl.def( pybind11::init( [](const class wxString & a0){ return new wxBitmap(a0); }, [](const class wxString & a0){ return new PyCallBack_wxBitmap(a0); } ), "doc");
		cl.def( pybind11::init<const class wxString &, enum wxBitmapType>(), pybind11::arg("filename"), pybind11::arg("type") );

		cl.def( pybind11::init( [](const class wxImage & a0){ return new wxBitmap(a0); }, [](const class wxImage & a0){ return new PyCallBack_wxBitmap(a0); } ), "doc");
		cl.def( pybind11::init<const class wxImage &, int>(), pybind11::arg("image"), pybind11::arg("depth") );

		cl.def( pybind11::init( [](PyCallBack_wxBitmap const &o){ return new PyCallBack_wxBitmap(o); } ) );
		cl.def( pybind11::init( [](wxBitmap const &o){ return new wxBitmap(o); } ) );
		cl.def("Create", [](wxBitmap &o, int const & a0, int const & a1) -> bool { return o.Create(a0, a1); }, "", pybind11::arg("width"), pybind11::arg("height"));
		cl.def("Create", (bool (wxBitmap::*)(int, int, int)) &wxBitmap::Create, "C++: wxBitmap::Create(int, int, int) --> bool", pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("depth"));
		cl.def("Create", [](wxBitmap &o, const class wxSize & a0) -> bool { return o.Create(a0); }, "", pybind11::arg("sz"));
		cl.def("Create", (bool (wxBitmap::*)(const class wxSize &, int)) &wxBitmap::Create, "C++: wxBitmap::Create(const class wxSize &, int) --> bool", pybind11::arg("sz"), pybind11::arg("depth"));
		cl.def("Create", (bool (wxBitmap::*)(int, int, const class wxDC &)) &wxBitmap::Create, "C++: wxBitmap::Create(int, int, const class wxDC &) --> bool", pybind11::arg("width"), pybind11::arg("height"), pybind11::arg(""));
		cl.def("GetHeight", (int (wxBitmap::*)() const) &wxBitmap::GetHeight, "C++: wxBitmap::GetHeight() const --> int");
		cl.def("GetWidth", (int (wxBitmap::*)() const) &wxBitmap::GetWidth, "C++: wxBitmap::GetWidth() const --> int");
		cl.def("GetDepth", (int (wxBitmap::*)() const) &wxBitmap::GetDepth, "C++: wxBitmap::GetDepth() const --> int");
		cl.def("ConvertToImage", (class wxImage (wxBitmap::*)() const) &wxBitmap::ConvertToImage, "C++: wxBitmap::ConvertToImage() const --> class wxImage");
		cl.def("CopyFromIcon", (bool (wxBitmap::*)(const class wxIcon &)) &wxBitmap::CopyFromIcon, "C++: wxBitmap::CopyFromIcon(const class wxIcon &) --> bool", pybind11::arg("icon"));
		cl.def("GetMask", (class wxMask * (wxBitmap::*)() const) &wxBitmap::GetMask, "C++: wxBitmap::GetMask() const --> class wxMask *", pybind11::return_value_policy::automatic);
		cl.def("SetMask", (void (wxBitmap::*)(class wxMask *)) &wxBitmap::SetMask, "C++: wxBitmap::SetMask(class wxMask *) --> void", pybind11::arg("mask"));
		cl.def("GetSubBitmap", (class wxBitmap (wxBitmap::*)(const class wxRect &) const) &wxBitmap::GetSubBitmap, "C++: wxBitmap::GetSubBitmap(const class wxRect &) const --> class wxBitmap", pybind11::arg("rect"));
		cl.def("LoadFile", [](wxBitmap &o, const class wxString & a0) -> bool { return o.LoadFile(a0); }, "", pybind11::arg("name"));
		cl.def("LoadFile", (bool (wxBitmap::*)(const class wxString &, enum wxBitmapType)) &wxBitmap::LoadFile, "C++: wxBitmap::LoadFile(const class wxString &, enum wxBitmapType) --> bool", pybind11::arg("name"), pybind11::arg("type"));
		cl.def_static("InitStandardHandlers", (void (*)()) &wxBitmap::InitStandardHandlers, "C++: wxBitmap::InitStandardHandlers() --> void");
		cl.def("SetHeight", (void (wxBitmap::*)(int)) &wxBitmap::SetHeight, "C++: wxBitmap::SetHeight(int) --> void", pybind11::arg("height"));
		cl.def("SetWidth", (void (wxBitmap::*)(int)) &wxBitmap::SetWidth, "C++: wxBitmap::SetWidth(int) --> void", pybind11::arg("width"));
		cl.def("SetDepth", (void (wxBitmap::*)(int)) &wxBitmap::SetDepth, "C++: wxBitmap::SetDepth(int) --> void", pybind11::arg("depth"));
		cl.def("HasAlpha", (bool (wxBitmap::*)() const) &wxBitmap::HasAlpha, "C++: wxBitmap::HasAlpha() const --> bool");
		cl.def("GetClassInfo", (class wxClassInfo * (wxBitmap::*)() const) &wxBitmap::GetClassInfo, "C++: wxBitmap::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxBitmap::wxCreateObject, "C++: wxBitmap::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxBitmap & (wxBitmap::*)(const class wxBitmap &)) &wxBitmap::operator=, "C++: wxBitmap::operator=(const class wxBitmap &) --> class wxBitmap &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}

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

// wxImage file: line:223
struct PyCallBack_wxImage : public wxImage {
	using wxImage::wxImage;

	bool LoadFile(const class wxString & a0, enum wxBitmapType a1, int a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImage *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxImage::LoadFile(a0, a1, a2);
	}
	bool LoadFile(const class wxString & a0, const class wxString & a1, int a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImage *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxImage::LoadFile(a0, a1, a2);
	}
	bool LoadFile(class wxInputStream & a0, enum wxBitmapType a1, int a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImage *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxImage::LoadFile(a0, a1, a2);
	}
	bool LoadFile(class wxInputStream & a0, const class wxString & a1, int a2) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImage *>(this), "LoadFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1, a2);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxImage::LoadFile(a0, a1, a2);
	}
	bool SaveFile(const class wxString & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImage *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxImage::SaveFile(a0);
	}
	bool SaveFile(const class wxString & a0, enum wxBitmapType a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImage *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxImage::SaveFile(a0, a1);
	}
	bool SaveFile(const class wxString & a0, const class wxString & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImage *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxImage::SaveFile(a0, a1);
	}
	bool SaveFile(class wxOutputStream & a0, enum wxBitmapType a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImage *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxImage::SaveFile(a0, a1);
	}
	bool SaveFile(class wxOutputStream & a0, const class wxString & a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImage *>(this), "SaveFile");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		return wxImage::SaveFile(a0, a1);
	}
	class wxRefCounter * CreateRefData() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImage *>(this), "CreateRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxImage::CreateRefData();
	}
	class wxRefCounter * CloneRefData(const class wxRefCounter * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImage *>(this), "CloneRefData");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRefCounter *>::value) {
				static pybind11::detail::override_caster_t<class wxRefCounter *> caster;
				return pybind11::detail::cast_ref<class wxRefCounter *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRefCounter *>(std::move(o));
		}
		return wxImage::CloneRefData(a0);
	}
	class wxClassInfo * GetClassInfo() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxImage *>(this), "GetClassInfo");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<class wxClassInfo *>::value) {
				static pybind11::detail::override_caster_t<class wxClassInfo *> caster;
				return pybind11::detail::cast_ref<class wxClassInfo *>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxClassInfo *>(std::move(o));
		}
		return wxImage::GetClassInfo();
	}
};

void bind_unknown_unknown_73(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxImageHistogramBase file: line:191
		pybind11::class_<wxImageHistogramBase, std::shared_ptr<wxImageHistogramBase>, wxImageHistogramBase_wxImplementation_HashTable> cl(M(""), "wxImageHistogramBase", "");
		cl.def( pybind11::init( [](){ return new wxImageHistogramBase(); } ), "doc" );
		cl.def( pybind11::init( [](unsigned long const & a0){ return new wxImageHistogramBase(a0); } ), "doc" , pybind11::arg("hint"));
		cl.def( pybind11::init( [](unsigned long const & a0, struct wxIntegerHash const & a1){ return new wxImageHistogramBase(a0, a1); } ), "doc" , pybind11::arg("hint"), pybind11::arg("hf"));
		cl.def( pybind11::init<unsigned long, struct wxIntegerHash, struct wxIntegerEqual>(), pybind11::arg("hint"), pybind11::arg("hf"), pybind11::arg("eq") );

		cl.def( pybind11::init( [](wxImageHistogramBase const &o){ return new wxImageHistogramBase(o); } ) );
		cl.def("__getitem__", (class wxImageHistogramEntry & (wxImageHistogramBase::*)(const unsigned long &)) &wxImageHistogramBase::operator[], "C++: wxImageHistogramBase::operator[](const unsigned long &) --> class wxImageHistogramEntry &", pybind11::return_value_policy::automatic, pybind11::arg("key"));
		cl.def("find", (class wxImageHistogramBase_wxImplementation_HashTable::iterator (wxImageHistogramBase::*)(const unsigned long &)) &wxImageHistogramBase::find, "C++: wxImageHistogramBase::find(const unsigned long &) --> class wxImageHistogramBase_wxImplementation_HashTable::iterator", pybind11::arg("key"));
		cl.def("insert", (class wxImageHistogramBase::Insert_Result (wxImageHistogramBase::*)(const class wxImageHistogramBase_wxImplementation_Pair &)) &wxImageHistogramBase::insert, "C++: wxImageHistogramBase::insert(const class wxImageHistogramBase_wxImplementation_Pair &) --> class wxImageHistogramBase::Insert_Result", pybind11::arg("v"));
		cl.def("erase", (unsigned long (wxImageHistogramBase::*)(const unsigned long &)) &wxImageHistogramBase::erase, "C++: wxImageHistogramBase::erase(const unsigned long &) --> unsigned long", pybind11::arg("k"));
		cl.def("erase", (void (wxImageHistogramBase::*)(const class wxImageHistogramBase_wxImplementation_HashTable::iterator &)) &wxImageHistogramBase::erase, "C++: wxImageHistogramBase::erase(const class wxImageHistogramBase_wxImplementation_HashTable::iterator &) --> void", pybind11::arg("it"));
		cl.def("count", (unsigned long (wxImageHistogramBase::*)(const unsigned long &)) &wxImageHistogramBase::count, "C++: wxImageHistogramBase::count(const unsigned long &) --> unsigned long", pybind11::arg("key"));
		cl.def("assign", (class wxImageHistogramBase & (wxImageHistogramBase::*)(const class wxImageHistogramBase &)) &wxImageHistogramBase::operator=, "C++: wxImageHistogramBase::operator=(const class wxImageHistogramBase &) --> class wxImageHistogramBase &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxImageHistogramBase::Insert_Result file: line:645
			auto & enclosing_class = cl;
			pybind11::class_<wxImageHistogramBase::Insert_Result, std::shared_ptr<wxImageHistogramBase::Insert_Result>> cl(enclosing_class, "Insert_Result", "");
			cl.def( pybind11::init<const class wxImageHistogramBase_wxImplementation_HashTable::iterator &, const bool &>(), pybind11::arg("f"), pybind11::arg("s") );

			cl.def( pybind11::init( [](wxImageHistogramBase::Insert_Result const &o){ return new wxImageHistogramBase::Insert_Result(o); } ) );
			cl.def_readwrite("first", &wxImageHistogramBase::Insert_Result::first);
			cl.def_readwrite("second", &wxImageHistogramBase::Insert_Result::second);
		}

	}
	{ // wxImageHistogram file: line:193
		pybind11::class_<wxImageHistogram, std::shared_ptr<wxImageHistogram>, wxImageHistogramBase> cl(M(""), "wxImageHistogram", "");
		cl.def( pybind11::init( [](){ return new wxImageHistogram(); } ) );
		cl.def_static("MakeKey", (unsigned long (*)(unsigned char, unsigned char, unsigned char)) &wxImageHistogram::MakeKey, "C++: wxImageHistogram::MakeKey(unsigned char, unsigned char, unsigned char) --> unsigned long", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("FindFirstUnusedColour", [](wxImageHistogram const &o, unsigned char * a0, unsigned char * a1, unsigned char * a2) -> bool { return o.FindFirstUnusedColour(a0, a1, a2); }, "", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("FindFirstUnusedColour", [](wxImageHistogram const &o, unsigned char * a0, unsigned char * a1, unsigned char * a2, unsigned char const & a3) -> bool { return o.FindFirstUnusedColour(a0, a1, a2, a3); }, "", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("startR"));
		cl.def("FindFirstUnusedColour", [](wxImageHistogram const &o, unsigned char * a0, unsigned char * a1, unsigned char * a2, unsigned char const & a3, unsigned char const & a4) -> bool { return o.FindFirstUnusedColour(a0, a1, a2, a3, a4); }, "", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("startR"), pybind11::arg("startG"));
		cl.def("FindFirstUnusedColour", (bool (wxImageHistogram::*)(unsigned char *, unsigned char *, unsigned char *, unsigned char, unsigned char, unsigned char) const) &wxImageHistogram::FindFirstUnusedColour, "C++: wxImageHistogram::FindFirstUnusedColour(unsigned char *, unsigned char *, unsigned char *, unsigned char, unsigned char, unsigned char) const --> bool", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("startR"), pybind11::arg("startG"), pybind11::arg("startB"));
	}
	{ // wxImage file: line:223
		pybind11::class_<wxImage, std::shared_ptr<wxImage>, PyCallBack_wxImage, wxObject> cl(M(""), "wxImage", "");
		cl.def( pybind11::init( [](){ return new wxImage(); }, [](){ return new PyCallBack_wxImage(); } ) );
		cl.def( pybind11::init( [](int const & a0, int const & a1){ return new wxImage(a0, a1); }, [](int const & a0, int const & a1){ return new PyCallBack_wxImage(a0, a1); } ), "doc");
		cl.def( pybind11::init<int, int, bool>(), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("clear") );

		cl.def( pybind11::init( [](int const & a0, int const & a1, unsigned char * a2){ return new wxImage(a0, a1, a2); }, [](int const & a0, int const & a1, unsigned char * a2){ return new PyCallBack_wxImage(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init<int, int, unsigned char *, bool>(), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("data"), pybind11::arg("static_data") );

		cl.def( pybind11::init( [](int const & a0, int const & a1, unsigned char * a2, unsigned char * a3){ return new wxImage(a0, a1, a2, a3); }, [](int const & a0, int const & a1, unsigned char * a2, unsigned char * a3){ return new PyCallBack_wxImage(a0, a1, a2, a3); } ), "doc");
		cl.def( pybind11::init<int, int, unsigned char *, unsigned char *, bool>(), pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("data"), pybind11::arg("alpha"), pybind11::arg("static_data") );

		cl.def( pybind11::init( [](const class wxSize & a0){ return new wxImage(a0); }, [](const class wxSize & a0){ return new PyCallBack_wxImage(a0); } ), "doc");
		cl.def( pybind11::init<const class wxSize &, bool>(), pybind11::arg("sz"), pybind11::arg("clear") );

		cl.def( pybind11::init( [](const class wxSize & a0, unsigned char * a1){ return new wxImage(a0, a1); }, [](const class wxSize & a0, unsigned char * a1){ return new PyCallBack_wxImage(a0, a1); } ), "doc");
		cl.def( pybind11::init<const class wxSize &, unsigned char *, bool>(), pybind11::arg("sz"), pybind11::arg("data"), pybind11::arg("static_data") );

		cl.def( pybind11::init( [](const class wxSize & a0, unsigned char * a1, unsigned char * a2){ return new wxImage(a0, a1, a2); }, [](const class wxSize & a0, unsigned char * a1, unsigned char * a2){ return new PyCallBack_wxImage(a0, a1, a2); } ), "doc");
		cl.def( pybind11::init<const class wxSize &, unsigned char *, unsigned char *, bool>(), pybind11::arg("sz"), pybind11::arg("data"), pybind11::arg("alpha"), pybind11::arg("static_data") );

		cl.def( pybind11::init( [](const class wxString & a0){ return new wxImage(a0); }, [](const class wxString & a0){ return new PyCallBack_wxImage(a0); } ), "doc");
		cl.def( pybind11::init( [](const class wxString & a0, enum wxBitmapType const & a1){ return new wxImage(a0, a1); }, [](const class wxString & a0, enum wxBitmapType const & a1){ return new PyCallBack_wxImage(a0, a1); } ), "doc");
		cl.def( pybind11::init<const class wxString &, enum wxBitmapType, int>(), pybind11::arg("name"), pybind11::arg("type"), pybind11::arg("index") );

		cl.def( pybind11::init( [](const class wxString & a0, const class wxString & a1){ return new wxImage(a0, a1); }, [](const class wxString & a0, const class wxString & a1){ return new PyCallBack_wxImage(a0, a1); } ), "doc");
		cl.def( pybind11::init<const class wxString &, const class wxString &, int>(), pybind11::arg("name"), pybind11::arg("mimetype"), pybind11::arg("index") );

		cl.def( pybind11::init( [](class wxInputStream & a0){ return new wxImage(a0); }, [](class wxInputStream & a0){ return new PyCallBack_wxImage(a0); } ), "doc");
		cl.def( pybind11::init( [](class wxInputStream & a0, enum wxBitmapType const & a1){ return new wxImage(a0, a1); }, [](class wxInputStream & a0, enum wxBitmapType const & a1){ return new PyCallBack_wxImage(a0, a1); } ), "doc");
		cl.def( pybind11::init<class wxInputStream &, enum wxBitmapType, int>(), pybind11::arg("stream"), pybind11::arg("type"), pybind11::arg("index") );

		cl.def( pybind11::init( [](class wxInputStream & a0, const class wxString & a1){ return new wxImage(a0, a1); }, [](class wxInputStream & a0, const class wxString & a1){ return new PyCallBack_wxImage(a0, a1); } ), "doc");
		cl.def( pybind11::init<class wxInputStream &, const class wxString &, int>(), pybind11::arg("stream"), pybind11::arg("mimetype"), pybind11::arg("index") );

		cl.def( pybind11::init( [](const class wxString & a0, long const & a1){ return new wxImage(a0, a1); }, [](const class wxString & a0, long const & a1){ return new PyCallBack_wxImage(a0, a1); } ), "doc");
		cl.def( pybind11::init<const class wxString &, long, int>(), pybind11::arg("name"), pybind11::arg("type"), pybind11::arg("index") );

		cl.def( pybind11::init( [](class wxInputStream & a0, long const & a1){ return new wxImage(a0, a1); }, [](class wxInputStream & a0, long const & a1){ return new PyCallBack_wxImage(a0, a1); } ), "doc");
		cl.def( pybind11::init<class wxInputStream &, long, int>(), pybind11::arg("stream"), pybind11::arg("type"), pybind11::arg("index") );

		cl.def( pybind11::init( [](PyCallBack_wxImage const &o){ return new PyCallBack_wxImage(o); } ) );
		cl.def( pybind11::init( [](wxImage const &o){ return new wxImage(o); } ) );
		cl.def("Create", [](wxImage &o, int const & a0, int const & a1) -> bool { return o.Create(a0, a1); }, "", pybind11::arg("width"), pybind11::arg("height"));
		cl.def("Create", (bool (wxImage::*)(int, int, bool)) &wxImage::Create, "C++: wxImage::Create(int, int, bool) --> bool", pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("clear"));
		cl.def("Create", [](wxImage &o, int const & a0, int const & a1, unsigned char * a2) -> bool { return o.Create(a0, a1, a2); }, "", pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("data"));
		cl.def("Create", (bool (wxImage::*)(int, int, unsigned char *, bool)) &wxImage::Create, "C++: wxImage::Create(int, int, unsigned char *, bool) --> bool", pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("data"), pybind11::arg("static_data"));
		cl.def("Create", [](wxImage &o, int const & a0, int const & a1, unsigned char * a2, unsigned char * a3) -> bool { return o.Create(a0, a1, a2, a3); }, "", pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("data"), pybind11::arg("alpha"));
		cl.def("Create", (bool (wxImage::*)(int, int, unsigned char *, unsigned char *, bool)) &wxImage::Create, "C++: wxImage::Create(int, int, unsigned char *, unsigned char *, bool) --> bool", pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("data"), pybind11::arg("alpha"), pybind11::arg("static_data"));
		cl.def("Create", [](wxImage &o, const class wxSize & a0) -> bool { return o.Create(a0); }, "", pybind11::arg("sz"));
		cl.def("Create", (bool (wxImage::*)(const class wxSize &, bool)) &wxImage::Create, "C++: wxImage::Create(const class wxSize &, bool) --> bool", pybind11::arg("sz"), pybind11::arg("clear"));
		cl.def("Create", [](wxImage &o, const class wxSize & a0, unsigned char * a1) -> bool { return o.Create(a0, a1); }, "", pybind11::arg("sz"), pybind11::arg("data"));
		cl.def("Create", (bool (wxImage::*)(const class wxSize &, unsigned char *, bool)) &wxImage::Create, "C++: wxImage::Create(const class wxSize &, unsigned char *, bool) --> bool", pybind11::arg("sz"), pybind11::arg("data"), pybind11::arg("static_data"));
		cl.def("Create", [](wxImage &o, const class wxSize & a0, unsigned char * a1, unsigned char * a2) -> bool { return o.Create(a0, a1, a2); }, "", pybind11::arg("sz"), pybind11::arg("data"), pybind11::arg("alpha"));
		cl.def("Create", (bool (wxImage::*)(const class wxSize &, unsigned char *, unsigned char *, bool)) &wxImage::Create, "C++: wxImage::Create(const class wxSize &, unsigned char *, unsigned char *, bool) --> bool", pybind11::arg("sz"), pybind11::arg("data"), pybind11::arg("alpha"), pybind11::arg("static_data"));
		cl.def("Destroy", (void (wxImage::*)()) &wxImage::Destroy, "C++: wxImage::Destroy() --> void");
		cl.def("Clear", [](wxImage &o) -> void { return o.Clear(); }, "");
		cl.def("Clear", (void (wxImage::*)(unsigned char)) &wxImage::Clear, "C++: wxImage::Clear(unsigned char) --> void", pybind11::arg("value"));
		cl.def("Copy", (class wxImage (wxImage::*)() const) &wxImage::Copy, "C++: wxImage::Copy() const --> class wxImage");
		cl.def("GetSubImage", (class wxImage (wxImage::*)(const class wxRect &) const) &wxImage::GetSubImage, "C++: wxImage::GetSubImage(const class wxRect &) const --> class wxImage", pybind11::arg("rect"));
		cl.def("Size", [](wxImage const &o, const class wxSize & a0, const class wxPoint & a1) -> wxImage { return o.Size(a0, a1); }, "", pybind11::arg("size"), pybind11::arg("pos"));
		cl.def("Size", [](wxImage const &o, const class wxSize & a0, const class wxPoint & a1, int const & a2) -> wxImage { return o.Size(a0, a1, a2); }, "", pybind11::arg("size"), pybind11::arg("pos"), pybind11::arg("r"));
		cl.def("Size", [](wxImage const &o, const class wxSize & a0, const class wxPoint & a1, int const & a2, int const & a3) -> wxImage { return o.Size(a0, a1, a2, a3); }, "", pybind11::arg("size"), pybind11::arg("pos"), pybind11::arg("r"), pybind11::arg("g"));
		cl.def("Size", (class wxImage (wxImage::*)(const class wxSize &, const class wxPoint &, int, int, int) const) &wxImage::Size, "C++: wxImage::Size(const class wxSize &, const class wxPoint &, int, int, int) const --> class wxImage", pybind11::arg("size"), pybind11::arg("pos"), pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("Paste", (void (wxImage::*)(const class wxImage &, int, int)) &wxImage::Paste, "C++: wxImage::Paste(const class wxImage &, int, int) --> void", pybind11::arg("image"), pybind11::arg("x"), pybind11::arg("y"));
		cl.def("Scale", [](wxImage const &o, int const & a0, int const & a1) -> wxImage { return o.Scale(a0, a1); }, "", pybind11::arg("width"), pybind11::arg("height"));
		cl.def("Scale", (class wxImage (wxImage::*)(int, int, enum wxImageResizeQuality) const) &wxImage::Scale, "C++: wxImage::Scale(int, int, enum wxImageResizeQuality) const --> class wxImage", pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("quality"));
		cl.def("ResampleNearest", (class wxImage (wxImage::*)(int, int) const) &wxImage::ResampleNearest, "C++: wxImage::ResampleNearest(int, int) const --> class wxImage", pybind11::arg("width"), pybind11::arg("height"));
		cl.def("ResampleBox", (class wxImage (wxImage::*)(int, int) const) &wxImage::ResampleBox, "C++: wxImage::ResampleBox(int, int) const --> class wxImage", pybind11::arg("width"), pybind11::arg("height"));
		cl.def("ResampleBilinear", (class wxImage (wxImage::*)(int, int) const) &wxImage::ResampleBilinear, "C++: wxImage::ResampleBilinear(int, int) const --> class wxImage", pybind11::arg("width"), pybind11::arg("height"));
		cl.def("ResampleBicubic", (class wxImage (wxImage::*)(int, int) const) &wxImage::ResampleBicubic, "C++: wxImage::ResampleBicubic(int, int) const --> class wxImage", pybind11::arg("width"), pybind11::arg("height"));
		cl.def("Blur", (class wxImage (wxImage::*)(int) const) &wxImage::Blur, "C++: wxImage::Blur(int) const --> class wxImage", pybind11::arg("radius"));
		cl.def("BlurHorizontal", (class wxImage (wxImage::*)(int) const) &wxImage::BlurHorizontal, "C++: wxImage::BlurHorizontal(int) const --> class wxImage", pybind11::arg("radius"));
		cl.def("BlurVertical", (class wxImage (wxImage::*)(int) const) &wxImage::BlurVertical, "C++: wxImage::BlurVertical(int) const --> class wxImage", pybind11::arg("radius"));
		cl.def("ShrinkBy", (class wxImage (wxImage::*)(int, int) const) &wxImage::ShrinkBy, "C++: wxImage::ShrinkBy(int, int) const --> class wxImage", pybind11::arg("xFactor"), pybind11::arg("yFactor"));
		cl.def("Rescale", [](wxImage &o, int const & a0, int const & a1) -> wxImage & { return o.Rescale(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("width"), pybind11::arg("height"));
		cl.def("Rescale", (class wxImage & (wxImage::*)(int, int, enum wxImageResizeQuality)) &wxImage::Rescale, "C++: wxImage::Rescale(int, int, enum wxImageResizeQuality) --> class wxImage &", pybind11::return_value_policy::automatic, pybind11::arg("width"), pybind11::arg("height"), pybind11::arg("quality"));
		cl.def("Resize", [](wxImage &o, const class wxSize & a0, const class wxPoint & a1) -> wxImage & { return o.Resize(a0, a1); }, "", pybind11::return_value_policy::automatic, pybind11::arg("size"), pybind11::arg("pos"));
		cl.def("Resize", [](wxImage &o, const class wxSize & a0, const class wxPoint & a1, int const & a2) -> wxImage & { return o.Resize(a0, a1, a2); }, "", pybind11::return_value_policy::automatic, pybind11::arg("size"), pybind11::arg("pos"), pybind11::arg("r"));
		cl.def("Resize", [](wxImage &o, const class wxSize & a0, const class wxPoint & a1, int const & a2, int const & a3) -> wxImage & { return o.Resize(a0, a1, a2, a3); }, "", pybind11::return_value_policy::automatic, pybind11::arg("size"), pybind11::arg("pos"), pybind11::arg("r"), pybind11::arg("g"));
		cl.def("Resize", (class wxImage & (wxImage::*)(const class wxSize &, const class wxPoint &, int, int, int)) &wxImage::Resize, "C++: wxImage::Resize(const class wxSize &, const class wxPoint &, int, int, int) --> class wxImage &", pybind11::return_value_policy::automatic, pybind11::arg("size"), pybind11::arg("pos"), pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("Rotate", [](wxImage const &o, double const & a0, const class wxPoint & a1) -> wxImage { return o.Rotate(a0, a1); }, "", pybind11::arg("angle"), pybind11::arg("centre_of_rotation"));
		cl.def("Rotate", [](wxImage const &o, double const & a0, const class wxPoint & a1, bool const & a2) -> wxImage { return o.Rotate(a0, a1, a2); }, "", pybind11::arg("angle"), pybind11::arg("centre_of_rotation"), pybind11::arg("interpolating"));
		cl.def("Rotate", (class wxImage (wxImage::*)(double, const class wxPoint &, bool, class wxPoint *) const) &wxImage::Rotate, "C++: wxImage::Rotate(double, const class wxPoint &, bool, class wxPoint *) const --> class wxImage", pybind11::arg("angle"), pybind11::arg("centre_of_rotation"), pybind11::arg("interpolating"), pybind11::arg("offset_after_rotation"));
		cl.def("Rotate90", [](wxImage const &o) -> wxImage { return o.Rotate90(); }, "");
		cl.def("Rotate90", (class wxImage (wxImage::*)(bool) const) &wxImage::Rotate90, "C++: wxImage::Rotate90(bool) const --> class wxImage", pybind11::arg("clockwise"));
		cl.def("Rotate180", (class wxImage (wxImage::*)() const) &wxImage::Rotate180, "C++: wxImage::Rotate180() const --> class wxImage");
		cl.def("Mirror", [](wxImage const &o) -> wxImage { return o.Mirror(); }, "");
		cl.def("Mirror", (class wxImage (wxImage::*)(bool) const) &wxImage::Mirror, "C++: wxImage::Mirror(bool) const --> class wxImage", pybind11::arg("horizontally"));
		cl.def("Replace", (void (wxImage::*)(unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, unsigned char)) &wxImage::Replace, "C++: wxImage::Replace(unsigned char, unsigned char, unsigned char, unsigned char, unsigned char, unsigned char) --> void", pybind11::arg("r1"), pybind11::arg("g1"), pybind11::arg("b1"), pybind11::arg("r2"), pybind11::arg("g2"), pybind11::arg("b2"));
		cl.def("ConvertToGreyscale", (class wxImage (wxImage::*)(double, double, double) const) &wxImage::ConvertToGreyscale, "C++: wxImage::ConvertToGreyscale(double, double, double) const --> class wxImage", pybind11::arg("weight_r"), pybind11::arg("weight_g"), pybind11::arg("weight_b"));
		cl.def("ConvertToGreyscale", (class wxImage (wxImage::*)() const) &wxImage::ConvertToGreyscale, "C++: wxImage::ConvertToGreyscale() const --> class wxImage");
		cl.def("ConvertToMono", (class wxImage (wxImage::*)(unsigned char, unsigned char, unsigned char) const) &wxImage::ConvertToMono, "C++: wxImage::ConvertToMono(unsigned char, unsigned char, unsigned char) const --> class wxImage", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("ConvertToDisabled", [](wxImage const &o) -> wxImage { return o.ConvertToDisabled(); }, "");
		cl.def("ConvertToDisabled", (class wxImage (wxImage::*)(unsigned char) const) &wxImage::ConvertToDisabled, "C++: wxImage::ConvertToDisabled(unsigned char) const --> class wxImage", pybind11::arg("brightness"));
		cl.def("SetRGB", (void (wxImage::*)(int, int, unsigned char, unsigned char, unsigned char)) &wxImage::SetRGB, "C++: wxImage::SetRGB(int, int, unsigned char, unsigned char, unsigned char) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("SetRGB", (void (wxImage::*)(const class wxRect &, unsigned char, unsigned char, unsigned char)) &wxImage::SetRGB, "C++: wxImage::SetRGB(const class wxRect &, unsigned char, unsigned char, unsigned char) --> void", pybind11::arg("rect"), pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("GetRed", (unsigned char (wxImage::*)(int, int) const) &wxImage::GetRed, "C++: wxImage::GetRed(int, int) const --> unsigned char", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetGreen", (unsigned char (wxImage::*)(int, int) const) &wxImage::GetGreen, "C++: wxImage::GetGreen(int, int) const --> unsigned char", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("GetBlue", (unsigned char (wxImage::*)(int, int) const) &wxImage::GetBlue, "C++: wxImage::GetBlue(int, int) const --> unsigned char", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("SetAlpha", (void (wxImage::*)(int, int, unsigned char)) &wxImage::SetAlpha, "C++: wxImage::SetAlpha(int, int, unsigned char) --> void", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("alpha"));
		cl.def("GetAlpha", (unsigned char (wxImage::*)(int, int) const) &wxImage::GetAlpha, "C++: wxImage::GetAlpha(int, int) const --> unsigned char", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("FindFirstUnusedColour", [](wxImage const &o, unsigned char * a0, unsigned char * a1, unsigned char * a2) -> bool { return o.FindFirstUnusedColour(a0, a1, a2); }, "", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("FindFirstUnusedColour", [](wxImage const &o, unsigned char * a0, unsigned char * a1, unsigned char * a2, unsigned char const & a3) -> bool { return o.FindFirstUnusedColour(a0, a1, a2, a3); }, "", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("startR"));
		cl.def("FindFirstUnusedColour", [](wxImage const &o, unsigned char * a0, unsigned char * a1, unsigned char * a2, unsigned char const & a3, unsigned char const & a4) -> bool { return o.FindFirstUnusedColour(a0, a1, a2, a3, a4); }, "", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("startR"), pybind11::arg("startG"));
		cl.def("FindFirstUnusedColour", (bool (wxImage::*)(unsigned char *, unsigned char *, unsigned char *, unsigned char, unsigned char, unsigned char) const) &wxImage::FindFirstUnusedColour, "C++: wxImage::FindFirstUnusedColour(unsigned char *, unsigned char *, unsigned char *, unsigned char, unsigned char, unsigned char) const --> bool", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"), pybind11::arg("startR"), pybind11::arg("startG"), pybind11::arg("startB"));
		cl.def("SetMaskFromImage", (bool (wxImage::*)(const class wxImage &, unsigned char, unsigned char, unsigned char)) &wxImage::SetMaskFromImage, "C++: wxImage::SetMaskFromImage(const class wxImage &, unsigned char, unsigned char, unsigned char) --> bool", pybind11::arg("mask"), pybind11::arg("mr"), pybind11::arg("mg"), pybind11::arg("mb"));
		cl.def("ConvertAlphaToMask", [](wxImage &o) -> bool { return o.ConvertAlphaToMask(); }, "");
		cl.def("ConvertAlphaToMask", (bool (wxImage::*)(unsigned char)) &wxImage::ConvertAlphaToMask, "C++: wxImage::ConvertAlphaToMask(unsigned char) --> bool", pybind11::arg("threshold"));
		cl.def("ConvertAlphaToMask", [](wxImage &o, unsigned char const & a0, unsigned char const & a1, unsigned char const & a2) -> bool { return o.ConvertAlphaToMask(a0, a1, a2); }, "", pybind11::arg("mr"), pybind11::arg("mg"), pybind11::arg("mb"));
		cl.def("ConvertAlphaToMask", (bool (wxImage::*)(unsigned char, unsigned char, unsigned char, unsigned char)) &wxImage::ConvertAlphaToMask, "C++: wxImage::ConvertAlphaToMask(unsigned char, unsigned char, unsigned char, unsigned char) --> bool", pybind11::arg("mr"), pybind11::arg("mg"), pybind11::arg("mb"), pybind11::arg("threshold"));
		cl.def("ConvertColourToAlpha", (bool (wxImage::*)(unsigned char, unsigned char, unsigned char)) &wxImage::ConvertColourToAlpha, "C++: wxImage::ConvertColourToAlpha(unsigned char, unsigned char, unsigned char) --> bool", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def_static("CanRead", (bool (*)(const class wxString &)) &wxImage::CanRead, "C++: wxImage::CanRead(const class wxString &) --> bool", pybind11::arg("name"));
		cl.def_static("GetImageCount", [](const class wxString & a0) -> int { return wxImage::GetImageCount(a0); }, "", pybind11::arg("name"));
		cl.def_static("GetImageCount", (int (*)(const class wxString &, enum wxBitmapType)) &wxImage::GetImageCount, "C++: wxImage::GetImageCount(const class wxString &, enum wxBitmapType) --> int", pybind11::arg("name"), pybind11::arg("type"));
		cl.def("LoadFile", [](wxImage &o, const class wxString & a0) -> bool { return o.LoadFile(a0); }, "", pybind11::arg("name"));
		cl.def("LoadFile", [](wxImage &o, const class wxString & a0, enum wxBitmapType const & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("name"), pybind11::arg("type"));
		cl.def("LoadFile", (bool (wxImage::*)(const class wxString &, enum wxBitmapType, int)) &wxImage::LoadFile, "C++: wxImage::LoadFile(const class wxString &, enum wxBitmapType, int) --> bool", pybind11::arg("name"), pybind11::arg("type"), pybind11::arg("index"));
		cl.def("LoadFile", [](wxImage &o, const class wxString & a0, const class wxString & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("name"), pybind11::arg("mimetype"));
		cl.def("LoadFile", (bool (wxImage::*)(const class wxString &, const class wxString &, int)) &wxImage::LoadFile, "C++: wxImage::LoadFile(const class wxString &, const class wxString &, int) --> bool", pybind11::arg("name"), pybind11::arg("mimetype"), pybind11::arg("index"));
		cl.def_static("CanRead", (bool (*)(class wxInputStream &)) &wxImage::CanRead, "C++: wxImage::CanRead(class wxInputStream &) --> bool", pybind11::arg("stream"));
		cl.def_static("GetImageCount", [](class wxInputStream & a0) -> int { return wxImage::GetImageCount(a0); }, "", pybind11::arg("stream"));
		cl.def_static("GetImageCount", (int (*)(class wxInputStream &, enum wxBitmapType)) &wxImage::GetImageCount, "C++: wxImage::GetImageCount(class wxInputStream &, enum wxBitmapType) --> int", pybind11::arg("stream"), pybind11::arg("type"));
		cl.def("LoadFile", [](wxImage &o, class wxInputStream & a0) -> bool { return o.LoadFile(a0); }, "", pybind11::arg("stream"));
		cl.def("LoadFile", [](wxImage &o, class wxInputStream & a0, enum wxBitmapType const & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("stream"), pybind11::arg("type"));
		cl.def("LoadFile", (bool (wxImage::*)(class wxInputStream &, enum wxBitmapType, int)) &wxImage::LoadFile, "C++: wxImage::LoadFile(class wxInputStream &, enum wxBitmapType, int) --> bool", pybind11::arg("stream"), pybind11::arg("type"), pybind11::arg("index"));
		cl.def("LoadFile", [](wxImage &o, class wxInputStream & a0, const class wxString & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("stream"), pybind11::arg("mimetype"));
		cl.def("LoadFile", (bool (wxImage::*)(class wxInputStream &, const class wxString &, int)) &wxImage::LoadFile, "C++: wxImage::LoadFile(class wxInputStream &, const class wxString &, int) --> bool", pybind11::arg("stream"), pybind11::arg("mimetype"), pybind11::arg("index"));
		cl.def("SaveFile", (bool (wxImage::*)(const class wxString &) const) &wxImage::SaveFile, "C++: wxImage::SaveFile(const class wxString &) const --> bool", pybind11::arg("name"));
		cl.def("SaveFile", (bool (wxImage::*)(const class wxString &, enum wxBitmapType) const) &wxImage::SaveFile, "C++: wxImage::SaveFile(const class wxString &, enum wxBitmapType) const --> bool", pybind11::arg("name"), pybind11::arg("type"));
		cl.def("SaveFile", (bool (wxImage::*)(const class wxString &, const class wxString &) const) &wxImage::SaveFile, "C++: wxImage::SaveFile(const class wxString &, const class wxString &) const --> bool", pybind11::arg("name"), pybind11::arg("mimetype"));
		cl.def("SaveFile", (bool (wxImage::*)(class wxOutputStream &, enum wxBitmapType) const) &wxImage::SaveFile, "C++: wxImage::SaveFile(class wxOutputStream &, enum wxBitmapType) const --> bool", pybind11::arg("stream"), pybind11::arg("type"));
		cl.def("SaveFile", (bool (wxImage::*)(class wxOutputStream &, const class wxString &) const) &wxImage::SaveFile, "C++: wxImage::SaveFile(class wxOutputStream &, const class wxString &) const --> bool", pybind11::arg("stream"), pybind11::arg("mimetype"));
		cl.def("Ok", (bool (wxImage::*)() const) &wxImage::Ok, "C++: wxImage::Ok() const --> bool");
		cl.def("IsOk", (bool (wxImage::*)() const) &wxImage::IsOk, "C++: wxImage::IsOk() const --> bool");
		cl.def("GetWidth", (int (wxImage::*)() const) &wxImage::GetWidth, "C++: wxImage::GetWidth() const --> int");
		cl.def("GetHeight", (int (wxImage::*)() const) &wxImage::GetHeight, "C++: wxImage::GetHeight() const --> int");
		cl.def("GetSize", (class wxSize (wxImage::*)() const) &wxImage::GetSize, "C++: wxImage::GetSize() const --> class wxSize");
		cl.def("GetType", (enum wxBitmapType (wxImage::*)() const) &wxImage::GetType, "C++: wxImage::GetType() const --> enum wxBitmapType");
		cl.def("SetType", (void (wxImage::*)(enum wxBitmapType)) &wxImage::SetType, "C++: wxImage::SetType(enum wxBitmapType) --> void", pybind11::arg("type"));
		cl.def("GetData", (unsigned char * (wxImage::*)() const) &wxImage::GetData, "C++: wxImage::GetData() const --> unsigned char *", pybind11::return_value_policy::automatic);
		cl.def("SetData", [](wxImage &o, unsigned char * a0) -> void { return o.SetData(a0); }, "", pybind11::arg("data"));
		cl.def("SetData", (void (wxImage::*)(unsigned char *, bool)) &wxImage::SetData, "C++: wxImage::SetData(unsigned char *, bool) --> void", pybind11::arg("data"), pybind11::arg("static_data"));
		cl.def("SetData", [](wxImage &o, unsigned char * a0, int const & a1, int const & a2) -> void { return o.SetData(a0, a1, a2); }, "", pybind11::arg("data"), pybind11::arg("new_width"), pybind11::arg("new_height"));
		cl.def("SetData", (void (wxImage::*)(unsigned char *, int, int, bool)) &wxImage::SetData, "C++: wxImage::SetData(unsigned char *, int, int, bool) --> void", pybind11::arg("data"), pybind11::arg("new_width"), pybind11::arg("new_height"), pybind11::arg("static_data"));
		cl.def("GetAlpha", (unsigned char * (wxImage::*)() const) &wxImage::GetAlpha, "C++: wxImage::GetAlpha() const --> unsigned char *", pybind11::return_value_policy::automatic);
		cl.def("HasAlpha", (bool (wxImage::*)() const) &wxImage::HasAlpha, "C++: wxImage::HasAlpha() const --> bool");
		cl.def("SetAlpha", [](wxImage &o) -> void { return o.SetAlpha(); }, "");
		cl.def("SetAlpha", [](wxImage &o, unsigned char * a0) -> void { return o.SetAlpha(a0); }, "", pybind11::arg("alpha"));
		cl.def("SetAlpha", (void (wxImage::*)(unsigned char *, bool)) &wxImage::SetAlpha, "C++: wxImage::SetAlpha(unsigned char *, bool) --> void", pybind11::arg("alpha"), pybind11::arg("static_data"));
		cl.def("InitAlpha", (void (wxImage::*)()) &wxImage::InitAlpha, "C++: wxImage::InitAlpha() --> void");
		cl.def("ClearAlpha", (void (wxImage::*)()) &wxImage::ClearAlpha, "C++: wxImage::ClearAlpha() --> void");
		cl.def("IsTransparent", [](wxImage const &o, int const & a0, int const & a1) -> bool { return o.IsTransparent(a0, a1); }, "", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("IsTransparent", (bool (wxImage::*)(int, int, unsigned char) const) &wxImage::IsTransparent, "C++: wxImage::IsTransparent(int, int, unsigned char) const --> bool", pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("threshold"));
		cl.def("SetMaskColour", (void (wxImage::*)(unsigned char, unsigned char, unsigned char)) &wxImage::SetMaskColour, "C++: wxImage::SetMaskColour(unsigned char, unsigned char, unsigned char) --> void", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("GetOrFindMaskColour", (bool (wxImage::*)(unsigned char *, unsigned char *, unsigned char *) const) &wxImage::GetOrFindMaskColour, "C++: wxImage::GetOrFindMaskColour(unsigned char *, unsigned char *, unsigned char *) const --> bool", pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b"));
		cl.def("GetMaskRed", (unsigned char (wxImage::*)() const) &wxImage::GetMaskRed, "C++: wxImage::GetMaskRed() const --> unsigned char");
		cl.def("GetMaskGreen", (unsigned char (wxImage::*)() const) &wxImage::GetMaskGreen, "C++: wxImage::GetMaskGreen() const --> unsigned char");
		cl.def("GetMaskBlue", (unsigned char (wxImage::*)() const) &wxImage::GetMaskBlue, "C++: wxImage::GetMaskBlue() const --> unsigned char");
		cl.def("SetMask", [](wxImage &o) -> void { return o.SetMask(); }, "");
		cl.def("SetMask", (void (wxImage::*)(bool)) &wxImage::SetMask, "C++: wxImage::SetMask(bool) --> void", pybind11::arg("mask"));
		cl.def("HasMask", (bool (wxImage::*)() const) &wxImage::HasMask, "C++: wxImage::HasMask() const --> bool");
		cl.def("HasPalette", (bool (wxImage::*)() const) &wxImage::HasPalette, "C++: wxImage::HasPalette() const --> bool");
		cl.def("SetOption", (void (wxImage::*)(const class wxString &, const class wxString &)) &wxImage::SetOption, "C++: wxImage::SetOption(const class wxString &, const class wxString &) --> void", pybind11::arg("name"), pybind11::arg("value"));
		cl.def("SetOption", (void (wxImage::*)(const class wxString &, int)) &wxImage::SetOption, "C++: wxImage::SetOption(const class wxString &, int) --> void", pybind11::arg("name"), pybind11::arg("value"));
		cl.def("GetOption", (class wxString (wxImage::*)(const class wxString &) const) &wxImage::GetOption, "C++: wxImage::GetOption(const class wxString &) const --> class wxString", pybind11::arg("name"));
		cl.def("GetOptionInt", (int (wxImage::*)(const class wxString &) const) &wxImage::GetOptionInt, "C++: wxImage::GetOptionInt(const class wxString &) const --> int", pybind11::arg("name"));
		cl.def("HasOption", (bool (wxImage::*)(const class wxString &) const) &wxImage::HasOption, "C++: wxImage::HasOption(const class wxString &) const --> bool", pybind11::arg("name"));
		cl.def("CountColours", [](wxImage const &o) -> unsigned long { return o.CountColours(); }, "");
		cl.def("CountColours", (unsigned long (wxImage::*)(unsigned long) const) &wxImage::CountColours, "C++: wxImage::CountColours(unsigned long) const --> unsigned long", pybind11::arg("stopafter"));
		cl.def("ComputeHistogram", (unsigned long (wxImage::*)(class wxImageHistogram &) const) &wxImage::ComputeHistogram, "C++: wxImage::ComputeHistogram(class wxImageHistogram &) const --> unsigned long", pybind11::arg("h"));
		cl.def("RotateHue", (void (wxImage::*)(double)) &wxImage::RotateHue, "C++: wxImage::RotateHue(double) --> void", pybind11::arg("angle"));
		cl.def_static("GetHandlers", (class wxList & (*)()) &wxImage::GetHandlers, "C++: wxImage::GetHandlers() --> class wxList &", pybind11::return_value_policy::automatic);
		cl.def_static("AddHandler", (void (*)(class wxImageHandler *)) &wxImage::AddHandler, "C++: wxImage::AddHandler(class wxImageHandler *) --> void", pybind11::arg("handler"));
		cl.def_static("InsertHandler", (void (*)(class wxImageHandler *)) &wxImage::InsertHandler, "C++: wxImage::InsertHandler(class wxImageHandler *) --> void", pybind11::arg("handler"));
		cl.def_static("RemoveHandler", (bool (*)(const class wxString &)) &wxImage::RemoveHandler, "C++: wxImage::RemoveHandler(const class wxString &) --> bool", pybind11::arg("name"));
		cl.def_static("FindHandler", (class wxImageHandler * (*)(const class wxString &)) &wxImage::FindHandler, "C++: wxImage::FindHandler(const class wxString &) --> class wxImageHandler *", pybind11::return_value_policy::automatic, pybind11::arg("name"));
		cl.def_static("FindHandler", (class wxImageHandler * (*)(const class wxString &, enum wxBitmapType)) &wxImage::FindHandler, "C++: wxImage::FindHandler(const class wxString &, enum wxBitmapType) --> class wxImageHandler *", pybind11::return_value_policy::automatic, pybind11::arg("extension"), pybind11::arg("imageType"));
		cl.def_static("FindHandler", (class wxImageHandler * (*)(enum wxBitmapType)) &wxImage::FindHandler, "C++: wxImage::FindHandler(enum wxBitmapType) --> class wxImageHandler *", pybind11::return_value_policy::automatic, pybind11::arg("imageType"));
		cl.def_static("FindHandlerMime", (class wxImageHandler * (*)(const class wxString &)) &wxImage::FindHandlerMime, "C++: wxImage::FindHandlerMime(const class wxString &) --> class wxImageHandler *", pybind11::return_value_policy::automatic, pybind11::arg("mimetype"));
		cl.def_static("GetImageExtWildcard", (class wxString (*)()) &wxImage::GetImageExtWildcard, "C++: wxImage::GetImageExtWildcard() --> class wxString");
		cl.def_static("CleanUpHandlers", (void (*)()) &wxImage::CleanUpHandlers, "C++: wxImage::CleanUpHandlers() --> void");
		cl.def_static("InitStandardHandlers", (void (*)()) &wxImage::InitStandardHandlers, "C++: wxImage::InitStandardHandlers() --> void");
		cl.def_static("RGBtoHSV", (class wxImage::HSVValue (*)(const class wxImage::RGBValue &)) &wxImage::RGBtoHSV, "C++: wxImage::RGBtoHSV(const class wxImage::RGBValue &) --> class wxImage::HSVValue", pybind11::arg("rgb"));
		cl.def_static("HSVtoRGB", (class wxImage::RGBValue (*)(const class wxImage::HSVValue &)) &wxImage::HSVtoRGB, "C++: wxImage::HSVtoRGB(const class wxImage::HSVValue &) --> class wxImage::RGBValue", pybind11::arg("hsv"));
		cl.def("LoadFile", [](wxImage &o, class wxInputStream & a0, long const & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("stream"), pybind11::arg("type"));
		cl.def("LoadFile", (bool (wxImage::*)(class wxInputStream &, long, int)) &wxImage::LoadFile, "C++: wxImage::LoadFile(class wxInputStream &, long, int) --> bool", pybind11::arg("stream"), pybind11::arg("type"), pybind11::arg("index"));
		cl.def("SaveFile", (bool (wxImage::*)(class wxOutputStream &, long) const) &wxImage::SaveFile, "C++: wxImage::SaveFile(class wxOutputStream &, long) const --> bool", pybind11::arg("stream"), pybind11::arg("type"));
		cl.def("LoadFile", [](wxImage &o, const class wxString & a0, long const & a1) -> bool { return o.LoadFile(a0, a1); }, "", pybind11::arg("name"), pybind11::arg("type"));
		cl.def("LoadFile", (bool (wxImage::*)(const class wxString &, long, int)) &wxImage::LoadFile, "C++: wxImage::LoadFile(const class wxString &, long, int) --> bool", pybind11::arg("name"), pybind11::arg("type"), pybind11::arg("index"));
		cl.def("SaveFile", (bool (wxImage::*)(const class wxString &, long) const) &wxImage::SaveFile, "C++: wxImage::SaveFile(const class wxString &, long) const --> bool", pybind11::arg("name"), pybind11::arg("type"));
		cl.def_static("FindHandler", (class wxImageHandler * (*)(const class wxString &, long)) &wxImage::FindHandler, "C++: wxImage::FindHandler(const class wxString &, long) --> class wxImageHandler *", pybind11::return_value_policy::automatic, pybind11::arg("ext"), pybind11::arg("type"));
		cl.def_static("FindHandler", (class wxImageHandler * (*)(long)) &wxImage::FindHandler, "C++: wxImage::FindHandler(long) --> class wxImageHandler *", pybind11::return_value_policy::automatic, pybind11::arg("imageType"));
		cl.def("GetClassInfo", (class wxClassInfo * (wxImage::*)() const) &wxImage::GetClassInfo, "C++: wxImage::GetClassInfo() const --> class wxClassInfo *", pybind11::return_value_policy::automatic);
		cl.def_static("wxCreateObject", (class wxObject * (*)()) &wxImage::wxCreateObject, "C++: wxImage::wxCreateObject() --> class wxObject *", pybind11::return_value_policy::automatic);
		cl.def("assign", (class wxImage & (wxImage::*)(const class wxImage &)) &wxImage::operator=, "C++: wxImage::operator=(const class wxImage &) --> class wxImage &", pybind11::return_value_policy::automatic, pybind11::arg(""));

		{ // wxImage::RGBValue file: line:228
			auto & enclosing_class = cl;
			pybind11::class_<wxImage::RGBValue, std::shared_ptr<wxImage::RGBValue>> cl(enclosing_class, "RGBValue", "");
			cl.def( pybind11::init( [](){ return new wxImage::RGBValue(); } ), "doc" );
			cl.def( pybind11::init( [](unsigned char const & a0){ return new wxImage::RGBValue(a0); } ), "doc" , pybind11::arg("r"));
			cl.def( pybind11::init( [](unsigned char const & a0, unsigned char const & a1){ return new wxImage::RGBValue(a0, a1); } ), "doc" , pybind11::arg("r"), pybind11::arg("g"));
			cl.def( pybind11::init<unsigned char, unsigned char, unsigned char>(), pybind11::arg("r"), pybind11::arg("g"), pybind11::arg("b") );

			cl.def_readwrite("red", &wxImage::RGBValue::red);
			cl.def_readwrite("green", &wxImage::RGBValue::green);
			cl.def_readwrite("blue", &wxImage::RGBValue::blue);
		}

		{ // wxImage::HSVValue file: line:239
			auto & enclosing_class = cl;
			pybind11::class_<wxImage::HSVValue, std::shared_ptr<wxImage::HSVValue>> cl(enclosing_class, "HSVValue", "");
			cl.def( pybind11::init( [](){ return new wxImage::HSVValue(); } ), "doc" );
			cl.def( pybind11::init( [](double const & a0){ return new wxImage::HSVValue(a0); } ), "doc" , pybind11::arg("h"));
			cl.def( pybind11::init( [](double const & a0, double const & a1){ return new wxImage::HSVValue(a0, a1); } ), "doc" , pybind11::arg("h"), pybind11::arg("s"));
			cl.def( pybind11::init<double, double, double>(), pybind11::arg("h"), pybind11::arg("s"), pybind11::arg("v") );

			cl.def_readwrite("hue", &wxImage::HSVValue::hue);
			cl.def_readwrite("saturation", &wxImage::HSVValue::saturation);
			cl.def_readwrite("value", &wxImage::HSVValue::value);
		}

	}
}

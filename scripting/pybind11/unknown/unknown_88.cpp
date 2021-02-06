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

// wxTransform2D file: line:786
struct PyCallBack_wxTransform2D : public wxTransform2D {
	using wxTransform2D::wxTransform2D;

	void Transform(class wxPoint2DInt * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTransform2D *>(this), "Transform");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxTransform2D::Transform\"");
	}
	void Transform(class wxRect2DInt * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTransform2D *>(this), "Transform");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTransform2D::Transform(a0);
	}
	class wxPoint2DInt Transform(const class wxPoint2DInt & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTransform2D *>(this), "Transform");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPoint2DInt>::value) {
				static pybind11::detail::override_caster_t<class wxPoint2DInt> caster;
				return pybind11::detail::cast_ref<class wxPoint2DInt>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPoint2DInt>(std::move(o));
		}
		return wxTransform2D::Transform(a0);
	}
	class wxRect2DInt Transform(const class wxRect2DInt & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTransform2D *>(this), "Transform");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRect2DInt>::value) {
				static pybind11::detail::override_caster_t<class wxRect2DInt> caster;
				return pybind11::detail::cast_ref<class wxRect2DInt>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRect2DInt>(std::move(o));
		}
		return wxTransform2D::Transform(a0);
	}
	void InverseTransform(class wxPoint2DInt * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTransform2D *>(this), "InverseTransform");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxTransform2D::InverseTransform\"");
	}
	void InverseTransform(class wxRect2DInt * a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTransform2D *>(this), "InverseTransform");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		return wxTransform2D::InverseTransform(a0);
	}
	class wxPoint2DInt InverseTransform(const class wxPoint2DInt & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTransform2D *>(this), "InverseTransform");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPoint2DInt>::value) {
				static pybind11::detail::override_caster_t<class wxPoint2DInt> caster;
				return pybind11::detail::cast_ref<class wxPoint2DInt>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPoint2DInt>(std::move(o));
		}
		return wxTransform2D::InverseTransform(a0);
	}
	class wxRect2DInt InverseTransform(const class wxRect2DInt & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxTransform2D *>(this), "InverseTransform");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxRect2DInt>::value) {
				static pybind11::detail::override_caster_t<class wxRect2DInt> caster;
				return pybind11::detail::cast_ref<class wxRect2DInt>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxRect2DInt>(std::move(o));
		}
		return wxTransform2D::InverseTransform(a0);
	}
};

// wxAffineMatrix2DBase file: line:38
struct PyCallBack_wxAffineMatrix2DBase : public wxAffineMatrix2DBase {
	using wxAffineMatrix2DBase::wxAffineMatrix2DBase;

	void Set(const struct wxMatrix2D & a0, const class wxPoint2DDouble & a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2DBase *>(this), "Set");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAffineMatrix2DBase::Set\"");
	}
	void Get(struct wxMatrix2D * a0, class wxPoint2DDouble * a1) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2DBase *>(this), "Get");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAffineMatrix2DBase::Get\"");
	}
	void Concat(const class wxAffineMatrix2DBase & a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2DBase *>(this), "Concat");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAffineMatrix2DBase::Concat\"");
	}
	bool Invert() override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2DBase *>(this), "Invert");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAffineMatrix2DBase::Invert\"");
	}
	bool IsIdentity() const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2DBase *>(this), "IsIdentity");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>();
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAffineMatrix2DBase::IsIdentity\"");
	}
	bool IsEqual(const class wxAffineMatrix2DBase & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2DBase *>(this), "IsEqual");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<bool>::value) {
				static pybind11::detail::override_caster_t<bool> caster;
				return pybind11::detail::cast_ref<bool>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<bool>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAffineMatrix2DBase::IsEqual\"");
	}
	void Translate(double a0, double a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2DBase *>(this), "Translate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAffineMatrix2DBase::Translate\"");
	}
	void Scale(double a0, double a1) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2DBase *>(this), "Scale");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0, a1);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAffineMatrix2DBase::Scale\"");
	}
	void Rotate(double a0) override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2DBase *>(this), "Rotate");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<void>::value) {
				static pybind11::detail::override_caster_t<void> caster;
				return pybind11::detail::cast_ref<void>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<void>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAffineMatrix2DBase::Rotate\"");
	}
	class wxPoint2DDouble DoTransformPoint(const class wxPoint2DDouble & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2DBase *>(this), "DoTransformPoint");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPoint2DDouble>::value) {
				static pybind11::detail::override_caster_t<class wxPoint2DDouble> caster;
				return pybind11::detail::cast_ref<class wxPoint2DDouble>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPoint2DDouble>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAffineMatrix2DBase::DoTransformPoint\"");
	}
	class wxPoint2DDouble DoTransformDistance(const class wxPoint2DDouble & a0) const override {
		pybind11::gil_scoped_acquire gil;
		pybind11::function overload = pybind11::get_overload(static_cast<const wxAffineMatrix2DBase *>(this), "DoTransformDistance");
		if (overload) {
			auto o = overload.operator()<pybind11::return_value_policy::reference>(a0);
			if (pybind11::detail::cast_is_temporary_value_reference<class wxPoint2DDouble>::value) {
				static pybind11::detail::override_caster_t<class wxPoint2DDouble> caster;
				return pybind11::detail::cast_ref<class wxPoint2DDouble>(std::move(o), caster);
			}
			else return pybind11::detail::cast_safe<class wxPoint2DDouble>(std::move(o));
		}
		pybind11::pybind11_fail("Tried to call pure virtual function \"wxAffineMatrix2DBase::DoTransformDistance\"");
	}
};

void bind_unknown_unknown_88(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // wxRect2DInt file: line:651
		pybind11::class_<wxRect2DInt, std::shared_ptr<wxRect2DInt>> cl(M(""), "wxRect2DInt", "");
		cl.def( pybind11::init( [](){ return new wxRect2DInt(); } ) );
		cl.def( pybind11::init<const class wxRect &>(), pybind11::arg("r") );

		cl.def( pybind11::init<int, int, int, int>(), pybind11::arg("x"), pybind11::arg("y"), pybind11::arg("w"), pybind11::arg("h") );

		cl.def( pybind11::init<const class wxPoint2DInt &, const class wxPoint2DInt &>(), pybind11::arg("topLeft"), pybind11::arg("bottomRight") );

		cl.def( pybind11::init<const class wxPoint2DInt &, const class wxSize &>(), pybind11::arg("pos"), pybind11::arg("size") );

		cl.def( pybind11::init( [](wxRect2DInt const &o){ return new wxRect2DInt(o); } ) );
		cl.def_readwrite("m_x", &wxRect2DInt::m_x);
		cl.def_readwrite("m_y", &wxRect2DInt::m_y);
		cl.def_readwrite("m_width", &wxRect2DInt::m_width);
		cl.def_readwrite("m_height", &wxRect2DInt::m_height);
		cl.def("GetPosition", (class wxPoint2DInt (wxRect2DInt::*)() const) &wxRect2DInt::GetPosition, "C++: wxRect2DInt::GetPosition() const --> class wxPoint2DInt");
		cl.def("GetSize", (class wxSize (wxRect2DInt::*)() const) &wxRect2DInt::GetSize, "C++: wxRect2DInt::GetSize() const --> class wxSize");
		cl.def("GetLeft", (int (wxRect2DInt::*)() const) &wxRect2DInt::GetLeft, "C++: wxRect2DInt::GetLeft() const --> int");
		cl.def("SetLeft", (void (wxRect2DInt::*)(int)) &wxRect2DInt::SetLeft, "C++: wxRect2DInt::SetLeft(int) --> void", pybind11::arg("n"));
		cl.def("MoveLeftTo", (void (wxRect2DInt::*)(int)) &wxRect2DInt::MoveLeftTo, "C++: wxRect2DInt::MoveLeftTo(int) --> void", pybind11::arg("n"));
		cl.def("GetTop", (int (wxRect2DInt::*)() const) &wxRect2DInt::GetTop, "C++: wxRect2DInt::GetTop() const --> int");
		cl.def("SetTop", (void (wxRect2DInt::*)(int)) &wxRect2DInt::SetTop, "C++: wxRect2DInt::SetTop(int) --> void", pybind11::arg("n"));
		cl.def("MoveTopTo", (void (wxRect2DInt::*)(int)) &wxRect2DInt::MoveTopTo, "C++: wxRect2DInt::MoveTopTo(int) --> void", pybind11::arg("n"));
		cl.def("GetBottom", (int (wxRect2DInt::*)() const) &wxRect2DInt::GetBottom, "C++: wxRect2DInt::GetBottom() const --> int");
		cl.def("SetBottom", (void (wxRect2DInt::*)(int)) &wxRect2DInt::SetBottom, "C++: wxRect2DInt::SetBottom(int) --> void", pybind11::arg("n"));
		cl.def("MoveBottomTo", (void (wxRect2DInt::*)(int)) &wxRect2DInt::MoveBottomTo, "C++: wxRect2DInt::MoveBottomTo(int) --> void", pybind11::arg("n"));
		cl.def("GetRight", (int (wxRect2DInt::*)() const) &wxRect2DInt::GetRight, "C++: wxRect2DInt::GetRight() const --> int");
		cl.def("SetRight", (void (wxRect2DInt::*)(int)) &wxRect2DInt::SetRight, "C++: wxRect2DInt::SetRight(int) --> void", pybind11::arg("n"));
		cl.def("MoveRightTo", (void (wxRect2DInt::*)(int)) &wxRect2DInt::MoveRightTo, "C++: wxRect2DInt::MoveRightTo(int) --> void", pybind11::arg("n"));
		cl.def("GetLeftTop", (class wxPoint2DInt (wxRect2DInt::*)() const) &wxRect2DInt::GetLeftTop, "C++: wxRect2DInt::GetLeftTop() const --> class wxPoint2DInt");
		cl.def("SetLeftTop", (void (wxRect2DInt::*)(const class wxPoint2DInt &)) &wxRect2DInt::SetLeftTop, "C++: wxRect2DInt::SetLeftTop(const class wxPoint2DInt &) --> void", pybind11::arg("pt"));
		cl.def("MoveLeftTopTo", (void (wxRect2DInt::*)(const class wxPoint2DInt &)) &wxRect2DInt::MoveLeftTopTo, "C++: wxRect2DInt::MoveLeftTopTo(const class wxPoint2DInt &) --> void", pybind11::arg("pt"));
		cl.def("GetLeftBottom", (class wxPoint2DInt (wxRect2DInt::*)() const) &wxRect2DInt::GetLeftBottom, "C++: wxRect2DInt::GetLeftBottom() const --> class wxPoint2DInt");
		cl.def("SetLeftBottom", (void (wxRect2DInt::*)(const class wxPoint2DInt &)) &wxRect2DInt::SetLeftBottom, "C++: wxRect2DInt::SetLeftBottom(const class wxPoint2DInt &) --> void", pybind11::arg("pt"));
		cl.def("MoveLeftBottomTo", (void (wxRect2DInt::*)(const class wxPoint2DInt &)) &wxRect2DInt::MoveLeftBottomTo, "C++: wxRect2DInt::MoveLeftBottomTo(const class wxPoint2DInt &) --> void", pybind11::arg("pt"));
		cl.def("GetRightTop", (class wxPoint2DInt (wxRect2DInt::*)() const) &wxRect2DInt::GetRightTop, "C++: wxRect2DInt::GetRightTop() const --> class wxPoint2DInt");
		cl.def("SetRightTop", (void (wxRect2DInt::*)(const class wxPoint2DInt &)) &wxRect2DInt::SetRightTop, "C++: wxRect2DInt::SetRightTop(const class wxPoint2DInt &) --> void", pybind11::arg("pt"));
		cl.def("MoveRightTopTo", (void (wxRect2DInt::*)(const class wxPoint2DInt &)) &wxRect2DInt::MoveRightTopTo, "C++: wxRect2DInt::MoveRightTopTo(const class wxPoint2DInt &) --> void", pybind11::arg("pt"));
		cl.def("GetRightBottom", (class wxPoint2DInt (wxRect2DInt::*)() const) &wxRect2DInt::GetRightBottom, "C++: wxRect2DInt::GetRightBottom() const --> class wxPoint2DInt");
		cl.def("SetRightBottom", (void (wxRect2DInt::*)(const class wxPoint2DInt &)) &wxRect2DInt::SetRightBottom, "C++: wxRect2DInt::SetRightBottom(const class wxPoint2DInt &) --> void", pybind11::arg("pt"));
		cl.def("MoveRightBottomTo", (void (wxRect2DInt::*)(const class wxPoint2DInt &)) &wxRect2DInt::MoveRightBottomTo, "C++: wxRect2DInt::MoveRightBottomTo(const class wxPoint2DInt &) --> void", pybind11::arg("pt"));
		cl.def("GetCentre", (class wxPoint2DInt (wxRect2DInt::*)() const) &wxRect2DInt::GetCentre, "C++: wxRect2DInt::GetCentre() const --> class wxPoint2DInt");
		cl.def("SetCentre", (void (wxRect2DInt::*)(const class wxPoint2DInt &)) &wxRect2DInt::SetCentre, "C++: wxRect2DInt::SetCentre(const class wxPoint2DInt &) --> void", pybind11::arg("pt"));
		cl.def("MoveCentreTo", (void (wxRect2DInt::*)(const class wxPoint2DInt &)) &wxRect2DInt::MoveCentreTo, "C++: wxRect2DInt::MoveCentreTo(const class wxPoint2DInt &) --> void", pybind11::arg("pt"));
		cl.def("GetOutCode", (enum wxOutCode (wxRect2DInt::*)(const class wxPoint2DInt &) const) &wxRect2DInt::GetOutCode, "C++: wxRect2DInt::GetOutCode(const class wxPoint2DInt &) const --> enum wxOutCode", pybind11::arg("pt"));
		cl.def("GetOutcode", (enum wxOutCode (wxRect2DInt::*)(const class wxPoint2DInt &) const) &wxRect2DInt::GetOutcode, "C++: wxRect2DInt::GetOutcode(const class wxPoint2DInt &) const --> enum wxOutCode", pybind11::arg("pt"));
		cl.def("Contains", (bool (wxRect2DInt::*)(const class wxPoint2DInt &) const) &wxRect2DInt::Contains, "C++: wxRect2DInt::Contains(const class wxPoint2DInt &) const --> bool", pybind11::arg("pt"));
		cl.def("Contains", (bool (wxRect2DInt::*)(const class wxRect2DInt &) const) &wxRect2DInt::Contains, "C++: wxRect2DInt::Contains(const class wxRect2DInt &) const --> bool", pybind11::arg("rect"));
		cl.def("IsEmpty", (bool (wxRect2DInt::*)() const) &wxRect2DInt::IsEmpty, "C++: wxRect2DInt::IsEmpty() const --> bool");
		cl.def("HaveEqualSize", (bool (wxRect2DInt::*)(const class wxRect2DInt &) const) &wxRect2DInt::HaveEqualSize, "C++: wxRect2DInt::HaveEqualSize(const class wxRect2DInt &) const --> bool", pybind11::arg("rect"));
		cl.def("Inset", (void (wxRect2DInt::*)(int, int)) &wxRect2DInt::Inset, "C++: wxRect2DInt::Inset(int, int) --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("Inset", (void (wxRect2DInt::*)(int, int, int, int)) &wxRect2DInt::Inset, "C++: wxRect2DInt::Inset(int, int, int, int) --> void", pybind11::arg("left"), pybind11::arg("top"), pybind11::arg("right"), pybind11::arg("bottom"));
		cl.def("Offset", (void (wxRect2DInt::*)(const class wxPoint2DInt &)) &wxRect2DInt::Offset, "C++: wxRect2DInt::Offset(const class wxPoint2DInt &) --> void", pybind11::arg("pt"));
		cl.def("ConstrainTo", (void (wxRect2DInt::*)(const class wxRect2DInt &)) &wxRect2DInt::ConstrainTo, "C++: wxRect2DInt::ConstrainTo(const class wxRect2DInt &) --> void", pybind11::arg("rect"));
		cl.def("Interpolate", (class wxPoint2DInt (wxRect2DInt::*)(int, int)) &wxRect2DInt::Interpolate, "C++: wxRect2DInt::Interpolate(int, int) --> class wxPoint2DInt", pybind11::arg("widthfactor"), pybind11::arg("heightfactor"));
		cl.def_static("Intersect", (void (*)(const class wxRect2DInt &, const class wxRect2DInt &, class wxRect2DInt *)) &wxRect2DInt::Intersect, "C++: wxRect2DInt::Intersect(const class wxRect2DInt &, const class wxRect2DInt &, class wxRect2DInt *) --> void", pybind11::arg("src1"), pybind11::arg("src2"), pybind11::arg("dest"));
		cl.def("Intersect", (void (wxRect2DInt::*)(const class wxRect2DInt &)) &wxRect2DInt::Intersect, "C++: wxRect2DInt::Intersect(const class wxRect2DInt &) --> void", pybind11::arg("otherRect"));
		cl.def("CreateIntersection", (class wxRect2DInt (wxRect2DInt::*)(const class wxRect2DInt &) const) &wxRect2DInt::CreateIntersection, "C++: wxRect2DInt::CreateIntersection(const class wxRect2DInt &) const --> class wxRect2DInt", pybind11::arg("otherRect"));
		cl.def("Intersects", (bool (wxRect2DInt::*)(const class wxRect2DInt &) const) &wxRect2DInt::Intersects, "C++: wxRect2DInt::Intersects(const class wxRect2DInt &) const --> bool", pybind11::arg("rect"));
		cl.def_static("Union", (void (*)(const class wxRect2DInt &, const class wxRect2DInt &, class wxRect2DInt *)) &wxRect2DInt::Union, "C++: wxRect2DInt::Union(const class wxRect2DInt &, const class wxRect2DInt &, class wxRect2DInt *) --> void", pybind11::arg("src1"), pybind11::arg("src2"), pybind11::arg("dest"));
		cl.def("Union", (void (wxRect2DInt::*)(const class wxRect2DInt &)) &wxRect2DInt::Union, "C++: wxRect2DInt::Union(const class wxRect2DInt &) --> void", pybind11::arg("otherRect"));
		cl.def("Union", (void (wxRect2DInt::*)(const class wxPoint2DInt &)) &wxRect2DInt::Union, "C++: wxRect2DInt::Union(const class wxPoint2DInt &) --> void", pybind11::arg("pt"));
		cl.def("CreateUnion", (class wxRect2DInt (wxRect2DInt::*)(const class wxRect2DInt &) const) &wxRect2DInt::CreateUnion, "C++: wxRect2DInt::CreateUnion(const class wxRect2DInt &) const --> class wxRect2DInt", pybind11::arg("otherRect"));
		cl.def("Scale", (void (wxRect2DInt::*)(int)) &wxRect2DInt::Scale, "C++: wxRect2DInt::Scale(int) --> void", pybind11::arg("f"));
		cl.def("Scale", (void (wxRect2DInt::*)(int, int)) &wxRect2DInt::Scale, "C++: wxRect2DInt::Scale(int, int) --> void", pybind11::arg("num"), pybind11::arg("denum"));
		cl.def("assign", (class wxRect2DInt & (wxRect2DInt::*)(const class wxRect2DInt &)) &wxRect2DInt::operator=, "C++: wxRect2DInt::operator=(const class wxRect2DInt &) --> class wxRect2DInt &", pybind11::return_value_policy::automatic, pybind11::arg("rect"));
		cl.def("__eq__", (bool (wxRect2DInt::*)(const class wxRect2DInt &) const) &wxRect2DInt::operator==, "C++: wxRect2DInt::operator==(const class wxRect2DInt &) const --> bool", pybind11::arg("rect"));
		cl.def("__ne__", (bool (wxRect2DInt::*)(const class wxRect2DInt &) const) &wxRect2DInt::operator!=, "C++: wxRect2DInt::operator!=(const class wxRect2DInt &) const --> bool", pybind11::arg("rect"));
	}
	{ // wxTransform2D file: line:786
		pybind11::class_<wxTransform2D, std::shared_ptr<wxTransform2D>, PyCallBack_wxTransform2D> cl(M(""), "wxTransform2D", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxTransform2D(); } ) );
		cl.def("Transform", (void (wxTransform2D::*)(class wxPoint2DInt *) const) &wxTransform2D::Transform, "C++: wxTransform2D::Transform(class wxPoint2DInt *) const --> void", pybind11::arg("pt"));
		cl.def("Transform", (void (wxTransform2D::*)(class wxRect2DInt *) const) &wxTransform2D::Transform, "C++: wxTransform2D::Transform(class wxRect2DInt *) const --> void", pybind11::arg("r"));
		cl.def("Transform", (class wxPoint2DInt (wxTransform2D::*)(const class wxPoint2DInt &) const) &wxTransform2D::Transform, "C++: wxTransform2D::Transform(const class wxPoint2DInt &) const --> class wxPoint2DInt", pybind11::arg("pt"));
		cl.def("Transform", (class wxRect2DInt (wxTransform2D::*)(const class wxRect2DInt &) const) &wxTransform2D::Transform, "C++: wxTransform2D::Transform(const class wxRect2DInt &) const --> class wxRect2DInt", pybind11::arg("r"));
		cl.def("InverseTransform", (void (wxTransform2D::*)(class wxPoint2DInt *) const) &wxTransform2D::InverseTransform, "C++: wxTransform2D::InverseTransform(class wxPoint2DInt *) const --> void", pybind11::arg("pt"));
		cl.def("InverseTransform", (void (wxTransform2D::*)(class wxRect2DInt *) const) &wxTransform2D::InverseTransform, "C++: wxTransform2D::InverseTransform(class wxRect2DInt *) const --> void", pybind11::arg("r"));
		cl.def("InverseTransform", (class wxPoint2DInt (wxTransform2D::*)(const class wxPoint2DInt &) const) &wxTransform2D::InverseTransform, "C++: wxTransform2D::InverseTransform(const class wxPoint2DInt &) const --> class wxPoint2DInt", pybind11::arg("pt"));
		cl.def("InverseTransform", (class wxRect2DInt (wxTransform2D::*)(const class wxRect2DInt &) const) &wxTransform2D::InverseTransform, "C++: wxTransform2D::InverseTransform(const class wxRect2DInt &) const --> class wxRect2DInt", pybind11::arg("r"));
		cl.def("assign", (class wxTransform2D & (wxTransform2D::*)(const class wxTransform2D &)) &wxTransform2D::operator=, "C++: wxTransform2D::operator=(const class wxTransform2D &) --> class wxTransform2D &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	{ // wxMatrix2D file: line:19
		pybind11::class_<wxMatrix2D, std::shared_ptr<wxMatrix2D>> cl(M(""), "wxMatrix2D", "");
		cl.def( pybind11::init( [](){ return new wxMatrix2D(); } ), "doc" );
		cl.def( pybind11::init( [](double const & a0){ return new wxMatrix2D(a0); } ), "doc" , pybind11::arg("v11"));
		cl.def( pybind11::init( [](double const & a0, double const & a1){ return new wxMatrix2D(a0, a1); } ), "doc" , pybind11::arg("v11"), pybind11::arg("v12"));
		cl.def( pybind11::init( [](double const & a0, double const & a1, double const & a2){ return new wxMatrix2D(a0, a1, a2); } ), "doc" , pybind11::arg("v11"), pybind11::arg("v12"), pybind11::arg("v21"));
		cl.def( pybind11::init<double, double, double, double>(), pybind11::arg("v11"), pybind11::arg("v12"), pybind11::arg("v21"), pybind11::arg("v22") );

		cl.def_readwrite("m_11", &wxMatrix2D::m_11);
		cl.def_readwrite("m_12", &wxMatrix2D::m_12);
		cl.def_readwrite("m_21", &wxMatrix2D::m_21);
		cl.def_readwrite("m_22", &wxMatrix2D::m_22);
	}
	{ // wxAffineMatrix2DBase file: line:38
		pybind11::class_<wxAffineMatrix2DBase, std::shared_ptr<wxAffineMatrix2DBase>, PyCallBack_wxAffineMatrix2DBase> cl(M(""), "wxAffineMatrix2DBase", "");
		cl.def( pybind11::init( [](){ return new PyCallBack_wxAffineMatrix2DBase(); } ) );
		cl.def(pybind11::init<PyCallBack_wxAffineMatrix2DBase const &>());
		cl.def("Set", (void (wxAffineMatrix2DBase::*)(const struct wxMatrix2D &, const class wxPoint2DDouble &)) &wxAffineMatrix2DBase::Set, "C++: wxAffineMatrix2DBase::Set(const struct wxMatrix2D &, const class wxPoint2DDouble &) --> void", pybind11::arg("mat2D"), pybind11::arg("tr"));
		cl.def("Get", (void (wxAffineMatrix2DBase::*)(struct wxMatrix2D *, class wxPoint2DDouble *) const) &wxAffineMatrix2DBase::Get, "C++: wxAffineMatrix2DBase::Get(struct wxMatrix2D *, class wxPoint2DDouble *) const --> void", pybind11::arg("mat2D"), pybind11::arg("tr"));
		cl.def("Concat", (void (wxAffineMatrix2DBase::*)(const class wxAffineMatrix2DBase &)) &wxAffineMatrix2DBase::Concat, "C++: wxAffineMatrix2DBase::Concat(const class wxAffineMatrix2DBase &) --> void", pybind11::arg("t"));
		cl.def("Invert", (bool (wxAffineMatrix2DBase::*)()) &wxAffineMatrix2DBase::Invert, "C++: wxAffineMatrix2DBase::Invert() --> bool");
		cl.def("IsIdentity", (bool (wxAffineMatrix2DBase::*)() const) &wxAffineMatrix2DBase::IsIdentity, "C++: wxAffineMatrix2DBase::IsIdentity() const --> bool");
		cl.def("IsEqual", (bool (wxAffineMatrix2DBase::*)(const class wxAffineMatrix2DBase &) const) &wxAffineMatrix2DBase::IsEqual, "C++: wxAffineMatrix2DBase::IsEqual(const class wxAffineMatrix2DBase &) const --> bool", pybind11::arg("t"));
		cl.def("__eq__", (bool (wxAffineMatrix2DBase::*)(const class wxAffineMatrix2DBase &) const) &wxAffineMatrix2DBase::operator==, "C++: wxAffineMatrix2DBase::operator==(const class wxAffineMatrix2DBase &) const --> bool", pybind11::arg("t"));
		cl.def("__ne__", (bool (wxAffineMatrix2DBase::*)(const class wxAffineMatrix2DBase &) const) &wxAffineMatrix2DBase::operator!=, "C++: wxAffineMatrix2DBase::operator!=(const class wxAffineMatrix2DBase &) const --> bool", pybind11::arg("t"));
		cl.def("Translate", (void (wxAffineMatrix2DBase::*)(double, double)) &wxAffineMatrix2DBase::Translate, "C++: wxAffineMatrix2DBase::Translate(double, double) --> void", pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("Scale", (void (wxAffineMatrix2DBase::*)(double, double)) &wxAffineMatrix2DBase::Scale, "C++: wxAffineMatrix2DBase::Scale(double, double) --> void", pybind11::arg("xScale"), pybind11::arg("yScale"));
		cl.def("Rotate", (void (wxAffineMatrix2DBase::*)(double)) &wxAffineMatrix2DBase::Rotate, "C++: wxAffineMatrix2DBase::Rotate(double) --> void", pybind11::arg("ccRadians"));
		cl.def("Mirror", [](wxAffineMatrix2DBase &o) -> void { return o.Mirror(); }, "");
		cl.def("Mirror", (void (wxAffineMatrix2DBase::*)(int)) &wxAffineMatrix2DBase::Mirror, "C++: wxAffineMatrix2DBase::Mirror(int) --> void", pybind11::arg("direction"));
		cl.def("TransformPoint", (class wxPoint2DDouble (wxAffineMatrix2DBase::*)(const class wxPoint2DDouble &) const) &wxAffineMatrix2DBase::TransformPoint, "C++: wxAffineMatrix2DBase::TransformPoint(const class wxPoint2DDouble &) const --> class wxPoint2DDouble", pybind11::arg("src"));
		cl.def("TransformPoint", (void (wxAffineMatrix2DBase::*)(double *, double *) const) &wxAffineMatrix2DBase::TransformPoint, "C++: wxAffineMatrix2DBase::TransformPoint(double *, double *) const --> void", pybind11::arg("x"), pybind11::arg("y"));
		cl.def("TransformDistance", (class wxPoint2DDouble (wxAffineMatrix2DBase::*)(const class wxPoint2DDouble &) const) &wxAffineMatrix2DBase::TransformDistance, "C++: wxAffineMatrix2DBase::TransformDistance(const class wxPoint2DDouble &) const --> class wxPoint2DDouble", pybind11::arg("src"));
		cl.def("TransformDistance", (void (wxAffineMatrix2DBase::*)(double *, double *) const) &wxAffineMatrix2DBase::TransformDistance, "C++: wxAffineMatrix2DBase::TransformDistance(double *, double *) const --> void", pybind11::arg("dx"), pybind11::arg("dy"));
		cl.def("assign", (class wxAffineMatrix2DBase & (wxAffineMatrix2DBase::*)(const class wxAffineMatrix2DBase &)) &wxAffineMatrix2DBase::operator=, "C++: wxAffineMatrix2DBase::operator=(const class wxAffineMatrix2DBase &) --> class wxAffineMatrix2DBase &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
}

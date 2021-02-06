#include <gal/color4d.h> // EDA_COLOR_T
#include <gal/color4d.h> // KIGFX::COLOR4D
#include <gal/color4d.h> // KIGFX::from_json
#include <gal/color4d.h> // KIGFX::to_json
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

void bind_gal_color4d_1(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	{ // KIGFX::COLOR4D file:gal/color4d.h line:98
		pybind11::class_<KIGFX::COLOR4D, std::shared_ptr<KIGFX::COLOR4D>> cl(M("KIGFX"), "COLOR4D", "A color representation with 4 components: red, green, blue, alpha.");
		cl.def( pybind11::init( [](){ return new KIGFX::COLOR4D(); } ) );
		cl.def( pybind11::init<double, double, double, double>(), pybind11::arg("aRed"), pybind11::arg("aGreen"), pybind11::arg("aBlue"), pybind11::arg("aAlpha") );

		cl.def( pybind11::init<enum EDA_COLOR_T>(), pybind11::arg("aColor") );

		cl.def( pybind11::init( [](KIGFX::COLOR4D const &o){ return new KIGFX::COLOR4D(o); } ) );
		cl.def_readwrite("r", &KIGFX::COLOR4D::r);
		cl.def_readwrite("g", &KIGFX::COLOR4D::g);
		cl.def_readwrite("b", &KIGFX::COLOR4D::b);
		cl.def_readwrite("a", &KIGFX::COLOR4D::a);
		cl.def("FromCSSRGBA", [](KIGFX::COLOR4D &o, int const & a0, int const & a1, int const & a2) -> KIGFX::COLOR4D & { return o.FromCSSRGBA(a0, a1, a2); }, "", pybind11::return_value_policy::automatic, pybind11::arg("aRed"), pybind11::arg("aGreen"), pybind11::arg("aBlue"));
		cl.def("FromCSSRGBA", (class KIGFX::COLOR4D & (KIGFX::COLOR4D::*)(int, int, int, double)) &KIGFX::COLOR4D::FromCSSRGBA, "Initialize the color from a RGBA value with 0-255 red/green/blue and 0-1 alpha.\n\n Suitable for taking the values directly from the \"CSS syntax\" from ToWxString.\n\n \n this color.\n\nC++: KIGFX::COLOR4D::FromCSSRGBA(int, int, int, double) --> class KIGFX::COLOR4D &", pybind11::return_value_policy::automatic, pybind11::arg("aRed"), pybind11::arg("aGreen"), pybind11::arg("aBlue"), pybind11::arg("aAlpha"));
		cl.def("ToHSL", (void (KIGFX::COLOR4D::*)(double &, double &, double &) const) &KIGFX::COLOR4D::ToHSL, "Converts current color (stored in RGB) to HSL format.\n\n \n is the conversion result for hue component, in degrees 0 ... 360.0.\n \n\n is the conversion result for saturation component (0 ... 1.0).\n \n\n is conversion result for value component (0 ... 1.0).\n \n\n saturation is set to 0.0 for black color if r = g = b,\n\nC++: KIGFX::COLOR4D::ToHSL(double &, double &, double &) const --> void", pybind11::arg("aOutHue"), pybind11::arg("aOutSaturation"), pybind11::arg("aOutValue"));
		cl.def("FromHSL", (void (KIGFX::COLOR4D::*)(double, double, double)) &KIGFX::COLOR4D::FromHSL, "Change currently used color to the one given by hue, saturation and lightness parameters.\n\n \n is hue component, in degrees (0.0 - 360.0).\n \n\n is saturation component (0.0 - 1.0).\n \n\n is lightness component (0.0 - 1.0).\n\nC++: KIGFX::COLOR4D::FromHSL(double, double, double) --> void", pybind11::arg("aInHue"), pybind11::arg("aInSaturation"), pybind11::arg("aInLightness"));
		cl.def("Brighten", (class KIGFX::COLOR4D & (KIGFX::COLOR4D::*)(double)) &KIGFX::COLOR4D::Brighten, "Makes the color brighter by a given factor.\n\n \n Specifies how bright the color should become (valid values: 0.0 .. 1.0).\n \n\n COLOR4D& Brightened color.\n\nC++: KIGFX::COLOR4D::Brighten(double) --> class KIGFX::COLOR4D &", pybind11::return_value_policy::automatic, pybind11::arg("aFactor"));
		cl.def("Darken", (class KIGFX::COLOR4D & (KIGFX::COLOR4D::*)(double)) &KIGFX::COLOR4D::Darken, "Makes the color darker by a given factor.\n\n \n Specifies how dark the color should become (valid values: 0.0 .. 1.0).\n \n\n COLOR4D& Darkened color.\n\nC++: KIGFX::COLOR4D::Darken(double) --> class KIGFX::COLOR4D &", pybind11::return_value_policy::automatic, pybind11::arg("aFactor"));
		cl.def("Invert", (class KIGFX::COLOR4D & (KIGFX::COLOR4D::*)()) &KIGFX::COLOR4D::Invert, "Makes the color inverted, alpha remains the same.\n\n \n COLOR4D& Inverted color.\n\nC++: KIGFX::COLOR4D::Invert() --> class KIGFX::COLOR4D &", pybind11::return_value_policy::automatic);
		cl.def("Saturate", (class KIGFX::COLOR4D & (KIGFX::COLOR4D::*)(double)) &KIGFX::COLOR4D::Saturate, "Saturates the color to a given factor (in HSV model)\n\nC++: KIGFX::COLOR4D::Saturate(double) --> class KIGFX::COLOR4D &", pybind11::return_value_policy::automatic, pybind11::arg("aFactor"));
		cl.def("Brightened", (class KIGFX::COLOR4D (KIGFX::COLOR4D::*)(double) const) &KIGFX::COLOR4D::Brightened, "Return a color that is brighter by a given factor, without modifying object.\n\n \n Specifies how bright the color should become (valid values: 0.0 .. 1.0).\n \n\n COLOR4D Highlighted color.\n\nC++: KIGFX::COLOR4D::Brightened(double) const --> class KIGFX::COLOR4D", pybind11::arg("aFactor"));
		cl.def("Darkened", (class KIGFX::COLOR4D (KIGFX::COLOR4D::*)(double) const) &KIGFX::COLOR4D::Darkened, "Return a color that is darker by a given factor, without modifying object.\n\n \n Specifies how dark the color should become (valid values: 0.0 .. 1.0).\n \n\n COLOR4D Darkened color.\n\nC++: KIGFX::COLOR4D::Darkened(double) const --> class KIGFX::COLOR4D", pybind11::arg("aFactor"));
		cl.def("Mix", (class KIGFX::COLOR4D (KIGFX::COLOR4D::*)(const class KIGFX::COLOR4D &, double) const) &KIGFX::COLOR4D::Mix, "Return a color that is mixed with the input by a factor.\n\n \n Specifies how much of the original color to keep (valid values: 0.0 .. 1.0).\n \n\n COLOR4D Mixed color.\n\nC++: KIGFX::COLOR4D::Mix(const class KIGFX::COLOR4D &, double) const --> class KIGFX::COLOR4D", pybind11::arg("aColor"), pybind11::arg("aFactor"));
		cl.def("WithAlpha", (class KIGFX::COLOR4D (KIGFX::COLOR4D::*)(double) const) &KIGFX::COLOR4D::WithAlpha, "Return a color with the same color, but the given alpha.\n\n \n specifies the alpha of the new color\n \n\n COLOR4D color with that alpha\n\nC++: KIGFX::COLOR4D::WithAlpha(double) const --> class KIGFX::COLOR4D", pybind11::arg("aAlpha"));
		cl.def("Inverted", (class KIGFX::COLOR4D (KIGFX::COLOR4D::*)() const) &KIGFX::COLOR4D::Inverted, "Returns an inverted color, alpha remains the same.\n\n \n COLOR4D& Inverted color.\n\nC++: KIGFX::COLOR4D::Inverted() const --> class KIGFX::COLOR4D");
		cl.def("GetBrightness", (double (KIGFX::COLOR4D::*)() const) &KIGFX::COLOR4D::GetBrightness, "Returns the brightness value of the color ranged from 0.0 to 1.0.\n\n \n The brightness value.\n\nC++: KIGFX::COLOR4D::GetBrightness() const --> double");
		cl.def("ToHSV", [](KIGFX::COLOR4D const &o, double & a0, double & a1, double & a2) -> void { return o.ToHSV(a0, a1, a2); }, "", pybind11::arg("aOutHue"), pybind11::arg("aOutSaturation"), pybind11::arg("aOutValue"));
		cl.def("ToHSV", (void (KIGFX::COLOR4D::*)(double &, double &, double &, bool) const) &KIGFX::COLOR4D::ToHSV, "Convert current color (stored in RGB) to HSV format.\n\n \n is the conversion result for hue component, in degrees 0 ... 360.0.\n \n\n is the conversion result for saturation component (0 ... 1.0).\n \n\n is conversion result for value component (0 ... 1.0).\n \n\n controls the way hue is defined when r = v = b\n \n\n saturation is set to 0.0 for black color (r = v = b = 0), and if r = v = b,\n hue is set to 0.0 if aAlwaysDefineHue = true, and set to NAN if aAlwaysDefineHue = false.\n this option is useful to convert a 4D color to a legacy color, because Red has hue = 0,\n therefore aAlwaysDefineHue = false makes difference between Red and Gray colors.\n\nC++: KIGFX::COLOR4D::ToHSV(double &, double &, double &, bool) const --> void", pybind11::arg("aOutHue"), pybind11::arg("aOutSaturation"), pybind11::arg("aOutValue"), pybind11::arg("aAlwaysDefineHue"));
		cl.def("FromHSV", (void (KIGFX::COLOR4D::*)(double, double, double)) &KIGFX::COLOR4D::FromHSV, "Changes currently used color to the one given by hue, saturation and value parameters.\n\n \n is hue component, in degrees.\n \n\n is saturation component.\n \n\n is value component.\n\nC++: KIGFX::COLOR4D::FromHSV(double, double, double) --> void", pybind11::arg("aInH"), pybind11::arg("aInS"), pybind11::arg("aInV"));
		cl.def_static("FindNearestLegacyColor", (enum EDA_COLOR_T (*)(int, int, int)) &KIGFX::COLOR4D::FindNearestLegacyColor, "Returns a legacy color ID that is closest to the given 8-bit RGB values.\n\nC++: KIGFX::COLOR4D::FindNearestLegacyColor(int, int, int) --> enum EDA_COLOR_T", pybind11::arg("aR"), pybind11::arg("aG"), pybind11::arg("aB"));
		cl.def("assign", (class KIGFX::COLOR4D & (KIGFX::COLOR4D::*)(const class KIGFX::COLOR4D &)) &KIGFX::COLOR4D::operator=, "C++: KIGFX::COLOR4D::operator=(const class KIGFX::COLOR4D &) --> class KIGFX::COLOR4D &", pybind11::return_value_policy::automatic, pybind11::arg(""));
	}
	// KIGFX::to_json(int &, const class KIGFX::COLOR4D &) file:gal/color4d.h line:384
	M("KIGFX").def("to_json", (void (*)(int &, const class KIGFX::COLOR4D &)) &KIGFX::to_json, "C++: KIGFX::to_json(int &, const class KIGFX::COLOR4D &) --> void", pybind11::arg("aJson"), pybind11::arg("aColor"));

	// KIGFX::from_json(const int &, class KIGFX::COLOR4D &) file:gal/color4d.h line:387
	M("KIGFX").def("from_json", (void (*)(const int &, class KIGFX::COLOR4D &)) &KIGFX::from_json, "C++: KIGFX::from_json(const int &, class KIGFX::COLOR4D &) --> void", pybind11::arg("aJson"), pybind11::arg("aColor"));

}

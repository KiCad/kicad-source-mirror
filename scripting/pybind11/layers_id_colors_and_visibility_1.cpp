#include <iterator> // __gnu_cxx::__normal_iterator
#include <layers_id_colors_and_visibility.h> // FlipLayer
#include <layers_id_colors_and_visibility.h> // FlipLayerMask
#include <layers_id_colors_and_visibility.h> // GetNetnameLayer
#include <layers_id_colors_and_visibility.h> // IsBackLayer
#include <layers_id_colors_and_visibility.h> // IsCopperLayer
#include <layers_id_colors_and_visibility.h> // IsDCodeLayer
#include <layers_id_colors_and_visibility.h> // IsFrontLayer
#include <layers_id_colors_and_visibility.h> // IsHoleLayer
#include <layers_id_colors_and_visibility.h> // IsNetCopperLayer
#include <layers_id_colors_and_visibility.h> // IsNetnameLayer
#include <layers_id_colors_and_visibility.h> // IsUserLayer
#include <layers_id_colors_and_visibility.h> // IsViaPadLayer
#include <layers_id_colors_and_visibility.h> // IsZoneLayer
#include <layers_id_colors_and_visibility.h> // LSEQ
#include <layers_id_colors_and_visibility.h> // LSET
#include <layers_id_colors_and_visibility.h> // PCB_LAYER_ID
#include <layers_id_colors_and_visibility.h> // ToLAYER_ID
#include <memory> // std::allocator
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

void bind_layers_id_colors_and_visibility_1(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// IsCopperLayer(int, bool) file:layers_id_colors_and_visibility.h line:816
	M("").def("IsCopperLayer", (bool (*)(int, bool)) &IsCopperLayer, "Tests whether a layer is a copper layer, optionally including synthetic copper layers such\n as LAYER_VIA_THROUGH, LAYER_PAD_FR, etc.\n\n \n\n \n\n\n \n\n\n\n\nC++: IsCopperLayer(int, bool) --> bool", pybind11::arg("aLayerId"), pybind11::arg("aIncludeSyntheticCopperLayers"));

	// IsViaPadLayer(int) file:layers_id_colors_and_visibility.h line:824
	M("").def("IsViaPadLayer", (bool (*)(int)) &IsViaPadLayer, "C++: IsViaPadLayer(int) --> bool", pybind11::arg("aLayer"));

	// IsHoleLayer(int) file:layers_id_colors_and_visibility.h line:831
	M("").def("IsHoleLayer", (bool (*)(int)) &IsHoleLayer, "C++: IsHoleLayer(int) --> bool", pybind11::arg("aLayer"));

	// IsUserLayer(enum PCB_LAYER_ID) file:layers_id_colors_and_visibility.h line:846
	M("").def("IsUserLayer", (bool (*)(enum PCB_LAYER_ID)) &IsUserLayer, "Test whether a layer is a non copper and a non tech layer.\n\n \n = Layer to test\n \n\n true if aLayer is a user layer\n\nC++: IsUserLayer(enum PCB_LAYER_ID) --> bool", pybind11::arg("aLayerId"));

	// IsFrontLayer(enum PCB_LAYER_ID) file:layers_id_colors_and_visibility.h line:868
	M("").def("IsFrontLayer", (bool (*)(enum PCB_LAYER_ID)) &IsFrontLayer, "Layer classification: check if it's a front layer\n\nC++: IsFrontLayer(enum PCB_LAYER_ID) --> bool", pybind11::arg("aLayerId"));

	// IsBackLayer(enum PCB_LAYER_ID) file:layers_id_colors_and_visibility.h line:891
	M("").def("IsBackLayer", (bool (*)(enum PCB_LAYER_ID)) &IsBackLayer, "Layer classification: check if it's a back layer\n\nC++: IsBackLayer(enum PCB_LAYER_ID) --> bool", pybind11::arg("aLayerId"));

	// FlipLayer(enum PCB_LAYER_ID, int) file:layers_id_colors_and_visibility.h line:920
	M("").def("FlipLayer", [](enum PCB_LAYER_ID const & a0) -> PCB_LAYER_ID { return FlipLayer(a0); }, "", pybind11::arg("aLayerId"));
	M("").def("FlipLayer", (enum PCB_LAYER_ID (*)(enum PCB_LAYER_ID, int)) &FlipLayer, "the layer number after flipping an item\n some (not all) layers: external copper, and paired layers( Mask, Paste, solder ... )\n are swapped between front and back sides\n internal layers are flipped only if the copper layers count is known\n \n\n = the PCB_LAYER_ID to flip\n \n\n = the number of copper layers. if 0 (in fact if < 4 )\n  internal layers will be not flipped because the layer count is not known\n\nC++: FlipLayer(enum PCB_LAYER_ID, int) --> enum PCB_LAYER_ID", pybind11::arg("aLayerId"), pybind11::arg("aCopperLayersCount"));

	// FlipLayerMask(class LSET, int) file:layers_id_colors_and_visibility.h line:931
	M("").def("FlipLayerMask", [](class LSET const & a0) -> LSET { return FlipLayerMask(a0); }, "", pybind11::arg("aMask"));
	M("").def("FlipLayerMask", (class LSET (*)(class LSET, int)) &FlipLayerMask, "Calculate the mask layer when flipping a footprint.\n\n BACK and FRONT copper layers, mask, paste, solder layers are swapped\n internal layers are flipped only if the copper layers count is known\n \n\n = the LSET to flip\n \n\n = the number of copper layers. if 0 (in fact if < 4 )\n  internal layers will be not flipped because the layer count is not known\n\nC++: FlipLayerMask(class LSET, int) --> class LSET", pybind11::arg("aMask"), pybind11::arg("aCopperLayersCount"));

	// GetNetnameLayer(int) file:layers_id_colors_and_visibility.h line:937
	M("").def("GetNetnameLayer", (int (*)(int)) &GetNetnameLayer, "Returns a netname layer corresponding to the given layer.\n\nC++: GetNetnameLayer(int) --> int", pybind11::arg("aLayer"));

	// IsNetnameLayer(int) file:layers_id_colors_and_visibility.h line:960
	M("").def("IsNetnameLayer", (bool (*)(int)) &IsNetnameLayer, "Test whether a layer is a netname layer.\n\n \n = Layer to test\n \n\n true if aLayer is a valid netname layer\n\nC++: IsNetnameLayer(int) --> bool", pybind11::arg("aLayer"));

	// IsZoneLayer(int) file:layers_id_colors_and_visibility.h line:967
	M("").def("IsZoneLayer", (bool (*)(int)) &IsZoneLayer, "C++: IsZoneLayer(int) --> bool", pybind11::arg("aLayer"));

	// IsDCodeLayer(int) file:layers_id_colors_and_visibility.h line:973
	M("").def("IsDCodeLayer", (bool (*)(int)) &IsDCodeLayer, "C++: IsDCodeLayer(int) --> bool", pybind11::arg("aLayer"));

	// IsNetCopperLayer(int) file:layers_id_colors_and_visibility.h line:986
	M("").def("IsNetCopperLayer", (bool (*)(int)) &IsNetCopperLayer, "Checks if the given layer is \"net copper\", meaning it is eligible for net coloring.\n\n \n is the layer to test\n \n\n true if the layer is one that participates in net coloring\n\nC++: IsNetCopperLayer(int) --> bool", pybind11::arg("aLayer"));

	// ToLAYER_ID(int) file:layers_id_colors_and_visibility.h line:1004
	M("").def("ToLAYER_ID", (enum PCB_LAYER_ID (*)(int)) &ToLAYER_ID, "C++: ToLAYER_ID(int) --> enum PCB_LAYER_ID", pybind11::arg("aLayer"));

}

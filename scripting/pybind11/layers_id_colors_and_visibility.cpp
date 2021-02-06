#include <iterator> // __gnu_cxx::__normal_iterator
#include <layers_id_colors_and_visibility.h> // GAL_LAYER_ID
#include <layers_id_colors_and_visibility.h> // GAL_SET
#include <layers_id_colors_and_visibility.h> // GERBVIEW_LAYER_ID
#include <layers_id_colors_and_visibility.h> // IsCopperLayer
#include <layers_id_colors_and_visibility.h> // IsNonCopperLayer
#include <layers_id_colors_and_visibility.h> // IsPcbLayer
#include <layers_id_colors_and_visibility.h> // IsValidLayer
#include <layers_id_colors_and_visibility.h> // LAYER_3D_ID
#include <layers_id_colors_and_visibility.h> // LSEQ
#include <layers_id_colors_and_visibility.h> // LSET
#include <layers_id_colors_and_visibility.h> // LayerName
#include <layers_id_colors_and_visibility.h> // PCB_LAYER_ID
#include <layers_id_colors_and_visibility.h> // SCH_LAYER_ID
#include <layers_id_colors_and_visibility.h> // ToGalLayer
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

void bind_layers_id_colors_and_visibility(std::function< pybind11::module &(std::string const &namespace_) > &M)
{
	// ToGalLayer(int) file:layers_id_colors_and_visibility.h line:266
	M("").def("ToGalLayer", (enum GAL_LAYER_ID (*)(int)) &ToGalLayer, "C++: ToGalLayer(int) --> enum GAL_LAYER_ID", pybind11::arg("aInteger"));

	{ // GAL_SET file:layers_id_colors_and_visibility.h line:283
		pybind11::class_<GAL_SET, std::shared_ptr<GAL_SET>> cl(M(""), "GAL_SET", "Helper for storing and iterating over GAL_LAYER_IDs");
		cl.def( pybind11::init( [](){ return new GAL_SET(); } ) );
		cl.def( pybind11::init( [](GAL_SET const &o){ return new GAL_SET(o); } ) );
		cl.def( pybind11::init<const enum GAL_LAYER_ID *, unsigned int>(), pybind11::arg("aArray"), pybind11::arg("aCount") );

		cl.def("set", (class GAL_SET & (GAL_SET::*)()) &GAL_SET::set, "C++: GAL_SET::set() --> class GAL_SET &", pybind11::return_value_policy::automatic);
		cl.def("set", [](GAL_SET &o, int const & a0) -> GAL_SET & { return o.set(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("aPos"));
		cl.def("set", (class GAL_SET & (GAL_SET::*)(int, bool)) &GAL_SET::set, "C++: GAL_SET::set(int, bool) --> class GAL_SET &", pybind11::return_value_policy::automatic, pybind11::arg("aPos"), pybind11::arg("aVal"));
		cl.def("set", [](GAL_SET &o, enum GAL_LAYER_ID const & a0) -> GAL_SET & { return o.set(a0); }, "", pybind11::return_value_policy::automatic, pybind11::arg("aPos"));
		cl.def("set", (class GAL_SET & (GAL_SET::*)(enum GAL_LAYER_ID, bool)) &GAL_SET::set, "C++: GAL_SET::set(enum GAL_LAYER_ID, bool) --> class GAL_SET &", pybind11::return_value_policy::automatic, pybind11::arg("aPos"), pybind11::arg("aVal"));
		cl.def("Contains", (bool (GAL_SET::*)(enum GAL_LAYER_ID)) &GAL_SET::Contains, "C++: GAL_SET::Contains(enum GAL_LAYER_ID) --> bool", pybind11::arg("aPos"));
		cl.def("Seq", (int (GAL_SET::*)() const) &GAL_SET::Seq, "C++: GAL_SET::Seq() const --> int");
		cl.def_static("DefaultVisible", (class GAL_SET (*)()) &GAL_SET::DefaultVisible, "C++: GAL_SET::DefaultVisible() --> class GAL_SET");
	}
	// SCH_LAYER_ID file:layers_id_colors_and_visibility.h line:328
	pybind11::enum_<SCH_LAYER_ID>(M(""), "SCH_LAYER_ID", pybind11::arithmetic(), "Eeschema drawing layers")
		.value("SCH_LAYER_ID_START", SCH_LAYER_ID_START)
		.value("LAYER_WIRE", LAYER_WIRE)
		.value("LAYER_BUS", LAYER_BUS)
		.value("LAYER_JUNCTION", LAYER_JUNCTION)
		.value("LAYER_LOCLABEL", LAYER_LOCLABEL)
		.value("LAYER_GLOBLABEL", LAYER_GLOBLABEL)
		.value("LAYER_HIERLABEL", LAYER_HIERLABEL)
		.value("LAYER_PINNUM", LAYER_PINNUM)
		.value("LAYER_PINNAM", LAYER_PINNAM)
		.value("LAYER_REFERENCEPART", LAYER_REFERENCEPART)
		.value("LAYER_VALUEPART", LAYER_VALUEPART)
		.value("LAYER_FIELDS", LAYER_FIELDS)
		.value("LAYER_DEVICE", LAYER_DEVICE)
		.value("LAYER_NOTES", LAYER_NOTES)
		.value("LAYER_NETNAM", LAYER_NETNAM)
		.value("LAYER_PIN", LAYER_PIN)
		.value("LAYER_SHEET", LAYER_SHEET)
		.value("LAYER_SHEETNAME", LAYER_SHEETNAME)
		.value("LAYER_SHEETFILENAME", LAYER_SHEETFILENAME)
		.value("LAYER_SHEETFIELDS", LAYER_SHEETFIELDS)
		.value("LAYER_SHEETLABEL", LAYER_SHEETLABEL)
		.value("LAYER_NOCONNECT", LAYER_NOCONNECT)
		.value("LAYER_ERC_WARN", LAYER_ERC_WARN)
		.value("LAYER_ERC_ERR", LAYER_ERC_ERR)
		.value("LAYER_DEVICE_BACKGROUND", LAYER_DEVICE_BACKGROUND)
		.value("LAYER_SHEET_BACKGROUND", LAYER_SHEET_BACKGROUND)
		.value("LAYER_SCHEMATIC_GRID", LAYER_SCHEMATIC_GRID)
		.value("LAYER_SCHEMATIC_GRID_AXES", LAYER_SCHEMATIC_GRID_AXES)
		.value("LAYER_SCHEMATIC_BACKGROUND", LAYER_SCHEMATIC_BACKGROUND)
		.value("LAYER_SCHEMATIC_CURSOR", LAYER_SCHEMATIC_CURSOR)
		.value("LAYER_BRIGHTENED", LAYER_BRIGHTENED)
		.value("LAYER_HIDDEN", LAYER_HIDDEN)
		.value("LAYER_SELECTION_SHADOWS", LAYER_SELECTION_SHADOWS)
		.value("LAYER_SCHEMATIC_DRAWINGSHEET", LAYER_SCHEMATIC_DRAWINGSHEET)
		.value("LAYER_BUS_JUNCTION", LAYER_BUS_JUNCTION)
		.value("LAYER_SCHEMATIC_AUX_ITEMS", LAYER_SCHEMATIC_AUX_ITEMS)
		.value("SCH_LAYER_ID_END", SCH_LAYER_ID_END)
		.export_values();

;

	// GERBVIEW_LAYER_ID file:layers_id_colors_and_visibility.h line:385
	pybind11::enum_<GERBVIEW_LAYER_ID>(M(""), "GERBVIEW_LAYER_ID", pybind11::arithmetic(), "GerbView draw layers")
		.value("GERBVIEW_LAYER_ID_START", GERBVIEW_LAYER_ID_START)
		.value("GERBVIEW_LAYER_ID_RESERVED", GERBVIEW_LAYER_ID_RESERVED)
		.value("LAYER_DCODES", LAYER_DCODES)
		.value("LAYER_NEGATIVE_OBJECTS", LAYER_NEGATIVE_OBJECTS)
		.value("LAYER_GERBVIEW_GRID", LAYER_GERBVIEW_GRID)
		.value("LAYER_GERBVIEW_AXES", LAYER_GERBVIEW_AXES)
		.value("LAYER_GERBVIEW_BACKGROUND", LAYER_GERBVIEW_BACKGROUND)
		.value("LAYER_GERBVIEW_DRAWINGSHEET", LAYER_GERBVIEW_DRAWINGSHEET)
		.value("GERBVIEW_LAYER_ID_END", GERBVIEW_LAYER_ID_END)
		.export_values();

;

	// LAYER_3D_ID file:layers_id_colors_and_visibility.h line:410
	pybind11::enum_<LAYER_3D_ID>(M(""), "LAYER_3D_ID", pybind11::arithmetic(), "3D Viewer virtual layers for color settings")
		.value("LAYER_3D_START", LAYER_3D_START)
		.value("LAYER_3D_BACKGROUND_BOTTOM", LAYER_3D_BACKGROUND_BOTTOM)
		.value("LAYER_3D_BACKGROUND_TOP", LAYER_3D_BACKGROUND_TOP)
		.value("LAYER_3D_BOARD", LAYER_3D_BOARD)
		.value("LAYER_3D_COPPER", LAYER_3D_COPPER)
		.value("LAYER_3D_SILKSCREEN_BOTTOM", LAYER_3D_SILKSCREEN_BOTTOM)
		.value("LAYER_3D_SILKSCREEN_TOP", LAYER_3D_SILKSCREEN_TOP)
		.value("LAYER_3D_SOLDERMASK", LAYER_3D_SOLDERMASK)
		.value("LAYER_3D_SOLDERPASTE", LAYER_3D_SOLDERPASTE)
		.value("LAYER_3D_END", LAYER_3D_END)
		.export_values();

;

	// LayerName(int) file:layers_id_colors_and_visibility.h line:434
	M("").def("LayerName", (class wxString (*)(int)) &LayerName, "Returns the string equivalent of a given layer\n \n\n is a valid layer ID\n\nC++: LayerName(int) --> class wxString", pybind11::arg("aLayer"));

	{ // LSEQ file:layers_id_colors_and_visibility.h line:468
		pybind11::class_<LSEQ, std::shared_ptr<LSEQ>> cl(M(""), "LSEQ", "LSEQ is a sequence (and therefore also a set) of PCB_LAYER_IDs.  A sequence provides\n a certain order.\n \n It can also be used as an iterator:\n \n\n      for( LSEQ cu_stack = aSet.CuStack();  cu_stack;  ++cu_stack )\n      {\n          layer_id = *cu_stack;\n          :\n          things to do with layer_id;\n      }\n\n ");
		cl.def( pybind11::init( [](){ return new LSEQ(); } ) );
		cl.def("Rewind", (void (LSEQ::*)()) &LSEQ::Rewind, "C++: LSEQ::Rewind() --> void");
		cl.def("plus_plus", (void (LSEQ::*)()) &LSEQ::operator++, "C++: LSEQ::operator++() --> void");
		cl.def("plus_plus", (void (LSEQ::*)(int)) &LSEQ::operator++, "C++: LSEQ::operator++(int) --> void", pybind11::arg(""));
		cl.def("__mul__", (enum PCB_LAYER_ID (LSEQ::*)() const) &LSEQ::operator*, "C++: LSEQ::operator*() const --> enum PCB_LAYER_ID");
	}
	{ // LSET file:layers_id_colors_and_visibility.h line:507
		pybind11::class_<LSET, std::shared_ptr<LSET>> cl(M(""), "LSET", "LSET is a set of PCB_LAYER_IDs.  It can be converted to numerous purpose LSEQs using\n the various member functions, most of which are based on Seq(). The advantage\n of converting to LSEQ using purposeful code, is it removes any dependency\n on order/sequence inherent in this set.");
		cl.def( pybind11::init( [](){ return new LSET(); } ) );
		cl.def( pybind11::init<enum PCB_LAYER_ID>(), pybind11::arg("aLayer") );

		cl.def( pybind11::init<const enum PCB_LAYER_ID *, unsigned int>(), pybind11::arg("aArray"), pybind11::arg("aCount") );

		cl.def( pybind11::init( [](unsigned int const & a0, int const & a1){ return new LSET(a0, a1); } ), "doc" , pybind11::arg("aIdCount"), pybind11::arg("aFirst"));
		cl.def( pybind11::init( [](LSET const &o){ return new LSET(o); } ) );
		cl.def("Contains", (bool (LSET::*)(enum PCB_LAYER_ID)) &LSET::Contains, "See if the layer set contains a PCB layer.\n\n \n is the layer to check\n \n\n true if the layer is included\n\nC++: LSET::Contains(enum PCB_LAYER_ID) --> bool", pybind11::arg("aLayer"));
		cl.def_static("Name", (const wchar_t * (*)(enum PCB_LAYER_ID)) &LSET::Name, "Return the fixed name association with aLayerId.\n\nC++: LSET::Name(enum PCB_LAYER_ID) --> const wchar_t *", pybind11::return_value_policy::automatic, pybind11::arg("aLayerId"));
		cl.def_static("InternalCuMask", (class LSET (*)()) &LSET::InternalCuMask, "Return a complete set of internal copper layers which is all Cu layers\n except F_Cu and B_Cu.\n\nC++: LSET::InternalCuMask() --> class LSET");
		cl.def_static("FrontAssembly", (class LSET (*)()) &LSET::FrontAssembly, "Return a complete set of all top assembly layers which is all F_SilkS and F_Mask\n\nC++: LSET::FrontAssembly() --> class LSET");
		cl.def_static("BackAssembly", (class LSET (*)()) &LSET::BackAssembly, "Return a complete set of all bottom assembly layers which is all B_SilkS and B_Mask\n\nC++: LSET::BackAssembly() --> class LSET");
		cl.def_static("AllCuMask", []() -> LSET { return LSET::AllCuMask(); }, "");
		cl.def_static("AllCuMask", (class LSET (*)(int)) &LSET::AllCuMask, "Return a mask holding the requested number of Cu PCB_LAYER_IDs.\n\nC++: LSET::AllCuMask(int) --> class LSET", pybind11::arg("aCuLayerCount"));
		cl.def_static("ExternalCuMask", (class LSET (*)()) &LSET::ExternalCuMask, "Return a mask holding the Front and Bottom layers.\n\nC++: LSET::ExternalCuMask() --> class LSET");
		cl.def_static("AllNonCuMask", (class LSET (*)()) &LSET::AllNonCuMask, "Return a mask holding all layer minus CU layers.\n\nC++: LSET::AllNonCuMask() --> class LSET");
		cl.def_static("AllLayersMask", (class LSET (*)()) &LSET::AllLayersMask, "C++: LSET::AllLayersMask() --> class LSET");
		cl.def_static("FrontTechMask", (class LSET (*)()) &LSET::FrontTechMask, "Return a mask holding all technical layers (no CU layer) on front side.\n\nC++: LSET::FrontTechMask() --> class LSET");
		cl.def_static("FrontBoardTechMask", (class LSET (*)()) &LSET::FrontBoardTechMask, "Return a mask holding technical layers used in a board fabrication\n (no CU layer) on front side.\n\nC++: LSET::FrontBoardTechMask() --> class LSET");
		cl.def_static("BackTechMask", (class LSET (*)()) &LSET::BackTechMask, "Return a mask holding all technical layers (no CU layer) on back side.\n\nC++: LSET::BackTechMask() --> class LSET");
		cl.def_static("BackBoardTechMask", (class LSET (*)()) &LSET::BackBoardTechMask, "Return a mask holding technical layers used in a board fabrication\n (no CU layer) on Back side.\n\nC++: LSET::BackBoardTechMask() --> class LSET");
		cl.def_static("AllTechMask", (class LSET (*)()) &LSET::AllTechMask, "Return a mask holding all technical layers (no CU layer) on both side.\n\nC++: LSET::AllTechMask() --> class LSET");
		cl.def_static("AllBoardTechMask", (class LSET (*)()) &LSET::AllBoardTechMask, "Return a mask holding board technical layers (no CU layer) on both side.\n\nC++: LSET::AllBoardTechMask() --> class LSET");
		cl.def_static("FrontMask", (class LSET (*)()) &LSET::FrontMask, "Return a mask holding all technical layers and the external CU layer on front side.\n\nC++: LSET::FrontMask() --> class LSET");
		cl.def_static("BackMask", (class LSET (*)()) &LSET::BackMask, "Return a mask holding all technical layers and the external CU layer on back side.\n\nC++: LSET::BackMask() --> class LSET");
		cl.def_static("UserMask", (class LSET (*)()) &LSET::UserMask, "C++: LSET::UserMask() --> class LSET");
		cl.def_static("PhysicalLayersMask", (class LSET (*)()) &LSET::PhysicalLayersMask, "Return a mask holding all layers which are physically realized.  Equivalent to the copper\n layers + the board tech mask.\n\nC++: LSET::PhysicalLayersMask() --> class LSET");
		cl.def_static("UserDefinedLayers", (class LSET (*)()) &LSET::UserDefinedLayers, "Return a mask with all of the allowable user defined layers.\n\nC++: LSET::UserDefinedLayers() --> class LSET");
		cl.def_static("ForbiddenFootprintLayers", (class LSET (*)()) &LSET::ForbiddenFootprintLayers, "Layers which are not allowed within footprint definitions.  Currently internal\n copper layers and Margin.\n\nC++: LSET::ForbiddenFootprintLayers() --> class LSET");
		cl.def("CuStack", (class LSEQ (LSET::*)() const) &LSET::CuStack, "Return a sequence of copper layers in starting from the front/top\n and extending to the back/bottom.  This specific sequence is depended upon\n in numerous places.\n\nC++: LSET::CuStack() const --> class LSEQ");
		cl.def("Technicals", [](LSET const &o) -> LSEQ { return o.Technicals(); }, "");
		cl.def("Technicals", (class LSEQ (LSET::*)(class LSET) const) &LSET::Technicals, "Return a sequence of technical layers.  A sequence provides a certain order.\n\n \n is the subset of the technical layers to omit, defaults to none.\n\nC++: LSET::Technicals(class LSET) const --> class LSEQ", pybind11::arg("aSubToOmit"));
		cl.def("Users", (class LSEQ (LSET::*)() const) &LSET::Users, "*_User layers.\n\nC++: LSET::Users() const --> class LSEQ");
		cl.def("TechAndUserUIOrder", (class LSEQ (LSET::*)() const) &LSET::TechAndUserUIOrder, "Returns the technical and user layers in the order shown in layer widget\n\nC++: LSET::TechAndUserUIOrder() const --> class LSEQ");
		cl.def("UIOrder", (class LSEQ (LSET::*)() const) &LSET::UIOrder, "C++: LSET::UIOrder() const --> class LSEQ");
		cl.def("Seq", (class LSEQ (LSET::*)(const enum PCB_LAYER_ID *, unsigned int) const) &LSET::Seq, "Return an LSEQ from the union of this LSET and a desired sequence.  The LSEQ\n element will be in the same sequence as aWishListSequence if they are present.\n \n\n establishes the order of the returned LSEQ, and the LSEQ will only\n contain PCB_LAYER_IDs which are present in this set.\n \n\n is the length of aWishListSequence array.\n\nC++: LSET::Seq(const enum PCB_LAYER_ID *, unsigned int) const --> class LSEQ", pybind11::arg("aWishListSequence"), pybind11::arg("aCount"));
		cl.def("Seq", (class LSEQ (LSET::*)() const) &LSET::Seq, "Return a LSEQ from this LSET in ascending PCB_LAYER_ID order.  Each LSEQ\n element will be in the same sequence as in PCB_LAYER_ID and only present\n in the resultant LSEQ if present in this set.  Therefore the sequence is\n subject to change, use it only when enumeration and not order is important.\n\nC++: LSET::Seq() const --> class LSEQ");
		cl.def("SeqStackupBottom2Top", (class LSEQ (LSET::*)() const) &LSET::SeqStackupBottom2Top, "Return the sequence that is typical for a bottom-to-top stack-up.\n For instance, to plot multiple layers in a single image, the top layers output last.\n\nC++: LSET::SeqStackupBottom2Top() const --> class LSEQ");
		cl.def("ParseHex", (int (LSET::*)(const char *, int)) &LSET::ParseHex, "Convert the output of FmtHex() and replaces this set's values\n with those given in the input string.  Parsing stops at the first\n non hex ASCII byte, except that marker bytes output from FmtHex are\n not terminators.\n \n\n int - number of bytes consumed\n\nC++: LSET::ParseHex(const char *, int) --> int", pybind11::arg("aStart"), pybind11::arg("aCount"));
		cl.def("ExtractLayer", (enum PCB_LAYER_ID (LSET::*)() const) &LSET::ExtractLayer, "Find the first set PCB_LAYER_ID. Returns UNDEFINED_LAYER if more\n than one is set or UNSELECTED_LAYER if none is set.\n\nC++: LSET::ExtractLayer() const --> enum PCB_LAYER_ID");
	}
	// IsValidLayer(int) file:layers_id_colors_and_visibility.h line:770
	M("").def("IsValidLayer", (bool (*)(int)) &IsValidLayer, "Test whether a given integer is a valid layer index, i.e. can\n be safely put in a PCB_LAYER_ID\n\n \n = Layer index to test. It can be an int, so its useful during I/O\n \n\n true if aLayerIndex is a valid layer index\n\nC++: IsValidLayer(int) --> bool", pybind11::arg("aLayerId"));

	// IsPcbLayer(int) file:layers_id_colors_and_visibility.h line:781
	M("").def("IsPcbLayer", (bool (*)(int)) &IsPcbLayer, "Test whether a layer is a valid layer for Pcbnew\n\n \n = Layer to test\n \n\n true if aLayer is a layer valid in Pcbnew\n\nC++: IsPcbLayer(int) --> bool", pybind11::arg("aLayer"));

	// IsCopperLayer(int) file:layers_id_colors_and_visibility.h line:792
	M("").def("IsCopperLayer", (bool (*)(int)) &IsCopperLayer, "Tests whether a layer is a copper layer.\n\n \n = Layer  to test\n \n\n true if aLayer is a valid copper layer\n\nC++: IsCopperLayer(int) --> bool", pybind11::arg("aLayerId"));

	// IsNonCopperLayer(int) file:layers_id_colors_and_visibility.h line:803
	M("").def("IsNonCopperLayer", (bool (*)(int)) &IsNonCopperLayer, "Test whether a layer is a non copper layer.\n\n \n = Layer to test\n \n\n true if aLayer is a non copper layer\n\nC++: IsNonCopperLayer(int) --> bool", pybind11::arg("aLayerId"));

}

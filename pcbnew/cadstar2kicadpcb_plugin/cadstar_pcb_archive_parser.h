/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <@Qbort>
 * Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file cadstar_pcb_archive_parser.cpp
 * @brief Reads in a CADSTAR PCB Archive (*.cpa) file
 */

#ifndef CADSTAR_PCB_ARCHIVE_PARSER_H_
#define CADSTAR_PCB_ARCHIVE_PARSER_H_

#include <cadstar_common.h>
#include <map>
#include <vector>

const long CPA_UNDEFINED = -1; ///< Undefined Parameter

typedef wxString CPA_ID;


//=================================
// HEADER
//=================================


struct CPA_FORMAT
{
    wxString Type;
    long     SomeInt; ///< It is unclear what this parameter is used for
    long     Version; ///< Archive version number (e.g. 21 => CADSTAR 2018.0 and 2019.0 arhive,
                      ///< 20=> CADSTAR 18.0 archive, 19=> CADSTAR 17.0 archive, etc.)
    void Parse( XNODE* aNode );
};


struct CPA_TIMESTAMP
{
    long Year;
    long Month;
    long Day;
    long Hour;
    long Minute;
    long Second;

    void Parse( XNODE* aNode );
};

//Note: there are possibly several other resolutions, but HUNDREDTH MICRON is only one known
enum class CPA_RESOLUTION
{
    HUNDREDTH_MICRON
};


struct CPA_HEADER
{
    CPA_FORMAT     Format;
    wxString       JobFile;
    wxString       JobTitle;
    wxString       Generator;
    CPA_RESOLUTION Resolution;
    CPA_TIMESTAMP  Timestamp;

    void Parse( XNODE* aNode );
};


//=================================
// ASSIGNMENTS
//=================================

//.................................
// ASSIGNMENTS -> LAYERDEFS
//.................................


/**
 * @brief subset of CPA_LAYER_TYPE - for materials only
*/
enum class CPA_MATERIAL_LAYER_TYPE
{
    CONSTRUCTION,
    ELECTRICAL,
    NON_ELECTRICAL
};


struct CPA_MATERIAL
{
    CPA_ID                  ID;
    wxString                Name;
    CPA_MATERIAL_LAYER_TYPE Type; ///< Type of layer appropriate for the material being set up
    CADSTAR_COMMON::EVALUE  Permittivity;
    CADSTAR_COMMON::EVALUE  LossTangent;
    CADSTAR_COMMON::EVALUE  Resistivity; ///< x10^-8 ohm*metre

    void Parse( XNODE* aNode );
};


enum class CPA_LAYER_TYPE
{
    UNDEFINED,   ///< Only used for error detection
    ALLLAYER,    ///< Inbuilt layer type (cannot be assigned to user layers)
    ALLELEC,     ///< Inbuilt layer type (cannot be assigned to user layers)
    ALLDOC,      ///< Inbuilt layer type (cannot be assigned to user layers)
    NOLAYER,     ///< Inbuilt layer type (cannot be assigned to user layers)
    ASSCOMPCOPP, ///< Inbuilt layer type (cannot be assigned to user layers)
    JUMPERLAYER, ///< Inbuilt layer type (cannot be assigned to user layers)
    ELEC,
    POWER,
    NONELEC, ///< This type has subtypes
    CONSTRUCTION,
    DOC
};


enum class CPA_LAYER_SUBTYPE
{
    LAYERSUBTYPE_NONE,
    LAYERSUBTYPE_SILKSCREEN,
    LAYERSUBTYPE_PLACEMENT,
    LAYERSUBTYPE_ASSEMBLY,
    LAYERSUBTYPE_SOLDERRESIST,
    LAYERSUBTYPE_PASTE
};


enum class CPA_ROUTING_BIAS
{
    UNBIASED,   ///< Keyword "UNBIASED" (default)
    X,          ///< Keyword "X_BIASED"
    Y,          ///< Keyword "Y_BIASED"
    ANTI_ROUTE, ///< Keyword "ANTITRACK"
    OBSTACLE    ///< Keyword "OBSTACLE"
};


enum class CPA_EMBEDDING
{
    NONE,
    ABOVE,
    BELOW
};

typedef long CPA_PHYSICAL_LAYER;

struct CPA_LAYER
{
    CPA_ID             ID;
    wxString           Name;
    CPA_LAYER_TYPE     Type          = CPA_LAYER_TYPE::UNDEFINED;
    CPA_LAYER_SUBTYPE  SubType       = CPA_LAYER_SUBTYPE::LAYERSUBTYPE_NONE;
    CPA_PHYSICAL_LAYER PhysicalLayer = CPA_UNDEFINED; ///< If CPA_UNDEFINED, no physical layer is
                                                      ///< assigned (e.g. documentation and
                                                      ///< construction layers)
    CPA_ID           SwapLayerID = wxEmptyString;     ///< If empty, no swap layer
    CPA_ROUTING_BIAS RoutingBias = CPA_ROUTING_BIAS::UNBIASED;
    long             Thickness   = 0; ///< Note: Units of length are defined in file header
    CPA_ID           MaterialId;
    CPA_EMBEDDING    Embedding = CPA_EMBEDDING::NONE;

    void Parse( XNODE* aNode );
};


struct CPA_LAYERDEFS
{
    std::map<CPA_ID, CPA_MATERIAL> Materials;
    std::map<CPA_ID, CPA_LAYER>    Layers;
    std::vector<CPA_ID>            LayerStack;

    void Parse( XNODE* aNode );
};


//.................................
// ASSIGNMENTS -> CODEDEFS
//.................................

enum class CPA_LINESTYLE
{
    SOLID,
    DASH,
    DASHDOT,
    DASHDOTDOT,
    DOT
};

struct CPA_LINECODE
{
    CPA_ID        ID;
    wxString      Name;
    long          Width;
    CPA_LINESTYLE Style;

    void Parse( XNODE* aNode );
};


struct CPA_HATCH
{
    long Step;
    long LineWidth;
    long OrientAngle; ///< 1/1000 of a Degree

    void Parse( XNODE* aNode );
};


struct CPA_HATCHCODE
{
    CPA_ID                 ID;
    wxString               Name;
    std::vector<CPA_HATCH> Hatches;

    void Parse( XNODE* aNode );
};


const long CPA_FONT_NORMAL = 400;
const long CPA_FONT_BOLD   = 700;


struct CPA_FONT
{
    wxString Name  = wxT( "CADSTAR" );
    long Modifier1 = CPA_FONT_NORMAL; ///< It seems this is related to weight. 400=Normal, 700=Bold.
    long Modifier2 = 0;               ///< It seems this is always 0 regardless of settings
    bool KerningPairs = false; ///< From CADSTAR Help: "Kerning Pairs is for causing the system to
                               ///< automatically reduce the spacing between certain pairs of
                               ///< characters in order to improve the appearance of the text"
    bool Italic = false;

    void Parse( XNODE* aNode );
};


struct CPA_TEXTCODE
{
    CPA_ID   ID;
    wxString Name;
    long     LineWidth;
    long     Height;
    long     Width; ///< Defaults to 0 if using system fonts or, if using CADSTAR font, default to
                    ///< equal height (1:1 aspect ratio). Allows for system fonts to be rendered in
                    ///< a different aspect ratio.
    CPA_FONT Font;

    void Parse( XNODE* aNode );
};


struct CPA_ROUTECODE
{
    CPA_ID   ID;
    wxString Name;
    long     OptimalWidth;
    long     MinWidth;
    long     MaxWidth;
    long     NeckedWidth;

    void Parse( XNODE* aNode );
};


struct CPA_COPREASSIGN
{
    CPA_ID LayerID;
    long   CopperWidth;

    void Parse( XNODE* aNode );
};


struct CPA_COPPERCODE
{
    CPA_ID                       ID;
    wxString                     Name;
    long                         CopperWidth;
    std::vector<CPA_COPREASSIGN> Reassigns;

    void Parse( XNODE* aNode );
};


struct CPA_SPACINGCODE
{
    wxString Code;
    long     Spacing;

    void Parse( XNODE* aNode );
};


enum class CPA_SHAPE_TYPE
{
    ANNULUS,
    BULLET,
    CIRCLE, ///< Keyword "ROUND"
    DIAMOND,
    FINGER,
    OCTAGON,
    RECTANGLE,
    ROUNDED_RECT, ///< Keyword "ROUNDED"
    SQUARE
};


struct CPA_PAD_SHAPE
{
    CPA_SHAPE_TYPE ShapeType;
    long           Size            = CPA_UNDEFINED;
    long           LeftLength      = CPA_UNDEFINED;
    long           RightLength     = CPA_UNDEFINED;
    long           InternalFeature = CPA_UNDEFINED;
    long           OrientAngle     = 0; ///< 1/1000 of a Degree

    static bool IsShape( XNODE* aNode );
    void        Parse( XNODE* aNode );
};


struct CPA_PADREASSIGN
{
    CPA_ID        LayerID;
    CPA_PAD_SHAPE Shape;

    void Parse( XNODE* aNode );
};


struct CPA_PADCODE
{
    CPA_ID        ID;
    wxString      Name;
    CPA_PAD_SHAPE Shape;
    long          ReliefClearance = CPA_UNDEFINED; ///< if undefined inherits from design
    long          ReliefWidth     = CPA_UNDEFINED; ///< if undefined inherits from design
    bool          Plated          = true;
    long          DrillDiameter   = CPA_UNDEFINED;
    long          DrillOversize   = CPA_UNDEFINED;
    long          SlotLength      = CPA_UNDEFINED;
    long          SlotOrientation = CPA_UNDEFINED;
    long          DrillXoffset    = CPA_UNDEFINED;
    long          DrillYoffset    = CPA_UNDEFINED;

    std::vector<CPA_PADREASSIGN> Reassigns;

    void Parse( XNODE* aNode );
};


struct CPA_VIAREASSIGN
{
    CPA_ID        LayerID;
    CPA_PAD_SHAPE Shape;

    void Parse( XNODE* aNode );
};


struct CPA_VIACODE
{
    CPA_ID        ID;
    wxString      Name;
    CPA_PAD_SHAPE Shape;
    long          ReliefClearance = CPA_UNDEFINED; ///< if undefined inherits from design
    long          ReliefWidth     = CPA_UNDEFINED; ///< if undefined inherits from design
    long          DrillDiameter   = CPA_UNDEFINED;
    long          DrillOversize   = CPA_UNDEFINED;

    std::vector<CPA_VIAREASSIGN> Reassigns;

    void Parse( XNODE* aNode );
};


struct CPA_LAYERPAIR
{
    CPA_ID             ID;
    wxString           Name;
    CPA_PHYSICAL_LAYER PhysicalLayerStart;
    CPA_PHYSICAL_LAYER PhysicalLayerEnd;
    CPA_ID             ViacodeID;

    void Parse( XNODE* aNode );
};


enum class CPA_ATTROWNER
{
    ALL_ITEMS,
    AREA,
    BOARD,
    COMPONENT,
    CONNECTION,
    COPPER,
    DOCSYMBOL,
    FIGURE,
    NET,
    NETCLASS,
    PART,            ///< Only library Attributes
    PART_DEFINITION, ///< Only library Attributes
    PIN,
    SYMDEF,
    TEMPLATE,
    TESTPOINT
};


enum class CPA_ATTRUSAGE
{
    BOTH,            ///< From CADSTAR Help: Assigned to both Schematic symbols and PCB components,
                     ///< and displayed on Schematic and PCB designs.
    COMPONENT,       ///< From CADSTAR Help: Assigned to PCB components and displayed on PCB designs
    PART_DEFINITION, ///< From CADSTAR Help: Assigned to Parts library Definitions and displayed
                     ///< by the Library searcher
    PART_LIBRARY,    ///< From CADSTAR Help: Only used by non-Cadstar applicationws
    SYMBOL,          ///< From CADSTAR Help: Assigned to Schematic Symbols and displayed on
                     ///< Schematic Designs
    UNDEFINED        ///< Note: It seems that some attribute have no "ATTRUSAGE" defined. It appears
                     ///< that the attributes that fall under this category arethe ones associated
                     ///< with the design itself (i.e. not inherited from the library)
};

/**
 * @brief NOTE from CADSTAR help: To convert a Part Definition Attribute into a hyperlink, prefix
 * the attribute name with "Link "
 */
struct CPA_ATTRNAME
{
    CPA_ID        ID;
    wxString      Name;
    CPA_ATTROWNER AttributeOwner = CPA_ATTROWNER::ALL_ITEMS;
    CPA_ATTRUSAGE AttributeUsage = CPA_ATTRUSAGE::UNDEFINED;
    bool          NoTransfer     = false; ///< True="All Design Types", False="Current Design Type"
                                          ///< "All Design Types" Description from CADSTAR Help:
                                          ///< "The selected attribute name will beavailable when
                                          ///< any design is displayed"
                                          ///< "Current Design Type" From CADSTAR Help: This
                                          ///< restricts the availability of the selected attribute
                                          ///< name to the current design.

    void Parse( XNODE* aNode );
};


struct CPA_ATTRIBUTE_VALUE
{
    CPA_ID   AttributeID;
    wxString Value;

    void Parse( XNODE* aNode );
};


struct CPA_NETCLASS
{
    CPA_ID                           ID;
    wxString                         Name;
    std::vector<CPA_ATTRIBUTE_VALUE> Attributes;

    void Parse( XNODE* aNode );
};


struct CPA_SPCCLASSNAME
{
    CPA_ID   ID; //TODO convert to an enum class containing all valid spacings
    wxString Name;

    void Parse( XNODE* aNode );
};

const CPA_ID CPA_NO_SPACE_CLASS_ID = wxT( "NO_SPACE_CLASS" ); ///< Default spacing class

struct CPA_SPCCLASSSPACE
{
    CPA_ID SpacingClassID1;
    CPA_ID SpacingClassID2;
    CPA_ID LayerID; ///< Normally LAY0, which corresponds to (All Layers)
    long   Spacing;

    void Parse( XNODE* aNode );
};


struct CPA_CODEDEFS
{
    std::map<CPA_ID, CPA_LINECODE>     LineCodes;
    std::map<CPA_ID, CPA_HATCHCODE>    HatchCodes;
    std::map<CPA_ID, CPA_TEXTCODE>     TextCodes;
    std::map<CPA_ID, CPA_ROUTECODE>    RouteCodes;
    std::map<CPA_ID, CPA_COPPERCODE>   CopperCodes;
    std::vector<CPA_SPACINGCODE>       SpacingCodes; ///< Spacing Design Rules
    std::map<CPA_ID, CPA_PADCODE>      PadCodes;
    std::map<CPA_ID, CPA_VIACODE>      ViaCodes;
    std::map<CPA_ID, CPA_LAYERPAIR>    LayerPairs; ///< Default vias to use between pairs of layers
    std::map<CPA_ID, CPA_ATTRNAME>     AttributeNames;
    std::map<CPA_ID, CPA_NETCLASS>     NetClasses;
    std::map<CPA_ID, CPA_SPCCLASSNAME> SpacingClassNames;
    std::vector<CPA_SPCCLASSSPACE>     SpacingClasses;

    void Parse( XNODE* aNode );
};

//.................................
// ASSIGNMENTS -> TECHNOLOGY
//.................................

enum class CPA_UNITS
{
    THOU,
    INCH,
    MICROMETRE,
    MM,
    CENTIMETER,
    METER
};

struct CPA_TECHNOLOGY
{
    CPA_UNITS Unit;               ///< Unit to display for linear dimensions
    long      UnitDisplPrecision; ///< Number of decimal points to display for linear dimensions
    long      InterlineGap;       ///< For CADSTAR font only, distance between lines of text,
                                  ///< expressed as a percentage of the text height (accepted
                                  ///< values are 0-100)
    long BarlineGap;              ///< For CADSTAR font only, distance between top bar and
                                  ///< character, expressed as a percentage of the text height
                                  ///< (accepted values are 0-100)
    bool AllowBarredText = false; ///< Specifies if barring is allowed in the design
    long AngularPrecision;        ///< Number of decimal points to display for angular dimensions
    long MinRouteWidth;           ///< Manufacturing Design Rule. Corresponds to "Thin Route Width"
    long MinNeckedLength;         ///< Manufacturing Design Rule. Corresponds to
                                  ///< "Minimum Thinner Track Length"
    long MinUnneckedLength;       ///< Manufacturing Design Rule. Corresponds to
                                  ///< "Minimum Thicker Track Length"
    long MinMitre;                ///< Manufacturing Design Rule. Corresponds to "Minimum Mitre"
    long MaxMitre;                ///< Manufacturing Design Rule. Corresponds to "Maximum Mitre"
    long MaxPhysicalLayer;        ///< Should equal number of copper layers. However, it seems this
                                  ///< can be set to any arbitrarily high number as long as it is
                                  ///< greater or equal to the number of copper layers. Also the
                                  ///< last copper layer (bottom) must have this set as its value.
    long TrackGrid;               ///< Grid for Routes (equal X and Y steps)
    long ViaGrid;                 ///< Grid for Vias (equal X and Y steps)

    CADSTAR_COMMON::POINT                                   DesignOrigin;
    std::pair<CADSTAR_COMMON::POINT, CADSTAR_COMMON::POINT> DesignArea;
    CADSTAR_COMMON::POINT                                   DesignRef; ///< Appears to be 0,0 always
    CADSTAR_COMMON::POINT                                   DesignLimit;

    bool BackOffJunctions   = false;
    bool BackOffWidthChange = false;

    void Parse( XNODE* aNode );
};

//.................................
// ASSIGNMENTS -> GRIDS
//.................................

enum class CPA_GRID_TYPE
{
    FRACTIONALGRID, ///< Param1 = Unit, Param2 = Divisor. The grid is equal in X and Y dimensions
                    ///< with a step size equal to Param1/Param2
    STEPGRID        ///< Param1 = X Step, Param2 = Y Step. A standard x,y grid.
};


struct CPA_GRID
{
    CPA_GRID_TYPE Type;
    wxString      Name;
    long          Param1; ///< Either Unit or X step, depending on Type (see CPA_GRID_TYPE for
                          ///< more details)
    long Param2;          ///< Either Divisor or Y step, depending on Type (see CPA_GRID_TYPE for
                          ///< more details)

    static bool IsGrid( XNODE* aNode );
    void        Parse( XNODE* aNode );
};


struct CPA_GRIDS
{
    CPA_GRID WorkingGrid;
    CPA_GRID ScreenGrid;             ///< From CADSTAR Help: "There is one Screen Grid, which is
                                     ///< visible as dots on the screen. You cannot specify your
                                     ///< own name for the Screen Grid. The visibility and colour
                                     ///< of the dots representing the Screen Grid is set up by
                                     ///< the Highlight category on the Colours dialog.
                                     ///<
                                     ///< The type of Screen grid displayed(Lined or Points) is
                                     ///< set up on the Display dialog within Options(File menu)."
    std::vector<CPA_GRID> UserGrids; ///< List of predefined grids created by the user

    void Parse( XNODE* aNode );
};


struct CPA_ASSIGNMENTS
{
    CPA_LAYERDEFS  Layerdefs;
    CPA_CODEDEFS   Codedefs;
    CPA_TECHNOLOGY Technology;
    CPA_GRIDS      Grids;
    bool           NetclassEditAttributeSettings     = false; //< Unclear what this does
    bool           SpacingclassEditAttributeSettings = false; //< Unclear what this does

    void Parse( XNODE* aNode );
};


//=================================
// CPA_FILE
//=================================

/**
 * @brief Represents a CADSTAR PCB Archive (CPA) file
 */
class CPA_FILE
{
public:
    explicit CPA_FILE( wxString aFilename ) : Filename( aFilename )
    {
    }

    /**
     * @brief Parses the file
     * @throw IO_ERROR if file could not be opened or there was
     * an error while parsing
     */
    void Parse();

    wxString Filename;

    CPA_HEADER      Header;
    CPA_ASSIGNMENTS Assignments;
    //TODO Add Library, Defaults, Parts, etc..

    int KiCadUnitMultiplier; ///<Use this value to convert units in this CPA file to KiCad units
};


#endif // CADSTAR_PCB_ARCHIVE_PARSER_H_

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

#include <boost/serialization/strong_typedef.hpp>
#include <cadstar_archive_common.h>
#include <map>
#include <vector>


//=================================
// MACRO DEFINITIONS
//=================================
#define UNDEFINED_LAYER_ID ( LAYER_ID ) wxEmptyString
#define UNDEFINED_MATERIAL_ID ( MATERIAL_ID ) wxEmptyString
#define UNDEFINED_PHYSICAL_LAYER ( PHYSICAL_LAYER_ID ) - 1

/**
 * Default spacing class for all nets
 */
#define NO_SPACE_CLASS_ID ( SPACING_CLASS_ID ) wxT( "NO_SPACE_CLASS" )

/**
 * Component Name Attribute ID - typically used for placement of designators on silk screen.
 */
#define COMPONENT_NAME_ATTRID ( ATTRIBUTE_ID ) wxT( "__COMPONENT_NAME__" )

/**
 * Component Name 2 Attribute ID - typically used for indicating the placement of designators in
 * placement drawings.
 */
#define COMPONENT_NAME_2_ATTRID ( ATTRIBUTE_ID ) wxT( "__COMPONENT_NAME_2__" )
#define PART_NAME_ATTRID ( ATTRIBUTE_ID ) wxT( "__PART_NAME__" )


/**
 * @brief Represents a CADSTAR PCB Archive (CPA) file
 */
class CADSTAR_PCB_ARCHIVE_PARSER : public CADSTAR_ARCHIVE_COMMON
{
public:
    explicit CADSTAR_PCB_ARCHIVE_PARSER( wxString aFilename )
            : Filename( aFilename ), CADSTAR_ARCHIVE_COMMON()
    {
        KiCadUnitMultiplier = 10; // assume hundredth micron
    }

    /**
     * @brief Parses the file
     * @throw IO_ERROR if file could not be opened or there was
     * an error while parsing
     */
    void Parse();

    typedef wxString MATERIAL_ID;
    typedef wxString LAYER_ID;
    typedef long     PHYSICAL_LAYER_ID;
    typedef wxString LINECODE_ID;
    typedef wxString HATCHCODE_ID;
    typedef wxString TEXTCODE_ID;
    typedef wxString ROUTECODE_ID;
    typedef wxString COPPERCODE_ID;
    typedef wxString PADCODE_ID;
    typedef wxString VIACODE_ID;
    typedef wxString LAYERPAIR_ID;
    typedef wxString ATTRIBUTE_ID;
    typedef wxString NETCLASS_ID;
    typedef wxString SPACING_CLASS_ID;
    typedef wxString RULESET_ID;
    typedef wxString FIGURE_ID;
    typedef wxString COMP_AREA_ID;
    typedef long     PAD_ID; ///< Pad identifier (pin) in the PCB
    typedef wxString TEXT_ID;
    typedef wxString DIMENSION_ID;
    typedef wxString SYMDEF_ID;
    typedef wxString PART_ID;
    typedef wxString GATE_ID;
    typedef long     TERMINAL_ID;            ///< Terminal is the pin identifier in the schematic
    typedef long     PART_DEFINITION_PIN_ID; ///< Pin identifier in the part definition
    typedef long     PART_PIN_ID;            ///< Pin identifier in the part
    typedef wxString GROUP_ID;
    typedef wxString REUSEBLOCK_ID;
    typedef wxString BOARD_ID;
    typedef wxString AREA_ID;
    typedef wxString NET_ID;
    typedef wxString COMPONENT_ID;
    typedef wxString DOCUMENTATION_SYMBOL_ID;
    typedef wxString NETELEMENT_ID;
    typedef wxString TEMPLATE_ID;
    typedef long     NETREF_TERMINAL_ID;
    typedef wxString COPPER_ID;
    typedef wxString DRILL_TABLE_ID;

    //=================================
    // HEADER
    //=================================


    struct FORMAT
    {
        wxString Type;
        long     SomeInt; ///< It is unclear what this parameter is used for
        long     Version; ///< Archive version number (e.g. 21 => CADSTAR 2018.0 and 2019.0 arhive,
                          ///< 20=> CADSTAR 18.0 archive, 19=> CADSTAR 17.0 archive, etc.)
        void Parse( XNODE* aNode );
    };


    struct TIMESTAMP
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
    enum class RESOLUTION
    {
        HUNDREDTH_MICRON
    };


    struct HEADER
    {
        FORMAT     Format;
        wxString   JobFile;
        wxString   JobTitle;
        wxString   Generator;
        RESOLUTION Resolution;
        TIMESTAMP  Timestamp;

        void Parse( XNODE* aNode );
    };


    //=================================
    // ASSIGNMENTS
    //=================================

    //.................................
    // ASSIGNMENTS -> LAYERDEFS
    //.................................


    /**
     * @brief Type of layer appropriate for the material being set up
     */
    enum class MATERIAL_LAYER_TYPE
    {
        CONSTRUCTION,
        ELECTRICAL, ///< Either @see LAYER_TYPE::ELEC or @see LAYER_TYPE::POWER
        NON_ELECTRICAL
    };


    struct MATERIAL
    {
        MATERIAL_ID         ID;
        wxString            Name;
        MATERIAL_LAYER_TYPE Type;
        EVALUE              Permittivity;
        EVALUE              LossTangent;
        EVALUE              Resistivity; ///< x10^-8 ohm*metre

        void Parse( XNODE* aNode );
    };


    enum class LAYER_TYPE
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


    enum class LAYER_SUBTYPE
    {
        LAYERSUBTYPE_NONE,
        LAYERSUBTYPE_SILKSCREEN,
        LAYERSUBTYPE_PLACEMENT,
        LAYERSUBTYPE_ASSEMBLY,
        LAYERSUBTYPE_SOLDERRESIST,
        LAYERSUBTYPE_PASTE
    };


    enum class ROUTING_BIAS
    {
        UNBIASED,   ///< Keyword "UNBIASED" (default)
        X,          ///< Keyword "X_BIASED"
        Y,          ///< Keyword "Y_BIASED"
        ANTI_ROUTE, ///< Keyword "ANTITRACK"
        OBSTACLE    ///< Keyword "OBSTACLE"
    };


    enum class EMBEDDING
    {
        NONE,
        ABOVE,
        BELOW
    };

    struct LAYER
    {
        LAYER_ID          ID;
        wxString          Name;
        wxString          Description = wxEmptyString;
        LAYER_TYPE        Type        = LAYER_TYPE::UNDEFINED;
        LAYER_SUBTYPE     SubType     = LAYER_SUBTYPE::LAYERSUBTYPE_NONE;
        PHYSICAL_LAYER_ID PhysicalLayer =
                UNDEFINED_PHYSICAL_LAYER;              ///< If UNDEFINED, no physical layer is
                                                       ///< assigned (e.g. documentation and
                                                       ///< construction layers)
        LAYER_ID     SwapLayerID = UNDEFINED_LAYER_ID; ///< If UNDEFINED_LAYER_ID, no swap layer
        ROUTING_BIAS RoutingBias = ROUTING_BIAS::UNBIASED;
        long         Thickness   = 0; ///< Note: Units of length are defined in file header
        MATERIAL_ID  MaterialId;
        EMBEDDING    Embedding = EMBEDDING::NONE;

        void Parse( XNODE* aNode );
    };


    struct LAYERDEFS
    {
        std::map<MATERIAL_ID, MATERIAL> Materials;
        std::map<LAYER_ID, LAYER>       Layers;
        std::vector<LAYER_ID>           LayerStack;

        void Parse( XNODE* aNode );
    };


    //.................................
    // ASSIGNMENTS -> CODEDEFS
    //.................................

    enum class LINESTYLE
    {
        SOLID,
        DASH,
        DASHDOT,
        DASHDOTDOT,
        DOT
    };

    struct LINECODE
    {
        LINECODE_ID ID;
        wxString    Name;
        long        Width;
        LINESTYLE   Style;

        void Parse( XNODE* aNode );
    };


    struct HATCH
    {
        long Step;
        long LineWidth;
        long OrientAngle; ///< 1/1000 of a Degree

        void Parse( XNODE* aNode );
    };


    struct HATCHCODE
    {
        HATCHCODE_ID       ID;
        wxString           Name;
        std::vector<HATCH> Hatches;

        void Parse( XNODE* aNode );
    };


    static const long FONT_NORMAL = 400;
    static const long FONT_BOLD   = 700;


    struct FONT
    {
        wxString Name  = wxT( "CADSTAR" );
        long Modifier1 = FONT_NORMAL; ///< It seems this is related to weight. 400=Normal, 700=Bold.
        long Modifier2 = 0;           ///< It seems this is always 0 regardless of settings
        bool KerningPairs =
                false; ///< From CADSTAR Help: "Kerning Pairs is for causing the system to
                       ///< automatically reduce the spacing between certain pairs of
                       ///< characters in order to improve the appearance of the text"
        bool Italic = false;

        void Parse( XNODE* aNode );
    };


    struct TEXTCODE
    {
        TEXTCODE_ID ID;
        wxString    Name;
        long        LineWidth;
        long        Height;
        long Width; ///< Defaults to 0 if using system fonts or, if using CADSTAR font, default to
                    ///< equal height (1:1 aspect ratio). Allows for system fonts to be rendered in
                    ///< a different aspect ratio.
        FONT Font;

        void Parse( XNODE* aNode );
    };


    struct ROUTECODE
    {
        //TODO: Generalise this so it can also be used with CSA files
        // (CSA files use "SROUTEWIDTH" subnode, instead of attribute)

        ROUTECODE_ID ID;
        wxString     Name;
        long         OptimalWidth;
        long         MinWidth;
        long         MaxWidth;
        long         NeckedWidth;

        void Parse( XNODE* aNode );
    };


    struct COPREASSIGN
    {
        LAYER_ID LayerID;
        long     CopperWidth;

        void Parse( XNODE* aNode );
    };


    struct COPPERCODE
    {
        COPPERCODE_ID            ID;
        wxString                 Name;
        long                     CopperWidth;
        std::vector<COPREASSIGN> Reassigns;

        void Parse( XNODE* aNode );
    };

    struct SPACINGCODE
    {
        struct REASSIGN
        {
            LAYER_ID LayerID;
            long     Spacing;

            void Parse( XNODE* aNode );
        };

        wxString              Code; //TODO convert to an enum class containing all valid spacings
        long                  Spacing;
        std::vector<REASSIGN> Reassigns; ///< Can have different spacings on differnt layers

        void Parse( XNODE* aNode );
    };


    enum class PAD_SHAPE_TYPE
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


    struct PAD_SHAPE
    {
        PAD_SHAPE_TYPE ShapeType;
        long           Size            = UNDEFINED_VALUE;
        long           LeftLength      = UNDEFINED_VALUE;
        long           RightLength     = UNDEFINED_VALUE;
        long           InternalFeature = UNDEFINED_VALUE;
        long           OrientAngle     = 0; ///< 1/1000 of a Degree

        static bool IsPadShape( XNODE* aNode );
        void        Parse( XNODE* aNode );
    };


    struct PADREASSIGN
    {
        LAYER_ID  LayerID;
        PAD_SHAPE Shape;

        void Parse( XNODE* aNode );
    };


    struct PADCODE
    {
        PADCODE_ID ID;
        wxString   Name;
        PAD_SHAPE  Shape;
        long       ReliefClearance = UNDEFINED_VALUE; ///< if undefined inherits from design
        long       ReliefWidth     = UNDEFINED_VALUE; ///< if undefined inherits from design
        bool       Plated          = true;
        long       DrillDiameter   = UNDEFINED_VALUE;
        long       DrillOversize   = UNDEFINED_VALUE;
        long       SlotLength      = UNDEFINED_VALUE;
        long       SlotOrientation = UNDEFINED_VALUE;
        long       DrillXoffset    = UNDEFINED_VALUE;
        long       DrillYoffset    = UNDEFINED_VALUE;

        std::map<LAYER_ID, PAD_SHAPE> Reassigns;

        void Parse( XNODE* aNode );
    };


    struct VIAREASSIGN
    {
        LAYER_ID  LayerID;
        PAD_SHAPE Shape;

        void Parse( XNODE* aNode );
    };


    struct VIACODE
    {
        VIACODE_ID ID;
        wxString   Name;
        PAD_SHAPE  Shape;
        long       ReliefClearance = UNDEFINED_VALUE; ///< if undefined inherits from design
        long       ReliefWidth     = UNDEFINED_VALUE; ///< if undefined inherits from design
        long       DrillDiameter   = UNDEFINED_VALUE;
        long       DrillOversize   = UNDEFINED_VALUE;

        std::map<LAYER_ID, PAD_SHAPE> Reassigns;

        void Parse( XNODE* aNode );
    };


    struct LAYERPAIR
    {
        LAYERPAIR_ID      ID;
        wxString          Name;
        PHYSICAL_LAYER_ID PhysicalLayerStart;
        PHYSICAL_LAYER_ID PhysicalLayerEnd;
        VIACODE_ID        ViacodeID;

        void Parse( XNODE* aNode );
    };


    enum class ATTROWNER
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


    enum class ATTRUSAGE
    {
        BOTH,      ///< From CADSTAR Help: Assigned to both Schematic symbols and PCB components,
                   ///< and displayed on Schematic and PCB designs.
        COMPONENT, ///< From CADSTAR Help: Assigned to PCB components and displayed on PCB designs
        PART_DEFINITION, ///< From CADSTAR Help: Assigned to Parts library Definitions and displayed
                         ///< by the Library searcher
        PART_LIBRARY,    ///< From CADSTAR Help: Only used by non-Cadstar applications
        SYMBOL,          ///< From CADSTAR Help: Assigned to Schematic Symbols and displayed on
                         ///< Schematic Designs
        UNDEFINED ///< Note: It seems that some attribute have no "ATTRUSAGE" defined. It appears
                  ///< that the attributes that fall under this category arethe ones associated
                  ///< with the design itself (i.e. not inherited from the library)
    };

    /**
     * @brief NOTE from CADSTAR help: To convert a Part Definition Attribute into a hyperlink, prefix
     * the attribute name with "Link "
     */
    struct ATTRNAME
    {
        struct COLUMNORDER
        {
            long ID;
            long Order;

            void Parse( XNODE* aNode );
        };


        struct COLUMNWIDTH
        {
            long ID;
            long Width;

            void Parse( XNODE* aNode );
        };

        ATTRIBUTE_ID ID;
        wxString     Name; ///< Parenthesis aren't permitted in user attributes in CADSTAR. Any
                           ///< Attributes in Parenthesis indicate an internal CADSTAR attribute
                           ///< Examples: "(PartDescription)" "(PartDefinitionNameStem)",etc.
                           ///TODO: create a list of all CADSTAR internal attribute names.
        ATTROWNER AttributeOwner = ATTROWNER::ALL_ITEMS;
        ATTRUSAGE AttributeUsage = ATTRUSAGE::UNDEFINED;
        bool      NoTransfer     = false; ///< True="All Design Types", False="Current Design Type"
                                          ///< "All Design Types" Description from CADSTAR Help:
                                          ///< "The selected attribute name will beavailable when
                                          ///< any design is displayed"
                                          ///< "Current Design Type" From CADSTAR Help: This
                                          ///< restricts the availability of the selected attribute
                                          ///< name to the current design.
        std::vector<COLUMNORDER> ColumnOrders;
        std::vector<COLUMNWIDTH> ColumnWidths;
        bool                     ColumnInvisible = false;

        void Parse( XNODE* aNode );
    };


    /**
     * @brief From CADSTAR Help: "Text Alignment enables you to define the position of an alignment
     * origin for all text items in CADSTAR. The alignment origin is a point on or within the text
     * boundary and defines how the text is displayed.
     *
     * For example, with an alignment of bottom-right the origin will be positioned at the bottom
     * right of the text boundary. This makes it easier to right-align several text items 
     * regardless of the length of text displayed.
     *
     * Text Alignment applies to all CADSTAR text. [...]
     *
     * Note: Unaligned text operates in the way CADSTAR text always has. In most cases this behaves
     * as Bottom Left alignment, but there are a few exceptions, e.g. pin names. Also unaligned
     * multiline text has an origin Bottom Left of the first line."
     * 
     * See also JUSTIFICATION
     */
    enum class ALIGNMENT
    {
        NO_ALIGNMENT, ///< NO_ALIGNMENT has different meaning depending on the object type
        TOPLEFT,
        TOPCENTER,
        TOPRIGHT,
        CENTERLEFT,
        CENTERCENTER,
        CENTERRIGHT,
        BOTTOMLEFT,
        BOTTOMCENTER,
        BOTTOMRIGHT
    };

    static ALIGNMENT ParseAlignment( XNODE* aNode );

    /**
     * @brief From CADSTAR Help: "Multi Line Text can also be justified as Left, Centre or Right. 
     * This does not affect the text alignment. Note: Justification of single line text has no 
     * effect."
     *
     * This only affects multiline text
     *
     * See also ALIGNMENT
     */
    enum class JUSTIFICATION
    {
        LEFT,
        CENTER,
        RIGHT
    };

    static JUSTIFICATION ParseJustification( XNODE* aNode );

    /**
     * @brief Sets the readibility direction of text. From CADSTAR Help: "Horizontal text will
     * always be displayed from left to right (i.e. never upside down). Vertical text can be set as
     * readable from either the left or right edge of the design."
     *
     * I.e. Vertical text can either be rotated 90 degrees clockwise or 90 degrees anticlockwise from
     * horizontal. This does not impact vertical text
     */
    enum class READABILITY
    {
        BOTTOM_TO_TOP, ///< When text is vertical, show it rotated 90 degrees anticlockwise
        TOP_TO_BOTTOM  ///< When text is vertical, show it rotated 90 degrees clockwise
    };

    static READABILITY ParseReadability( XNODE* aNode );


    struct ATTRIBUTE_LOCATION
    {
        TEXTCODE_ID   TextCodeID;
        LAYER_ID      LayerID;
        POINT         Position;
        long          OrientAngle = 0;
        bool          Mirror      = false;
        bool          Fixed       = false;
        JUSTIFICATION Justification =
                JUSTIFICATION::LEFT; ///< Note: Justification has no effect on single lines of text
        ALIGNMENT Alignment = ALIGNMENT::
                NO_ALIGNMENT; ///< In CADSTAR The default alignment for a TEXT object (when
                              ///< "(No Alignment()" is selected) Bottom Left of the *first line*.
                              ///< Note that this is different from BOTTOM_LEFT (which is bottom
                              ///< left of the whole text block)

        void Parse( XNODE* aNode );
    };

    struct ATTRIBUTE_VALUE
    {
        ATTRIBUTE_ID AttributeID;
        wxString     Value;
        bool         ReadOnly    = false;
        bool         HasLocation = false; ///< Flag to know if this ATTRIBUTE_VALUE has a location
                                          ///< i.e. is displayed
        ATTRIBUTE_LOCATION AttributeLocation;

        void Parse( XNODE* aNode );
    };


    struct NETCLASS
    {
        NETCLASS_ID                  ID;
        wxString                     Name;
        std::vector<ATTRIBUTE_VALUE> Attributes;

        void Parse( XNODE* aNode );
    };


    struct SPCCLASSNAME
    {
        SPACING_CLASS_ID ID;
        wxString         Name;

        void Parse( XNODE* aNode );
    };


    struct SPCCLASSSPACE
    {
        SPACING_CLASS_ID SpacingClassID1;
        SPACING_CLASS_ID SpacingClassID2;
        LAYER_ID         LayerID; ///< Normally LAY0, which corresponds to (All Layers)
        long             Spacing;

        void Parse( XNODE* aNode );
    };


    struct RULESET
    {
        RULESET_ID   ID;
        wxString     Name;
        ROUTECODE_ID AreaRouteCodeID; ///< For assigning a net route code to a rule set. The
                                      ///< optimal and necked route widths from the net route code
                                      ///< will be used when routing through an area that has been
                                      ///< assigned this rule set. ("ROUCODEREF")
        VIACODE_ID AreaViaCodeID;     ///< For assigning a via code to a rule set. This via code
                                      ///< will be used when inserting new vias within an area that
                                      ///< has been assigned this rule set. ("VIACODEREF")

        std::vector<SPACINGCODE> SpacingCodes; ///< Overrides these spacing rules in the specific
                                               ///< area.
        void Parse( XNODE* aNode );
    };


    struct CODEDEFS
    {
        std::map<LINECODE_ID, LINECODE>     LineCodes;
        std::map<HATCHCODE_ID, HATCHCODE>   HatchCodes;
        std::map<TEXTCODE_ID, TEXTCODE>     TextCodes;
        std::map<ROUTECODE_ID, ROUTECODE>   RouteCodes;
        std::map<COPPERCODE_ID, COPPERCODE> CopperCodes;
        std::vector<SPACINGCODE>            SpacingCodes; ///< Spacing Design Rules
        std::map<RULESET_ID, RULESET>       Rulesets;     ///< Used for area design rules
        std::map<PADCODE_ID, PADCODE>       PadCodes;
        std::map<VIACODE_ID, VIACODE>       ViaCodes;
        std::map<LAYERPAIR_ID, LAYERPAIR>
                LayerPairs; ///< Default vias to use between pairs of layers

        std::map<ATTRIBUTE_ID, ATTRNAME>         AttributeNames;
        std::map<NETCLASS_ID, NETCLASS>          NetClasses;
        std::map<SPACING_CLASS_ID, SPCCLASSNAME> SpacingClassNames;
        std::vector<SPCCLASSSPACE>               SpacingClasses;

        void Parse( XNODE* aNode );
    };

    //.................................
    // ASSIGNMENTS -> TECHNOLOGY
    //.................................

    enum class UNITS
    {
        THOU,
        INCH,
        MICROMETRE,
        MM,
        CENTIMETER,
        METER
    };

    static UNITS ParseUnits( XNODE* aNode );

    struct TECHNOLOGY_SECTION
    {
        UNITS Units;                  ///< Units to display for linear dimensions
        long  UnitDisplPrecision;     ///< Number of decimal points to display for linear dimensions
        long  InterlineGap;           ///< For CADSTAR font only, distance between lines of text,
                                      ///< expressed as a percentage of the text height (accepted
                                      ///< values are 0-100)
        long BarlineGap;              ///< For CADSTAR font only, distance between top bar and
                                      ///< character, expressed as a percentage of the text height
                                      ///< (accepted values are 0-100)
        bool AllowBarredText = false; ///< Specifies if barring is allowed in the design
        long AngularPrecision;  ///< Number of decimal points to display for angular dimensions
        long MinRouteWidth;     ///< Manufacturing Design Rule. Corresponds to "Thin Route Width"
        long MinNeckedLength;   ///< Manufacturing Design Rule. Corresponds to
                                ///< "Minimum Thinner Track Length"
        long MinUnneckedLength; ///< Manufacturing Design Rule. Corresponds to
                                ///< "Minimum Thicker Track Length"
        long MinMitre;          ///< Manufacturing Design Rule. Corresponds to "Minimum Mitre"
        long MaxMitre;          ///< Manufacturing Design Rule. Corresponds to "Maximum Mitre"
        long MaxPhysicalLayer;  ///< Should equal number of copper layers. However, it seems this
                                ///< can be set to any arbitrarily high number as long as it is
                                ///< greater or equal to the number of copper layers. Also the
                                ///< last copper layer (bottom) must have this set as its value.
        long TrackGrid;         ///< Grid for Routes (equal X and Y steps)
        long ViaGrid;           ///< Grid for Vias (equal X and Y steps)

        POINT                   DesignOrigin;
        std::pair<POINT, POINT> DesignArea;
        POINT                   DesignRef; ///< Appears to be 0,0 always
        POINT                   DesignLimit;

        bool BackOffJunctions   = false;
        bool BackOffWidthChange = false;

        void Parse( XNODE* aNode );
    };

    //.................................
    // ASSIGNMENTS -> GRIDS
    //.................................

    enum class GRID_TYPE
    {
        FRACTIONALGRID, ///< Param1 = Units, Param2 = Divisor. The grid is equal in X and Y
                        ///< dimensions with a step size equal to Param1/Param2
        STEPGRID        ///< Param1 = X Step, Param2 = Y Step. A standard x,y grid.
    };


    struct GRID
    {
        GRID_TYPE Type;
        wxString  Name;
        long      Param1; ///< Either Units or X step, depending on Type (see GRID_TYPE for
                          ///< more details)
        long Param2;      ///< Either Divisor or Y step, depending on Type (see GRID_TYPE for
                          ///< more details)

        static bool IsGrid( XNODE* aNode );
        void        Parse( XNODE* aNode );
    };


    struct GRIDS
    {
        GRID WorkingGrid;
        GRID ScreenGrid;             ///< From CADSTAR Help: "There is one Screen Grid, which is
                                     ///< visible as dots on the screen. You cannot specify your
                                     ///< own name for the Screen Grid. The visibility and colour
                                     ///< of the dots representing the Screen Grid is set up by
                                     ///< the Highlight category on the Colours dialog.
                                     ///<
                                     ///< The type of Screen grid displayed(Lined or Points) is
                                     ///< set up on the Display dialog within Options(File menu)."
        std::vector<GRID> UserGrids; ///< List of predefined grids created by the user

        void Parse( XNODE* aNode );
    };


    struct ASSIGNMENTS
    {
        LAYERDEFS          Layerdefs;
        CODEDEFS           Codedefs;
        TECHNOLOGY_SECTION Technology;
        GRIDS              Grids;
        bool               NetclassEditAttributeSettings     = false; //< Unclear what this does
        bool               SpacingclassEditAttributeSettings = false; //< Unclear what this does

        void Parse( XNODE* aNode );
    };


    /**
     * @brief Corresponds to "Display when" Item property. From CADSTAR
     *        Help: "This parameter enables you to make the visibility of
     *        a component outline/area (or an area of component copper, or
     *        a string of component text) dependent on the current mirror
     *        status of the component.
     *        
     *        For example, you may require a string of component text to
     *        be displayed only when the component is mirrored."
    */
    enum class SWAP_RULE
    {
        NO_SWAP,        ///< Display when Unmirrored
        USE_SWAP_LAYER, ///< Display when Mirrored
        BOTH            ///< Always display (Mirrored and Unmirrored)
    };

    static SWAP_RULE ParseSwapRule( XNODE* aNode );

    struct REUSEBLOCK
    {
        REUSEBLOCK_ID ID;
        wxString      Name;
        wxString FileName; ///< Filename of the reuse block (usually a .pcb). Used for reloading
        bool     Mirror      = false;
        long     OrientAngle = 0;

        void Parse( XNODE* aNode );
    };


    /**
     * @brief References an element from a design reuse block
    */
    struct REUSEBLOCKREF
    {
        REUSEBLOCK_ID ReuseBlockID = wxEmptyString;
        wxString ItemReference = wxEmptyString; ///< For Components, this references the designator
                                                ///< in the reuse file.
                                                ///< For net elements (such as vias, routes, etc.),
                                                ///< coppers and templates, this parameter is blank.

        bool IsEmpty(); ///< Determines if this is empty (i.e. no design reuse associated)
        void Parse( XNODE* aNode );
    };


    struct GROUP
    {
        GROUP_ID ID;
        wxString Name;
        bool     Fixed   = false;
        GROUP_ID GroupID = wxEmptyString; ///< If not empty, this GROUP
                                          ///< is part of another GROUP
        REUSEBLOCKREF ReuseBlockRef;

        void Parse( XNODE* aNode );
    };


    struct FIGURE
    {
        FIGURE_ID   ID;
        LINECODE_ID LineCodeID;
        LAYER_ID    LayerID;
        SHAPE       Shape; //< Uses the component's coordinate frame if within a component
                           //< definition, otherwise uses the design's coordinate frame.
        GROUP_ID      GroupID = wxEmptyString; ///< If not empty, this FIGURE is part of a group
        REUSEBLOCKREF ReuseBlockRef;
        SWAP_RULE     SwapRule = SWAP_RULE::BOTH; ///< Only applicable to Figures in Components
        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues;

        void Parse( XNODE* aNode );
    };

    /**
     * @brief A shape of copper in the component footprint. For KiCad import, this could
     * be converted to a custom shaped pad (as long as AssociatedPadIDs is not empty)
     */
    struct COMPONENT_COPPER
    {
        COPPERCODE_ID       CopperCodeID;
        LAYER_ID            LayerID;
        SHAPE               Shape; //< Uses the component's coordinate frame.
        SWAP_RULE           SwapRule = SWAP_RULE::BOTH;
        std::vector<PAD_ID> AssociatedPadIDs;

        void Parse( XNODE* aNode );
    };

    /**
     * @brief From CADSTAR Help: "Area is for creating areas within which, and nowhere else, certain
     * operations are carried out (e.g. Placement.); and for creating 'keep out' areas, within which
     * no operations are carried out and where no items are placed by operations such as Placement 
     * and Routing."
     */
    struct COMPONENT_AREA
    {
        COMP_AREA_ID ID;
        LINECODE_ID  LineCodeID;
        LAYER_ID     LayerID;
        SHAPE        Shape; //< Uses the component's coordinate frame.
        SWAP_RULE    SwapRule = SWAP_RULE::BOTH;

        bool NoTracks = false; ///< From CADSTAR Help: "Check this button to specify that any area
                               ///< created by the Rectangle, Circle and Polygon icons can be used
                               ///< by the Auto Router and Route Editor options as the area within
                               ///< which no routes are placed during automatic routing."
        bool NoVias = false;   ///< From CADSTAR Help: "Check this button to specify that any area
                               ///< created by the Rectangle, Circle and Polygon icons can be used
                               ///< by the Auto Router and Route Editor options as the area within
                               ///< which no vias are placed during automatic routing."

        void Parse( XNODE* aNode );
    };


    /**
     * @brief From CADSTAR Help: "This parameter indicates the physical layers on which the selected 
     * pad is placed. Note: When you change the Side parameter in PCB Design, the Side assigned to the
     * pad in the library is not overwritten."
     */
    enum class PAD_SIDE
    {
        MINIMUM,     ///< PHYSICAL_LAYER_ID 1 (i.e. front / top side). Normally used for surface
                     ///< mount devices.
        MAXIMUM,     ///< The highest PHYSICAL_LAYER_ID currently defined (i.e. back / bottom
                     ///< side). Normally used for surface mount devices.
        THROUGH_HOLE ///< All physical layers currently defined
    };


    /**
     * @brief From CADSTAR help: "For specifying the directions in which routes can enter or exit the
     * pad. There are eight pre-defined directions to choose from, North, South, East, West, 
     * North-East, North-West, South-East and South-West, plus "Free Angle" which allows routes to exit
     * in any direction.
     *
     * If none of the direction boxes are checked, the system uses the default which is all directions 
     * (as shown above) for all pad shapes, except the long (drawn) Routes exit from the short sides of 
     * long pads - in other words in line with the long axis.
     *
     * Note: These Exit Directions are applied to the PCB component. If the PCB component is rotated 
     * when it is used on a PCB Design, the Exit Directions will rotate with it."
     *
     * The main thing to note is that the exit angle is not relative to the pad (even if the pad is
     * at an angle): it is relative to the component as a whole. This is confusing, considering this
     * property belongs to the pad...
     */
    struct PAD_EXITS
    {
        bool FreeAngle = false;
        bool North     = false;
        bool NorthEast = false;
        bool East      = false;
        bool SouthEast = false;
        bool South     = false;
        bool SouthWest = false;
        bool West      = false;
        bool NorthWest = false;

        void Parse( XNODE* aNode );
    };


    struct PAD
    {
        PAD_ID     ID;
        POINT      Position; ///< Pad position within the component's coordinate frame.
        PADCODE_ID PadCodeID;
        PAD_SIDE   Side; ///< See PAD_SIDE
        long       OrientAngle = 0;
        PAD_EXITS  Exits; ///< See PAD_EXITS

        wxString Identifier; ///< This is an identifier that is displayed to the user.
                             ///< Internally, the pad is identified by sequential Pad ID
                             ///< (see ID). From CADSTAR Help: "This is how the pin is
                             ///< identified, and is used when creating a part and for reload
                             ///< and replace. It  replaces the CADSTAR 13.0 pad sequence
                             ///< number but is much less restrictive i.e. It need not be 1, 2,
                             ///< 3 etc. and can contain alpha and / or numeric characters."

        bool FirstPad = false; ///< From CADSTAR Help: "Only one pad can have this property; if an
                               ///< existing pad in the design already has this property it will be
                               ///< removed from the existing pad when this new pad is added. The
                               ///< property is used by the 'First Pad' highlight when in a PCB
                               ///< design."
        bool PCBonlyPad =
                false; ///< From CADSTAR Help: "The PCB Only Pad property can be used to stop
                       ///< ECO Update, Back Annotation, and Design Comparison incorrectly
                       ///< acting on mechanical pads / components that only appear in the
                       ///< PCB design."

        void Parse( XNODE* aNode );
    };

    /**
     * @brief Corresponds to CADSTAR "origin". This is used for setting a location of an attribute
     * e.g. Designator (called Component Name in CADSTAR), Part Name (name of component in the
     * libary), etc. The atom identifier is "TEXTLOC"
     */
    struct TEXT_LOCATION
    {
        ATTRIBUTE_ID  AttributeID;
        TEXTCODE_ID   TextCodeID;
        LAYER_ID      LayerID;
        POINT         Position;
        long          OrientAngle = 0;
        bool          Mirror      = false;
        bool          Fixed       = false;
        JUSTIFICATION Justification =
                JUSTIFICATION::LEFT; ///< Note: Justification has no effect on single lines of text
        ALIGNMENT Alignment = ALIGNMENT::
                BOTTOMLEFT; ///< The default alignment for TEXT_LOCATION (when "NO_ALIGNMENT" is
                            ///< selected) is Bottom left, matching CADSTAR's default behaviour

        void Parse( XNODE* aNode );
    };


    struct TEXT
    {
        TEXT_ID       ID;
        wxString      Text; //TODO: Need to lex/parse to identify design fields and hyperlinks
        TEXTCODE_ID   TextCodeID;
        LAYER_ID      LayerID;
        POINT         Position;
        long          OrientAngle = 0;
        bool          Mirror      = false;
        bool          Fixed       = false;
        SWAP_RULE     SwapRule    = SWAP_RULE::BOTH;
        JUSTIFICATION Justification =
                JUSTIFICATION::LEFT; ///< Note: Justification has no effect on single lines of text
        ALIGNMENT Alignment = ALIGNMENT::
                NO_ALIGNMENT; ///< In CADSTAR The default alignment for a TEXT object (when
                              ///< "(No Alignment()" is selected) Bottom Left of the *first line*.
                              ///< Note that this is different from BOTTOM_LEFT (which is bottom
                              ///< left of the whole text block)
        GROUP_ID      GroupID = wxEmptyString; ///< If not empty, this FIGURE is part of a group
        REUSEBLOCKREF ReuseBlockRef;

        void Parse( XNODE* aNode );
    };


    enum class ANGUNITS
    {
        DEGREES,
        RADIANS
    };

    static ANGUNITS ParseAngunits( XNODE* aNode );

    /**
     * @brief Linear, leader (radius/diameter) or angular dimension
    */
    struct DIMENSION
    {
        enum class TYPE
        {
            LINEARDIM, ///< Linear Dimension
            LEADERDIM, ///< Typically used for Radius/Diameter Dimension
            ANGLEDIM   ///< Angular Dimension
        };

        enum class SUBTYPE
        {
            ORTHOGONAL, ///< An orthogonal dimension (either x or y measurement)
                        ///< token=DIMENSION_ORTHOGONAL
            DIRECT,     ///< A linear dimension parallel to measurement with perpendicular
                        ///< extension lines token=DIMENSION_DIRECT
            ANGLED,     ///< A linear dimension parallel to measurement but with orthogonal
                        ///< extension lines (i.e. x or y axis, angled with respect to measurement)
                        ///< token=DIMENSION_ANGLED
            DIAMETER,   ///< token=DIMENSION_DIAMETER
            RADIUS,     ///< token=DIMENSION_RADIUS
            ANGULAR     ///< token=DIMENSION_ANGULAR
        };

        struct ARROW //"DIMARROW"
        {
            enum class STYLE
            {
                OPEN,         ///< The arrow head is made up of two angled lines either side of
                              ///< main line. "DIMENSION_ARROWOPEN"
                CLOSED,       ///< The arrow head is made up of two angled lines either side of
                              ///< main line plus two other lines perpendicular to the main line
                              ///< finishing at the end of each of the two angled lines, with the
                              ///< main line still reaching tip of the arrow.
                              ///< "DIMENSION_ARROWCLOSED"
                CLEAR,        ///< Same as closed but the main line finishes at the start of the
                              ///< perpendicular lines."DIMENSION_ARROWCLEAR"
                CLOSED_FILLED ///< The same as CLOSED or CLEAR arrows, but with a solid fill
                              ///< "DIMENSION_ARROWCLOSEDFILLED"
            };

            STYLE ArrowStyle;  ///< Subnode="ARROWSTYLE"
            long  UpperAngle;  ///< token="ARROWANGLEA"
            long  LowerAngle;  ///< token="ARROWANGLEB"
            long  ArrowLength; ///< The length of the angled lines that make up the arrow head

            void Parse( XNODE* aNode );
        };

        /**
         * @brief Contains formatting specific for a CADSTAR_PCB_ARCHIVE_PARSER::DIMENSION object.
         * Note that none of the parameters has any effect on the position of the dimension text - 
         * it is more of an "intention" of where it should be placed. The user can manually
         * drag the location of the dimension text. Therefore, the actual position of the dimension
         * text is as defined in the `Text` object within CADSTAR_PCB_ARCHIVE_PARSER::DIMENSION.
         *
         * However, TEXTFORMAT does specifify `TextGap` (i.e. how far to pull back the dimension
         * line from the dimension text which does get used when the dimension text is placed
         * on top of the dimension line, if the dimension line is STYLE::INTERNAL).
         *
         * Note: the token is "DIMTEXT" in the CADSTAR format, but this has been renamed to
         * TEXTFORMAT in the cadstar2kicadplugin for ease of understanding. 
         */
        struct TEXTFORMAT
        {
            enum class STYLE ///< Token "TXTSTYLE"
            {
                INSIDE, ///< Embedded with the line (the Gap parameter specifies the gap between
                        ///< the text and the end of the line) DIMENSION_INTERNAL
                OUTSIDE ///< Above the line (the Offset parameter specifies how far above the line
                        ///< the text is) DIMENSION_EXTERNAL
            };

            STYLE Style;
            long  TextGap;    ///< Specifies the gap between the text and the end of the line
            long  TextOffset; ///< Specifies how far above the line the text is (doesn't have
                              ///< an effect on actual position!)

            void Parse( XNODE* aNode );
        };


        struct EXTENSION_LINE ///< Token "EXTLINE"
        {
            LINECODE_ID LineCodeID;

            POINT Start;
            POINT End;
            long  Overshoot;             ///< Overshoot of the extension line past the arrow line
            long  Offset;                ///< Offset from the measurement point
            bool  SuppressFirst = false; ///< If true, excludes the first extension line (only
                                         ///< shows extension line at end)

            void Parse( XNODE* aNode );
        };


        struct LINE ///< Token can be either "LEADERLINE", "LINEARLINE" or "ANGULARLINE"
        {
            enum class TYPE
            {
                LINEARLINE, ///< Only for dimensions of type LINEARDIM
                LEADERLINE, ///< Only for dimensions of type LEADERRDIM. If STYLE = INTERNAL, the
                            ///< result is the same as a LINEARLINE, i.e. all Leader Line-specific
                            ///< elements are ignored.
                ANGULARLINE ///< Only for dimensions of type ANGULARDIM
            };

            enum class STYLE //DIMLINETYPE
            {
                INTERNAL, ///< The lines are placed inside the measurement
                          ///< token=DIMENSION_INTERNAL
                EXTERNAL  ///< The lines are placed outside the measurement
                          ///< (typically used when limited space) token=DIMENSION_EXTERNAL
            };

            TYPE        Type;
            LINECODE_ID LineCodeID; ///< param0
            STYLE       Style;      ///< Subnode="DIMLINETYPE"

            POINT Start;  ///< [point1]
            POINT End;    ///< [point2]
            POINT Centre; ///< Only for TYPE=ANGULARLINE [point3]

            long LeaderAngle = UNDEFINED_VALUE; ///< Only for TYPE=LEADERLINE subnode "LEADERANG"
            long LeaderLineLength = UNDEFINED_VALUE; ///< Only for TYPE=LEADERLINE Length of the
                                                     ///< angled part of the leader line [param5]
            long LeaderLineExtensionLength = UNDEFINED_VALUE; ///< Only for TYPE=LEADERLINE Length
                                                              ///< of the horizontal part of the
                                                              ///< leader line [param6]

            static bool IsLine( XNODE* aNode );
            void        Parse( XNODE* aNode );
        };


        TYPE         Type;
        DIMENSION_ID ID;             ///< Some ID (doesn't seem to be used) subnode="DIMREF"
        LAYER_ID     LayerID;        ///< ID on which to draw this [param1]
        SUBTYPE      Subtype;        ///< [param2]
        long         Precision;      ///< Number of decimal points to display in the measurement
                                     ///< [param3]
        UNITS          LinearUnits;  ///<
        ANGUNITS       AngularUnits; ///< Only Applicable to TYPE=ANGLEDIM
        ARROW          Arrow;
        TEXTFORMAT     TextParams;
        EXTENSION_LINE ExtensionLineParams; ///< Not applicable to TYPE=LEADERDIM
        LINE           Line;
        TEXT           Text;
        bool           Fixed   = false;
        GROUP_ID       GroupID = wxEmptyString; ///< If not empty, this DIMENSION
                                                ///< is part of a group
        REUSEBLOCKREF ReuseBlockRef;

        static bool IsDimension( XNODE* aNode );
        void        Parse( XNODE* aNode );
    };

    /**
     * @brief A symbol definition can represent a number of different objects in CADSTAR.
     * SYMDEF_TYPE is used in cadstar2kicadplugin to simplify identification of each type.
     */
    enum class SYMDEF_TYPE
    {
        COMPONENT, ///< Standard PCB Component definition
        JUMPER,    ///< From CADSTAR Help: "Jumpers are components used primarily for the purpose
                   ///< of routing. A jumper's two pins are connected with a wire, joining together
                   ///< the nets on either side. Typically, a jumper is used to bridge across other
                   ///< routes where a routing problem is particularly difficult or to make a small
                   ///< modification to a design when most of the routing has been finalised.
                   ///<
                   ///< In CADSTAR, components are designated as jumpers in the component footprint.
                   ///< If the footprint has two pins and the first eight letters of the reference
                   ///< name spell "JUMPERNF" then the component will be treated as a jumper.
                   ///<
                   ///< From CADSTAR 8.0 archive format onwards, jumpers are saved as jumpers. For
                   ///< older archive formats and for most other export formats and post
                   ///< processing, jumpers are treated just as components. When CADSTAR saves a PCB
                   ///< design in an old archive format, the implied connection between the
                   ///< jumper's pins is added as an actual connection. When an old archive or
                   ///< binary PCB design is opened in the latest CADSTAR, any component footprint
                   ///< with two pins and whose first eight letters spell "JUMPERNF" will be
                   ///< treated as a jumper footprint automatically. If there is an actual
                   ///< connection between the pins, it will be removed and one implied from then
                   ///< on. If there isn't an actual connection, the nets connected to the pins
                   ///< will be merged. If a merge is needed and if each net has a user-supplied
                   ///< name (not $...), you will be asked to choose which name to use."
        STARPOINT, ///< From CADSTAR Help: "Starpoints are special symbols/components that can be
                   ///< used to electrically connect different nets together, whilst avoiding any
                   ///< of the standard design rule error codes that would normally occur.
                   ///<
                   ///< If the first eleven letters of the reference name spell "STARPOINTNF" then
                   ///< the component will be treated as a starpoint.
                   ///<
                   ///< For a starpoint component to be valid:
                   ///< - There must be at least two pads
                   ///< - All pads must have the same position
                   ///< - All pads must have the same orientation
                   ///< - All pads must be on the same side
                   ///< - All pad codes must be the same
                   ///< - All pads must have the same exit direction
                   ///< - All pads must have the same PCB Only Pad setting
                   ///<
                   ///< If a starpoint component is not valid you will get an error message when
                   ///< trying to add it to the PCB component library."
        TESTPOINT  ///< From CADSTAR Help: "A testpoint is an area of copper connected to a net. It
                   ///< allows a test probe to investigate the electrical properties of that net.
                   ///< The Testpoint Symbols provided in the CADSTAR libraries are the Alternates
                   ///< for a pre-defined symbol called TESTPOINT."
                   ///<
                   ///< If the ReferenceName equals "TESTPOINT", then the component is treated
                   ///< as such. Note that the library manager does not permit adding a component
                   ///< with the name "TESTPOINT" if it has more than one pad defined.
    };


    struct SYMDEF
    {
        SYMDEF_ID   ID;
        SYMDEF_TYPE Type = SYMDEF_TYPE::COMPONENT;
        wxString    ReferenceName; ///< This is the name which identifies the symbol in the library
                                   ///< Multiple components may exist with the same ReferenceName.
        wxString Alternate;        ///< This is in addition to ReferenceName. It allows defining
                                   ///< different versions, views etc. of the same basic symbol.
        POINT Origin;              ///< Origin of the component (this is used as the reference point
                                   ///< when placing the component in the design)
        bool Stub = false;         ///< When the CADSTAR Archive file is exported without the
                                   ///< component library, if components on the board are still
                                   ///< exported, the Reference and Alternate names will still be
                                   ///< exported but the content is replaced with a "STUB" atom,
                                   ///< requiring access to the original library for component
                                   ///< definition.
        long SymHeight;            ///< The Height of the component (3D height in z direction)
        long Version;              ///< Version is a sequential integer number to identify
                                   ///< discrepancies between the library and the design.

        std::map<FIGURE_ID, FIGURE>            Figures;
        std::vector<COMPONENT_COPPER>          ComponentCoppers;
        std::map<COMP_AREA_ID, COMPONENT_AREA> ComponentAreas;
        std::map<TEXT_ID, TEXT>                Texts;
        std::map<PAD_ID, PAD>                  Pads;
        std::map<ATTRIBUTE_ID, TEXT_LOCATION>  TextLocations;    ///< This contains location of
                                                                 ///< any attributes, including
                                                                 ///< designator position
        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues; ///< These attributes might also
                                                                 ///< have a location
        std::map<DIMENSION_ID, DIMENSION> Dimensions;            ///< inside "DIMENSIONS" subnode

        void Parse( XNODE* aNode );
    };


    struct LIBRARY
    {
        std::map<SYMDEF_ID, SYMDEF> ComponentDefinitions;

        void Parse( XNODE* aNode );
    };

    struct PART
    {
        enum class PIN_TYPE
        {
            UNCOMMITTED,        ///< Uncomitted pin (default)
            INPUT,              ///< Input pin
            OUTPUT_OR,          ///< Output pin OR tieable
            OUTPUT_NOT_OR,      ///< Output pin not OR tieable
            OUTPUT_NOT_NORM_OR, ///< Output pin not normally OR tieable
            POWER,              ///< Power pin
            GROUND,             ///< Ground pin
            TRISTATE_BIDIR,     ///< Tristate bi-directional driver pin
            TRISTATE_INPUT,     ///< Tristate input pin
            TRISTATE_DRIVER     ///< Tristate output pin
        };

        static PIN_TYPE GetPinType( XNODE* aNode );

        struct DEFINITION ///< "PARTDEFINITION" node name
        {
            struct GATE ///< "GATEDEFINITION" node name
            {
                GATE_ID  ID;        ///< Usually "A", "B", "C", etc.
                wxString Name;      ///< Symbol name in the symbol library
                wxString Alternate; ///< Symbol alternate name in the symbol library
                long     PinCount;  ///< Number of pins (terminals) in the symbol

                void Parse( XNODE* aNode );
            };


            struct PIN ///< "PARTDEFINITIONPIN" node name
            {
                /**
                 * @brief Positioning of pin names can be in one of four quadrants
                */
                enum class POSITION
                {
                    TOP_RIGHT    = 0, ///< Default
                    TOP_LEFT     = 1,
                    BOTTOM_LEFT  = 2,
                    BOTTOM_RIGHT = 3
                };

                PART_DEFINITION_PIN_ID ID;
                wxString Name = wxEmptyString;            ///< Can be empty. If empty the pin name
                                                          ///< displayed wil be Identifier
                                                          ///< (subnode="PINNAME")
                wxString Label = wxEmptyString;           ///< This Can be empty (subnode=
                                                          ///< "PINLABEL")
                                                          ///< From CADSTAR Help: "Pin
                                                          ///< Labels are an optional replacement
                                                          ///< for the free text sometimes placed
                                                          ///< in schematic symbols. Using Pin
                                                          ///< Labels instead has the advantage of
                                                          ///< associating each piece of label
                                                          ///< text with a particular pin. This
                                                          ///< means that the text remains
                                                          ///< correctly placed after any Gate and
                                                          ///< Pin Swaps are Back Annotated to the
                                                          ///< Schematic design."
                wxString Signal = wxEmptyString;          ///< Usually for Power/Ground pins,
                                                          ///< (subnode="PINSIGNAL")
                GATE_ID     TerminalGate;                 ///< (subnode="PINTERM", param0)
                TERMINAL_ID TerminalPin;                  ///< (subnode="PINTERM", param1)
                PIN_TYPE    Type = PIN_TYPE::UNCOMMITTED; ///< subnode="PINTYPE"
                long        Load = UNDEFINED_VALUE;       ///< The electrical current expected on
                                                          ///< the pin (It is unclear what the units
                                                          ///< are, but only accepted values are
                                                          ///< integers) subnode ="PINLOAD"
                POSITION Position =
                        POSITION::TOP_RIGHT; ///< The pin names will use these positions when
                                             ///< the symbol is added to a schematic design
                                             ///< subnode="PINPOSITION"

                wxString Identifier =
                        wxEmptyString; ///< This should match a pad identifier in the component
                                       ///< footprint subnode="PINIDENTIFIER". It is assumed that
                                       ///< this could be empty in earlier versions of CADSTAR

                void Parse( XNODE* aNode );
            };

            struct PIN_EQUIVALENCE ///< "PINEQUIVALENCE" Node name (represents "Equivalence")
            {
                std::vector<PART_DEFINITION_PIN_ID> PinIDs; ///< All the pins in this vector are
                                                            ///< equivalent and can be swapped with
                                                            ///< each other

                void Parse( XNODE* aNode );
            };


            struct SWAP_GATE ///< "SWAPGATE" Node name (represents an "Element")
            {
                std::vector<PART_DEFINITION_PIN_ID> PinIDs; ///< The pins in this vector
                                                            ///< describe a "gate"

                void Parse( XNODE* aNode );
            };

            struct SWAP_GROUP
            {
                wxString GateName =
                        wxEmptyString; ///< Optional. If not empty, should match the Name
                                       ///< attribute of one of the gates defined in the
                                       ///< part definition

                bool External = false; ///< Determines if this swap group is external (and internal)
                                       ///< or internal only. External Gate swapping allows Gates on
                                       ///< different components with the same Part Definition to
                                       ///< swap with one another.
                                       ///<
                                       ///< The external swapping groups must be at the root level
                                       ///< (i.e. they cannot be contained by other swapping groups)

                std::vector<SWAP_GATE> SwapGates; ///< Each of the elements in this vector can be
                                                  ///< swapped with each other - i.e. *all* of the
                                                  ///< pins in each swap gate can be swapped with
                                                  ///< *all* in another swap gate defined in this
                                                  ///< vector

                void Parse( XNODE* aNode );
            };

            wxString Name;         ///< This name can be different to the PART name
            bool     HidePinNames; ///< Specifies whether to display the pin names/indentifier in
                                   ///< the schematic symbol or not. E.g. it is useful to display
                                   ///< pin name information for integrated circuits but less so
                                   ///< for resistors and capacitors. (subnode HIDEPINNAMES)

            long MaxPinCount =
                    UNDEFINED_VALUE; ///< Optional parameter which is used for specifying the number
                                     ///< of electrical pins on the PCB component symbol to be
                                     ///< used in the  part definition (this should not include
                                     ///< mechanical pins for fixing etc.). This value must be less
                                     ///< than or equal to than the number of pins on the PCB
                                     ///< Component symbol.

            std::map<GATE_ID, GATE>                 GateSymbols;
            std::map<PART_DEFINITION_PIN_ID, PIN>   Pins;
            std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues; ///< Some attributes are
                                                                     ///< defined within the part
                                                                     ///< definition, whilst others
                                                                     ///< are defined in the part
            std::vector<PIN_EQUIVALENCE> PinEquivalences;
            std::vector<SWAP_GROUP>      SwapGroups;

            void Parse( XNODE* aNode );
        };


        struct PART_PIN ///< "PARTPIN" node name
        {
            PART_PIN_ID ID;
            wxString    Name       = wxEmptyString;
            PIN_TYPE    Type       = PIN_TYPE::UNCOMMITTED;
            wxString    Identifier = wxEmptyString;

            void Parse( XNODE* aNode );
        };


        PART_ID                         ID;
        wxString                        Name;
        long                            Version;
        DEFINITION                      Definition;
        std::map<PART_PIN_ID, PART_PIN> PartPins; ///< It is unclear why there are two "Pin"
                                                  ///< structures in CPA files... PART_PIN seems to
                                                  ///< be a reduced version of PART::DEFINITION::PIN
                                                  ///< Therefore, PART_PIN is only included for
                                                  ///< completeness of the parser, but won't be used


        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues; ///< Some attributes are defined
                                                                 ///< within the part definition,
                                                                 ///< whilst others are defined in
                                                                 ///< the part itself

        void Parse( XNODE* aNode );
    };

    struct PARTS
    {
        std::map<PART_ID, PART> PartDefinitions;

        void Parse( XNODE* aNode );
    };


    struct BOARD
    {
        BOARD_ID                                ID;
        LINECODE_ID                             LineCodeID;
        SHAPE                                   Shape;
        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues;
        bool                                    Fixed = false;

        GROUP_ID      GroupID = wxEmptyString; ///< If not empty, this BOARD is part of a group
        REUSEBLOCKREF ReuseBlockRef;           ///< Normally BOARD cannot be part of a reuseblock,
                                               ///< but included for completeness

        void Parse( XNODE* aNode );
    };

    /**
     * @brief From CADSTAR Help: "Area is for creating areas within which, and nowhere else, certain
     * operations are carried out (e.g. Placement.); and for creating 'keep out' areas, within which
     * no operations are carried out and where no items are placed by operations such as Placement 
     * and Routing. [...]
     * More than one function can be assigned to an area."
     */
    struct AREA
    {
        AREA_ID     ID;
        LINECODE_ID LineCodeID;
        wxString    Name;
        LAYER_ID    LayerID;
        SHAPE       Shape;

        //TODO find out what token is used for specifying "Rule Set"
        RULESET_ID RuleSetID = wxEmptyString;

        bool Fixed = false;

        bool Placement = false; ///< From CADSTAR Help: "Auto Placement can place components within
                                ///< this area."
        bool Routing = false;   ///< From CADSTAR Help: "Area can be used to place routes during
                                ///< Automatic Routing."
        bool Keepout = false;   ///< From CADSTAR Help: "Auto Placement cannot place components
                                ///< within this area."
        bool NoTracks = false;  ///< From CADSTAR Help: "Area cannot be used to place routes during
                                ///< automatic routing."
        bool NoVias = false;    ///< From CADSTAR Help: "No vias will be placed within this area by
                                ///< the automatic router."

        long AreaHeight = UNDEFINED_VALUE; ///< From CADSTAR Help: "The Height value specified for
                                           ///< the PCB component is checked against the Height
                                           ///< value assigned to the Area in which the component
                                           ///< is placed. If the component height exceeds the area
                                           ///< height, an error is output"

        GROUP_ID      GroupID = wxEmptyString; ///< If not empty, this AREA is part of a group
        REUSEBLOCKREF ReuseBlockRef;
        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues;

        void Parse( XNODE* aNode );
    };


    struct PIN_ATTRIBUTE
    {
        PART_DEFINITION_PIN_ID                  Pin;
        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues;

        void Parse( XNODE* aNode );
    };

    struct COMPONENT
    {
        COMPONENT_ID ID;
        wxString     Name; ///< Designator e.g. "C1", "R1", etc.
        PART_ID      PartID;
        SYMDEF_ID    SymdefID;
        POINT        Origin; ///< Origin of the component (this is used as the reference point
                             ///< when placing the component in the design)

        GROUP_ID      GroupID = wxEmptyString; ///< If not empty, this component is part of a group
        REUSEBLOCKREF ReuseBlockRef;
        long          OrientAngle = 0;
        bool          TestPoint   = false; ///< Indicates whether this component should be treated
                                           ///< as a testpoint. See SYMDEF_TYPE::TESTPOINT
        bool        Mirror      = false;
        bool        Fixed       = false;
        READABILITY Readability = READABILITY::BOTTOM_TO_TOP;

        std::map<ATTRIBUTE_ID, TEXT_LOCATION> TextLocations; ///< This contains location of
                                                             ///< any attributes, including
                                                             ///< designator position
        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE>    AttributeValues;
        std::map<PART_DEFINITION_PIN_ID, wxString> PinLabels; ///< This is inherited from the
                                                              ///< PARTS library but is allowed
                                                              ///< to be out of sync.
                                                              ///< See PART::DEFINITION::PIN::Label
        std::map<PART_DEFINITION_PIN_ID, PIN_ATTRIBUTE> PinAttributes;

        void Parse( XNODE* aNode );
    };


    struct DOCUMENTATION_SYMBOL
    {
        DOCUMENTATION_SYMBOL_ID ID;

        SYMDEF_ID SymdefID; ///< Normally documentation symbols only have TEXT, FIGURE and
                            ///< TEXT_LOCATION objects which are all drawn on the "(Undefined)"
                            ///< Layer. When used in the design, the user has to specify which
                            ///< layer to draw it on.
        LAYER_ID LayerID;   ///< Move all FIGURE objects in the Symdef to this layer.
        POINT    Origin;    ///< Origin of the component (this is used as the reference point
                            ///< when placing the component in the design)

        GROUP_ID      GroupID = wxEmptyString; ///< If not empty, this component is part of a group
        REUSEBLOCKREF ReuseBlockRef;
        long          OrientAngle = 0;
        bool          Mirror      = false;
        bool          Fixed       = false;
        READABILITY   Readability = READABILITY::BOTTOM_TO_TOP;

        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues;

        void Parse( XNODE* aNode );
    };

    enum class TESTLAND_SIDE
    {
        NONE,
        MAX, ///< The highest PHYSICAL_LAYER_ID currently defined (i.e. back / bottom side).
        MIN, ///< The lowest PHYSICAL_LAYER_ID currently defined (i.e. front / top side).
        BOTH
    };


    static TESTLAND_SIDE ParseTestlandSide( XNODE* aNode );

    struct NET
    {
        struct PIN ///< "PIN" nodename (represents a PAD in a PCB component)
        {
            NETELEMENT_ID ID; ///< First character is "P"
            COMPONENT_ID  ComponentID;
            PAD_ID        PadID;

            void Parse( XNODE* aNode );
        };

        struct VIA ///< "VIA" nodename
        {
            NETELEMENT_ID ID; ///< First character is "V"
            VIACODE_ID    ViaCodeID;
            LAYERPAIR_ID  LayerPairID;
            POINT         Location;
            GROUP_ID      GroupID = wxEmptyString; ///< If not empty, this VIA is part of a group.
            REUSEBLOCKREF ReuseBlockRef;
            TESTLAND_SIDE TestlandSide = TESTLAND_SIDE::NONE;
            bool          Fixed        = false;

            void Parse( XNODE* aNode );
        };

        struct JUNCTION ///< "JPT" nodename.
        {
            NETELEMENT_ID ID; ///< First character is "J"
            LAYER_ID      LayerID;
            POINT         Location;
            GROUP_ID      GroupID = wxEmptyString; ///< If not empty, this JUCTION is part of a
                                                   ///< group
            REUSEBLOCKREF ReuseBlockRef;
            bool          Fixed = false;

            void Parse( XNODE* aNode );
        };


        struct COPPER_TERMINAL ///< "COPTERM" nodename
        {
            NETELEMENT_ID      ID; ///< First two character are "CT"
            COPPER_ID          CopperID;
            NETREF_TERMINAL_ID CopperTermNum;

            void Parse( XNODE* aNode );
        };

        struct ROUTE_VERTEX ///< Two sibbling nodes: first node being "ROUTEWIDTH" and next
                            ///< node being a VERTEX (e.g. PT, CWARC, etc.)
        {
            long   RouteWidth;
            bool   Fixed = false;
            VERTEX Vertex;

            XNODE* Parse( XNODE* aNode ); ///< Returns a pointer to the last node
        };

        struct ROUTE ///< "ROUTE" nodename
        {
            LAYER_ID                  LayerID = wxEmptyString;
            POINT                     StartPoint;
            std::vector<ROUTE_VERTEX> RouteVertices;

            void Parse( XNODE* aNode );
        };

        struct CONNECTION ///< "CONN" nodename
        {
            NETELEMENT_ID StartNode;
            NETELEMENT_ID EndNode;
            ROUTECODE_ID  RouteCodeID;
            ROUTE         Route;
            bool          Unrouted = false; ///< Instead of a ROUTE, the CONNECTION might have an
                                            ///< "UNROUTE" token. This appears to indicate that
                                            ///< the connection is made via a power plane layer
                                            ///< as opposed to a route (track in KiCad terms)
            bool     Fixed   = false;
            bool     Hidden  = false;
            GROUP_ID GroupID = wxEmptyString; ///< If not empty, this connection is part of a group
            REUSEBLOCKREF ReuseBlockRef;

            LAYER_ID UnrouteLayerID = wxEmptyString; ///< See Unrouted member variable.
            std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues; ///< It is possible to add
                                                                     ///< attributes solely to a
                                                                     ///< particular connection

            void Parse( XNODE* aNode );
        };

        NET_ID       ID;
        ROUTECODE_ID RouteCodeID = wxEmptyString;   ///< "NETCODE" subnode
        long         SignalNum   = UNDEFINED_VALUE; ///< This is undefined if the net has been
                                                    ///< given a name. "SIGNUM" subnode.
        wxString Name = wxEmptyString; ///< This is undefined (wxEmptyString) if the net
                                       ///< is unnamed. "SIGNAME" subnode

        std::map<NETELEMENT_ID, PIN>             Pins;
        std::map<NETELEMENT_ID, VIA>             Vias;
        std::map<NETELEMENT_ID, JUNCTION>        Junctions;
        std::map<NETELEMENT_ID, COPPER_TERMINAL> CopperTerminals;
        std::vector<CONNECTION>                  Connections;
        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE>  AttributeValues;

        NETCLASS_ID NetClassID =
                wxEmptyString; ///< The net might not have a net class, in which case it will be
                               ///<  wxEmptyString ("NETCLASSREF" subnode)
        SPACING_CLASS_ID SpacingClassID =
                wxEmptyString; ///< The net might not have a spacing class, in which case it will
                               ///< be wxEmptyString ("SPACINGCLASS" subnode)

        void Parse( XNODE* aNode );
    };

    /**
     * @brief Templates are CADSTAR's equivalent to a "filled zone". However the difference is that
     * in CADSTAR the template just specifies the rules for "pouring" copper. Then, if the template
     * has indeed been "poured", there will be one or more separate COPPER objects linked to the
     * TEMPLATE via COPPER::PouredTemplateID
     */
    struct TEMPLATE
    {
        struct POURING
        {
            enum class COPPER_FILL_TYPE
            {
                UNDEFINED,
                FILLED,
                HATCHED ///< This is a user defined HATCHCODE_ID
            };

            /**
             * @brief From CADSTAR Help: "With this parameter you can select one of two ways in 
             * which to generate thermal reliefs."
             * Note: there doesn't appear to be any noticeable difference between the options.
             */
            enum class RELIEF_TYPE
            {
                CROSS,  ///< This method applies short copper stubs to form a cross. (default)
                CUTOUTS ///< This method uses four cutouts in the copper to leave the reliefs
                        ///< required. Note: The disadvantage of using cutouts is that they
                        ///< can be slower to generate.
            };


            COPPERCODE_ID CopperCodeID;       ///< From CADSTAR Help: "Copper Code is for
                                              ///< selecting the width of the line used to
                                              ///< draw the outline and filling for the copper
                                              ///< shape. This subsequently controls the pen
                                              ///< width (or aperture etc ...) used on a
                                              ///< plotting machine." (param0)
            COPPERCODE_ID ReliefCopperCodeID; ///< From CADSTAR Help: "Relief Copper Code is for
                                              ///< selecting the width of line used to draw the
                                              ///< thermal reliefs for pads connected to the
                                              ///< power planes (created by Copper Pour) This
                                              ///< subsequently controls the pen width (or
                                              ///< aperture etc ...) used on a plotting machine"
                                              ///< (param1)

            long ClearanceWidth;         ///< (param2)
            long SliverWidth;            ///< (param3)
            long AdditionalIsolation;    ///< (param4)
            long ThermalReliefPadsAngle; ///< Disabled when !ThermalReliefOnPads (param5)
            long ThermalReliefViasAngle; ///< Disabled when !ThermalReliefOnVias (param6)
            long MinIsolatedCopper = UNDEFINED_VALUE; ///< Disabled when UNDEFINED_VALUE (param7)
            long MinDisjointCopper = UNDEFINED_VALUE; ///< Disabled when UNDEFINED_VALUE (param8)

            bool ThermalReliefOnPads  = true;  ///< false when subnode "NOPINRELIEF" is present
            bool ThermalReliefOnVias  = true;  ///< false when subnode "NOVIARELIEF" is present
            bool AllowInNoRouting     = false; ///< true when subnode "IGNORETRN" is present
            bool BoxIsolatedPins      = false; ///< true when subnode "BOXPINS" is present
            bool AutomaticRepour      = false; ///< true when subnode "REGENERATE" is present
            bool TargetForAutorouting = false; ///< true when subnode "AUTOROUTETARGET" is present

            RELIEF_TYPE      ReliefType  = RELIEF_TYPE::CROSS; ///< See RELIEF_TYPE
            COPPER_FILL_TYPE FillType    = COPPER_FILL_TYPE::UNDEFINED;
            HATCHCODE_ID     HatchCodeID = wxEmptyString; ///< Only for FillType = HATCHED

            void Parse( XNODE* aNode );
        };

        TEMPLATE_ID   ID;
        LINECODE_ID   LineCodeID;
        wxString      Name;
        NET_ID        NetID;
        LAYER_ID      LayerID;
        POURING       Pouring; ///< Copper pour settings (e.g. relief / hatching /etc.)
        SHAPE         Shape;
        bool          Fixed   = false;
        GROUP_ID      GroupID = wxEmptyString; ///< If not empty, this TEMPLATE is part of a group
        REUSEBLOCKREF ReuseBlockRef;

        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues;

        void Parse( XNODE* aNode );
    };


    struct COPPER
    {
        struct NETREF
        {
            struct TERMINAL
            {
                NETREF_TERMINAL_ID ID;
                POINT              Location;
                bool               Fixed = false;

                void Parse( XNODE* aNode );
            };

            NET_ID                                 NetID = wxEmptyString;
            std::map<NETREF_TERMINAL_ID, TERMINAL> Terminals;
            bool                                   Fixed = false;

            void Parse( XNODE* aNode );
        };

        COPPER_ID     ID;
        COPPERCODE_ID CopperCodeID;
        LAYER_ID      LayerID;
        NETREF        NetRef;
        SHAPE         Shape;
        TEMPLATE_ID   PouredTemplateID = wxEmptyString; ///< If not empty, it means this COPPER
                                                        ///< is part of a poured template.
        bool          Fixed   = false;
        GROUP_ID      GroupID = wxEmptyString; ///< If not empty, this COPPER is part of a group
        REUSEBLOCKREF ReuseBlockRef;

        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues;

        void Parse( XNODE* aNode );
    };


    enum class NETSYNCH
    {
        UNDEFINED,
        WARNING,
        FULL
    };


    struct DRILL_TABLE
    {
        DRILL_TABLE_ID ID;
        LAYER_ID       LayerID;
        POINT          Position;
        long           OrientAngle = 0;
        bool           Mirror      = false;
        bool           Fixed       = false;
        READABILITY    Readability = READABILITY::BOTTOM_TO_TOP;
        GROUP_ID       GroupID     = wxEmptyString; ///< If not empty, this DRILL_TABLE
                                                    ///< is part of a group
        REUSEBLOCKREF ReuseBlockRef;

        void Parse( XNODE* aNode );
    };


    struct LAYOUT
    {
        NETSYNCH NetSynch = NETSYNCH::UNDEFINED;

        std::map<GROUP_ID, GROUP>           Groups;
        std::map<REUSEBLOCK_ID, REUSEBLOCK> ReuseBlocks;

        std::map<BOARD_ID, BOARD> Boards; ///< Normally CADSTAR only allows one board but
                                          ///< implemented this as a map just in case
        std::map<FIGURE_ID, FIGURE>                             Figures;
        std::map<AREA_ID, AREA>                                 Areas;
        std::map<COMPONENT_ID, COMPONENT>                       Components;
        std::map<DOCUMENTATION_SYMBOL_ID, DOCUMENTATION_SYMBOL> DocumentationSymbols;
        std::map<NET_ID, NET>                                   Nets; ///< Contains tracks and vias
        std::map<TEMPLATE_ID, TEMPLATE>                         Templates;
        std::map<COPPER_ID, COPPER>                             Coppers;
        std::map<TEXT_ID, TEXT>                                 Texts;
        std::map<DIMENSION_ID, DIMENSION>                       Dimensions;
        std::map<DRILL_TABLE_ID, DRILL_TABLE>                   DrillTables;

        void Parse( XNODE* aNode );
    };


    wxString Filename;

    HEADER      Header;
    ASSIGNMENTS Assignments;
    LIBRARY     Library;
    PARTS       Parts;
    LAYOUT      Layout;

    int KiCadUnitMultiplier; ///<Use this value to convert units in this CPA file to KiCad units

}; //CADSTAR_PCB_ARCHIVE_PARSER

#endif // CADSTAR_PCB_ARCHIVE_PARSER_H_

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <map>
#include <io/cadstar/cadstar_archive_parser.h>
#include <vector>

//=================================
// MACRO DEFINITIONS
//=================================
#define UNDEFINED_MATERIAL_ID ( MATERIAL_ID ) wxEmptyString
#define UNDEFINED_PHYSICAL_LAYER ( PHYSICAL_LAYER_ID ) - 1

/**
 * @brief Represents a CADSTAR PCB Archive (CPA) file
 */
class CADSTAR_PCB_ARCHIVE_PARSER : public CADSTAR_ARCHIVE_PARSER
{
public:
    explicit CADSTAR_PCB_ARCHIVE_PARSER( const wxString& aFilename ) :
            Filename( aFilename ), Header(), Assignments(),
            m_rootNode( nullptr ), CADSTAR_ARCHIVE_PARSER()
    {
        KiCadUnitMultiplier = 10; // assume hundredth micron
    }


    virtual ~CADSTAR_PCB_ARCHIVE_PARSER()
    {
        if( m_rootNode )
            delete m_rootNode;
    }

    /**
     * @brief Parses the file
     * @throw IO_ERROR if file could not be opened or there was
     * an error while parsing
     */
    void Parse( bool aLibrary = false );

    typedef wxString MATERIAL_ID;
    typedef long     PHYSICAL_LAYER_ID;
    typedef wxString COPPERCODE_ID;
    typedef wxString PADCODE_ID;
    typedef wxString VIACODE_ID;
    typedef wxString SPACINGCODE_ID;
    typedef wxString LAYERPAIR_ID;
    typedef wxString RULESET_ID;
    typedef wxString COMP_AREA_ID;
    typedef long     PAD_ID; ///< Pad identifier (pin) in the PCB
    typedef wxString DIMENSION_ID;
    typedef wxString BOARD_ID;
    typedef wxString AREA_ID;
    typedef wxString COMPONENT_ID;
    typedef wxString TEMPLATE_ID;
    typedef long     COPPER_TERM_ID;
    typedef wxString COPPER_ID;
    typedef wxString DRILL_TABLE_ID;
    typedef wxString TRUNK_ID;

    /**
     * @brief Type of layer appropriate for the material being set up
     */
    enum class MATERIAL_LAYER_TYPE
    {
        CONSTRUCTION,
        ELECTRICAL, ///< Either @see LAYER_TYPE::ELEC or @see LAYER_TYPE::POWER
        NON_ELECTRICAL
    };


    struct MATERIAL : PARSER
    {
        MATERIAL_ID         ID;
        wxString            Name;
        MATERIAL_LAYER_TYPE Type;
        EVALUE              Permittivity;
        EVALUE              LossTangent;
        EVALUE              Resistivity; ///< x10^-8 ohm*metre

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
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
        LAYERSUBTYPE_PASTE,
        LAYERSUBTYPE_CLEARANCE,
        LAYERSUBTYPE_ROUT,
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


    struct LAYER : PARSER
    {
        LAYER_ID          ID;
        wxString          Name;
        wxString          Description = wxEmptyString;
        LAYER_TYPE        Type        = LAYER_TYPE::UNDEFINED;
        LAYER_SUBTYPE     SubType     = LAYER_SUBTYPE::LAYERSUBTYPE_NONE;
        PHYSICAL_LAYER_ID PhysicalLayer =
                UNDEFINED_PHYSICAL_LAYER;                 ///< If UNDEFINED, no physical layer is
                                                          ///< assigned (e.g. documentation and
                                                          ///< construction layers)
        LAYER_ID     SwapLayerID    = UNDEFINED_LAYER_ID; ///< If UNDEFINED_LAYER_ID, no swap layer
        ROUTING_BIAS RoutingBias    = ROUTING_BIAS::UNBIASED;
        long         Thickness      = 0; ///< Note: Units of length are defined in file header
        MATERIAL_ID  MaterialId     = UNDEFINED_MATERIAL_ID;
        EMBEDDING    Embedding      = EMBEDDING::NONE;
        bool         ReferencePlane = false;
        bool         VariantLayer   = false;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct LAYERDEFS : PARSER
    {
        std::map<MATERIAL_ID, MATERIAL> Materials;
        std::map<LAYER_ID, LAYER>       Layers;
        std::vector<LAYER_ID>           LayerStack;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct COPREASSIGN : PARSER
    {
        LAYER_ID LayerID;
        long     CopperWidth;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct COPPERCODE : PARSER
    {
        COPPERCODE_ID            ID;
        wxString                 Name;
        long                     CopperWidth;
        std::vector<COPREASSIGN> Reassigns;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct SPACINGCODE : PARSER
    {
        struct REASSIGN : PARSER
        {
            LAYER_ID LayerID;
            long     Spacing;

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };

        /**
         * @brief Possible spacing rules:
         *        - A_A = Component Placement to Component Placement
         *        - C_B = Copper to Board
         *        - C_C = Copper to Copper
         *        - H_H = Hole to Hole
         *        - OT_P = Optimal Route to Pad (Optional Rule)
         *        - OT_T = Optimal Route to Route (Optional Rule)
         *        - OT_V = Optimal Route to Via (Optional Rule)
         *        - P_B = Pad to Board
         *        - P_C = Pad to Copper
         *        - P_P = Pad to Pad
         *        - P_S = Pad to SMD pad (Optional Rule)
         *        - P_V = Pad to Via
         *        - T_B = Route to Board outline
         *        - T_C = Route to Copper
         *        - T_P = Route to Pad
         *        - T_T = Route to Route
         *        - T_S = Route to SMD Pad (Optional Rule)
         *        - T_V = Route to Via
         *        - S_B = SMD Pad to Board (Optional Rule)
         *        - S_C = SMD Pad to Copper (Optional Rule)
         *        - S_S = SMD Pad to SMD Pad (Optional Rule)
         *        - L_B = Test Land to Board
         *        - L_O = Test Land to Component
         *        - L_L = Test Land to Test Land
         *        - V_B = Via to Board
         *        - V_C = Via to Copper
         *        - V_S = Via to SMD Pad (Optional Rule)
         *        - V_V = Via to Via
         *
         * Other design rules are in:
         * TECHNOLOGY->MAXMITER = Maximum Mitre (This parameter is not actually checked in Cadstar)
         * TECHNOLOGY->MINMITER = Minimum Mitre (This parameter is not actually checked in Cadstar)
         * TECHNOLOGY->MINUNNECKED = Minimum Thicker Track Length
         * TECHNOLOGY->MINNECKED = Minimum Thinner Track Length
         * TECHNOLOGY->MINROUTEWIDTH = Thin Route Width
         */
        SPACINGCODE_ID        ID;
        long                  Spacing;
        std::vector<REASSIGN> Reassigns; ///< Can have different spacings on different layers

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
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


    struct CADSTAR_PAD_SHAPE : PARSER
    {
        PAD_SHAPE_TYPE ShapeType;
        long           Size            = UNDEFINED_VALUE;
        long           LeftLength      = UNDEFINED_VALUE;
        long           RightLength     = UNDEFINED_VALUE;
        long           InternalFeature = UNDEFINED_VALUE;
        long           OrientAngle     = 0; ///< 1/1000 of a Degree

        static bool IsPadShape( XNODE* aNode );
        void        Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct PADREASSIGN : PARSER
    {
        LAYER_ID  LayerID;
        CADSTAR_PAD_SHAPE Shape;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct PADCODE : PARSER
    {
        PADCODE_ID ID;
        wxString   Name;
        CADSTAR_PAD_SHAPE Shape;
        long       ReliefClearance = UNDEFINED_VALUE; ///< if undefined inherits from design
        long       ReliefWidth     = UNDEFINED_VALUE; ///< if undefined inherits from design
        bool       Plated          = true;
        long       DrillDiameter   = UNDEFINED_VALUE;
        long       DrillOversize   = UNDEFINED_VALUE;
        long       SlotLength      = UNDEFINED_VALUE;
        long       SlotOrientation = 0;
        long       DrillXoffset    = 0;
        long       DrillYoffset    = 0;

        std::map<LAYER_ID, CADSTAR_PAD_SHAPE> Reassigns;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct VIAREASSIGN : PARSER
    {
        LAYER_ID  LayerID;
        CADSTAR_PAD_SHAPE Shape;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct VIACODE : PARSER
    {
        VIACODE_ID ID;
        wxString   Name;
        CADSTAR_PAD_SHAPE Shape;
        long       ReliefClearance = UNDEFINED_VALUE; ///< if undefined inherits from design
        long       ReliefWidth     = UNDEFINED_VALUE; ///< if undefined inherits from design
        long       DrillDiameter   = UNDEFINED_VALUE;
        long       DrillOversize   = UNDEFINED_VALUE;

        std::map<LAYER_ID, CADSTAR_PAD_SHAPE> Reassigns;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct LAYERPAIR : PARSER
    {
        LAYERPAIR_ID      ID;
        wxString          Name;
        PHYSICAL_LAYER_ID PhysicalLayerStart;
        PHYSICAL_LAYER_ID PhysicalLayerEnd;
        VIACODE_ID        ViacodeID;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct SPCCLASSSPACE : PARSER
    {
        SPACING_CLASS_ID SpacingClassID1;
        SPACING_CLASS_ID SpacingClassID2;
        LAYER_ID         LayerID; ///< Normally LAY0, which corresponds to (All Layers)
        long             Spacing;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct RULESET : PARSER
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

        std::map<SPACINGCODE_ID, SPACINGCODE>
                SpacingCodes; ///< Overrides these spacing rules in the specific
                              ///< area.
        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct CODEDEFS_PCB : CADSTAR_ARCHIVE_PARSER::CODEDEFS
    {
        std::map<COPPERCODE_ID, COPPERCODE>   CopperCodes;
        std::map<SPACINGCODE_ID, SPACINGCODE> SpacingCodes; ///< Spacing Design Rules
        std::map<RULESET_ID, RULESET>         Rulesets;     ///< Used for area design rules
        std::map<PADCODE_ID, PADCODE>         PadCodes;
        std::map<VIACODE_ID, VIACODE>         ViaCodes;
        std::map<LAYERPAIR_ID, LAYERPAIR>
                                   LayerPairs; ///< Default vias to use between pairs of layers
        std::vector<SPCCLASSSPACE> SpacingClasses;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct TECHNOLOGY_SECTION : CADSTAR_ARCHIVE_PARSER::SETTINGS
    {
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

        bool BackOffJunctions   = false;
        bool BackOffWidthChange = false;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct ASSIGNMENTS : PARSER
    {
        LAYERDEFS          Layerdefs;
        CODEDEFS_PCB       Codedefs;
        TECHNOLOGY_SECTION Technology;
        GRIDS              Grids;
        bool               NetclassEditAttributeSettings     = false; //< Unclear what this does
        bool               SpacingclassEditAttributeSettings = false; //< Unclear what this does

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };

    /**
     * @brief A shape of copper in the component footprint. For KiCad import, this could
     * be converted to a custom shaped pad (as long as AssociatedPadIDs is not empty)
     */
    struct COMPONENT_COPPER : PARSER
    {
        COPPERCODE_ID       CopperCodeID;
        LAYER_ID            LayerID;
        SHAPE               Shape; //< Uses the component's coordinate frame.
        SWAP_RULE           SwapRule = SWAP_RULE::BOTH;
        std::vector<PAD_ID> AssociatedPadIDs;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };

    /**
     * @brief From CADSTAR Help: "Area is for creating areas within which, and nowhere else, certain
     * operations are carried out (e.g. Placement.); and for creating 'keep out' areas, within which
     * no operations are carried out and where no items are placed by operations such as Placement
     * and Routing."
     */
    struct COMPONENT_AREA : PARSER
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

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };

    /**
     * @brief From CADSTAR Help: "This parameter indicates the physical layers on which the selected
     * pad is placed. Note: When you change the Side parameter in PCB Design, the Side assigned to
     * the pad in the library is not overwritten."
     */
    enum class PAD_SIDE
    {
        MINIMUM,     ///< PHYSICAL_LAYER_ID 1 (i.e. front / top side). Normally used for surface
                     ///< mount devices.
        MAXIMUM,     ///< The highest PHYSICAL_LAYER_ID currently defined (i.e. back / bottom
                     ///< side). Normally used for surface mount devices.
        THROUGH_HOLE ///< All physical layers currently defined
    };


    static PAD_SIDE GetPadSide( const wxString& aPadSideString );

    /**
     * @brief From CADSTAR help: "For specifying the directions in which routes can enter or exit
     * the pad. There are eight pre-defined directions to choose from, North, South, East, West,
     * North-East, North-West, South-East and South-West, plus "Free Angle" which allows routes to
     * exit in any direction.
     *
     * If none of the direction boxes are checked, the system uses the default which is all
     * directions (as shown above) for all pad shapes, except the long (drawn) Routes exit from
     * the short sides of long pads - in other words in line with the long axis.
     *
     * Note: These Exit Directions are applied to the PCB component. If the PCB component is rotated
     * when it is used on a PCB Design, the Exit Directions will rotate with it."
     *
     * The main thing to note is that the exit angle is not relative to the pad (even if the pad is
     * at an angle): it is relative to the component as a whole. This is confusing, considering this
     * property belongs to the pad...
     */
    struct PAD_EXITS : PARSER
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

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct COMPONENT_PAD : PARSER
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
        bool PCBonlyPad = false; ///< From CADSTAR Help: "The PCB Only Pad property can be used to
                                 ///< stop ECO Update, Back Annotation, and Design Comparison
                                 ///< incorrectly acting on mechanical pads / components that only
                                 ///< appear in the PCB design."

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };

    /**
     * @brief Linear, leader (radius/diameter) or angular dimension
     */
    struct DIMENSION : PARSER
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


        struct ARROW : PARSER //"DIMARROW"
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
            long  UpperAngle = 0;  ///< token="ARROWANGLEA"
            long  LowerAngle = 0;  ///< token="ARROWANGLEB"
            long  ArrowLength = 0; ///< The length of the angled lines that make up the arrow head

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
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
        struct TEXTFORMAT : PARSER
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

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };


        struct EXTENSION_LINE : PARSER ///< Token "EXTLINE"
        {
            LINECODE_ID LineCodeID;

            POINT Start;
            POINT End;
            long  Overshoot;             ///< Overshoot of the extension line past the arrow line
            long  Offset;                ///< Offset from the measurement point
            bool  SuppressFirst = false; ///< If true, excludes the first extension line (only
                                         ///< shows extension line at end)

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };


        struct LINE : PARSER ///< Token can be either "LEADERLINE", "LINEARLINE" or "ANGULARLINE"
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
            void        Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
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
        void        Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
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


    struct SYMDEF_PCB : CADSTAR_ARCHIVE_PARSER::SYMDEF
    {
        SYMDEF_TYPE Type      = SYMDEF_TYPE::COMPONENT;
        long        SymHeight = 0; ///< The Height of the component (3D height in z direction)

        std::vector<COMPONENT_COPPER>          ComponentCoppers;
        std::map<COMP_AREA_ID, COMPONENT_AREA> ComponentAreas;
        std::map<PAD_ID, COMPONENT_PAD>        ComponentPads;
        std::map<DIMENSION_ID, DIMENSION>      Dimensions; ///< inside "DIMENSIONS" subnode

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct LIBRARY : PARSER
    {
        std::map<SYMDEF_ID, SYMDEF_PCB> ComponentDefinitions;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct CADSTAR_BOARD : PARSER
    {
        BOARD_ID                                ID;
        LINECODE_ID                             LineCodeID;
        SHAPE                                   Shape;
        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues;
        bool                                    Fixed = false;

        ///< If not empty, this CADSTAR_BOARD is part of a group
        GROUP_ID      GroupID = wxEmptyString;

        ///< Normally CADSTAR_BOARD cannot be part of a reuseblock, but included for completeness.
        REUSEBLOCKREF ReuseBlockRef;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };

    /**
     * @brief From CADSTAR Help: "Area is for creating areas within which, and nowhere else, certain
     * operations are carried out (e.g. Placement.); and for creating 'keep out' areas, within which
     * no operations are carried out and where no items are placed by operations such as Placement
     * and Routing. [...]
     * More than one function can be assigned to an area."
     */
    struct AREA : PARSER
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

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    enum class TESTLAND_SIDE
    {
        NONE,
        MAX, ///< The highest PHYSICAL_LAYER_ID currently defined (i.e. back / bottom side).
        MIN, ///< The lowest PHYSICAL_LAYER_ID currently defined (i.e. front / top side).
        BOTH
    };


    static TESTLAND_SIDE ParseTestlandSide( XNODE* aNode );


    struct PIN_ATTRIBUTE : PARSER
    {
        PART_DEFINITION_PIN_ID                  Pin;
        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues;
        TESTLAND_SIDE                           TestlandSide = TESTLAND_SIDE::NONE;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct PADEXCEPTION : PARSER
    {
        PAD_ID     ID;
        PADCODE_ID PadCode      = wxEmptyString; ///< If not empty, override padcode
        bool       OverrideExits = false;
        PAD_EXITS  Exits;
        bool       OverrideSide = false;
        PAD_SIDE   Side;
        bool       OverrideOrientation = false;
        long       OrientAngle         = 0;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct COMPONENT : PARSER
    {
        COMPONENT_ID ID;
        wxString     Name; ///< Designator e.g. "C1", "R1", etc.
        PART_ID      PartID;
        SYMDEF_ID    SymdefID;
        POINT        Origin; ///< Origin of the component (this is used as the reference point
                             ///< when placing the component in the design)

        GROUP_ID      GroupID = wxEmptyString; ///< If not empty, this component is part of a group
        REUSEBLOCKREF ReuseBlockRef;
        COMPONENT_ID  VariantParentComponentID = wxEmptyString;
        VARIANT_ID    VariantID                = wxEmptyString;
        long          OrientAngle              = 0;
        bool          TestPoint = false; ///< Indicates whether this component should be treated
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
        std::map<PAD_ID, PADEXCEPTION>                  PadExceptions; ///< Override pad definitions
                                                                       ///< for this instance

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct TRUNK : PARSER
    {
        TRUNK_ID ID;
        wxString Definition; // TODO: more work required to fully parse the TRUNK structure

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct NET_PCB : CADSTAR_ARCHIVE_PARSER::NET
    {
        struct PIN : PARSER ///< "PIN" nodename (represents a PAD in a PCB component)
        {
            NETELEMENT_ID ID; ///< First character is "P"
            COMPONENT_ID  ComponentID;
            PAD_ID        PadID;

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };

        struct JUNCTION_PCB : CADSTAR_ARCHIVE_PARSER::NET::JUNCTION ///< "JPT" nodename
        {
            TRUNK_ID TrunkID; ///< TRUNKREF Statements

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };

        struct VIA : PARSER ///< "VIA" nodename
        {
            NETELEMENT_ID ID; ///< First character is "V"
            VIACODE_ID    ViaCodeID;
            LAYERPAIR_ID  LayerPairID;
            POINT         Location;
            TRUNK_ID      TrunkID;                 ///< TRUNKREF Statements
            GROUP_ID      GroupID = wxEmptyString; ///< If not empty, this VIA is part of a group.
            REUSEBLOCKREF ReuseBlockRef;
            TESTLAND_SIDE TestlandSide = TESTLAND_SIDE::NONE;
            bool          Fixed        = false;

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };

        struct COPPER_TERMINAL : PARSER ///< "COPTERM" nodename
        {
            NETELEMENT_ID  ID; ///< First two character are "CT"
            COPPER_ID      CopperID;
            COPPER_TERM_ID CopperTermNum;

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };

        struct ROUTE_VERTEX ///< Two sibbling nodes: first node being "ROUTEWIDTH" and
                            ///< next node being a VERTEX (e.g. PT, CWARC, etc.)
        {
            long   RouteWidth;
            bool   TeardropAtStart = false;
            bool   TeardropAtEnd = false;
            long   TeardropAtStartAngle = 0;
            long   TeardropAtEndAngle = 0;
            bool   Fixed = false;
            VERTEX Vertex;

            ///< Returns a pointer to the last node.
            XNODE* Parse( XNODE* aNode, PARSER_CONTEXT* aContext );
        };

        struct ROUTE : PARSER ///< "ROUTE" nodename
        {
            LAYER_ID                  LayerID = wxEmptyString;
            POINT                     StartPoint;
            std::vector<ROUTE_VERTEX> RouteVertices;

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };

        struct CONNECTION_PCB : CADSTAR_ARCHIVE_PARSER::NET::CONNECTION ///< "CONN" nodename
        {
            ROUTE Route;
            bool  Unrouted = false; ///< Instead of a ROUTE, the CONNECTION might have an
                                    ///< "UNROUTE" token. This appears to indicate that
                                    ///< the connection is made via a power plane layer
                                    ///< as opposed to a route (track in KiCad terms)

            LAYER_ID UnrouteLayerID = wxEmptyString; ///< See Unrouted member variable.
            TRUNK_ID TrunkID;                        ///< TRUNKREF Statements

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };

        std::map<NETELEMENT_ID, PIN>             Pins;
        std::map<NETELEMENT_ID, JUNCTION_PCB>    Junctions;
        std::map<NETELEMENT_ID, VIA>             Vias;
        std::map<NETELEMENT_ID, COPPER_TERMINAL> CopperTerminals;
        std::vector<CONNECTION_PCB>              Connections;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };

    /**
     * @brief Templates are CADSTAR's equivalent to a "filled zone". However the difference is that
     * in CADSTAR the template just specifies the rules for "pouring" copper. Then, if the template
     * has indeed been "poured", there will be one or more separate COPPER objects linked to the
     * TEMPLATE via COPPER::PouredTemplateID
     */
    struct TEMPLATE : PARSER
    {
        struct POURING : PARSER
        {
            enum class COPPER_FILL_TYPE
            {
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

            long ClearanceWidth;      ///< Specifies the space around pads when pouring
                                      ///< (i.e. Thermal relief clearance)
            long SliverWidth;         ///< Minimum width of copper that may be created
            long AdditionalIsolation; ///< This is the gap to apply in routes and pads
                                      ///< in addition to the existing pad-to-copper or
                                      ///< route-to-copper spacing (see SPACINGCODE.ID)
            long ThermalReliefPadsAngle; ///< Orientation for the thermal reliefs. Disabled when
                                         ///< !ThermalReliefOnPads (param5)
            long ThermalReliefViasAngle; ///< Disabled when !ThermalReliefOnVias (param6)
            long MinIsolatedCopper = UNDEFINED_VALUE; ///< The value is the length of one side of
                                                      ///< a notional square. Disabled when
                                                      ///< UNDEFINED_VALUE
            long MinDisjointCopper = UNDEFINED_VALUE; ///< The value is the length of one side of
                                                      ///< a notional square. Disabled when
                                                      ///< UNDEFINED_VALUE

            bool ThermalReliefOnPads  = true;  ///< false when subnode "NOPINRELIEF" is present
            bool ThermalReliefOnVias  = true;  ///< false when subnode "NOVIARELIEF" is present
            bool AllowInNoRouting     = false; ///< true when subnode "IGNORETRN" is present
            bool BoxIsolatedPins      = false; ///< true when subnode "BOXPINS" is present
            bool AutomaticRepour      = false; ///< true when subnode "REGENERATE" is present
            bool TargetForAutorouting = false; ///< true when subnode "AUTOROUTETARGET" is present

            RELIEF_TYPE      ReliefType  = RELIEF_TYPE::CROSS;       ///< See RELIEF_TYPE
            COPPER_FILL_TYPE FillType    = COPPER_FILL_TYPE::FILLED; ///< Assume solid fill
            HATCHCODE_ID     HatchCodeID = wxEmptyString; ///< Only for FillType = HATCHED

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
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

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct COPPER : PARSER
    {
        struct NETREF : PARSER
        {
            struct COPPER_TERM : PARSER
            {
                COPPER_TERM_ID ID;
                POINT          Location;
                bool           Fixed = false;

                void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
            };

            NET_ID                                NetID = wxEmptyString;
            std::map<COPPER_TERM_ID, COPPER_TERM> CopperTerminals;
            bool                                  Fixed = false;

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
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

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    enum class NETSYNCH
    {
        UNDEFINED,
        WARNING,
        FULL
    };


    struct DRILL_TABLE : PARSER
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

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct LAYOUT : PARSER
    {
        NETSYNCH NetSynch = NETSYNCH::UNDEFINED;

        std::map<GROUP_ID, GROUP>           Groups;
        std::map<REUSEBLOCK_ID, REUSEBLOCK> ReuseBlocks;

        std::map<BOARD_ID, CADSTAR_BOARD> Boards; ///< Normally CADSTAR only allows one board but
                                          ///< implemented this as a map just in case
        std::map<FIGURE_ID, FIGURE>                             Figures;
        std::map<AREA_ID, AREA>                                 Areas;
        std::map<COMPONENT_ID, COMPONENT>                       Components;
        std::map<DOCUMENTATION_SYMBOL_ID, DOCUMENTATION_SYMBOL> DocumentationSymbols;
        std::map<TRUNK_ID, TRUNK>                               Trunks;
        std::map<NET_ID, NET_PCB>                               Nets; ///< Contains tracks and vias
        std::map<TEMPLATE_ID, TEMPLATE>                         Templates;
        std::map<COPPER_ID, COPPER>                             Coppers;
        std::map<TEXT_ID, TEXT>                                 Texts;
        std::map<DIMENSION_ID, DIMENSION>                       Dimensions;
        std::map<DRILL_TABLE_ID, DRILL_TABLE>                   DrillTables;
        VARIANT_HIERARCHY                                       VariantHierarchy;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    wxString Filename;

    HEADER      Header;
    ASSIGNMENTS Assignments;
    LIBRARY     Library;
    PARTS       Parts;
    LAYOUT      Layout;

    int KiCadUnitMultiplier; ///<Use this value to convert units in this CPA file to KiCad units

private:
    XNODE* m_rootNode; // Currently parsed root node

}; //CADSTAR_PCB_ARCHIVE_PARSER

#endif // CADSTAR_PCB_ARCHIVE_PARSER_H_

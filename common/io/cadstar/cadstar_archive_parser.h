/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020-2021 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
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
 * @file cadstar_archive_parser.h
 * @brief Helper functions and common defines between schematic and PCB Archive files
 */

#ifndef CADSTAR_ARCHIVE_PARSER_H_
#define CADSTAR_ARCHIVE_PARSER_H_

#include <richio.h>
#include <wx/string.h>
#include <functional>
#include <map>
#include <set>
#include <vector>
#include <xnode.h>

#include <math/vector2d.h>
#include <io/cadstar/cadstar_archive_objects.h>

// THROW_IO_ERROR definitions to ensure consistent wording is used in the error messages

#define THROW_MISSING_NODE_IO_ERROR( nodename, location ) \
    THROW_IO_ERROR( wxString::Format( _( "Missing node '%s' in '%s'" ), nodename, location ) )

#define THROW_UNKNOWN_NODE_IO_ERROR( nodename, location ) \
    THROW_IO_ERROR( wxString::Format( _( "Unknown node '%s' in '%s'" ), nodename, location ) )

#define THROW_MISSING_PARAMETER_IO_ERROR( param, location ) \
    THROW_IO_ERROR( wxString::Format( _( "Missing Parameter '%s' in '%s'" ), param, location ) )

#define THROW_UNKNOWN_PARAMETER_IO_ERROR( param, location ) \
    THROW_IO_ERROR( wxString::Format( _( "Unknown Parameter '%s' in '%s'" ), param, location ) )

#define THROW_PARSING_IO_ERROR( param, location ) \
    THROW_IO_ERROR( wxString::Format( _( "Unable to parse '%s' in '%s'" ), param, location ) )

//=================================
// MACRO DEFINITIONS
//=================================
#define UNDEFINED_LAYER_ID ( LAYER_ID ) wxEmptyString


/**
 * Component Name Attribute ID - typically used for placement of designators on silk screen.
 */
#define COMPONENT_NAME_ATTRID ( ATTRIBUTE_ID ) wxT( "__COMPONENT_NAME__" )

/**
 * Component Name 2 Attribute ID - typically used for indicating the placement of designators in
 * placement drawings.
 */
#define COMPONENT_NAME_2_ATTRID ( ATTRIBUTE_ID ) wxT( "__COMPONENT_NAME_2__" )

/**
 * Symbol Name attribute ID - used for placement of designators on the schematic
 */
#define SYMBOL_NAME_ATTRID ( ATTRIBUTE_ID ) wxT( "__SYMBOL_NAME__" )
#define LINK_ORIGIN_ATTRID ( ATTRIBUTE_ID ) wxT( "__LINK_ORIGIN__" )
#define SIGNALNAME_ORIGIN_ATTRID ( ATTRIBUTE_ID ) wxT( "__SIGNALNAME_ORIGIN__" )
#define PART_NAME_ATTRID ( ATTRIBUTE_ID ) wxT( "__PART_NAME__" )

class EDA_TEXT;
class wxXmlAttribute;
class PROGRESS_REPORTER;
class SHAPE_LINE_CHAIN;
class SHAPE_POLY_SET;
class SHAPE_ARC;

/**
 * Helper functions and common structures for CADSTAR PCB and Schematic archive files.
 */
class CADSTAR_ARCHIVE_PARSER
{
public:
    CADSTAR_ARCHIVE_PARSER() { m_progressReporter = nullptr; }

    virtual ~CADSTAR_ARCHIVE_PARSER() {}


    typedef wxString LINECODE_ID;
    typedef wxString HATCHCODE_ID;
    typedef wxString ROUTECODE_ID;
    typedef wxString NETCLASS_ID;
    typedef wxString SPACING_CLASS_ID;
    typedef wxString TEXTCODE_ID;
    typedef wxString LAYER_ID; ///< ID of a Sheet (if schematic) or board Layer (if PCB)
    typedef wxString VARIANT_ID;
    typedef wxString ATTRIBUTE_ID;
    typedef wxString SYMDEF_ID;
    typedef wxString PART_ID;
    typedef wxString GATE_ID;
    typedef long     TERMINAL_ID;            ///< Terminal is the pin identifier in the schematic
    typedef long     PART_DEFINITION_PIN_ID; ///< Pin identifier in the part definition
    typedef long     PART_PIN_ID;            ///< Pin identifier in the part
    typedef wxString TEXT_ID;
    typedef wxString FIGURE_ID;
    typedef wxString GROUP_ID;
    typedef wxString REUSEBLOCK_ID;
    typedef wxString NET_ID;
    typedef wxString NETELEMENT_ID;
    typedef wxString DOCUMENTATION_SYMBOL_ID;
    typedef wxString COLOR_ID;

    static const long UNDEFINED_VALUE = -1;

    /**
     * CADSTAR fonts are drawn on a 24x24 integer matrix, where the each axis goes from 0 to 24.
     * The characters can each specify a width of between 12 and 24, but the height is fixed at 24.
     *
     * The default CADSTAR font uses y=5 as the starting point for capital letters, leaving space
     * for the tails of letters such as "g", "p", "y", "q", etc.
     *
     * The font height in CADSTAR corresponds to the full 24 point height. In KiCad it only
     * corresponds to the height above the guide line, meaning the overall text height will be
     * larger in KiCad.
     */
    static const double TXT_HEIGHT_RATIO;

    /**
     * These are special fields in text objects enclosed between the tokens '<@' and '@>' such as
     * <@[FIELD_NAME][FIELD_VALUE]@>. For example: "<@DESIGN TITLEProject Title@>"
     */
    enum class TEXT_FIELD_NAME
    {
        DESIGN_TITLE,
        SHORT_JOBNAME,
        LONG_JOBNAME,
        NUM_OF_SHEETS,
        SHEET_NUMBER,
        SHEET_NAME,
        VARIANT_NAME,
        VARIANT_DESCRIPTION,
        REG_USER,
        COMPANY_NAME,
        CURRENT_USER,
        DATE,
        TIME,
        MACHINE_NAME,
        FROM_FILE,
        DISTANCE,
        UNITS_SHORT,
        UNITS_ABBREV,
        UNITS_FULL,
        HYPERLINK,
        NONE ///< Synthetic for flagging
    };

    /**
     * Map between CADSTAR fields and KiCad text variables. This is used as a lookup table when
     * parsing CADSTAR text fields. Most variables have a similar name in KiCad as in CADSTAR.
     */
    static const std::map<TEXT_FIELD_NAME, wxString> CADSTAR_TO_KICAD_FIELDS;


    struct PARSER_CONTEXT
    {
        /**
         * CADSTAR doesn't have user defined text fields but does allow loading text from a
         * file. This map stores all the text items that exist in the original CADSTAR design. They
         * will be replaced by a text variable of the form ${FROM_FILE_[filename]_[extension]}
         */
        std::map<wxString, wxString> FilenamesToTextMap;

        /**
         * KiCad doesn't support hyperlinks but we use this map to display warning messages
         * after import. First element = Display Text. Second element = Hyperlink
         */
        std::map<wxString, wxString> TextToHyperlinksMap;

        /**
         * Values for the text field elements used in the CADSTAR design extracted from the
         * text element instances
         */
        std::map<TEXT_FIELD_NAME, wxString> TextFieldToValuesMap;

        /**
         * Text fields need to be updated in CADSTAR and it is possible that they are not
         * consistent across text elements.
         */
        std::set<TEXT_FIELD_NAME> InconsistentTextFields;

        /**
         * Callback function to report progress
         */
        std::function<void()> CheckPointCallback = []() {};
    };

    /**
     * Replaces CADSTAR fields for the equivalent in KiCad and stores the field values
     * in \a aParserContext.
     *
     * @param aTextString Text string to parse.
     * @param aParserContext #PARSER_CONTEXT in which to store the values of the found fields.
     * @return the parsed field.
     */
    static wxString ParseTextFields( const wxString& aTextString, PARSER_CONTEXT* aParserContext );


    struct PARSER
    {
        virtual void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) = 0;

        virtual ~PARSER() {};
    };


    struct FORMAT : PARSER
    {
        wxString Type;
        long     SomeInt; ///< It is unclear what this parameter is used for
        long     Version; ///< Archive version number (e.g. for PCB: 19=> CADSTAR 17.0 archive,
                          ///<  20=> CADSTAR 18.0 archive, 21 => CADSTAR 2018.0 / 2019.0 / 2020.0,
                          ///<  etc.)
        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct TIMESTAMP : PARSER
    {
        long Year;
        long Month;
        long Day;
        long Hour;
        long Minute;
        long Second;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };

    //Note: there are possibly several other resolutions, but HUNDREDTH MICRON is only one known
    enum class RESOLUTION
    {
        HUNDREDTH_MICRON
    };


    struct HEADER : PARSER
    {
        FORMAT     Format;
        wxString   JobFile;
        wxString   JobTitle;
        wxString   Generator;
        RESOLUTION Resolution;
        TIMESTAMP  Timestamp;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct VARIANT : PARSER ///< Nodename = "VARIANT" or "VMASTER" (master variant
    {
        VARIANT_ID ID          = wxEmptyString;
        VARIANT_ID ParentID    = wxEmptyString; ///< if empty, then this one is the master
        wxString   Name        = wxEmptyString;
        wxString   Description = wxEmptyString;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct VARIANT_HIERARCHY : PARSER
    {
        std::map<VARIANT_ID, VARIANT> Variants;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    enum class LINESTYLE
    {
        SOLID,
        DASH,
        DASHDOT,
        DASHDOTDOT,
        DOT
    };


    struct LINECODE : PARSER
    {
        LINECODE_ID ID;
        wxString    Name;
        long        Width;
        LINESTYLE   Style;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct HATCH : PARSER
    {
        long Step;
        long LineWidth;
        long OrientAngle; ///< 1/1000 of a Degree

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct HATCHCODE : PARSER
    {
        HATCHCODE_ID       ID;
        wxString           Name;
        std::vector<HATCH> Hatches;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    static const long FONT_NORMAL = 400;
    static const long FONT_BOLD   = 700;


    struct FONT : PARSER
    {
        wxString Name  = wxT( "CADSTAR" );
        long Modifier1 = FONT_NORMAL; ///< It seems this is related to weight. 400=Normal, 700=Bold.
        long Modifier2 = 0;           ///< It seems this is always 0 regardless of settings
        bool KerningPairs = false;    ///< From CADSTAR Help: "Kerning Pairs is for causing the
                                      ///< system to automatically reduce the spacing between
                                      ///< certain pairs of characters in order to improve the
                                      ///< appearance of the text".
        bool Italic = false;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct TEXTCODE : PARSER
    {
        TEXTCODE_ID ID;
        wxString    Name;
        long        LineWidth;
        long        Height;
        long Width; ///< Defaults to 0 if using system fonts or, if using CADSTAR font, default to
                    ///< equal height (1:1 aspect ratio). Allows for system fonts to be rendered in
                    ///< a different aspect ratio.
        FONT Font;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct ROUTEREASSIGN : PARSER
    {
        LAYER_ID LayerID;
        long     OptimalWidth;
        long     MinWidth;
        long     MaxWidth;
        long     NeckedWidth;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct ROUTECODE : PARSER
    {
        ROUTECODE_ID ID;
        wxString     Name;
        long         OptimalWidth;
        long         MinWidth;
        long         MaxWidth;
        long         NeckedWidth;

        std::vector<ROUTEREASSIGN> RouteReassigns;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    /**
     * Represent a floating value in E notation.
     */
    struct EVALUE : PARSER
    {
        long Base     = 0;
        long Exponent = 0;

        void   Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        double GetDouble();
    };

    /**
     * Represent a point in x,y coordinates.
     */
    struct POINT : VECTOR2I, PARSER
    {
        POINT() : VECTOR2I( UNDEFINED_VALUE, UNDEFINED_VALUE ) {}
        POINT( int aX, int aY ) : VECTOR2I( aX, aY ) {}

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct LONGPOINT : PARSER
    {
        long x = UNDEFINED_VALUE;
        long y = UNDEFINED_VALUE;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct TRANSFORM
    {
        virtual VECTOR2I Apply( POINT aCadstarPoint ) = 0;
    };


    enum class VERTEX_TYPE
    {
        VT_POINT,
        CLOCKWISE_ARC,
        CLOCKWISE_SEMICIRCLE,
        ANTICLOCKWISE_ARC,
        ANTICLOCKWISE_SEMICIRCLE
    };

    /**
     * Represents a vertex in a shape. E.g. A circle is made by two semicircles with the same
     * center point.
     */
    struct VERTEX : PARSER
    {
        VERTEX( VERTEX_TYPE aType = VERTEX_TYPE::VT_POINT, POINT aEnd = POINT(), POINT aCenter = POINT() ) :
                Type( aType ),
                End( aEnd ),
                Center( aCenter )
        {}

        VERTEX_TYPE Type;
        POINT       End;
        POINT       Center;

        static bool IsVertex( XNODE* aNode );
        void        Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;

        void AppendToChain( SHAPE_LINE_CHAIN* aChainToAppendTo,
                            const std::function<VECTOR2I( const VECTOR2I& )> aCadstarToKicadPointCallback,
                            int aAccuracy ) const;

        SHAPE_ARC BuildArc( const VECTOR2I& aPrevPoint,
                            const std::function<VECTOR2I( const VECTOR2I& )> aCadstarToKicadPointCallback ) const;
    };

    /**
     * Represent a cutout in a closed shape (e.g. OUTLINE).
     */
    struct CUTOUT : PARSER
    {
        std::vector<VERTEX> Vertices;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    enum class SHAPE_TYPE
    {
        OPENSHAPE, ///< Unfilled open shape. Cannot have cutouts.
        OUTLINE,   ///< Unfilled closed shape.
        SOLID,     ///< Filled closed shape (solid fill).
        HATCHED    ///< Filled closed shape (hatch fill).
    };


    struct SHAPE : PARSER
    {
        SHAPE_TYPE          Type;
        std::vector<VERTEX> Vertices;
        std::vector<CUTOUT> Cutouts;     ///< Not Applicable to OPENSHAPE Type
        wxString            HatchCodeID; ///< Only Applicable for HATCHED Type

        static bool IsShape( XNODE* aNode );
        void        Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;

        SHAPE_LINE_CHAIN OutlineAsChain( const std::function<VECTOR2I( const VECTOR2I& )> aCadstarToKicadPointCallback,
                                         int aAccuracy ) const;

        SHAPE_POLY_SET ConvertToPolySet( const std::function<VECTOR2I( const VECTOR2I& )> aCadstarToKicadPointCallback,
                                         int aAccuracy  ) const;
    };


    enum class UNITS
    {
        DESIGN, ///< Inherits from design units (assumed Assignments->Technology->Units)
        THOU,
        INCH,
        MICROMETRE,
        MM,
        CENTIMETER,
        METER
    };


    static UNITS ParseUnits( XNODE* aNode );


    enum class ANGUNITS
    {
        DEGREES,
        RADIANS
    };


    static ANGUNITS ParseAngunits( XNODE* aNode );


    enum class GRID_TYPE
    {
        FRACTIONALGRID, ///< Param1 = Units, Param2 = Divisor. The grid is equal in X and Y
                        ///< dimensions with a step size equal to Param1/Param2
        STEPGRID        ///< Param1 = X Step, Param2 = Y Step. A standard x,y grid.
    };


    struct GRID : PARSER
    {
        GRID_TYPE Type;
        wxString  Name;
        long      Param1; ///< Either Units or X step, depending on Type (see GRID_TYPE for
                          ///< more details)
        long      Param2; ///< Either Divisor or Y step, depending on Type (see GRID_TYPE for
                          ///< more details)

        static bool IsGrid( XNODE* aNode );
        void        Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct GRIDS : PARSER
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

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct SETTINGS : PARSER
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
        long AngularPrecision; ///< Number of decimal points to display for angular dimensions

        long PinNoOffset; ///< The distance, of a Pin Name/Identifier from its parent terminal.
        long PinNoAngle; ///< The angle at which the Pin ID is positioned relative to a terminal.

        LONGPOINT               DesignOrigin;
        std::pair<POINT, POINT> DesignArea;
        LONGPOINT               DesignRef; ///< Appears to be 0,0 always
        LONGPOINT               DesignLimit;

        bool         ParseSubNode( XNODE* aChildNode, PARSER_CONTEXT* aContext );
        virtual void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };



    /**
     * From CADSTAR Help: "Text Alignment enables you to define the position of an alignment
     * origin for all text items in CADSTAR.
     *
     * The alignment origin is a point on or within the text boundary and defines how the text
     * is displayed.  For example, with an alignment of bottom-right the origin will be positioned
     * at the bottom right of the text boundary. This makes it easier to right-align several text
     * items regardless of the length of text displayed.  Text Alignment applies to all CADSTAR
     * text. [...]
     *
     * @note Unaligned text operates in the way CADSTAR text always has. In most cases this behaves
     * as Bottom Left alignment, but there are a few exceptions, e.g. pin names. Also unaligned
     * multiline text has an origin Bottom Left of the first line."
     *
     * @see JUSTIFICATION
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
     * From CADSTAR Help: "Multi Line Text can also be justified as Left, Centre or Right.
     *
     * This does not affect the text alignment. Note: Justification of single line text has no
     * effect."  This only affects multiline text
     *
     * @see ALIGNMENT
     */
    enum class JUSTIFICATION
    {
        LEFT,
        CENTER,
        RIGHT
    };


    static JUSTIFICATION ParseJustification( XNODE* aNode );

    /**
     * Sets the readability direction of text. From CADSTAR Help: "Horizontal text will
     * always be displayed from left to right (i.e. never upside down).
     *
     * Vertical text can be set as readable from either the left or right edge of the design."
     * I.e. Vertical text can either be rotated 90 degrees clockwise or 90 degrees counterclockwise
     * from horizontal. This does not impact vertical text.
     */
    enum class READABILITY
    {
        BOTTOM_TO_TOP, ///< When text is vertical, show it rotated 90 degrees anticlockwise
        TOP_TO_BOTTOM  ///< When text is vertical, show it rotated 90 degrees clockwise
    };


    static READABILITY ParseReadability( XNODE* aNode );


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
        SIGNALREF,
        SYMBOL,
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
                  ///< that the attributes that fall under this category are the ones associated
                  ///< with the design itself (i.e. not inherited from the library)
    };


    struct ATTRIBUTE_LOCATION : PARSER
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

        void         ParseIdentifiers( XNODE* aNode, PARSER_CONTEXT* aContext );
        bool         ParseSubNode( XNODE* aChildNode, PARSER_CONTEXT* aContext );
        virtual void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    /**
     * NOTE from CADSTAR help: To convert a Part Definition Attribute into a hyperlink, prefix
     * the attribute name with "Link "
     */
    struct ATTRNAME : PARSER
    {
        struct COLUMNORDER : PARSER
        {
            long ID;
            long Order;

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };


        struct COLUMNWIDTH : PARSER
        {
            long ID;
            long Width;

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
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
                                          ///< "The selected attribute name will be available when
                                          ///< any design is displayed"
                                          ///< "Current Design Type" From CADSTAR Help: This
                                          ///< restricts the availability of the selected attribute
                                          ///< name to the current design.
        std::vector<COLUMNORDER> ColumnOrders;
        std::vector<COLUMNWIDTH> ColumnWidths;
        bool                     ColumnInvisible = false;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct ATTRIBUTE_VALUE : PARSER
    {
        ATTRIBUTE_ID AttributeID;
        wxString     Value;
        bool         ReadOnly    = false;
        bool         HasLocation = false; ///< Flag to know if this ATTRIBUTE_VALUE has a location
                                          ///< i.e. is displayed
        ATTRIBUTE_LOCATION AttributeLocation;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    /**
     * Corresponds to CADSTAR "origin". This is used for setting a location of an attribute
     * e.g. Designator (called Component Name in CADSTAR), Part Name (name of component in the
     * library), etc. The atom identifier is "TEXTLOC"
     */
    struct TEXT_LOCATION : ATTRIBUTE_LOCATION
    {
        TEXT_LOCATION()
        {
            // The default alignment for TEXT_LOCATION (when "NO_ALIGNMENT" is selected) is
            // Bottom left, matching CADSTAR's default behavior
            Alignment = ALIGNMENT::BOTTOMLEFT;
        }
        ATTRIBUTE_ID AttributeID;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct CADSTAR_NETCLASS : PARSER
    {
        NETCLASS_ID                  ID;
        wxString                     Name;
        std::vector<ATTRIBUTE_VALUE> Attributes;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct SPCCLASSNAME : PARSER
    {
        SPACING_CLASS_ID ID;
        wxString         Name;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct CODEDEFS : PARSER
    {
        std::map<LINECODE_ID, LINECODE>          LineCodes;
        std::map<HATCHCODE_ID, HATCHCODE>        HatchCodes;
        std::map<TEXTCODE_ID, TEXTCODE>          TextCodes;
        std::map<ROUTECODE_ID, ROUTECODE>        RouteCodes;
        std::map<ATTRIBUTE_ID, ATTRNAME>         AttributeNames;
        std::map<NETCLASS_ID, CADSTAR_NETCLASS>  NetClasses;
        std::map<SPACING_CLASS_ID, SPCCLASSNAME> SpacingClassNames;

        bool ParseSubNode( XNODE* aChildNode, PARSER_CONTEXT* aContext );
    };

    /**
     * Corresponds to "Display when" Item property.
     *
     * From CADSTAR Help: "This parameter enables you to make the visibility of a component
     * outline/area (or an area of component copper, or a string of component text) dependent
     * on the current mirror status of the component.  For example, you may require a string
     * of component text to be displayed only when the component is mirrored."
     */
    enum class SWAP_RULE
    {
        NO_SWAP,        ///< Display when Unmirrored
        USE_SWAP_LAYER, ///< Display when Mirrored
        BOTH            ///< Always display (Mirrored and Unmirrored)
    };


    static SWAP_RULE ParseSwapRule( XNODE* aNode );


    struct REUSEBLOCK : PARSER
    {
        REUSEBLOCK_ID ID;
        wxString      Name;
        wxString FileName; ///< Filename of the reuse block (usually a .pcb). Used for reloading
        bool     Mirror      = false;
        long     OrientAngle = 0;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    /**
     * References an element from a design reuse block.
     */
    struct REUSEBLOCKREF : PARSER
    {
        REUSEBLOCK_ID ReuseBlockID = wxEmptyString;
        wxString ItemReference = wxEmptyString; ///< For Components, this references the designator
                                                ///< in the reuse file.
                                                ///< For net elements (such as vias, routes, etc.),
                                                ///< coppers and templates, this parameter is blank.

        bool IsEmpty(); ///< Determines if this is empty (i.e. no design reuse associated)
        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct GROUP : PARSER
    {
        GROUP_ID ID;
        wxString Name;
        bool     Fixed    = false;
        bool     Transfer = false;         ///< If true, the group is transferred to PCB
        GROUP_ID GroupID  = wxEmptyString; ///< If not empty, this GROUP
                                           ///< is part of another GROUP
        REUSEBLOCKREF ReuseBlockRef;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct FIGURE : PARSER
    {
        FIGURE_ID   ID;
        LINECODE_ID LineCodeID;
        LAYER_ID    LayerID;
        SHAPE       Shape; //< Uses the component's coordinate frame if within a component
                           //< definition, otherwise uses the design's coordinate frame.
        GROUP_ID      GroupID = wxEmptyString; ///< If not empty, this FIGURE is part of a group
        REUSEBLOCKREF ReuseBlockRef;
        SWAP_RULE     SwapRule = SWAP_RULE::BOTH; ///< Only applicable to Figures in Components
        bool          Fixed    = false;
        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct TEXT : PARSER
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

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext, bool aParseFields );
    };


    struct SYMDEF : PARSER
    {
        SYMDEF_ID ID;
        wxString  ReferenceName; ///< This is the name which identifies the symbol in the library
                                 ///< Multiple components may exist with the same ReferenceName.
        wxString Alternate;      ///< This is in addition to ReferenceName. It allows defining
                                 ///< different versions, views etc. of the same basic symbol.
        POINT Origin;            ///< Origin of the component (this is used as the reference point
                                 ///< when placing the component in the design)
        bool Stub = false;       ///< When the CADSTAR Archive file is exported without the
                                 ///< component library, if components on the board are still
                                 ///< exported, the Reference and Alternate names will still be
                                 ///< exported but the content is replaced with a "STUB" atom,
                                 ///< requiring access to the original library for component
                                 ///< definition.
        long Version = UNDEFINED_VALUE; ///< Version is a sequential integer number to identify
                                        ///< discrepancies between the library and the design.

        std::map<FIGURE_ID, FIGURE>           Figures;
        std::map<TEXT_ID, TEXT>               Texts;
        std::map<ATTRIBUTE_ID, TEXT_LOCATION> TextLocations;     ///< This contains location of
                                                                 ///< any attributes, including
                                                                 ///< designator position
        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues; ///< These attributes might also
                                                                 ///< have a location

        wxString BuildLibName() const;
        void ParseIdentifiers( XNODE* aNode, PARSER_CONTEXT* aContext );
        bool ParseSubNode( XNODE* aChildNode, PARSER_CONTEXT* aContext );
    };


    struct PART : PARSER
    {
        static CADSTAR_PIN_TYPE GetPinType( XNODE* aNode );

        struct DEFINITION : PARSER ///< "PARTDEFINITION" node name
        {
            struct GATE : PARSER ///< "GATEDEFINITION" node name
            {
                GATE_ID  ID;        ///< Usually "A", "B", "C", etc.
                wxString Name;      ///< Symbol name in the symbol library
                wxString Alternate; ///< Symbol alternate name in the symbol library
                long     PinCount;  ///< Number of pins (terminals) in the symbol

                void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
            };


            struct PIN : PARSER ///< "PARTDEFINITIONPIN" node name
            {
                PART_DEFINITION_PIN_ID ID = UNDEFINED_VALUE;

                wxString Identifier = wxEmptyString;      ///< This should match a pad identifier
                                                          ///< in the component footprint
                                                          ///< subnode="PINIDENTIFIER". It is
                                                          ///< assumed that this could be empty in
                                                          ///< earlier versions of CADSTAR
                wxString Name = wxEmptyString;            ///< Can be empty. If empty the pin name
                                                          ///< displayed will be Identifier
                                                          ///< (subnode="PINNAME")
                                                          ///< This seems to be equivalent to "Pin
                                                          ///< Number" in KiCad.
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
                                                          ///< This seems to be equivalent to "Pin
                                                          ///< Name" in KiCad.
                wxString Signal = wxEmptyString;          ///< Usually for Power/Ground pins,
                                                          ///< (subnode="PINSIGNAL")
                GATE_ID     TerminalGate;                 ///< (subnode="PINTERM", param0)
                TERMINAL_ID TerminalPin;                  ///< (subnode="PINTERM", param1)

                CADSTAR_PIN_TYPE Type = CADSTAR_PIN_TYPE::UNCOMMITTED; ///< subnode="PINTYPE"

                long Load = UNDEFINED_VALUE;              ///< The electrical current expected on
                                                          ///< the pin (It is unclear what the units
                                                          ///< are, but only accepted values are
                                                          ///< integers) subnode ="PINLOAD"
                CADSTAR_PIN_POSITION Position =
                    CADSTAR_PIN_POSITION::TOP_RIGHT; ///< The pin names will use these positions
                                                     ///< when the symbol is added to a design
                                                     ///< subnode="PINPOSITION"

                void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
            };


            struct PIN_EQUIVALENCE : PARSER ///< "PINEQUIVALENCE" Node name
            {
                std::vector<PART_DEFINITION_PIN_ID> PinIDs; ///< All the pins in this vector are
                                                            ///< equivalent and can be swapped with
                                                            ///< each other

                void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
            };


            struct SWAP_GATE : PARSER ///< "SWAPGATE" Node name (represents an "Element")
            {
                std::vector<PART_DEFINITION_PIN_ID> PinIDs; ///< The pins in this vector
                                                            ///< describe a "gate"

                void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
            };


            struct SWAP_GROUP : PARSER
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

                void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
            };

            wxString Name; ///< This name can be different to the PART name
            bool     HidePinNames =
                    false; ///< Specifies whether to display the pin names/identifier in
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

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };


        struct PART_PIN : PARSER ///< "PARTPIN" node name
        {
            PART_PIN_ID ID;
            wxString         Name = wxEmptyString;
            CADSTAR_PIN_TYPE Type = CADSTAR_PIN_TYPE::UNCOMMITTED;
            wxString         Identifier = wxEmptyString;

            void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
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

        bool HidePinNames = false; ///< This seems to be a duplicate of DEFINITION::HidePinNames
                                   ///< Possibly only used in older file formats?

        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues; ///< Some attributes are defined
                                                                 ///< within the part definition,
                                                                 ///< whilst others are defined in
                                                                 ///< the part itself

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct PARTS : PARSER
    {
        std::map<PART_ID, PART> PartDefinitions;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct NET : PARSER
    {
        struct JUNCTION : PARSER ///< "JPT" nodename.
        {
            NETELEMENT_ID ID; ///< First character is "J"
            LAYER_ID      LayerID;
            POINT         Location;
            GROUP_ID      GroupID = wxEmptyString; ///< If not empty, this JUNCTION is part of a
                                                   ///< group
            REUSEBLOCKREF ReuseBlockRef;
            bool          Fixed = false;

            void         ParseIdentifiers( XNODE* aNode, PARSER_CONTEXT* aContext );
            bool         ParseSubNode( XNODE* aChildNode, PARSER_CONTEXT* aContext );
            virtual void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
        };


        struct CONNECTION : PARSER ///< "CONN" nodename
        {
            NETELEMENT_ID StartNode;
            NETELEMENT_ID EndNode;
            ROUTECODE_ID  RouteCodeID;

            bool     Fixed   = false;
            bool     Hidden  = false;
            GROUP_ID GroupID = wxEmptyString; ///< If not empty, this connection is part of a group
            REUSEBLOCKREF ReuseBlockRef;

            std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues; ///< It is possible to add
                                                                     ///< attributes solely to a
                                                                     ///< particular connection
            void ParseIdentifiers( XNODE* aNode, PARSER_CONTEXT* aContext );
            bool ParseSubNode( XNODE* aChildNode, PARSER_CONTEXT* aContext );

            virtual ~CONNECTION() {}
        };

        NET_ID       ID;
        ROUTECODE_ID RouteCodeID = wxEmptyString;   ///< "NETCODE" subnode
        long         SignalNum   = UNDEFINED_VALUE; ///< This is undefined if the net has been
                                                    ///< given a name. "SIGNUM" subnode.
        wxString Name = wxEmptyString; ///< This is undefined (wxEmptyString) if the net
                                       ///< is unnamed. "SIGNAME" subnode
        bool Highlight = false;

        std::map<NETELEMENT_ID, JUNCTION>       Junctions;
        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues;

        NETCLASS_ID NetClassID =
                wxEmptyString; ///< The net might not have a net class, in which case it will be
                               ///<  wxEmptyString ("NETCLASSREF" subnode)
        SPACING_CLASS_ID SpacingClassID =
                wxEmptyString; ///< The net might not have a spacing class, in which case it will
                               ///< be wxEmptyString ("SPACINGCLASS" subnode)

        void ParseIdentifiers( XNODE* aNode, PARSER_CONTEXT* aContext );
        bool ParseSubNode( XNODE* aChildNode, PARSER_CONTEXT* aContext );
    };


    struct DOCUMENTATION_SYMBOL : PARSER
    {
        DOCUMENTATION_SYMBOL_ID ID;

        SYMDEF_ID SymdefID; ///< Normally documentation symbols only have TEXT, FIGURE and
                            ///< TEXT_LOCATION objects which are all drawn on the "(Undefined)"
                            ///< Layer. When used in the design, the user has to specify which
                            ///< layer to draw it on.
        LAYER_ID LayerID;   ///< Move all objects in the Symdef to this layer.
        POINT    Origin;    ///< Origin of the component (this is used as the reference point
                            ///< when placing the component in the design)

        GROUP_ID      GroupID = wxEmptyString; ///< If not empty, this component is part of a group
        REUSEBLOCKREF ReuseBlockRef;
        long          OrientAngle = 0;
        bool          Mirror      = false;
        bool          Fixed       = false;
        READABILITY   Readability = READABILITY::BOTTOM_TO_TOP;

        long ScaleRatioNumerator = 1;   ///< Documentation symbols can be arbitrarily scaled when
                                        ///< added to a design
        long ScaleRatioDenominator = 1; ///< Documentation symbols can be arbitrarily scaled when
                                        ///< added to a design

        std::map<ATTRIBUTE_ID, ATTRIBUTE_VALUE> AttributeValues;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct DFLTSETTINGS : PARSER
    {
        COLOR_ID Color;
        bool     IsVisible = true;

        void     Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct ATTRCOL : PARSER
    {
        ATTRIBUTE_ID AttributeID;
        COLOR_ID     Color;
        bool         IsVisible = true;
        bool         IsPickable = true;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct ATTRCOLORS : PARSER
    {
        DFLTSETTINGS DefaultSettings;
        std::map<ATTRIBUTE_ID, ATTRCOL> AttributeColors;

        bool IsVisible = true; // unclear what this represents - maybe all attributes are hidden?

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };


    struct PARTNAMECOL : PARSER
    {
        COLOR_ID Color;
        bool     IsVisible = true;
        bool     IsPickable = true;

        void Parse( XNODE* aNode, PARSER_CONTEXT* aContext ) override;
    };

    ///////////////////////
    // HELPER FUNCTIONS: //
    ///////////////////////


    static void InsertAttributeAtEnd( XNODE* aNode, wxString aValue );

    /**
     * Reads a CADSTAR Archive file (S-parameter format).
     *
     * @param aFileName
     * @param aFileTypeIdentifier Identifier of the first node in the file to check against.
              E.g. "CADSTARPCB"
     * @param aProgressReporter Pointer to a Progress Reporter to report progress to.
     * @return XNODE pointing to the top of the tree for further parsing. Each node has the first
     *         element as the node's name and subsequent elements as node attributes ("attr0",
     *         "attr1", "attr2", etc.). Caller is responsible for deleting to avoid memory leaks.
     * @throws IO_ERROR
     */
    static XNODE* LoadArchiveFile( const wxString& aFileName, const wxString& aFileTypeIdentifier,
                                   PROGRESS_REPORTER* aProgressReporter = nullptr );

    /**
     * @param aAttribute
     * @return
     */
    static bool IsValidAttribute( wxXmlAttribute* aAttribute );

    /**
     * @param aNode
     * @param aID
     * @param aIsRequired Prevents exception throwing if false.
     * @return returns the value (wxString) of attribute "attrX" in aNode where 'X' is aID
     * @throws IO_ERROR if attribute does not exist
     */
    static wxString GetXmlAttributeIDString( XNODE* aNode, unsigned int aID,
                                             bool aIsRequired = true );

    /**
     * @param aNode
     * @param aID
     * @param aIsRequired Prevents exception throwing if false.
     * @return returns the value (long) of attribute "attrX" in aNode where 'X' is aID
     * @throws IO_ERROR if attribute does not exist
     */
    static long GetXmlAttributeIDLong( XNODE* aNode, unsigned int aID, bool aIsRequired = true );

    /**
     * @param aNode
     * @throw IO_ERROR if a child node was found
     */
    static void CheckNoChildNodes( XNODE* aNode );

    /**
     * @param aNode
     * @throw IO_ERROR if a node adjacent to aNode was found
     */
    static void CheckNoNextNodes( XNODE* aNode );

    /**
     * @param aNode with a child node containing an EVALUE
     * @param aValueToParse
     * @throw IO_ERROR if unable to parse or node is not an EVALUE
     */
    static void ParseChildEValue( XNODE* aNode, PARSER_CONTEXT* aContext, EVALUE& aValueToParse );

    /**
     * If no children are present, it just returns an empty vector (without throwing an exception).
     *
     * @param aNode containing a series of POINT objects
     * @param aTestAllChildNodes
     * @param aExpectedNumPoints if UNDEFINED_VALUE (i.e. -1), this is check is disabled
     * @return std::vector containing all POINT objects
     * @throw IO_ERROR if one of the following:
     *         - Unable to parse a POINT object
     *         - aTestAllChildNodes is true and one of the child nodes is not a valid POINT object
     *         - aExpectedNumPoints is non-negative and the number of POINT objects found is
     *           different
     */
    static std::vector<POINT> ParseAllChildPoints( XNODE* aNode, PARSER_CONTEXT* aContext,
                                                   bool aTestAllChildNodes = false,
                                                   int aExpectedNumPoints = UNDEFINED_VALUE );

    /**
     * If no children are present, it just returns an empty vector (without throwing an exception).
     *
     * @param aNode containing a series of VERTEX objects
     * @param aTestAllChildNodes
     * @param aExpectedNumPoints if -1, this is check is disabled
     * @return std::vector containing all VERTEX objects
     * @throw IO_ERROR if one of the following:
     *         - Unable to parse a VERTEX object
     *         - aTestAllChildNodes is true and one of the child nodes is not a valid VERTEX object
     */
    static std::vector<VERTEX> ParseAllChildVertices( XNODE* aNode, PARSER_CONTEXT* aContext,
                                                      bool aTestAllChildNodes = false );

    /**
     * If no children are present, it just returns an empty vector (without throwing an exception).
     *
     * @param aNode containing a series of CUTOUT objects
     * @param aTestAllChildNodes
     * @param aExpectedNumPoints if -1, this is check is disabled
     * @return std::vector containing all CUTOUT objects
     * @throw IO_ERROR if one of the following:
     *         - Unable to parse a CUTOUT object
     *         - aTestAllChildNodes is true and one of the child nodes is not a valid CUTOUT object
     */
    static std::vector<CUTOUT> ParseAllChildCutouts( XNODE* aNode, PARSER_CONTEXT* aContext,
                                                     bool aTestAllChildNodes = false );

    static long GetNumberOfChildNodes( XNODE* aNode );

    static long GetNumberOfStepsForReporting( XNODE*                aRootNode,
                                              std::vector<wxString> aSubNodeChildrenToCount );


    static wxString EscapeFieldText( const wxString& aFieldText )
    {
        wxString ret = aFieldText;
        ret.Replace( wxT( "\n" ), wxT( "\\n" ) );
        ret.Replace( wxT( "\r" ), wxT( "\\r" ) );
        ret.Replace( wxT( "\t" ), wxT( "\\t" ) );

        return ret;
    }

    /**
     * Convert a string with CADSTAR overbar characters to equivalent in KiCad.
     *
     * @param aCadstarString Input string
     * @return KiCad string with overbar characters
     */
    static wxString HandleTextOverbar( wxString aCadstarString );

    /**
     * Correct the position of a text element that had NO_ALIGNMENT in CADSTAR.
     *
     * Assumes that the provided text element has been initialised with a position and orientation.
     *
     * @param aKiCadTextItem a KiCad item to correct
     */
    static void FixTextPositionNoAlignment( EDA_TEXT* aKiCadTextItem );

    static wxString generateLibName( const wxString& aRefName, const wxString& aAlternateName );


protected:
    void checkPoint(); ///< Updates m_progressReporter or throws if user canceled

    PARSER_CONTEXT     m_context;
    PROGRESS_REPORTER* m_progressReporter; // optional; may be nullptr


}; // class CADSTAR_ARCHIVE_PARSER

#endif // CADSTAR_ARCHIVE_PARSER_H_

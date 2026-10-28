/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef TARGET3001_PARSER_H
#define TARGET3001_PARSER_H

#include <array>
#include <cmath>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <wx/datetime.h>
#include <wx/string.h>


/**
 * Reader for TARGET 3001! project files (.T3001).
 *
 * A project is one little-endian stream with no container framing: a version word, a header, four
 * element trees (signals, components, free schematic items, free board items) and the project
 * settings.  Every element is a one-byte class tag followed by that class's body, and lists are
 * elements too, so the trees form a single recursive grammar.  The layout is documented in
 * target3001.ksy next to this file; this parser is a transcription of it.
 *
 * Coordinates are nanometres with Y pointing up.  Schematic items live on layers 101 and up, one per
 * sheet; board items use the board layer numbers.
 */
namespace TARGET3001
{

/// Element class tags as stored in the file.
enum class ELEMENT_TYPE : uint8_t
{
    LINE = 1,
    PIN = 2,
    PAD = 3,
    TRIANGLE = 4,
    RECTANGLE = 5,
    ARC = 6,
    DISC = 7,
    TEXT = 8,
    FIDUCIAL = 9,
    COMPONENT = 10,
    SYMBOL = 11,
    PACKAGE = 12,
    SIGNAL = 13,
    SPLINE = 14,
    DRC_MARKER = 15,
    LIST = 16,
    LAYER_FUNCTIONS = 17,
    NONE = 255
};


/// First layer number used for schematic sheets; sheet N (1-based) is layer 100 + N.
constexpr int SCHEMATIC_LAYER_BASE = 100;

/// Sheets occupy layers 101 to 200
constexpr int SCHEMATIC_SHEET_COUNT = 100;

/// Board elements on this layer exist on every copper layer.
constexpr int ALL_COPPER_LAYER = 100;


/// Fields shared by every drawable element.
struct ELEMENT_BASE
{
    uint16_t component = 0;   ///< ID of the owning component (a component stores its own), 0 for none
    uint16_t signal = 0;      ///< ID of the owning signal (a signal stores its own), 0 for none
    int32_t  x = 0;
    int32_t  y = 0;
    uint8_t  flags = 0;
    uint8_t  status = 0;
    uint8_t  layer = 0;
    uint8_t  classTag = 0;    ///< repeats the element's class tag

    std::optional<uint32_t> extra;

    /// Free-form key/value properties in file order.
    std::vector<std::pair<std::string, std::string>> props;

    /// Return the value of the first property named @a aKey, if any.
    const std::string* FindProp( const std::string& aKey ) const;

    /// True when property @a aKey holds one of the format's boolean true words
    bool FlagProp( const std::string& aKey ) const;

    /// ID of the signal polygon whose stored fill this element is part of, if any
    std::optional<int64_t> PolygonId() const;

    /// Right end of right-aligned text from its ARXY property, in file coordinates
    std::optional<std::pair<int32_t, int32_t>> TextRightEnd() const;

    /// Encoded image a rectangle shows stretched over its area instead of its fill, empty for none
    std::vector<uint8_t> Picture() const;
};


struct LINE
{
    int32_t x2 = 0;
    int32_t y2 = 0;
    int32_t width = 0;
    int32_t clearance = 0;
    uint8_t dashStyle = 0;    ///< 0 solid, 1 long dash, 2 dash-dot, 3 other, 4 short dash
    uint8_t roundEnds = 0;
};


/// Schematic pin.  The base position is the connection point; the pin extends along direction.
struct PIN
{
    float    direction = 0.0f;
    int32_t  length = 0;
    int32_t  width = 0;
    uint8_t  electricalType = 0;
    uint16_t swapLevel = 0;
    uint8_t  inverted = 0;
    uint8_t  clock = 0;
    uint8_t  showNumber = 0;
    uint8_t  supplyPin = 0;
    uint16_t pinNumber = 0;
};


enum class PAD_SHAPE : uint8_t
{
    ROUND = 0,
    OCTAGON = 1,
    RECTANGLE = 2,
    OVAL = 3,
    POLYGON = 4     ///< vertices in the POLY_PAD property
};


struct PAD
{
    float    direction = 0.0f;
    uint8_t  shape = 0;
    int32_t  width = 0;       ///< along the pad's own X axis before rotation
    int32_t  height = 0;
    int32_t  clearance = 0;
    int32_t  drill = 0;       ///< 0 for no hole
    int32_t  reserved = 0;
    uint16_t number = 0;
};


/// Filled triangle given by three absolute corners (the first is the base position).
struct TRIANGLE
{
    int32_t x2 = 0;
    int32_t y2 = 0;
    int32_t x3 = 0;
    int32_t y3 = 0;
};


/// Filled rectangle anchored at a corner (the base position); sizes are signed.
struct RECTANGLE
{
    int32_t height = 0;
    int32_t width = 0;
    float   direction = 0.0f;
};


/// Stroked arc or circle.  The base position is the start point.
struct ARC
{
    int32_t endX = 0;
    int32_t endY = 0;
    int32_t centerX = 0;
    int32_t centerY = 0;
    int32_t radius = 0;
    int32_t width = 0;
    int32_t clearance = 0;
    float   startAngle = 0.0f;    ///< degrees, counter-clockwise from +X
    float   endAngle = 0.0f;

    /// Counter-clockwise sweep in (0, 360]; equal angles give a full circle
    double Sweep() const
    {
        double sweep = std::fmod( double( endAngle ) - double( startAngle ), 360.0 );
        return sweep <= 0.0 ? sweep + 360.0 : sweep;
    }
};


/// Filled disc centred on the base position.
struct DISC
{
    int32_t radius = 0;
};


/// Drawing of a fiducial mark
enum class FIDUCIAL_KIND : uint8_t
{
    CROSSHAIR_RINGS = 0,     ///< cross and rings at 1, 2/3 and 1/3 of the outer centreline radius
    TRIANGLE_DIAMETER = 1,
    PAIRED_SECTORS = 2,
    FILLED_CIRCLE = 3        ///< solid disc of the outer radius
};


struct FIDUCIAL
{
    uint8_t kind = 0;        ///< FIDUCIAL_KIND
    int32_t radius = 0;
    int32_t width = 0;
    float   direction = 0.0f;
};


/// Quadratic Bezier from the base position to the end point.
struct SPLINE
{
    int32_t endX = 0;
    int32_t endY = 0;
    int32_t controlX = 0;
    int32_t controlY = 0;
    int32_t width = 0;
    int32_t clearance = 0;
};


enum class TEXT_FUNCTION : uint8_t
{
    LITERAL = 0,
    REFERENCE = 1,
    VALUE = 2,
    GATE_NAME = 3,
    SIGNAL_NAME = 4,
    PIN_NAME = 5,
    CHANGE_DATE = 11,
    CHANGE_TIME = 12,
    OUTPUT_DATE = 13,
    OUTPUT_TIME = 14,
    SCALE = 15,
    PROJECT_NAME = 16,
    SHEET_NUMBER = 100,
    FILE_NAME = 103,
    PAGE_NAME = 105
};


enum class TEXT_LANGUAGE : uint8_t
{
    GERMAN = 0,
    ENGLISH = 1,
    FRENCH = 2
};


struct TEXT
{
    std::string text;
    int32_t     height = 0;
    int32_t     width = 0;
    float       direction = 0.0f;
    uint8_t     weight = 0;
    uint8_t     noAutoUpright = 0;
    uint8_t     mirrored = 0;
    uint8_t     function = 0;
    std::string font;
};


struct DRC_MARKER
{
    int32_t     radius = 0;
    std::string text;
};


struct ELEMENT;


/// One schematic gate of a component.  Children are already in sheet coordinates.
struct SYMBOL
{
    std::string          name;            ///< full gate name, e.g. "R1a"
    std::string          suffix;          ///< gate suffix, e.g. "a"
    uint16_t             gateOrdinal = 0;
    uint8_t              insertRequired = 0;
    uint16_t             swapLevel = 0;
    std::vector<ELEMENT> children;
};


/// Footprint placement.  Pads and graphics of the footprint are stored in board coordinates.
struct PACKAGE
{
    std::string package;
    std::string library;
    uint16_t    padCount = 0;
    uint8_t     bottomSide = 0;
    float       direction = 0.0f;
};


struct COMPONENT
{
    std::string          name;
    std::string          value;
    std::string          prefix;              ///< reference designator prefix
    std::string          library;
    std::string          package;
    std::string          packageSource;
    uint16_t             referenceNumber = 0;
    std::optional<uint16_t> referenceExtension;
    uint16_t             connectionCount = 0;
    uint8_t              gateState = 0;
    std::vector<ELEMENT> packageItems;
    std::vector<ELEMENT> symbols;
    uint8_t              setBitmap[5] = {};
};


/**
 * Copper polygon of a signal, stored in a "$%&POLY" signal property.  The fill computed from it is
 * stored among the signal's tracks, each element tagged with the polygon's POLYGONID property.
 */
struct POLYGON_POUR
{
    int     layer = 0;
    int64_t id = 0;
    int32_t width = 0;       ///< fill stroke width, 0 for the board default
    int32_t rank = 0;        ///< a polygon is cut out of every one of lower rank on its layer
    bool    thermals = true; ///< own pads join by spokes rather than solidly
    bool    removeIslands = false;
    int32_t edgeClearance = 0; ///< distance kept from the board contour, 0 for none
    int32_t aura = 0;          ///< distance kept from other signals' polygons, 0 for the default

    std::vector<std::pair<int32_t, int32_t>> outline; ///< closed, first vertex repeated last
};


struct SIGNAL
{
    std::string          name;
    std::vector<ELEMENT> connections;    ///< schematic wires, junctions and labels
    std::vector<ELEMENT> tracks;         ///< board copper
    std::vector<POLYGON_POUR> polygons;
    int32_t              minWidth = 0;
    int32_t              minSpacing = 0;
    std::string          string3;
    std::string          string4;
    std::vector<uint8_t> record;
    std::string          string5;
};


/**
 * One element of any class.  Only the member matching @a type is meaningful; lists are flattened
 * into the owning container so a LIST element never appears inside another element.
 */
struct ELEMENT
{
    ELEMENT_TYPE type = ELEMENT_TYPE::NONE;
    ELEMENT_BASE base;

    LINE       line;
    PIN        pin;
    PAD        pad;
    TRIANGLE   triangle;
    RECTANGLE  rectangle;
    ARC        arc;
    DISC       disc;
    TEXT       text;
    FIDUCIAL   fiducial;
    SPLINE     spline;
    DRC_MARKER marker;

    // Larger aggregates are held indirectly to keep ELEMENT small.
    std::shared_ptr<SYMBOL>    symbol;
    std::shared_ptr<PACKAGE>   package;
    std::shared_ptr<COMPONENT> component;
    std::shared_ptr<SIGNAL>    signal;

    /// Children of a pin (its number text).
    std::vector<ELEMENT> children;
};


/// Board layers 0..99 carry a per-project function; 100 and above have fixed meanings
constexpr int BOARD_LAYER_COUNT = 100;


/// A pad on board layer FIRST_PADSTACK_LAYER + n takes its layers from project padstack n
constexpr int FIRST_PADSTACK_LAYER = 101;


/// Project padstack; the board layers a pad exists on and how far it grows on each
struct PADSTACK_DEF
{
    std::string                            name;
    std::array<bool, BOARD_LAYER_COUNT>    onLayer{};
    std::array<int32_t, BOARD_LAYER_COUNT> oversize{};    ///< growth per side, nm
};


/// Settings that the importers use.  The rest of the settings block is validated and skipped.
struct PROJECT_SETTINGS
{
    std::vector<std::string>  sheetNames;     ///< one entry per schematic sheet slot
    std::string               savedBy;

    /// Language whose text variants the project shows; absent in older files
    std::optional<TEXT_LANGUAGE> textLanguage;

    /// Project properties in file order, as (key, value)
    std::vector<std::pair<std::string, std::string>> properties;
    std::vector<PADSTACK_DEF> padstacks;
};


/// Board layer function codes stored per layer in the project header
enum class LAYER_FUNCTION : uint8_t
{
    COPPER_BOTTOM = 0,
    COPPER_INSIDE = 1,
    COPPER_TOP = 2,
    POSITION_BOTTOM = 3,
    POSITION_TOP = 4,
    DELETION_BOTTOM = 5,
    DELETION_INSIDE = 6,
    DELETION_TOP = 7,
    AREA_BOTTOM = 8,
    AREA_INSIDE = 9,
    AREA_TOP = 10,
    MASK_BOTTOM = 11,
    MASK_TOP = 12,
    DRILL_PLAN = 13,
    PASTE_BOTTOM = 14,
    PASTE_TOP = 15,
    GOLD_BOTTOM = 16,
    GOLD_TOP = 17,
    GLUE_BOTTOM = 18,
    GLUE_TOP = 19,
    DIMENSIONING = 20,
    OUTLINE = 21,
    MILLING = 22,
    PAD_NUMBERS = 23,
    OTHER = 24,
    AIR_WIRES = 25,
    NAMES_TOP = 41,
    VALUES_TOP = 42,
    NAMES_BOTTOM = 43,
    VALUES_BOTTOM = 44
};

/// Layer association value for a layer that belongs to no copper layer
constexpr uint8_t NO_ASSOCIATION = 255;


struct PROJECT
{
    int32_t     version = 0;

    /// Function code of each board layer, see LAYER_FUNCTION
    std::array<uint8_t, BOARD_LAYER_COUNT> layerFunctions{};

    /// Copper layer each board layer belongs to; area and deletion layers point at the copper they affect
    std::array<uint8_t, BOARD_LAYER_COUNT> layerAssociations{};

    std::string schematicTextFont;
    std::string boardTextFont;
    std::string signalNameFont;

    std::vector<ELEMENT> signals;
    std::vector<ELEMENT> components;
    std::vector<ELEMENT> schematicItems;
    std::vector<ELEMENT> boardItems;

    PROJECT_SETTINGS settings;

    wxString   fileName;        ///< source file name without directory, empty for a buffer
    wxDateTime fileModified;    ///< source file modification time, invalid for a buffer

    /// Value of project property @a aKey, compared case-insensitively
    const std::string* FindProperty( const std::string& aKey ) const;

    /**
     * Text that a computed text function shows, where KiCad has an equivalent.
     *
     * Stored text holds placeholders in the authoring program's language, so functions are evaluated by
     * code.  Returns nothing to keep the stored text.
     */
    std::optional<wxString> FunctionText( const TEXT& aText, bool aSchematic ) const;

    /// Signal element whose ID is @a aId, or nullptr
    const ELEMENT* FindSignal( uint16_t aId ) const;

    /// Replace literal texts by their variant in the project's stored language when a text reads
    /// AUTOLANGUAGE=TRUE, as the source program does
    void ApplyTextLanguage();
};


/**
 * Parse a complete .T3001 file.  Bytes after the project settings are ignored.
 *
 * @throw IO_ERROR when the file cannot be read, is password protected, or does not match the layout.
 */
PROJECT ParseFile( const wxString& aFileName );

/// Parse a project held in memory.  @see ParseFile
PROJECT ParseBuffer( const std::vector<uint8_t>& aData );

/**
 * Cheap check used by the plugin managers: the extension matches and the header is plausible.
 */
bool LooksLikeProject( const wxString& aFileName );

/// Convert a stored ANSI string to a wxString.
wxString ToWx( const std::string& aRaw );


/// Number of a pad element in KiCad, its PADNAME when present, else its stored number
wxString PadNumber( const ELEMENT& aPad );

} // namespace TARGET3001

#endif // TARGET3001_PARSER_H

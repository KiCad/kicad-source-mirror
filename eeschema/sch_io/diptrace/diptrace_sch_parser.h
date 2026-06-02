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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef DIPTRACE_SCH_PARSER_H_
#define DIPTRACE_SCH_PARSER_H_

#include <io/diptrace/diptrace_binary_reader.h>

#include <map>
#include <memory>
#include <set>
#include <vector>
#include <set>

#include <math/vector2d.h>
#include <wx/string.h>


class LIB_SYMBOL;
class PROGRESS_REPORTER;
class REPORTER;
class SCH_IO;
class SCH_LABEL;
class SCH_LINE;
class SCH_PIN;
class SCH_SCREEN;
class SCH_SHEET;
class SCH_SYMBOL;
class SCHEMATIC;


namespace DIPTRACE
{

// ---------------------------------------------------------------------------
// Intermediate data structures for parsed .dch data
// ---------------------------------------------------------------------------

/// Sheet definition as read from the file header.
struct DCH_SHEET_DEF
{
    wxString name;
    int      field_a = 0;
};


/// A top-level schematic sheet graphical primitive.
struct DCH_SHEET_SHAPE
{
    int                   kindCode = 0; ///< 1 line, 4 rectangle.
    int                   sheetIndex = 0;
    int                   lineWidth = 0;
    uint8_t               color[3] = {};
    std::vector<VECTOR2I> points; ///< Points in DipTrace coord units.
};


/// A component pin as stored in the .dch file.
struct DCH_PIN
{
    int      index = 0;
    bool     hasHeader = false;
    int      headerA = 0;
    int      headerB = 0;
    int      headerC = 0;
    int      typeCode = 0;
    int      x = 0; ///< DipTrace coordinate units (100/3 nm)
    int      y = 0;
    int      length = 0;
    wxString name;
    wxString number;
    int      netFlagA = 0;
    int      netFlagB = 0; ///< 1 when the pin name is shown, 0 when hidden
    int      labelXOff = 0;
    int      labelYOff = 0;
    int      numXOff = 0;
    int      numYOff = 0;
    wxString midTailText;
    int      stubDx = 0;
    int      stubDy = 0;
    int      tailByte = 0;
};


/// A graphical shape primitive (polyline) in a component.
struct DCH_SHAPE
{
    uint8_t               flags[3] = {};
    int                   shapeField = 0;
    int                   lineWidth = 0;
    std::vector<VECTOR2I> points; ///< Points in DipTrace coord units
    int                   fontX = 0;
    int                   fontY = 0;
    /// Leading kind int3: 1 line, 3 arrow, 4 rect, 6 obround, 8 filled polygon,
    /// 9 outline polygon/polyline.
    int kindCode = 0;
    /// Leading kind int3; observed 0 for decoded drawing shapes.
    int kindFlag = 0;
};


/// A stored component text field record that precedes the embedded footprint pattern.
struct DCH_COMPONENT_TEXT
{
    uint8_t  flags[3] = {};
    int      type = 0;
    wxString fontName;
    wxString text;
    int      fontSize = 0;
    int      fieldA = 0;
    int      coordX = 0;
    int      coordY = 0;
    int      fieldB = 0;
    int      fieldC = 0;
    uint8_t  flagA = 0;
    uint8_t  flagB = 0;
    int      fieldD = 0;
    int      fieldE = 0;
    int      fieldF = 0;
    int      fieldG = 0;
    uint8_t  flags2[4] = {};
    int      fieldH = 0;
};


/// A component as read from the .dch file.
struct DCH_COMPONENT
{
    size_t   fileOffset = 0;
    int      bboxX1 = 0;
    int      bboxY1 = 0;
    int      bboxX2 = 0;
    int      bboxY2 = 0;
    wxString compName;
    wxString refdes;
    wxString value;
    wxString prefix;
    wxString nameDup;
    wxString partName;
    wxString partNumber;
    bool     isMultiPart = false;
    wxString partId;
    int      sheetIndex = 0;
    int      rotationE4 = 0; ///< Placement rotation in radians x 1e4 (0, 15708, 31416, 47124)
    wxString libPath;
    wxString patternName; ///< Embedded footprint pattern name (e.g. "LED100", "CR0805")
    wxString datasheet;   ///< Datasheet URL stored in the placement tail

    /// User-defined additional fields, as (name, value) pairs (e.g. "Part Number (Digi-Key)").
    std::vector<std::pair<wxString, wxString>> additionalFields;

    std::vector<DCH_PIN>            pins;
    std::vector<DCH_SHAPE>          shapes;
    std::vector<DCH_COMPONENT_TEXT> texts;
};


/// A bus entry as read from the bus section.
struct DCH_BUS_ENTRY
{
    int      coordX = 0;
    int      coordY = 0;
    int      sheetIndex = 0;
    int      busType = 0;
    int      instanceId = 0;
    int      signalCount = 0;
    wxString name;
};


/// A net label/wire entry from the net section.
struct DCH_NET_ENTRY
{
    wxString name;
    int      coordX = 0;
    int      coordY = 0;
    int      field1 = 0;
};


/// Decoded page geometry. DipTrace stores the sheet centered on the origin, so the importer offsets
/// every absolute placement by half the page to land it on the top-left-origin KiCad page.
struct DCH_PAGE
{
    bool   found = false; ///< True when a page record was located in the binary
    double widthMM = 0.0;
    double heightMM = 0.0;
};


/// A single schematic wire decoded from the net/wire section.
struct DCH_WIRE
{
    int                   sheetIndex = 0; ///< DipTrace sheet index
    std::vector<VECTOR2I> points;         ///< KiCad nm, ready for SCH_LINE
    int                   object1 = 0;    ///< Connected item id at endpoint 1
    int                   subObject1 = 0; ///< Pin/sub index at endpoint 1
    int                   bus1 = -1;      ///< Bus index at endpoint 1 (-1 = none)
    int                   object2 = 0;
    int                   subObject2 = 0;
    int                   bus2 = -1;
};


// ---------------------------------------------------------------------------
// Parser class
// ---------------------------------------------------------------------------

/**
 * Parser for DipTrace .dch schematic binary files.
 *
 * Reads the binary file using DIPTRACE::BINARY_READER, populates intermediate
 * data structures, then creates KiCad schematic objects (SCH_SYMBOL, SCH_LINE,
 * SCH_LABEL, SCH_SHEET, LIB_SYMBOL).
 *
 * Supports format versions 31 through 49.
 */
class SCH_PARSER
{
public:
    /**
     * @param aFileName path to the DipTrace .dch file.
     * @param aSchematic the KiCad schematic object being populated.
     * @param aRootSheet the root sheet to populate.
     * @param aProgressReporter optional progress reporter (may be nullptr).
     * @param aReporter optional message reporter (may be nullptr).
     */
    SCH_PARSER( const wxString& aFileName, SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet,
                PROGRESS_REPORTER* aProgressReporter = nullptr, REPORTER* aReporter = nullptr );

    ~SCH_PARSER();

    /**
     * Parse the .dch file and populate the schematic with KiCad objects.
     *
     * @throw IO_ERROR on fatal parse errors.
     */
    void Parse();

    int ComponentBoundaryScanCount() const { return m_componentBoundaryScanCount; }

private:
    // -- Binary format parsing ------------------------------------------------

    void parseHeader();
    void parseSheetDefinitions();
    void parseDisplaySettings();
    void parseTextStyles();
    void parsePreComponentSettings();
    void parseComponents( size_t aBusSectionOffset );
    void parseOneComponent( size_t aCompEnd, bool aUseCompEnd = true );
    void parsePin( int aPinIndex, DCH_COMPONENT& aComp );
    void parseShape( DCH_COMPONENT& aComp );
    void parseFontBearingShape( DCH_COMPONENT& aComp );
    bool parseComponentTextField( DCH_COMPONENT& aComp, size_t aCompEnd );
    void parseEmbeddedPattern( DCH_COMPONENT& aComp, size_t aCompEnd );
    void parseBusSection();
    void parseNetSection();
    void parseWireSection();
    void parseSheetShapes();

    /// Locate the page-geometry record (width/height/margins, each mm*30000) in the binary and fill
    /// m_page.  The record is not present in every file; m_page.found stays false when absent.
    void findPageGeometry();

    /// Apply the page-center offset to an absolute KiCad-nm placement so 0,0-centered DipTrace
    /// content lands on the top-left-origin KiCad page.  A no-op when no page was decoded.
    VECTOR2I applyPageOffset( const VECTOR2I& aPos ) const;

    // -- Structural scanning --------------------------------------------------

    /**
     * Find the bus section start offset by searching for the characteristic
     * marker pattern: int4(10000) int4(10000) byte(0) byte(0) int3(count).
     */
    size_t findBusSection( size_t aSearchStart ) const;

    /**
     * Find where the int3(0) tail padding begins by scanning backward
     * from the end of file.
     */
    size_t findTailStart() const;

    /**
     * Pre-scan the file to find component start offsets using the
     * bbox(4*int4) + 5-string pattern.
     */
    std::vector<size_t> scanComponentBoundaries( size_t aFirstComp, size_t aBusSectionOffset ) const;

    // -- Shape detection helpers -----------------------------------------------

    /**
     * Check if the data at the given offset looks like a shape/polyline start.
     */
    bool isShapeStart( size_t aOffset ) const;
    bool isFontBearingShapeStart( size_t aOffset ) const;

    // -- KiCad object creation ------------------------------------------------

    /**
     * Create KiCad objects from the parsed intermediate data and add them
     * to the appropriate schematic sheets.
     */
    void createKiCadObjects();

    /**
     * Get or create the KiCad sheet and screen for the given DipTrace sheet index.
     */
    SCH_SCREEN* getOrCreateSheet( int aSheetIndex );
    void        finalizeFlatSheetOrder();
    void        assignSheetPageNumbers();

    /**
     * Create a LIB_SYMBOL from the DipTrace component data.
     *
     * If a library symbol with the same name already exists, returns the
     * existing one (handles multi-unit symbols by adding units).
     */
    LIB_SYMBOL* getOrCreateLibSymbol( const DCH_COMPONENT& aComp, int aUnit );
    void        populateLibSymbolUnit( LIB_SYMBOL* aLibSymbol, const DCH_COMPONENT& aComp, int aUnit );
    void        syncEmbeddedLibrarySymbols();

    /**
     * Library symbol name for a component. Multi-part components group by normalized reference so
     * their units share one symbol. Single-part symbols keep a rotation suffix because their pin and
     * shape coordinates are stored already rotated.
     */
    wxString componentSymbolName( const DCH_COMPONENT& aComp ) const;
    wxString normalizedRefdes( const DCH_COMPONENT& aComp ) const;

    /**
     * Create a SCH_SYMBOL instance on the given screen from a DipTrace component.
     */
    void createSymbolInstance( const DCH_COMPONENT& aComp, SCH_SCREEN* aFallbackScreen );

    /**
     * Create a global net label for every DipTrace net-port component (auto_net_ports library).
     * The label name is the port's component name (which equals its net name) and its placement is
     * the port's stored bbox center, so ports without an explicit wire still appear.  This is the
     * only label source; internal auto-named nets own no port object and so carry no label, matching
     * the DipTrace rendering.  Fills m_netPortNames and m_netPortLabelCount.
     */
    void createNetPortLabels();

    /// Sheet whose decoded wire geometry is closest to aPos, or -1 when no wire exists.  Used as the
    /// final fallback for an isolated net port whose pin matched no wire and whose part id has no
    /// wire-derived sheet.
    int sheetForNearestWire( const VECTOR2I& aPos ) const;

    /// Emit SCH_LINE wire segments decoded from the net/wire section.
    void createWires();
    void createSheetShapes();

    /// Synthesize junctions where conductors coincide (DipTrace stores none explicitly).
    void createJunctions();

    /// Build the maps from wire-point position to the sheet(s) and part(s) connecting there, and
    /// the part-id -> sheet map used for deterministic sheet assignment.
    void buildWirePointSheets();

    /// Enumerate every component header in the file (real components and net ports alike) so each
    /// component's file offset maps to its DipTrace part id.  Fills m_offsetToPartId.
    void buildComponentPartIds();

    /// True if a component record header (placement + five header strings) starts at aOffset.
    /// Looser than scanComponentBoundaries so net ports are enumerated too.
    bool isComponentHeaderAt( size_t aOffset ) const;

    /**
     * Pick the sheet a placed item belongs to by matching its connection points against the
     * decoded wire geometry (DipTrace does not store a per-component sheet field).  Returns the
     * majority-voted sheet, or aFallback when no point coincides with a wire.
     */
    int sheetForPositions( const std::vector<VECTOR2I>& aPositions, int aFallback ) const;

    /**
     * Resolve a symbol's sheet from its pin connection points.  Wire endpoints carry both a part
     * id and a sheet, so the candidate (partId, sheet) pairs are tallied; on identical/duplicate
     * sheets the tie is broken by the monotonic part-id order (components are stored in part-id
     * order), which selects the correct one of the duplicated sheets.  Falls back to position
     * voting, then to the root sheet.  Updates m_lastSymbolPartId.
     */
    int sheetForComponentPins( const std::vector<VECTOR2I>& aConnectionPoints );

    /// Resolve a (partId, sheet) -> hit-count tally to a sheet, preferring the highest-count pair
    /// with a part id greater than the last assigned (monotonic).  Updates m_lastSymbolPartId.
    int resolveSheetTally( const std::map<std::pair<int, int>, int>& aTally );

    /**
     * Determine the pin orientation from the pin connection-point offset relative to the symbol
     * body center.  DipTrace stores the connection point and a length; the body extends inward, so
     * a pin on the right side of the body points left, and so on.  Returns a KiCad orientation code
     * (0 = right, 1 = up, 2 = left, 3 = down).
     */
    static int pinOrientationFromOffset( int aOffsetX, int aOffsetY, int aHalfWidth, int aHalfHeight );

    /**
     * Convert a DipTrace coordinate to KiCad schematic internal units.
     *
     * One DipTrace coordinate unit is 100/3 nm and one schematic IU is 100 nm,
     * so the conversion divides by 3. The PCB importer shares the 100/3 nm unit
     * but multiplies by 100/3 because pcbnew IU is 1 nm.
     *
     * DipTrace .dch stores schematic Y already in screen-down convention (matching KiCad), with any
     * placement rotation baked into the stored coordinates, so Y is scaled without an axis flip.
     */
    static int toKiCadCoordX( int aDipTraceCoord );
    static int toKiCadCoordY( int aDipTraceCoord );

    /**
     * Convert a DipTrace length or stroke width to KiCad schematic internal units.
     *
     * Same unit as coordinates but unsigned and without the Y axis flip.
     */
    static int toKiCadSize( int aDipTraceCoord );

    /**
     * Build a library name string for the import.
     */
    wxString getLibName() const;

    // -- Member data ----------------------------------------------------------

    BINARY_READER      m_reader;
    SCHEMATIC*         m_schematic;
    SCH_SHEET*         m_rootSheet;
    PROGRESS_REPORTER* m_progressReporter;
    REPORTER*          m_reporter;
    int                m_version;
    int                m_magicMajor;
    int                m_componentCount;
    int                m_componentBoundaryScanCount = 0;
    wxString           m_fileName;

    // Parsed intermediate data
    int                          m_numSheets;
    std::vector<DCH_SHEET_DEF>   m_sheetDefs;
    std::vector<DCH_COMPONENT>   m_components;
    std::vector<DCH_BUS_ENTRY>   m_buses;
    std::vector<DCH_NET_ENTRY>   m_nets;
    std::vector<DCH_WIRE>        m_wires;
    std::vector<DCH_SHEET_SHAPE> m_sheetShapes;

    /// Wire-point position (KiCad nm) -> set of sheet indices carrying a wire there.  Used to
    /// recover each symbol's / label's owning sheet, which DipTrace stores only implicitly.
    std::map<std::pair<int, int>, std::set<int>> m_wirePointSheets;

    /// Wire-endpoint position (KiCad nm) -> (partId, sheet) pairs of the parts connecting there.
    /// Lets a symbol recover its exact part id and disambiguate duplicate sheets.
    std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> m_pointPartSheets;

    /// Largest part id assigned to a symbol so far; enforces the monotonic part-id order used to
    /// disambiguate identical duplicate sheets.
    int m_lastSymbolPartId = -1;

    /// Decoded page geometry and the resulting half-page placement offset (KiCad nm).
    DCH_PAGE m_page;
    VECTOR2I m_pageOffset;

    /// File offset of the component section start, used to enumerate components in part-id order.
    size_t m_componentSectionStart = 0;

    /// End offset of the decoded wire section; the sheet-shape section follows it in modern files.
    size_t m_wireSectionEnd = 0;

    /// Component start offset -> DipTrace part id (its index in the in-file component order).
    std::map<size_t, int> m_offsetToPartId;

    /// DipTrace part id -> sheet index, resolved from the wire connectivity.  This places a symbol
    /// on its true sheet even when its pin geometry was mis-parsed.
    std::map<int, int> m_partIdSheet;

    /// Names of nets that own a placed net-port component; these are the only nets DipTrace draws a
    /// label for, so they are the only labels emitted (createNetPortLabels).
    std::set<wxString> m_netPortNames;

    /// Count of net-port labels emitted, for the import summary report.
    size_t m_netPortLabelCount = 0;

    // KiCad object management
    std::vector<SCH_SHEET*>                         m_sheets;     ///< One per DipTrace sheet
    std::map<wxString, std::unique_ptr<LIB_SYMBOL>> m_libSymbols; ///< Symbol library cache
    std::map<wxString, std::vector<SCH_SYMBOL*>>    m_placedSymbolsByLibName;

    /// Map from refdes to the number of units already created for multi-unit symbols.
    std::map<wxString, int> m_refdesUnitMap;

    size_t m_busSectionOffset;
    size_t m_tailOffset;
};

} // namespace DIPTRACE

#endif // DIPTRACE_SCH_PARSER_H_

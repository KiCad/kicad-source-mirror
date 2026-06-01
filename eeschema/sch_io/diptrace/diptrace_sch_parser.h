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
#include <vector>

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


/// A component pin as stored in the .dch file.
struct DCH_PIN
{
    int      index      = 0;
    bool     hasHeader  = false;
    int      headerA    = 0;
    int      headerB    = 0;
    int      headerC    = 0;
    int      typeCode   = 0;
    int      x          = 0;   ///< DipTrace coordinate units (10 nm)
    int      y          = 0;
    int      length     = 0;
    wxString name;
    wxString number;
    int      netFlagA   = 0;
    int      netFlagB   = 0;
    int      labelXOff  = 0;
    int      labelYOff  = 0;
    int      numXOff    = 0;
    int      numYOff    = 0;
    int      stubDx     = 0;
    int      stubDy     = 0;
    int      tailByte   = 0;
};


/// A graphical shape primitive (polyline) in a component.
struct DCH_SHAPE
{
    uint8_t               flags[3] = {};
    int                   shapeField = 0;
    int                   lineWidth  = 0;
    std::vector<VECTOR2I> points;        ///< Points in DipTrace coord units
    int                   fontX      = 0;
    int                   fontY      = 0;
};


/// A component as read from the .dch file.
struct DCH_COMPONENT
{
    size_t   fileOffset  = 0;
    int      bboxX1      = 0;
    int      bboxY1      = 0;
    int      bboxX2      = 0;
    int      bboxY2      = 0;
    wxString compName;
    wxString refdes;
    wxString value;
    wxString prefix;
    wxString nameDup;
    wxString partName;
    wxString partNumber;
    bool     isMultiPart = false;
    wxString partId;
    int      sheetIndex  = 0;
    int      libId       = 0;
    wxString libPath;
    wxString patternName;   ///< Embedded footprint pattern name (e.g. "LED100", "CR0805")

    std::vector<DCH_PIN>   pins;
    std::vector<DCH_SHAPE> shapes;
};


/// A bus entry as read from the bus section.
struct DCH_BUS_ENTRY
{
    int      coordX      = 0;
    int      coordY      = 0;
    int      sheetIndex  = 0;
    int      busType     = 0;
    int      instanceId  = 0;
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


/// A net-name label to place on a specific sheet, derived from the net/wire section.
struct DCH_NET_LABEL
{
    wxString name;
    VECTOR2I pos;                ///< KiCad nm, a wire endpoint carrying the net
    int      sheetIndex = 0;
};


/// A single schematic wire decoded from the net/wire section.
struct DCH_WIRE
{
    int                   sheetIndex = 0;   ///< DipTrace sheet index
    std::vector<VECTOR2I> points;           ///< KiCad nm, ready for SCH_LINE
    int                   object1    = 0;   ///< Connected item id at endpoint 1
    int                   subObject1 = 0;   ///< Pin/sub index at endpoint 1
    int                   bus1       = -1;  ///< Bus index at endpoint 1 (-1 = none)
    int                   object2    = 0;
    int                   subObject2 = 0;
    int                   bus2       = -1;
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
                PROGRESS_REPORTER* aProgressReporter = nullptr,
                REPORTER* aReporter = nullptr );

    ~SCH_PARSER();

    /**
     * Parse the .dch file and populate the schematic with KiCad objects.
     *
     * @throw IO_ERROR on fatal parse errors.
     */
    void Parse();

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
    void parseEmbeddedPattern( DCH_COMPONENT& aComp, size_t aCompEnd );
    void parseBusSection();
    void parseNetSection();
    void parseWireSection();

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
    std::vector<size_t> scanComponentBoundaries( size_t aFirstComp,
                                                 size_t aBusSectionOffset ) const;

    /**
     * Validate that a refdes string matches expected patterns.
     */
    static bool isValidRefdes( const wxString& aRefdes );

    // -- Shape detection helpers -----------------------------------------------

    /**
     * Check if the data at the given offset looks like a shape/polyline start.
     */
    bool isShapeStart( size_t aOffset ) const;

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

    /**
     * Create a LIB_SYMBOL from the DipTrace component data.
     *
     * If a library symbol with the same name already exists, returns the
     * existing one (handles multi-unit symbols by adding units).
     */
    LIB_SYMBOL* getOrCreateLibSymbol( const DCH_COMPONENT& aComp, int aUnit );

    /**
     * Create a SCH_SYMBOL instance on the given screen from a DipTrace component.
     */
    void createSymbolInstance( const DCH_COMPONENT& aComp, SCH_SCREEN* aFallbackScreen );

    /**
     * Create net label objects from parsed net entries.
     */
    void createNetLabels();

    /// Emit SCH_LINE wire segments decoded from the net/wire section.
    void createWires();

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
     * Determine the pin orientation from the stub direction vector.
     */
    static int pinOrientationFromStub( int aStubDx, int aStubDy );

    /**
     * Convert a DipTrace coordinate (10 nm units) to KiCad internal units.
     *
     * KiCad eeschema uses nanometers internally (same as PCB).
     * DipTrace coordinate unit is 10 nm, so: kicad_nm = diptrace * 10.
     *
     * DipTrace Y axis increases upward; KiCad Y axis increases downward,
     * so Y coordinates are negated.
     */
    static int toKiCadCoordX( int aDipTraceCoord );
    static int toKiCadCoordY( int aDipTraceCoord );

    /**
     * Build a library name string for the import.
     */
    wxString getLibName() const;

    // -- Member data ----------------------------------------------------------

    BINARY_READER        m_reader;
    SCHEMATIC*           m_schematic;
    SCH_SHEET*           m_rootSheet;
    PROGRESS_REPORTER*   m_progressReporter;
    REPORTER*            m_reporter;
    int                  m_version;
    int                  m_magicMajor;
    int                  m_componentCount;
    wxString             m_fileName;

    // Parsed intermediate data
    int                          m_numSheets;
    std::vector<DCH_SHEET_DEF>   m_sheetDefs;
    std::vector<DCH_COMPONENT>   m_components;
    std::vector<DCH_BUS_ENTRY>   m_buses;
    std::vector<DCH_NET_ENTRY>   m_nets;
    std::vector<DCH_WIRE>        m_wires;

    /// Wire-point position (KiCad nm) -> set of sheet indices carrying a wire there.  Used to
    /// recover each symbol's / label's owning sheet, which DipTrace stores only implicitly.
    std::map<std::pair<int, int>, std::set<int>> m_wirePointSheets;

    /// Wire-endpoint position (KiCad nm) -> (partId, sheet) pairs of the parts connecting there.
    /// Lets a symbol recover its exact part id and disambiguate duplicate sheets.
    std::map<std::pair<int, int>, std::vector<std::pair<int, int>>> m_pointPartSheets;

    /// Largest part id assigned to a symbol so far; enforces the monotonic part-id order used to
    /// disambiguate identical duplicate sheets.
    int                          m_lastSymbolPartId = -1;

    /// File offset of the component section start, used to enumerate components in part-id order.
    size_t                       m_componentSectionStart = 0;

    /// Component start offset -> DipTrace part id (its index in the in-file component order).
    std::map<size_t, int>        m_offsetToPartId;

    /// DipTrace part id -> sheet index, resolved from the wire connectivity.  This places a symbol
    /// on its true sheet even when its pin geometry was mis-parsed.
    std::map<int, int>           m_partIdSheet;

    /// Net-name labels (one per net per sheet) derived from the net/wire section.
    std::vector<DCH_NET_LABEL>   m_netLabels;

    // KiCad object management
    std::vector<SCH_SHEET*>                         m_sheets;       ///< One per DipTrace sheet
    std::map<wxString, std::unique_ptr<LIB_SYMBOL>> m_libSymbols;   ///< Symbol library cache

    /// Map from refdes to the number of units already created for multi-unit symbols.
    std::map<wxString, int> m_refdesUnitMap;

    size_t m_busSectionOffset;
    size_t m_tailOffset;
};

} // namespace DIPTRACE

#endif // DIPTRACE_SCH_PARSER_H_

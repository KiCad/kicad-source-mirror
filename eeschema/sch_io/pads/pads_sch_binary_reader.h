/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PADS_SCH_BINARY_READER_H_
#define PADS_SCH_BINARY_READER_H_

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <wx/string.h>

class SCHEMATIC;
class SCH_SHEET;
class SCH_SCREEN;
class SCH_SHEET_PATH;

namespace PADS_SCH_BINARY
{

/// One placed schematic symbol recovered from the binary .sch part array.
/// Placement of one component text field (refdes/value/user) relative to the symbol origin.
struct FIELD_PLACEMENT
{
    int  dx_mils = 0;          ///< Page-relative X offset from the symbol origin.
    int  dy_mils = 0;          ///< Page-relative Y offset (PADS Y-up).
    int  orientation_deg = 0;  ///< 0/90/180/270.
    bool visible = false;      ///< Whether the field is shown.
    bool valid = false;        ///< A real record was decoded for this field.
};

struct PLACEMENT
{
    std::string reference;   ///< Reference designator (e.g. "U1", "J6-1").
    int         x_mils = 0;  ///< Page X in mils.
    int         y_mils = 0;  ///< Page Y in mils (PADS is Y-up).
    int         rotation = 0;///< 0/90/180/270 degrees.
    int         sheetIndex = 0; ///< Owning PADS sheet (0-based).
    std::string decalName;   ///< Bound CAE-decal (gate symbol) name, empty if unbound.
    std::string partType;    ///< Bound part-type name, empty if unbound.

    /// Component user-attribute fields (key, value) of this part's part-type.
    std::vector<std::pair<std::string, std::string>> fields;

    /// Reference-designator field placement (inline in the placement record).
    FIELD_PLACEMENT refdesPlace;

    /// Per-field placements from the post-placement array, index-aligned with @ref fields.
    std::vector<FIELD_PLACEMENT> fieldPlaces;
};

/// One drawing piece of a CAE decal (a polyline of decal-relative mil vertices).
struct DECAL_PIECE
{
    bool                            closed = false;  ///< Filled/closed outline.
    int                             width_mils = 0;
    std::vector<std::pair<int, int>> verts;          ///< Decal-relative mils.
};

/// One CAE decal (gate symbol) body and its pin terminals.
struct DECAL
{
    std::string                      name;
    std::vector<DECAL_PIECE>         pieces;
    std::vector<std::pair<int, int>> terminals;      ///< Pin connection points (mils).
};

/// One wire vertex recovered from the binary 8-byte vertex pool.
struct WIRE_VERTEX
{
    int x_mils = 0;
    int y_mils = 0;
};

/// One junction (PADS tie-dot) recovered from the binary 12-byte tie-dot pool.
struct JUNCTION
{
    int x_mils = 0;
    int y_mils = 0;
    int sheetIndex = 0; ///< Owning PADS sheet (0-based).
};

/// What KiCad element a recovered off-page reference represents.
enum class NETLABEL_KIND
{
    GLOBAL, ///< Off-sheet connector ($OSR group) -> global label.
    LOCAL,  ///< Net-name port (@TERM) -> local label.
    POWER,  ///< Power/ground port ($GND/$PWR group) -> power symbol.
    BUS     ///< Bus tap (@@@Bn) -> bus label.
};

/// One net label / off-page reference (power port, off-sheet connector).
struct NET_LABEL
{
    int           x_mils = 0;
    int           y_mils = 0;
    std::string   netName;
    int           sheetIndex = 0;
    NETLABEL_KIND kind = NETLABEL_KIND::GLOBAL;
};

/// One free-text item recovered from the binary 32-byte text record pool.
struct TEXT_ITEM
{
    int         x_mils = 0;
    int         y_mils = 0;
    int         orientation_deg = 0; ///< 0 or 90.
    int         justification = 0;   ///< PADS JUST column verbatim.
    int         height_mils = 0;
    int         linewidth_mils = 0;
    std::string text;                ///< Recovered string content (may be empty).
    int         sheetIndex = 0;      ///< Owning PADS sheet (0-based).
};

/**
 * Reader for the proprietary PADS Logic binary .sch format (magic 00 FE,
 * version 0x000D).
 *
 * eeschema's primary PADS path is the PADS-LOGIC ASCII export; this reader
 * adds a path for the binary .sch.  The decode is driven entirely by the
 * serialized record structure:
 *
 *   - SYMBOLS  the stride-136 part-instance records, one run per sheet, framed
 *              by the MFC class tag and the text-style trailer; recovered as a
 *              generic placeholder symbol at the stored page position and
 *              orientation (the placement->parttype->graphic link is a runtime
 *              heap pointer and is not in the file, so the real symbol graphic
 *              cannot be recovered)
 *   - WIRES    the 8-byte vertex pools tiled by the stride-40 split-header
 *              cumulative-index chain, emitted as SCH_LINE wires; the explicit
 *              gap slices between cumulative jumps are bus polylines
 *   - TEXT     the 32-byte free-text records (position, orientation,
 *              justification, height, linewidth), with string content recovered
 *              by an ordered length-matched walk of the shared string pool
 *   - JUNCTIONS the 12-byte tie-dot records (one run per sheet, marker 0xfc),
 *              emitted as SCH_JUNCTION
 *
 * The placement->parttype->graphic link (the real symbol body) is recovered
 * through the part-type and used-decal pools rather than the 136-byte record;
 * that binding is decoded separately.
 */
class PADS_SCH_BINARY_READER
{
public:
    PADS_SCH_BINARY_READER() = default;

    /// Return true if @p aData is a PADS Logic binary schematic.
    static bool IsBinarySch( const std::vector<uint8_t>& aData );

    /// Read the file at @p aFileName into @p aData. Returns false on I/O error.
    static bool ReadFile( const wxString& aFileName, std::vector<uint8_t>& aData );

    /// Parse @p aData. Returns false if the container header is invalid.
    bool Parse( const std::vector<uint8_t>& aData );

    const std::vector<PLACEMENT>&                   GetPlacements() const { return m_placements; }
    const std::vector<WIRE_VERTEX>&                 GetWireVertices() const { return m_wireVertices; }
    const std::vector<std::vector<WIRE_VERTEX>>&    GetWirePolylines() const { return m_wirePolylines; }
    const std::vector<std::vector<WIRE_VERTEX>>&    GetBusPolylines() const { return m_busPolylines; }
    const std::vector<int>&                         GetWirePolylineSheets() const { return m_wirePolylineSheets; }
    const std::vector<int>&                         GetBusPolylineSheets() const { return m_busPolylineSheets; }
    const std::vector<TEXT_ITEM>&                   GetTexts() const { return m_texts; }
    const std::vector<JUNCTION>&                    GetJunctions() const { return m_junctions; }
    const std::vector<NET_LABEL>&                   GetNetLabels() const { return m_netLabels; }
    const std::vector<DECAL>&                       GetDecals() const { return m_decals; }

    /// Number of PADS sheets in the design (>= 1).
    size_t GetSheetCount() const { return m_sheetOffsets.empty() ? 1 : m_sheetOffsets.size(); }

    /// Per-sheet "[N]NAME" names from the pool3 sheet table, in file order.
    const std::vector<std::string>& GetSheetNames() const { return m_sheetNames; }

    /**
     * Build the recovered symbols and wires onto @p aRootSheet's screen.
     *
     * @return the number of objects (symbols + wires) appended.
     */
    int BuildSchematic( SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet ) const;

private:
    void decodeSheets( const std::vector<uint8_t>& aData );

    /// Map a record's file offset to its owning sheet (0-based) via the per-sheet
    /// block boundaries.
    int sheetIndexForOffset( size_t aOffset ) const;

    /// Decode the part-type pool and the per-part-type component attribute pool.
    void decodeFields( const std::vector<uint8_t>& aData );

    /// Decode component fields via the u32 offset-index table (present in
    /// compaction-saved files; exact).  Returns true if a genuine index was used.
    bool decodeFieldsViaIndex( const std::vector<uint8_t>& aData, size_t aPoolBase );

    /// Decode the CAE-decal geometry library + pin terminals, and locate the
    /// per-sheet used-decal name tables (for the placement->decal binding).
    void decodeDecals( const std::vector<uint8_t>& aData );

    /// The used-decal name table that owns @p aOffset (greatest table at/below it).
    const std::vector<std::string>* usedDecalTableForOffset( size_t aOffset ) const;

    /// The per-sheet part-type pool that owns @p aOffset (greatest pool at/below it).
    const std::vector<std::string>* partTypePoolForOffset( size_t aOffset ) const;

    void decodePlacements( const std::vector<uint8_t>& aData );

    /// Decode the per-sheet 24-byte field-placement array following a placement run and
    /// attach each record to its placement's @ref PLACEMENT::fieldPlaces.
    void assignFieldPlacements( const std::vector<uint8_t>& aData, size_t aBlockEnd,
                                size_t aRunFirst, const std::vector<size_t>& aRunOffsets );
    void decodeWires( const std::vector<uint8_t>& aData );
    void decodeTexts( const std::vector<uint8_t>& aData );
    void decodeJunctions( const std::vector<uint8_t>& aData );
    void decodeNetLabels( const std::vector<uint8_t>& aData );

    /// Emit every element belonging to @p aSheetIndex (or all sheets when it is
    /// negative) onto @p aScreen, returning the count appended.
    int appendSheetContent( SCH_SCREEN* aScreen, const SCH_SHEET_PATH& aPath, int aSheetIndex,
                            int aPageHeightIU ) const;

    /// Start file offset of each per-sheet object block, from the pool3 sheet table
    /// (empty for a single-sheet design).
    std::vector<size_t>                  m_sheetOffsets;

    /// Per-sheet display name ("[N]NAME") from the sheet table, parallel to m_sheetOffsets.
    std::vector<std::string>             m_sheetNames;

    /// CAE-decal geometry library, indexed by name via m_decalIndex.
    std::vector<DECAL>                   m_decals;
    std::map<std::string, size_t>        m_decalIndex;

    /// Per-sheet used-decal name tables: (file offset, ordered decal names).
    std::vector<std::pair<size_t, std::vector<std::string>>> m_usedDecalTables;
    size_t                               m_decalBuiltinCount = 0; ///< pool5.used_count (handle base).

    /// Part-type pool names (block+4 ordinal indexes this) and per-part-type fields.
    std::vector<std::string>             m_partTypeNames;

    // Per-sheet part-type pools (file offset -> stride-0x4c name list). A placement's
    // ptidx indexes the pool of its OWN sheet; m_partTypeNames is the union of all pools.
    std::vector<std::pair<size_t, std::vector<std::string>>> m_partTypePools;

    std::map<std::string, std::vector<std::pair<std::string, std::string>>> m_partTypeFields;

    std::vector<PLACEMENT>               m_placements;
    std::vector<WIRE_VERTEX>             m_wireVertices;   ///< Flat pool, file order.
    std::vector<std::vector<WIRE_VERTEX>> m_wirePolylines; ///< Per-connection polylines.
    std::vector<std::vector<WIRE_VERTEX>> m_busPolylines;  ///< Bus polylines (split-run gaps).
    std::vector<int>                     m_wirePolylineSheets; ///< Sheet index per wire polyline.
    std::vector<int>                     m_busPolylineSheets;  ///< Sheet index per bus polyline.
    std::vector<TEXT_ITEM>               m_texts;          ///< Free-text items, file order.
    std::vector<JUNCTION>                m_junctions;      ///< Tie-dot junctions, all sheets.
    std::vector<NET_LABEL>               m_netLabels;      ///< Off-page / power-port net labels.
};

} // namespace PADS_SCH_BINARY

#endif // PADS_SCH_BINARY_READER_H_

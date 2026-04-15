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

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <wx/string.h>

class SCHEMATIC;
class SCH_SHEET;
class SCH_SCREEN;
class SCH_SHEET_PATH;
class LIB_SYMBOL;

namespace PADS_SCH_BINARY
{

/**
 * The SDB pool directory of a PADS Logic binary `.sch`.
 *
 * A `.sch` is a serialized PADS SDB database, the same container family as the
 * `.pcb`: a header, a fixed directory of controller descriptors, the serialized
 * controller payloads, and a footer GUID.  The directory is 20 descriptors of 28
 * bytes each at file offset 0x20 (so the payload stream begins at 0x250, exactly
 * past the table).  Each descriptor's `used_count` (+8) is the authoritative
 * object count for that controller and `used_bytes` (+12) its serialized extent,
 * giving `element_stride = used_bytes / used_count`.
 *
 * Reading the directory turns the per-pool object count into a deterministic
 * value rather than something a payload scan has to rediscover from a heuristic
 * terminator.  Pool roles are stable across the v0x000D corpus; only the two the
 * decode consumes directly are named here.
 */
struct POOL_DIRECTORY
{
    static constexpr size_t   POOL_COUNT = 20;
    static constexpr size_t   TABLE_OFFSET = 0x20;
    static constexpr size_t   DESCRIPTOR_SIZE = 28;
    static constexpr unsigned USED_COUNT_OFFSET = 8;
    static constexpr unsigned USED_BYTES_OFFSET = 12;

    static constexpr int SHEETS = 3;            ///< pool3.used_count = sheet count.
    static constexpr int DECAL_HANDLE_BASE = 5; ///< pool5.used_count = builtin decal handle base.
    static constexpr int NETS = 8;              ///< pool8.used_count = net-table row count (stride 88).

    /// Read the descriptor table from @p aData.  Leaves @ref valid false (and all
    /// counts zero) when the buffer is too small to hold the table.
    void Parse( const std::vector<uint8_t>& aData );

    /// Authoritative object count of controller @p aPool, or 0 when out of range.
    uint32_t Count( int aPool ) const;

    std::array<uint32_t, POOL_COUNT> usedCount{};
    std::array<uint32_t, POOL_COUNT> usedBytes{};
    bool                             valid = false;
};

/// One placed schematic symbol recovered from the binary .sch part array.
/// Placement of one component text field (refdes/value/user) relative to the symbol origin.
struct FIELD_PLACEMENT
{
    int  dx_mils = 0;          ///< Page-relative X offset from the symbol origin.
    int  dy_mils = 0;          ///< Page-relative Y offset (PADS Y-up).
    int  orientation_deg = 0;  ///< 0/90/180/270.
    int  height_mils = 0;      ///< Field text height (0 = use the KiCad default).
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

    /// Multi-gate grouping: base reference (suffix stripped) shared by a part's gates, the
    /// 1-based KiCad unit, and whether this placement is one gate of a multi-unit part.
    std::string baseRef;
    int         unit = 1;
    bool        multiUnit = false;
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

/// One part-type pin: its number, optional name, and electrical-type letter.
struct PIN_INFO
{
    std::string number;       ///< Pin number/designator (e.g. "1", "A8").
    std::string name;         ///< Pin name; empty when the name is the number.
    char        type = 'U';   ///< PADS electrical-type letter (U S L B T C P G Z).
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
    const std::map<std::string, std::vector<PIN_INFO>>& GetPartTypePins() const { return m_partTypePins; }
    const std::map<std::string, std::vector<std::vector<PIN_INFO>>>& GetPartTypeGatePins() const { return m_partTypeGatePins; }

    /// Number of PADS sheets in the design (>= 1).
    size_t GetSheetCount() const { return m_sheetOffsets.empty() ? 1 : m_sheetOffsets.size(); }

    /// Decoded WDITBSIZE page extent in mils (shared by every sheet).
    int GetPageWidthMils() const { return m_pageWidthMils; }
    int GetPageHeightMils() const { return m_pageHeightMils; }

    /// Per-sheet "[N]NAME" names from the pool3 sheet table, in file order.
    const std::vector<std::string>& GetSheetNames() const { return m_sheetNames; }

    /// The net-table row count discovered by the decodeNetLabels scan (the longest
    /// stride-88 run). Exposed so a corpus test can prove it equals pool8.used_count
    /// before the heuristic terminator is retired in favour of the authoritative count.
    size_t GetNetTableScanCount() const { return m_netTableScanCount; }

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

    /// Upper bound for the object scans: the end of the schematic SDB payload (the
    /// last sheet block's end, from the p3 sheet table) so the embedded OLE preview
    /// blobs that follow are never scanned.  Falls back to the buffer size when the
    /// sheet table was not located.
    size_t streamLimit( const std::vector<uint8_t>& aData ) const;

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

    /// Decode every part-type's pin numbers, names and electrical types from the per-sheet
    /// stride-24 pin pool sliced by the part-type pool's +0x30 cursor, into m_partTypePins.
    void decodePinNames( const std::vector<uint8_t>& aData );

    /// Build a multi-unit LIB_SYMBOL for a part whose gates share @p aBase: one unit per
    /// gate, each carrying that gate's body (its placed decal) and pin slice. Returns null
    /// when the part-type has fewer than two gates.
    std::unique_ptr<LIB_SYMBOL> buildMultiUnitLib( const std::string& aBase,
                                                   const std::string& aPartType ) const;
    void decodeWires( const std::vector<uint8_t>& aData );
    void decodeTexts( const std::vector<uint8_t>& aData );
    void decodeJunctions( const std::vector<uint8_t>& aData );
    void decodeNetLabels( const std::vector<uint8_t>& aData );

    /// Emit every element belonging to @p aSheetIndex (or all sheets when it is
    /// negative) onto @p aScreen, returning the count appended.
    int appendSheetContent( SCH_SCREEN* aScreen, const SCH_SHEET_PATH& aPath, int aSheetIndex,
                            int aPageHeightIU ) const;

    /// The SDB pool directory, parsed once at the top of Parse().  Supplies the
    /// authoritative per-controller object counts the decoders rely on.
    POOL_DIRECTORY                       m_pools;

    /// Start file offset of each per-sheet object block, from the pool3 sheet table
    /// (empty for a single-sheet design).
    std::vector<size_t>                  m_sheetOffsets;

    /// End of the schematic SDB payload = the highest sheet-block end (start+length)
    /// from the p3 sheet table; 0 when the table was not located.  Everything past it
    /// is the embedded OLE preview region, which the object scans must not read.
    size_t                               m_streamEnd = 0;

    /// Per-sheet display name ("[N]NAME") from the sheet table, parallel to m_sheetOffsets.
    std::vector<std::string>             m_sheetNames;

    /// Decoded WDITBSIZE page extent in mils (B = 17000 x 11000 when unspecified).
    int                                  m_pageWidthMils = 17000;
    int                                  m_pageHeightMils = 11000;

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

    // Part-type name -> its ordered pins (number, name, electrical type). Decoded from the
    // per-sheet stride-24 pin pool; used to label symbol pins instead of bare numbers.
    std::map<std::string, std::vector<PIN_INFO>> m_partTypePins;

    // Part-type name -> its pins split per gate (the stride-12 gate pool boundaries). Drives
    // multi-unit symbol reconstruction: gate g's pins go on LIB_SYMBOL unit g+1.
    std::map<std::string, std::vector<std::vector<PIN_INFO>>> m_partTypeGatePins;

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
    size_t                               m_netTableScanCount = 0; ///< Validated stride-88 net run length (== pool8.used_count).
};

} // namespace PADS_SCH_BINARY

#endif // PADS_SCH_BINARY_READER_H_

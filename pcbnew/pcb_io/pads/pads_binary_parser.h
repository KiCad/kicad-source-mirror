/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>
#include <cstdint>

#include <math/vector2d.h>
#include <wx/string.h>

#include <io/pads/pads_binary_utils.h>

#include "pads_parser.h"
#include "pads_sdb.h"

namespace PADS_IO
{


enum class SECTION : int
{
    BoardSetup     = 1,    // view-state / origin header
    PadStacks      = 4,    // padstack definitions (64 B/rec)
    PadShapes      = 5,    // per-padstack pad-shape layer table
    FreeText       = 8,    // free-text / label table + string pool
    StringPool     = 9,    // string-pool tail + DRW array head
    DrwItems       = 10,   // LINES/DRW drawing-object array (112 B)
    GraphicPieces  = 11,   // graphic-piece header + inline outline coords (20 B)
    Vertices       = 12,   // graphic-piece vertex pool (12 B)
    DecalLibrary   = 13,   // per-decal block table + trailing library padstack-index table
    DecalHeader    = 14,   // PARTDECAL definition table (112 B)
    TerminalPool   = 15,   // decal terminal position table (36 B)
    ParttypeDefs   = 17,   // parttype/footprint defs (224 B)
    PartPins       = 19,   // PARTTYPE pin-definition table (88 B)
    Placements     = 22,   // part placements (112 B)
    Nets           = 23,   // net records (424 B)
    ClearanceRules = 49,   // clearance/design-rule heap
    PourTokensA    = 52,   // pour-relation token streams
    PourTokensB    = 53,
    PourTokensC    = 54,
    Vias           = 60,   // route-junction / via records (64 B)
};

/**
 * A PADS net class recovered from the binary design-rule graph.
 *
 * Membership is deterministic: every section-23 net record carries its net-class
 * object pointer at +188, identical for all members of a class. Class names and
 * per-class clearance-rule layers come from the trailing-arena type-66 rule table
 * (24 B records, tag 0x42 @ +4, net-class pointer @ +8, layer @ +20).
 */
struct NETCLASS_DEF
{
    std::string              name;        // net-class name (from the 0x118 name table)
    std::vector<std::string> nets;        // member net names (sec23 +188 grouping)
    std::vector<int>         ruleLayers;  // layers carrying a clearance rule (0 = all layers)

    // Per-class rule VALUES from the layer-0 (discriminator 1) clearance record, positionally
    // joined to the type-66 clearance edge by declaration order (see parseNetClasses). All in
    // BASIC units; hasRuleValues is false when the value/edge counts disagree (correct-or-silent).
    int  clearance     = 0;   // ASC TRACK_TO_TRACK (core[0])
    int  trackWidth    = 0;   // ASC REC_TRACK_WIDTH (core[34])
    int  minTrackWidth = 0;   // ASC MIN_TRACK_WIDTH (core[33])
    int  maxTrackWidth = 0;   // ASC MAX_TRACK_WIDTH (core[35])
    int  viaClearance  = 0;   // ASC SAME_NET_VIA_TO_VIA (core[2])
    bool hasRuleValues = false;
};

/**
 * A PADS *CLUSTER* (group/union) recovered from the binary part-cluster table.
 *
 * This is the part-keyed cluster of the .asc *CLUSTER* section: a named set of
 * PARTS. It is deliberately distinct from the net-keyed CLUSTER in pads_parser.h
 * (the ASCII net-cluster struct), which carries net/segment members this binary
 * table does not, so a separate type avoids a redefinition clash.
 */
struct PART_CLUSTER
{
    std::string name;     // cluster name (.asc *CLUSTER* NAME column)
    int         id = 0;   // 1-based ordinal; equals the sec22 +108 CLSTID reference
};

/**
 * Reader for the PADS PowerPCB binary `.pcb` format.
 *
 * The file is a serialized snapshot of PADS' in-memory SDB (System DataBase). This
 * class is organized around that model: a PADS_SDB (see pads_sdb.h) owns the bytes and
 * decodes the file container (header, the section directory of per-controller record
 * streams, and the coordinate origin), and the per-section readers below sit on top of
 * it. A reader names its section by role (the SECTION enum), walks the section's records
 * through an SDB_RECORD reader (named field offsets, design-coordinate helpers), and
 * populates the same intermediate structs as the ASCII PARSER (PART, NET, ROUTE, ...) so
 * the struct-to-KiCad conversion is shared between both importers.
 *
 * Supported versions: 0x2021, 0x2022, 0x2024, 0x2025, 0x2026, 0x2027.
 */
class BINARY_PARSER
{
public:
    BINARY_PARSER();
    ~BINARY_PARSER();

    // m_cursor holds a reference to m_data, so a copy would bind the clone's reads to
    // the source buffer. The parser is never copied; forbid it rather than leave the
    // latent dangling-reference trap.
    BINARY_PARSER( const BINARY_PARSER& ) = delete;
    BINARY_PARSER& operator=( const BINARY_PARSER& ) = delete;

    void Parse( const wxString& aFileName );

    /**
     * Check if a file appears to be a PADS binary PCB file.
     * Checks the 2-byte magic (0x00FF) and version field.
     */
    static bool IsBinaryPadsFile( const wxString& aFileName );

    const PARAMETERS& GetParameters() const { return m_parameters; }
    const std::vector<PART>& GetParts() const { return m_parts; }
    const std::vector<NET>& GetNets() const { return m_nets; }
    const std::vector<NETCLASS_DEF>& GetNetClasses() const { return m_netClasses; }
    const std::vector<DIFF_PAIR_DEF>& GetDiffPairs() const { return m_diffPairs; }
    const std::vector<ROUTE>& GetRoutes() const { return m_routes; }
    const std::vector<TEXT>& GetTexts() const { return m_texts; }
    const std::vector<POUR>& GetPours() const { return m_pours; }
    const std::vector<KEEPOUT>& GetKeepouts() const { return m_keepouts; }
    const std::vector<COPPER_SHAPE>& GetCopperShapes() const { return m_copper_shapes; }
    const std::vector<DIMENSION>& GetDimensions() const { return m_dimensions; }
    const std::vector<POLYLINE>& GetBoardOutlines() const { return m_boardOutlines; }
    const std::map<std::string, PART_DECAL>& GetPartDecals() const { return m_decals; }

    // Part clusters (.asc *CLUSTER* groups) and the per-part membership map. The
    // membership key is an index into GetParts(); the value is the 1-based CLSTID,
    // which equals the cluster's GetClusters() ordinal.
    const std::vector<PART_CLUSTER>& GetClusters() const { return m_clusters; }
    const std::map<size_t, int>& GetPartClusterIds() const { return m_partClusterId; }

    int GetLayerCount() const { return m_parameters.layer_count; }
    bool IsBasicUnits() const { return true; }

    double GetDefaultViaSize() const { return m_defaultViaSize; }
    double GetDefaultViaDrill() const { return m_defaultViaDrill; }

    std::vector<LAYER_INFO> GetLayerInfos() const;

    // Test-only access to the structural shape -> sec12 link. A direct probe asserts the
    // structural decode and catches a sec12 base or owner-lag regression that a board-level
    // zone count would not surface.
    int32_t GetSec12BaseForTest() const { return m_sec12Base; }

    // Test-only access to the distinct section-4 pad-shape names the parser decoded.
    // Asserts the shape-code enum directly (e.g. that a board's code-0 oblong padstacks
    // surface as "OF"), independent of the placement->decal->padstack assignment.
    std::set<std::string> GetPadStackShapesForTest() const
    {
        std::set<std::string> shapes;

        for( const auto& [idx, layers] : m_padStackCache )
        {
            for( const PAD_STACK_LAYER& psl : layers )
                shapes.insert( psl.shape );
        }

        return shapes;
    }

    bool GetOwnerLoopForTest( const std::string& aName, std::vector<VECTOR2I>& aOut ) const
    {
        return fetchOwnerLoop( aName, 200, aOut );
    }

private:
    static constexpr int32_t  ANGLE_SCALE = 1800000;

    // Version helpers
    bool isOldFormat() const { return m_sdb.IsOldFormat(); }

    // The v0x2021 direct decal-index pad chain (96 B placements + JMPVIA 100 B decal
    // table + @+56 lag) is verified only on v0x2021. v0x2022 shares the old-format
    // placement layout but its decal chain is unverified, so it is not enabled.
    bool isV2021PadChain() const { return m_version == 0x2021; }

    // Section lookup over the parsed SDB directory. The SECTION overload forwards to the
    // int form so a constant section reads by role; the int form serves the few callers
    // that iterate a computed directory index. Records are read through m_sdb.Record /
    // RecordAt; byte-scan readers index m_sdb.Bytes() at the section's dataOffset.
    const SDB_SECTION* getSection( int aIndex ) const;
    const SDB_SECTION* getSection( SECTION aSection ) const { return getSection( static_cast<int>( aSection ) ); }

    // Section parsers
    void parseBoardSetup();
    void parsePartPlacements();
    void parseSection19Parts();
    void parsePadStacks();
    void parsePartDecals();
    void parseDecalNameTable();
    void parseDecalNameTableOld();
    void parsePartTypeTable();
    void parseBoardOutlineDrwOrigin();
    void parseBoardOutline();
    bool parseArcBoardOutline();
    void parseNetNames();

    // Recover part clusters (.asc *CLUSTER* groups). Locates the 60-byte cluster-table
    // run in the late route-coord section tail (whole-file fallback), validating each
    // record by a printable name at +0 and sane design coords from the +16/+20 RAW
    // XLOC/YLOC. A record's 1-based ordinal is the CLSTID that sec22 +108 references.
    // Membership itself is captured during parsePartPlacements into m_partClusterId.
    void parseClusters();

    // Recover net classes deterministically from the trailing design-rule arena:
    // group nets by their section-23 +188 net-class pointer (membership), name each
    // class from the 0x118 name table, and read per-class clearance-rule layers from
    // the type-66 rule table (tag 0x42 @ +4, class pointer @ +8, layer @ +20).
    void parseNetClasses();

    // Recover serialized differential pairs from the sec49 (ClearanceRules) MFC heap.
    // Each override pair is an 864-byte DIF_PAIR object located by its trailing 0xFF
    // allocator free-fill run (object_start = ff_run_start - 864). Member nets are the
    // self-pointers at +12/+16, value-joined to a net name via m_netSelfPtrToName (the
    // sec23 +184 self-pointer). GAP = f64@+56 if != -1.0 else f64@+40; WIDTH = i32@+600
    // if != -1 else i32@+592. Inherit-default pairs are not serialized, so coverage is
    // limited to override pairs (the same limit as the clearance matrices). v0x2027 only.
    void parseDiffPairs();
    void parseMetadataRegion();
    void parseDftConfig( size_t aStart, size_t aEnd );
    void parseRouteVertices();
    void parseTextRecords();
    void parseTerminals();
    void parseTerminalsOld();

    // Synthesize a placeholder terminal layout for every decal that has no terminals yet
    // but carries a recorded count in m_decalTerminalCount. Positions are not separately
    // indexed for these, so the layout is correct in count only.
    void synthesizePlaceholderTerminals();

    // Assign the default pad stack (global cache index 0) to every decal that has terminals.
    void assignDefaultPadStacks();

    // Recover the per-pin pad-stack assignment from the section-15 tail (pin, ref) pair pool.
    // Descriptor decals are sliced by the section-14 descriptor table; the de-duplicated
    // library decals are sliced by the section-13 0x4D00 table. Each ref indexes m_padStackPool.
    void parsePerPinPadstacks();

    // Apply one decal's (pin, ref) pair slice: pin 0 sets the decal default, pin>0 overrides
    // that terminal. ref indexes m_padStackPool. Shared by the descriptor and library passes.
    void applyPadstackPairs( PART_DECAL& aDecal,
                             const std::vector<std::pair<int32_t, int32_t>>& aPairs,
                             int32_t aStart, int32_t aCount );
    void parseKeepouts();
    void parseCopperShapes();
    void parseCopperPours();

    // Reconstruct PADS dimensions, which the binary does not store as a dedicated section.
    // Each dimension is a DRW graphic-piece owner named DIM* (sec10) whose sec12 vertex run
    // holds the BASPNT/ARWLN/ARWHD/EXTLN sub-pieces in ASC order, plus a sec8 value-text
    // record (idx2 string, anchor, height, width, rotation, layer) bound to the owner by
    // anchor-in-bbox. Geometry comes from the owner run via sec12Vertex; the value text uses
    // the same +1 metadata-lag layout as parseTextRecords. Consumes the owner runs and the
    // sec8 text stream, so it must run after buildOwnerRuns and parseTextRecords.
    void parseDimensions();

    // Decode the sec69 layer-definition + physical-stackup table (31 records of 152 bytes).
    // The table is located by the inline string "(All layers)" rather than the directory
    // data_offset, which is stale/overflowed on large boards. Populates m_layerInfos with
    // per-layer thickness, copper thickness and dielectric constant for stackup import.
    // Verified on v0x2027 only; older dialects keep the synthesized fallback.
    void parseLayerStackup();

    void linkPartsToDecals();

    // Structural shape -> sec12 vertex link.
    //
    // The owner stream is a marker-walk over [sec8.dataOffset, sec10 end): 112-byte
    // records accepted on u16@+0 in {0xFFFE,0xFFFF}, u16@+2 == 0, a >=2-char ASCII name
    // at +44 and an in-range cursor triple. It is phase-shift-aware (a record can start
    // mid-sec8/sec9), so it is NOT the sec10 fixed grid. Owner R[i]'s run cursors live in
    // the FOLLOWING record R[i+1] (a one-record lag); the last owner's cursors are carried
    // by the next physical record (the trailing terminator slot).
    struct OWNER_RUN
    {
        int32_t pieceStart  = 0;   // R[i+1] @ +8
        int32_t vertexStart = 0;   // R[i+1] @ +12, cumulative sec12 corner cursor
        int32_t arcStart    = 0;   // R[i+1] @ +16
        int32_t pieceCount  = 0;   // R[i+1] @ +24
    };

    // Owner DRW name -> lagged run. Built once by buildOwnerRuns(), consumed by the
    // copper/keepout vertex fetch. Keyed by the +44 name shared with the classified
    // owners read off the sec10 grid.
    std::map<std::string, OWNER_RUN> m_ownerRuns;

    // Logical-to-physical sec12 base, derived from sec12 framing only:
    //   base = sec12_directory_rows - clean_sec12_rows
    // where clean rows are the contiguous valid 12-byte prefix before the per-save heap
    // tail. A vertex's physical sec12 row is vertexStart - base.
    int32_t m_sec12Base      = 0;
    int32_t m_sec12CleanRows = 0;

    void buildOwnerRuns();
    void computeSec12Base();

    // Read a single sec12 vertex (x, y, attr). Returns false out of range.
    bool sec12Vertex( int32_t aRow, int32_t& aX, int32_t& aY, int32_t& aAttr ) const;

    // Fetch the structurally-anchored closed polygon for an owner starting at its
    // vertexStart cursor. Consumes vertices until the run returns to the start point.
    // Returns the design-coordinate vertices (excluding the duplicate close point) and
    // true on a clean closure; false when no run exists, the cursor is out of the clean
    // prefix, or the run does not close within aMaxVerts.
    bool fetchOwnerLoop( const std::string& aName, size_t aMaxVerts,
                         std::vector<VECTOR2I>& aOut ) const;

    // DFT format parsers
    std::map<std::string, std::string> parseDftDotPadded( size_t aPos, size_t aEnd ) const;
    std::map<std::string, std::string> parseDftNullSeparated( size_t aPos, size_t aEnd ) const;

    bool isValidNetName( const std::string& aName ) const;

    // Coordinate conversion from binary absolute to PADS_IO design-relative units
    double toBasicCoordX( int32_t aRawValue ) const;
    double toBasicCoordY( int32_t aRawValue ) const;
    double toBasicAngle( int32_t aRawAngle ) const;

    // The parsed file container: header, section directory, and coordinate origin.
    // Owns the file bytes; the section readers below it work through its directory.
    PADS_SDB             m_sdb;

    // Read substrate shared by the section readers. m_data aliases the SDB's bytes so
    // the absolute-offset readers keep their form; m_cursor follows it. Both are
    // declared after m_sdb so their references bind to its live buffer.
    const std::vector<uint8_t>& m_data = m_sdb.Bytes();
    BINARY_CURSOR               m_cursor{ m_data };

    uint16_t             m_version = 0;

    // Coordinate origin from DFT_CONFIGURATION (may be overwritten by parseDftConfig)
    int32_t m_originX = 0;
    int32_t m_originY = 0;
    bool    m_originFound = false;

    // Board outline DRW absolute origin from section 9 LINE item records.
    // Section 11 board outline vertices are DRW-relative and need this
    // offset to convert to binary absolute coordinates.
    int32_t m_boardDrwOriginX = 0;
    int32_t m_boardDrwOriginY = 0;
    bool    m_boardDrwOriginFound = false;

    // Default via dimensions extracted from section 4 pad stacks
    double m_defaultViaSize  = 0.0;
    double m_defaultViaDrill = 0.0;

    // Pad stack cache indexed by section 4 record number
    std::map<int, std::vector<PAD_STACK_LAYER>> m_padStackCache;

    // Extended section-4 pad-stack pool, 0-based from the true pool start (sec4.dataOffset
    // minus the de-duplicated library head the section directory does not index). The
    // per-pin (pin, ref) pairs in the section-15 tail index this pool directly.
    std::vector<std::vector<PAD_STACK_LAYER>> m_padStackPool;

    // Part index -> parttype index from the NEXT section 22 record (@+4 with +1
    // block lag). Indexes into m_partTypeDecalIndex (the parttype-definition table).
    std::map<size_t, uint32_t> m_partTypeIndex;

    // Part index -> direct decal index for the v0x2021 dialect. v0x2021 carries no
    // 224 B parttype-definition layer; the placement's decal is selected straight from
    // the NEXT 96 B placement record's @+56 field (the same +1 block-interleave lag).
    // Indexes directly into m_decalNameTable.
    std::map<size_t, uint32_t> m_partDecalIndex;

    // Parttype-definition table (sec17.dataOffset - 1232, 224 B records). Each
    // parttype carries a decal_index at payload +96 that indexes m_decalNameTable.
    std::vector<int32_t> m_partTypeDecalIndex;

    // Complete decal-name table (sec14.dataOffset - 1188, 112 B records, NAME @ +0).
    // Indexed directly by a parttype's decal_index. Includes vias, connectors and
    // mounting holes that section 10 lacks. table[0] is always JMPVIA_AAAAA.
    std::vector<std::string> m_decalNameTable;

    // Decal name -> terminal count, read from the +72 in-record field of the
    // decal-name header table (112 B records, NAME @ +0, 0xFFFE @ +64, count @ +72).
    // Covers passives/connectors that lack a sec14 descriptor.
    std::map<std::string, uint32_t> m_decalTerminalCount;

    // Decal name -> start cursor (i32 @ +68 of the same header record) into the unified
    // terminal stream S = POOL33 ++ SEC15 (the 33-record sec14 de-dup pool followed by the
    // section-15 geometry pool). A decal's terminals are S[start .. start+count). This is
    // the de-dup-aware per-decal terminal index; it cannot be derived from counts because
    // the pool de-duplicates geometrically identical decals onto shared windows.
    std::map<std::string, int32_t> m_decalTerminalStart;

    // Decal name -> pad-stack count (i32 @ +88 of the same -1188 header record): the number
    // of distinct pad stacks the decal uses, i.e. the length of its (pin, ref) pair slice.
    // Drives the library-decal pass in parsePerPinPadstacks.
    std::map<std::string, int32_t> m_decalStackCount;

    // Section 23 array index -> net name, used to attribute structural vias to nets
    std::map<uint32_t, std::string> m_sec23IndexToNet;

    // Net name -> its net-class object pointer (section-23 record +188). Zero/absent
    // for unclassed nets. The membership key: nets sharing a value are one class.
    std::map<std::string, uint32_t> m_netClassOwner;

    // Recovered net classes (membership + per-class clearance-rule layers).
    std::vector<NETCLASS_DEF> m_netClasses;

    // Net self-pointer (section-23 record +184) -> net name. The within-file JOIN key
    // that resolves a sec49 DIF_PAIR object's +12/+16 member-net pointers to names.
    std::map<uint32_t, std::string> m_netSelfPtrToName;

    // Recovered differential pairs (sec49 serialized override objects). Reuses the ASCII
    // DIFF_PAIR_DEF shape so the importer's diff-pair consumers are shared verbatim.
    std::vector<DIFF_PAIR_DEF> m_diffPairs;

    // Output data (same structs as ASCII parser)
    PARAMETERS                          m_parameters;
    std::vector<PART>                   m_parts;
    std::vector<NET>                    m_nets;
    std::vector<ROUTE>                  m_routes;
    std::vector<TEXT>                   m_texts;
    std::vector<POUR>                   m_pours;
    std::vector<KEEPOUT>                m_keepouts;
    std::vector<COPPER_SHAPE>           m_copper_shapes;
    std::vector<DIMENSION>              m_dimensions;
    std::vector<POLYLINE>               m_boardOutlines;
    std::map<std::string, PART_DECAL>   m_decals;

    // Part clusters (.asc *CLUSTER* groups), in table order. Index+1 is the CLSTID.
    std::vector<PART_CLUSTER>          m_clusters;

    // Part index (into m_parts) -> 1-based CLSTID from the sec22 placement record's
    // +108 field. Recorded for the new 112-byte layout only; -1/absent = unclustered.
    std::map<size_t, int>              m_partClusterId;

    // sec69 physical stackup, decoded by parseLayerStackup() on v0x2027. Empty when the
    // table could not be located, in which case GetLayerInfos() synthesizes a fallback.
    std::vector<LAYER_INFO>             m_layerInfos;
};

} // namespace PADS_IO

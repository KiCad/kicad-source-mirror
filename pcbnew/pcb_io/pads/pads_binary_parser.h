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
#include <optional>

#include <math/vector2d.h>
#include <wx/string.h>

#include <io/pads/pads_binary_utils.h>

#include "pads_parser.h"
#include "pads_sdb.h"

namespace PADS_IO
{

/**
 * Named section indices into the PADS binary section directory.
 *
 * Each enumerator's value IS the directory index, so a section constant reads as its role
 * rather than a bare integer.
 */
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
    Clusters       = 68,   // part-cluster (*CLUSTER*) controller (60 B/rec)
    LayerTable     = 69,   // ODBLayer physical-stackup table (152 B/rec)
};

/**
 * A PADS net class recovered from the binary design-rule graph.
 *
 * Every net record carries its net-class object pointer at +188, identical for all members
 * of a class. Class names and per-class clearance-rule layers come from the type-66 rule
 * table (24 B records, tag 0x42 @ +4, net-class pointer @ +8, layer @ +20).
 */
struct BIN_NET_CLASS_DEF
{
    std::string              name;
    std::vector<std::string> nets;
    std::vector<int>         ruleLayers;  // layers carrying a clearance rule (0 = all layers)

    // Per-class rule values, positionally joined to the type-66 clearance edge by declaration
    // order. All in BASIC units. hasRuleValues is false when the value and edge counts disagree.
    int  clearance     = 0;
    int  trackWidth    = 0;
    int  minTrackWidth = 0;
    int  maxTrackWidth = 0;
    int  viaClearance  = 0;
    bool hasRuleValues = false;
};

/// One copper-pour POR header: its byte offset, name and vertex count.
struct POUR_HEADER
{
    size_t      offset = 0;
    std::string name;
    uint32_t    vtxCount = 0;
};

/// One board-outline vertex triplet: [i32 X, i32 Y, i32 attr] where attr -1 is a plain corner
/// and attr >= 0 is the arc-parameter ordinal.
struct ARC_VERTEX
{
    int32_t x = 0;
    int32_t y = 0;
    int32_t attr = 0;
};

/// One DRW drawing-item bbox, made origin-relative, used to match a board-outline vertex run.
struct ARC_DRAWING_ITEM
{
    int32_t originX = 0;
    int32_t originY = 0;
    int64_t localMinX = 0;
    int64_t localMinY = 0;
    int64_t localMaxX = 0;
    int64_t localMaxY = 0;
    int64_t span = 0;
    bool    preferred = false;
};

/// One type-66 net-class rule edge: its owner pointer, rule-detail page, full rule pointer
/// (declaration order within a page), layer and file offset.
struct NET_CLASS_RULE_EDGE
{
    uint32_t owner = 0;
    uint32_t page = 0;
    uint32_t rulePtr = 0;
    int      layer = 0;
    size_t   off = 0;
};

/**
 * A PADS part cluster (named group of parts).
 *
 * Deliberately distinct from the net-keyed CLUSTER in pads_parser.h, which carries
 * net/segment members this part-keyed table does not, so a separate type avoids a clash.
 */
struct PART_CLUSTER
{
    std::string name;
    int         id = 0;   // 1-based ordinal; equals the sec22 +108 CLSTID reference
};

/**
 * Reader for the PADS PowerPCB binary `.pcb` format.
 *
 * The file is a serialized snapshot of PADS' in-memory SDB (System DataBase). A PADS_SDB owns
 * the bytes and decodes the file container (header, section directory, coordinate origin), and
 * the per-section readers below sit on top of it. A reader names its section by role (the
 * SECTION enum), walks the records through an SDB_RECORD reader, and populates the same
 * intermediate structs as the ASCII PARSER so the struct-to-KiCad conversion is shared.
 *
 * Supported versions: 0x2021, 0x2022, 0x2024, 0x2025, 0x2026, 0x2027.
 */
class BINARY_PARSER
{
public:
    BINARY_PARSER();
    ~BINARY_PARSER();

    // m_cursor holds a reference to m_data, so a copy would bind the clone's reads to the
    // source buffer. Forbid copying rather than leave a dangling-reference trap.
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
    const std::vector<BIN_NET_CLASS_DEF>& GetNetClasses() const { return m_netClasses; }
    const std::vector<DIFF_PAIR_DEF>& GetDiffPairs() const { return m_diffPairs; }
    const std::vector<ROUTE>& GetRoutes() const { return m_routes; }
    const std::vector<TEXT>& GetTexts() const { return m_texts; }
    const std::vector<POUR>& GetPours() const { return m_pours; }
    const std::vector<KEEPOUT>& GetKeepouts() const { return m_keepouts; }
    const std::vector<COPPER_SHAPE>& GetCopperShapes() const { return m_copper_shapes; }
    const std::vector<DIMENSION>& GetDimensions() const { return m_dimensions; }
    const std::vector<POLYLINE>& GetBoardOutlines() const { return m_boardOutlines; }
    const std::map<std::string, PART_DECAL>& GetPartDecals() const { return m_decals; }

    // Part clusters and the per-part membership map. The membership key is an index into
    // GetParts(); the value is the 1-based CLSTID, which equals the GetClusters() ordinal.
    const std::vector<PART_CLUSTER>& GetClusters() const { return m_clusters; }
    const std::map<size_t, int>& GetPartClusterIds() const { return m_partClusterId; }

    int GetLayerCount() const { return m_parameters.layer_count; }
    bool IsBasicUnits() const { return true; }

    double GetDefaultViaSize() const { return m_defaultViaSize; }
    double GetDefaultViaDrill() const { return m_defaultViaDrill; }

    std::vector<LAYER_INFO> GetLayerInfos() const;

    int32_t GetSec12BaseForTest() const { return m_sec12Base; }

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

    bool isOldFormat() const { return m_sdb.IsOldFormat(); }

    // Both old dialects resolve a placement's decal via a direct index in the next physical
    // record rather than through a parttype-definition table (v0x2022 does have such a table,
    // but placements don't reference it); only the field's offset within that record differs.
    bool usesDirectDecalChain() const { return m_version == 0x2021 || m_version == 0x2022; }

    // The SECTION overload forwards to the int form so a constant section reads by role; the
    // int form serves callers that iterate a computed directory index.
    const SDB_SECTION* getSection( int aIndex ) const;
    const SDB_SECTION* getSection( SECTION aSection ) const { return getSection( static_cast<int>( aSection ) ); }

    void parseBoardSetup();
    void parsePartPlacements();
    void recoverOmittedPlacements();

    // Old-format (v0x2017-2022) omitted-placement recovery via a bounded 0xFEFF section scan;
    // the scored new-format locator does not resolve the v2021 decal-index lag.
    void recoverOmittedPlacementsOld();

    // v0x2022 placements are relative to a different section-1 origin field than every other
    // geometry type in the file (aAltOriginXOff/YOff, when set); returns the (x, y) shift to
    // add to a raw placement coordinate so the shared origin subtraction downstream still lands
    // on the correct design coordinate. Zero when either offset is unset (every other version).
    std::pair<int32_t, int32_t> placementOriginAdjust( std::optional<int> aAltOriginXOff,
                                                        std::optional<int> aAltOriginYOff ) const;

    // Build a PART from a placement record's refdes and its coordinate, rotation and side fields.
    // aXAdjust/aYAdjust shift the raw coordinate before the shared origin subtraction downstream;
    // only v0x2022 uses a nonzero value, to compensate for its placement-specific origin field.
    PART makePlacementPart( const SDB_RECORD& aRec, int aXOff, std::optional<int> aYOff,
                            int aAngleOff, int aNameOff, const std::string& aRefDes,
                            int32_t aXAdjust = 0, int32_t aYAdjust = 0 ) const;
    void parsePadStacks();
    void parsePartDecals();
    void parseDecalNameTable();
    void parseDecalNameTableOld();
    void parsePartTypeTable();
    void parseBoardOutlineDrwOrigin();
    void parseBoardOutline();
    bool parseArcBoardOutline();
    std::vector<ARC_DRAWING_ITEM> collectArcDrawingItems();
    int64_t findArcParameterTable( const std::vector<ARC_VERTEX>& aVerts,
                                   const std::vector<size_t>& aArcIdx );
    void parseNetNames();
    void parseNetNamesNew();
    void parseNetNamesOld();

    // Recover part clusters. Membership itself is captured during parsePartPlacements into
    // m_partClusterId; a record's 1-based ordinal is the CLSTID that sec22 +108 references.
    void parseClusters();

    // Group nets by their +188 net-class pointer for membership, name each class from the
    // 0x118 name table, and read per-class clearance-rule layers from the type-66 rule table.
    void parseNetClasses();
    std::vector<NET_CLASS_RULE_EDGE> collectNetClassRuleEdges( const std::set<uint32_t>& aOwnerSet );
    void applyNetClassClearances( const std::vector<NET_CLASS_RULE_EDGE>& aEdges,
                                  const std::map<uint32_t, size_t>& aOwnerOrdinal );

    // Recover serialized differential pairs from the sec49 MFC heap. Member nets are the
    // self-pointers at +12/+16, joined to a net name via m_netSelfPtrToName. Inherit-default
    // pairs are not serialized, so coverage is limited to override pairs. v0x2027 only.
    void parseDiffPairs();
    void parseMetadataRegion();
    void parseDftConfig( size_t aStart, size_t aEnd );
    void parseRouteVertices();
    void parseTextRecords();
    void parseTerminals();
    void parseTerminalsOld();

    // Synthesize a placeholder terminal layout for every decal that has no terminals yet but
    // carries a recorded count in m_decalTerminalCount. Positions are not separately indexed
    // for these, so the layout is correct in count only.
    void synthesizePlaceholderTerminals();

    void assignDefaultPadStacks();

    // Recover the per-pin pad-stack assignment from the section-15 tail (pin, ref) pair pool.
    // Descriptor decals are sliced by the section-14 descriptor table; the de-duplicated
    // library decals by the section-13 0x4D00 table. Each ref indexes m_padStackPool.
    void parsePerPinPadstacks();

    // Apply one decal's (pin, ref) pair slice: pin 0 sets the decal default, pin>0 overrides
    // that terminal. Shared by the descriptor and library passes.
    void applyPadstackPairs( PART_DECAL& aDecal,
                             const std::vector<std::pair<int32_t, int32_t>>& aPairs,
                             int32_t aStart, int32_t aCount );
    void parseKeepouts();
    void parseCopperShapes();
    void parseCopperPours();
    void parseCopperPoursSimple( const std::vector<POUR_HEADER>& aHeaders,
                                 const SDB_SECTION& aSec49 );
    void parseCopperPoursComplex();

    // Reconstruct PADS dimensions, which the binary does not store as a dedicated section.
    // Each dimension is a DRW graphic-piece owner named DIM* whose sec12 vertex run holds the
    // leader sub-pieces. Must run after buildOwnerRuns and parseTextRecords, which it consumes.
    void parseDimensions();

    // Decode the sec69 layer-definition + physical-stackup table (31 records of 152 bytes),
    // located by the inline string "(All layers)" because the directory data_offset overflows
    // the indexed region on large boards. Verified on v0x2027 only; older dialects keep the
    // synthesized fallback.
    void parseLayerStackup();

    void linkPartsToDecals();

    // Structural shape -> sec12 vertex link.
    //
    // The owner stream is a phase-shift-aware marker-walk over [sec8.dataOffset, sec10 end):
    // 112-byte records accepted on u16@+0 in {0xFFFE,0xFFFF}, u16@+2 == 0, a >=2-char ASCII
    // name at +44 and an in-range cursor triple, so it is not the sec10 fixed grid. Owner
    // R[i]'s run cursors live in the FOLLOWING record R[i+1] (a one-record lag); the last
    // owner's cursors are carried by the next physical record (the trailing terminator slot).
    struct OWNER_RUN
    {
        int32_t pieceStart  = 0;   // R[i+1] @ +8
        int32_t vertexStart = 0;   // R[i+1] @ +12, cumulative sec12 corner cursor
        int32_t arcStart    = 0;   // R[i+1] @ +16
        int32_t pieceCount  = 0;   // R[i+1] @ +24
    };

    // Owner DRW name -> lagged run, keyed by the +44 name. Built once by buildOwnerRuns().
    std::map<std::string, OWNER_RUN> m_ownerRuns;

    // Logical-to-physical sec12 base = directory_rows - clean_rows, where clean rows are the
    // contiguous valid 12-byte prefix before the per-save heap tail. A vertex's physical sec12
    // row is vertexStart - base.
    int32_t m_sec12Base      = 0;
    int32_t m_sec12CleanRows = 0;

    void buildOwnerRuns();
    void computeSec12Base();

    bool sec12Vertex( int32_t aRow, int32_t& aX, int32_t& aY, int32_t& aAttr ) const;

    // Fetch the closed polygon for an owner starting at its vertexStart cursor, consuming
    // vertices until the run returns to the start point. Returns the design-coordinate vertices
    // (excluding the duplicate close point) and true on a clean closure; false when no run
    // exists, the cursor is out of the clean prefix, or the run does not close within aMaxVerts.
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

    // Owns the file bytes and decodes the container; the section readers work through it.
    PADS_SDB             m_sdb;

    // m_data aliases the SDB's bytes for the absolute-offset readers; m_cursor follows it.
    // Both are declared after m_sdb so their references bind to its live buffer.
    const std::vector<uint8_t>& m_data = m_sdb.Bytes();
    BINARY_CURSOR               m_cursor{ m_data };

    uint16_t             m_version = 0;

    // Coordinate origin from DFT_CONFIGURATION (may be overwritten by parseDftConfig)
    int32_t m_originX = 0;
    int32_t m_originY = 0;
    bool    m_originFound = false;

    // Board outline DRW absolute origin from section 9 LINE item records. Section 11 board
    // outline vertices are DRW-relative and need this offset to reach binary absolute.
    int32_t m_boardDrwOriginX = 0;
    int32_t m_boardDrwOriginY = 0;
    bool    m_boardDrwOriginFound = false;

    // Default via dimensions extracted from section 4 pad stacks
    double m_defaultViaSize  = 0.0;
    double m_defaultViaDrill = 0.0;

    // Pad stack cache indexed by section 4 record number
    std::map<int, std::vector<PAD_STACK_LAYER>> m_padStackCache;

    // Extended section-4 pad-stack pool, 0-based from the true pool start (sec4.dataOffset
    // minus the de-duplicated library head the directory does not index). The section-15 tail
    // (pin, ref) pairs index this pool directly.
    std::vector<std::vector<PAD_STACK_LAYER>> m_padStackPool;

    // Part index -> parttype index from the NEXT section 22 record (@+4 with +1 block lag).
    // Indexes into m_partTypeDecalIndex.
    std::map<size_t, uint32_t> m_partTypeIndex;

    // Part index -> direct decal index for v0x2021, which carries no parttype layer; the decal
    // is selected from the NEXT 96 B placement record's @+56 field. Indexes m_decalNameTable.
    std::map<size_t, uint32_t> m_partDecalIndex;

    // Parttype-definition table (sec17.dataOffset - 1232, 224 B records). Each parttype carries
    // a decal_index at payload +96 that indexes m_decalNameTable.
    std::vector<int32_t> m_partTypeDecalIndex;

    // Complete decal-name table (sec14.dataOffset - 1188, 112 B records, NAME @ +0), indexed by
    // a parttype's decal_index. Includes vias, connectors and mounting holes that section 10
    // lacks. table[0] is always JMPVIA_AAAAA.
    std::vector<std::string> m_decalNameTable;

    // Decal name -> terminal count (+72 of the decal-name header record). Covers
    // passives/connectors that lack a sec14 descriptor.
    std::map<std::string, uint32_t> m_decalTerminalCount;

    // Decal name -> start cursor (i32 @ +68) into the unified terminal stream POOL33 ++ SEC15.
    // A decal's terminals are stream[start .. start+count). Stored rather than derived from
    // counts because the pool de-duplicates geometrically identical decals onto shared windows.
    std::map<std::string, int32_t> m_decalTerminalStart;

    // Decal name -> pad-stack count (i32 @ +88), the length of its (pin, ref) pair slice.
    std::map<std::string, int32_t> m_decalStackCount;

    // Section 23 array index -> net name, used to attribute structural vias to nets.
    std::map<uint32_t, std::string> m_sec23IndexToNet;

    // Net name -> its net-class object pointer (record +188); zero/absent for unclassed nets.
    // Nets sharing a value are one class.
    std::map<std::string, uint32_t> m_netClassOwner;

    std::vector<BIN_NET_CLASS_DEF> m_netClasses;

    // Net self-pointer (record +184) -> net name. The join key that resolves a sec49 DIF_PAIR
    // object's +12/+16 member-net pointers to names.
    std::map<uint32_t, std::string> m_netSelfPtrToName;

    std::vector<DIFF_PAIR_DEF> m_diffPairs;

    // Output data, same structs as the ASCII parser.
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

    // Part clusters in table order; index+1 is the CLSTID.
    std::vector<PART_CLUSTER>          m_clusters;

    // Part index -> 1-based CLSTID from the placement record's +108 field. Recorded for the new
    // 112-byte layout only; -1/absent = unclustered.
    std::map<size_t, int>              m_partClusterId;

    // sec69 physical stackup. Empty when the table could not be located, in which case
    // GetLayerInfos() synthesizes a fallback.
    std::vector<LAYER_INFO>             m_layerInfos;
};

} // namespace PADS_IO

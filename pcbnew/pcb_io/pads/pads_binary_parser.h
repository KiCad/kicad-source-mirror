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
    BoardSetup = 1,      // view-state / origin header
    PadStacks = 4,       // padstack definitions (64 B/rec)
    PadShapes = 5,       // per-padstack pad-shape layer table
    FreeText = 8,        // free-text / label table + string pool
    StringPool = 9,      // string-pool tail + DRW array head
    DrwItems = 10,       // LINES/DRW drawing-object array (112 B)
    GraphicPieces = 11,  // graphic-piece header + inline outline coords (20 B)
    Vertices = 12,       // graphic-piece vertex pool (12 B)
    DecalLibrary = 13,   // per-decal block table + trailing library padstack-index table
    DecalHeader = 14,    // PARTDECAL definition table (112 B)
    TerminalPool = 15,   // decal terminal position table (36 B)
    ParttypeDefs = 17,   // parttype/footprint defs (224 B)
    PartPins = 19,       // PARTTYPE pin-definition table (88 B)
    Placements = 22,     // part placements (112 B)
    Nets = 23,           // net records (424 B)
    Connections = 24,    // pin-pair topology records (68 B)
    ClearanceRules = 49, // clearance/design-rule heap
    PourTokensA = 52,    // pour-relation token streams
    PourTokensB = 53,
    PourTokensC = 54,
    PourTokensD = 55,  // pour arc records (16 B)
    Vias = 60,         // route-junction / via records (64 B)
    RouteObjects = 62, // routed-subpath descriptors (48 B)
    RouteLayers = 63,  // active copper-layer ordinals (2 B)
    RouteCells = 64,   // routed-subpath point cells (12 B)
    Clusters = 68,     // part-cluster (*CLUSTER*) controller (60 B/rec)
    LayerTable = 69,   // ODBLayer physical-stackup table (152 B/rec)
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
    std::vector<int>         ruleLayers; // layers carrying a clearance rule (0 = all layers)

    // Per-class rule values, positionally joined to the type-66 clearance edge by declaration
    // order. All in BASIC units. hasRuleValues is false when the value and edge counts disagree.
    int  clearance = 0;
    int  trackWidth = 0;
    int  minTrackWidth = 0;
    int  maxTrackWidth = 0;
    int  viaClearance = 0;
    bool hasRuleValues = false;
};

/// One board-outline vertex triplet: [i32 X, i32 Y, i32 attr] where attr -1 is a plain corner
/// and attr >= 0 is the arc-parameter ordinal.
struct ARC_VERTEX
{
    int32_t x = 0;
    int32_t y = 0;
    int32_t attr = 0;
};

/// One type-66 net-class rule edge: its owner pointer, rule-detail page, full rule pointer
/// (declaration order within a page), layer and file offset.
struct NET_CLASS_RULE_EDGE
{
    uint32_t owner = 0;
    uint32_t ruleKind = 0;
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
    int         id = 0; // 1-based ordinal; equals the sec22 +108 CLSTID reference
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

    const PARAMETERS&                        GetParameters() const { return m_parameters; }

    const std::vector<PART>&                 GetParts() const { return m_parts; }
    const std::vector<NET>&                  GetNets() const { return m_nets; }
    const std::vector<BIN_NET_CLASS_DEF>&    GetNetClasses() const { return m_netClasses; }
    const std::vector<DIFF_PAIR_DEF>&        GetDiffPairs() const { return m_diffPairs; }
    const std::vector<ROUTE>&                GetRoutes() const { return m_routes; }
    const std::vector<TEXT>&                 GetTexts() const { return m_texts; }
    const std::vector<POUR>&                 GetPours() const { return m_pours; }
    const std::vector<KEEPOUT>&              GetKeepouts() const { return m_keepouts; }
    const std::vector<COPPER_SHAPE>&         GetCopperShapes() const { return m_copper_shapes; }
    const std::vector<GRAPHIC_LINE>&         GetGraphicLines() const { return m_graphicLines; }
    const std::vector<DIMENSION>&            GetDimensions() const { return m_dimensions; }
    const std::vector<POLYLINE>&             GetBoardOutlines() const { return m_boardOutlines; }
    const std::map<std::string, PART_DECAL>& GetPartDecals() const { return m_decals; }

    // Part clusters and the per-part membership map. The membership key is an index into
    // GetParts(); the value is the 1-based CLSTID, which equals the GetClusters() ordinal.
    const std::vector<PART_CLUSTER>& GetClusters() const { return m_clusters; }
    const std::map<size_t, int>&     GetPartClusterIds() const { return m_partClusterId; }

    int      GetLayerCount() const { return m_parameters.layer_count; }
    uint16_t GetVersion() const { return m_version; }

    std::vector<LAYER_INFO> GetLayerInfos() const;

    std::set<std::string> GetPadStackShapesForTest() const
    {
        std::set<std::string> shapes;

        for( const std::vector<PAD_STACK_LAYER>& layers : m_padStackPool )
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
    static constexpr int32_t ANGLE_SCALE = 1800000;

    /// One placed via recovered from the section-60 junction ring. relationshipNet records
    /// whether netName came from the section-49 relationship graph, which outranks the
    /// record's own net index when co-located vias disagree.
    struct VIA_LOCATION
    {
        int32_t     x = 0;
        int32_t     y = 0;
        std::string netName;
        int         viaIndex = -1;
        bool        relationshipNet = false;
    };

    bool isOldFormat() const { return m_sdb.IsOldFormat(); }

    /// Base of section 69's layer records after its fixed controller lead-in.
    size_t layerStackupBase() const;

    // Both old dialects resolve a placement's decal via a direct index in the next physical
    // record rather than through a parttype-definition table (v0x2022 does have such a table,
    // but placements don't reference it); only the field's offset within that record differs.
    bool usesDirectDecalChain() const { return m_version <= 0x2022; }

    // The SECTION overload forwards to the int form so a constant section reads by role; the
    // int form serves callers that iterate a computed directory index.
    const SDB_SECTION* getSection( int aIndex ) const;
    const SDB_SECTION* getSection( SECTION aSection ) const { return getSection( static_cast<int>( aSection ) ); }

    void parseBoardSetup();
    void parsePartPlacements();


    // Build a PART from a placement record's refdes and its coordinate, rotation and side fields.
    PART makePlacementPart( const SDB_RECORD& aRec, int aXOff, std::optional<int> aYOff, int aAngleOff, int aNameOff,
                            const std::string& aRefDes ) const;
    void parsePadStacks();
    void parsePartDecals();
    void parseDecalNameTable();
    void parseDecalNameTableOld();
    void parsePartTypeTable();
    void parseBoardOutlineDirect();
    void parseGraphicLines();

    // Resolve the section-13 arc-parameter record that a corner's attr ordinal names, relative to
    // its owner's arc cursor. aWhat names the calling decoder in the range error.
    SDB_RECORD arcRecordFor( const SDB_SECTION& aArcParameters, int32_t aArcStart, int32_t aAttr,
                             const char* aWhat ) const;

    void parseNetNames();
    void parseNetNamesNew();
    void parseNetNamesOld();
    void parseNetConnectionsNew();
    std::map<uint32_t, size_t> parseRouteJunctionNets() const;

    // Offsets of the legacy serialized net table in dense-index order. Its logical ring starts
    // 20 bytes after section 23's physical cursor; a record's position in this sequence IS the
    // net ordinal that route and via
    // records reference, so the sequence must stay unfiltered even where a record is unusable.
    std::vector<size_t> oldNetRecordOffsets() const;

    // Recover v0x2021 pin-to-net membership from the section-24 connection stream. The stream is
    // not partitioned into per-net runs, so a net's members are the connected component of the
    // pin graph reached from the net record's first-connection index (record +8); the record's
    // connection count (+92) corroborates the join before any pin is attributed.
    void parseNetConnectionsOld();

    void resolveNetAnchors();

    // Recover part clusters. Membership itself is captured during parsePartPlacements into
    // m_partClusterId; a record's 1-based ordinal is the CLSTID that sec22 +108 references.
    void parseClusters();

    // Group nets by their +188 net-class pointer for membership, name each class from the
    // 0x118 name table, and read per-class clearance-rule layers from the type-66 rule table.
    void                             parseNetClasses();
    std::vector<NET_CLASS_RULE_EDGE> collectNetClassRuleEdges( const std::set<uint32_t>& aOwnerSet );
    void                             applyNetClassClearances( const std::vector<NET_CLASS_RULE_EDGE>& aEdges,
                                                              const std::map<uint32_t, size_t>&       aOwnerOrdinal );

    // Recover serialized differential pairs from the sec49 MFC heap. Member nets are the
    // self-pointers at +12/+16, joined to a net name via m_netSelfPtrToName. Inherit-default
    // pairs are not serialized, so coverage is limited to override pairs. v0x2027 only.
    void parseDiffPairs();
    void parseRouteVertices();

    // Section-60 phase of parseRouteVertices: decode the via-junction ring, join each junction
    // to its net through the section-49 relationship graph, and de-duplicate by coordinate.
    std::vector<VIA_LOCATION> decodeViaLocations();

    // Seed one ROUTE per net from the decoded vias, resolving each via's padstack and drill span
    // through the decal it names.
    std::map<std::string, ROUTE> seedRoutesFromVias( const std::vector<VIA_LOCATION>& aVias ) const;

    // Decode the sections 62/63/64 routed-copper descriptors into tracks and append them to the
    // seeded routes, attributing each object to a net through its route-node handle. Returns
    // false when the route-object controller declares nothing, which the caller reads as "this
    // board carries no routing" and drops the seeded via-only routes along with it.
    void decodeRoutedCopper( std::map<std::string, ROUTE>& aRoutes );

    /// Every routed-copper object's PADS layer and route-node handle, indexed by object ordinal,
    /// plus the net set each handle reaches through the node link graph.
    struct ROUTE_OBJECT_NODES
    {
        std::vector<int>                     layers;
        std::vector<uint32_t>                handles;
        std::map<uint32_t, std::set<size_t>> handleNets;
    };

    // Walk the section-25/26/27/29/61 route-node allocator to recover the layer, node handle and
    // reachable nets of every routed-copper object. aObjectCount is the section-62 descriptor
    // count, which the per-layer handle scan must reproduce exactly.
    ROUTE_OBJECT_NODES resolveRouteObjectNodes( const SDB_SECTION& aRouteLayers, size_t aObjectCount,
                                                const std::vector<int>& aSerializedLayerOrder );

    void parseTextRecords();

    // Attach the section-8 field-presentation chain of every placement that declares one to its
    // PART as attributes. Consumes m_partFieldStart.
    void parsePlacementFields( const SDB_SECTION& aText, size_t aRecordBase, size_t aRecordSize, size_t aRingRotation );

    // Emit a TEXT for every free-text section-8 record whose lagged metadata declares a
    // string-pool offset and a text layer.
    void parseFreeText( const SDB_SECTION& aText, size_t aRecordBase, size_t aRecordSize, size_t aRingRotation,
                        size_t aPoolBase, size_t aPoolHi );

    void parseTerminals();

    // Give every decal that owns terminals the global padstack-zero default; a decal's own
    // serialized (pin, ref) pairs override it later.
    void assignDefaultPadStacks();

    // Apply one decal's (pin, ref) pair slice: pin 0 sets the decal default, pin>0 overrides
    // that terminal. Shared by the descriptor and library passes.
    void applyPadstackPairs( PART_DECAL& aDecal, const std::vector<std::pair<int32_t, int32_t>>& aPairs, int32_t aStart,
                             int32_t aCount );
    void parseKeepouts();
    void parseCopperShapes();
    void parseCopperPours();

    // Reconstruct PADS dimensions, which the binary does not store as a dedicated section.
    // Each dimension is a DRW graphic-piece owner named DIM* whose sec12 vertex run holds the
    // leader sub-pieces. Must run after buildOwnerRuns and parseTextRecords, which it consumes.
    void parseDimensions();

    // Decode the versioned section-69 layer-definition and physical-stackup table from its
    // directly framed physical controller.
    void parseLayerStackup();

    void linkPartsToDecals();

    // Structural shape -> section-12 vertex link. Section 10 is a declared circular fixed array,
    // rotated left 68 bytes within its own extent. Owner R[i]'s cursors and item kind live in
    // R[(i+1) mod count]; no marker search or cross-controller walk participates.
    struct OWNER_RUN
    {
        int32_t  pieceStart = 0;  // R[i+1] @ +8
        int32_t  vertexStart = 0; // R[i+1] @ +12, cumulative sec12 corner cursor
        int32_t  arcStart = 0;    // R[i+1] @ +16
        int32_t  pieceCount = 0;  // R[i+1] @ +24
        uint32_t itemKind = 0;    // R[i+1] @ +28: low 16 bits are the item enum; high 16 are flags
        uint32_t ownerIndex = 0;  // R[i]'s index in the rotated section-10 record ring
    };

    // Owner DRW name -> lagged run, keyed by the +44 name. Built once by buildOwnerRuns().
    std::map<std::string, OWNER_RUN> m_ownerRuns;

    // Section 12 is a direct fixed array, so its usable row count is its declared record count.
    int32_t m_sec12CleanRows = 0;

    void buildOwnerRuns();
    void computeSec12CleanRows();

    uint8_t     ringU8( const SDB_SECTION& aSection, size_t aRotation, size_t aStride, uint32_t aIndex,
                        size_t aField ) const;
    uint32_t    ringU32( const SDB_SECTION& aSection, size_t aRotation, size_t aStride, uint32_t aIndex,
                         size_t aField ) const;
    int32_t     ringI32( const SDB_SECTION& aSection, size_t aRotation, size_t aStride, uint32_t aIndex,
                         size_t aField ) const;
    std::string ringStr( const SDB_SECTION& aSection, size_t aRotation, size_t aStride, uint32_t aIndex, size_t aField,
                         size_t aLength ) const;

    bool sec12Vertex( int32_t aRow, int32_t& aX, int32_t& aY, int32_t& aAttr ) const;

    // Fetch the closed polygon declared by the owner's vertexStart and its first piece's exact
    // corner count. Returns the design-coordinate vertices excluding the serialized closing
    // point. The repeated first point validates the declared range; it does not delimit it.
    bool fetchOwnerLoop( const std::string& aName, size_t aMaxVerts, std::vector<VECTOR2I>& aOut ) const;

    // A circular piece (PADS' COPCIR convention) stores exactly two diametrically opposite
    // endpoints, not a closed loop, so its declared corner count is two rather than a polygon's
    // closing-point-inclusive count. Returns the two points directly; the caller is
    // expected to cross-check the derived circle (center = midpoint, radius = half the span)
    // against the owner's own declared bbox, since two arbitrary adjacent points are not enough
    // signal on their own.
    bool fetchOwnerCirclePoints( const std::string& aName, VECTOR2I& aP0, VECTOR2I& aP1 ) const;

    bool isValidNetName( const std::string& aName ) const;

    // Coordinate conversion from binary absolute to PADS_IO design-relative units
    double toBasicCoordX( int32_t aRawValue ) const;
    double toBasicCoordY( int32_t aRawValue ) const;
    double toBasicAngle( int32_t aRawAngle ) const;

    // Owns the file bytes and decodes the container; the section readers work through it.
    PADS_SDB m_sdb;

    // m_data aliases the SDB's bytes for the absolute-offset readers; m_cursor follows it.
    // Both are declared after m_sdb so their references bind to its live buffer.
    const std::vector<uint8_t>& m_data = m_sdb.Bytes();
    BINARY_CURSOR               m_cursor{ m_data };

    uint16_t m_version = 0;

    // Coordinate origin from the serialized board-setup block.
    int32_t m_originX = 0;
    int32_t m_originY = 0;

    // Section-4 pad-stack pool, 0-based from its versioned logical ring start. Section-16
    // (terminal, padstack) pairs index this pool directly.
    std::vector<std::vector<PAD_STACK_LAYER>> m_padStackPool;
    std::vector<std::pair<int, int>>          m_padStackDrillSpans;

    // Part index -> parttype index from the NEXT section 22 record (@+4 with +1 block lag).
    // Indexes into m_partTypeDecalIndices.
    std::map<size_t, uint32_t> m_partTypeIndex;

    // Placement index -> first section-8 field-presentation record from section 22 +96.
    std::map<size_t, int32_t> m_partFieldStart;

    // Part index -> zero-based alternate decal selector from the NEXT placement record @+17.
    std::map<size_t, uint8_t> m_partDecalAlternate;

    // Part index -> direct decal index for v0x2021, which carries no parttype layer; the decal
    // is selected from the NEXT 96 B placement record's @+56 field. Indexes m_decalNameTable.
    std::map<size_t, uint32_t> m_partDecalIndex;

    // All decal indices for each parttype, primary first, then alternates. The parttype table is
    // the logical ring 44 bytes before section 17's physical cursor. New-format records store
    // duplicate index pairs at +96/+100, +104/+108, ... until -1; v0x2022 carries a single index
    // at +112. An empty entry means the parttype declares no decal.
    std::vector<std::vector<int32_t>> m_partTypeDecalIndices;

    // The parttype's own alias name at logical +44, parallel to
    // m_partTypeDecalIndices. This is the *PARTTYPE name a part references directly -- often a
    // manufacturer part number -- distinct from the physical decal it resolves to.
    std::vector<std::string> m_partTypeNames;

    // Complete section-14 logical decal table, whose record-zero NAME lands at the physical
    // cursor. Includes vias, connectors and mounting holes that section 10 lacks. table[0] is
    // always JMPVIA_AAAAA.
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

    // Decal name -> start cursor (i32 @ +44) into the section-15 (pin, padstack-ref) pool.
    std::map<std::string, int32_t> m_decalStackStart;

    // Route-node object handle -> the nets its section-60 junction belongs to. Built by
    // decodeViaLocations from the section-49 relationship graph; decodeRoutedCopper walks the
    // route-node link graph out from each object to reach it.
    std::map<uint32_t, std::set<size_t>> m_junctionHandleNets;

    // Section 23 array index -> net name, used to attribute structural vias to nets.
    std::map<uint32_t, std::string> m_sec23IndexToNet;
    std::map<uint32_t, size_t>      m_sec23RecordToNet;

    struct NET_ANCHOR
    {
        size_t   netIndex;
        uint32_t placementObject;
        uint32_t terminalOrdinal;
    };

    std::vector<NET_ANCHOR> m_netAnchors;
    std::vector<NET_ANCHOR> m_netConnectionEndpoints;

    // The serialized placement object ID is the nominal section-22 record ordinal plus 11.
    // Keep the object identity separate from m_parts order because invalid directory records
    // do not produce imported parts.
    std::map<uint32_t, size_t> m_placementObjectToPart;

    // Net name -> its net-class object pointer (record +188); zero/absent for unclassed nets.
    // Nets sharing a value are one class.
    std::map<std::string, uint32_t> m_netClassOwner;

    std::vector<BIN_NET_CLASS_DEF> m_netClasses;

    // Net self-pointer (record +184) -> net name. The join key that resolves a sec49 DIF_PAIR
    // object's +12/+16 member-net pointers to names.
    std::map<uint32_t, std::string> m_netSelfPtrToName;

    std::vector<DIFF_PAIR_DEF> m_diffPairs;

    // Output data, same structs as the ASCII parser.
    PARAMETERS                        m_parameters;
    std::vector<PART>                 m_parts;
    std::vector<NET>                  m_nets;
    std::vector<ROUTE>                m_routes;
    std::vector<TEXT>                 m_texts;
    std::vector<POUR>                 m_pours;
    std::vector<KEEPOUT>              m_keepouts;
    std::vector<COPPER_SHAPE>         m_copper_shapes;
    std::vector<GRAPHIC_LINE>         m_graphicLines;
    std::vector<DIMENSION>            m_dimensions;
    std::vector<POLYLINE>             m_boardOutlines;
    std::map<std::string, PART_DECAL> m_decals;

    // Part clusters in table order; index+1 is the CLSTID.
    std::vector<PART_CLUSTER> m_clusters;

    // Part index -> 1-based CLSTID from the placement record's +108 field. Recorded for the new
    // 112-byte layout only; -1/absent = unclustered.
    std::map<size_t, int> m_partClusterId;

    // Directly framed section-69 physical stackup.
    std::vector<LAYER_INFO> m_layerInfos;
};

} // namespace PADS_IO

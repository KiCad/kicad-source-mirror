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

/**
 * @file diptrace_pcb_parser.h
 * @brief Parser for DipTrace binary .dip board files.
 *
 * Reads the DipTrace binary format (magic: 07 DTBOARD) and creates
 * KiCad BOARD objects. Supports format versions 37 through 60.
 *
 * The DipTrace binary format uses big-endian encoding with biased integers:
 *   - int3: 3-byte, bias 1,000,000 (zero = 0x0F4240)
 *   - int4: 4-byte, bias 1,000,000,000 (zero = 0x3B9ACA00)
 *   - Strings: uint16-BE char count + UTF-16BE (v39+) or int3(byte_count) + ASCII (v37)
 *   - Coordinate unit: 100/3 nm (~33.33nm), 762 units per mil
 */

#ifndef DIPTRACE_PCB_PARSER_H_
#define DIPTRACE_PCB_PARSER_H_

#include <io/diptrace/diptrace_binary_reader.h>
#include <math/vector2d.h>

#include <map>
#include <unordered_map>
#include <vector>

class BOARD;
class FOOTPRINT;
class NETINFO_ITEM;
class PAD;
class PCB_SHAPE;
class PCB_TEXT;
enum PCB_LAYER_ID : int;

namespace DIPTRACE
{

// ---------------------------------------------------------------------------
// Intermediate data structures for parsed DipTrace content
// ---------------------------------------------------------------------------

struct DT_VERTEX
{
    int     x = 0;         ///< X coordinate in DipTrace units
    int     y = 0;         ///< Y coordinate in DipTrace units
    uint8_t arc = 0;       ///< 0 = straight segment, 1 = arc segment
};

struct DT_LAYER
{
    uint8_t  flag = 0;
    int      index = 0;
    uint32_t color = 0;    ///< 0x00RRGGBB
    wxString name;
    int      type = 0;     ///< Layer type from record field_a (0 = Signal, 1 = Plane)
    int      planeNetIndex = -1; ///< Plane net DipTrace index from record field_c (-1 = none/Signal)
    int      fieldD = 0;   ///< Possibly default trace width
};

struct DT_VIA_STYLE
{
    wxString name;
    int      outerDiameter = 0;
    int      drillDiameter = 0;
    int      layer1 = -1;
    int      layer2 = -1;
};

struct DT_DESIGN_RULE
{
    wxString name;
    int      clearance = 0;
    int      trackWidth = 0;
};

struct DT_PAD
{
    int      index = 0;        ///< Sequential pad index within the component (1-based)
    int      netIndex = -1;    ///< Net index from DipTrace file (-1 = unconnected)
    int      x = 0;            ///< X offset from component origin in DipTrace units
    int      y = 0;            ///< Y offset from component origin in DipTrace units
    wxString number;           ///< Pad number/name (e.g. "1", "2")
    wxString label;            ///< Functional label (e.g. "POS", "GND")
    int      width = 0;        ///< Copper pad width in DipTrace units
    int      height = 0;       ///< Copper pad height in DipTrace units
    int      drillWidth = 0;   ///< Drill width in DipTrace units (0 for SMD)
    int      drillHeight = 0;  ///< Drill height in DipTrace units (0 for SMD)
    int      style = 1;        ///< Pad style (0=ellipse, 1=oval, 2=rectangle, 3=polygon)
    uint8_t  mountType = 0;    ///< Explicit mount class from pad post-block (0=through, 1=SMD)
    uint8_t  orientClass = 0;  ///< Pad orientation class from pad post-block tail byte

    /// Custom polygon vertices relative to pad center (DipTrace units).
    /// Non-empty when the pad style field C=3, indicating a polygon/custom shape.
    std::vector<std::pair<int, int>> polygonVertices;
};


struct DT_MOUNT_HOLE
{
    int x = 0;              ///< X offset from component origin in DipTrace units
    int y = 0;              ///< Y offset from component origin in DipTrace units
    int outerDiameter = 0;  ///< Non-copper/clearance diameter in DipTrace units
    int drillDiameter = 0;  ///< Drill diameter in DipTrace units
};

enum DT_SHAPE_TYPE
{
    DT_SHAPE_EMPTY      = 0,
    DT_SHAPE_LINE       = 1,
    DT_SHAPE_RECT       = 2,
    DT_SHAPE_CIRCLE     = 3,
    DT_SHAPE_ARC        = 6,
    DT_SHAPE_FILLOBROUND = 700, ///< Filled obround marker (e.g. diode cathode / pin-1 dot)
    DT_SHAPE_END        = -1
};

struct DT_FP_SHAPE
{
    int type = DT_SHAPE_EMPTY;   ///< Shape type (DT_SHAPE_TYPE)
    int x1 = 0;                  ///< Start X in normalized units (range ±5000)
    int y1 = 0;                  ///< Start Y in normalized units
    int x2 = 0;                  ///< End X in normalized units
    int y2 = 0;                  ///< End Y in normalized units
    int midX = 0;                ///< Arc midpoint X in normalized units
    int midY = 0;                ///< Arc midpoint Y in normalized units
    int width = 0;               ///< Line width in normalized units (-10000 = default)
    int layer = 0;               ///< DipTrace layer index
};

struct DT_COMPONENT
{
    wxString libraryPath;
    int      positionX = 0;    ///< DipTrace units
    int      positionY = 0;    ///< DipTrace units
    int      placementQuarterTurns = 0; ///< Board-placement angle snapped to 90-degree turns (metadata Id-6 int3)
    bool     hasPlacementQuarterTurns = false;
    double   placementAngleDeg = 0.0;  ///< Exact board-placement angle in degrees (placement section), when available
    bool     hasPlacementAngle = false;
    int      rotation = 0;     ///< Raw header int4; matches Pattern.Float1 in DipXML (not placement angle)
    int      fieldC = 0;       ///< Raw header int4; matches Pattern.Float2 in DipXML
    int      fieldD = 0;       ///< Raw header int4; matches Pattern.Float3 in DipXML
    wxString patternName;
    wxString displayName;
    wxString refdes;
    wxString value;
    int      layer = 0;        ///< 0 = top, 1 = bottom
    int      bboxWidth = 0;    ///< Footprint X extent in DipTrace units (for shape scaling)
    int      bboxHeight = 0;   ///< Footprint Y extent in DipTrace units (for shape scaling)
    int      padWidthHint = 0;   ///< Raw bbox companion field (pad width in DipTrace units)
    int      padHeightHint = 0;  ///< Raw bbox companion field (pad height in DipTrace units)
    int      drillWidthHint = 0; ///< Raw bbox companion field (drill width in DipTrace units)
    int      drillHeightHint = 0;///< Raw bbox companion field (drill height in DipTrace units)
    int      fieldA = 0;       ///< Raw header int3 field A
    int      fieldF = 0;       ///< Raw header int3 field F (component kind discriminator)
    bool     isStandaloneVia = false; ///< True for explicit standalone via components
    std::vector<uint8_t>    flags;
    std::vector<DT_PAD>     pads;
    std::vector<DT_MOUNT_HOLE> holes;
    std::vector<DT_FP_SHAPE> shapes;  ///< Graphics in normalized coordinates
    size_t boundaryOffset = 0;         ///< Boundary marker offset for this component record
    size_t stringStartOffset = 0;      ///< Parsed start offset of library-path string
    size_t regionEndOffset = 0;        ///< Component region end offset (next boundary / upper bound)
    size_t headerEndOffset = 0;        ///< Byte offset after parsed component header strings
    size_t padRegionEnd = 0;           ///< Byte offset after last pad record (for shape finding)

    // Text positioning from the 37-byte component tail
    int  refdesYOffset = 0;    ///< Refdes text Y offset from component origin (DipTrace units)
    int  valueYOffset = 0;     ///< Value text Y offset from component origin (DipTrace units)
    bool refdesVisible = true; ///< False when text visibility flag is -1
    bool valueVisible = true;  ///< (Currently same flag as refdesVisible)
    bool hasTailData = false;  ///< True if the 37-byte tail was successfully parsed
};

struct DT_TEXT_OBJECT
{
    wxString text;
    wxString fontName;
    int      lineWidth = 0;    ///< DipTrace units
    int      layer = 0;
    int      x1 = 0;
    int      y1 = 0;
    int      x2 = 0;
    int      y2 = 0;
    uint32_t color = 0;        ///< 0x00RRGGBB
};

struct DT_PAD_REF
{
    int componentIndex = -1;   ///< Component index referenced from net routing metadata
    int padIndex = -1;         ///< Pad index (1-based within the component)
};

struct DT_NET
{
    int      index = 0;        ///< Sequential net index from the DipTrace file
    wxString name;             ///< Stored net name; may be empty in DipTrace files
    int      traceWidth = 0;   ///< Default trace width in DipTrace units; parsed but not yet used
    int      defaultViaOuterDiam = 0;  ///< Default via OD from net routing preamble
    int      defaultViaDrillDiam = 0;  ///< Default via drill from net routing preamble
    std::vector<DT_PAD_REF> padRefs;   ///< Optional net-to-pad links from routing metadata
};

struct DT_TRACK_NODE
{
    int  x = 0;               ///< X coordinate in DipTrace units
    int  y = 0;               ///< Y coordinate in DipTrace units
    int  layer = 0;           ///< Copper layer index (0=top, 1=bottom, 14+=inner)
    int  width = 0;           ///< Track width in DipTrace units
    uint8_t routeFlag = 0;    ///< Raw routing-point flag byte at payload +22 (semantics unresolved)
    int  routeMode = 0;       ///< Raw routing-point mode int3 at payload +37 (observed: 0/1/3)
    bool hasVia = false;      ///< True if a via exists at this node
    int  viaOuterDiam = 0;    ///< Via outer diameter in DipTrace units
    int  viaDrillDiam = 0;    ///< Via drill diameter in DipTrace units
    int  viaStyleIdx = -1;    ///< Index into ViaStyle table (-1 = none)
};

struct DT_TRACK_CHAIN
{
    int                        netIndex = -1;  ///< Net this chain belongs to
    std::vector<DT_TRACK_NODE> nodes;          ///< Ordered sequence of routing nodes
};

struct DT_ZONE_CACHED_FILL_RECORD
{
    int field0 = 0; ///< Raw int3 discriminator/index
    int field1 = 0; ///< Raw int4 payload field 1
    int field2 = 0; ///< Raw int4 payload field 2
    int field3 = 0; ///< Raw int4 payload field 3
    int field4 = 0; ///< Raw int4 payload field 4
    int field5 = 0; ///< Raw int4 payload field 5
};

struct DT_ZONE
{
    int netIndex = -1;     ///< Net index (-1 = unconnected)
    int layer = 0;         ///< DipTrace layer (0=top, 1=bottom)
    int priority = 0;      ///< Fill priority; left at the DipTrace default until the binary field is located
    uint8_t fillMode = 0;  ///< Raw zone flag byte at header +3
    uint8_t rawFlag2 = 0;  ///< Raw zone flag byte at header +4 (semantics unknown)
    uint8_t connectionMode = 0; ///< Raw zone flag byte at header +5
    int separator = 0;     ///< Raw zone separator int3 at header +18
    int clearance = 0;     ///< Zone clearance in DipTrace units
    int minWidth = 0;      ///< Copper pour line width in DipTrace units (DipXML: LineWidth)
    int minimumArea = 0;   ///< Minimum island area scalar in DipTrace units (DipXML: MinimumArea)
    int spokeMode = -1;    ///< Raw spoke enum from post-fill style block (0=Direct, 3=4 spoke 45, 4=4 spoke)
    int lineSpacing = 0;   ///< Copper pour line spacing in DipTrace units (DipXML: LineSpacing)
    int spokeWidth = 0;    ///< Thermal relief spoke width in DipTrace units
    int regionsCounted = 0; ///< Raw trailer int3, likely CopperPour Regions_Counted from Pcb.exe
    int cachedFillByteLen = 0; ///< Raw bytes between style block and trailer in inter-zone gap
    int cachedFillRecordCount = 0; ///< Cached-fill record count when payload is 23-byte aligned
    int boardClearance = 0; ///< Raw board-clearance field from zone trailer
    int zoneId = -1;       ///< Raw per-zone id from zone trailer (matches DipXML CopperPour@Id)
    int smdSpokeMode = -1; ///< Raw SMD_Spoke enum from zone trailer
    int smdSpokeWidth = 0; ///< Raw SMD_SpokeWidth from zone trailer
    uint8_t viaDirect = 0; ///< Raw ViaDirect flag from zone trailer
    uint8_t smdSeparate = 0; ///< Raw SMD_Separate flag from zone trailer
    uint8_t islandRegion = 0; ///< Raw IslandRegion flag from zone trailer
    uint8_t islandInternal = 0; ///< Raw IslandInternal flag from zone trailer
    uint8_t islandConnection = 0; ///< Raw IslandConnection flag from zone trailer
    uint8_t ratlineMode = 0; ///< Raw ratline mode (0=Automatically, 1=All Ratlines, 2=Do Not Hide)
    uint8_t regionsDone = 0; ///< Raw RegionsDone flag from zone trailer
    std::vector<DT_ZONE_CACHED_FILL_RECORD> cachedFillRecords; ///< Raw 23-byte cached fill records
    std::vector<std::pair<int, int>> outline;  ///< Outline vertices (x, y) in DipTrace units
};


// ---------------------------------------------------------------------------
// Main parser class
// ---------------------------------------------------------------------------

/**
 * Parses a DipTrace .dip binary board file and populates a KiCad BOARD.
 */
class PCB_PARSER
{
public:
    /**
     * Construct a parser for the given file.
     *
     * @param aFileName path to the .dip file
     * @param aBoard target BOARD to populate (must not be null)
     */
    PCB_PARSER( const wxString& aFileName, BOARD* aBoard );
    ~PCB_PARSER();

    /// Parse the file and populate the board. Throws IO_ERROR on failure.
    void Parse();

    /**
     * Number of objects or sections that were located by byte-pattern scanning rather
     * than a deterministic field-derived offset during the last Parse(). Zero means the
     * decode was fully deterministic; the remaining pattern scanners act only as recovery
     * fallbacks. Mirrors the schematic importer's ComponentBoundaryScanCount() invariant.
     */
    int ScanLocatorUseCount() const
    {
        return m_componentLocatorScans + m_padLocatorScans + m_shapeLocatorScans
               + m_mountHoleLocatorScans + m_sectionLocatorScans;
    }

    int ComponentLocatorScans() const { return m_componentLocatorScans; }
    int PadLocatorScans() const { return m_padLocatorScans; }
    int ShapeLocatorScans() const { return m_shapeLocatorScans; }
    int MountHoleLocatorScans() const { return m_mountHoleLocatorScans; }
    int SectionLocatorScans() const { return m_sectionLocatorScans; }

    /**
     * Build a KiCad custom design-rule (.kicad_dru) document for the per-zone
     * DipTrace properties that have no native KiCad zone equivalent.
     *
     * Covers zone-to-board-edge clearance (BoardClearance) and direct via
     * connections (ViaDirect). Returns an empty string when no such rules apply.
     * Must be called after Parse().
     */
    wxString GenerateDesignRules() const;

private:
    // --- Section parsers ---
    void ParseMagic();
    void ParseBoardProperties();
    void ParseOutline();
    void ParsePostOutline();
    void ParseLayers();
    void ParseFontStyle();
    void ParsePatternNameGroups( int aGroupCount );
    void ParsePatternStyleGroups( int aGroupCount );
    void ParseImplicitPatternStyleGroup();
    void ParseDesignRules();
    void FindAndParseComponents();

    /**
     * Refine component placement angles with the exact values from the placement section.
     *
     * The per-component metadata block yields only a 90-degree-snapped quarter-turn count.
     * A separate placement section stores the exact placement angle (radians, fixed-point)
     * for every component, but the section has no per-entry key that maps cleanly back to a
     * geometry record. When the section's angle list lines up with the parsed components --
     * same count and every angle snapping to the component's own quarter-turn -- the exact
     * angles are adopted; otherwise the snapped quarter turns are kept.
     */
    void ApplyPlacementAngles();

    void ParsePostComponentSections();

    /// Deterministically walk the component boundaries from the design-rules end, anchoring on
    /// each component's 37-byte tail (followed by a boundary core) instead of scanning the whole
    /// region for boundary patterns. Returns (boundaryOffset, stringStart) per component, or an
    /// empty vector when the walk cannot be trusted (the caller then falls back to the scan).
    std::vector<std::pair<size_t, size_t>> FieldWalkComponentBoundaries( size_t aUpperBound );

    // --- Component parsing helpers ---
    bool ParseSingleComponent( size_t aBoundaryOffset, size_t aUpperBound,
                               DT_COMPONENT& aComp );

    /// Decide whether a parsed component is a standalone via rather than a placed footprint.
    static bool ClassifyStandaloneVia( const DT_COMPONENT& aComp );

    /// Search a component's data region for pad records using pad name anchors.
    void FindPadsInRegion( DT_COMPONENT& aComp, size_t aRegionStart, size_t aRegionEnd );

    /// Parse component-local mechanical holes (NPTH) from the post-pad region.
    void FindMountHolesInRegion( DT_COMPONENT& aComp, size_t aRegionStart, size_t aRegionEnd );

    /// Parse footprint outline shapes from the region after pad data.
    void FindShapesInRegion( DT_COMPONENT& aComp, size_t aRegionStart, size_t aRegionEnd );

    /// Parse shapes from per-layer font blocks (v46+ format).
    void FindShapesInFontBlocks( DT_COMPONENT& aComp, size_t aRegionStart, size_t aRegionEnd );

    /// Parse shapes from chained fixed-size records used by some v46+ footprints.
    void FindShapesInChainedBlocks( DT_COMPONENT& aComp, size_t aRegionStart, size_t aRegionEnd );

    /// Parse the 37-byte tail at the end of a component region to extract text positioning.
    void ParseComponentTail( DT_COMPONENT& aComp, size_t aRegionEnd );

    // --- Net and routing parsing ---
    void FindAndParseNets( size_t aSearchStart, size_t aSearchEnd );
    void ParseNetRouting( DT_NET& aNet );
    void InferPadNetsFromRoutingRefs();

    // --- Zone parsing ---
    void FindAndParseZones( size_t aSearchStart, size_t aSearchEnd );

    // --- Board object creation ---
    void ApplyBoardSettings();
    void CreateBoardOutline();
    void CreateFootprint( const DT_COMPONENT& aComp );
    void CreateStandaloneVias();
    void CreateTextObject( const DT_TEXT_OBJECT& aText );
    void CreateNets();
    void CreateTracksAndVias();
    void CreateZones();

    /// Synthesize board-outline-bounded plane fills for negative/solid-plane copper layers.
    void CreatePlaneZones();

    /// Resolve a DipTrace net index to the corresponding KiCad net object.
    NETINFO_ITEM* ResolveNetByIndex( int aDipTraceNetIndex ) const;
    const DT_NET* ResolveDipTraceNetByIndex( int aDipTraceNetIndex ) const;

    // --- Text object parsing ---
    void FindAndParseTextObjects( size_t aSearchStart, size_t aSearchEnd );
    void ParseTextRecords( int aCount );

    // --- Utility ---

    /// Map a DipTrace layer index to a KiCad PCB_LAYER_ID.
    PCB_LAYER_ID MapLayer( int aDipTraceLayer ) const;
    PCB_LAYER_ID MapCopperLayer( int aDipTraceLayer ) const;

    /// Convert a DipTrace coordinate (DipTrace units) to KiCad internal units (nm).
    static int ToKiCadCoord( int aDipTraceCoord );

    /// Convert a DipTrace angle value to degrees (tenths of degree).
    static double ToKiCadAngleDeg( int aDipTraceAngle );

    /// Read an inter-ruleset transition block.
    void SkipInterRulesetTransition();

    /// Try to read a string at a given raw data position.
    static bool TryReadStringAt( const uint8_t* aData, size_t aDataSize,
                                 size_t aPos, int aVersion,
                                 wxString& aOut, size_t& aNewPos );

    /// Find all occurrences of a byte pattern within data[start:end].
    static std::vector<size_t> FindAllBoundaries( const uint8_t* aData, size_t aDataSize,
                                                  const uint8_t* aPattern, size_t aPatternLen,
                                                  size_t aStart, size_t aEnd );

    // --- Data ---
    BINARY_READER           m_reader;
    BOARD*                  m_board;
    int                     m_version;
    bool                    m_hasInlineVersion;
    bool                    m_hasLegacyMagicLayout;

    // Parsed intermediate data
    std::vector<DT_VERTEX>      m_outline;
    std::vector<DT_LAYER>       m_layers;
    std::vector<DT_VIA_STYLE>   m_viaStyles;
    std::vector<DT_DESIGN_RULE> m_designRules;
    std::vector<DT_COMPONENT>   m_components;
    std::vector<DT_TEXT_OBJECT>  m_textObjects;
    std::vector<DT_NET>         m_nets;
    std::vector<DT_TRACK_CHAIN> m_trackChains;
    std::vector<DT_ZONE>        m_zones;
    std::unordered_map<int, int> m_copperLayerOrdinalById;
    std::unordered_map<int, std::vector<VECTOR2I>> m_routingAnchorsByNet;
    bool m_routingAnchorCacheBuilt = false;

    // Link table between DipTrace net indices and created KiCad nets.
    std::unordered_map<int, NETINFO_ITEM*> m_kicadNetByDipTraceIndex;
    std::unordered_map<int, const DT_NET*> m_dipTraceNetByIndex;

    // Board properties
    int m_bboxXMin = 0;
    int m_bboxYMin = 0;
    int m_bboxXMax = 0;
    int m_bboxYMax = 0;

    // Section tracking
    int    m_ruleNameCount = 0;
    size_t m_postLayersOffset = 0;
    size_t m_postDesignRulesOffset = 0;
    size_t m_componentUpperBound = 0;

    // Determinism instrumentation. Each counter records how many objects/sections of its
    // category were located by byte-pattern scanning rather than a field-derived offset.
    // The determinism work drives every category to zero, one at a time, while keeping the
    // scanners as recovery fallbacks. See ScanLocatorUseCount().
    int m_componentLocatorScans = 0;
    int m_padLocatorScans = 0;
    int m_shapeLocatorScans = 0;
    int m_mountHoleLocatorScans = 0;
    int m_sectionLocatorScans = 0;
};

}  // namespace DIPTRACE

#endif    // DIPTRACE_PCB_PARSER_H_

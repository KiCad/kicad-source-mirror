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

#include "pads_binary_parser.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <numbers>
#include <numeric>
#include <optional>
#include <set>
#include <unordered_set>

#include <fmt/format.h>
#include <io/pads/pads_binary_utils.h>
#include <ki_exception.h>
#include <wx/log.h>

namespace PADS_IO
{

// Section 4 pad-shape codes. The finger shapes 0 (OF) and 1 (RF) carry a non-zero finLength
// (size B); the round 2 (R) and square 3 (S) never do.
static const std::map<uint8_t, std::string> PAD_SHAPE_NAMES = {
    { 0x00, "OF" },
    { 0x01, "RF" },
    { 0x02, "R" },
    { 0x03, "S" },
};


// Per-version field layout for the section-4 padstack records. As with the placements, the old
// and new dialects carry the same geometry fields at different offsets.
struct PADSTACK_LAYOUT
{
    int padWidthOff = 0;
    int drillOff = 0;
    int finLenOff = 0;
    int cornerOff = 0;
    int angleOff = 0;
    int layerStartOff = 0;
    int markerOff = 0;
    int shapeOff = 0;
    int layerCountOff = 0;
    int drillStartOff = -1;
    int drillEndOff = -1;
};


static const PADSTACK_LAYOUT& padstackLayout( uint16_t aVersion )
{
    static constexpr PADSTACK_LAYOUT v2021{ .padWidthOff = 24,
                                            .drillOff = 28,
                                            .finLenOff = 32,
                                            .cornerOff = 36,
                                            .angleOff = 40,
                                            .layerStartOff = 44,
                                            .markerOff = 48,
                                            .shapeOff = 49,
                                            .layerCountOff = 50,
                                            .drillStartOff = -1,
                                            .drillEndOff = -1 };
    static constexpr PADSTACK_LAYOUT v2022{ .padWidthOff = 20,
                                            .drillOff = 24,
                                            .finLenOff = 28,
                                            .cornerOff = 32,
                                            .angleOff = 40,
                                            .layerStartOff = 44,
                                            .markerOff = 48,
                                            .shapeOff = 49,
                                            .layerCountOff = 50,
                                            .drillStartOff = 51,
                                            .drillEndOff = 52 };
    static constexpr PADSTACK_LAYOUT vNew{ .padWidthOff = 28,
                                           .drillOff = 32,
                                           .finLenOff = 36,
                                           .cornerOff = 44,
                                           .angleOff = 48,
                                           .layerStartOff = 52,
                                           .markerOff = 56,
                                           .shapeOff = 57,
                                           .layerCountOff = 58,
                                           .drillStartOff = 59,
                                           .drillEndOff = 60 };

    if( aVersion <= 0x2021 )
        return v2021;

    if( aVersion == 0x2022 )
        return v2022;

    return vNew;
}


// The 112-byte header shared by the LINES/DRW owner and item records. Several decoders read the
// same fields (origin, bounding box, run cursors, name, classifier word); one descriptor keeps
// their offsets in a single place. The +24 word has two record-role-dependent meanings, both
// named here: CLASS_WORD in the copper/keepout headers and PIECE_COUNT in the owner-run cursor.
namespace DRW_ITEM
{
    constexpr size_t SIZE = 112;
    constexpr int    PIECE_START = 8;
    constexpr int    VERTEX_START = 12;
    constexpr int    ARC_START = 16;
    constexpr int    CLASS_WORD = 24;
    constexpr int    PIECE_COUNT = 24;
    constexpr int    SUBTYPE_WORD = 28;
    constexpr int    NAME = 44;
    constexpr int    TYPE_TAG = 84;
    constexpr int    ORIGIN_X = 88;
    constexpr int    ORIGIN_Y = 92;
    constexpr int    BBOX_MIN_X = 96;
    constexpr int    BBOX_MIN_Y = 100;
    constexpr int    BBOX_MAX_X = 104;
    constexpr int    BBOX_MAX_Y = 108;
} // namespace DRW_ITEM


// Block-class tags carried at DRW_ITEM::TYPE_TAG. 0x4D00 also marks the board-outline item and
// is the library decal marker elsewhere.
namespace DRW_TAG
{
    constexpr uint32_t COPPER_FILL = 0x00004900;   // filled copper region
    constexpr uint32_t COPPER_FILL_B = 0x0000FF00; // second filled-copper tag, same class/shape
    constexpr uint32_t LINE_ITEM = 0x00004D00;     // line/outline item (board outline, v2026 line copper)
} // namespace DRW_TAG


BINARY_PARSER::BINARY_PARSER() = default;


BINARY_PARSER::~BINARY_PARSER() = default;


bool BINARY_PARSER::IsBinaryPadsFile( const wxString& aFileName )
{
    std::ifstream file( aFileName.fn_str(), std::ios::binary );

    if( !file.is_open() )
        return false;

    uint8_t header[4];
    file.read( reinterpret_cast<char*>( header ), 4 );

    if( file.gcount() < 4 )
        return false;

    if( header[0] != 0x00 || header[1] != 0xFF )
        return false;

    uint16_t version = static_cast<uint16_t>( header[2] ) | ( static_cast<uint16_t>( header[3] ) << 8 );

    return PADS_SDB::IsSupportedVersion( version );
}


/// Enabled by setting KICAD_PADS_PROFILE.
static void logParsePhase( const char* aWhat, std::chrono::steady_clock::time_point aStart )
{
    static const bool enabled = getenv( "KICAD_PADS_PROFILE" ) != nullptr;

    if( !enabled )
        return;

    const double ms = std::chrono::duration<double, std::milli>( std::chrono::steady_clock::now() - aStart ).count();

    fputs( fmt::format( "PROF {:<32} {:>8.1f} ms\n", aWhat, ms ).c_str(), stderr );
}


void BINARY_PARSER::Parse( const wxString& aFileName )
{
    std::vector<uint8_t> bytes;

    if( !PADS_IO::ReadFileToBuffer( aFileName, bytes ) )
        THROW_IO_ERROR( "Cannot open or read file" );

    m_sdb.Load( std::move( bytes ) );

    m_version = m_sdb.Version();
    m_originX = m_sdb.Coords().OriginX();
    m_originY = m_sdb.Coords().OriginY();
    m_originFound = m_sdb.Coords().Found();

    m_parameters.origin.x = static_cast<double>( m_originX );
    m_parameters.origin.y = static_cast<double>( m_originY );

#define KITIME( call )                                                                                                 \
    do                                                                                                                 \
    {                                                                                                                  \
        auto _t0 = std::chrono::steady_clock::now();                                                                   \
        call;                                                                                                          \
        logParsePhase( #call, _t0 );                                                                                   \
    } while( 0 )

    // The call order below is load-bearing; the dependency edges noted at each group fix it.

    // Container state loads the version, origin and parameter block.
    KITIME( parseBoardSetup() );

    // Part placements and the part-cluster groups that reference them.
    KITIME( parsePartPlacements() );
    KITIME( parseClusters() );

    // Padstacks and the decal / part-type tables that linkPartsToDecals joins at the end.
    KITIME( parsePadStacks() );
    KITIME( parseDecalNameTable() );
    KITIME( parsePartTypeTable() );
    KITIME( parsePartDecals() );

    // Sections 10 and 11 are circularly serialized fixed arrays. Reconstruct their direct
    // owner/piece links before any graphic decoder consumes them.
    KITIME( computeSec12Base() );
    KITIME( buildOwnerRuns() );

    KITIME( parseBoardOutlineDirect() );
    KITIME( parseGraphicLines() );

    // Net names first; the net-class and diff-pair passes key off the net records they produce.
    KITIME( parseNetNames() );
    KITIME( parseNetClasses() );
    KITIME( parseDiffPairs() );

    KITIME( linkPartsToDecals() );

    // Route-cell coordinate order is selected by each layer's serialized routing direction.
    KITIME( parseLayerStackup() );

    // Structural geometry.
    KITIME( parseTextRecords() );
    KITIME( parseRouteVertices() );
    KITIME( parseKeepouts() );
    KITIME( parseCopperShapes() );
    KITIME( parseCopperPours() );
    KITIME( parseDimensions() );

    KITIME( resolveNetAnchors() );

    m_parts.erase( std::remove_if( m_parts.begin(), m_parts.end(),
                                   []( const PART& p )
                                   {
                                       return p.name.empty();
                                   } ),
                   m_parts.end() );
}


const SDB_SECTION* BINARY_PARSER::getSection( int aIndex ) const
{
    return m_sdb.Section( aIndex );
}


double BINARY_PARSER::toBasicCoordX( int32_t aRawValue ) const
{
    return static_cast<double>( aRawValue );
}


double BINARY_PARSER::toBasicCoordY( int32_t aRawValue ) const
{
    return static_cast<double>( aRawValue );
}


double BINARY_PARSER::toBasicAngle( int32_t aRawAngle ) const
{
    if( aRawAngle == 0 )
        return 0.0;

    return static_cast<double>( aRawAngle ) / static_cast<double>( ANGLE_SCALE );
}


void BINARY_PARSER::parseBoardSetup()
{
    // MAXIMUMLAYER lives in the same directly framed *PCB* board-setup parameter block as the
    // coordinate origin.
    if( !m_sdb.Coords().Found() )
        THROW_IO_ERROR( "Missing PADS board origin" );

    uint32_t headerBase = m_sdb.Coords().HeaderBase();

    if( !m_cursor.InBounds( headerBase, 20 ) )
        THROW_IO_ERROR( "Invalid PADS board-setup parameter extent" );

    // Word 4 of the fixed u32 parameter block is the maximum layer count.
    uint32_t maxLayer = m_sdb.RecordAt( headerBase ).U32( 16 );

    if( maxLayer >= 1 && maxLayer <= 64 )
        m_parameters.layer_count = static_cast<int>( maxLayer );
    else
        THROW_IO_ERROR( "Invalid PADS maximum-layer field" );

    // Binary coordinates are BASIC units (1/38100 mil). MILS is only the display unit;
    // coordinate handling uses BASIC mode in the wrapper via SetBasicUnitsMode(true).
    m_parameters.units = UNIT_TYPE::MILS;
}


PART BINARY_PARSER::makePlacementPart( const SDB_RECORD& aRec, int aXOff, std::optional<int> aYOff, int aAngleOff,
                                       int aNameOff, const std::string& aRefDes ) const
{
    PART part;
    part.name = aRefDes;
    part.location.x = toBasicCoordX( aRec.I32( aXOff ) );
    part.location.y = aYOff ? toBasicCoordY( aRec.I32( *aYOff ) ) : 0;
    part.rotation = toBasicAngle( aRec.I32( aAngleOff ) );

    // The side flag is the word at nameOff+28 in both dialects; bit 0 marks a bottom placement.
    part.bottom_layer = ( aRec.U8( aNameOff + 28 ) & 0x01 ) != 0;
    part.units = "M";
    return part;
}


void BINARY_PARSER::parsePartPlacements()
{
    const SDB_SECTION* section = getSection( SECTION::Placements );

    constexpr size_t LOGICAL_ROTATION = 44;

    if( !section )
        THROW_IO_ERROR( "Missing PADS placement controller" );

    if( section->count == 0 )
        return;

    if( section->physicalCount != section->count || section->physicalOffset < LOGICAL_ROTATION
        || ( section->stride != 96 && section->stride != 112 ) )
        THROW_IO_ERROR( "Invalid PADS placement-controller framing" );

    const size_t logicalBase = section->physicalOffset - LOGICAL_ROTATION;

    if( !m_cursor.InBounds( logicalBase, section->totalBytes + LOGICAL_ROTATION ) )
        THROW_IO_ERROR( "Invalid PADS placement-controller extent" );

    for( uint32_t index = 0; index < section->physicalCount; ++index )
    {
        const size_t base = logicalBase + static_cast<size_t>( index ) * section->stride;
        SDB_RECORD   record = m_sdb.RecordAt( static_cast<uint32_t>( base ) );
        std::string  refdes = record.Str( 44, 16 );
        const size_t nextBase = logicalBase + static_cast<size_t>( index + 1 ) * section->stride;
        SDB_RECORD   nextRecord = m_sdb.RecordAt( static_cast<uint32_t>( nextBase ) );

        PART part = makePlacementPart( record, 60, 64, 68, 44, refdes );

        if( section->stride == 112 )
            m_partFieldStart[m_parts.size()] = record.I32( 96 );

        if( section->stride == 96 )
            m_partDecalIndex[m_parts.size()] = nextRecord.U32( 24 );
        else
        {
            m_partTypeIndex[m_parts.size()] = nextRecord.U32( 4 );
            m_partDecalAlternate[m_parts.size()] = nextRecord.U8( 17 );
        }

        if( section->stride == 112 )
        {
            int32_t clusterId = record.I32( 108 );

            if( clusterId > 0 )
                m_partClusterId[m_parts.size()] = clusterId;
        }

        m_placementObjectToPart[index] = m_parts.size();
        m_parts.push_back( std::move( part ) );
    }
}

void BINARY_PARSER::parseClusters()
{
    // A part cluster is a fixed 60-byte record, id@+0 and name@+4 (char[16], NUL-padded), in
    // cluster order. The stored id and the record's 1-based ordinal both equal the CLSTID the
    // +108 field references. Membership
    // is captured during parsePartPlacements. The old 96-byte placement layout has no room for
    // the +108 CLSTID, so old-format boards carry no clusters.
    //
    // Section 68 directly precedes section 69 in the physical loader stream.
    if( isOldFormat() )
        return;

    const SDB_SECTION* sec68 = getSection( SECTION::Clusters );

    if( !sec68 )
        THROW_IO_ERROR( "Missing PADS cluster controller" );

    if( sec68->count == 0 )
        return;

    if( sec68->stride != 60 )
        THROW_IO_ERROR( "Invalid PADS cluster-controller stride" );

    static constexpr size_t REC_SIZE = 60;
    size_t                  sec69Rec0 = layerStackupBase();

    if( sec69Rec0 == 0 )
        THROW_IO_ERROR( "Missing PADS layer-stackup framing" );

    constexpr size_t SEC69_LEAD_IN = 12;

    if( sec69Rec0 < SEC69_LEAD_IN || sec68->count > ( sec69Rec0 - SEC69_LEAD_IN ) / REC_SIZE )
        THROW_IO_ERROR( "Invalid PADS cluster-controller extent" );

    size_t base = sec69Rec0 - SEC69_LEAD_IN - static_cast<size_t>( sec68->count ) * REC_SIZE;

    for( uint32_t i = 0; i < sec68->count; ++i )
    {
        SDB_RECORD  rec = m_sdb.RecordAt( base + i * REC_SIZE );
        std::string name = rec.Str( 4, 16 );

        // A misaligned base would read non-cluster bytes. The stored ordinal and printable name
        // confirm the run without constraining retained state fields that vary among boards.
        if( rec.U32( 0 ) != i + 1 || name.empty() )
            THROW_IO_ERROR( "Invalid PADS cluster record" );

        PART_CLUSTER cluster;
        cluster.name = std::move( name );
        cluster.id = static_cast<int>( i ) + 1;
        m_clusters.push_back( std::move( cluster ) );
    }
}


void BINARY_PARSER::parsePadStacks()
{
    const SDB_SECTION* entry = getSection( SECTION::PadStacks );
    const SDB_SECTION* layerSection = getSection( SECTION::PadShapes );

    if( !entry || !layerSection )
        THROW_IO_ERROR( "Missing PADS padstack controllers" );

    const uint32_t expectedRecordSize = m_version <= 0x2021 ? 52 : m_version == 0x2022 ? 56 : 64;

    if( entry->physicalCount != entry->count
        || static_cast<uint64_t>( entry->count ) * expectedRecordSize != entry->totalBytes )
        THROW_IO_ERROR( "Invalid PADS padstack-controller framing" );

    if( entry->count == 0 )
        return;

    const uint32_t logicalRotation = m_version == 0x2022 ? 20 : m_version <= 0x2021 ? 24 : 28;

    if( entry->physicalOffset < logicalRotation )
        THROW_IO_ERROR( "Invalid PADS padstack-controller rotation" );

    const PADSTACK_LAYOUT& layout = padstackLayout( m_version );
    uint32_t               recSize = entry->stride;

    // Read one padstack record's default (layer 0) geometry. For finger pads (RF, OF, RC)
    // finLength is the second dimension; round (R) and square (S) reuse sizeA.
    auto readLayer = [&]( uint32_t aBase ) -> PAD_STACK_LAYER
    {
        SDB_RECORD  rec = m_sdb.RecordAt( aBase );
        std::string shapeName = "R";
        auto        shapeIt = PAD_SHAPE_NAMES.find( rec.U8( layout.shapeOff ) );

        if( shapeIt != PAD_SHAPE_NAMES.end() )
            shapeName = shapeIt->second;

        int32_t padWidth = rec.I32( layout.padWidthOff );
        int32_t drill = rec.I32( layout.drillOff );
        int32_t finLength = rec.I32( layout.finLenOff );

        PAD_STACK_LAYER psl;
        psl.layer = -2;
        psl.shape = shapeName;
        psl.sizeA = static_cast<double>( padWidth );
        psl.drill = static_cast<double>( drill );
        psl.corner_radius = std::max( rec.I32( layout.cornerOff ), 0 );

        if( m_version <= 0x2022 )
        {
            constexpr int OLD_NPTH_CLEARANCE = 4 * static_cast<int>( SDB_BASIC_PER_MIL );
            psl.plated = drill > 0 && padWidth - drill > OLD_NPTH_CLEARANCE;
        }
        else
        {
            psl.plated = drill > 0 && drill < padWidth;
        }

        bool isFinger = ( shapeName == "RF" || shapeName == "OF" || shapeName == "RC" );

        if( isFinger )
            psl.rotation = toBasicAngle( rec.I32( layout.angleOff ) );

        if( isFinger && finLength > 0 )
            psl.sizeB = static_cast<double>( finLength );
        else
            psl.sizeB = static_cast<double>( padWidth );

        return psl;
    };

    const uint32_t poolStart = entry->physicalOffset - logicalRotation;
    const uint64_t poolBytes = static_cast<uint64_t>( entry->physicalCount ) * recSize;

    if( !m_cursor.InBounds( poolStart, poolBytes ) )
        THROW_IO_ERROR( "Invalid PADS padstack-pool extent" );

    m_padStackPool.assign( entry->physicalCount, {} );
    m_padStackDrillSpans.assign( entry->physicalCount, { 0, 0 } );
    m_padStackCache.clear();
    std::vector<int32_t> maxPadDiameter( entry->physicalCount, 0 );

    const uint32_t layerRecordSize = m_version <= 0x2021 ? 20 : 24;
    const uint32_t layerTableHeaderSize = m_version == 0x2022 ? 64 : m_version <= 0x2021 ? 20 : 24;
    uint64_t       layerTableStart64 = static_cast<uint64_t>( poolStart ) + poolBytes + layerTableHeaderSize;
    const uint64_t layerTableBytes = static_cast<uint64_t>( layerSection->count ) * layerRecordSize;

    if( layerTableBytes != layerSection->totalBytes || layerTableStart64 > m_data.size()
        || layerTableBytes > m_data.size() - layerTableStart64 )
    {
        THROW_IO_ERROR( "Invalid PADS pad-layer-controller framing" );
    }

    const uint32_t layerTableStart = static_cast<uint32_t>( layerTableStart64 );

    // Pad stacks are indexed by their position in the object array; part decals reference them by
    // that stable index.
    for( uint32_t i = 0; i < entry->physicalCount; ++i )
    {
        uint32_t base = poolStart + i * recSize;

        if( m_sdb.RecordAt( base ).U8( layout.markerOff ) != 0xFE )
            continue;

        PAD_STACK_LAYER defaultLayer = readLayer( base );

        // Slotted-drill metadata is carried by the following physical padstack record. Bit 3
        // marks the carrier; +8 is the preceding stack's slot length and +12 its orientation.
        if( m_version >= 0x2024 && defaultLayer.drill > 0 && i + 1 < entry->physicalCount )
        {
            SDB_RECORD nextRec = m_sdb.RecordAt( base + recSize );

            if( ( nextRec.U32( 0 ) & 8U ) != 0 )
            {
                defaultLayer.slot_length = nextRec.I32( 8 );
                defaultLayer.slot_orientation = toBasicAngle( nextRec.I32( 12 ) );
            }
        }

        std::vector<PAD_STACK_LAYER>& layers = m_padStackPool[i];
        layers.push_back( defaultLayer );
        maxPadDiameter[i] = static_cast<int32_t>( defaultLayer.sizeA );

        if( layout.drillStartOff >= 0 && layout.drillEndOff >= 0 )
        {
            m_padStackDrillSpans[i] = { m_cursor.U8At( base + layout.drillStartOff ),
                                        m_cursor.U8At( base + layout.drillEndOff ) };
        }

        SDB_RECORD stackRec = m_sdb.RecordAt( base );
        uint32_t   layerStart = stackRec.U32( layout.layerStartOff );
        uint8_t    layerCount = stackRec.U8( layout.layerCountOff );

        if( layerCount > 0
            && ( layerStart >= layerSection->count || layerCount > layerSection->count - layerStart ) )
            THROW_IO_ERROR( "Invalid PADS pad-layer range" );

        if( layerCount > 0 )
        {
            for( uint32_t layerIdx = 0; layerIdx < layerCount; ++layerIdx )
            {
                uint32_t   rowBase = layerTableStart + ( layerStart + layerIdx ) * layerRecordSize;
                SDB_RECORD rowRec = m_sdb.RecordAt( rowBase );
                uint32_t   geometryBase = rowBase;

                if( m_version == 0x2022 && layerStart + layerIdx > 0 )
                    geometryBase -= layerRecordSize;

                SDB_RECORD geometryRec = m_sdb.RecordAt( geometryBase );
                uint8_t    selector = rowRec.U8( 0 );
                uint8_t    shapeCode = rowRec.U8( 1 );
                auto       shapeIt = PAD_SHAPE_NAMES.find( shapeCode );
                maxPadDiameter[i] = std::max( maxPadDiameter[i], geometryRec.I32( 4 ) );

                if( shapeIt == PAD_SHAPE_NAMES.end() )
                    continue;

                PAD_STACK_LAYER layer = defaultLayer;
                layer.layer = selector == 0      ? 0
                              : selector == 0xFF ? -1
                                                 : static_cast<int>( selector ) + ( m_version == 0x2022 ? 0 : 1 );
                layer.shape = shapeIt->second;
                layer.sizeA = static_cast<double>( geometryRec.I32( 4 ) );
                int32_t sizeB = geometryRec.I32( 8 );
                layer.sizeB = sizeB > 0 ? static_cast<double>( sizeB ) : layer.sizeA;
                layers.push_back( std::move( layer ) );
            }
        }

        m_padStackCache[static_cast<int>( i )] = layers;
    }

}


/// Rotation between the physical section cursor and the logical record base.
static constexpr int DECAL_NAME_OFFSET = 44;

/// Report a structurally derived physical section base for cross-checking the Kaitai grammar.
/// Enabled by setting KICAD_PADS_SECBASE.
static void logResolvedBase( int aSection, const char* aWhat, size_t aResolved, uint32_t aPhysical )
{
    static const bool enabled = getenv( "KICAD_PADS_SECBASE" ) != nullptr;

    if( !enabled )
        return;

    fprintf( stderr, "SECBASE %d %s resolved=%zu physical=%u\n", aSection, aWhat, aResolved, aPhysical );
}


void BINARY_PARSER::parseDecalNameTable()
{
    // The complete decal-name table is the logical section-14 ring. Each record is 112 bytes
    // with the decal NAME at the physical section cursor, a 0xFFFE
    // sentinel at +64 and the terminal count at +72. Unlike section 10 this table includes vias,
    // connectors and mounting holes, and is indexed directly (base 0) by a parttype's
    // decal_index. The first record is always JMPVIA_AAAAA, used as an anchor sanity check. The
    // +72 count is harvested into m_decalTerminalCount so passives without a section 14
    // descriptor get exact pad counts.
    if( m_version <= 0x2022 )
    {
        parseDecalNameTableOld();
        return;
    }

    // The table is section 14's own records, reached from the corrected payload offset, so no
    // header-size constant is needed.
    static constexpr int REC_SIZE = 112;
    static constexpr int STACK_START_OFFSET = 44;
    static constexpr int SENTINEL_OFFSET = 64;
    static constexpr int START_OFFSET = 68;
    static constexpr int COUNT_OFFSET = 72;
    static constexpr int STACK_COUNT_OFFSET = 88;

    const SDB_SECTION* sec14 = getSection( SECTION::DecalHeader );

    if( !sec14 )
        THROW_IO_ERROR( "Missing PADS decal controller" );

    if( sec14->count == 0 )
        return;

    uint32_t start = sec14->physicalOffset;

    if( start + 12 > m_data.size() || m_sdb.RecordAt( start ).Str( 0, 12 ) != "JMPVIA_AAAAA" )
        THROW_IO_ERROR( "Invalid PADS decal-controller framing" );

    // Bound the file-supplied count before it sizes an allocation or a loop
    if( static_cast<uint64_t>( sec14->count ) * REC_SIZE > m_data.size() - start )
        THROW_IO_ERROR( "Invalid PADS decal-name table extent" );

    m_decalNameTable.clear();
    m_decalNameTable.reserve( sec14->count );

    for( uint32_t k = 0; k < sec14->count; ++k )
    {
        uint32_t   off = start + k * REC_SIZE;
        SDB_RECORD rec = m_sdb.RecordAt( off );

        if( off + REC_SIZE > m_data.size() || rec.U16( SENTINEL_OFFSET ) != SDB_RECORD_SENTINEL )
        {
            m_decalNameTable.emplace_back();
            continue;
        }

        std::string name = rec.Str( 0, 41 );
        m_decalNameTable.push_back( name );

        int32_t startCursor = rec.I32( START_OFFSET );
        int32_t count = rec.I32( COUNT_OFFSET );

        if( !name.empty() && count > 0 && count <= 1000 )
        {
            m_decalTerminalCount.emplace( name, static_cast<uint32_t>( count ) );

            if( startCursor >= 0 )
                m_decalTerminalStart.emplace( name, startCursor );

            int32_t stackCount = rec.I32( STACK_COUNT_OFFSET );

            if( stackCount > 0 && stackCount <= 1000 )
            {
                m_decalStackCount.emplace( name, stackCount );

                int32_t stackStart = rec.I32( STACK_START_OFFSET );

                if( stackStart >= 0 )
                    m_decalStackStart.emplace( name, stackStart );
            }
        }
    }
}


void BINARY_PARSER::parseDecalNameTableOld()
{
    // The v0x2017 through v0x2022 dialects carry the same complete decal-name table the newer
    // ones do, at the same place in their section 14: record 0's name starts at the physical
    // section cursor, 44 bytes into the logical record. Each record holds NAME @ +0, a 0xFFFE
    // sentinel @ +64 that terminates the table, and a terminal count @ +72.
    //
    // The stride is the section's own declared stride, so the 100-byte old and 112-byte new
    // records need no version branch. Verified against all 165 corpus files of every version --
    // the JMPVIA_AAAAA name of record 0 lands exactly on this offset on every one of them, which
    // is what retired the whole-file signature scan this used to do.
    if( m_version > 0x2022 )
        return;

    const SDB_SECTION* sec14 = m_sdb.Section( 14 );

    if( !sec14 || sec14->stride == 0 )
        THROW_IO_ERROR( "Invalid PADS legacy decal controller" );

    static constexpr int STACK_START_OFFSET = 44;
    static constexpr int SENTINEL_OFFSET = 64;
    static constexpr int START_OFFSET = 68;
    static constexpr int COUNT_OFFSET = 72;

    size_t start = sec14->physicalOffset;
    size_t stride = sec14->stride;

    std::vector<std::string>        table;
    std::map<std::string, uint32_t> counts;
    std::map<std::string, int32_t>  starts;
    std::map<std::string, int32_t>  stackCounts;
    std::map<std::string, int32_t>  stackStarts;

    if( start > m_data.size()
        || static_cast<uint64_t>( sec14->count ) * stride > static_cast<uint64_t>( m_data.size() - start ) )
    {
        THROW_IO_ERROR( "Invalid PADS legacy decal-controller extent" );
    }

    table.reserve( sec14->count );

    for( size_t k = 0; k < sec14->count; ++k )
    {
        size_t off = start + k * stride;
        SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( off ) );

        if( rec.U16( SENTINEL_OFFSET ) != SDB_RECORD_SENTINEL )
        {
            table.emplace_back();
            continue;
        }

        std::string name = rec.Str( 0, 40 );
        table.push_back( name );

        if( off + COUNT_OFFSET + 4 <= m_data.size() )
        {
            int32_t count = rec.I32( COUNT_OFFSET );
            int32_t cursor = rec.I32( START_OFFSET );
            int32_t stackCount = rec.I32( 88 );

            if( !name.empty() && count > 0 && count <= 1000 )
            {
                counts.emplace( name, static_cast<uint32_t>( count ) );

                if( cursor >= 0 )
                {
                    // v2017/v2019 store positive terminal cursors as one-based pool ordinals;
                    // zero is the shared first slot used by the built-in via decals.
                    starts.emplace( name, m_version <= 0x2019 && cursor > 0 ? cursor - 1 : cursor );
                }

                if( stackCount > 0 && stackCount <= 1000 )
                {
                    stackCounts.emplace( name, stackCount );

                    // Every dialect stores this cursor, and it has to be read rather than
                    // re-derived by accumulating counts in decal-table order: the slices form
                    // a ring, so the table's first decals do not sit at the front of the pair
                    // pool. On PSTAGE-002 the 36 decals tile 64 slots with STANDARDVIA at 0,
                    // S2 last at 61, then THERMALVIA at 62 and JMPVIA_AAAAA at 63 wrapping
                    // back to 0.
                    int32_t stackStart = rec.I32( STACK_START_OFFSET );

                    if( stackStart >= 0 )
                        stackStarts.emplace( name, stackStart );
                }
            }
        }
    }

    // Record 0 is always the JMPVIA_AAAAA pseudo-decal, so its name doubles as a check that the
    // structural offset landed on the table rather than on unrelated bytes.
    if( table.empty() || table[0] != "JMPVIA_AAAAA" )
        THROW_IO_ERROR( "Invalid PADS legacy decal-controller framing" );

    m_decalNameTable = std::move( table );
    m_decalTerminalCount = std::move( counts );
    m_decalTerminalStart = std::move( starts );
    m_decalStackCount = std::move( stackCounts );
    m_decalStackStart = std::move( stackStarts );
}


void BINARY_PARSER::parsePartTypeTable()
{
    // The parttype-definition table's record size, decal_index field offset, and header framing
    // all differ between dialects. The logical record ring starts 44 bytes before section 17's
    // physical cursor. Modern records are 224 bytes with decal_index at +96; legacy records
    // are 208 bytes with decal_index at +112 (v0x2021 has no parttype-definition layer at all).
    // Verified against 2FOC_4.pcb: the decal_index for DSPIC33FJ128MC802/TP-LC/LMC7101/
    // MOLEX53261_0790 all land on +112 and resolve to their correct ground-truth decal names via
    // m_decalNameTable. Note placements don't reference this table for v0x2022 -- see
    // usesDirectDecalChain -- but it is still populated for potential future use (e.g. rules).
    if( m_version == 0x2021 )
        return;

    const SDB_SECTION* sec17 = getSection( SECTION::ParttypeDefs );

    if( !sec17 || sec17->count == 0 )
        return;

    if( sec17->totalBytes % sec17->count != 0 )
        THROW_IO_ERROR( "Invalid PADS parttype-controller record extent" );

    const uint32_t recSize = sec17->totalBytes / sec17->count;

    if( recSize != 208 && recSize != 224 )
        THROW_IO_ERROR( "Unsupported PADS parttype-controller record stride" );

    const bool     isLegacyLayout = recSize == 208;
    const uint32_t decalOff = isLegacyLayout ? 112 : 96;

    if( sec17->physicalOffset < DECAL_NAME_OFFSET )
        THROW_IO_ERROR( "Invalid PADS parttype-controller framing" );

    uint32_t start = sec17->physicalOffset - DECAL_NAME_OFFSET;

    // The parttype's own name (the *PARTTYPE alias, e.g. a manufacturer part number such as
    // GRM15XR71C103KA86D that resolves to a generic decal like C-0402) sits at +44 in both the
    // 208-byte and 224-byte records.
    const uint32_t nameOff = 44;

    if( !m_cursor.InBounds( start, static_cast<size_t>( sec17->count ) * recSize ) )
        THROW_IO_ERROR( "Invalid PADS parttype-controller extent" );

    m_partTypeDecalIndex.clear();
    m_partTypeDecalIndex.reserve( sec17->count );
    m_partTypeDecalIndices.clear();
    m_partTypeDecalIndices.reserve( sec17->count );
    m_partTypeNames.clear();

    m_partTypeNames.reserve( sec17->count );

    for( uint32_t k = 0; k < sec17->count; ++k )
    {
        uint32_t off = start + k * recSize;

        SDB_RECORD           record = m_sdb.RecordAt( off );
        std::vector<int32_t> decalIndices;

        for( uint32_t indexOff = decalOff; !isLegacyLayout && indexOff + 8 <= recSize; indexOff += 8 )
        {
            int32_t decalIndex = record.I32( indexOff );

            if( decalIndex < 0 || record.I32( indexOff + 4 ) != decalIndex )
                break;

            decalIndices.push_back( decalIndex );
        }

        if( isLegacyLayout && record.I32( decalOff ) >= 0 )
            decalIndices.push_back( record.I32( decalOff ) );

        m_partTypeDecalIndex.push_back( decalIndices.empty() ? -1 : decalIndices.front() );
        m_partTypeDecalIndices.push_back( std::move( decalIndices ) );

        m_partTypeNames.push_back( record.Str( nameOff, 36 ) );
    }
}


void BINARY_PARSER::parsePartDecals()
{
    // Section 14 is the complete PARTDECAL declaration table. Section 10 is the unrelated
    // board-drawing owner ring; treating its DRW names as decals created phantom definitions.
    for( const std::string& name : m_decalNameTable )
    {
        if( name.empty() )
            continue;

        PART_DECAL decal;
        decal.name = name;
        decal.units = "M";
        m_decals[name] = decal;
    }

    parseTerminals();
}


void BINARY_PARSER::parseTerminals()
{
    const SDB_SECTION* section = getSection( SECTION::TerminalPool );

    if( !section )
        THROW_IO_ERROR( "Missing PADS terminal controller" );

    if( section->count == 0 )
        return;

    const size_t stride = m_version <= 0x2019 ? 20 : 36;
    const size_t base = static_cast<size_t>( section->physicalOffset ) + ( m_version <= 0x2019 ? 16 : 0 );
    const size_t bytes = static_cast<size_t>( section->count ) * stride;

    if( base > m_data.size() || bytes > m_data.size() - base )
        THROW_IO_ERROR( "Invalid PADS terminal-controller extent" );

    struct DIRECT_TERMINAL
    {
        int32_t     x;
        int32_t     y;
        std::string name;
    };

    std::vector<DIRECT_TERMINAL> terminals;
    terminals.reserve( section->count );

    for( uint32_t index = 0; index < section->count; ++index )
    {
        SDB_RECORD record = m_sdb.RecordAt( static_cast<uint32_t>( base + index * stride ) );

        if( m_version <= 0x2019 )
            terminals.push_back( { record.I32( 4 ), record.I32( 8 ), {} } );
        else
            terminals.push_back( { record.I32( 0 ), record.I32( 4 ), record.Str( 20, 4 ) } );
    }

    for( auto& [name, decal] : m_decals )
    {
        auto startIt = m_decalTerminalStart.find( name );
        auto countIt = m_decalTerminalCount.find( name );

        if( startIt == m_decalTerminalStart.end() || countIt == m_decalTerminalCount.end() || startIt->second < 0 )
        {
            continue;
        }

        const size_t start = static_cast<size_t>( startIt->second );
        const size_t count = countIt->second;

        if( start > terminals.size() || count > terminals.size() - start )
            THROW_IO_ERROR( "Invalid PADS decal terminal range" );

        decal.terminals.clear();
        decal.terminals.reserve( count );

        for( size_t index = 0; index < count; ++index )
        {
            const DIRECT_TERMINAL& serialized = terminals[start + index];
            TERMINAL               terminal;
            terminal.x = toBasicCoordX( serialized.x );
            terminal.y = toBasicCoordY( serialized.y );
            terminal.name = serialized.name.empty() ? std::to_string( index + 1 ) : serialized.name;
            decal.terminals.push_back( std::move( terminal ) );
        }
    }

    assignDefaultPadStacks();

    size_t pairCount = 0;

    for( const auto& [name, start] : m_decalStackStart )
    {
        auto countIt = m_decalStackCount.find( name );

        if( start >= 0 && countIt != m_decalStackCount.end() && countIt->second > 0 )
        {
            pairCount = std::max( pairCount, static_cast<size_t>( start ) + static_cast<size_t>( countIt->second ) );
        }
    }

    const size_t pairBase = base + bytes - ( m_version <= 0x2019 ? 16 : 0 );

    if( pairCount > 0 )
    {
        if( pairBase > m_data.size() || pairCount * 8 > m_data.size() - pairBase )
            THROW_IO_ERROR( "Invalid PADS per-pin padstack extent" );

        std::vector<std::pair<int32_t, int32_t>> pairs;
        pairs.reserve( pairCount );

        for( size_t index = 0; index < pairCount; ++index )
        {
            SDB_RECORD record = m_sdb.RecordAt( static_cast<uint32_t>( pairBase + index * 8 ) );

            pairs.emplace_back( record.I32( 0 ), record.I32( 4 ) );
        }

        for( const auto& [name, start] : m_decalStackStart )
        {
            auto decalIt = m_decals.find( name );
            auto countIt = m_decalStackCount.find( name );

            if( decalIt != m_decals.end() && countIt != m_decalStackCount.end() )
                applyPadstackPairs( decalIt->second, pairs, start, countIt->second );
        }
    }

    return;
}


void BINARY_PARSER::assignDefaultPadStacks()
{
    // Global padstack zero is the implicit default until a decal's serialized ordinal-zero pair
    // replaces it. Positive pair ordinals override individual one-based terminals.
    for( auto& [name, decal] : m_decals )
    {
        if( decal.terminals.empty() )
            continue;

        if( m_padStackCache.count( 0 ) )
        {
            decal.pad_stacks[0] = m_padStackCache[0];

            if( !m_padStackDrillSpans.empty() )
                decal.drill_spans[0] = m_padStackDrillSpans[0];
        }
    }
}


void BINARY_PARSER::applyPadstackPairs( PART_DECAL& aDecal, const std::vector<std::pair<int32_t, int32_t>>& aPairs,
                                        int32_t aStart, int32_t aCount )
{
    if( aStart < 0 || aCount <= 0 || static_cast<size_t>( aStart ) + static_cast<size_t>( aCount ) > aPairs.size() )
        THROW_IO_ERROR( "Invalid PADS per-pin padstack range" );

    for( int32_t p = 0; p < aCount; ++p )
    {
        const std::pair<int32_t, int32_t>& pair = aPairs[static_cast<size_t>( aStart ) + p];

        if( pair.first < 0 || static_cast<size_t>( pair.second ) >= m_padStackPool.size()
            || m_padStackPool[pair.second].empty() )
        {
            THROW_IO_ERROR( "Invalid PADS per-pin padstack reference" );
        }

        if( pair.first > 0 && static_cast<size_t>( pair.first ) > aDecal.terminals.size() )
            THROW_IO_ERROR( "Invalid PADS per-pin terminal ordinal" );

        aDecal.pad_stacks[pair.first] = m_padStackPool[pair.second];
        aDecal.drill_spans[pair.first] = m_padStackDrillSpans[pair.second];
    }
}


bool BINARY_PARSER::isValidNetName( const std::string& aName ) const
{
    return !aName.empty() && aName != "___Unassigned_Obstacles_";
}


void BINARY_PARSER::parseNetNames()
{
    if( m_version <= 0x2022 )
        parseNetNamesOld();
    else
        parseNetNamesNew();
}


void BINARY_PARSER::parseNetNamesNew()
{
    std::unordered_set<std::string> existing;

    // Section 23 is a 44-byte-rotated circular array. Its physical cursor is the logical base
    // plus 44 bytes.
    const uint32_t     netRecordSize = m_version == 0x2024 ? 416 : 424;
    constexpr uint32_t NET_NAME = 76;
    constexpr uint32_t NET_NAME_LEN = 48;
    constexpr uint32_t NET_SELF_PTR = 144;
    constexpr uint32_t NET_CLASS_PTR = 148;

    const SDB_SECTION* nets = getSection( SECTION::Nets );

    if( !nets )
        THROW_IO_ERROR( "Missing PADS net controller" );

    if( nets->count == 0 )
        return;

    if( nets->stride != netRecordSize || nets->physicalOffset < 44
        || !m_cursor.InBounds( nets->physicalOffset - 44, nets->physicalBytes ) )
    {
        THROW_IO_ERROR( "Invalid PADS net-controller framing" );
    }

    const size_t base = nets->physicalOffset - 44;

    for( uint32_t i = 0; i < nets->count; ++i )
    {
        SDB_RECORD  rec = m_sdb.RecordAt( base + static_cast<size_t>( i ) * netRecordSize );
        std::string name = rec.Str( NET_NAME, NET_NAME_LEN );

        if( name.empty() || !isValidNetName( name ) || !existing.insert( name ).second )
            continue;

        NET net;
        net.name = name;
        m_nets.push_back( net );
        m_sec23RecordToNet[i] = m_nets.size() - 1;
        m_sec23IndexToNet[i] = name;
        m_netAnchors.push_back( { m_nets.size() - 1, rec.U32( 64 ), rec.U32( 68 ) } );

        if( uint32_t owner = rec.U32( NET_CLASS_PTR ) )
            m_netClassOwner[name] = owner;

        if( uint32_t selfPtr = rec.U32( NET_SELF_PTR ) )
            m_netSelfPtrToName[selfPtr] = name;
    }

    parseNetConnectionsNew();
}


void BINARY_PARSER::parseNetConnectionsNew()
{
    constexpr size_t   RECORD_SIZE = 68;
    constexpr uint32_t MARKER = 0xFE000000;
    constexpr uint32_t FLAG = 0x0000FFFE;

    const SDB_SECTION* connections = getSection( SECTION::Connections );
    const SDB_SECTION* nets = getSection( SECTION::Nets );

    const size_t netRecordSize = m_version == 0x2024 ? 416 : 424;

    if( !connections || !nets )
        THROW_IO_ERROR( "Missing PADS net-connection controllers" );

    if( connections->count == 0 )
        return;

    if( nets->stride != netRecordSize )
        THROW_IO_ERROR( "Invalid PADS net-controller stride" );

    constexpr size_t RING_PREFIX = 36;
    if( connections->physicalBytes != connections->physicalCount * RECORD_SIZE
        || connections->physicalOffset < RING_PREFIX
        || !m_cursor.InBounds( connections->physicalOffset - RING_PREFIX,
                               connections->physicalBytes + RING_PREFIX ) )
    {
        THROW_IO_ERROR( "Invalid PADS net-connection ring extent" );
    }

    auto connectionU32 = [&]( uint32_t aIndex, size_t aField )
    {
        size_t logical = connections->physicalOffset - RING_PREFIX
                         + static_cast<size_t>( aIndex ) * RECORD_SIZE + aField;
        return m_cursor.U32At( logical );
    };

    size_t runBase = static_cast<size_t>( connections->physicalOffset ) - RING_PREFIX;
    size_t runCount = static_cast<size_t>( connections->count ) + 1;

    // Record zero is the topology root. The directory count is the number of edges, so the
    // logical ring has one more record than the physical controller: its final 36-byte head is
    // serialized after the controller while record zero's 36-byte prefix precedes it.
    for( uint32_t recordIndex = 0; recordIndex < runCount; ++recordIndex )
    {
        if( ( recordIndex > 0 && ( connectionU32( recordIndex, 20 ) & 0xFFFFFFC0U ) != MARKER )
            || ( recordIndex < connections->count && ( connectionU32( recordIndex, 52 ) & 0xFFFFU ) != FLAG ) )
        {
            THROW_IO_ERROR( "Invalid PADS net-connection ring framing" );
        }
    }

    logResolvedBase( 24, "connRun", runBase, connections ? connections->physicalOffset : 0 );

    using PIN_ID = std::pair<uint32_t, uint32_t>;

    std::map<PIN_ID, size_t> pinIndex;
    std::vector<PIN_ID>      pins;
    std::vector<size_t>      parent;

    auto intern = [&]( const PIN_ID& aPin )
    {
        auto [it, inserted] = pinIndex.emplace( aPin, parent.size() );

        if( inserted )
        {
            pins.push_back( aPin );
            parent.push_back( parent.size() );
        }

        return it->second;
    };

    auto findRoot = [&]( size_t aNode )
    {
        while( parent[aNode] != aNode )
        {
            parent[aNode] = parent[parent[aNode]];
            aNode = parent[aNode];
        }

        return aNode;
    };

    auto join = [&]( const PIN_ID& aPin, const PIN_ID& bPin )
    {
        const size_t indexA = intern( aPin );
        const size_t indexB = intern( bPin );
        const size_t rootA = findRoot( indexA );
        const size_t rootB = findRoot( indexB );

        if( rootA != rootB )
            parent[rootA] = rootB;
    };

    for( uint32_t recordIndex = 0; recordIndex + 1 < runCount; ++recordIndex )
    {
        join( { connectionU32( recordIndex, 60 ), connectionU32( recordIndex + 1, 0 ) },
              { connectionU32( recordIndex, 64 ), connectionU32( recordIndex + 1, 4 ) } );
    }

    std::map<size_t, size_t> componentNet;

    for( const NET_ANCHOR& anchor : m_netAnchors )
    {
        if( anchor.netIndex >= m_nets.size() || anchor.terminalOrdinal == 0 )
            continue;

        const size_t root = findRoot( intern( { anchor.placementObject, anchor.terminalOrdinal } ) );
        auto [it, inserted] = componentNet.emplace( root, anchor.netIndex );

        if( !inserted && it->second != anchor.netIndex )
        {
            // PADS keeps $$$ autoroute aliases as separate net records even after joining
            // their pins into a named signal's connection component.
            const bool existingIsAlias = m_nets[it->second].name.rfind( "$$$", 0 ) == 0;
            const bool incomingIsAlias = m_nets[anchor.netIndex].name.rfind( "$$$", 0 ) == 0;

            if( existingIsAlias && incomingIsAlias )
                continue;
            else if( existingIsAlias != incomingIsAlias )
                it->second = incomingIsAlias ? it->second : anchor.netIndex;
            else
                THROW_IO_ERROR( "Conflicting PADS net anchors '" + m_nets[it->second].name + "' and '"
                                + m_nets[anchor.netIndex].name + "'" );
        }
    }

    for( size_t index = 0; index < pins.size(); ++index )
    {
        auto netIt = componentNet.find( findRoot( index ) );

        if( netIt == componentNet.end() )
            continue;

        const auto [placementObject, terminalOrdinal] = pins[index];
        m_netConnectionEndpoints.push_back( { netIt->second, placementObject, terminalOrdinal } );

        auto partIt = m_placementObjectToPart.find( placementObject );

        if( partIt != m_placementObjectToPart.end() && partIt->second < m_parts.size() )
            m_nets[netIt->second].component_refs.push_back( m_parts[partIt->second].name );
    }
}


void BINARY_PARSER::resolveNetAnchors()
{
    using PIN_KEY = std::pair<std::string, std::string>;

    std::map<PIN_KEY, size_t> pinOwners;

    auto claim = [&]( const NET_ANCHOR& anchor )
    {
        if( anchor.netIndex >= m_nets.size() || anchor.terminalOrdinal == 0 )
            return;

        auto partIt = m_placementObjectToPart.find( anchor.placementObject );

        if( partIt == m_placementObjectToPart.end() || partIt->second >= m_parts.size() )
            return;

        const PART& part = m_parts[partIt->second];
        auto        decalIt = m_decals.find( part.decal );

        if( decalIt == m_decals.end() || anchor.terminalOrdinal > decalIt->second.terminals.size() )
            return;

        const std::string& terminalName = decalIt->second.terminals[anchor.terminalOrdinal - 1].name;

        if( terminalName.empty() )
            return;

        PIN_KEY key = std::make_pair( part.name, terminalName );
        auto [owner, inserted] = pinOwners.emplace( key, anchor.netIndex );

        if( inserted || owner->second == anchor.netIndex )
            return;

        // Zero-edge $$$ placeholders retain a named signal's anchor without owning its pin.
        const bool existingIsAlias = m_nets[owner->second].name.rfind( "$$$", 0 ) == 0;
        const bool incomingIsAlias = m_nets[anchor.netIndex].name.rfind( "$$$", 0 ) == 0;

        if( existingIsAlias && !incomingIsAlias )
            owner->second = anchor.netIndex;
        else if( !existingIsAlias && incomingIsAlias )
            return;
        else if( existingIsAlias && incomingIsAlias )
            return;
        else
            THROW_IO_ERROR( "Conflicting PADS net ownership for pin '" + part.name + "." + terminalName + "'" );
    };

    for( const NET_ANCHOR& endpoint : m_netConnectionEndpoints )
        claim( endpoint );

    for( const NET_ANCHOR& anchor : m_netAnchors )
        claim( anchor );

    for( const auto& [key, netIndex] : pinOwners )
    {
        NET_PIN pin;
        pin.ref_des = key.first;
        pin.pin_name = key.second;
        m_nets[netIndex].pins.push_back( std::move( pin ) );
    }
}


void BINARY_PARSER::parseNetNamesOld()
{
    std::unordered_set<std::string> existing;
    std::unordered_map<std::string, size_t> netIndexByName;

    // Route and via records address nets by a dense ordinal into the serialized net table, not by
    // the stored net ID. Build the index from that table so a via
    // resolves the net PADS wrote: verified against the ASCII exports' own via nets, 2533 of 2533
    // vias across the v0x2021 corpus resolve to the right name, none to a wrong one.
    m_sec23IndexToNet.clear();

    const std::vector<size_t> netOffsets = oldNetRecordOffsets();

    for( size_t i = 0; i < netOffsets.size(); ++i )
    {
        SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( netOffsets[i] ) );
        std::string name = rec.Str( 12, 48 );

        if( name.empty() || !isValidNetName( name ) )
            continue;

        m_sec23IndexToNet[static_cast<uint32_t>( i )] = name;

        if( existing.insert( name ).second )
        {
            NET net;
            net.name = name;
            m_nets.push_back( net );
            netIndexByName.emplace( name, m_nets.size() - 1 );
        }

        m_sec23RecordToNet[static_cast<uint32_t>( i )] = netIndexByName.at( name );

        if( uint32_t owner = rec.U32( 84 ) )
            m_netClassOwner[name] = owner;
    }

    parseNetConnectionsOld();
}


std::vector<size_t> BINARY_PARSER::oldNetRecordOffsets() const
{
    constexpr uint32_t NET_RECORD_SIZE = 144;

    std::vector<size_t> offsets;
    const SDB_SECTION*  nets = getSection( SECTION::Nets );

    if( !nets || nets->stride != NET_RECORD_SIZE )
        THROW_IO_ERROR( "Invalid PADS legacy net-controller framing" );

    const uint64_t base = static_cast<uint64_t>( nets->physicalOffset ) + 20;

    if( base + static_cast<uint64_t>( nets->count ) * NET_RECORD_SIZE > m_data.size() )
        THROW_IO_ERROR( "Invalid PADS legacy net-controller extent" );

    for( size_t i = 0; i < nets->count; ++i )
        offsets.push_back( base + i * NET_RECORD_SIZE );

    return offsets;
}


void BINARY_PARSER::parseNetConnectionsOld()
{
    constexpr size_t   RECORD_SIZE = 68;
    constexpr uint32_t ENDPOINT_FLAG = 0x0000FFFE;
    constexpr uint32_t MARKER = 0xFE000000;
    constexpr uint32_t NET_RECORD_SIZE = 144;
    constexpr uint32_t NET_FIRST = 8; // index of this net's first connection record
    constexpr uint32_t NET_NAME = 12;
    constexpr uint32_t NET_COUNT = 92; // number of connection records in this net

    const SDB_SECTION* connections = getSection( SECTION::Connections );
    const SDB_SECTION* nets = getSection( SECTION::Nets );

    if( !connections || !nets )
        THROW_IO_ERROR( "Missing PADS legacy net-connection controllers" );

    if( connections->count == 0 )
        return;

    if( nets->stride != NET_RECORD_SIZE )
        THROW_IO_ERROR( "Invalid PADS legacy net-controller stride" );

    const uint64_t base = static_cast<uint64_t>( connections->physicalOffset ) + 16;

    if( base + static_cast<uint64_t>( connections->count ) * RECORD_SIZE > m_data.size() )
    {
        THROW_IO_ERROR( "Invalid PADS legacy net-connection extent" );
    }

    const size_t total = connections->count;

    // Union-find over pin identities, so each net becomes one connected component of the graph
    // the connection records span.
    std::map<std::pair<uint32_t, uint32_t>, size_t> pinIndex;
    std::vector<size_t>                             parent;

    auto intern = [&]( uint32_t aObject, uint32_t aOrdinal )
    {
        auto [it, inserted] = pinIndex.emplace( std::make_pair( aObject, aOrdinal ), parent.size() );

        if( inserted )
            parent.push_back( parent.size() );

        return it->second;
    };

    auto findRoot = [&]( size_t aNode )
    {
        while( parent[aNode] != aNode )
        {
            parent[aNode] = parent[parent[aNode]];
            aNode = parent[aNode];
        }

        return aNode;
    };

    struct CONNECTION
    {
        uint32_t objectA;
        uint32_t ordinalA;
        uint32_t objectB;
        uint32_t ordinalB;
        size_t   pinA;
    };

    std::vector<CONNECTION> stream;
    stream.reserve( total );

    for( size_t i = 0; i < total; ++i )
    {
        SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( base + i * RECORD_SIZE ) );

        if( ( rec.U32( 0 ) & 0xFFFFU ) != ENDPOINT_FLAG
            || ( rec.U32( 36 ) & 0xFFFFFFC0U ) != MARKER )
        {
            THROW_IO_ERROR( "Invalid PADS legacy net-connection ring framing" );
        }

        CONNECTION conn{ rec.U32( 8 ), rec.U32( 16 ), rec.U32( 12 ), rec.U32( 20 ), 0 };

        conn.pinA = intern( conn.objectA, conn.ordinalA );
        size_t pinB = intern( conn.objectB, conn.ordinalB );

        size_t rootA = findRoot( conn.pinA );
        size_t rootB = findRoot( pinB );

        if( rootA != rootB )
            parent[rootA] = rootB;

        stream.push_back( conn );
    }

    std::map<size_t, size_t> componentSize;

    for( size_t i = 0; i < stream.size(); ++i )
    {
        size_t root = findRoot( stream[i].pinA );

        ++componentSize[root];
    }

    auto netRecordName = [&]( size_t aOffset, std::string& aName, uint32_t& aCount, uint32_t& aFirst,
                              uint32_t& aAnchorObject, uint32_t& aAnchorOrdinal )
    {
        if( !m_cursor.InBounds( aOffset, NET_RECORD_SIZE ) )
            return false;

        SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( aOffset ) );

        aName = rec.Str( NET_NAME, 48 );
        aCount = rec.U32( NET_COUNT );
        aFirst = rec.U32( NET_FIRST );
        aAnchorObject = rec.U32( 0 );
        aAnchorOrdinal = rec.U32( 4 );

        return !aName.empty() && aCount > 0 && aCount <= connections->count;
    };

    std::map<std::string, size_t> netIndexByName;

    for( size_t i = 0; i < m_nets.size(); ++i )
        netIndexByName.emplace( m_nets[i].name, i );

    std::set<size_t> assignedComponents;

    for( size_t offset : oldNetRecordOffsets() )
    {
        std::string name;
        uint32_t    count = 0;
        uint32_t    edgeIndex = 0;
        uint32_t    anchorObject = 0;
        uint32_t    anchorOrdinal = 0;

        if( !netRecordName( offset, name, count, edgeIndex, anchorObject, anchorOrdinal )
            || !isValidNetName( name ) )
            continue;

        if( edgeIndex >= stream.size() )
            THROW_IO_ERROR( "Invalid PADS legacy net edge reference" );

        auto anchor = pinIndex.find( { anchorObject, anchorOrdinal } );

        if( anchor == pinIndex.end() )
            THROW_IO_ERROR( "Invalid PADS legacy net pin anchor" );

        const size_t root = findRoot( anchor->second );

        if( findRoot( stream[edgeIndex].pinA ) != root )
            THROW_IO_ERROR( "Conflicting PADS legacy net anchors" );

        if( count != componentSize[root] )
            THROW_IO_ERROR( "Invalid PADS legacy net component size" );

        if( !assignedComponents.insert( root ).second )
            THROW_IO_ERROR( "Conflicting PADS legacy net component owners" );

        auto netIt = netIndexByName.find( name );

        if( netIt == netIndexByName.end() )
            THROW_IO_ERROR( "Missing PADS legacy net record" );

        for( const CONNECTION& conn : stream )
        {
            if( findRoot( conn.pinA ) != root )
                continue;

            m_netConnectionEndpoints.push_back( { netIt->second, conn.objectA, conn.ordinalA } );
            m_netConnectionEndpoints.push_back( { netIt->second, conn.objectB, conn.ordinalB } );
        }
    }
}


void BINARY_PARSER::parseNetClasses()
{
    if( m_netClassOwner.empty() || m_data.size() < 24 )
        return;

    // Distinct net-class owner pointers; ascending order is net-class declaration order.
    std::set<uint32_t> ownerSet;

    for( const auto& [name, owner] : m_netClassOwner )
        ownerSet.insert( owner );

    std::vector<uint32_t>      owners( ownerSet.begin(), ownerSet.end() );
    std::map<uint32_t, size_t> ownerOrdinal;

    for( size_t k = 0; k < owners.size(); ++k )
        ownerOrdinal[owners[k]] = k;

    std::vector<NET_CLASS_RULE_EDGE> edges = collectNetClassRuleEdges( ownerSet );

    if( edges.empty() )
        return;

    const size_t NAME_STRIDE = m_version <= 0x2022 ? 28 : 280;
    constexpr size_t NAME_OFFSET = 8;
    const size_t NAME_LEN = m_version <= 0x2022 ? 8 : 48;
    const SDB_SECTION* names = getSection( 66 );

    if( !names || names->physicalCount < owners.size()
        || static_cast<uint64_t>( names->physicalOffset ) + owners.size() * NAME_STRIDE > m_data.size() )
    {
        return;
    }

    m_netClasses.clear();
    m_netClasses.resize( owners.size() );

    for( size_t k = 0; k < owners.size(); ++k )
    {
        std::string name = m_sdb.RecordAt( names->physicalOffset + k * NAME_STRIDE ).Str( NAME_OFFSET, NAME_LEN );

        if( name.empty() || !std::isalnum( static_cast<unsigned char>( name[0] ) ) )
            name = "PADS_NetClass_" + std::to_string( k + 1 );

        m_netClasses[k].name = name;
    }

    for( const auto& [net, owner] : m_netClassOwner )
    {
        auto it = ownerOrdinal.find( owner );

        if( it != ownerOrdinal.end() )
            m_netClasses[it->second].nets.push_back( net );
    }

    applyNetClassClearances( edges, ownerOrdinal );

    // Sort for reproducible output.
    for( BIN_NET_CLASS_DEF& nc : m_netClasses )
    {
        std::sort( nc.nets.begin(), nc.nets.end() );
        std::sort( nc.ruleLayers.begin(), nc.ruleLayers.end() );
        nc.ruleLayers.erase( std::unique( nc.ruleLayers.begin(), nc.ruleLayers.end() ), nc.ruleLayers.end() );
    }
}


std::vector<NET_CLASS_RULE_EDGE> BINARY_PARSER::collectNetClassRuleEdges( const std::set<uint32_t>& aOwnerSet )
{
    constexpr size_t   RECORD_SIZE = 28;
    constexpr size_t   RULE_KIND = 0;
    constexpr size_t   RULE_DETAIL_HANDLE = 4;
    constexpr size_t   SCOPE_TYPE = 8;
    constexpr uint32_t NET_CLASS_SCOPE = 0x42;
    constexpr size_t   SCOPE_REFERENCE = 12;
    constexpr size_t   LAYER = 24;

    std::vector<NET_CLASS_RULE_EDGE> edges;
    const SDB_SECTION*               relationships = getSection( 67 );

    if( !relationships || relationships->physicalBytes != relationships->count * RECORD_SIZE )
        return edges;

    for( uint32_t i = 0; i < relationships->count; ++i )
    {
        size_t     off = relationships->physicalOffset + static_cast<size_t>( i ) * RECORD_SIZE;
        SDB_RECORD rec = m_sdb.RecordAt( off );

        if( rec.U32( SCOPE_TYPE ) != NET_CLASS_SCOPE )
            continue;

        uint32_t owner = rec.U32( SCOPE_REFERENCE );

        if( !aOwnerSet.count( owner ) )
            continue;

        edges.push_back( { owner, rec.U32( RULE_KIND ), rec.U32( RULE_DETAIL_HANDLE ),
                           static_cast<int>( rec.U32( LAYER ) ), off } );
    }

    return edges;
}


void BINARY_PARSER::applyNetClassClearances( const std::vector<NET_CLASS_RULE_EDGE>& aEdges,
                                             const std::map<uint32_t, size_t>&       aOwnerOrdinal )
{
    constexpr uint32_t CLEARANCE_RULE_KIND = 0x29;

    for( const NET_CLASS_RULE_EDGE& e : aEdges )
    {
        if( e.ruleKind != CLEARANCE_RULE_KIND )
            continue;

        auto owner = aOwnerOrdinal.find( e.owner );

        if( owner != aOwnerOrdinal.end() )
            m_netClasses[owner->second].ruleLayers.push_back( e.layer );
    }

    const SDB_SECTION* relationships = getSection( 67 );
    const SDB_SECTION* values = getSection( 41 );

    if( !relationships || !values || values->physicalCount == 0 )
        return;

    constexpr size_t RELATIONSHIP_SIZE = 28;
    uint32_t         firstClearanceHandle = UINT32_MAX;

    for( uint32_t i = 0; i < relationships->count; ++i )
    {
        SDB_RECORD rec = m_sdb.RecordAt( relationships->physicalOffset + i * RELATIONSHIP_SIZE );

        if( rec.U32( 0 ) == CLEARANCE_RULE_KIND )
            firstClearanceHandle = std::min( firstClearanceHandle, rec.U32( 4 ) );
    }

    if( firstClearanceHandle == UINT32_MAX )
        return;

    const uint32_t valueStride = m_version == 0x2017 ? 180 : 188;

    for( const NET_CLASS_RULE_EDGE& edge : aEdges )
    {
        if( edge.ruleKind != CLEARANCE_RULE_KIND || edge.layer != 0 || edge.rulePtr < firstClearanceHandle )
            continue;

        uint32_t delta = edge.rulePtr - firstClearanceHandle;

        if( delta % valueStride != 0 )
            continue;

        uint32_t index = delta / valueStride;

        if( index >= values->physicalCount )
            continue;

        auto owner = aOwnerOrdinal.find( edge.owner );

        if( owner == aOwnerOrdinal.end() )
            continue;

        SDB_RECORD value = m_sdb.RecordAt( values->physicalOffset + index * valueStride );
        BIN_NET_CLASS_DEF& netClass = m_netClasses[owner->second];

        netClass.clearance = value.I32( 12 );
        netClass.viaClearance = value.I32( 20 );
        netClass.minTrackWidth = value.I32( 144 );
        netClass.trackWidth = value.I32( 148 );
        netClass.maxTrackWidth = value.I32( 152 );
        netClass.hasRuleValues = true;
    }
}


void BINARY_PARSER::parseDiffPairs()
{
    constexpr size_t  OBJECT_SIZE = 864;
    constexpr double  F64_INHERIT = -1.0;
    constexpr int32_t I32_INHERIT = -1;

    // Field offsets within the 864-byte DIF_PAIR object.
    constexpr size_t NET_A_HANDLE = 12;    // member-net A; equals a net record's +184 self-handle
    constexpr size_t NET_B_HANDLE = 16;    // member-net B
    constexpr size_t GAP_INHERIT = 40;     // f64 gap, used when the override is inherited
    constexpr size_t GAP_OVERRIDE = 56;    // f64 gap override
    constexpr size_t WIDTH_INHERIT = 592;  // i32 width, used when the override is inherited
    constexpr size_t WIDTH_OVERRIDE = 600; // i32 width override

    const SDB_SECTION* records = getSection( 48 );

    if( !records )
        THROW_IO_ERROR( "Missing PADS differential-pair controller" );

    if( records->physicalCount == 0 || m_netSelfPtrToName.empty() )
        return;

    if( records->physicalBytes != records->physicalCount * OBJECT_SIZE || records->physicalOffset < 8 )
        THROW_IO_ERROR( "Invalid PADS differential-pair framing" );

    const size_t base = records->physicalOffset - 8;

    if( static_cast<uint64_t>( base ) + static_cast<uint64_t>( records->physicalCount ) * OBJECT_SIZE > m_data.size() )
        THROW_IO_ERROR( "Invalid PADS differential-pair extent" );

    std::set<std::pair<std::string, std::string>> seen;

    for( uint32_t i = 0; i < records->physicalCount; ++i )
    {
        size_t             objStart = base + static_cast<size_t>( i ) * OBJECT_SIZE;
        SDB_RECORD         obj = m_sdb.RecordAt( objStart );

        if( !m_netSelfPtrToName.count( obj.U32( NET_A_HANDLE ) )
            || !m_netSelfPtrToName.count( obj.U32( NET_B_HANDLE ) ) )
        {
            continue;
        }

        const std::string& nameA = m_netSelfPtrToName.at( obj.U32( NET_A_HANDLE ) );
        const std::string& nameB = m_netSelfPtrToName.at( obj.U32( NET_B_HANDLE ) );

        if( !seen.insert( { nameA, nameB } ).second )
            continue;

        double  gapOverride = obj.F64( GAP_OVERRIDE );
        double  gap = ( gapOverride != F64_INHERIT ) ? gapOverride : obj.F64( GAP_INHERIT );
        int32_t widthOverride = obj.I32( WIDTH_OVERRIDE );
        double  width = ( widthOverride != I32_INHERIT ) ? static_cast<double>( widthOverride )
                                                         : static_cast<double>( obj.I32( WIDTH_INHERIT ) );

        DIFF_PAIR_DEF dp;
        dp.name = nameA + "_" + nameB;
        dp.positive_net = nameA;
        dp.negative_net = nameB;
        dp.gap = ( gap != F64_INHERIT ) ? gap : 0.0;
        dp.width = ( width != static_cast<double>( I32_INHERIT ) ) ? width : 0.0;

        m_diffPairs.push_back( std::move( dp ) );
    }
}


void BINARY_PARSER::parseTextRecords()
{
    const SDB_SECTION* s8 = getSection( SECTION::FreeText );
    const SDB_SECTION* s9 = getSection( SECTION::StringPool );

    constexpr size_t GEOMETRY_BYTES = 36;
    const size_t     recordSize = s8 ? s8->stride : 0;
    const size_t     ringRotation = recordSize >= GEOMETRY_BYTES ? recordSize - GEOMETRY_BYTES : 0;

    if( !s8 || !s9 )
        THROW_IO_ERROR( "Missing PADS text controllers" );

    if( s8->physicalCount == 0 )
        return;

    const bool validStringPool = ( s9->physicalBytes == 0 && s9->count == 0 )
                                 || ( s9->stride == 1 && s9->physicalBytes == s9->count );

    if( ( recordSize != 64 && recordSize != 72 ) || s8->physicalOffset < ringRotation || !validStringPool
        || s9->physicalOffset != s8->physicalOffset + s8->physicalBytes )
        THROW_IO_ERROR( "Invalid PADS text-controller framing" );

    const size_t recordBase = s8->physicalOffset - ringRotation;
    const size_t poolBase = s9->physicalOffset;
    const size_t poolHi = poolBase + s9->physicalBytes;

    // The final record's lagged metadata occupies the 36-byte circular-controller tail ending
    // at section 9. Only fields through +28 are consumed from it.
    if( !m_cursor.InBounds( recordBase, s8->physicalBytes + ringRotation )
        || !m_cursor.InBounds( poolBase, s9->physicalBytes ) )
    {
        THROW_IO_ERROR( "Invalid PADS text-controller extent" );
    }

    auto cStringStartAt = [this, poolHi]( size_t aAbs ) -> bool
    {
        if( aAbs >= poolHi )
            return false;

        size_t e = aAbs;

        while( e < poolHi && m_data[e] != 0 )
        {
            if( m_data[e] < 0x20 || m_data[e] >= 0x7F )
                return false;

            ++e;
        }

        return e > aAbs;
    };

    auto fieldAttribute = [&]( uint32_t aIndex )
    {
        SDB_RECORD record = m_sdb.RecordAt( recordBase + static_cast<size_t>( aIndex ) * recordSize );
        ATTRIBUTE  attribute;
        attribute.height = record.I32( ringRotation );
        attribute.width = record.I32( ringRotation + 4 );
        attribute.x = record.I32( ringRotation + 8 );
        attribute.y = record.I32( ringRotation + 12 );
        attribute.orientation = toBasicAngle( record.I32( ringRotation + 16 ) );
        attribute.mirrored = record.I32( ringRotation + 20 ) != 0;
        return attribute;
    };

    for( const auto& [partIndex, fieldStart] : m_partFieldStart )
    {
        if( fieldStart < 0 )
            continue;

        if( partIndex >= m_parts.size() || static_cast<uint32_t>( fieldStart ) >= s8->physicalCount )
            THROW_IO_ERROR( "Invalid PADS placement field-presentation link" );

        std::vector<ATTRIBUTE> attributes;
        std::set<uint32_t>     visited;
        uint32_t               fieldIndex = static_cast<uint32_t>( fieldStart );

        while( true )
        {
            if( !visited.insert( fieldIndex ).second )
                THROW_IO_ERROR( "Cyclic PADS placement field-list link" );

            if( fieldIndex >= s8->physicalCount )
                THROW_IO_ERROR( "Invalid PADS placement field-list link" );

            ATTRIBUTE  attribute = fieldAttribute( fieldIndex );
            SDB_RECORD metadata = m_sdb.RecordAt( recordBase + static_cast<size_t>( fieldIndex + 1 ) * recordSize );
            uint32_t   presentation = metadata.U32( 24 );
            uint8_t    fieldKind = static_cast<uint8_t>( presentation >> 16 );
            attribute.visible = ( fieldKind & 0x20 ) != 0;

            if( ( presentation & 0x11000000U ) == 0x11000000U )
            {
                attribute.hjust = "CENTER";
                attribute.vjust = "CENTER";
            }
            else
            {
                attribute.hjust = "LEFT";
                attribute.vjust = ( presentation & 0x20000000U ) ? "UP" : "DOWN";
            }

            if( ( fieldKind & 0x1FU ) == 0x03 )
                attribute.name = "Part Type";
            else if( ( fieldKind & 0x1FU ) == 0x02 )
                attribute.name = "Ref.Des.";

            if( !attribute.name.empty() )
                attributes.push_back( std::move( attribute ) );

            uint32_t association = metadata.U32( 12 );

            uint8_t associationKind = static_cast<uint8_t>( association >> 24 );

            if( associationKind == 0x16 )
            {
                if( ( association & 0x00FFFFFFU ) != partIndex )
                    THROW_IO_ERROR( "Mismatched PADS placement field-list owner" );

                break;
            }

            if( associationKind != 0x08 )
                THROW_IO_ERROR( "Invalid PADS placement field-list terminator" );

            fieldIndex = association & 0x00FFFFFFU;
        }

        m_parts[partIndex].attributes = std::move( attributes );
    }

    for( uint32_t index = 0; index < s8->physicalCount; ++index )
    {
        SDB_RECORD geom = m_sdb.RecordAt( recordBase + static_cast<size_t>( index ) * recordSize );
        SDB_RECORD meta = m_sdb.RecordAt( recordBase + static_cast<size_t>( index + 1 ) * recordSize );

        if( meta.U16( 4 ) != 0xFFFE || meta.U32( 12 ) != 0 || ( recordSize == 72 && meta.U32( 28 ) != 0x49000000 ) )
            continue;

        uint32_t layerWord = meta.U32( 24 );

        if( ( layerWord >> 16 ) != 0x0020 || geom.I32( ringRotation ) <= 0 || geom.I32( ringRotation + 4 ) <= 0 )
            continue;

        size_t soff = poolBase + meta.U32( 8 );

        if( soff < poolBase || soff >= poolHi || ( soff != poolBase && m_data[soff - 1] != 0 )
            || !cStringStartAt( soff ) )
        {
            continue;
        }

        std::string content = m_sdb.RecordAt( soff ).Str( 0, poolHi - soff );

        if( content.empty() )
            continue;

        int32_t    height = geom.I32( ringRotation );
        int32_t    linewidth = geom.I32( ringRotation + 4 );
        int32_t    x = geom.I32( ringRotation + 8 );
        int32_t    y = geom.I32( ringRotation + 12 );
        int32_t    angleRaw = geom.I32( ringRotation + 16 );

        TEXT text;
        text.content = content;
        text.location.x = toBasicCoordX( x );
        text.location.y = toBasicCoordY( y );
        text.height = static_cast<double>( height );
        text.width = static_cast<double>( linewidth );
        text.layer = static_cast<int>( layerWord & 0xFF );
        text.rotation = toBasicAngle( angleRaw );

        m_texts.push_back( text );
    }
}


// Section 60 is a heterogeneous node table (via records, route corner nodes, free slots),
// discriminated by a type byte four bytes from the end of its circular fixed-stride record.
// Every other field is defined relative to that byte T and is stride-independent:
//   T-27  i32 raw X                    T-23  i32 raw Y
//   T+0   u8  type (0x0E = via)        T+1   u16 net index, biased by 3 (see parseNetNamesNew)
//   T+5   u8  saved start layer         T+6   u8  saved end layer
// The layer numbers are stored in the low five bits of the following logical record's head.
// Valid, unequal in-range endpoints distinguish vias from other 0x0E junctions.
// The logical ring begins stride-31 bytes before the direct physical section-60 extent. Verified
// against the compiled Kaitai view on all 597 distinct corpus binaries.
//
// The T+1 net index is real and exact for vias. Grouping vias by its raw value reproduces the
// true per-net partition at 100% purity on BR350430B (42 vias) and OC_LTE_BASEBAND (3943),
// against a 7-9% shuffled-value control, so the field identifies the net even where the name
// lookup cannot. Only the 0-2 slots, whose nets live outside section 23, fail to resolve.
//
// The other row types are a different matter. No u16 field anywhere in the record resolves a net
// for the 0x16 or 0x00 rows above 4.2%, against a 0.14% random-assignment control and a
// section-23 name table independently verified as 979 of 980 real net names, so per-node net
// membership for non-via nodes is not in this table and has to come from somewhere else. The
// section-23 self-pointer route is untested rather than negative -- only 73 of 983 records carry
// that field on the file it was tried on.
static constexpr int VIA_TYPE = 0x0E;


static bool viaRecordValid( const BINARY_CURSOR& aCur, size_t aTypeByte )
{
    return aCur.U8At( aTypeByte ) == VIA_TYPE && aCur.U8At( aTypeByte + 4 ) == 0x17
           && ( aCur.U8At( aTypeByte + 5 ) & 0x02 ) != 0;
}


std::map<uint32_t, size_t> BINARY_PARSER::parseRouteJunctionNets() const
{
    constexpr uint32_t JUNCTION_TAG = 0x3C000000;
    constexpr uint32_t ROUTE_CHAIN_TAG = 0x18000000;
    constexpr uint32_t TAG_MASK = 0xFF000000;
    constexpr uint32_t INDEX_MASK = 0x00FFFFFF;

    const SDB_SECTION* relationships = getSection( 49 );
    const SDB_SECTION* junctions = getSection( SECTION::Vias );
    const SDB_SECTION* nets = getSection( SECTION::Nets );
    const SDB_SECTION* routeChains = getSection( SECTION::Connections );

    if( !relationships || !junctions || !nets || !routeChains )
        THROW_IO_ERROR( "Missing PADS route-relationship controllers" );

    std::map<uint32_t, size_t> result;
    std::vector<std::pair<uint32_t, size_t>> junctionSignals;
    size_t                     cursor = relationships->physicalOffset;
    const size_t               end = cursor + relationships->physicalBytes;
    size_t                     signalIndex = 0;

    auto readU32 = [&]()
    {
        if( cursor > end || end - cursor < 4 )
            THROW_IO_ERROR( "Invalid PADS route-relationship extent" );

        uint32_t value = m_cursor.U32At( cursor );
        cursor += 4;
        return value;
    };

    while( cursor < end )
    {
        if( signalIndex >= nets->count )
            THROW_IO_ERROR( "Invalid PADS route-relationship signal count" );

        for( int direction = 0; direction < 2; ++direction )
        {
            uint32_t numRelationships = readU32();

            for( uint32_t relationship = 0; relationship < numRelationships; ++relationship )
            {
                uint32_t objectId = readU32();
                uint32_t numValues = readU32();

                if( numValues > ( end - cursor ) / 4 )
                    THROW_IO_ERROR( "Invalid PADS route-relationship value count" );

                uint32_t expectedObjectTag = direction == 0 ? JUNCTION_TAG : ROUTE_CHAIN_TAG;
                uint32_t objectIndex = objectId & INDEX_MASK;
                uint32_t objectLimit = direction == 0 ? junctions->count : routeChains->count;

                if( ( objectId & TAG_MASK ) != expectedObjectTag || objectIndex >= objectLimit )
                    THROW_IO_ERROR( "Invalid PADS route-relationship object identifier" );

                if( direction == 0 )
                {
                    junctionSignals.emplace_back( objectIndex, signalIndex );
                }

                uint32_t expectedValueTag = direction == 0 ? ROUTE_CHAIN_TAG : JUNCTION_TAG;
                uint32_t valueLimit = direction == 0 ? routeChains->count : junctions->count;

                for( uint32_t valueIndex = 0; valueIndex < numValues; ++valueIndex )
                {
                    uint32_t value = readU32();

                    if( ( value & TAG_MASK ) != expectedValueTag || ( value & INDEX_MASK ) >= valueLimit )
                        THROW_IO_ERROR( "Invalid PADS route-relationship member identifier" );
                }
            }
        }

        ++signalIndex;
    }

    if( cursor != end || ( signalIndex != m_nets.size() && signalIndex != nets->count ) )
        THROW_IO_ERROR( "Invalid PADS route-relationship framing" );

    for( const auto& [junctionIndex, sourceSignalIndex] : junctionSignals )
    {
        std::optional<size_t> netIndex;

        if( signalIndex == nets->count )
        {
            auto netIt = m_sec23RecordToNet.find( static_cast<uint32_t>( sourceSignalIndex ) );

            if( netIt != m_sec23RecordToNet.end() )
                netIndex = netIt->second;
        }
        else
        {
            netIndex = sourceSignalIndex;
        }

        if( !netIndex )
            continue;

        auto [it, inserted] = result.emplace( junctionIndex, *netIndex );

        if( !inserted && it->second != *netIndex )
            THROW_IO_ERROR( "Conflicting PADS route-junction relationship" );
    }

    return result;
}


void BINARY_PARSER::parseRouteVertices()
{
    // Section 60 identifies via instances and their net/via-definition ordinals. Sections
    // 62-64 store routed copper as 48-byte object descriptors, a layer table, and compressed
    // 12-byte geometry cells; a separate 32-byte header arena carries width, layer, and links.
    struct ViaLocation
    {
        int32_t     x = 0;
        int32_t     y = 0;
        std::string netName;
        int         viaIndex = -1;
        bool        relationshipNet = false;
    };

    std::vector<ViaLocation> viaLocations;

    const SDB_SECTION* entry60 = getSection( SECTION::Vias );

    if( !entry60 )
        THROW_IO_ERROR( "Missing PADS via controller" );

    if( entry60->count == 0 )
        return;

    if( entry60->stride == 0 )
        THROW_IO_ERROR( "Invalid PADS via-controller stride" );

    uint32_t stride = entry60->stride;

    if( stride < 31 || entry60->physicalCount != entry60->count )
        THROW_IO_ERROR( "Invalid PADS via-controller framing" );

    // Section 60 is a ring rotated left to its X field. Its logical record-grid
    // origin is therefore stride-31 bytes before the directly framed physical range.
    std::vector<size_t> typeBytes;
    const size_t        ringRotation = stride - 31;

    if( entry60->physicalOffset >= ringRotation
        && m_cursor.InBounds( entry60->physicalOffset, entry60->physicalBytes ) )
    {
        const size_t base = entry60->physicalOffset - ringRotation;

        for( uint32_t rec = 0; rec < entry60->physicalCount; ++rec )
            typeBytes.push_back( base + static_cast<size_t>( rec ) * stride + stride - 4 );

        logResolvedBase( 60, "physicalRing", base, entry60->physicalOffset );
    }

    // A nonempty controller must have one directly framed physical record per directory item.
    if( typeBytes.empty() )
        THROW_IO_ERROR( "Invalid PADS via-controller extent" );

    const std::map<uint32_t, size_t> junctionNets = parseRouteJunctionNets();

    std::map<uint32_t, std::set<size_t>> junctionHandleNets;

    for( size_t junction = 0; junction < typeBytes.size(); ++junction )
    {
        auto netIt = junctionNets.find( static_cast<uint32_t>( junction ) );

        if( netIt == junctionNets.end() )
            continue;

        if( netIt->second >= m_nets.size() || typeBytes[junction] < 27 )
            THROW_IO_ERROR( "Invalid PADS route-junction net mapping" );

        const size_t typeByte = typeBytes[junction];
        uint32_t     objectHandle = m_cursor.U32At( typeByte - 19 );

        if( objectHandle != 0 )
            junctionHandleNets[objectHandle].insert( netIt->second );
    }

    auto _pt_emitPass = std::chrono::steady_clock::now();
    for( size_t junction = 0; junction < typeBytes.size(); ++junction )
    {
        size_t  typeByte = typeBytes[junction];
        uint8_t type = m_cursor.U8At( typeByte );
        bool    isVia = type == VIA_TYPE && viaRecordValid( m_cursor, typeByte );

        if( !isVia )
            continue;

        if( typeByte < 27 || !m_cursor.InBounds( typeByte - 27, 8 ) )
            continue;

        int32_t vx = m_cursor.I32At( typeByte - 27 );
        int32_t vy = m_cursor.I32At( typeByte - 23 );

        std::string netName;
        uint32_t    netIdx = m_cursor.U16At( typeByte + 1 );
        auto        it = m_sec23IndexToNet.find( netIdx );
        auto        relationshipIt = junctionNets.find( static_cast<uint32_t>( junction ) );

        if( relationshipIt != junctionNets.end() && relationshipIt->second < m_nets.size() )
        {
            netName = m_nets[relationshipIt->second].name;
        }
        else if( it != m_sec23IndexToNet.end() )
        {
            netName = it->second;
        }

        ViaLocation via;
        via.x = vx;
        via.y = vy;
        via.netName = netName;
        via.viaIndex = m_cursor.U8At( typeByte - 3 );
        via.relationshipNet = relationshipIt != junctionNets.end();
        viaLocations.push_back( via );
    }

    std::map<std::pair<int32_t, int32_t>, ViaLocation> uniqueVias;

    for( const ViaLocation& via : viaLocations )
    {
        auto [it, inserted] = uniqueVias.emplace( std::make_pair( via.x, via.y ), via );

        if( inserted )
            continue;

        ViaLocation& existing = it->second;

        if( existing.viaIndex != via.viaIndex )
            THROW_IO_ERROR( "Conflicting co-located PADS via definitions" );

        if( via.relationshipNet )
        {
            if( existing.relationshipNet && existing.netName != via.netName )
                THROW_IO_ERROR( "Conflicting co-located PADS via relationships" );

            existing.netName = via.netName;
            existing.relationshipNet = true;
        }
        else if( !existing.relationshipNet && !via.netName.empty() )
        {
            if( !existing.netName.empty() && existing.netName != via.netName )
                THROW_IO_ERROR( "Conflicting co-located PADS via net state" );

            existing.netName = via.netName;
        }
    }

    viaLocations.clear();
    viaLocations.reserve( uniqueVias.size() );

    for( auto& [coordinate, via] : uniqueVias )
        viaLocations.push_back( std::move( via ) );

    logParsePhase( "emitPass", _pt_emitPass );

    std::map<std::string, ROUTE> routes;

    for( const ViaLocation& via : viaLocations )
    {
        ROUTE& route = routes[via.netName];
        route.net_name = via.netName;

        VIA viaDef;
        viaDef.location.x = static_cast<double>( via.x );
        viaDef.location.y = static_cast<double>( via.y );
        viaDef.start_layer = 1;
        viaDef.end_layer = m_parameters.layer_count;

        if( via.viaIndex >= 0 && static_cast<size_t>( via.viaIndex ) < m_decalNameTable.size() )
        {
            auto decalIt = m_decals.find( m_decalNameTable[via.viaIndex] );

            if( decalIt != m_decals.end() )
            {
                auto stackIt = decalIt->second.pad_stacks.find( 1 );

                if( stackIt == decalIt->second.pad_stacks.end() )
                    stackIt = decalIt->second.pad_stacks.find( 0 );

                if( stackIt != decalIt->second.pad_stacks.end() && !stackIt->second.empty() )
                {
                    viaDef.stack = stackIt->second;

                    auto spanIt = decalIt->second.drill_spans.find( stackIt->first );

                    if( spanIt != decalIt->second.drill_spans.end() && spanIt->second.first > 0
                        && spanIt->second.second > 0 )
                    {
                        // The pair is stored in either order and is not bounded by the layer
                        // count, so normalize and range-check it here the way the ASCII path
                        // does rather than let a stale byte surface as an unusable via span
                        int spanStart = std::min( spanIt->second.first, spanIt->second.second );
                        int spanEnd = std::max( spanIt->second.first, spanIt->second.second );

                        if( spanEnd <= m_parameters.layer_count )
                        {
                            viaDef.start_layer = spanStart;
                            viaDef.end_layer = spanEnd;
                        }
                    }
                }
            }
        }

        if( viaDef.stack.empty() )
        {
            const std::string name = via.viaIndex >= 0 && static_cast<size_t>( via.viaIndex ) < m_decalNameTable.size()
                                             ? m_decalNameTable[via.viaIndex]
                                             : std::string( "<out-of-range>" );
            THROW_IO_ERROR(
                    wxString::Format( "Invalid PADS via padstack reference %d (%s)", via.viaIndex, name.c_str() ) );
        }

        route.vias.push_back( std::move( viaDef ) );
    }

    const SDB_SECTION* routeObjects = getSection( SECTION::RouteObjects );
    const SDB_SECTION* routeLayers = getSection( SECTION::RouteLayers );
    const SDB_SECTION* routeCells = getSection( SECTION::RouteCells );

    if( !routeObjects || !routeLayers || !routeCells )
        THROW_IO_ERROR( "Missing PADS route controllers" );

    if( routeLayers->count == 0
        || static_cast<uint64_t>( routeLayers->count ) * 2 != routeLayers->totalBytes )
    {
        THROW_IO_ERROR( "Invalid PADS route-layer controller extent" );
    }

    if( static_cast<uint64_t>( routeCells->count ) * 12 != routeCells->totalBytes )
        THROW_IO_ERROR( "Invalid PADS route-cell controller extent" );

    if( routeObjects->count == 0 )
    {
        if( routeObjects->totalBytes != 0 || routeCells->count != 0 )
            THROW_IO_ERROR( "Invalid empty PADS route-object controller" );

        return;
    }

    if( ( routeObjects->stride != 36 && routeObjects->stride != 48 )
        || static_cast<uint64_t>( routeObjects->count ) * routeObjects->stride != routeObjects->totalBytes )
    {
        THROW_IO_ERROR( "Invalid PADS route-object controller extent" );
    }

    if( routeObjects->count > 0 )
    {
        struct ROUTE_OBJECT
        {
            int32_t  width = 0;
            uint32_t style = 0;
            uint32_t cellCount = 0;
        };

        size_t           objectBytes = routeObjects->physicalBytes;
        size_t           layerTable = routeLayers->physicalOffset;
        std::vector<int> serializedLayerOrder;
        bool             legacyObjects = routeObjects->stride == 36;
        size_t           objectStride = legacyObjects ? 36 : 48;
        size_t           widthOffset = legacyObjects ? 8 : 20;
        size_t           cellCountOffset = legacyObjects ? 24 : 36;

        auto ringU32 = [&]( size_t aRingStart, size_t aOffset )
        {
            uint32_t value = 0;

            for( size_t byte = 0; byte < 4; ++byte )
            {
                size_t physical = aRingStart + ( aOffset + byte ) % objectBytes;
                value |= static_cast<uint32_t>( m_cursor.U8At( physical ) ) << ( byte * 8 );
            }

            return value;
        };

        std::vector<bool> seenLayers( routeLayers->physicalCount, false );
        serializedLayerOrder.reserve( routeLayers->physicalCount );

        for( uint32_t layer = 0; layer < routeLayers->physicalCount; ++layer )
        {
            uint16_t serializedLayer = m_cursor.U16At( layerTable + static_cast<size_t>( layer ) * 2 );

            if( serializedLayer >= routeLayers->physicalCount || seenLayers[serializedLayer] )
                THROW_IO_ERROR( "Invalid PADS route-layer permutation" );

            seenLayers[serializedLayer] = true;
            serializedLayerOrder.push_back( serializedLayer );
        }

        if( layerTable != 0 )
        {
            auto padsLayerForSerializedIndex = [&]( size_t aIndex )
            {
                return aIndex < serializedLayerOrder.size() ? serializedLayerOrder[aIndex] + 1
                                                            : static_cast<int>( aIndex ) + 1;
            };

            size_t                    ringStart = routeObjects->physicalOffset;
            std::vector<ROUTE_OBJECT> objects;

            for( uint32_t i = 0; i < routeObjects->physicalCount; ++i )
            {
                size_t       base = ( 32 + static_cast<size_t>( i ) * objectStride ) % objectBytes;
                ROUTE_OBJECT object;

                object.width = static_cast<int32_t>( ringU32( ringStart, base + widthOffset ) ) * 4;
                object.style = ringU32( ringStart, base + ( legacyObjects ? 20 : 32 ) );
                object.cellCount = ringU32( ringStart, base + cellCountOffset );
                objects.push_back( object );
            }

            struct ROUTE_CELL
            {
                int32_t x1 = 0;
                int32_t y = 0;
                int32_t x2 = 0;
            };

            size_t                  cellStart = routeCells->physicalOffset;
            std::vector<ROUTE_CELL> cells;

            for( uint32_t i = 0; i < routeCells->count; ++i )
            {
                size_t  base = cellStart + static_cast<size_t>( i ) * 12;
                int32_t first = m_cursor.I32At( base );
                int32_t second = m_cursor.I32At( base + 4 );
                int32_t third = m_cursor.I32At( base + 8 );

                cells.push_back( { first, second, third } );
            }

            size_t             cursor = 0;
            struct DECODED_TRACK
            {
                TRACK       track;
                std::string netName;
            };

            std::vector<DECODED_TRACK> decodedPieces;
            std::vector<int>   objectLayers( objects.size() );
            std::vector<uint32_t> objectHandles( objects.size() );

            const SDB_SECTION* routeController = getSection( 25 );
            const SDB_SECTION* allocatorDescriptors = getSection( 26 );
            const SDB_SECTION* layerObjectCounts = getSection( 27 );
            const SDB_SECTION* layerObjectHandles = getSection( 29 );
            const SDB_SECTION* routeNodes = getSection( 61 );

            if( !routeController || !allocatorDescriptors || !layerObjectCounts || !layerObjectHandles
                || !routeNodes || !m_cursor.InBounds( routeController->physicalOffset + 180, 8 )
                || layerObjectCounts->physicalCount != routeLayers->physicalCount )
            {
                THROW_IO_ERROR( "Invalid PADS route allocator controllers" );
            }

            std::array<uint16_t, 4> allocatorPageCounts;

            for( size_t group = 0; group < allocatorPageCounts.size(); ++group )
                allocatorPageCounts[group] = m_cursor.U16At( routeController->physicalOffset + 180 + group * 2 );

            size_t nodePageStart = static_cast<size_t>( allocatorPageCounts[0] ) + allocatorPageCounts[1]
                                   + allocatorPageCounts[2];
            size_t nodePageCount = allocatorPageCounts[3];

            if( nodePageCount == 0 || nodePageStart + nodePageCount > allocatorDescriptors->physicalCount )
                THROW_IO_ERROR( "Invalid PADS route node page group" );

            struct NODE_PAGE
            {
                uint32_t base = 0;
                uint32_t liveCount = 0;
                size_t   firstOrdinal = 0;
            };

            std::vector<NODE_PAGE> nodePages;
            size_t                 allocatedNodes = 0;

            for( size_t page = 0; page < nodePageCount; ++page )
            {
                size_t descriptor = allocatorDescriptors->physicalOffset + ( nodePageStart + page ) * 12;
                uint32_t liveCount;

                if( page + 1 < nodePageCount )
                    liveCount = m_cursor.U32At( descriptor + 20 );
                else if( allocatedNodes <= routeNodes->physicalCount )
                    liveCount = routeNodes->physicalCount - allocatedNodes;
                else
                    THROW_IO_ERROR( "Invalid PADS route node page counts" );

                nodePages.push_back( { m_cursor.U32At( descriptor ), liveCount, allocatedNodes } );
                allocatedNodes += liveCount;
            }

            if( allocatedNodes != routeNodes->physicalCount )
                THROW_IO_ERROR( "Invalid PADS route node extent" );

            std::vector<uint32_t> routeNodeHandles;
            routeNodeHandles.reserve( routeNodes->physicalCount );

            for( const NODE_PAGE& page : nodePages )
            {
                for( uint32_t index = 0; index < page.liveCount; ++index )
                    routeNodeHandles.push_back( page.base + index * 56 );
            }

            std::map<uint32_t, size_t> routeNodeOrdinals;

            for( size_t ordinal = 0; ordinal < routeNodeHandles.size(); ++ordinal )
                routeNodeOrdinals.emplace( routeNodeHandles[ordinal], ordinal );

            std::vector<std::vector<size_t>> routeNodeLinks( routeNodeHandles.size() );

            for( size_t ordinal = 0; ordinal < routeNodeHandles.size(); ++ordinal )
            {
                size_t nodeOffset = routeNodes->physicalOffset + ordinal * 12;

                for( uint32_t linkedHandle : { m_cursor.U32At( nodeOffset ), m_cursor.U32At( nodeOffset + 4 ) } )
                {
                    auto linkedIt = routeNodeOrdinals.find( linkedHandle );

                    if( linkedIt == routeNodeOrdinals.end() )
                        continue;

                    routeNodeLinks[linkedIt->second].push_back( ordinal );
                }
            }

            std::map<uint32_t, std::set<size_t>> routeNodeNets;

            for( size_t root = 0; root < routeNodeHandles.size(); ++root )
            {
                std::set<size_t> componentNets;
                std::set<size_t> visited{ root };
                std::vector<size_t> frontier{ root };

                while( !frontier.empty() && componentNets.empty() )
                {
                    std::vector<size_t> next;

                    for( size_t ordinal : frontier )
                    {
                        auto netIt = junctionHandleNets.find( routeNodeHandles[ordinal] );

                        if( netIt != junctionHandleNets.end() )
                            componentNets.insert( netIt->second.begin(), netIt->second.end() );

                        for( size_t linked : routeNodeLinks[ordinal] )
                        {
                            if( visited.insert( linked ).second )
                                next.push_back( linked );
                        }
                    }

                    frontier = std::move( next );
                }

                routeNodeNets.emplace( routeNodeHandles[root], std::move( componentNets ) );
            }

            auto nodeOrdinal = [&]( uint32_t aHandle ) -> std::optional<size_t>
            {
                auto it = routeNodeOrdinals.find( aHandle );

                if( it != routeNodeOrdinals.end() )
                    return it->second;

                return std::nullopt;
            };

            std::vector<int> serializedObjectLayers;
            std::vector<uint32_t> serializedObjectHandles;
            std::map<uint32_t, int> routeHandleLayers;
            size_t           handleOrdinal = 0;

            for( size_t layer = 0; layer < layerObjectCounts->physicalCount; ++layer )
            {
                uint32_t count = m_cursor.U32At( layerObjectCounts->physicalOffset + layer * 4 );

                if( handleOrdinal + count > layerObjectHandles->physicalCount )
                    THROW_IO_ERROR( "Invalid PADS per-layer route handle counts" );

                for( size_t index = 0; index < count; ++index, ++handleOrdinal )
                {
                    uint32_t handle = m_cursor.U32At( layerObjectHandles->physicalOffset + handleOrdinal * 4 );

                    if( handle == 0 )
                        continue;

                    int padsLayer = padsLayerForSerializedIndex( layer );
                    auto [layerIt, inserted] = routeHandleLayers.emplace( handle, padsLayer );

                    if( !inserted && layerIt->second != padsLayer )
                        THROW_IO_ERROR( "Conflicting PADS route-handle layers" );

                    auto     ordinal = nodeOrdinal( handle );

                    if( !ordinal )
                        THROW_IO_ERROR( "Invalid PADS route node handle" );

                    uint32_t classTag = m_cursor.U32At( routeNodes->physicalOffset + *ordinal * 12 + 8 );

                    if( ( classTag & 0x00800000 ) != 0 )
                    {
                        serializedObjectLayers.push_back( padsLayer );
                        serializedObjectHandles.push_back( handle );
                    }
                }
            }

            if( handleOrdinal != layerObjectHandles->physicalCount
                || serializedObjectLayers.size() != objects.size() )
            {
                THROW_IO_ERROR( "Invalid PADS route object-node mapping" );
            }

            for( size_t sequence = 0; sequence < serializedObjectLayers.size(); ++sequence )
            {
                size_t object = ( sequence + objects.size() - 1 ) % objects.size();
                objectLayers[object] = serializedObjectLayers[sequence];
                objectHandles[object] = serializedObjectHandles[sequence];
            }

            struct ROUTE_CHUNK
            {
                size_t objectIndex = 0;
                size_t cellStart = 0;
            };

            std::vector<ROUTE_CHUNK> chunks;

            // The object array is a ring whose logical base is 32 bytes into its physical
            // storage. Its last descriptor therefore owns the first cell chunk, followed by
            // descriptors 0..N-2. The descriptor cell counts partition the cell stream exactly.
            for( size_t sequence = 0; sequence < objects.size(); ++sequence )
            {
                size_t match = ( sequence + objects.size() - 1 ) % objects.size();

                if( cursor + objects[match].cellCount > cells.size() )
                    THROW_IO_ERROR( "Invalid PADS route-cell partition" );

                const ROUTE_OBJECT& object = objects[match];

                // 0x100 and 0x1000 are the serialized jumper and via/special bits used by
                // the PADS ROUTE writer. Their cells belong to those auxiliary objects, not
                // ordinary routed-copper polylines.
                if( object.width > 0 && ( object.style & 0x1100 ) == 0 )
                    chunks.push_back( { match, cursor } );

                cursor += object.cellCount;
            }

            if( cursor != cells.size() )
                THROW_IO_ERROR( "Invalid PADS route-cell extent" );

            for( const ROUTE_CHUNK& chunk : chunks )
            {
                const ROUTE_OBJECT& object = objects[chunk.objectIndex];

                TRACK  track;
                track.layer = objectLayers[chunk.objectIndex];
                track.width = object.width;

                if( track.layer <= 0 || static_cast<size_t>( track.layer ) >= m_layerInfos.size() )
                    THROW_IO_ERROR( "Invalid PADS route-object layer" );

                int routingDirection = m_layerInfos[track.layer].routing_direction;

                if( routingDirection < 0 || routingDirection > 4 )
                    THROW_IO_ERROR( "Invalid PADS routing direction" );

                bool fixedIsX = routingDirection == 1;

                for( size_t j = 0; j < object.cellCount; ++j )
                {
                    const ROUTE_CELL& cell = cells[chunk.cellStart + j];
                    double            x1 = fixedIsX ? cell.x1 : cell.y;
                    double            y1 = fixedIsX ? cell.y : cell.x1;
                    double            x2 = fixedIsX ? cell.x2 : cell.y;
                    double            y2 = fixedIsX ? cell.y : cell.x2;

                    if( track.points.empty() || track.points.back().x != x1 || track.points.back().y != y1 )
                    {
                        track.points.emplace_back( x1, y1 );
                    }

                    if( x1 != x2 || y1 != y2 )
                        track.points.emplace_back( x2, y2 );

                }

                if( track.points.size() < 2 )
                    continue;

                auto                    handleIt = routeNodeNets.find( objectHandles[chunk.objectIndex] );
                std::optional<size_t>   netIndex;

                if( handleIt == routeNodeNets.end() )
                    THROW_IO_ERROR( "Missing PADS route-object node relationship" );

                const std::set<size_t>& handleNets = handleIt->second;

                if( handleNets.size() > 1 )
                    THROW_IO_ERROR( wxString::Format( "Conflicting PADS route-object handle nets "
                                                      "(object %zu, handle 0x%08X, nets %zu)",
                                                      chunk.objectIndex, objectHandles[chunk.objectIndex],
                                                      handleNets.size() ) );

                if( handleNets.size() == 1 )
                    netIndex = *handleNets.begin();

                if( !netIndex || *netIndex >= m_nets.size() )
                {
                    const ARC_POINT& first = track.points.front();
                    const ARC_POINT& last = track.points.back();
                    THROW_IO_ERROR( fmt::format( "Missing PADS route-object net relationship "
                                                 "(object {}, handle 0x{:08X}, layer {}, cells {}, "
                                                 "first {:.0f},{:.0f}, last {:.0f},{:.0f})",
                                                 chunk.objectIndex, objectHandles[chunk.objectIndex], track.layer,
                                                 object.cellCount, first.x, first.y, last.x, last.y ) );
                }

                decodedPieces.push_back( { std::move( track ), m_nets[*netIndex].name } );
            }

            for( DECODED_TRACK& decoded : decodedPieces )
            {
                ROUTE& route = routes[decoded.netName];
                route.net_name = decoded.netName;
                route.tracks.push_back( std::move( decoded.track ) );
            }
        }
    }

    for( auto& [netName, route] : routes )
    {
        if( route.tracks.empty() && route.vias.empty() )
            continue;

        m_routes.push_back( std::move( route ) );
    }
}


void BINARY_PARSER::parseCopperShapes()
{
    const SDB_SECTION* sec10 = getSection( SECTION::DrwItems );
    const SDB_SECTION* sec11 = getSection( SECTION::GraphicPieces );
    const SDB_SECTION* sec12 = getSection( SECTION::Vertices );

    if( m_version <= 0x2022 )
        return;

    if( !sec10 || !sec11 || !sec12 )
        THROW_IO_ERROR( "Missing PADS copper-shape controllers" );

    const size_t pieceStride = m_version <= 0x2024 ? 16 : 20;

    if( sec10->stride != 112 || sec11->stride != pieceStride || sec12->stride != 12 )
        THROW_IO_ERROR( "Invalid PADS copper-shape controller framing" );

    constexpr size_t MAX_COPPER_SHAPE_EDGES = 80;

    // Section 10 is a circular controller. buildOwnerRuns follows its declared count and the
    // serialized owner-to-piece cursors; section 12 vertices start at their physical offset.
    for( const auto& [name, run] : m_ownerRuns )
    {
        if( name.size() < 4 || name.substr( 0, 3 ) != "DRW" )
            continue;

        const size_t ownerStride = m_version <= 0x2022 ? 100 : 112;

        if( ( run.itemKind & 0xFFFFU ) != 3 )
            continue;

        uint32_t sec11Index = static_cast<uint32_t>( run.pieceStart );

        if( sec11Index >= sec11->count )
            continue;

        int32_t originX = ringI32( *sec10, 68, ownerStride, run.ownerIndex, DRW_ITEM::ORIGIN_X );
        int32_t originY = ringI32( *sec10, 68, ownerStride, run.ownerIndex, DRW_ITEM::ORIGIN_Y );

        // fetchOwnerLoop already strips the duplicate closing point, so its own >= 3 success
        // condition is the true minimum for a valid polygon (a triangle). The previous minEdges
        // of 5 (4 for v2026) was excluding legitimate 4-corner rectangles -- PADS' own corner
        // count includes that closing repeat, so a "5 corner" ASCII COPCLS is a plain rectangle,
        // not a pentagon. Verified against MC4_PLUS_CSHAPE.pcb's DRW43215695/DRW89204466 (both
        // real 4-corner COPCLS rectangles, dropped by the old threshold) and the eight
        // DRW_TAG::COPPER_FILL_B rectangles alongside them.
        std::vector<VECTOR2I> loop;

        if( fetchOwnerLoop( name, MAX_COPPER_SHAPE_EDGES, loop ) )
        {
        }
        else
        {
            // A circular copper fill (PADS' COPCIR piece type) stores exactly two diametrically
            // opposite endpoints -- fetchOwnerLoop above always rejects that shape since it
            // never finds a closing point. The bbox-equality cross-check still applies, now
            // against the circle's own derived extent (center = midpoint, radius = half the
            // point-to-point span): verified against MC4_PLUS_CSHAPE.pcb's DRW9467290, whose
            // derived circle bbox matches its declared header bbox exactly.
            VECTOR2I p0, p1;

            if( !fetchOwnerCirclePoints( name, p0, p1 ) )
                continue;

            const double cx = ( p0.x + p1.x ) / 2.0;
            const double cy = ( p0.y + p1.y ) / 2.0;
            const double radius = std::hypot( p1.x - p0.x, p1.y - p0.y ) / 2.0;

            constexpr int CIRCLE_SEGMENTS = 32;
            loop.clear();

            for( int i = 0; i < CIRCLE_SEGMENTS; ++i )
            {
                double angle = ( 2.0 * M_PI * static_cast<double>( i ) ) / static_cast<double>( CIRCLE_SEGMENTS );
                loop.emplace_back( static_cast<int32_t>( std::lround( cx + radius * std::cos( angle ) ) ),
                                   static_cast<int32_t>( std::lround( cy + radius * std::sin( angle ) ) ) );
            }
        }

        COPPER_SHAPE copper;
        copper.name = name;
        copper.filled = true;

        size_t pieceRotation = sec11->totalBytes - ( pieceStride - 8 );
        uint32_t levelIndex = ( sec11Index + 1 ) % sec11->count;
        uint8_t level = ringU8( *sec11, pieceRotation, pieceStride, levelIndex, 1 );
        copper.width = static_cast<double>( ringI32( *sec11, pieceRotation, pieceStride,
                                                     sec11Index, pieceStride - 8 ) );
        copper.layer = level;

        for( const VECTOR2I& pt : loop )
        {
            int32_t rawX = static_cast<int32_t>( originX + pt.x );
            int32_t rawY = static_cast<int32_t>( originY + pt.y );
            copper.outline.emplace_back( toBasicCoordX( rawX ), toBasicCoordY( rawY ) );
        }

        m_copper_shapes.push_back( std::move( copper ) );
    }
}


void BINARY_PARSER::parseDimensions()
{
    const SDB_SECTION* sec10 = getSection( SECTION::DrwItems );

    if( !sec10 || m_version <= 0x2022 || m_ownerRuns.empty() )
        return;

    // A dimension's leader geometry is the sec12 vertex run of its DIM* DRW owner, laid out in
    // sub-piece order: BASPNT1(2v) BASPNT2(2v) ARWLN1(2v) ARWHD1(4v) ARWLN2(2v) ARWHD2(4v)
    // EXTLN1(2v) EXTLN2(2v). The measurement endpoints are the two BASPNT first points (run rows
    // 0 and 2); the crossbar is the ARWLN1 first point (run row 4). The vertices are absolute
    // design coords (no owner-origin shift).
    //
    // The value-label text lives in a sec8 record bound to the dimension only by anchor
    // proximity, which is unreliable when title-block notes share the dimension layer and
    // overlap the leader extent. We emit only the exact geometry and leave the override text
    // empty, so KiCad recomputes the displayed value from start/end (which equals the PADS value).
    for( uint32_t rec = 0; rec < sec10->count; ++rec )
    {
        std::string name = ringStr( *sec10, 68, 112, rec, 44, 24 );

        if( name.size() < 4 || name.substr( 0, 3 ) != "DIM" )
            continue;

        auto it = m_ownerRuns.find( name );

        if( it == m_ownerRuns.end() )
            continue;

        int32_t startRow = it->second.vertexStart - m_sec12Base;

        int32_t bp1x = 0, bp1y = 0, bp2x = 0, bp2y = 0, arwx = 0, arwy = 0, attr = 0;

        if( !sec12Vertex( startRow + 0, bp1x, bp1y, attr ) || !sec12Vertex( startRow + 2, bp2x, bp2y, attr )
            || !sec12Vertex( startRow + 4, arwx, arwy, attr ) )
        {
            continue;
        }

        DIMENSION dim;
        dim.name = name;
        dim.x = toBasicCoordX( bp1x );
        dim.y = toBasicCoordY( bp1y );

        POINT pt1{ toBasicCoordX( bp1x ), toBasicCoordY( bp1y ) };
        POINT pt2{ toBasicCoordX( bp2x ), toBasicCoordY( bp2y ) };
        dim.points.push_back( pt1 );
        dim.points.push_back( pt2 );

        // Horizontal vs vertical from the larger BASPNT delta; crossbar_pos is the ARWLN1 first
        // point projected onto the measured axis.
        dim.is_horizontal = std::abs( bp2x - bp1x ) > std::abs( bp2y - bp1y );
        dim.crossbar_pos = dim.is_horizontal ? toBasicCoordY( arwy ) : toBasicCoordX( arwx );

        m_dimensions.push_back( std::move( dim ) );
    }
}


void BINARY_PARSER::computeSec12Base()
{
    m_sec12Base = 0;
    m_sec12CleanRows = 0;

    const SDB_SECTION* sec12 = getSection( SECTION::Vertices );

    if( !sec12 )
        THROW_IO_ERROR( "Missing PADS vertex controller" );

    if( sec12->totalBytes != static_cast<uint64_t>( sec12->count ) * 12 )
        THROW_IO_ERROR( "Invalid PADS vertex-controller extent" );

    m_sec12CleanRows = static_cast<int32_t>( sec12->count );
}


uint8_t BINARY_PARSER::ringU8( const SDB_SECTION& aSection, size_t aRotation, size_t aStride,
                               uint32_t aIndex, size_t aField ) const
{
    if( aSection.totalBytes == 0 || aIndex >= aSection.count || aField >= aStride )
        THROW_IO_ERROR( "Invalid PADS circular-controller record access" );

    size_t logical = static_cast<size_t>( aIndex ) * aStride + aField;
    size_t physical = ( aRotation + logical ) % aSection.totalBytes;
    return m_cursor.U8At( aSection.physicalOffset + physical );
}


uint32_t BINARY_PARSER::ringU32( const SDB_SECTION& aSection, size_t aRotation, size_t aStride,
                                uint32_t aIndex, size_t aField ) const
{
    uint32_t value = 0;

    for( size_t byte = 0; byte < 4; ++byte )
        value |= static_cast<uint32_t>( ringU8( aSection, aRotation, aStride, aIndex, aField + byte ) ) << ( byte * 8 );

    return value;
}


int32_t BINARY_PARSER::ringI32( const SDB_SECTION& aSection, size_t aRotation, size_t aStride,
                                uint32_t aIndex, size_t aField ) const
{
    return static_cast<int32_t>( ringU32( aSection, aRotation, aStride, aIndex, aField ) );
}


std::string BINARY_PARSER::ringStr( const SDB_SECTION& aSection, size_t aRotation, size_t aStride,
                                    uint32_t aIndex, size_t aField, size_t aLength ) const
{
    std::string value;
    value.reserve( aLength );

    for( size_t byte = 0; byte < aLength; ++byte )
    {
        char c = static_cast<char>( ringU8( aSection, aRotation, aStride, aIndex, aField + byte ) );

        if( c == '\0' )
            break;

        value.push_back( c );
    }

    return value;
}


void BINARY_PARSER::buildOwnerRuns()
{
    m_ownerRuns.clear();

    const SDB_SECTION* sec10 = getSection( SECTION::DrwItems );

    if( !sec10 )
        THROW_IO_ERROR( "Missing PADS drawing controller" );

    if( sec10->count == 0 )
        return;

    const size_t stride = m_version <= 0x2022 ? 100 : 112;
    const size_t nameOffset = m_version <= 0x2022 ? 32 : 44;

    if( sec10->totalBytes != static_cast<uint64_t>( sec10->count ) * stride )
        THROW_IO_ERROR( "Invalid PADS drawing-controller extent" );

    for( uint32_t ownerIndex = 0; ownerIndex < sec10->count; ++ownerIndex )
    {
        std::string name = ringStr( *sec10, 68, stride, ownerIndex, nameOffset, stride - nameOffset );

        if( name.empty() || m_ownerRuns.count( name ) )
            continue;

        uint32_t lagIndex = ( ownerIndex + 1 ) % sec10->count;
        OWNER_RUN run;
        run.pieceStart = ringI32( *sec10, 68, stride, lagIndex, DRW_ITEM::PIECE_START );
        run.vertexStart = ringI32( *sec10, 68, stride, lagIndex, DRW_ITEM::VERTEX_START );
        run.arcStart = ringI32( *sec10, 68, stride, lagIndex, DRW_ITEM::ARC_START );
        run.pieceCount = ringI32( *sec10, 68, stride, lagIndex, DRW_ITEM::PIECE_COUNT );
        run.itemKind = ringU32( *sec10, 68, stride, lagIndex, DRW_ITEM::SUBTYPE_WORD );
        run.ownerIndex = ownerIndex;
        m_ownerRuns.emplace( std::move( name ), run );
    }
}


void BINARY_PARSER::parseBoardOutlineDirect()
{
    const SDB_SECTION* owners = getSection( SECTION::DrwItems );
    const SDB_SECTION* pieces = getSection( SECTION::GraphicPieces );
    const SDB_SECTION* vertices = getSection( SECTION::Vertices );
    const SDB_SECTION* arcParameters = getSection( SECTION::DecalLibrary );

    if( !owners || !pieces || !vertices || !arcParameters )
        THROW_IO_ERROR( "Missing PADS outline controllers" );

    const size_t ownerStride = m_version <= 0x2022 ? 100 : 112;
    const size_t pieceStride = m_version <= 0x2024 ? 16 : 20;
    const size_t pieceHead = pieceStride == 16 ? 8 : 12;
    const size_t cornerField = pieceStride == 16 ? 12 : 16;
    const size_t originXField = ownerStride == 100 ? 72 : 88;
    const size_t originYField = originXField + 4;
    const size_t pieceRotation = pieces->totalBytes - pieceHead;

    for( const auto& [name, run] : m_ownerRuns )
    {
        if( ( run.itemKind & 0xFFFFU ) != 1 || run.pieceCount <= 0 || run.pieceStart < 0 || run.vertexStart < 0 )
            continue;

        if( static_cast<uint64_t>( run.pieceStart ) + static_cast<uint64_t>( run.pieceCount ) > pieces->count )
            THROW_IO_ERROR( "Invalid PADS board-outline piece range" );

        int32_t originX = ringI32( *owners, 68, ownerStride, run.ownerIndex, originXField );
        int32_t originY = ringI32( *owners, 68, ownerStride, run.ownerIndex, originYField );
        int32_t vertexCursor = run.vertexStart;

        for( int32_t piece = 0; piece < run.pieceCount; ++piece )
        {
            uint32_t pieceIndex = static_cast<uint32_t>( run.pieceStart + piece );
            int32_t  corners = ringI32( *pieces, pieceRotation, pieceStride, pieceIndex, cornerField );

            if( corners < 1
                || static_cast<uint64_t>( vertexCursor ) + static_cast<uint64_t>( corners ) > vertices->count )
                THROW_IO_ERROR( "Invalid PADS board-outline vertex range" );

            std::vector<ARC_VERTEX> decoded;
            decoded.reserve( static_cast<size_t>( corners ) );

            for( int32_t corner = 0; corner < corners; ++corner )
            {
                size_t     offset = vertices->physicalOffset + static_cast<size_t>( vertexCursor + corner ) * 12;
                SDB_RECORD record = m_sdb.RecordAt( offset );
                decoded.push_back( { record.I32( 0 ), record.I32( 4 ), record.I32( 8 ) } );
            }

            vertexCursor += corners;

            POLYLINE outline;
            outline.layer = 1;
            outline.width = static_cast<double>( ringI32( *pieces, pieceRotation, pieceStride,
                                                          pieceIndex, pieceStride - 8 ) );
            outline.closed = decoded.size() >= 3 && decoded.front().x == decoded.back().x
                             && decoded.front().y == decoded.back().y;

            for( size_t index = 0; index < decoded.size(); ++index )
            {
                const ARC_VERTEX& vertex = decoded[index];
                double rawX = static_cast<double>( vertex.x ) + originX;
                double rawY = static_cast<double>( vertex.y ) + originY;

                if( index > 0 && decoded[index - 1].attr >= 0 )
                {
                    uint64_t arcIndex = static_cast<uint64_t>( run.arcStart )
                                        + static_cast<uint32_t>( decoded[index - 1].attr );

                    if( run.arcStart < 0 || arcIndex >= arcParameters->count
                        || arcParameters->totalBytes != static_cast<uint64_t>( arcParameters->count ) * 20 )
                    {
                        THROW_IO_ERROR( "Invalid PADS board-outline arc range" );
                    }

                    SDB_RECORD arcRecord = m_sdb.RecordAt( arcParameters->physicalOffset + arcIndex * 20 );
                    double xmin = arcRecord.I32( 0 );
                    double ymin = arcRecord.I32( 4 );
                    double xmax = arcRecord.I32( 8 );
                    double ymax = arcRecord.I32( 12 );
                    double centerX = ( xmin + xmax ) / 2.0;
                    double centerY = ( ymin + ymax ) / 2.0;
                    double radius = ( xmax - xmin ) / 2.0;
                    const ARC_VERTEX& start = decoded[index - 1];
                    double startAngle = std::atan2( start.y - centerY, start.x - centerX ) * 180.0 / M_PI;
                    double endAngle = std::atan2( vertex.y - centerY, vertex.x - centerX ) * 180.0 / M_PI;
                    double delta = endAngle - startAngle;

                    while( delta <= -180.0 )
                        delta += 360.0;

                    while( delta > 180.0 )
                        delta -= 360.0;

                    ARC arc{};
                    arc.cx = centerX + originX;
                    arc.cy = centerY + originY;
                    arc.radius = radius;
                    arc.start_angle = startAngle;
                    arc.delta_angle = delta;
                    outline.points.emplace_back( rawX, rawY, arc );
                }
                else
                {
                    outline.points.emplace_back( rawX, rawY );
                }
            }

            m_boardOutlines.push_back( std::move( outline ) );
        }
    }
}


void BINARY_PARSER::parseGraphicLines()
{
    const SDB_SECTION* owners = getSection( SECTION::DrwItems );
    const SDB_SECTION* pieces = getSection( SECTION::GraphicPieces );
    const SDB_SECTION* vertices = getSection( SECTION::Vertices );
    const SDB_SECTION* arcParameters = getSection( SECTION::DecalLibrary );

    if( !owners || !pieces || !vertices || !arcParameters )
        THROW_IO_ERROR( "Missing PADS graphic controllers" );

    const size_t ownerStride = m_version <= 0x2022 ? 100 : 112;
    const size_t pieceStride = m_version <= 0x2024 ? 16 : 20;
    const size_t pieceHead = pieceStride - 8;
    const size_t cornerField = pieceStride - 4;
    const size_t originXField = ownerStride == 100 ? 72 : 88;
    const size_t originYField = originXField + 4;
    const size_t pieceRotation = pieces->totalBytes - pieceHead;

    if( vertices->totalBytes != static_cast<uint64_t>( vertices->count ) * 12
        || arcParameters->totalBytes != static_cast<uint64_t>( arcParameters->count ) * 20 )
    {
        THROW_IO_ERROR( "Invalid PADS graphic-controller framing" );
    }

    for( const auto& [name, run] : m_ownerRuns )
    {
        if( ( run.itemKind & 0xFFFFU ) != 0 || name.compare( 0, 3, "DRW" ) != 0 || run.pieceCount <= 0
            || run.pieceStart < 0 || run.vertexStart < 0 )
        {
            continue;
        }

        if( static_cast<uint64_t>( run.pieceStart ) + static_cast<uint64_t>( run.pieceCount ) > pieces->count )
            THROW_IO_ERROR( "Invalid PADS graphic piece range" );

        int32_t originX = ringI32( *owners, 68, ownerStride, run.ownerIndex, originXField );
        int32_t originY = ringI32( *owners, 68, ownerStride, run.ownerIndex, originYField );
        int32_t vertexCursor = run.vertexStart;

        for( int32_t piece = 0; piece < run.pieceCount; ++piece )
        {
            uint32_t pieceIndex = static_cast<uint32_t>( run.pieceStart + piece );
            int32_t  corners = ringI32( *pieces, pieceRotation, pieceStride, pieceIndex, cornerField );

            if( corners < 1
                || static_cast<uint64_t>( vertexCursor ) + static_cast<uint64_t>( corners ) > vertices->count )
            {
                THROW_IO_ERROR( wxString::Format( "Invalid PADS graphic vertex range (%s piece %d, start %d, "
                                                  "corners %d, count %u)",
                                                  name.c_str(), piece, vertexCursor, corners, vertices->count ) );
            }

            GRAPHIC_LINE graphic;
            graphic.name = name;
            graphic.width = ringI32( *pieces, pieceRotation, pieceStride, pieceIndex, pieceStride - 8 );
            graphic.layer = ringU8( *pieces, pieceRotation, pieceStride, ( pieceIndex + 1 ) % pieces->count, 1 );
            graphic.points.reserve( static_cast<size_t>( corners ) );

            std::vector<ARC_VERTEX> decoded;
            decoded.reserve( static_cast<size_t>( corners ) );

            for( int32_t corner = 0; corner < corners; ++corner )
            {
                SDB_RECORD record = m_sdb.RecordAt( vertices->physicalOffset
                                                    + static_cast<uint32_t>( vertexCursor + corner ) * 12 );
                decoded.push_back( { record.I32( 0 ), record.I32( 4 ), record.I32( 8 ) } );
            }

            vertexCursor += corners;
            graphic.closed = decoded.size() >= 3 && decoded.front().x == decoded.back().x
                             && decoded.front().y == decoded.back().y;

            uint8_t pieceType = ringU8( *pieces, pieceRotation, pieceStride, ( pieceIndex + 1 ) % pieces->count, 0 );

            if( pieceType == 2 && decoded.size() == 2 )
            {
                double centerX = ( decoded[0].x + decoded[1].x ) / 2.0 + originX;
                double centerY = ( decoded[0].y + decoded[1].y ) / 2.0 + originY;
                double radius = std::hypot( decoded[1].x - decoded[0].x, decoded[1].y - decoded[0].y ) / 2.0;
                ARC    arc{};
                arc.cx = centerX;
                arc.cy = centerY;
                arc.radius = radius;
                arc.start_angle = 0.0;
                arc.delta_angle = 360.0;
                graphic.closed = true;
                graphic.points.emplace_back( centerX + radius, centerY, arc );
                m_graphicLines.push_back( std::move( graphic ) );
                continue;
            }

            for( size_t index = 0; index < decoded.size(); ++index )
            {
                const ARC_VERTEX& vertex = decoded[index];
                double            rawX = static_cast<double>( vertex.x ) + originX;
                double            rawY = static_cast<double>( vertex.y ) + originY;

                if( index > 0 && decoded[index - 1].attr >= 0 )
                {
                    uint64_t arcIndex =
                            static_cast<uint64_t>( run.arcStart ) + static_cast<uint32_t>( decoded[index - 1].attr );

                    if( run.arcStart < 0 || arcIndex >= arcParameters->count )
                        THROW_IO_ERROR( "Invalid PADS graphic arc range" );

                    SDB_RECORD        arcRecord = m_sdb.RecordAt( arcParameters->physicalOffset + arcIndex * 20 );
                    double            xmin = arcRecord.I32( 0 );
                    double            ymin = arcRecord.I32( 4 );
                    double            xmax = arcRecord.I32( 8 );
                    double            ymax = arcRecord.I32( 12 );
                    double            centerX = ( xmin + xmax ) / 2.0;
                    double            centerY = ( ymin + ymax ) / 2.0;
                    double            radius = ( xmax - xmin ) / 2.0;
                    const ARC_VERTEX& start = decoded[index - 1];
                    double            startAngle = std::atan2( start.y - centerY, start.x - centerX ) * 180.0 / M_PI;
                    double            endAngle = std::atan2( vertex.y - centerY, vertex.x - centerX ) * 180.0 / M_PI;
                    double            delta = endAngle - startAngle;

                    while( delta <= -180.0 )
                        delta += 360.0;

                    while( delta > 180.0 )
                        delta -= 360.0;

                    ARC arc{};
                    arc.cx = centerX + originX;
                    arc.cy = centerY + originY;
                    arc.radius = radius;
                    arc.start_angle = startAngle;
                    arc.delta_angle = delta;
                    graphic.points.emplace_back( rawX, rawY, arc );
                }
                else
                {
                    graphic.points.emplace_back( rawX, rawY );
                }
            }

            m_graphicLines.push_back( std::move( graphic ) );
        }
    }
}


bool BINARY_PARSER::sec12Vertex( int32_t aRow, int32_t& aX, int32_t& aY, int32_t& aAttr ) const
{
    const SDB_SECTION* sec12 = getSection( SECTION::Vertices );

    if( !sec12 || aRow < 0 || aRow >= m_sec12CleanRows )
        return false;

    if( sec12->physicalOffset + static_cast<size_t>( aRow + 1 ) * 12 > m_data.size() )
        return false;

    SDB_RECORD rec = m_sdb.RecordAt( sec12->physicalOffset + static_cast<uint32_t>( aRow ) * 12 );
    aX = rec.I32( 0 );
    aY = rec.I32( 4 );
    aAttr = rec.I32( 8 );
    return true;
}


bool BINARY_PARSER::fetchOwnerLoop( const std::string& aName, size_t aMaxVerts, std::vector<VECTOR2I>& aOut ) const
{
    aOut.clear();

    auto it = m_ownerRuns.find( aName );

    if( it == m_ownerRuns.end() )
        return false;

    const SDB_SECTION* pieces = getSection( SECTION::GraphicPieces );

    if( !pieces || it->second.pieceCount < 1 || it->second.pieceStart < 0 )
        return false;

    size_t pieceStride = m_version <= 0x2024 ? 16 : 20;
    size_t pieceHead = pieceStride == 16 ? 8 : 12;
    size_t cornerField = pieceStride == 16 ? 12 : 16;
    size_t pieceRotation = pieces->totalBytes - pieceHead;
    int32_t corners = ringI32( *pieces, pieceRotation, pieceStride,
                               static_cast<uint32_t>( it->second.pieceStart ), cornerField );

    if( corners < 4 || static_cast<size_t>( corners ) > aMaxVerts + 1 )
        return false;

    int32_t startRow = it->second.vertexStart;

    int32_t firstX = 0;
    int32_t firstY = 0;
    int32_t attr = 0;

    if( !sec12Vertex( startRow, firstX, firstY, attr ) )
        return false;

    for( int32_t corner = 0; corner < corners - 1; ++corner )
    {
        int32_t x = 0;
        int32_t y = 0;

        if( !sec12Vertex( startRow + corner, x, y, attr ) )
            return false;

        aOut.emplace_back( x, y );
    }

    int32_t lastX = 0;
    int32_t lastY = 0;

    if( !sec12Vertex( startRow + corners - 1, lastX, lastY, attr ) || lastX != firstX || lastY != firstY )
    {
        aOut.clear();
        return false;
    }

    return true;
}


bool BINARY_PARSER::fetchOwnerCirclePoints( const std::string& aName, VECTOR2I& aP0, VECTOR2I& aP1 ) const
{
    auto it = m_ownerRuns.find( aName );

    if( it == m_ownerRuns.end() )
        return false;

    const SDB_SECTION* pieces = getSection( SECTION::GraphicPieces );

    if( !pieces || it->second.pieceCount < 1 || it->second.pieceStart < 0 )
        return false;

    size_t pieceStride = m_version <= 0x2024 ? 16 : 20;
    size_t pieceHead = pieceStride == 16 ? 8 : 12;
    size_t cornerField = pieceStride == 16 ? 12 : 16;
    size_t pieceRotation = pieces->totalBytes - pieceHead;

    if( ringI32( *pieces, pieceRotation, pieceStride, static_cast<uint32_t>( it->second.pieceStart ),
                 cornerField ) != 2 )
    {
        return false;
    }

    int32_t startRow = it->second.vertexStart;

    int32_t x0 = 0, y0 = 0, x1 = 0, y1 = 0, attr = 0;

    if( !sec12Vertex( startRow, x0, y0, attr ) || !sec12Vertex( startRow + 1, x1, y1, attr ) )
        return false;

    if( x0 == x1 && y0 == y1 )
        return false;

    aP0 = VECTOR2I( x0, y0 );
    aP1 = VECTOR2I( x1, y1 );
    return true;
}


void BINARY_PARSER::parseKeepouts()
{
    const SDB_SECTION* sec10 = getSection( SECTION::DrwItems );
    const SDB_SECTION* sec12 = getSection( SECTION::Vertices );

    if( m_version <= 0x2022 )
        return;

    if( !sec10 || !sec12 )
        THROW_IO_ERROR( "Missing PADS keepout controllers" );

    if( sec10->stride < 112 || sec12->stride < 12 )
        THROW_IO_ERROR( "Invalid PADS keepout-controller framing" );

    struct Owner
    {
        std::string name;
        int32_t     originX = 0;
        int32_t     originY = 0;
        int64_t     minX = 0;
        int64_t     minY = 0;
        int64_t     maxX = 0;
        int64_t     maxY = 0;
    };

    std::vector<Owner> owners;

    for( const auto& [name, run] : m_ownerRuns )
    {
        if( ( run.itemKind & 0xFFFFU ) != 10 )
            continue;

        Owner owner;
        owner.name = name;
        owner.originX = ringI32( *sec10, 68, 112, run.ownerIndex, DRW_ITEM::ORIGIN_X );
        owner.originY = ringI32( *sec10, 68, 112, run.ownerIndex, DRW_ITEM::ORIGIN_Y );
        owner.minX = static_cast<int64_t>( ringI32( *sec10, 68, 112, run.ownerIndex, DRW_ITEM::BBOX_MIN_X ) )
                     - owner.originX;
        owner.minY = static_cast<int64_t>( ringI32( *sec10, 68, 112, run.ownerIndex, DRW_ITEM::BBOX_MIN_Y ) )
                     - owner.originY;
        owner.maxX = static_cast<int64_t>( ringI32( *sec10, 68, 112, run.ownerIndex, DRW_ITEM::BBOX_MAX_X ) )
                     - owner.originX;
        owner.maxY = static_cast<int64_t>( ringI32( *sec10, 68, 112, run.ownerIndex, DRW_ITEM::BBOX_MAX_Y ) )
                     - owner.originY;

        owners.push_back( std::move( owner ) );
    }

    if( owners.empty() )
        return;

    constexpr size_t MAX_KEEP_OUT_VERTICES = 80;

    for( const Owner& owner : owners )
    {
        KEEPOUT keepout;
        keepout.type = KEEPOUT_TYPE::ALL;

        // The owner's vertexStart cursor anchors a contiguous run in sec12 that closes back to
        // its first vertex. Vertices are design coordinates; add the DRW raw origin to get RAW.
        std::vector<VECTOR2I> structuralLoop;

        if( fetchOwnerLoop( owner.name, MAX_KEEP_OUT_VERTICES, structuralLoop ) )
        {
            for( const VECTOR2I& vertex : structuralLoop )
            {
                int32_t rawX = owner.originX + static_cast<int32_t>( vertex.x );
                int32_t rawY = owner.originY + static_cast<int32_t>( vertex.y );
                keepout.outline.emplace_back( toBasicCoordX( rawX ), toBasicCoordY( rawY ) );
            }

            m_keepouts.push_back( std::move( keepout ) );
            continue;
        }

        // A circle keepout has a degenerate (2-point) sec12 run that does not close. Its geometry
        // is the owner record's +96..+108 bbox: center = midpoint, radius = (xmax - xmin) / 2.
        int64_t spanX = owner.maxX - owner.minX;
        int64_t spanY = owner.maxY - owner.minY;

        if( spanX <= 0 || spanX != spanY )
            continue;

        constexpr int ELLIPSE_SEGMENTS = 32;
        double        cx = static_cast<double>( owner.minX + owner.maxX ) / 2.0;
        double        cy = static_cast<double>( owner.minY + owner.maxY ) / 2.0;
        double        radius = static_cast<double>( spanX ) / 2.0;

        for( int i = 0; i < ELLIPSE_SEGMENTS; ++i )
        {
            double  angle = ( 2.0 * M_PI * static_cast<double>( i ) ) / static_cast<double>( ELLIPSE_SEGMENTS );
            int32_t rawX = owner.originX + static_cast<int32_t>( std::lround( cx + radius * std::cos( angle ) ) );
            int32_t rawY = owner.originY + static_cast<int32_t>( std::lround( cy + radius * std::sin( angle ) ) );
            keepout.outline.emplace_back( toBasicCoordX( rawX ), toBasicCoordY( rawY ) );
        }

        m_keepouts.push_back( std::move( keepout ) );
    }
}


void BINARY_PARSER::parseCopperPours()
{
    // Section 52 is the declared 88-byte outline-owner array. The four-byte state stream after
    // section 49 has one word per live section-46 slot; accounting for it places sections 52--55
    // directly, with no signature search or phase selection.
    //
    // Owner record (88 bytes):
    //   +0   u32 first piece index
    //   +4   u32 first vertex index
    //   +8   u32 first arc index (arcs are not yet imported)
    //   +24  i32 raw XLOC -- each pour owns its own anchor, not a shared board anchor
    //   +28  i32 raw YLOC
    //   +70  char name[16]
    //
    // Piece record (16 bytes), addressed by the owner's piece index:
    //   +0   u32 corner count
    //   +4   u32 arc count
    //   +8   i32 width, BASIC units
    //   +12  u8  piece type: 0x32 = polygon, 0x33 = circle (two diametrically-opposite corners,
    //        not a 2-point polygon)
    //   +13  u8  layer
    //
    // The vertex array is a flat run of 8-byte local (i32 x, i32 y) pairs.
    // Arc records decorate specific corner-to-corner segments with a curve rather than adding
    // extra boundary points, so the corner list alone still yields a closed (if not smoothly
    // curved) outline; arc-to-curve conversion is not implemented.
    static constexpr size_t OWNER_SIZE = 88;
    static constexpr size_t PIECE_SIZE = 16;
    static constexpr size_t VERTEX_SIZE = 8;

    struct POUR_OWNER
    {
        uint32_t    pieceStart = 0;
        uint32_t    vertexStart = 0;
        uint32_t    pieceCount = 0;
        int32_t     rawX = 0;
        int32_t     rawY = 0;
        std::string name;
    };

    std::vector<POUR_OWNER> owners;

    const SDB_SECTION* sec52 = getSection( SECTION::PourTokensA );
    const SDB_SECTION* sec53 = getSection( SECTION::PourTokensB );
    const SDB_SECTION* sec54 = getSection( SECTION::PourTokensC );

    if( !sec52 || !sec53 || !sec54 )
        THROW_IO_ERROR( "Missing PADS copper-pour controllers" );

    if( sec52->physicalBytes != sec52->count * OWNER_SIZE
        || sec53->physicalBytes != sec53->count * PIECE_SIZE
        || sec54->physicalBytes != sec54->count * VERTEX_SIZE )
        THROW_IO_ERROR( "Invalid PADS copper-pour controller framing" );

    for( uint32_t index = 0; index < sec52->count; ++index )
    {
        const size_t offset = sec52->physicalOffset + static_cast<size_t>( index ) * OWNER_SIZE;
        const uint8_t outlineType = m_cursor.U8At( offset + 87 );
        std::string name = m_cursor.StringAt( offset + 70, 14 );

        if( outlineType != 0x32 || name.rfind( "POR", 0 ) != 0 )
            continue;

        POUR_OWNER owner;
        owner.pieceStart = m_cursor.U32At( offset );
        owner.vertexStart = m_cursor.U32At( offset + 4 );
        owner.rawX = m_cursor.I32At( offset + 24 );
        owner.rawY = m_cursor.I32At( offset + 28 );
        owner.pieceCount = m_cursor.U32At( offset + 64 );
        owner.name = std::move( name );
        owners.push_back( std::move( owner ) );
    }

    for( const POUR_OWNER& owner : owners )
    {
        uint32_t vertexIndex = owner.vertexStart;

        for( uint32_t pieceOrdinal = 0; pieceOrdinal < owner.pieceCount; ++pieceOrdinal )
        {
            const uint32_t pieceIndex = owner.pieceStart + pieceOrdinal;

            if( pieceIndex >= sec53->count )
                THROW_IO_ERROR( "Invalid PADS copper-pour piece range" );

            const size_t pieceOff = sec53->physicalOffset + static_cast<size_t>( pieceIndex ) * PIECE_SIZE;
            uint32_t     cornerCount = m_cursor.U32At( pieceOff );
            int32_t      width = m_cursor.I32At( pieceOff + 8 );
            uint8_t      pieceType = m_cursor.U8At( pieceOff + 12 );
            uint8_t      layer = m_cursor.U8At( pieceOff + 13 );

            if( cornerCount == 0 || vertexIndex > sec54->count || cornerCount > sec54->count - vertexIndex )
                THROW_IO_ERROR( "Invalid PADS copper-pour vertex range" );

            const size_t vOff = sec54->physicalOffset + static_cast<size_t>( vertexIndex ) * VERTEX_SIZE;
            vertexIndex += cornerCount;

            POUR pour;
            pour.owner_pour = owner.name;
            pour.width = static_cast<double>( width );
            pour.layer = static_cast<int>( layer );

            if( pieceType == 0x33 && cornerCount == 2 )
            {
            // Circle piece: the two "corners" are diametrically opposite endpoints, not a
            // 2-point polygon -- the downstream zone builder requires at least 3 points and
            // would silently drop it. Synthesize a regular polygon approximation instead.
                int32_t x0 = owner.rawX + m_cursor.I32At( vOff );
                int32_t y0 = owner.rawY + m_cursor.I32At( vOff + 4 );
                int32_t x1 = owner.rawX + m_cursor.I32At( vOff + VERTEX_SIZE );
                int32_t y1 = owner.rawY + m_cursor.I32At( vOff + VERTEX_SIZE + 4 );

                double cx = ( x0 + x1 ) / 2.0;
                double cy = ( y0 + y1 ) / 2.0;
                double radius = std::hypot( x1 - x0, y1 - y0 ) / 2.0;

                static constexpr int CIRCLE_SEGMENTS = 48;

                for( int s = 0; s < CIRCLE_SEGMENTS; ++s )
                {
                    double angle = 2.0 * M_PI * s / CIRCLE_SEGMENTS;
                    int32_t rawX = static_cast<int32_t>( std::lround( cx + radius * std::cos( angle ) ) );
                    int32_t rawY = static_cast<int32_t>( std::lround( cy + radius * std::sin( angle ) ) );

                    pour.points.emplace_back( toBasicCoordX( rawX ), toBasicCoordY( rawY ) );
                }
            }
            else
            {
                for( uint32_t vertex = 0; vertex < cornerCount; ++vertex )
                {
                    size_t  offset = vOff + static_cast<size_t>( vertex ) * VERTEX_SIZE;
                    int32_t localX = m_cursor.I32At( offset );
                    int32_t localY = m_cursor.I32At( offset + 4 );

                    pour.points.emplace_back( toBasicCoordX( owner.rawX + localX ),
                                              toBasicCoordY( owner.rawY + localY ) );
                }
            }

            m_pours.push_back( std::move( pour ) );
        }
    }
}


/// Section 69 begins with 12 bytes of controller state followed by its logical layer records.
size_t BINARY_PARSER::layerStackupBase() const
{
    const SDB_SECTION* section = m_sdb.Section( 69 );

    if( !section || section->physicalCount == 0 )
        return 0;

    constexpr uint64_t CONTROLLER_LEAD_IN = 12;
    const uint64_t     base = static_cast<uint64_t>( section->physicalOffset ) + CONTROLLER_LEAD_IN;
    const uint64_t     bytes = static_cast<uint64_t>( section->physicalCount ) * section->stride;

    return base + bytes <= m_data.size() ? static_cast<size_t>( base ) : 0;
}

void BINARY_PARSER::parseLayerStackup()
{
    m_layerInfos.clear();

    const SDB_SECTION* section = getSection( SECTION::LayerTable );

    if( !section )
        THROW_IO_ERROR( "Missing PADS layer-stackup controller" );

    if( section->stride != 128 && section->stride != 136 && section->stride != 152 )
        THROW_IO_ERROR( "Invalid PADS layer-stackup stride" );

    static constexpr size_t NAME_LEN = 24;
    static constexpr size_t OFF_ROUT = 32;
    static constexpr size_t OFF_LAYTH = 52;
    static constexpr size_t OFF_COPTH = 56;
    static constexpr size_t OFF_DIEL = 60;

    auto layerFunction = []( int32_t aSerializedType )
    {
        switch( aSerializedType )
        {
        case 0: return PADS_LAYER_FUNCTION::UNASSIGNED;
        case 1: return PADS_LAYER_FUNCTION::ROUTING;
        case 2: return PADS_LAYER_FUNCTION::DRILL;
        case 3: return PADS_LAYER_FUNCTION::SILK_SCREEN;
        case 4: return PADS_LAYER_FUNCTION::PASTE_MASK;
        case 5: return PADS_LAYER_FUNCTION::SOLDER_MASK;
        case 6: return PADS_LAYER_FUNCTION::ASSEMBLY;
        default: return PADS_LAYER_FUNCTION::UNKNOWN;
        }
    };

    size_t recordBase = layerStackupBase();

    if( recordBase == 0 )
        THROW_IO_ERROR( "Missing PADS layer-stackup framing" );

    if( !m_cursor.InBounds( recordBase, static_cast<size_t>( section->physicalCount ) * section->stride ) )
        THROW_IO_ERROR( "Invalid PADS layer-stackup extent" );

    for( size_t k = 0; k < section->physicalCount; ++k )
    {
        size_t rec = recordBase + k * section->stride;

        SDB_RECORD layerRec = m_sdb.RecordAt( rec );
        LAYER_INFO info;
        info.number = static_cast<int>( k );
        info.name = layerRec.Str( 0, NAME_LEN );

        int32_t routingDir = layerRec.I32( OFF_ROUT );
        info.routing_direction = routingDir;

        info.layer_thickness = static_cast<double>( layerRec.I32( OFF_LAYTH ) );
        info.copper_thickness = static_cast<double>( layerRec.I32( OFF_COPTH ) );

        float dielectric = 0.0f;
        std::memcpy( &dielectric, &m_data[rec + OFF_DIEL], sizeof( float ) );
        info.dielectric_constant = static_cast<double>( dielectric );

        // The final word of record K-1 owns record K's LAYER_TYPE. The lag leaves the final
        // record's word as retained carrier state, like the other rotated flat controllers.
        const int32_t serializedType = k > 0 ? m_cursor.I32At( rec - 4 ) : 0;
        info.layer_type = k > 0 ? layerFunction( serializedType ) : PADS_LAYER_FUNCTION::UNASSIGNED;

        if( info.layer_type == PADS_LAYER_FUNCTION::UNKNOWN )
            THROW_IO_ERROR( "Invalid PADS serialized layer type" );

        info.is_copper = info.layer_type == PADS_LAYER_FUNCTION::ROUTING;
        info.required = info.is_copper;

        m_layerInfos.push_back( std::move( info ) );
    }
}


std::vector<LAYER_INFO> BINARY_PARSER::GetLayerInfos() const
{
    return m_layerInfos;
}


void BINARY_PARSER::linkPartsToDecals()
{
    if( m_parts.empty() || m_decals.empty() )
        return;

    if( usesDirectDecalChain() )
    {
        // Both old dialects resolve a placement's decal via the direct index in
        // m_partDecalIndex, against the decal-name table -- not through the parttype-index chain
        // below (v0x2022 does have a parttype-definition table, see parsePartTypeTable, but
        // placements don't reference it for their decal).
        if( m_partDecalIndex.empty() || m_decalNameTable.empty() )
            return;

        for( size_t partIdx = 0; partIdx < m_parts.size(); ++partIdx )
        {
            PART& part = m_parts[partIdx];

            if( !part.decal.empty() )
                continue;

            auto hintIt = m_partDecalIndex.find( partIdx );

            if( hintIt == m_partDecalIndex.end() )
                continue;

            uint32_t decalIndex = hintIt->second;

            if( decalIndex >= m_decalNameTable.size() )
                continue;

            const std::string& decalName = m_decalNameTable[decalIndex];

            if( !decalName.empty() && m_decals.count( decalName ) )
                part.decal = decalName;
        }

        return;
    }

    // Placement -> decal chain: parttype index I from m_partTypeIndex, then
    // m_partTypeDecalIndex[I] for the decal_index, then m_decalNameTable[decal_index] for the
    // name. The decal-name table covers connectors and mounting holes section 10 lacks, so this
    // resolves the full placed set.
    if( m_partTypeDecalIndex.empty() || m_decalNameTable.empty() )
        return;

    for( size_t partIdx = 0; partIdx < m_parts.size(); ++partIdx )
    {
        PART& part = m_parts[partIdx];

        if( !part.decal.empty() )
            continue;

        auto hintIt = m_partTypeIndex.find( partIdx );

        if( hintIt == m_partTypeIndex.end() )
            continue;

        uint32_t partTypeIdx = hintIt->second;

        if( partTypeIdx >= m_partTypeDecalIndex.size() )
            continue;

        uint8_t alternate = 0;
        auto    alternateIt = m_partDecalAlternate.find( partIdx );

        if( alternateIt != m_partDecalAlternate.end() )
            alternate = alternateIt->second;

        int32_t decalIndex = m_partTypeDecalIndex[partTypeIdx];

        if( partTypeIdx < m_partTypeDecalIndices.size() && alternate < m_partTypeDecalIndices[partTypeIdx].size() )
        {
            decalIndex = m_partTypeDecalIndices[partTypeIdx][alternate];
        }

        if( decalIndex < 0 || static_cast<size_t>( decalIndex ) >= m_decalNameTable.size() )
            continue;

        const std::string& decalName = m_decalNameTable[decalIndex];

        if( !decalName.empty() && m_decals.count( decalName ) )
            part.decal = decalName;

        // The *PARTTYPE alias (often a manufacturer part number) is more useful as the
        // footprint's BOM value than the physical decal name it resolves to -- e.g. a part
        // referencing PARTTYPE GRM15XR71C103KA86D that resolves to decal C-0402 should show
        // the part number, not "C-0402", as its value.
        if( part.value.empty() && partTypeIdx < m_partTypeNames.size() )
        {
            const std::string& typeName = m_partTypeNames[partTypeIdx];

            if( !typeName.empty() && typeName != part.decal )
                part.value = typeName;
        }
    }
}


} // namespace PADS_IO

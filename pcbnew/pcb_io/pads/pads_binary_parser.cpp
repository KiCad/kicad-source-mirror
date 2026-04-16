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
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <limits>
#include <optional>
#include <set>
#include <unordered_set>

#include <io/pads/pads_binary_utils.h>
#include <ki_exception.h>
#include <wx/log.h>

namespace PADS_IO
{

// Section 4 pad-shape codes. Validated against the ASC PARTDECAL pad shapes and by
// the finger-geometry invariant: codes 0 (OF) and 1 (RF) carry a non-zero finLength
// (size B) in 100% of records on every v0x2021..v0x2027 board, while 2 (R) and 3 (S)
// never do. Code 4 occurs on no board; the earlier {1:RF,2:R,3:S,4:OF} map left code 0
// unmapped, so every oblong (OF) pad was silently imported as round.
static const std::map<uint8_t, std::string> PAD_SHAPE_NAMES = {
    { 0x00, "OF" },
    { 0x01, "RF" },
    { 0x02, "R" },
    { 0x03, "S" },
};


// Per-version field layout for the part-placement records read by both
// parsePartPlacements (section 22) and parseSection19Parts (sections 19/21). The
// old format (v0x2021/0x2022) and the newer dialects place the same fields at
// different offsets; selecting one descriptor keeps the offset block in a single
// place instead of re-declaring it at each scanner.
struct PLACEMENT_LAYOUT
{
    int                nameOff = 0;           // refdes string
    int                xOff = 0;              // i32 X
    std::optional<int> yOff = std::nullopt;   // i32 Y (immediately follows X at xOff+4)
    int                angleOff = 0;          // i32 rotation
    int                feffOff = 0;           // 0xFEFF marker offset within the record
    int                scanStride = 0;        // record stride for the section-19/21 FEFF scan
    bool               v2021PadChain = false; // direct decal-index pad chain (v0x2021 only)
};


static const PLACEMENT_LAYOUT& placementLayout( uint16_t aVersion )
{
    // v0x2021 and v0x2022 share the old offset block; only v0x2021 enables the
    // verified direct decal-index pad chain. The placement record is structurally
    // identical to the new dialect (refdes, then X at +16, Y at +20, angle at +24,
    // side at +28); the old framing simply anchors refdes at +76 instead of +44, so
    // X/Y/angle/side land at +92/+96/+100/+104. Validated 5/5 against the MAIS_FC
    // (v0x2021) ASC oracle and cross-checked by the inside-bbox invariant on 2FOC-001.
    static constexpr PLACEMENT_LAYOUT v2021{ .nameOff = 76, .xOff = 92, .yOff = 96, .angleOff = 100,
                                             .feffOff = 28, .scanStride = 96, .v2021PadChain = true };
    static constexpr PLACEMENT_LAYOUT vOld{ .nameOff = 76, .xOff = 92, .yOff = 96, .angleOff = 100,
                                            .feffOff = 28, .scanStride = 96, .v2021PadChain = false };
    static constexpr PLACEMENT_LAYOUT vNew{ .nameOff = 44, .xOff = 60, .yOff = 64, .angleOff = 68,
                                            .feffOff = 92, .scanStride = 94, .v2021PadChain = false };

    if( aVersion == 0x2021 )
        return v2021;

    if( aVersion == 0x2022 )
        return vOld;

    return vNew;
}


// Per-version field layout for the section-4 padstack records read by
// parsePadStacks. As with the placements, the old and new dialects carry the
// same geometry fields at different offsets.
struct PADSTACK_LAYOUT
{
    int padWidthOff = 0;
    int drillOff = 0;
    int finLenOff = 0;
    int angleOff = 0;
    int markerOff = 0;
    int shapeOff = 0;
};


static const PADSTACK_LAYOUT& padstackLayout( bool aIsOld )
{
    static constexpr PADSTACK_LAYOUT vOld{ .padWidthOff = 24, .drillOff = 28, .finLenOff = 32,
                                           .angleOff = 40, .markerOff = 48, .shapeOff = 49 };
    static constexpr PADSTACK_LAYOUT vNew{ .padWidthOff = 28, .drillOff = 32, .finLenOff = 36,
                                           .angleOff = 48, .markerOff = 56, .shapeOff = 57 };

    return aIsOld ? vOld : vNew;
}


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

    // Magic bytes: 0x00 0xFF
    if( header[0] != 0x00 || header[1] != 0xFF )
        return false;

    uint16_t version = static_cast<uint16_t>( header[2] ) | ( static_cast<uint16_t>( header[3] ) << 8 );

    return PADS_SDB::IsSupportedVersion( version );
}


void BINARY_PARSER::Parse( const wxString& aFileName )
{
    std::vector<uint8_t> bytes;

    if( !PADS_IO::ReadFileToBuffer( aFileName, bytes ) )
        THROW_IO_ERROR( "Cannot open or read file" );

    // The SDB owns the bytes and decodes the file container: header, directory and the
    // coordinate origin. The section readers below read its directory through getSection.
    m_sdb.Load( std::move( bytes ) );

    m_version     = m_sdb.Version();
    m_originX     = m_sdb.Coords().OriginX();
    m_originY     = m_sdb.Coords().OriginY();
    m_originFound = m_sdb.Coords().Found();

    m_parameters.origin.x = static_cast<double>( m_originX );
    m_parameters.origin.y = static_cast<double>( m_originY );

    parseBoardSetup();
    parseMetadataRegion();
    parsePartPlacements();
    parseClusters();
    parseSection19Parts();
    parsePadStacks();
    parseDecalNameTable();
    parsePartTypeTable();
    parsePartDecals();
    parseBoardOutlineDrwOrigin();
    parseBoardOutline();
    parseNetNames();
    parseNetClasses();
    parseDiffPairs();
    parseTextRecords();
    parseRouteVertices();
    computeSec12Base();
    buildOwnerRuns();
    parseKeepouts();
    parseCopperShapes();
    parseCopperPours();
    parseDimensions();
    parseLayerStackup();
    linkPartsToDecals();

    // Filter out parts with empty ref des
    m_parts.erase( std::remove_if( m_parts.begin(), m_parts.end(),
                                   []( const PART& p ) { return p.name.empty(); } ),
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
    const SDB_SECTION* setup = getSection( SECTION::BoardSetup );

    if( !setup || setup->totalBytes < 160 )
        return;

    // A directory that declares a section larger than the file must not abort the import;
    // skip gracefully and fall back to the default layer count, as every other reader does.
    if( setup->End() > m_data.size() )
        return;

    // The board-setup section holds u32 parameters at fixed word offsets; word 4 is the
    // maximum layer count. The coordinate origin (also in this section) is already read
    // by the SDB and applied in Parse().
    uint32_t maxLayer = m_sdb.RecordAt( setup->dataOffset ).U32( 16 );

    if( maxLayer >= 1 && maxLayer <= 64 )
        m_parameters.layer_count = static_cast<int>( maxLayer );
    else
        m_parameters.layer_count = 2;

    // Binary coordinates are in BASIC units (1 BASIC = 1/38100 mil).
    // Set MILS for the display unit; actual coordinate handling uses BASIC mode
    // in the wrapper via SetBasicUnitsMode(true).
    m_parameters.units = UNIT_TYPE::MILS;
}


void BINARY_PARSER::parsePartPlacements()
{
    const SDB_SECTION* entry = getSection( SECTION::Placements );

    if( !entry || entry->count == 0 || entry->perItem == 0 )
        return;

    if( entry->End() > m_data.size() )
        return;

    const PLACEMENT_LAYOUT& layout = placementLayout( m_version );
    bool                    isOld = isOldFormat();
    uint32_t                recSize = entry->perItem;

    // Highest byte touched by a record: refdes..+16, X/Y/angle, and the side flag at
    // nameOff+28. The old framing reads to nameOff+29 (104..105), past its 96 B stride.
    size_t fieldSpan = std::max<size_t>( recSize, static_cast<size_t>( layout.nameOff ) + 29 );

    for( uint32_t i = 0; i < entry->count; ++i )
    {
        size_t off = static_cast<size_t>( i ) * recSize;

        // fieldSpan covers the full record (it is max(recSize, ...)), so this single guard
        // also rejects a directory whose perItem is smaller than the fields we read, keeping
        // a count/size-skewed file from pulling bytes beyond the record into a placement.
        if( off + fieldSpan > entry->totalBytes )
            break;

        SDB_RECORD  rec    = m_sdb.Record( *entry, i, recSize );
        std::string refDes = rec.Str( layout.nameOff, 16 );

        if( refDes.empty() || !std::isalnum( static_cast<unsigned char>( refDes[0] ) ) )
            continue;

        // Coordinates are kept raw here; the origin is applied downstream by the IO layer.
        PART part;
        part.name = refDes;
        part.location.x = toBasicCoordX( rec.I32( layout.xOff ) );
        part.location.y = toBasicCoordY( layout.yOff ? rec.I32( *layout.yOff ) : 0 );
        part.rotation = toBasicAngle( rec.I32( layout.angleOff ) );

        // Side flag is the i32 at nameOff+28 in both dialects (new: +72, old: +104);
        // bit 0 set marks a mirrored (bottom-side) placement.
        part.bottom_layer = rec.U8( layout.nameOff + 28 ) != 0;
        part.units = "M";

        // The placement's parttype index lives in the NEXT physical sec22 record's
        // @+4 field (a verified +1 block-interleave lag). Record it so linkPartsToDecals
        // can resolve the parttype-definition table and, from there, the decal name.
        if( !isOld && i + 1 < entry->count )
        {
            size_t nextOff = ( static_cast<size_t>( i ) + 1 ) * recSize;

            if( nextOff + 8 <= entry->totalBytes )
                m_partTypeIndex[m_parts.size()] = m_sdb.Record( *entry, i + 1, recSize ).U32( 4 );
        }

        // v0x2021 has no parttype layer; the decal index is selected directly from the
        // NEXT 96 B placement record's @+56 field (the same +1 block-interleave lag).
        if( layout.v2021PadChain && i + 1 < entry->count )
        {
            size_t nextOff = ( static_cast<size_t>( i ) + 1 ) * recSize;

            if( nextOff + 60 <= entry->totalBytes )
                m_partDecalIndex[m_parts.size()] = m_sdb.Record( *entry, i + 1, recSize ).U32( 56 );
        }

        // Cluster (.asc *CLUSTER*) membership is the i32 at nameOff+64 (=+108) of this
        // placement record, present only in the new 112-byte layout. It is the 1-based
        // CLSTID into the cluster table; -1 means the part is in no cluster. Record it
        // under the new part's index before the push, mirroring m_partTypeIndex above.
        if( !isOld && layout.nameOff == 44
            && off + static_cast<size_t>( layout.nameOff ) + 68 <= entry->totalBytes )
        {
            int32_t clstid = rec.I32( layout.nameOff + 64 );

            if( clstid > 0 )
                m_partClusterId[m_parts.size()] = clstid;
        }

        m_parts.push_back( part );
    }
}


void BINARY_PARSER::parseClusters()
{
    if( isOldFormat() )
        return;

    const SDB_SECTION* sec68 = getSection( SECTION::Clusters );

    if( !sec68 || sec68->count == 0 || sec68->perItem != 60 )
        return;

    static constexpr size_t  REC_SIZE     = 60;
    static constexpr size_t  LAYER_STRIDE = 152;
    static const uint8_t     TOP_FRAME[12] = { 0, 0, 0, 0, 1, 0, 0, 0, 'T', 'o', 'p', 0 };

    auto match = std::search( m_data.begin(), m_data.end(), TOP_FRAME, TOP_FRAME + 12 );

    if( match == m_data.end() )
        return;

    // The anchor must be unambiguous. It is corpus-proven 0/1 per file, but never trust a
    // second occurrence: a duplicate means this heuristic-free locator is unsafe, so bail.
    if( std::search( match + 1, m_data.end(), TOP_FRAME, TOP_FRAME + 12 ) != m_data.end() )
        return;

    size_t matchOff = static_cast<size_t>( match - m_data.begin() );

    // Compute the base with division rather than multiplication so a malformed count cannot
    // overflow/underflow: sec69_rec0 = match - 152, then require count*60 <= sec69_rec0.
    if( matchOff < LAYER_STRIDE )
        return;

    size_t sec69Rec0 = matchOff - LAYER_STRIDE;

    if( sec68->count > sec69Rec0 / REC_SIZE )
        return;

    size_t base = sec69Rec0 - static_cast<size_t>( sec68->count ) * REC_SIZE;

    for( uint32_t i = 0; i < sec68->count; ++i )
    {
        SDB_RECORD   rec = m_sdb.RecordAt( base + i * REC_SIZE );
        PART_CLUSTER cluster;
        cluster.name = rec.Str( 0, 16 );
        cluster.id = static_cast<int>( i ) + 1;   // ordinal == CLSTID
        m_clusters.push_back( std::move( cluster ) );
    }
}


void BINARY_PARSER::parseSection19Parts()
{
    // Recover the parts OMITTED from section 22 (connectors, mounting/tooling holes, test
    // points, fiducials, a few passives). They are a physically contiguous run of full
    // placement records in the tail of the sec19/sec21 region - but the run is NOT always
    // adjacent to sec22.dataOffset (0-12 gap/sentinel records can sit between it and sec22),
    // and the per-record marker bytes (i32@+0, 0xFEFF@feffOff) are non-uniform across
    // test-points/fiducials/edge-fingers, so they cannot gate the walk.
    //
    // Locate the run by an asymmetric gap-tolerant back-walk from sec22.dataOffset that SCORES
    // each stride-aligned window. The decisive discriminator is the coordinate pair: a real
    // placement has BOTH |X| and |Y| in design range; every gap/sentinel/ASCII-as-coord
    // phantom fails it. The 0xFEFF and i32@+0 bytes are weak hints (+1), never gates. The walk
    // tolerates leading gap records to FIND the block but stops at the first non-member once it
    // starts (the block is internally contiguous). Validated on 178 new-format + v2021 boards,
    // 0 drops / 0 phantoms, against 40 paired .asc; see det-specs/sec19_robust.md. v2017/2019/
    // 2022 record field offsets are unresolved, so the scorer matches nothing and safely emits
    // no omitted parts for them.
    const PLACEMENT_LAYOUT& layout = placementLayout( m_version );
    const bool              isOld = isOldFormat();

    // OLD FORMAT (v2017-2022): keep the proven bounded 0xFEFF section scan (handled below). The
    // scored new-format locator recovers old-format part placements too, but not yet the v2021
    // decal-index +1 lag (e.g. CON6 on J4), so the validated scan stays for the old dialects.
    if( isOld )
    {
        parseSection19PartsOld();
        return;
    }

    const size_t       stride = 112;                 // new-format placement record stride
    const int          sideOff = layout.nameOff + 28;

    const SDB_SECTION* sec22 = getSection( SECTION::Placements );

    if( !sec22 || sec22->dataOffset < stride )
        return;

    static constexpr int64_t COORD_MIN     = 100000;
    static constexpr int64_t COORD_ABS_MAX = 1500000000;
    static constexpr int32_t ORI_UNIT      = 40500000;   // 0.5 degree (PADS stores deg * 9e6)
    static constexpr int     SCORE_PLACEMENT = 8;

    // Score a stride-aligned window: a real placement scores >= 8 (refdes 4 + coords 5 + side 1
    // + angle 1); gap/sentinel/phantom records score <= 2. The 0xFEFF and i32@+0 hints add at
    // most +1 each and never gate.
    auto score = [&]( size_t aBase ) -> int
    {
        if( !m_cursor.InBounds( aBase, stride ) )
            return -999;

        SDB_RECORD  rec = m_sdb.RecordAt( aBase );
        std::string ref = rec.Str( layout.nameOff, 16 );
        int         s = ( !ref.empty() && std::isalnum( static_cast<unsigned char>( ref[0] ) ) ) ? 4 : -4;

        int64_t x = std::llabs( static_cast<int64_t>( rec.I32( layout.xOff ) ) );
        int64_t y = std::llabs( static_cast<int64_t>( layout.yOff ? rec.I32( *layout.yOff ) : 0 ) );
        s += ( x >= COORD_MIN && x < COORD_ABS_MAX && y >= COORD_MIN && y < COORD_ABS_MAX ) ? 5 : -5;

        int32_t side = rec.I32( sideOff );
        s += ( side == 0 || side == 1 ) ? 1 : -1;

        int32_t ori = rec.I32( layout.angleOff );
        bool    angOk = ( ori == 0 );

        if( !angOk && ori > 0 && ori <= 700000000 )
        {
            int32_t r = ori % ORI_UNIT;
            angOk = ( r <= 200000 || ( ORI_UNIT - r ) <= 200000 );
        }

        s += angOk ? 1 : -1;

        if( m_cursor.InBounds( aBase + layout.feffOff, 2 ) && m_data[aBase + layout.feffOff] == 0xFE
            && m_data[aBase + layout.feffOff + 1] == 0xFF )
            s += 1;

        if( !isOld && rec.I32( 0 ) == 0 )
            s += 1;

        return s;
    };

    // Asymmetric gap-tolerant back-walk: skip up to 12 trailing gap records to FIND the block,
    // then stop at the first non-member once it starts.
    std::vector<size_t> bases;
    bool                started = false;
    int                 preGap = 0;

    for( int k = 1; k <= 60; ++k )
    {
        if( sec22->dataOffset < static_cast<size_t>( k ) * stride )
            break;

        size_t base = sec22->dataOffset - static_cast<size_t>( k ) * stride;

        if( score( base ) >= SCORE_PLACEMENT )
        {
            bases.push_back( base );
            started = true;
        }
        else if( !started )
        {
            if( ++preGap > 12 )
                break;
        }
        else
        {
            break;
        }
    }

    std::reverse( bases.begin(), bases.end() );   // restore file order

    std::unordered_set<std::string> existingRefs;

    for( const PART& p : m_parts )
        existingRefs.insert( p.name );

    for( size_t base : bases )
    {
        SDB_RECORD  rec = m_sdb.RecordAt( base );
        std::string refDes = rec.Str( layout.nameOff, 16 );

        if( refDes.empty() || existingRefs.count( refDes ) )
            continue;

        PART part;
        part.name = refDes;
        part.location.x = toBasicCoordX( rec.I32( layout.xOff ) );
        part.location.y = layout.yOff ? toBasicCoordY( rec.I32( *layout.yOff ) ) : 0;
        part.rotation = toBasicAngle( rec.I32( layout.angleOff ) );
        part.bottom_layer = rec.U8( sideOff ) != 0;
        part.units = "M";

        // The +1 block-interleave lag: parttype index (new) / decal index (v2021) lives in the
        // NEXT physical record. Trust it only when that record is itself a placement (another
        // block element, or sec22's first record) so a gap record above the block cannot supply
        // a bogus index.
        size_t next = base + stride;
        bool   nextIsPlacement = ( next == sec22->dataOffset ) || ( score( next ) >= SCORE_PLACEMENT );

        if( nextIsPlacement && !isOld && m_cursor.InBounds( next + 4, 4 ) )
            m_partTypeIndex[m_parts.size()] = m_sdb.RecordAt( next + 4 ).U32( 0 );
        else if( nextIsPlacement && layout.v2021PadChain && m_cursor.InBounds( next + 56, 4 ) )
            m_partDecalIndex[m_parts.size()] = m_sdb.RecordAt( next + 56 ).U32( 0 );

        m_parts.push_back( std::move( part ) );
        existingRefs.insert( refDes );
    }
}


void BINARY_PARSER::parseSection19PartsOld()
{
    // Old-format (v0x2017-2022) omitted placements: a bounded 0xFEFF marker walk over the
    // sec19/sec21 directory ranges (NOT a whole-file scan). Kept for the old dialects because
    // the scored new-format locator in parseSection19Parts does not yet resolve the v2021
    // decal-index +1 lag. v2021 carries its placements as a contiguous 96 B run whose first
    // (anchor) block has no 0xFEFF marker, handled as a one-shot leading-block recovery.
    const PLACEMENT_LAYOUT& layout = placementLayout( m_version );

    static constexpr int OLD_REC_SIZE = 96;
    int    recSize = layout.scanStride;
    size_t fieldSpan = std::max<size_t>( recSize, static_cast<size_t>( layout.nameOff ) + 29 );

    std::unordered_set<std::string> existingRefs;

    for( const auto& p : m_parts )
        existingRefs.insert( p.name );

    for( int secIdx : { 19, 21 } )
    {
        const SDB_SECTION* entry = getSection( secIdx );

        if( !entry || entry->totalBytes == 0 )
            continue;

        if( entry->End() > m_data.size() )
            continue;

        uint32_t size = entry->totalBytes;

        bool oldLeadingHandled = !layout.v2021PadChain;

        for( size_t pos = 0; pos + 1 < size; ++pos )
        {
            size_t markerBase = static_cast<size_t>( entry->dataOffset ) + pos;

            if( m_data[markerBase] != 0xFE || m_data[markerBase + 1] != 0xFF )
                continue;

            if( markerBase < static_cast<size_t>( layout.feffOff ) )
                continue;

            size_t base = markerBase - static_cast<size_t>( layout.feffOff );

            if( !m_cursor.InBounds( base, fieldSpan ) )
                continue;

            std::string refDes = m_sdb.RecordAt( base ).Str( layout.nameOff, 16 );

            if( refDes.empty() || !std::isalnum( static_cast<unsigned char>( refDes[0] ) ) )
                continue;

            // Emit the leading (anchor) block once, only after the first genuine placement is
            // confirmed. Its decal index is in this first marked block's @+56 (the +1 lag).
            if( !oldLeadingHandled )
            {
                oldLeadingHandled = true;

                if( base >= OLD_REC_SIZE )
                {
                    size_t      leadBase = base - OLD_REC_SIZE;
                    SDB_RECORD  leadRec = m_sdb.RecordAt( leadBase );
                    std::string leadRef = leadRec.Str( layout.nameOff, 16 );

                    if( m_cursor.InBounds( leadBase, fieldSpan ) && !leadRef.empty()
                        && std::isalnum( static_cast<unsigned char>( leadRef[0] ) )
                        && !existingRefs.count( leadRef ) )
                    {
                        PART lead;
                        lead.name = leadRef;
                        lead.location.x = toBasicCoordX( leadRec.I32( layout.xOff ) );
                        lead.location.y = layout.yOff
                                ? toBasicCoordY( leadRec.I32( *layout.yOff ) ) : 0;
                        lead.rotation = toBasicAngle( leadRec.I32( layout.angleOff ) );
                        lead.bottom_layer = leadRec.U8( layout.nameOff + 28 ) != 0;
                        lead.units = "M";

                        size_t leadField = base + 56;

                        if( m_cursor.InBounds( leadField, 4 ) )
                            m_partDecalIndex[m_parts.size()] = m_sdb.RecordAt( leadField ).U32( 0 );

                        m_parts.push_back( lead );
                        existingRefs.insert( leadRef );
                    }
                }
            }

            if( existingRefs.count( refDes ) )
                continue;

            SDB_RECORD partRec = m_sdb.RecordAt( base );

            PART part;
            part.name = refDes;
            part.location.x = toBasicCoordX( partRec.I32( layout.xOff ) );
            part.location.y = layout.yOff ? toBasicCoordY( partRec.I32( *layout.yOff ) ) : 0;
            part.rotation = toBasicAngle( partRec.I32( layout.angleOff ) );
            part.bottom_layer = partRec.U8( layout.nameOff + 28 ) != 0;
            part.units = "M";

            // The decal index follows the +1 block lag, in the NEXT 96 B block's @+56. Gate on
            // that block's own 0xFEFF marker so trailing section data cannot supply a bogus index.
            if( layout.v2021PadChain )
            {
                size_t nextMarker = markerBase + OLD_REC_SIZE;
                size_t nextField = base + OLD_REC_SIZE + 56;

                if( m_cursor.InBounds( nextMarker, 2 ) && m_data[nextMarker] == 0xFE
                    && m_data[nextMarker + 1] == 0xFF && m_cursor.InBounds( nextField, 4 ) )
                {
                    m_partDecalIndex[m_parts.size()] = m_sdb.RecordAt( nextField ).U32( 0 );
                }
            }

            m_parts.push_back( part );
            existingRefs.insert( refDes );
        }
    }
}


void BINARY_PARSER::parsePadStacks()
{
    const SDB_SECTION* entry = getSection( SECTION::PadStacks );

    if( !entry || entry->count == 0 || entry->perItem == 0 )
        return;

    if( entry->End() > m_data.size() )
        return;

    const PADSTACK_LAYOUT& layout = padstackLayout( isOldFormat() );
    uint32_t               recSize = entry->perItem;

    // Read one 0xFE-marked padstack record's default (layer 0) geometry. For finger pads
    // (RF, OF, RC) finLength is the second dimension; round (R) and square (S) use sizeA.
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
        psl.layer = 0;
        psl.shape = shapeName;
        psl.sizeA = static_cast<double>( padWidth );
        psl.drill = static_cast<double>( drill );
        psl.plated = ( drill > 0 );
        psl.rotation = toBasicAngle( rec.I32( layout.angleOff ) );

        bool isFinger = ( shapeName == "RF" || shapeName == "OF" || shapeName == "RC" );

        if( isFinger && finLength > 0 )
            psl.sizeB = static_cast<double>( finLength );
        else
            psl.sizeB = static_cast<double>( padWidth );

        return psl;
    };

    // We store pad stacks indexed by their position in section 4.
    // Part decals reference these by index.
    for( uint32_t i = 0; i < entry->count; ++i )
    {
        if( ( i + 1 ) * recSize > entry->totalBytes )
            break;

        uint32_t base = entry->dataOffset + i * recSize;

        // Only process valid pad definitions (marker == 0xFE)
        if( m_sdb.RecordAt( base ).U8( layout.markerOff ) != 0xFE )
            continue;

        m_padStackCache[static_cast<int>( i )].push_back( readLayer( base ) );
    }

    // The section directory points partway into the padstack table: a run of de-duplicated
    // library padstacks precedes section-4 dataOffset and the directory never indexes it.
    // Recover the true pool start by walking back over the contiguous 0xFE / shape<=3
    // records, then read the full pool 0-based from there. The per-pin (pin, ref) pairs in
    // the section-15 tail index this extended pool (see parsePerPinPadstacks).
    uint32_t head = 0;

    while( entry->dataOffset >= ( head + 1 ) * recSize )
    {
        SDB_RECORD rec = m_sdb.RecordAt( entry->dataOffset - ( head + 1 ) * recSize );

        if( rec.U8( layout.markerOff ) != 0xFE || rec.U8( layout.shapeOff ) > 3 )
            break;

        ++head;
    }

    uint32_t poolStart = entry->dataOffset - head * recSize;
    uint32_t poolCount = head + entry->count;

    m_padStackPool.assign( poolCount, {} );

    for( uint32_t i = 0; i < poolCount; ++i )
    {
        uint32_t base = poolStart + i * recSize;

        if( base + recSize > m_data.size() )
            break;

        if( m_sdb.RecordAt( base ).U8( layout.markerOff ) != 0xFE )
            continue;

        m_padStackPool[i].push_back( readLayer( base ) );
    }

    // Extract default via dimensions from the pad stack cache.
    // Via pad stacks have drill > 0. Exclude JMPVIA entries (drill > 1400000).
    std::map<std::pair<double, double>, int> viaDimCounts;

    for( const auto& [idx, layers] : m_padStackCache )
    {
        for( const auto& psl : layers )
        {
            if( psl.drill > 0 && psl.drill < 1400000.0 )
                viaDimCounts[{ psl.sizeA, psl.drill }]++;
        }
    }

    int bestCount = 0;

    for( const auto& [dims, count] : viaDimCounts )
    {
        if( count > bestCount )
        {
            bestCount = count;
            m_defaultViaSize = dims.first;
            m_defaultViaDrill = dims.second;
        }
    }
}


void BINARY_PARSER::parseDecalNameTable()
{
    // The complete decal-name table sits in a fixed-size header immediately before
    // section 14, at sec14.dataOffset - 1188 (constant across v0x2025/26/27). Each
    // record is 112 bytes with the decal NAME at +0, a 0xFFFE sentinel at +64 and the
    // terminal count at +72. Unlike section 10 this table includes vias, connectors and
    // mounting holes, and is indexed directly (base 0) by a parttype's decal_index. The
    // first record is always JMPVIA_AAAAA, which is used as an anchor sanity check.
    //
    // The +72 field is the structural per-decal terminal count for the complete library,
    // the same in-record field the v0x2021 dialect carries. We harvest it into
    // m_decalTerminalCount so passives without a section 14 descriptor get exact pad counts.
    if( isOldFormat() )
    {
        parseDecalNameTableOld();
        return;
    }

    static constexpr int DECAL_HDR_OFFSET = 1188;
    static constexpr int REC_SIZE = 112;
    static constexpr int SENTINEL_OFFSET = 64;
    static constexpr int START_OFFSET = 68;
    static constexpr int COUNT_OFFSET = 72;
    static constexpr int STACK_COUNT_OFFSET = 88;

    const SDB_SECTION* sec14 = getSection( SECTION::DecalHeader );

    if( !sec14 || sec14->count == 0 || sec14->dataOffset < DECAL_HDR_OFFSET )
        return;

    uint32_t start = sec14->dataOffset - DECAL_HDR_OFFSET;

    if( start + 12 > m_data.size() || m_sdb.RecordAt( start ).Str( 0, 12 ) != "JMPVIA_AAAAA" )
        return;

    // Bound the file-supplied count before it sizes an allocation or a loop
    if( static_cast<uint64_t>( sec14->count ) * REC_SIZE > m_data.size() - start )
        THROW_IO_ERROR( "Invalid PADS decal-name table extent" );

    m_decalNameTable.clear();
    m_decalNameTable.reserve( sec14->count );

    for( uint32_t k = 0; k < sec14->count; ++k )
    {
        uint32_t   off = start + k * REC_SIZE;
        SDB_RECORD rec = m_sdb.RecordAt( off );

        if( off + REC_SIZE > m_data.size() || rec.U16( SENTINEL_OFFSET ) != 0xFFFE )
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
                m_decalStackCount.emplace( name, stackCount );
        }
    }
}


void BINARY_PARSER::parseDecalNameTableOld()
{
    // The v0x2021 dialect stores no 224 B parttype-definition layer and no 1188-byte
    // sec14 header. Its complete decal-name table is a run of 100-byte records anchored
    // at the JMPVIA_AAAAA signature in the section 12 tail, each record carrying:
    //   NAME  @ +0   (null-terminated)
    //   0xFFFE sentinel @ +64   (terminates the table on the first non-matching record)
    //   terminal count @ +72   (a direct in-record field, verified exact vs the .asc)
    // The table is located by signature rather than a fixed offset because the v0x2021
    // section framing differs from the newer dialects.
    if( !isV2021PadChain() )
        return;

    static constexpr int REC_SIZE = 100;
    static constexpr int SENTINEL_OFFSET = 64;
    static constexpr int COUNT_OFFSET = 72;
    static const std::array<uint8_t, 12> SIGNATURE = { 'J', 'M', 'P', 'V', 'I', 'A',
                                                       '_', 'A', 'A', 'A', 'A', 'A' };

    // Try every signature occurrence: the table is the first one whose 100-byte run
    // validates (record 0 == JMPVIA_AAAAA with a 0xFFFE sentinel). This guards against
    // an earlier stray occurrence of the string elsewhere in the file.
    for( auto it = std::search( m_data.begin(), m_data.end(), SIGNATURE.begin(), SIGNATURE.end() );
         it != m_data.end();
         it = std::search( it + 1, m_data.end(), SIGNATURE.begin(), SIGNATURE.end() ) )
    {
        size_t start = static_cast<size_t>( std::distance( m_data.begin(), it ) );

        std::vector<std::string>        table;
        std::map<std::string, uint32_t> counts;

        for( size_t k = 0;; ++k )
        {
            size_t off = start + k * REC_SIZE;

            if( off + SENTINEL_OFFSET + 2 > m_data.size() )
                break;

            SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( off ) );

            if( rec.U16( SENTINEL_OFFSET ) != 0xFFFE )
                break;

            std::string name = rec.Str( 0, 40 );
            table.push_back( name );

            if( off + COUNT_OFFSET + 4 <= m_data.size() )
            {
                int32_t count = rec.I32( COUNT_OFFSET );

                if( !name.empty() && count > 0 && count <= 1000 )
                    counts.emplace( name, static_cast<uint32_t>( count ) );
            }
        }

        if( !table.empty() && table[0] == "JMPVIA_AAAAA" )
        {
            m_decalNameTable = std::move( table );
            m_decalTerminalCount = std::move( counts );
            return;
        }
    }
}


void BINARY_PARSER::parsePartTypeTable()
{
    // The parttype-definition table sits in a fixed-size header immediately before
    // section 17, at sec17.dataOffset - 1232 (constant across v0x2025/26/27). Each
    // record is 224 bytes; the decal_index (an index into m_decalNameTable) is at
    // payload +96. A placement's parttype index (sec22[k+1].@+4) selects a record here.
    if( isOldFormat() )
        return;

    static constexpr int PARTTYPE_HDR_OFFSET = 1232;
    static constexpr int REC_SIZE = 224;

    const SDB_SECTION* sec17 = getSection( SECTION::ParttypeDefs );

    if( !sec17 || sec17->count == 0 || sec17->dataOffset < PARTTYPE_HDR_OFFSET )
        return;

    uint32_t start = sec17->dataOffset - PARTTYPE_HDR_OFFSET;

    m_partTypeDecalIndex.clear();
    m_partTypeDecalIndex.reserve( sec17->count );

    for( uint32_t k = 0; k < sec17->count; ++k )
    {
        uint32_t off = start + k * REC_SIZE;

        if( off + 100 > m_data.size() )
        {
            m_partTypeDecalIndex.push_back( -1 );
            continue;
        }

        m_partTypeDecalIndex.push_back( m_sdb.RecordAt( off ).I32( 96 ) );
    }
}


void BINARY_PARSER::parsePartDecals()
{
    const SDB_SECTION* entry = getSection( SECTION::DrwItems );

    if( !entry || entry->count == 0 || entry->perItem == 0 )
        return;

    if( entry->End() > m_data.size() )
        return;

    bool     isNew = !isOldFormat();
    uint32_t recSize = entry->perItem;

    for( uint32_t i = 0; i < entry->count; ++i )
    {
        if( ( i + 1 ) * recSize > entry->totalBytes )
            break;

        SDB_RECORD rec = m_sdb.Record( *entry, i, recSize );

        // The decal NAME and (new format only) a unit flag: 0x4D 'M' = metric, else inch.
        std::string name = isNew ? rec.Str( 44, 32 ) : rec.Str( 28, 32 );

        if( name.empty() )
            continue;

        PART_DECAL decal;
        decal.name = name;
        decal.units = ( isNew && rec.U8( 76 ) == 0x4D ) ? "M" : "I";

        m_decals[name] = decal;
    }

    // Register every name from the complete decal-name table (sec14 header). It
    // covers connectors, mounting holes and vias that section 10 omits, so a placed
    // part whose decal lives only there still resolves to a known decal.
    for( const std::string& name : m_decalNameTable )
    {
        if( name.empty() || m_decals.count( name ) )
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
    // Multi-terminal decals (ICs, connectors, multi-pin parts) carry a descriptor in
    // section 14. Each descriptor is a 112-byte record with a 0xFFFE sentinel at +108
    // and the decal name at +44. Two fields drive terminal extraction:
    //   +0  (i32)  start index of this decal's terminals in the section 15 pool
    //   next record's @+4 (i32)  terminal count for this decal  (a +1 record lag)
    //
    // Section 15 is a flat pool of 36-byte terminal records, one per terminal across
    // all decals, with the terminal X/Y at +0/+4. A descriptor's start index selects
    // its run within that pool.
    //
    // Decals without a section 14 descriptor (passives, simple two-pad parts) get their
    // terminal count from m_decalTerminalCount (the sec14 lagged-record stream). Their
    // exact per-terminal positions are not separately indexed, so we synthesize a
    // placeholder layout of the correct count rather than fabricate a wrong index into
    // the shared pool.
    if( isOldFormat() )
    {
        parseTerminalsOld();
        return;
    }

    // Terminal positions for EVERY decal come from an explicit per-decal cursor (the +68
    // field of the decal-name header table, captured in m_decalTerminalStart) indexing a
    // unified terminal stream S = POOL33 ++ SEC15:
    //   POOL33 = the fixed 33-record (36 B each) de-dup pool in the sec14 trailer, holding
    //            the terminals of the high-volume passives that are de-duplicated out of
    //            section 15. Always exactly 33 records at (sec14.count-11)*112 + 44.
    //   SEC15  = the section-15 geometry pool (36 B records, x@+0/y@+4), zero-tail terminated.
    // A decal's terminals are the window S[start .. start+count): start < 33 selects the
    // de-dup pool, start >= 33 selects SEC15[start-33]. Coordinates are decal-LOCAL. The
    // start cursor is stored (not derivable from counts) because the pool de-duplicates
    // geometrically identical decals onto shared windows. This supersedes the section-14
    // descriptor walk (which mis-keyed the passives) and the old (0,0) placeholder.
    static constexpr int TERM_SIZE = 36;
    static constexpr int POOL_SIZE = 33;

    std::vector<std::pair<int32_t, int32_t>> stream;

    const SDB_SECTION* sec14 = getSection( SECTION::DecalHeader );

    if( sec14 && sec14->count >= 11 )
    {
        size_t poolBase = static_cast<size_t>( sec14->dataOffset )
                          + static_cast<size_t>( sec14->count - 11 ) * 112 + 44;

        for( int i = 0; i < POOL_SIZE; ++i )
        {
            size_t off = poolBase + static_cast<size_t>( i ) * TERM_SIZE;

            if( off + 8 <= m_data.size() )
            {
                SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( off ) );
                stream.emplace_back( rec.I32( 0 ), rec.I32( 4 ) );
            }
            else
            {
                stream.emplace_back( 0, 0 );
            }
        }
    }

    const SDB_SECTION* sec15 = getSection( SECTION::TerminalPool );

    if( sec15 && sec15->totalBytes > 0 && sec15->perItem == TERM_SIZE )
    {
        for( uint32_t i = 0; i < sec15->count; ++i )
        {
            if( ( i + 1 ) * TERM_SIZE > sec15->totalBytes )
                break;

            SDB_RECORD rec = m_sdb.Record( *sec15, i, TERM_SIZE );

            // The geometry block ends at the first record whose +24/+28/+32 tail is
            // non-zero (the per-save heap trailer that follows the terminal records).
            if( rec.I32( 24 ) != 0 || rec.I32( 28 ) != 0 || rec.I32( 32 ) != 0 )
                break;

            stream.emplace_back( rec.I32( 0 ), rec.I32( 4 ) );
        }
    }

    for( auto& [name, decal] : m_decals )
    {
        if( !decal.terminals.empty() )
            continue;

        auto startIt = m_decalTerminalStart.find( name );
        auto countIt = m_decalTerminalCount.find( name );

        if( startIt == m_decalTerminalStart.end() || countIt == m_decalTerminalCount.end() )
            continue;

        size_t   start = static_cast<size_t>( startIt->second );
        uint32_t count = countIt->second;

        if( start + count > stream.size() )
            continue;

        for( uint32_t t = 0; t < count; ++t )
        {
            TERMINAL term;
            term.x = toBasicCoordX( stream[start + t].first );
            term.y = toBasicCoordY( stream[start + t].second );
            term.name = std::to_string( t + 1 );
            decal.terminals.push_back( term );
        }
    }

    // Any decal still without terminals (a count-only entry whose +68 cursor is absent)
    // falls back to the count-correct placeholder layout.
    synthesizePlaceholderTerminals();
    assignDefaultPadStacks();
    parsePerPinPadstacks();
}


void BINARY_PARSER::parseTerminalsOld()
{
    // v0x2021 carries the per-decal terminal count directly in the JMPVIA-anchored
    // decal-name table (record +72), harvested into m_decalTerminalCount. Per-terminal
    // positions are not separately indexed in this dialect, so synthesize a placeholder
    // layout of the correct count rather than fabricate wrong positions. Decals with no
    // recorded count get no terminals, which yields a footprint with no fabricated pads.
    synthesizePlaceholderTerminals();
    assignDefaultPadStacks();
}


void BINARY_PARSER::synthesizePlaceholderTerminals()
{
    for( auto& [name, decal] : m_decals )
    {
        if( !decal.terminals.empty() )
            continue;

        auto countIt = m_decalTerminalCount.find( name );

        if( countIt == m_decalTerminalCount.end() )
            continue;

        uint32_t termCount = countIt->second;

        if( termCount == 0 || termCount > 1000 )
            continue;

        for( uint32_t t = 0; t < termCount; ++t )
        {
            TERMINAL term;
            term.x = 0.0;
            term.y = 0.0;
            term.name = std::to_string( t + 1 );
            decal.terminals.push_back( term );
        }
    }
}


void BINARY_PARSER::assignDefaultPadStacks()
{
    // The binary format stores all pad stacks in section 4; the decal references them by
    // convention with pad stack 0 the default for all terminals.
    for( auto& [name, decal] : m_decals )
    {
        if( decal.terminals.empty() )
            continue;

        if( m_padStackCache.count( 0 ) )
            decal.pad_stacks[0] = m_padStackCache[0];
    }
}


void BINARY_PARSER::parsePerPinPadstacks()
{
    // The per-pin padstack assignment is serialized in stable design data, not the volatile
    // object-graph heap snapshot. A flat pool of (pin, ref) pairs follows the terminal
    // records in section 15; the section-14 descriptor table slices it per decal. Each ref
    // is a direct index into the extended pad-stack pool. A (0, ref) pair sets the decal
    // default; a (pin>0, ref) pair overrides that one terminal. This recovers per-pin pad
    // geometry that the convention-based default (pad stack 0 everywhere) cannot express.
    //
    // Decals without a section-14 descriptor (the de-duplicated high-volume passives) keep
    // the convention default rather than a fabricated geometry.
    if( isOldFormat() || m_padStackPool.empty() )
        return;

    const SDB_SECTION* sec14 = getSection( SECTION::DecalHeader );
    const SDB_SECTION* sec15 = getSection( SECTION::TerminalPool );

    if( !sec14 || !sec15 || sec15->perItem != 36 )
        return;

    // The (pin, ref) pair pool begins at the first section-15 record whose +24/+28/+32 tail
    // is non-zero, the same boundary parseTerminals uses to end the terminal-geometry block.
    static constexpr size_t TERM_SIZE = 36;
    size_t end = static_cast<size_t>( sec15->dataOffset ) + sec15->totalBytes;
    size_t pairBase = end;

    for( uint32_t i = 0; i < sec15->count; ++i )
    {
        size_t recOff = static_cast<size_t>( sec15->dataOffset ) + i * TERM_SIZE;

        if( recOff + TERM_SIZE > end )
            break;

        SDB_RECORD rec = m_sdb.RecordAt( recOff );

        if( rec.I32( 24 ) != 0 || rec.I32( 28 ) != 0 || rec.I32( 32 ) != 0 )
        {
            pairBase = recOff;
            break;
        }
    }

    if( pairBase >= end )
        return;

    std::vector<std::pair<int32_t, int32_t>> pairs;

    for( size_t off = pairBase; off + 8 <= m_data.size(); off += 8 )
    {
        SDB_RECORD rec = m_sdb.RecordAt( off );
        int32_t    pin = rec.I32( 0 );
        int32_t    ref = rec.I32( 4 );

        if( pin < 0 || pin > 20000 || ref < 0 || ref > 20000 )
            break;

        pairs.emplace_back( pin, ref );
    }

    if( pairs.empty() )
        return;

    // The section-14 descriptor table is the section data itself (distinct from the -1188
    // header used for terminal positions): 112-byte records with a 0xFFFE sentinel at +108,
    // the decal NAME at +44, the pair count at +20 and the pair-pool start cursor at +88.
    static constexpr size_t DESC_SIZE = 112;
    static constexpr size_t DESC_COUNT_OFF = 20;
    static constexpr size_t DESC_NAME_OFF = 44;
    static constexpr size_t DESC_START_OFF = 88;
    static constexpr size_t DESC_SENTINEL_OFF = 108;

    std::set<std::string> descriptorDecals;

    for( uint32_t k = 0; k < sec14->count; ++k )
    {
        size_t off = static_cast<size_t>( sec14->dataOffset ) + k * DESC_SIZE;

        SDB_RECORD desc = m_sdb.RecordAt( off );

        if( off + DESC_SIZE > m_data.size() || desc.U16( DESC_SENTINEL_OFF ) != 0xFFFE )
            continue;

        std::string name = desc.Str( DESC_NAME_OFF, 41 );

        if( name.empty() )
            continue;

        auto decalIt = m_decals.find( name );

        if( decalIt == m_decals.end() )
            continue;

        descriptorDecals.insert( name );
        applyPadstackPairs( decalIt->second, pairs, desc.I32( DESC_START_OFF ),
                            desc.I32( DESC_COUNT_OFF ) );
    }

    // The de-duplicated library decals (high-volume passives, library connectors) carry no
    // section-14 descriptor. Their pair-pool start cursor lives in a trailing section-13
    // table of 112-byte records, each tagged with a 0x4D00 marker at +40 and the decal NAME
    // at +0; the start cursor is the i32 at +44. The slice length is the decal's pad-stack
    // count from the -1188 header (m_decalStackCount). They resolve through the same pair
    // pool and pad-stack pool as the descriptor decals.
    static constexpr int32_t LIB_MARKER = 0x4D00;
    static constexpr size_t  LIB_STRIDE = 112;
    static constexpr size_t  LIB_MARKER_OFF = 40;
    static constexpr size_t  LIB_START_OFF = 44;

    const SDB_SECTION* sec13 = getSection( SECTION::DecalLibrary );

    if( !sec13 || sec13->totalBytes < LIB_STRIDE )
        return;

    size_t sec13End = static_cast<size_t>( sec13->dataOffset ) + sec13->totalBytes;

    for( size_t off = sec13->dataOffset; off + LIB_START_OFF + 4 <= sec13End; off += 4 )
    {
        SDB_RECORD lib = m_sdb.RecordAt( off );

        if( lib.I32( LIB_MARKER_OFF ) != LIB_MARKER )
            continue;

        std::string name = lib.Str( 0, 40 );

        if( name.empty() || descriptorDecals.count( name ) )
            continue;

        auto decalIt = m_decals.find( name );
        auto countIt = m_decalStackCount.find( name );

        if( decalIt == m_decals.end() || countIt == m_decalStackCount.end() )
            continue;

        applyPadstackPairs( decalIt->second, pairs, lib.I32( LIB_START_OFF ),
                            countIt->second );
    }
}


void BINARY_PARSER::applyPadstackPairs( PART_DECAL&                                     aDecal,
                                        const std::vector<std::pair<int32_t, int32_t>>& aPairs,
                                        int32_t aStart, int32_t aCount )
{
    if( aStart < 0 || aCount <= 0
        || static_cast<size_t>( aStart ) + static_cast<size_t>( aCount ) > aPairs.size() )
        return;

    for( int32_t p = 0; p < aCount; ++p )
    {
        const std::pair<int32_t, int32_t>& pair = aPairs[static_cast<size_t>( aStart ) + p];

        if( pair.first < 0 || static_cast<size_t>( pair.second ) >= m_padStackPool.size()
            || m_padStackPool[pair.second].empty() )
            continue;

        aDecal.pad_stacks[pair.first] = m_padStackPool[pair.second];
    }
}


void BINARY_PARSER::parseBoardOutlineDrwOrigin()
{
    // Section 9 has a dual structure: text string pool followed by 112-byte
    // LINE item records. Each record has the format:
    //   @0:  u32 flags (0xFFFE or 0xFFFF)
    //   @4:  u32 sentinel (0xFFFFFFFF)
    //   @44: DRW/PAD name string (12 bytes, null-terminated)
    //   @88: i32 DRW absolute X origin
    //   @92: i32 DRW absolute Y origin
    //
    // The first DRW-named record is the BOARD outline. Its absolute origin
    // is needed to convert section 11 vertices from DRW-relative to binary
    // absolute coordinates.
    static constexpr int LINE_ITEM_SIZE = 112;
    static constexpr int LINE_FLAGS_OFF = 0;
    static constexpr int LINE_SENTINEL_OFF = 4;
    static constexpr int LINE_NAME_OFF = 44;
    static constexpr int LINE_DRWX_OFF = 88;
    static constexpr int LINE_DRWY_OFF = 92;

    const SDB_SECTION* entry9 = getSection( SECTION::StringPool );

    if( !entry9 || entry9->totalBytes < LINE_ITEM_SIZE )
        return;

    size_t scanEnd = entry9->dataOffset + entry9->totalBytes;

    for( size_t pos = entry9->dataOffset; pos + LINE_ITEM_SIZE <= scanEnd; ++pos )
    {
        SDB_RECORD rec = m_sdb.RecordAt( pos );
        uint32_t   flags = rec.U32( LINE_FLAGS_OFF );
        uint32_t   sentinel = rec.U32( LINE_SENTINEL_OFF );

        if( ( flags != 0xFFFE && flags != 0xFFFF ) || sentinel != 0xFFFFFFFF )
            continue;

        std::string name = rec.Str( LINE_NAME_OFF, 12 );

        if( name.size() >= 4 && name.substr( 0, 3 ) == "DRW" )
        {
            m_boardDrwOriginX = rec.I32( LINE_DRWX_OFF );
            m_boardDrwOriginY = rec.I32( LINE_DRWY_OFF );
            m_boardDrwOriginFound = true;
            return;
        }

        pos += LINE_ITEM_SIZE - 1;
    }
}


void BINARY_PARSER::parseBoardOutline()
{
    // Board outline vertices are embedded at the end of section 11 as
    // 12-byte triplets [i32 X, i32 Y, u32 0xFFFFFFFF]. The vertex region
    // occupies the tail of section 11, after the 20-byte piece descriptors.
    // Since count * perItem may equal totalBytes exactly (no mathematical
    // tail gap), we scan backward from the section end to find the vertex
    // region boundary.
    //
    // Coordinates are DRW-relative. The DRW absolute origin from section 9
    // must be added to convert them to binary absolute coordinates that the
    // converter expects.
    // Arc-laden outlines (e.g. rounded board edges) are not stored as the simple
    // section 11 vertex tail. Try the arc-aware decoder first; it only succeeds when
    // a genuine arc board outline is present and self-validates, so it never displaces
    // a correct rectilinear result.
    if( parseArcBoardOutline() )
        return;

    const SDB_SECTION* entry11 = getSection( SECTION::GraphicPieces );

    if( !entry11 || entry11->totalBytes < 12 )
        return;

    size_t secStart = entry11->dataOffset;
    size_t secEnd   = entry11->dataOffset + entry11->totalBytes;

    // Scan backward in 12-byte steps to find where the vertex region starts.
    // Each vertex has 0xFFFFFFFF at @8; piece descriptors have 0 at @8.
    size_t vtxStart = secEnd;

    for( size_t pos = secEnd - 12; pos >= secStart && pos < secEnd; pos -= 12 )
    {
        if( m_sdb.RecordAt( pos ).U32( 8 ) == 0xFFFFFFFF )
            vtxStart = pos;
        else
            break;
    }

    if( vtxStart >= secEnd )
        return;

    // Only extract the first closed polygon as the board outline.
    // The vertex region may contain additional line item vertices.
    POLYLINE outline;
    outline.layer  = 1;
    outline.width  = 0.0;
    outline.closed = true;

    // A board outline encloses area; a degenerate run that collapses to a line or point
    // is a stray graphic, not the outline. Shipping it would be worse than shipping
    // nothing, so this unvalidated tail-scan fallback drops non-2D results.
    auto isAreaOutline = []( const POLYLINE& aOutline ) -> bool
    {
        double minX = std::numeric_limits<double>::max();
        double minY = std::numeric_limits<double>::max();
        double maxX = std::numeric_limits<double>::lowest();
        double maxY = std::numeric_limits<double>::lowest();

        for( const ARC_POINT& pt : aOutline.points )
        {
            minX = std::min( minX, pt.x );
            minY = std::min( minY, pt.y );
            maxX = std::max( maxX, pt.x );
            maxY = std::max( maxY, pt.y );
        }

        return ( maxX - minX ) > 0.0 && ( maxY - minY ) > 0.0;
    };

    for( size_t pos = vtxStart; pos + 12 <= secEnd; pos += 12 )
    {
        SDB_RECORD rec = m_sdb.RecordAt( pos );
        uint32_t   sentinel = rec.U32( 8 );

        if( sentinel != 0xFFFFFFFF )
            break;

        int32_t rawX = rec.I32( 0 );
        int32_t rawY = rec.I32( 4 );

        // Convert from DRW-relative to binary absolute by adding the DRW origin
        int32_t absX = rawX;
        int32_t absY = rawY;

        if( m_boardDrwOriginFound )
        {
            absX += m_boardDrwOriginX;
            absY += m_boardDrwOriginY;
        }

        outline.points.emplace_back( toBasicCoordX( absX ), toBasicCoordY( absY ) );

        if( outline.points.size() >= 4 )
        {
            const auto& first = outline.points.front();
            const auto& last  = outline.points.back();

            if( first.x == last.x && first.y == last.y )
            {
                if( isAreaOutline( outline ) )
                    m_boardOutlines.push_back( std::move( outline ) );

                return;
            }
        }
    }

    if( outline.points.size() >= 3 && isAreaOutline( outline ) )
        m_boardOutlines.push_back( std::move( outline ) );
}


bool BINARY_PARSER::parseArcBoardOutline()
{
    // PADS stores graphic pieces (board outline, keepouts, copper-area decals) as
    // closed runs of 12-byte vertex triplets [i32 X, i32 Y, i32 attr] in sections
    // 10..12. attr == -1 marks a plain corner; attr == 0,1,2,... is the ordinal of an
    // arc corner into a parallel 20-byte arc-parameter table located geometrically by
    // findArcTable. A run may be purely rectilinear (no arcs) or arc-laden.
    //
    // There is no in-band board-vs-decal tag on the vertex run itself, so picking the
    // single board-outline run by "largest closed run" mis-fires on boards that carry
    // bigger decals or split the outline across pieces. Instead, every closed run is
    // matched against the drawing-item index in sections 8..10. Each DRW item there
    // carries its own origin and absolute bounding box; the board outline's run extent
    // equals exactly one item's local (origin-relative) bbox. A run is only accepted
    // when it matches an item bbox within a tight tolerance, and the owning item also
    // supplies the absolute origin. When nothing matches confidently the function
    // returns false and the caller falls back to the rectilinear tail decoder, so a
    // dubious run is never shipped as the outline.
    //
    // All thresholds below are structural (binary geometry sanity), not learned from
    // any ASCII reference.
    constexpr size_t VTX = 12;
    constexpr size_t ARC_REC = 20;
    constexpr int    MAX_CORNERS = 256;

    struct Vertex { int32_t x; int32_t y; int32_t attr; };

    struct DrawingItem
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

    struct RunCandidate
    {
        std::vector<Vertex> verts;
        std::vector<size_t> arcCornerIdx;
        size_t              arcTableOffset = 0;
        int64_t             span = 0;
        DrawingItem         owner;
        bool                haveOwner = false;
    };

    auto findArcTable = [&]( const std::vector<Vertex>& verts,
                             const std::vector<size_t>& arcIdx ) -> int64_t
    {
        size_t nArcs = arcIdx.size();

        if( nArcs == 0 )
            return -1;

        const SDB_SECTION* sec1 = getSection( SECTION::BoardSetup );
        size_t          scanStart = sec1 ? sec1->dataOffset : 0;

        if( m_data.size() < ARC_REC * nArcs )
            return -1;

        size_t scanEnd = m_data.size() - ARC_REC * nArcs;

        for( size_t off = scanStart; off <= scanEnd; ++off )
        {
            bool ok = true;

            for( size_t i = 0; i < nArcs && ok; ++i )
            {
                size_t     rec = off + i * ARC_REC;
                SDB_RECORD r = m_sdb.RecordAt( rec );
                double     xmin = r.I32( 0 );
                double     ymin = r.I32( 4 );
                double     xmax = r.I32( 8 );
                double     ymax = r.I32( 12 );
                double     rx = ( xmax - xmin ) / 2.0;
                double ry = ( ymax - ymin ) / 2.0;

                // A circle box is square and non-degenerate.
                if( rx < 1.0 || std::abs( rx - ry ) > 2.0 )
                {
                    ok = false;
                    break;
                }

                double cx = ( xmin + xmax ) / 2.0;
                double cy = ( ymin + ymax ) / 2.0;
                size_t j = arcIdx[i];
                const Vertex& s = verts[j];
                const Vertex& e = verts[( j + 1 ) % verts.size()];
                double tol = std::max( 3.0, rx * 1e-5 );
                double ds = std::hypot( s.x - cx, s.y - cy );
                double de = std::hypot( e.x - cx, e.y - cy );

                if( std::abs( ds - rx ) > tol || std::abs( de - rx ) > tol )
                    ok = false;
            }

            if( ok )
                return static_cast<int64_t>( off );
        }

        return -1;
    };

    // Build the drawing-item index from the 112-byte DRW records in sections 8..10.
    // Each record stores an absolute origin at +88/+92 and an absolute bbox at
    // +96..+108; the bbox is stored origin-relative here so it can be compared
    // directly against a run's local vertex extent. The +84 type word distinguishes
    // the board outline item (0x00004D00) from other DRW pieces, used only as a tie
    // break when several items match a run equally well.
    auto collectDrawingItems = [&]() -> std::vector<DrawingItem>
    {
        std::vector<DrawingItem> items;
        const SDB_SECTION* s8 = getSection( SECTION::FreeText );
        const SDB_SECTION* s10 = getSection( SECTION::DrwItems );

        if( !s8 || !s10 )
            return items;

        size_t start = s8->dataOffset;
        size_t end = s10->dataOffset + s10->totalBytes;

        for( size_t pos = start; pos + 112 <= end && pos + 112 <= m_data.size(); )
        {
            SDB_RECORD rec = m_sdb.RecordAt( pos );
            uint16_t   marker = rec.U16( 0 );

            if( ( marker == 0xFFFE || marker == 0xFFFF ) && rec.U16( 2 ) == 0 )
            {
                std::string name = rec.Str( 44, 12 );

                if( name.size() >= 3 && name.compare( 0, 3, "DRW" ) == 0 )
                {
                    DrawingItem item;
                    item.originX = rec.I32( 88 );
                    item.originY = rec.I32( 92 );
                    item.localMinX = (int64_t) rec.I32( 96 ) - item.originX;
                    item.localMinY = (int64_t) rec.I32( 100 ) - item.originY;
                    item.localMaxX = (int64_t) rec.I32( 104 ) - item.originX;
                    item.localMaxY = (int64_t) rec.I32( 108 ) - item.originY;
                    item.span = std::max( item.localMaxX - item.localMinX,
                                          item.localMaxY - item.localMinY );
                    item.preferred = rec.U32( 84 ) == 0x00004D00;
                    items.push_back( item );
                    pos += 112;
                    continue;
                }
            }

            ++pos;
        }

        return items;
    };

    std::vector<DrawingItem> drawingItems = collectDrawingItems();

    if( drawingItems.empty() )
        return false;

    int64_t placeCenterX = 0;
    int64_t placeCenterY = 0;
    int64_t placeW = 0;
    int64_t placeH = 0;
    int     nPlacedParts = 0;
    {
        int64_t pminX = std::numeric_limits<int64_t>::max();
        int64_t pminY = std::numeric_limits<int64_t>::max();
        int64_t pmaxX = std::numeric_limits<int64_t>::lowest();
        int64_t pmaxY = std::numeric_limits<int64_t>::lowest();

        for( const PART& p : m_parts )
        {
            if( p.location.x == 0 && p.location.y == 0 )
                continue;

            pminX = std::min( pminX, (int64_t) p.location.x );
            pminY = std::min( pminY, (int64_t) p.location.y );
            pmaxX = std::max( pmaxX, (int64_t) p.location.x );
            pmaxY = std::max( pmaxY, (int64_t) p.location.y );
            ++nPlacedParts;
        }

        if( nPlacedParts >= 2 )
        {
            placeCenterX = ( pminX + pmaxX ) / 2;
            placeCenterY = ( pminY + pmaxY ) / 2;
            placeW = pmaxX - pminX;
            placeH = pmaxY - pminY;
        }
    }

    RunCandidate best;
    bool    haveBest = false;
    bool    bestPreferred = false;
    int64_t bestOwnerSpan = 0;
    int64_t bestErr = std::numeric_limits<int64_t>::max();
    double  bestCenterDist = std::numeric_limits<double>::max();

    std::vector<Vertex> verts;
    std::vector<size_t> arcIdx;

    for( int si : { 10, 11, 12 } )
    {
        const SDB_SECTION* sec = getSection( si );

        if( !sec || sec->totalBytes < VTX )
            continue;

        size_t base = sec->dataOffset;
        size_t end = base + sec->totalBytes;

        for( size_t off = base; off + VTX <= end; ++off )
        {
            verts.clear();
            arcIdx.clear();
            int32_t nextArc = 0;
            bool valid = true;
            bool closed = false;

            for( size_t p = off; p + VTX <= end && (int) verts.size() < MAX_CORNERS; p += VTX )
            {
                SDB_RECORD r = m_sdb.RecordAt( p );
                Vertex     v{ r.I32( 0 ), r.I32( 4 ), r.I32( 8 ) };

                if( verts.size() >= 3 && verts.front().x == v.x && verts.front().y == v.y )
                {
                    verts.push_back( v );
                    closed = true;
                    break;
                }

                if( v.attr != -1 )
                {
                    if( v.attr != nextArc )
                    {
                        valid = false;
                        break;
                    }

                    arcIdx.push_back( verts.size() );
                    ++nextArc;
                }

                verts.push_back( v );
            }

            if( !valid || !closed )
                continue;

            int32_t minX = std::numeric_limits<int32_t>::max();
            int32_t minY = std::numeric_limits<int32_t>::max();
            int32_t maxX = std::numeric_limits<int32_t>::lowest();
            int32_t maxY = std::numeric_limits<int32_t>::lowest();

            for( const Vertex& v : verts )
            {
                minX = std::min( minX, v.x );
                minY = std::min( minY, v.y );
                maxX = std::max( maxX, v.x );
                maxY = std::max( maxY, v.y );
            }

            int64_t runW = (int64_t) maxX - minX;
            int64_t runH = (int64_t) maxY - minY;
            int64_t span = std::max( runW, runH );

            // Reject degenerate sub-micron runs and absurd coordinate noise before the
            // more expensive arc-table and item-bbox checks. A board outline encloses
            // area, so a run that collapses to a line in either axis is a stray graphic
            // (dimension, centerline) rather than the outline. The upper bound is only an
            // overflow guard (3.3 m, far past any real PCB); the DRW-bbox match below is
            // the real size check.
            if( span < 1000000 || span > 5000000000LL || runW <= 0 || runH <= 0 )
                continue;

            // Matched DRW records also include closed title-strip and dimension pieces.
            // Those are valid graphics but much thinner than an importable board area.
            if( (double) span / (double) std::min( runW, runH ) > 12.0 )
                continue;

            int64_t arcTable = -1;

            if( !arcIdx.empty() )
            {
                arcTable = findArcTable( verts, arcIdx );

                if( arcTable < 0 )
                    continue;
            }

            // Accept this run only when its extent coincides with a drawing item's
            // bbox. The tolerance absorbs arc bulge beyond the chord vertices and
            // rounding; it is far smaller than the gap between any real piece and an
            // unrelated coordinate run.
            for( const DrawingItem& item : drawingItems )
            {
                int64_t err = std::abs( item.localMinX - minX )
                              + std::abs( item.localMinY - minY )
                              + std::abs( item.localMaxX - maxX )
                              + std::abs( item.localMaxY - maxY );

                if( err > 3000000 )
                    continue;

                double centerDist = 0.0;

                if( nPlacedParts >= 2 && placeW > 0 && placeH > 0
                    && placeW <= span * 4 && placeH <= span * 4 )
                {
                    double cx = ( (double) minX + maxX ) / 2.0 + item.originX;
                    double cy = ( (double) minY + maxY ) / 2.0 + item.originY;
                    centerDist = std::hypot( cx - placeCenterX, cy - placeCenterY );
                }

                // Among matching runs prefer the explicit board-outline item type, then
                // the larger owning DRW item, then the larger run, then the tighter fit.
                // The owner bbox and vertices share the same local coordinate frame, so
                // this stays independent of still-incomplete unrelated streams. Placement
                // center is only a final duplicate-owner tie-break when its span is sane.
                bool replace = !haveBest;

                if( haveBest )
                {
                    if( item.preferred != bestPreferred )
                        replace = item.preferred;
                    else if( item.span != bestOwnerSpan )
                        replace = item.span > bestOwnerSpan;
                    else if( span != best.span )
                        replace = span > best.span;
                    else
                    {
                        if( err != bestErr )
                            replace = err < bestErr;
                        else
                            replace = centerDist < bestCenterDist;
                    }
                }

                if( replace )
                {
                    best.verts = verts;
                    best.arcCornerIdx = arcIdx;
                    best.arcTableOffset = arcTable >= 0 ? static_cast<size_t>( arcTable ) : 0;
                    best.span = span;
                    best.owner = item;
                    best.haveOwner = true;
                    bestPreferred = item.preferred;
                    bestOwnerSpan = item.span;
                    bestErr = err;
                    bestCenterDist = centerDist;
                    haveBest = true;
                }
            }
        }
    }

    // No run matched a drawing item closely enough; let the caller try its fallback
    // rather than ship a guessed outline.
    if( !haveBest || !best.haveOwner )
        return false;

    std::vector<bool> isArcStart( best.verts.size(), false );
    std::vector<size_t> arcRecOf( best.verts.size(), 0 );

    for( size_t i = 0; i < best.arcCornerIdx.size(); ++i )
    {
        isArcStart[best.arcCornerIdx[i]] = true;
        arcRecOf[best.arcCornerIdx[i]] = i;
    }

    POLYLINE outline;
    outline.layer = 1;
    outline.width = 0.0;
    outline.closed = true;

    size_t n = best.verts.size();

    for( size_t i = 0; i < n; ++i )
    {
        const Vertex& v = best.verts[i];

        // Vertices are local to the owning item; add the item origin to reach the binary
        // absolute frame that the converter expects (it subtracts the design origin once
        // in scaleCoord, exactly as it does for placements and the rectilinear fallback).
        double absX = static_cast<double>( v.x ) + best.owner.originX;
        double absY = static_cast<double>( v.y ) + best.owner.originY;

        if( i > 0 && isArcStart[i - 1] )
        {
            size_t     rec = best.arcTableOffset + arcRecOf[i - 1] * ARC_REC;
            SDB_RECORD r = m_sdb.RecordAt( rec );
            double     xmin = r.I32( 0 );
            double     ymin = r.I32( 4 );
            double     xmax = r.I32( 8 );
            double     ymax = r.I32( 12 );
            double     cxLocal = ( xmin + xmax ) / 2.0;
            double cyLocal = ( ymin + ymax ) / 2.0;
            double radius = ( xmax - xmin ) / 2.0;

            const Vertex& s = best.verts[i - 1];
            double startAng = std::atan2( s.y - cyLocal, s.x - cxLocal ) * 180.0 / M_PI;
            double endAng = std::atan2( v.y - cyLocal, v.x - cxLocal ) * 180.0 / M_PI;
            double delta = endAng - startAng;

            while( delta <= -180.0 )
                delta += 360.0;

            while( delta > 180.0 )
                delta -= 360.0;

            ARC arc{};
            arc.cx = cxLocal + best.owner.originX;
            arc.cy = cyLocal + best.owner.originY;
            arc.radius = radius;
            arc.start_angle = startAng;
            arc.delta_angle = delta;

            outline.points.emplace_back( absX, absY, arc );
        }
        else
        {
            outline.points.emplace_back( absX, absY );
        }
    }

    if( outline.points.size() < 3 )
        return false;

    m_boardOutlines.push_back( std::move( outline ) );
    return true;
}


bool BINARY_PARSER::isValidNetName( const std::string& aName ) const
{
    if( aName.empty() )
        return false;

    // PADS stores an internal sentinel net that holds unassigned copper/obstacles.
    // It is not a real signal and the ASCII export omits it, so drop it to match.
    if( aName == "___Unassigned_Obstacles_" )
        return false;

    if( aName == "\\" )
        return false;

    char first = aName[0];

    return std::isalpha( static_cast<unsigned char>( first ) )
           || std::isdigit( static_cast<unsigned char>( first ) )
           || first == '+' || first == '-' || first == '$' || first == '~'
           || first == '_' || first == '/' || first == '\\';
}


void BINARY_PARSER::parseNetNames()
{
    std::unordered_set<std::string> existing;

    if( !isOldFormat() )
    {
        // Section 23 net record (424 B): the net NAME plus the two object pointers that
        // drive net-class membership (the class owner, shared by every member) and the
        // diff-pair join (the net's own self-pointer, value-equal to a sec49 DIF_PAIR's
        // +12/+16 member fields).
        constexpr uint32_t NET_RECORD_SIZE = 424;
        constexpr uint32_t NET_NAME        = 116;
        constexpr uint32_t NET_NAME_LEN    = 48;
        constexpr uint32_t NET_SELF_PTR    = 184;
        constexpr uint32_t NET_CLASS_PTR   = 188;

        const SDB_SECTION* nets = getSection( SECTION::Nets );

        if( nets && nets->count > 0 && nets->perItem == NET_RECORD_SIZE )
        {
            for( uint32_t i = 0; i < nets->count; ++i )
            {
                if( ( i + 1 ) * NET_RECORD_SIZE > nets->totalBytes )
                    break;

                SDB_RECORD  rec  = m_sdb.Record( *nets, i, NET_RECORD_SIZE );
                std::string name = rec.Str( NET_NAME, NET_NAME_LEN );

                if( name.empty() || !isValidNetName( name ) || existing.count( name ) )
                    continue;

                NET net;
                net.name = name;
                m_nets.push_back( net );
                existing.insert( name );
                m_sec23IndexToNet[i] = name;

                if( uint32_t owner = rec.U32( NET_CLASS_PTR ) )
                    m_netClassOwner[name] = owner;

                if( uint32_t selfPtr = rec.U32( NET_SELF_PTR ) )
                    m_netSelfPtrToName[selfPtr] = name;
            }
        }

        // Section 22 (placements) fills in power/ground nets: each 112-byte record may
        // carry up to three net names at +28/+52/+76 (24 chars), each preceded by a
        // 4-byte net index that must look like a real index (small or the 0xFFFF.. sentinel).
        constexpr uint32_t PLACEMENT_RECORD_SIZE = 112;
        const SDB_SECTION* entry22 = getSection( SECTION::Placements );

        if( entry22 && entry22->count > 0 && entry22->perItem == PLACEMENT_RECORD_SIZE )
        {
            for( uint32_t i = 0; i < entry22->count; ++i )
            {
                if( ( i + 1 ) * PLACEMENT_RECORD_SIZE > entry22->totalBytes )
                    break;

                SDB_RECORD rec = m_sdb.Record( *entry22, i, PLACEMENT_RECORD_SIZE );

                for( uint32_t nameOff : { 28u, 52u, 76u } )
                {
                    std::string name = rec.Str( nameOff, 24 );

                    if( name.empty() || !isValidNetName( name ) || existing.count( name ) )
                        continue;

                    uint32_t netIdx = rec.U32( nameOff - 4 );

                    if( netIdx < 100000 || netIdx >= 0xFFFF0000 )
                    {
                        NET net;
                        net.name = name;
                        m_nets.push_back( net );
                        existing.insert( name );
                        break;
                    }
                }
            }
        }
    }
    else
    {
        // Old format net index table
        //
        // Route vertices in v0x2021 reference nets by a dense 0-based index stored
        // at sec60 @28 byte[1]. The master list is built from two sources in order:
        //
        //   1. sec19 "design rule" net-name records (FFFFFFFF-delimited, with the net
        //      name at a fixed +24 offset from the delimiter). These nets get the
        //      lowest indices.
        //
        //   2. sec22 + sec23 nets, sorted ascending by their stored net ID field
        //      (sec22 @8 or @56 depending on which record half, sec23 @8). These
        //      are appended after the sec19 nets.

        // Phase 1: collect sec19 net names in file order
        std::vector<std::string> sec19Nets;
        const SDB_SECTION*          entry19 = getSection( SECTION::PartPins );

        if( entry19 && entry19->count > 0 )
        {
            if( entry19->End() <= m_data.size() )
            {
                size_t sec19Size = entry19->totalBytes;

                // Sec19 net-rule records live in the tail of the section at 144-byte
                // intervals, each preceded by FFFFFFFF. The u32 at +4 is zero for net
                // records (non-zero for menu items and component refs), and the net
                // name string starts at +24.
                for( size_t pos = 0; pos + 28 < sec19Size; ++pos )
                {
                    SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( entry19->dataOffset + pos ) );

                    if( rec.U32( 0 ) != 0xFFFFFFFF )
                        continue;

                    if( rec.U32( 4 ) != 0 )
                    {
                        pos += 3;
                        continue;
                    }

                    if( pos + 72 > sec19Size )
                    {
                        pos += 3;
                        continue;
                    }

                    std::string name = rec.Str( 24, 48 );

                    if( !name.empty() && isValidNetName( name ) && !existing.count( name ) )
                    {
                        sec19Nets.push_back( name );
                        existing.insert( name );
                    }

                    pos += 3;
                }
            }
        }

        // Phase 2: collect sec22 + sec23 nets with their stored IDs
        struct IndexedNet
        {
            uint32_t    storedIdx;
            std::string name;
        };

        std::vector<IndexedNet> indexedNets;

        // Old-format placement record (96 B): up to two net names at +12/+60, each
        // preceded by a 4-byte stored net index.
        constexpr uint32_t OLD_PLACEMENT_SIZE = 96;
        const SDB_SECTION* entry22 = getSection( SECTION::Placements );

        if( entry22 && entry22->count > 0 && entry22->perItem == OLD_PLACEMENT_SIZE )
        {
            for( uint32_t i = 0; i < entry22->count; ++i )
            {
                if( ( i + 1 ) * OLD_PLACEMENT_SIZE > entry22->totalBytes )
                    break;

                SDB_RECORD rec = m_sdb.Record( *entry22, i, OLD_PLACEMENT_SIZE );

                for( uint32_t nameOff : { 12u, 60u } )
                {
                    std::string name = rec.Str( nameOff, 48 );

                    if( name.empty() || !isValidNetName( name ) || existing.count( name ) )
                        continue;

                    uint32_t netIdx = rec.U32( nameOff - 4 );

                    if( netIdx < 100000 )
                    {
                        indexedNets.push_back( { netIdx, name } );
                        existing.insert( name );
                        break;
                    }
                }
            }
        }

        // Old-format net record (144 B): stored net index at +8, name at +12.
        constexpr uint32_t OLD_NET_SIZE = 144;
        const SDB_SECTION* entry23 = getSection( SECTION::Nets );

        if( entry23 && entry23->count > 0 && entry23->perItem == OLD_NET_SIZE )
        {
            for( uint32_t i = 0; i < entry23->count; ++i )
            {
                if( ( i + 1 ) * OLD_NET_SIZE > entry23->totalBytes )
                    break;

                SDB_RECORD  rec    = m_sdb.Record( *entry23, i, OLD_NET_SIZE );
                uint32_t    netIdx = rec.U32( 8 );
                std::string name   = rec.Str( 12, 48 );

                if( !name.empty() && isValidNetName( name ) && netIdx < 100000
                    && !existing.count( name ) )
                {
                    indexedNets.push_back( { netIdx, name } );
                    existing.insert( name );
                }
            }
        }

        std::sort( indexedNets.begin(), indexedNets.end(),
                   []( const IndexedNet& a, const IndexedNet& b )
                   {
                       return a.storedIdx < b.storedIdx;
                   } );

        // Phase 3: build the dense net index table and emit NET objects
        uint32_t denseIdx = 0;

        for( const auto& name : sec19Nets )
        {
            m_sec23IndexToNet[denseIdx++] = name;

            NET net;
            net.name = name;
            m_nets.push_back( net );
        }

        for( const auto& entry : indexedNets )
        {
            m_sec23IndexToNet[denseIdx++] = entry.name;

            NET net;
            net.name = entry.name;
            m_nets.push_back( net );
        }
    }
}


void BINARY_PARSER::parseMetadataRegion()
{
    // The SDB locates the origin from section 1; only fall back to the
    // DFT_CONFIGURATION scan when that did not yield one.
    if( m_originFound )
        return;

    // The DFT region sits between the last section payload and the 46-byte footer.
    // The directory ends where section 1's payload begins (section 0 carries none).
    const SDB_SECTION* firstSection = getSection( 1 );
    size_t             dirEnd = firstSection ? firstSection->dataOffset : 0;
    size_t             lastDataEnd = dirEnd;

    for( size_t i = 0; i < m_sdb.SectionCount(); ++i )
    {
        const SDB_SECTION* entry = getSection( static_cast<int>( i ) );

        if( entry && entry->index > 0 && entry->totalBytes > 0 )
            lastDataEnd = std::max<size_t>( lastDataEnd, entry->End() );
    }

    constexpr size_t FOOTER_BYTES = 46;
    size_t           footerStart = m_data.size() - FOOTER_BYTES;

    if( lastDataEnd >= footerStart )
        return;

    parseDftConfig( dirEnd, footerStart );
}


void BINARY_PARSER::parseDftConfig( size_t aStart, size_t aEnd )
{
    // Search for "DFT_CONFIGURATION\0" marker
    static const char DFT_MARKER[] = "DFT_CONFIGURATION";
    size_t markerLen = std::strlen( DFT_MARKER );

    for( size_t pos = aStart; pos + markerLen + 1 < aEnd; ++pos )
    {
        if( std::memcmp( &m_data[pos], DFT_MARKER, markerLen ) == 0
            && m_data[pos + markerLen] == 0 )
        {
            size_t configStart = pos + markerLen + 1;

            // Skip PARENT markers and null bytes
            while( configStart < aEnd )
            {
                if( m_data[configStart] == 0 )
                {
                    ++configStart;
                    continue;
                }

                if( configStart + 7 <= aEnd
                    && std::memcmp( &m_data[configStart], "PARENT\0", 7 ) == 0 )
                {
                    configStart += 7;
                    continue;
                }

                break;
            }

            if( configStart >= aEnd )
                return;

            // Detect format by checking for '.' padding in the first 16 bytes
            std::map<std::string, std::string> config;
            bool hasDot = false;

            if( configStart + 16 <= aEnd )
            {
                for( size_t i = configStart; i < configStart + 16; ++i )
                {
                    if( m_data[i] == '.' )
                    {
                        hasDot = true;
                        break;
                    }
                }
            }

            if( hasDot )
                config = parseDftDotPadded( configStart, aEnd );
            else
                config = parseDftNullSeparated( configStart, aEnd );

            auto xIt = config.find( "X" );
            auto yIt = config.find( "Y" );

            if( xIt != config.end() && yIt != config.end() )
            {
                try
                {
                    m_originX = static_cast<int32_t>( std::stod( xIt->second ) );
                    m_originY = static_cast<int32_t>( std::stod( yIt->second ) );
                    m_originFound = true;

                    m_parameters.origin.x = static_cast<double>( m_originX );
                    m_parameters.origin.y = static_cast<double>( m_originY );
                }
                catch( ... )
                {
                    wxLogTrace( "PADS", "Failed to parse DFT origin values" );
                }
            }

            return;
        }
    }
}


std::map<std::string, std::string>
BINARY_PARSER::parseDftDotPadded( size_t aPos, size_t aEnd ) const
{
    std::map<std::string, std::string> config;

    while( aPos + 16 <= aEnd )
    {
        // Keys are 16-byte fields padded with ASCII '.' (0x2E)
        bool validKey = true;

        for( size_t i = aPos; i < aPos + 16; ++i )
        {
            uint8_t b = m_data[i];

            if( !( ( b >= 0x20 && b <= 0x7E ) || b == 0x00 ) )
            {
                validKey = false;
                break;
            }
        }

        if( !validKey )
            break;

        // Extract key by stripping null bytes and dot padding
        std::string key;

        for( size_t i = aPos; i < aPos + 16; ++i )
        {
            if( m_data[i] == 0 || m_data[i] == '.' )
                break;

            key += static_cast<char>( m_data[i] );
        }

        if( key.empty() )
            break;

        aPos += 16;

        // Skip optional null separator
        if( aPos < aEnd && m_data[aPos] == 0 )
            ++aPos;

        // Read null-terminated value
        size_t valStart = aPos;

        while( aPos < aEnd && m_data[aPos] != 0 )
            ++aPos;

        if( aPos > valStart )
        {
            std::string value( reinterpret_cast<const char*>( &m_data[valStart] ),
                               aPos - valStart );
            config[key] = value;
        }

        if( aPos < aEnd )
            ++aPos;

        // Skip PARENT markers
        if( aPos + 7 <= aEnd
            && std::memcmp( &m_data[aPos], "PARENT\0", 7 ) == 0 )
        {
            aPos += 7;
        }
    }

    return config;
}


std::map<std::string, std::string>
BINARY_PARSER::parseDftNullSeparated( size_t aPos, size_t aEnd ) const
{
    std::map<std::string, std::string> config;

    while( aPos < aEnd )
    {
        // Find null-terminated key
        size_t keyStart = aPos;

        while( aPos < aEnd && m_data[aPos] != 0 )
            ++aPos;

        if( aPos == keyStart )
            break;

        // Validate key is printable ASCII
        bool validKey = true;

        for( size_t i = keyStart; i < aPos; ++i )
        {
            if( m_data[i] < 0x20 || m_data[i] > 0x7E )
            {
                validKey = false;
                break;
            }
        }

        if( !validKey )
            break;

        std::string key( reinterpret_cast<const char*>( &m_data[keyStart] ), aPos - keyStart );

        // Skip null terminator
        if( aPos < aEnd )
            ++aPos;

        if( key == "PARENT" )
            continue;

        // Read null-terminated value
        size_t valStart = aPos;

        while( aPos < aEnd && m_data[aPos] != 0 )
            ++aPos;

        if( aPos <= valStart )
            break;

        std::string value( reinterpret_cast<const char*>( &m_data[valStart] ), aPos - valStart );
        config[key] = value;

        if( aPos < aEnd )
            ++aPos;
    }

    return config;
}


void BINARY_PARSER::parseNetClasses()
{
    if( m_netClassOwner.empty() || m_data.size() < 24 )
        return;

    // Distinct net-class owner pointers, ascending == net-class declaration order.
    std::set<uint32_t> ownerSet;

    for( const auto& [name, owner] : m_netClassOwner )
        ownerSet.insert( owner );

    std::vector<uint32_t>      owners( ownerSet.begin(), ownerSet.end() );
    std::map<uint32_t, size_t> ownerOrdinal;

    for( size_t k = 0; k < owners.size(); ++k )
        ownerOrdinal[owners[k]] = k;

    // Scan the trailing arena for the type-66 rule table: 24-byte records with tag 0x42
    // at +4 and a net-class owner pointer at +8 (== a net's +188). Each record carries
    // the rule-detail page at +0 (which selects the rule kind) and the layer at +20.
    struct EdgeRec
    {
        uint32_t owner;
        uint32_t page;     // rulePtr & ~0xfff, selects the rule kind
        uint32_t rulePtr;  // full rule-value-object pointer (+0); declaration order within a page
        int      layer;
        size_t   off;
    };

    std::vector<EdgeRec> edges;

    for( size_t off = 0; off + 24 <= m_data.size(); ++off )
    {
        SDB_RECORD rec = m_sdb.RecordAt( off );

        if( rec.U32( 4 ) != 0x42 )
            continue;

        uint32_t owner = rec.U32( 8 );

        if( !ownerSet.count( owner ) )
            continue;

        uint32_t rulePtr = rec.U32( 0 );

        edges.push_back( { owner, rulePtr & ~0xfffu, rulePtr,
                           static_cast<int>( rec.U32( 20 ) ), off } );
    }

    if( edges.empty() )
        return;

    // The 0x118-stride NET_CLASS name records sit just before the rule table; anchor off
    // the first rule record: name_head = first_edge - num_classes*0x118 - 0x50 (a blind
    // 0x118 ASCII scan false-positives on silkscreen text, so use this structural anchor).
    size_t firstEdge = edges.front().off;

    for( const EdgeRec& e : edges )
        firstEdge = std::min( firstEdge, e.off );

    size_t headSpan = owners.size() * 0x118 + 0x50;
    size_t nameHead = ( firstEdge >= headSpan ) ? firstEdge - headSpan : 0;

    m_netClasses.clear();
    m_netClasses.resize( owners.size() );

    for( size_t k = 0; k < owners.size(); ++k )
    {
        std::string name = m_sdb.RecordAt( nameHead + k * 0x118 ).Str( 0, 40 );

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

    // The clearance rule kind is the rule-detail page whose (class, layer) keys are all
    // unique and that spans the most classes. Record each class's clearance-rule layers.
    std::map<uint32_t, std::set<std::pair<uint32_t, int>>> pageKeys;
    std::map<uint32_t, bool>                               pageUnique;

    for( const EdgeRec& e : edges )
    {
        bool& uniq = pageUnique.try_emplace( e.page, true ).first->second;

        if( !pageKeys[e.page].insert( { e.owner, e.layer } ).second )
            uniq = false;
    }

    uint32_t clearancePage = 0;
    size_t   bestSpan = 0;
    bool     havePage = false;

    for( const auto& [page, keys] : pageKeys )
    {
        if( !pageUnique[page] )
            continue;

        std::set<uint32_t> classes;

        for( const auto& [owner, layer] : keys )
            classes.insert( owner );

        if( !havePage || classes.size() > bestSpan )
        {
            havePage = true;
            bestSpan = classes.size();
            clearancePage = page;
        }
    }

    if( havePage )
    {
        for( const EdgeRec& e : edges )
        {
            if( e.page != clearancePage )
                continue;

            auto it = ownerOrdinal.find( e.owner );

            if( it != ownerOrdinal.end() )
                m_netClasses[it->second].ruleLayers.push_back( e.layer );
        }

        // Per-class rule VALUES (clearance, track width, via clearance). The clearance-page
        // edge's full +0 pointer is the rule-value-object's declaration ordinal; the value
        // records live in a separate arena keyed by their own self-pointer at +12, with no
        // pointer chain between the two (independent malloc bases). The arenas are emitted in
        // the same declaration order, so the join is positional: the i-th layer-0 clearance
        // edge pairs with the i-th layer-0 (discriminator 1) value record.
        std::vector<const EdgeRec*> layer0Edges;

        for( const EdgeRec& e : edges )
        {
            if( e.page == clearancePage && e.layer == 0 )
                layer0Edges.push_back( &e );
        }

        std::sort( layer0Edges.begin(), layer0Edges.end(),
                   []( const EdgeRec* a, const EdgeRec* b ) { return a->rulePtr < b->rulePtr; } );

        // File-wide scan for the 457200 marker (== 12 mil, the TRACK_TO_TRACK schema default and
        // a VALUE, not a delimiter). The value arena sits in a broader MFC blob just outside the
        // sec49 directory byte-range, so the scan must cover the whole file. Discriminator 1 is a
        // layer-0 NET_CLASS rule; the int32[38] core begins at marker+20.
        struct ValueRec
        {
            uint32_t selfPtr;
            int32_t  core[38];
        };

        std::vector<ValueRec> values;

        for( size_t off = 0; off + 20 + 38 * sizeof( int32_t ) <= m_data.size(); ++off )
        {
            SDB_RECORD rec = m_sdb.RecordAt( off );

            if( rec.I32( 0 ) != 457200 )
                continue;

            uint32_t selfPtr = rec.U32( 12 );

            if( selfPtr < 0x10000000u || selfPtr >= 0x20000000u )
                continue;

            if( rec.I32( 8 ) != 1 )
                continue;

            ValueRec v{};
            v.selfPtr = selfPtr;
            bool anyNonZero = false;

            for( int i = 0; i < 38; ++i )
            {
                v.core[i] = rec.I32( 20 + sizeof( int32_t ) * i );

                if( v.core[i] != 0 )
                    anyNonZero = true;
            }

            if( anyNonZero )
                values.push_back( v );
        }

        std::sort( values.begin(), values.end(),
                   []( const ValueRec& a, const ValueRec& b ) { return a.selfPtr < b.selfPtr; } );

        // Equal counts are required for a sound positional join; otherwise leave values unset so
        // membership still ships (correct-or-silent).
        if( !layer0Edges.empty() && layer0Edges.size() == values.size() )
        {
            for( size_t i = 0; i < layer0Edges.size(); ++i )
            {
                auto it = ownerOrdinal.find( layer0Edges[i]->owner );

                if( it == ownerOrdinal.end() )
                    continue;

                const int32_t* core = values[i].core;
                NETCLASS_DEF&  nc = m_netClasses[it->second];

                nc.clearance     = core[0];
                nc.viaClearance  = core[2];
                nc.minTrackWidth = core[33];
                nc.trackWidth    = core[34];
                nc.maxTrackWidth = core[35];
                nc.hasRuleValues = true;
            }
        }
    }

    // Deterministic ordering for reproducible output.
    for( NETCLASS_DEF& nc : m_netClasses )
    {
        std::sort( nc.nets.begin(), nc.nets.end() );
        std::sort( nc.ruleLayers.begin(), nc.ruleLayers.end() );
        nc.ruleLayers.erase( std::unique( nc.ruleLayers.begin(), nc.ruleLayers.end() ),
                             nc.ruleLayers.end() );
    }
}


void BINARY_PARSER::parseDiffPairs()
{
    constexpr size_t  OBJECT_SIZE = 864;
    constexpr size_t  FF_TAIL_OFF = 604;             // 0xFF allocator free-fill begins here
    constexpr size_t  FF_TAIL_MIN = 200;             // tail validator: >=200 0xFF bytes
    constexpr double  MAX_LENGTH  = 17068800000.0;   // DIF_PAIR MAX_LENGTH default (seed marker)
    constexpr double  F64_INHERIT = -1.0;
    constexpr int32_t I32_INHERIT = -1;

    const SDB_SECTION* sec49 = getSection( SECTION::ClearanceRules );

    if( !sec49 || sec49->totalBytes == 0 || m_netSelfPtrToName.empty() )
        return;

    if( sec49->End() > m_data.size() )
        return;

    const size_t poolBase = sec49->dataOffset;
    const size_t poolSize = sec49->totalBytes;

    if( poolSize < OBJECT_SIZE )
        return;

    // The serialized DIF_PAIR objects are a packed 864-byte-stride array that MAY split across
    // several arena (malloc) chunks. There is no count word and no in-file handle->offset index
    // (reattack_carchive_objectgraph.md), so resolution is a within-file value JOIN: an object's
    // +12/+16 member-net handles equal a sec23 net record's +184 self-handle (m_netSelfPtrToName),
    // which is bijective per file. A record is a DIF_PAIR iff both handles resolve, its +32
    // MAX_LENGTH is the default or a sane override, and its tail carries the >=200-byte 0xFF
    // free-fill (a per-record VALIDATOR, never the locator). The prior "last 0xFF run minus 864"
    // anchor only caught the final chunk and silently dropped pairs (e.g. 42->3 on a UZCB board),
    // so locate every chunk by seeding on the MAX_LENGTH marker and extending 864-stride both
    // ways. See det-specs/diffpairs.md.
    auto looksDp = [&]( size_t aStart ) -> bool
    {
        if( aStart < poolBase || aStart + OBJECT_SIZE > poolBase + poolSize )
            return false;

        SDB_RECORD obj = m_sdb.RecordAt( aStart );

        if( !m_netSelfPtrToName.count( obj.U32( 12 ) ) || !m_netSelfPtrToName.count( obj.U32( 16 ) ) )
            return false;

        double maxLen = obj.F64( 32 );

        if( maxLen != MAX_LENGTH && !( maxLen > 0.0 && maxLen < 1e15 ) )
            return false;

        size_t fillBytes = 0;

        for( size_t k = aStart + FF_TAIL_OFF; k < aStart + OBJECT_SIZE; ++k )
            fillBytes += ( m_data[k] == 0xFF ) ? 1 : 0;

        return fillBytes >= FF_TAIL_MIN;
    };

    // Seeds carry the unambiguous MAX_LENGTH default (0 false positives across 132 clean v2027
    // boards); from each, extend in both directions at 864 stride so override pairs (whose +32
    // is off the default but still pass looksDp) are recovered. The objects are byte-aligned to
    // their arena chunk, not the pool, so the seed search is per-byte.
    std::set<size_t> found;

    for( size_t i = 0; i + OBJECT_SIZE <= poolSize; ++i )
    {
        size_t st = poolBase + i;

        if( m_sdb.RecordAt( st ).F64( 32 ) != MAX_LENGTH || !looksDp( st ) )
            continue;

        size_t start = st;

        while( start >= poolBase + OBJECT_SIZE && looksDp( start - OBJECT_SIZE ) )
            start -= OBJECT_SIZE;

        for( size_t o = start; looksDp( o ); o += OBJECT_SIZE )
            found.insert( o );
    }

    std::set<std::pair<std::string, std::string>> seen;

    for( size_t objStart : found )
    {
        SDB_RECORD         obj = m_sdb.RecordAt( objStart );
        const std::string& nameA = m_netSelfPtrToName.at( obj.U32( 12 ) );
        const std::string& nameB = m_netSelfPtrToName.at( obj.U32( 16 ) );

        if( !seen.insert( { nameA, nameB } ).second )
            continue;

        double  gapOverride = obj.F64( 56 );
        double  gap = ( gapOverride != F64_INHERIT ) ? gapOverride : obj.F64( 40 );
        int32_t w600 = obj.I32( 600 );
        double  width = ( w600 != I32_INHERIT ) ? static_cast<double>( w600 )
                                                : static_cast<double>( obj.I32( 592 ) );

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
    // Free-text items live in a 72-byte text-header stream that the container
    // directory splits across sections 5 and 8, sharing one packed C-string pool
    // near the top of section 8. Two structural quirks drove this layout:
    //
    //   1. The legend / title-block half of the stream sits in section 5's tail,
    //      the marker / fiducial half in section 8, so the scan must cover the
    //      combined byte range [sec5.dataOffset .. sec8.end].
    //   2. Metadata lags geometry by one record slot. Text K's GEOMETRY is in
    //      record K (height@36, width@40, X@44, Y@48, all RAW = design + origin),
    //      but its METADATA (string offset@8, layer word@24, object tag@28) is in
    //      record K+1. Reading record K's own @8 yields text K-1's string, which
    //      lands mid-string and produces fragment cascades.
    //
    // Discriminator (version-stable across 0x2025/0x2026/0x2027, validated string
    // and coordinate exact against the ASCII export): record K is a genuine text
    // item iff record K+1 has tag@28 == 0x49000000 and (word@24 >> 16) == 0x0020
    // (the high half marks a TEXT object, the low byte is the layer), and record K
    // has positive height and width.
    const SDB_SECTION* s8 = getSection( SECTION::FreeText );

    if( !s8 || s8->totalBytes == 0 || s8->perItem < 72 )
        return;

    const SDB_SECTION* s5 = getSection( SECTION::PadShapes );

    size_t lo = ( s5 && s5->totalBytes > 0 ) ? s5->dataOffset : s8->dataOffset;
    size_t hi = s8->dataOffset + s8->totalBytes;

    if( hi < lo + 144 || hi > m_data.size() )
        return;

    // The packed C-string pool starts near the top of section 8 but spills past its
    // end into section 9, a contiguous per-item-1 byte blob that the directory lays
    // down immediately after section 8. String resolution and pool-base calibration
    // must run against this extended upper bound or the legend / note strings whose
    // bytes cross the sec8/sec9 boundary get truncated. Require section 9 to be the
    // byte blob (perItem 1) directly after section 8 and bound the extension with
    // subtraction to avoid wrap.
    size_t          poolHi = hi;
    const SDB_SECTION* s9     = getSection( SECTION::StringPool );

    if( s9 && s9->totalBytes > 0 && s9->perItem == 1 && s9->dataOffset == hi
        && s9->dataOffset <= m_data.size() && s9->totalBytes <= m_data.size() - s9->dataOffset )
    {
        poolHi = s9->dataOffset + s9->totalBytes;
    }

    struct TextCand
    {
        size_t   geomBase;
        uint32_t strOffset;
        uint8_t  layer;
    };

    std::vector<TextCand> cands;

    for( size_t o = lo; o + 144 <= hi; ++o )
    {
        SDB_RECORD rec = m_sdb.RecordAt( o );

        if( rec.U32( 72 + 28 ) != 0x49000000 )
            continue;

        if( rec.U32( 72 + 12 ) != 0 )
            continue;

        uint32_t layerWord = rec.U32( 72 + 24 );

        if( ( ( layerWord >> 16 ) & 0xFFFF ) != 0x0020 )
            continue;

        if( rec.I32( 36 ) <= 0 || rec.I32( 40 ) <= 0 )
            continue;

        TextCand c;
        c.geomBase  = o;
        c.strOffset = rec.U32( 72 + 8 );
        c.layer     = static_cast<uint8_t>( layerWord & 0xFF );
        cands.push_back( c );
    }

    if( cands.empty() )
        return;

    // The string pool is in insertion order, not record order, so each text must
    // be resolved through its own string offset. The pool base is not on a clean
    // boundary; calibrate it within a window near the end of the header region by
    // choosing the base that lands the most string offsets on a C-string start.
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

    size_t winLo     = hi > 4000 ? hi - 4000 : lo;
    size_t poolBase  = hi;
    int    bestScore = -1;

    for( size_t base = winLo; base < hi; ++base )
    {
        int good = 0;

        for( const TextCand& c : cands )
        {
            size_t soff = base + c.strOffset;

            if( soff >= lo && soff < poolHi && ( soff == base || m_data[soff - 1] == 0 )
                && cStringStartAt( soff ) )
                ++good;
        }

        if( good > bestScore )
        {
            bestScore = good;
            poolBase  = base;

            if( good == static_cast<int>( cands.size() ) )
                break;
        }
    }

    for( const TextCand& c : cands )
    {
        size_t soff = poolBase + c.strOffset;

        // Only emit candidates that resolve to a real C-string inside the pool.
        // readFixedString clamps merely to the buffer end, so without this guard a
        // false candidate could read printable bytes out of a neighbouring section.
        if( soff < lo || soff >= poolHi || ( soff != poolBase && m_data[soff - 1] != 0 )
            || !cStringStartAt( soff ) )
            continue;

        std::string content = m_sdb.RecordAt( soff ).Str( 0, poolHi - soff );

        if( content.empty() )
            continue;

        SDB_RECORD geom = m_sdb.RecordAt( c.geomBase );
        int32_t    height    = geom.I32( 36 );
        int32_t    linewidth = geom.I32( 40 );
        int32_t    x         = geom.I32( 44 );
        int32_t    y         = geom.I32( 48 );
        int32_t    angleRaw  = geom.I32( 52 );

        TEXT text;
        text.content    = content;
        text.location.x = toBasicCoordX( x );
        text.location.y = toBasicCoordY( y );
        text.height     = static_cast<double>( height );
        text.width      = static_cast<double>( linewidth );
        text.layer      = static_cast<int>( c.layer );
        text.rotation   = toBasicAngle( angleRaw );

        m_texts.push_back( text );
    }
}


// Per-version field layout for the section-60 via records decoded by
// parseRouteVertices. Only v0x2021/0x2022 (old), v0x2025 and v0x2026 carry a
// recoverable via encoding; every other version selects no layout and emits no
// vias. The three encodings share one body (raw X/Y read, Y-flip about the
// origin, deviation guard, optional de-dup, optional net attribution) and differ
// only in the marker predicate, the field offsets, and the record-size gate.
struct VIA_SEC60_LAYOUT
{
    int                xOff = 0;             // raw via X
    int                yOff = 0;             // raw via Y (pre Y-flip)
    std::optional<int> netIndexOff = std::nullopt; // dense net-index byte; old dialect only
    bool               dedup = false;        // collapse repeated coordinates (new dialects only)
    uint32_t           minRecSize = 0;       // 0 = no record-size gate
    bool               exactRecSize = false; // require perItem == minRecSize (else >=)
    uint32_t           boundSize = 0;        // bytes that must be present per record (0 = use stride)
    bool               guardUsesRawY = false;// deviation guard on rawY (true) vs the narrowed vy
    bool ( *matchesMarker )( const BINARY_CURSOR&, size_t aBase ) = nullptr;
};


// v0x2021/0x2022 48-byte records. @28 = 0x0E fill type, @4/@5 frame a via slot,
// @16 = non-zero drill marker.
static bool matchViaOld( const BINARY_CURSOR& aCur, size_t aBase )
{
    return aCur.U8At( aBase + 28 ) == 0x0E && aCur.U8At( aBase + 4 ) == 0xFF
           && aCur.U8At( aBase + 5 ) != 0xFF && aCur.U32At( aBase + 16 ) != 0;
}


// v0x2025 64-byte records. 0x0E type at @50 with the 0x0217 tag at @54/@55.
static bool matchVia2025( const BINARY_CURSOR& aCur, size_t aBase )
{
    return aCur.U8At( aBase + 50 ) == 0x0E && aCur.U8At( aBase + 54 ) == 0x17
           && aCur.U8At( aBase + 55 ) == 0x02;
}


// v0x2026 (>=64 byte) records. A route-vertex type at @24 whose ordinal word at
// @44 has the 0x0E fill marker in its low byte.
static bool matchVia2026( const BINARY_CURSOR& aCur, size_t aBase )
{
    uint8_t vtxType = aCur.U8At( aBase + 24 );

    if( vtxType != 0xEF && !( vtxType >= 0xF1 && vtxType <= 0xF5 ) )
        return false;

    return ( aCur.U32At( aBase + 44 ) & 0xFF ) == 0x0E;
}


static const VIA_SEC60_LAYOUT* via60Layout( uint16_t aVersion )
{
    // Only the fields that differ from the defaults are listed, so each dialect's
    // distinguishing offsets and flags read at a glance.
    static const VIA_SEC60_LAYOUT vOld{ .xOff = 1, .yOff = 5, .netIndexOff = 29,
                                        .matchesMarker = &matchViaOld };
    static const VIA_SEC60_LAYOUT v2025{ .xOff = 23, .yOff = 27, .dedup = true, .minRecSize = 64,
                                         .exactRecSize = true, .boundSize = 64, .guardUsesRawY = true,
                                         .matchesMarker = &matchVia2025 };
    static const VIA_SEC60_LAYOUT v2026{ .xOff = 17, .yOff = 21, .dedup = true, .minRecSize = 64,
                                         .boundSize = 64, .matchesMarker = &matchVia2026 };

    if( aVersion == 0x2021 || aVersion == 0x2022 )
        return &vOld;

    if( aVersion == 0x2025 )
        return &v2025;

    if( aVersion == 0x2026 )
        return &v2026;

    return nullptr;
}


void BINARY_PARSER::parseRouteVertices()
{

    struct ViaLocation
    {
        int32_t     x = 0;
        int32_t     y = 0;
        std::string netName;
    };

    std::vector<ViaLocation> viaLocations;

    const SDB_SECTION* entry60 = getSection( SECTION::Vias );

    if( !entry60 || entry60->count == 0 || entry60->perItem == 0 || entry60->End() > m_data.size() )
        return;

    uint32_t n60 = entry60->count;
    uint32_t r60 = entry60->perItem;

    static constexpr int64_t MAX_COORD_DEVIATION = 1000000000; // ~660mm from origin

    const VIA_SEC60_LAYOUT* layout = via60Layout( m_version );

    bool gateOk = layout != nullptr;

    if( gateOk && layout->minRecSize > 0 )
        gateOk = layout->exactRecSize ? ( r60 == layout->minRecSize ) : ( r60 >= layout->minRecSize );

    if( gateOk )
    {
        // The deviation guard is symmetric about the origin, so it is evaluated on the raw
        // Y (rawY - originY) regardless of the Y-flip applied to the stored coordinate.
        size_t                                end = entry60->dataOffset + entry60->totalBytes;
        size_t                                need = layout->boundSize ? layout->boundSize : r60;
        std::set<std::pair<int32_t, int32_t>> seenVias;

        for( uint32_t vi = 0; vi < n60; ++vi )
        {
            uint32_t base = entry60->dataOffset + vi * r60;

            if( base + need > end )
                break;

            if( !layout->matchesMarker( m_cursor, base ) )
                continue;

            SDB_RECORD rec  = m_sdb.RecordAt( base );
            int32_t    vx   = rec.I32( layout->xOff );
            int32_t    rawY = rec.I32( layout->yOff );
            int32_t    vy   = static_cast<int32_t>( 2LL * m_originY - rawY );

            // Per-version: the old/0x2026 arms guarded the narrowed vy, 0x2025 the raw Y.
            // They share a magnitude only while 2*originY - rawY stays in int32_t range,
            // so reproduce each arm's source exactly rather than rely on that.
            int64_t dx = static_cast<int64_t>( vx ) - m_originX;
            int64_t dy = layout->guardUsesRawY ? ( static_cast<int64_t>( rawY ) - m_originY )
                                               : ( static_cast<int64_t>( vy ) - m_originY );

            if( std::abs( dx ) > MAX_COORD_DEVIATION || std::abs( dy ) > MAX_COORD_DEVIATION )
                continue;

            if( layout->dedup && !seenVias.insert( { vx, vy } ).second )
                continue;

            std::string netName;

            if( layout->netIndexOff )
            {
                uint32_t netIdx = rec.U8( *layout->netIndexOff );
                auto     it = m_sec23IndexToNet.find( netIdx );

                if( it != m_sec23IndexToNet.end() )
                    netName = it->second;
            }

            ViaLocation via;
            via.x       = vx;
            via.y       = vy;
            via.netName = netName;
            viaLocations.push_back( via );
        }
    }

    if( viaLocations.empty() )
        return;

    // Group vias by net name, then emit a ROUTE per net carrying only its vias.
    std::map<std::string, std::vector<const ViaLocation*>> netVias;

    for( const auto& via : viaLocations )
        netVias[via.netName].push_back( &via );

    for( const auto& [netName, vias] : netVias )
    {
        ROUTE route;
        route.net_name = netName;

        for( const auto* via : vias )
        {
            VIA viaDef;
            viaDef.location.x = static_cast<double>( via->x );
            viaDef.location.y = static_cast<double>( via->y );
            route.vias.push_back( std::move( viaDef ) );
        }

        m_routes.push_back( std::move( route ) );
    }
}


void BINARY_PARSER::parseCopperShapes()
{
    const SDB_SECTION* sec10 = getSection( SECTION::DrwItems );
    const SDB_SECTION* sec11 = getSection( SECTION::GraphicPieces );
    const SDB_SECTION* sec12 = getSection( SECTION::Vertices );

    if( !sec10 || !sec11 || !sec12 || sec10->perItem < 112
        || sec11->perItem < 20 || sec12->perItem < 12 )
    {
        return;
    }

    constexpr size_t MAX_COPPER_SHAPE_EDGES = 80;

    for( uint32_t rec = 0; rec < sec10->count; ++rec )
    {
        SDB_RECORD hdr = m_sdb.Record( *sec10, rec, sec10->perItem );

        uint32_t flag6 = hdr.U32( 24 );
        uint32_t flag7 = hdr.U32( 28 );
        uint32_t blockTag = hdr.U32( 84 );
        bool legacyCopper = ( blockTag == 0x00004900 && flag6 == 1 && flag7 != 3 );
        bool v2026LineCopper = ( m_version == 0x2026 && blockTag == 0x00004D00
                                 && flag6 == 7 && flag7 == 0 );

        if( !legacyCopper && !v2026LineCopper )
            continue;

        std::string name = hdr.Str( 44, 24 );

        if( name.size() < 4 || name.substr( 0, 3 ) != "DRW" )
            continue;

        uint32_t sec11Index = hdr.U32( 8 );

        if( legacyCopper && sec11Index >= sec11->count )
            continue;

        int32_t originX = hdr.I32( 88 );
        int32_t originY = hdr.I32( 92 );

        int64_t localMinX = static_cast<int64_t>( hdr.I32( 96 ) ) - originX;
        int64_t localMinY = static_cast<int64_t>( hdr.I32( 100 ) ) - originY;
        int64_t localMaxX = static_cast<int64_t>( hdr.I32( 104 ) ) - originX;
        int64_t localMaxY = static_cast<int64_t>( hdr.I32( 108 ) ) - originY;

        std::vector<VECTOR2I> loop;
        size_t minEdges = v2026LineCopper ? 4 : 5;

        // Fetch the owner's polygon structurally from its vertexStart cursor, then gate it
        // on the owner's recorded bbox. The structural slice resolves which sec12 loop
        // belongs to this owner; the bbox-equality gate is the filled-copper classifier,
        // since the grid classification is broader than filled copper (a stroked LINES item
        // records a pen-expanded bbox that no longer equals its vertex extent).
        if( !fetchOwnerLoop( name, MAX_COPPER_SHAPE_EDGES, loop ) || loop.size() < minEdges )
            continue;

        int64_t loopMinX = loop.front().x;
        int64_t loopMinY = loop.front().y;
        int64_t loopMaxX = loop.front().x;
        int64_t loopMaxY = loop.front().y;

        for( const VECTOR2I& pt : loop )
        {
            loopMinX = std::min<int64_t>( loopMinX, pt.x );
            loopMinY = std::min<int64_t>( loopMinY, pt.y );
            loopMaxX = std::max<int64_t>( loopMaxX, pt.x );
            loopMaxY = std::max<int64_t>( loopMaxY, pt.y );
        }

        if( loopMinX != localMinX || loopMinY != localMinY || loopMaxX != localMaxX
            || loopMaxY != localMaxY )
        {
            continue;
        }

        COPPER_SHAPE copper;
        copper.name = name;
        copper.filled = true;

        if( v2026LineCopper )
        {
            copper.layer = 1;
        }
        else
        {
            SDB_RECORD gfx = m_sdb.Record( *sec11, sec11Index, sec11->perItem );
            uint8_t    sideByte = gfx.U8( 0 );
            int32_t    layerHint = gfx.I32( 16 );
            copper.width = static_cast<double>( gfx.I32( 12 ) );

            if( sideByte == 0 || layerHint == m_parameters.layer_count )
                copper.layer = m_parameters.layer_count;
            else
                copper.layer = 1;
        }

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

    if( !sec10 || sec10->perItem < 112 || m_ownerRuns.empty() )
        return;

    // A dimension's leader geometry is the sec12 vertex run of its DIM* DRW owner, laid out
    // in ASC sub-piece order: BASPNT1(2v) BASPNT2(2v) ARWLN1(2v) ARWHD1(4v) ARWLN2(2v)
    // ARWHD2(4v) EXTLN1(2v) EXTLN2(2v). The measurement endpoints are the two BASPNT first
    // points (run rows 0 and 2); the crossbar is the ARWLN1 first point (run row 4). The
    // vertices are absolute DESIGN coords (no owner-origin shift), the same space as every
    // other parser geometry output.
    //
    // The value-label text lives in a sec8 record bound to the dimension only by anchor
    // proximity, and that bind is not reliable on boards whose title-block notes share the
    // dimension layer and overlap the leader extent. Following the correct-or-silent rule
    // we emit only the exact geometry and leave the override text empty, so KiCad recomputes
    // the displayed value from start/end. That computed value equals the PADS value exactly
    // (e.g. a 90000000 BASIC span renders as 60.00 mm, matching the ASC "60.00mm").
    for( uint32_t rec = 0; rec < sec10->count; ++rec )
    {
        std::string name = m_sdb.Record( *sec10, rec, sec10->perItem ).Str( 44, 24 );

        if( name.size() < 4 || name.substr( 0, 3 ) != "DIM" )
            continue;

        auto it = m_ownerRuns.find( name );

        if( it == m_ownerRuns.end() )
            continue;

        int32_t startRow = it->second.vertexStart - m_sec12Base;

        int32_t bp1x = 0, bp1y = 0, bp2x = 0, bp2y = 0, arwx = 0, arwy = 0, attr = 0;

        if( !sec12Vertex( startRow + 0, bp1x, bp1y, attr )
            || !sec12Vertex( startRow + 2, bp2x, bp2y, attr )
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

        // Horizontal vs vertical from the larger BASPNT delta, mirroring the ASCII parser.
        // crossbar_pos is the ARWLN1 first point projected onto the measured axis.
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

    if( !sec12 || sec12->perItem < 12 || sec12->totalBytes == 0 )
        return;

    int32_t directoryRows = static_cast<int32_t>( sec12->totalBytes / 12 );
    int32_t cleanRows = 0;

    // The clean prefix is the contiguous run of valid 12-byte vertex records before the
    // per-save heap tail. A record is clean while its attr is -1 (plain corner) or a
    // small arc-table ordinal; the tail is reached when attr becomes coordinate-scale.
    for( int32_t j = 0; j < directoryRows; ++j )
    {
        int32_t attr = m_sdb.Record( *sec12, static_cast<uint32_t>( j ), 12 ).I32( 8 );

        if( attr == -1 || ( attr >= 0 && attr < 4096 ) )
            cleanRows = j + 1;
        else if( std::abs( static_cast<int64_t>( attr ) ) >= 100000 )
            break;
    }

    m_sec12CleanRows = cleanRows;
    m_sec12Base = directoryRows - cleanRows;
}


void BINARY_PARSER::buildOwnerRuns()
{
    m_ownerRuns.clear();

    const SDB_SECTION* sec8 = getSection( SECTION::FreeText );
    const SDB_SECTION* sec10 = getSection( SECTION::DrwItems );

    if( !sec8 || !sec10 )
        return;

    size_t start = sec8->dataOffset;
    size_t end = std::min( static_cast<size_t>( sec10->dataOffset )
                                   + static_cast<size_t>( sec10->totalBytes ),
                           m_data.size() );

    if( start >= end )
        return;

    // A genuine 112-byte owner record carries a 0xFFFE/0xFFFF marker, a >=2-char ASCII
    // name at +44, and a cursor triple in range. The cursor guard rejects coincidental
    // heap markers (1-char names with wild cursors, e.g. the metric-flag byte in the
    // heap tail) so the marker walk does not bind garbage runs.
    auto isOwnerRecord = [&]( size_t aOff ) -> bool
    {
        SDB_RECORD rec = m_sdb.RecordAt( aOff );
        uint16_t   marker = rec.U16( 0 );
        uint16_t   hi = rec.U16( 2 );

        if( ( marker != 0xFFFE && marker != 0xFFFF ) || hi != 0 )
            return false;

        std::string name = rec.Str( 44, 44 );

        if( name.size() < 2 )
            return false;

        for( char c : name )
        {
            if( static_cast<unsigned char>( c ) < 32 || static_cast<unsigned char>( c ) >= 127 )
                return false;
        }

        int32_t vertexStart = rec.I32( 12 );
        int32_t pieceCount = rec.I32( 24 );

        return vertexStart >= 0 && vertexStart < ( 1 << 24 ) && pieceCount >= 0
               && pieceCount < ( 1 << 16 );
    };

    auto readRun = [&]( size_t aOff ) -> OWNER_RUN
    {
        SDB_RECORD rec = m_sdb.RecordAt( aOff );
        OWNER_RUN  run;
        run.pieceStart = rec.I32( 8 );
        run.vertexStart = rec.I32( 12 );
        run.arcStart = rec.I32( 16 );
        run.pieceCount = rec.I32( 24 );
        return run;
    };

    // Marker-walk the 112-byte owner records. The run cursors are read from the FOLLOWING
    // accepted record (the one-record lag), so we remember the previous record's name and
    // offset and bind it when the next record is found. The LAST owner has no successor in
    // the named list; its cursors are carried by the next physical 112-byte record (the
    // trailing terminator/seed slot), so we bind it from that record after the walk.
    std::string prevName;
    size_t      prevOff = 0;
    bool        havePrev = false;
    size_t      off = start;

    while( off + 112 <= end )
    {
        if( isOwnerRecord( off ) )
        {
            std::string name = m_sdb.RecordAt( off ).Str( 44, 44 );

            if( havePrev && !m_ownerRuns.count( prevName ) )
                m_ownerRuns.emplace( prevName, readRun( off ) );

            prevName = std::move( name );
            prevOff = off;
            havePrev = true;
            off += 112;
            continue;
        }

        ++off;
    }

    if( havePrev && !m_ownerRuns.count( prevName ) )
    {
        size_t tailOff = prevOff + 112;

        if( tailOff + 112 <= m_data.size() )
            m_ownerRuns.emplace( prevName, readRun( tailOff ) );
    }
}


bool BINARY_PARSER::sec12Vertex( int32_t aRow, int32_t& aX, int32_t& aY, int32_t& aAttr ) const
{
    const SDB_SECTION* sec12 = getSection( SECTION::Vertices );

    if( !sec12 || aRow < 0 || aRow >= m_sec12CleanRows )
        return false;

    if( sec12->dataOffset + static_cast<size_t>( aRow + 1 ) * 12 > m_data.size() )
        return false;

    SDB_RECORD rec = m_sdb.Record( *sec12, static_cast<uint32_t>( aRow ), 12 );
    aX = rec.I32( 0 );
    aY = rec.I32( 4 );
    aAttr = rec.I32( 8 );
    return true;
}


bool BINARY_PARSER::fetchOwnerLoop( const std::string& aName, size_t aMaxVerts,
                                    std::vector<VECTOR2I>& aOut ) const
{
    aOut.clear();

    auto it = m_ownerRuns.find( aName );

    if( it == m_ownerRuns.end() )
        return false;

    int32_t startRow = it->second.vertexStart - m_sec12Base;

    int32_t firstX = 0;
    int32_t firstY = 0;
    int32_t attr = 0;

    if( !sec12Vertex( startRow, firstX, firstY, attr ) )
        return false;

    aOut.emplace_back( firstX, firstY );

    for( size_t k = 1; k <= aMaxVerts; ++k )
    {
        int32_t x = 0;
        int32_t y = 0;

        if( !sec12Vertex( startRow + static_cast<int32_t>( k ), x, y, attr ) )
            break;

        if( x == firstX && y == firstY )
            return aOut.size() >= 3;

        aOut.emplace_back( x, y );
    }

    aOut.clear();
    return false;
}


void BINARY_PARSER::parseKeepouts()
{
    const SDB_SECTION* sec10 = getSection( SECTION::DrwItems );
    const SDB_SECTION* sec12 = getSection( SECTION::Vertices );

    if( !sec10 || !sec12 || sec10->perItem < 112 || sec12->perItem < 12 )
        return;

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

    for( uint32_t rec = 0; rec < sec10->count; ++rec )
    {
        SDB_RECORD hdr = m_sdb.Record( *sec10, rec, sec10->perItem );

        if( hdr.U32( 84 ) != 0 || hdr.U32( 24 ) != 1 )
            continue;

        uint32_t typeBucket = hdr.U32( 28 );

        if( typeBucket != 1 && typeBucket != 10 )
            continue;

        std::string name = hdr.Str( 44, 24 );

        if( name.size() < 4 || name.substr( 0, 3 ) != "DRW" )
            continue;

        Owner owner;
        owner.name = std::move( name );
        owner.originX = hdr.I32( 88 );
        owner.originY = hdr.I32( 92 );
        owner.minX = static_cast<int64_t>( hdr.I32( 96 ) ) - owner.originX;
        owner.minY = static_cast<int64_t>( hdr.I32( 100 ) ) - owner.originY;
        owner.maxX = static_cast<int64_t>( hdr.I32( 104 ) ) - owner.originX;
        owner.maxY = static_cast<int64_t>( hdr.I32( 108 ) ) - owner.originY;

        owners.push_back( std::move( owner ) );
    }

    if( owners.empty() )
        return;

    constexpr size_t MAX_KEEP_OUT_VERTICES = 80;

    for( const Owner& owner : owners )
    {
        KEEPOUT keepout;
        keepout.type = KEEPOUT_TYPE::ALL;

        // Polygon keepouts come from the structural owner -> sec12 vertex slice. The owner's
        // vertexStart cursor anchors a contiguous run in sec12 that closes back to its first
        // vertex. Vertices are DESIGN coordinates; add the DRW raw origin to get RAW.
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

        // A circle keepout has a degenerate (2-point) sec12 run, so the structural slice
        // does not close. Its geometry is the owner record's own +96..+108 bbox, a square:
        // center = bbox midpoint, radius = (xmax - xmin) / 2.
        int64_t spanX = owner.maxX - owner.minX;
        int64_t spanY = owner.maxY - owner.minY;

        if( spanX <= 0 || spanX != spanY )
            continue;

        constexpr int ELLIPSE_SEGMENTS = 32;
        double cx = static_cast<double>( owner.minX + owner.maxX ) / 2.0;
        double cy = static_cast<double>( owner.minY + owner.maxY ) / 2.0;
        double radius = static_cast<double>( spanX ) / 2.0;

        for( int i = 0; i < ELLIPSE_SEGMENTS; ++i )
        {
            double angle = ( 2.0 * M_PI * static_cast<double>( i ) )
                           / static_cast<double>( ELLIPSE_SEGMENTS );
            int32_t rawX = owner.originX + static_cast<int32_t>( std::lround( cx + radius * std::cos( angle ) ) );
            int32_t rawY = owner.originY + static_cast<int32_t>( std::lround( cy + radius * std::sin( angle ) ) );
            keepout.outline.emplace_back( toBasicCoordX( rawX ), toBasicCoordY( rawY ) );
        }

        m_keepouts.push_back( std::move( keepout ) );
    }
}


void BINARY_PARSER::parseCopperPours()
{
    // Two formats exist depending on file version:
    //
    // Simple format (v0x2021, v0x2026): POR headers and vertex data are embedded
    // in sec49's byte pool with FFFFFFFF delimiters, a trailing piece table, and
    // vertex coordinates appended at the end.
    //
    // Complex format (v0x2025, v0x2027): POR records reside in sec52 (88-byte
    // records with cumulative vertex counts), per-pour layer and width metadata
    // lives in a table at the tail of sec53, and all POR vertex coordinates are
    // stored contiguously in sec54 after its piece metadata block.

    struct PourHeader
    {
        size_t      offset   = 0;
        std::string name;
        uint32_t    vtxCount = 0;
    };

    std::vector<PourHeader> porHeaders;
    bool                    simpleFormat = false;

    // Try sec49 simple format first
    const SDB_SECTION* sec49 = getSection( SECTION::ClearanceRules );

    if( sec49 && sec49->totalBytes > 0 && sec49->End() <= m_data.size() )
    {
        uint32_t poolSize = sec49->totalBytes;

        for( size_t i = 0; i + 26 < poolSize; )
        {
            SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( sec49->dataOffset + i ) );

            // A pour header starts on a 0xFFFFFFFF delimiter, then marker==1 and sig 0x80.
            if( rec.U32( 0 ) != 0xFFFFFFFF )
            {
                i++;
                continue;
            }

            if( i + 32 > poolSize )
                break;

            if( rec.U32( 4 ) != 1 || rec.U8( 8 ) != 0x80 )
            {
                i += 4;
                continue;
            }

            std::string name = rec.Str( 10, 16 );

            if( name.size() < 4 || name.substr( 0, 3 ) != "POR" )
            {
                i += 4;
                continue;
            }

            PourHeader hdr;
            hdr.offset   = i;
            hdr.name     = name;
            hdr.vtxCount = rec.U32( 32 );
            porHeaders.push_back( hdr );
            i += 28;
        }

        if( !porHeaders.empty() )
            simpleFormat = true;
    }

    if( simpleFormat )
    {
        size_t       numPours  = porHeaders.size();
        const size_t lastOff   = porHeaders[numPours - 1].offset;
        const size_t tableBase = sec49->dataOffset + lastOff + 32;
        uint32_t     poolSize  = sec49->totalBytes;

        std::vector<uint32_t> vtxCounts( numPours );
        vtxCounts[0] = porHeaders[0].vtxCount;

        struct PourMeta
        {
            int32_t width     = 0;
            uint8_t pieceType = 0;
            uint8_t layer     = 0;
        };

        std::vector<PourMeta> pourMeta( numPours );

        size_t tablePos = tableBase + 4;

        for( size_t p = 0; p < numPours; ++p )
        {
            if( tablePos + 8 > sec49->dataOffset + poolSize )
                return;

            SDB_RECORD metaRec = m_sdb.RecordAt( tablePos );
            PourMeta   meta;
            meta.width     = metaRec.I32( 0 );
            meta.pieceType = metaRec.U8( 4 );
            meta.layer     = metaRec.U8( 5 );
            pourMeta[p]    = meta;
            tablePos += 8;

            if( p < numPours - 1 )
            {
                if( tablePos + 8 > sec49->dataOffset + poolSize )
                    return;

                vtxCounts[p + 1] = m_sdb.RecordAt( tablePos ).U32( 0 );
                tablePos += 8;
            }
        }

        size_t vtxPos = tablePos;

        for( size_t p = 0; p < numPours; ++p )
        {
            POUR pour;
            pour.owner_pour = porHeaders[p].name;
            pour.width      = static_cast<double>( pourMeta[p].width );
            pour.layer      = static_cast<int>( pourMeta[p].layer );

            uint32_t nVtx = vtxCounts[p];

            if( nVtx == 0 || nVtx > 100000 )
                continue;

            if( vtxPos + static_cast<size_t>( nVtx ) * 8 > sec49->dataOffset + poolSize )
                break;

            for( uint32_t v = 0; v < nVtx; ++v )
            {
                SDB_RECORD vtx = m_sdb.RecordAt( vtxPos );
                int32_t    x = vtx.I32( 0 );
                int32_t    y = vtx.I32( 4 );

                pour.points.emplace_back( toBasicCoordX( x ), toBasicCoordY( y ) );
                vtxPos += 8;
            }

            m_pours.push_back( std::move( pour ) );
        }

        return;
    }

    // Complex format: sec52 has POR records, sec53 has per-pour metadata table,
    // sec54 has piece metadata followed by vertex data.

    const SDB_SECTION* sec52 = getSection( SECTION::PourTokensA );
    const SDB_SECTION* sec53 = getSection( SECTION::PourTokensB );
    const SDB_SECTION* sec54 = getSection( SECTION::PourTokensC );

    if( !sec52 || sec52->totalBytes == 0 || !sec53 || sec53->totalBytes == 0
        || !sec54 || sec54->totalBytes == 0 )
    {
        return;
    }

    if( sec52->End() > m_data.size() || sec53->End() > m_data.size()
        || sec54->End() > m_data.size() )
    {
        return;
    }

    // Byte-stream scans over the three pour-token sections; the pointers index the SDB
    // bytes at each section's payload offset.
    const uint8_t* sec52Data = m_data.data() + sec52->dataOffset;
    const uint8_t* sec53Data = m_data.data() + sec53->dataOffset;
    const uint8_t* sec54Data = m_data.data() + sec54->dataOffset;

    // Scan sec52 byte stream for POR records identified by FFFFFFFF + u32(marker) +
    // u8(0x80) + u8(flag) + "POR..." name. Extract cumulative vertex counts at FF+32.
    for( size_t i = 0; i + 36 < sec52->totalBytes; ++i )
    {
        if( sec52Data[i] != 0xFF || sec52Data[i + 1] != 0xFF
            || sec52Data[i + 2] != 0xFF || sec52Data[i + 3] != 0xFF )
        {
            continue;
        }

        if( sec52Data[i + 8] != 0x80 )
        {
            i += 3;
            continue;
        }

        SDB_RECORD  rec  = m_sdb.RecordAt( static_cast<uint32_t>( sec52->dataOffset + i ) );
        std::string name = rec.Str( 10, 16 );

        if( name.size() < 4 || name.substr( 0, 3 ) != "POR" )
            continue;

        PourHeader hdr;
        hdr.offset   = i;
        hdr.name     = name;
        hdr.vtxCount = rec.U32( 32 );
        porHeaders.push_back( hdr );
    }

    if( porHeaders.empty() )
        return;

    size_t numPours = porHeaders.size();

    // Derive per-pour vertex counts from the cumulative values in sec52.
    // porHeaders[i].vtxCount is the running total through pour i.
    std::vector<uint32_t> vtxCounts( numPours );
    vtxCounts[0] = porHeaders[0].vtxCount;

    for( size_t p = 1; p < numPours; ++p )
        vtxCounts[p] = porHeaders[p].vtxCount - porHeaders[p - 1].vtxCount;

    // Locate the per-pour metadata table at the tail of sec53.
    // sec53 begins with FFFFFFFF-delimited ANP records. After the last one, a
    // table of 16-byte entries follows. Each entry carries the layer for the
    // corresponding POR pour at byte 0 and the next pour's width at bytes 11-14.
    // The first pour's width comes from the 16 bytes immediately preceding the table.

    // Find the last FFFFFFFF delimiter in sec53
    size_t lastFF53 = 0;
    bool   foundFF  = false;

    for( size_t i = sec53->totalBytes; i >= 4; --i )
    {
        size_t pos = i - 4;

        if( sec53Data[pos] == 0xFF && sec53Data[pos + 1] == 0xFF
            && sec53Data[pos + 2] == 0xFF && sec53Data[pos + 3] == 0xFF )
        {
            lastFF53 = pos;
            foundFF  = true;
            break;
        }
    }

    if( !foundFF )
        return;

    // The last FFFFFFFF record is 41 bytes: FFFFFFFF(4) + marker(4) + sig(1) +
    // flag(1) + name(16) + separator(1) + tail(14). The table starts immediately
    // after this record.
    static constexpr size_t LAST_FF_RECORD_SIZE = 41;
    size_t metaTableStart = lastFF53 + LAST_FF_RECORD_SIZE;

    if( metaTableStart + numPours * 16 > sec53->totalBytes )
        return;

    // The 16 bytes just before the table carry the first pour's width (bytes 11-14)
    size_t preTableStart = metaTableStart - 16;

    struct PourMeta
    {
        uint8_t layer = 0;
        int32_t width = 0;
    };

    std::vector<PourMeta> pourMeta( numPours );

    for( size_t p = 0; p < numPours; ++p )
    {
        size_t recOff = metaTableStart + p * 16;

        pourMeta[p].layer = sec53Data[recOff];

        // Width for pour p is stored in the previous 16-byte entry (bytes 11-14).
        // For the first pour, the previous entry is the pre-table block.
        size_t widthSrc = ( p == 0 ) ? preTableStart : metaTableStart + ( p - 1 ) * 16;
        pourMeta[p].width =
                m_sdb.RecordAt( static_cast<uint32_t>( sec53->dataOffset + widthSrc ) ).I32( 11 );
    }

    // Locate POR vertex data in sec54. The section begins with 16-byte piece
    // metadata records (byte 0 in {0x32, 0x33, 0x34}) followed by 4 padding bytes
    // and then contiguous vertex coordinates. The scan may include one extra
    // false-positive record, so the vertex start is computed as scan_end - 12.
    size_t metaScanEnd = 0;

    for( size_t pos = 0; pos + 16 <= sec54->totalBytes; pos += 16 )
    {
        uint8_t ptype = sec54Data[pos];

        if( ptype != 0x32 && ptype != 0x33 && ptype != 0x34 )
        {
            metaScanEnd = pos;
            break;
        }

        metaScanEnd = pos + 16;
    }

    // v0x2025 has a 3-byte header before piece metadata, shifting the scan start
    if( m_version == 0x2025 )
    {
        if( sec54Data[0] != 0x32 && sec54Data[0] != 0x33 && sec54Data[0] != 0x34 )
        {
            metaScanEnd = 0;

            for( size_t pos = 3; pos + 16 <= sec54->totalBytes; pos += 16 )
            {
                uint8_t ptype = sec54Data[pos];

                if( ptype != 0x32 && ptype != 0x33 && ptype != 0x34 )
                {
                    metaScanEnd = pos;
                    break;
                }

                metaScanEnd = pos + 16;
            }
        }
    }

    if( metaScanEnd < 12 )
        return;

    // The last scanned "record" is typically a false positive (vertex data that
    // happens to start with a valid piece-type byte). Back up by 12 bytes to get
    // the true vertex start (which accounts for the 4-byte padding after real
    // metadata, landing 12 bytes before the scan's end position).
    size_t vtxStart = metaScanEnd - 12;

    // Sanity check: total POR vertex bytes must fit within sec54
    uint32_t totalPorVtx = porHeaders.back().vtxCount;

    if( vtxStart + static_cast<size_t>( totalPorVtx ) * 8 > sec54->totalBytes )
    {
        // Fall back to scan_end + 4 in case the last metadata record was genuine
        vtxStart = metaScanEnd + 4;

        if( vtxStart + static_cast<size_t>( totalPorVtx ) * 8 > sec54->totalBytes )
            return;
    }

    size_t vtxPos = sec54->dataOffset + vtxStart;

    for( size_t p = 0; p < numPours; ++p )
    {
        POUR pour;
        pour.owner_pour = porHeaders[p].name;
        pour.width      = static_cast<double>( pourMeta[p].width );
        pour.layer      = static_cast<int>( pourMeta[p].layer );

        uint32_t nVtx = vtxCounts[p];

        if( nVtx == 0 || nVtx > 100000 )
            continue;

        for( uint32_t v = 0; v < nVtx; ++v )
        {
            SDB_RECORD vtx = m_sdb.RecordAt( vtxPos );
            int32_t    x = vtx.I32( 0 );
            int32_t    y = vtx.I32( 4 );

            pour.points.emplace_back( toBasicCoordX( x ), toBasicCoordY( y ) );
            vtxPos += 8;
        }

        m_pours.push_back( std::move( pour ) );
    }
}


void BINARY_PARSER::parseLayerStackup()
{
    m_layerInfos.clear();

    // The sec69 layout (name@+0, routing_dir@+32, layer_thickness@+52, copper_thickness@+56,
    // dielectric f32@+60, usage@+148; 31 records of 152 bytes) is verified on v0x2027 only.
    // Older dialects keep the synthesized fallback in GetLayerInfos().
    if( m_version != 0x2027 )
        return;

    static constexpr size_t REC_SIZE   = 152;
    static constexpr size_t REC_COUNT  = 31;
    static constexpr size_t NAME_LEN   = 24;
    static constexpr size_t OFF_ROUT   = 32;
    static constexpr size_t OFF_LAYTH  = 52;
    static constexpr size_t OFF_COPTH  = 56;
    static constexpr size_t OFF_DIEL   = 60;

    static const std::string ANCHOR = "(All layers)";

    // Locate the single "(All layers)" occurrence. This anchors the first record; the
    // directory data_offset is stale and overflows the indexed region on large boards.
    auto it = std::search( m_data.begin(), m_data.end(), ANCHOR.begin(), ANCHOR.end() );

    if( it == m_data.end() )
        return;

    size_t recordBase = static_cast<size_t>( it - m_data.begin() );

    if( !m_cursor.InBounds( recordBase, REC_COUNT * REC_SIZE ) )
        return;

    for( size_t k = 0; k < REC_COUNT; ++k )
    {
        size_t rec = recordBase + k * REC_SIZE;

        SDB_RECORD layerRec = m_sdb.RecordAt( rec );
        LAYER_INFO info;
        info.number = static_cast<int>( k );
        info.name = layerRec.Str( 0, NAME_LEN );

        int32_t routingDir = layerRec.I32( OFF_ROUT );

        info.layer_thickness   = static_cast<double>( layerRec.I32( OFF_LAYTH ) );
        info.copper_thickness  = static_cast<double>( layerRec.I32( OFF_COPTH ) );

        float dielectric = 0.0f;
        std::memcpy( &dielectric, &m_data[rec + OFF_DIEL], sizeof( float ) );
        info.dielectric_constant = static_cast<double>( dielectric );

        // A real copper layer carries a non-zero COPPER_THICKNESS; the count of these equals
        // the ASC MAXIMUMLAYER on every corpus file (4/4/12). The usage==1 enum is NOT a
        // reliable copper signal on its own: the "(All layers)" pseudo-layer also reads
        // usage==1, and a routed outer layer can read usage==0 (BR350460A's Bottom), so a
        // usage-only test both over- and under-counts. COPPER_THICKNESS is the stable
        // discriminator that includes Bottom and excludes the pseudo-layer and doc layers.
        info.is_copper = ( info.copper_thickness > 0.0 );
        info.required  = info.is_copper;

        if( info.is_copper )
        {
            info.layer_type = ( routingDir == 2 ) ? PADS_LAYER_FUNCTION::PLANE
                                                  : PADS_LAYER_FUNCTION::ROUTING;
        }
        else
        {
            info.layer_type = PADS_LAYER_FUNCTION::UNASSIGNED;
        }

        m_layerInfos.push_back( std::move( info ) );
    }

    // The leading "(All layers)" aggregate is a pseudo-layer, never a real copper layer.
    // It always carries usage==1, so demote it explicitly to avoid an extra copper entry.
    if( !m_layerInfos.empty() && m_layerInfos.front().name == ANCHOR )
    {
        m_layerInfos.front().is_copper = false;
        m_layerInfos.front().required  = false;
        m_layerInfos.front().layer_type = PADS_LAYER_FUNCTION::UNASSIGNED;
    }
}


std::vector<LAYER_INFO> BINARY_PARSER::GetLayerInfos() const
{
    // When the sec69 stackup table was decoded, renumber its active copper layers 1..N in
    // table order and append the standard non-copper technical layers (which sec69 names by
    // documentation subtype rather than the canonical PADS numbers the importer maps).
    if( !m_layerInfos.empty() )
    {
        std::vector<LAYER_INFO> infos;
        int                     copperNum = 0;

        for( const LAYER_INFO& src : m_layerInfos )
        {
            if( !src.is_copper )
                continue;

            LAYER_INFO info = src;
            info.number = ++copperNum;
            info.required = true;
            infos.push_back( std::move( info ) );
        }

        if( copperNum > 0 )
        {
            struct NonCopperDef
            {
                int                 number;
                const char*         name;
                PADS_LAYER_FUNCTION type;
            };

            static const NonCopperDef nonCopperLayers[] = {
                { 21, "Assembly Top",       PADS_LAYER_FUNCTION::ASSEMBLY },
                { 22, "Assembly Bottom",    PADS_LAYER_FUNCTION::ASSEMBLY },
                { 25, "Solder Mask Top",    PADS_LAYER_FUNCTION::SOLDER_MASK },
                { 26, "Silkscreen Top",     PADS_LAYER_FUNCTION::SILK_SCREEN },
                { 27, "Silkscreen Bottom",  PADS_LAYER_FUNCTION::SILK_SCREEN },
                { 28, "Solder Mask Bottom", PADS_LAYER_FUNCTION::SOLDER_MASK },
                { 29, "Paste Mask Top",     PADS_LAYER_FUNCTION::PASTE_MASK },
                { 30, "Paste Mask Bottom",  PADS_LAYER_FUNCTION::PASTE_MASK },
            };

            for( const auto& def : nonCopperLayers )
            {
                LAYER_INFO info;
                info.number = def.number;
                info.name = def.name;
                info.is_copper = false;
                info.required = false;
                info.layer_type = def.type;
                infos.push_back( info );
            }

            return infos;
        }
    }


    std::vector<LAYER_INFO> infos;
    int layerCount = m_parameters.layer_count;

    for( int i = 1; i <= layerCount; ++i )
    {
        LAYER_INFO info;
        info.number = i;
        info.name = "Layer " + std::to_string( i );
        info.is_copper = true;
        info.required = true;
        info.layer_type = PADS_LAYER_FUNCTION::ROUTING;
        infos.push_back( info );
    }

    // Standard PADS non-copper layers follow well-known numbering conventions.
    // These enable proper layer mapping for text, graphics, and mask layers.
    struct NonCopperDef
    {
        int                 number;
        const char*         name;
        PADS_LAYER_FUNCTION type;
    };

    static const NonCopperDef nonCopperLayers[] = {
        { 21, "Assembly Top",       PADS_LAYER_FUNCTION::ASSEMBLY },
        { 22, "Assembly Bottom",    PADS_LAYER_FUNCTION::ASSEMBLY },
        { 25, "Solder Mask Top",    PADS_LAYER_FUNCTION::SOLDER_MASK },
        { 26, "Silkscreen Top",     PADS_LAYER_FUNCTION::SILK_SCREEN },
        { 27, "Silkscreen Bottom",  PADS_LAYER_FUNCTION::SILK_SCREEN },
        { 28, "Solder Mask Bottom", PADS_LAYER_FUNCTION::SOLDER_MASK },
        { 29, "Paste Mask Top",     PADS_LAYER_FUNCTION::PASTE_MASK },
        { 30, "Paste Mask Bottom",  PADS_LAYER_FUNCTION::PASTE_MASK },
    };

    for( const auto& def : nonCopperLayers )
    {
        LAYER_INFO info;
        info.number = def.number;
        info.name = def.name;
        info.is_copper = false;
        info.required = false;
        info.layer_type = def.type;
        infos.push_back( info );
    }

    return infos;
}


void BINARY_PARSER::linkPartsToDecals()
{
    if( m_parts.empty() || m_decals.empty() )
        return;

    if( isOldFormat() )
    {
        // v0x2021 has no parttype-definition layer. The placement's decal is the direct
        // index captured in m_partDecalIndex (the NEXT 96 B placement record's @+56),
        // resolved against the JMPVIA-anchored decal-name table.
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

    // Deterministic placement -> decal chain (verified across v0x2025/26/27):
    //   parttype index I = the NEXT section 22 record's @+4 (a +1 block-interleave lag,
    //                      captured in m_partTypeIndex during parsePartPlacements)
    //   decal_index      = m_partTypeDecalIndex[I]  (parttype-definition table, payload +96)
    //   decal NAME       = m_decalNameTable[decal_index]  (complete decal-name table)
    //
    // The decal-name table covers connectors and mounting holes that section 10 lacks,
    // so this resolves the full placed set rather than just the section 10 subset.
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

        int32_t decalIndex = m_partTypeDecalIndex[partTypeIdx];

        if( decalIndex < 0 || static_cast<size_t>( decalIndex ) >= m_decalNameTable.size() )
            continue;

        const std::string& decalName = m_decalNameTable[decalIndex];

        if( !decalName.empty() && m_decals.count( decalName ) )
            part.decal = decalName;
    }
}


} // namespace PADS_IO

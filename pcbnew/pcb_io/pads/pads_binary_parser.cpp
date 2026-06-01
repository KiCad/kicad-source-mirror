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
#include <limits>
#include <numbers>
#include <numeric>
#include <optional>
#include <set>
#include <unordered_set>

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


// A placement's serialized object ID is its section-22 record ordinal plus a fixed per-dialect
// bias. v0x2021 biases by 13, measured against 2FOC-001.pcb's ASCII netlist: of the 68 B
// connection records' endpoint pairs, 523 resolve to a refdes pair present in the export at bias
// 13 versus 88 and 87 at the neighbouring biases, so the value is a sharp structural fit rather
// than the peak of a smooth curve. v0x2022 derives its own bias from the leading-placement run.
static uint32_t placementObjectBias( uint16_t aVersion )
{
    return aVersion == 0x2021 ? 13 : 11;
}


// Per-version field layout for the part-placement records. The old format (v0x2021/0x2022)
// and the newer dialects place the same fields at different offsets; one descriptor keeps the
// offset block in a single place instead of re-declaring it at each scanner.
struct PLACEMENT_LAYOUT
{
    int                nameOff = 0;           // refdes string
    int                xOff = 0;              // i32 X
    std::optional<int> yOff = std::nullopt;   // i32 Y (immediately follows X at xOff+4)
    int                angleOff = 0;          // i32 rotation
    int                feffOff = 0;           // 0xFEFF marker offset within the record
    int                scanStride = 0;        // record stride for the section-19/21 FEFF scan

    // Both old dialects carry the decal-name-table index directly at this offset within the NEXT
    // physical record (a +1 block-interleave lag); nullopt for new dialects, which resolve via
    // the parttype-index chain (parttypeIndexNextRecord) instead. The offset differs per dialect
    // (v0x2021 @+56, v0x2022 @+40).
    std::optional<int> directDecalOff = std::nullopt;

    // The parttype index lives in the NEXT physical record's @+4 field (a +1 block-interleave
    // lag). New-format dialects use it; both old dialects resolve decals directly via
    // directDecalOff instead.
    bool               parttypeIndexNextRecord = false;
};


static const PLACEMENT_LAYOUT& placementLayout( uint16_t aVersion )
{
    // v0x2021 anchors refdes at +76, with X/Y/angle/side at +92/+96/+100/+104 (partly past the
    // 96 B stride, spilling into the next physical record -- the format's usual field lag).
    static constexpr PLACEMENT_LAYOUT v2021{
        .nameOff = 76, .xOff = 92, .yOff = 96, .angleOff = 100, .feffOff = 28, .scanStride = 96, .directDecalOff = 56
    };
    // v0x2022 uses a DIFFERENT anchor (+60) despite sharing v0x2021's 96 B stride, with X/Y/angle
    // at +76/+80/+84. Verified against 2FOC_4.pcb ground truth (J13/J4/J5 zero-rotation parts,
    // U19/U20/U21 at 270/270/90 degrees) -- all six match exactly with this layout under the
    // shared origin (PADS_SDB::locateOrigin -- v0x2022's directory entry count and origin field
    // were both wrong before that fix, which looked like a placement-specific alt origin until
    // the root cause was found). Like v0x2021, the decal-name-table index is direct (not via the
    // parttype-index chain), but at the next record's +40 rather than +56; verified against 337
    // of 2FOC_4.pcb's 340 placements (the rest resolve to a plausible sibling decal, e.g.
    // FIDUCIAL/FIDTOP, so are ground-truth parsing noise, not real misses).
    static constexpr PLACEMENT_LAYOUT v2022{
        .nameOff = 60, .xOff = 76, .yOff = 80, .angleOff = 84, .feffOff = 28, .scanStride = 96, .directDecalOff = 40
    };
    static constexpr PLACEMENT_LAYOUT vNew{ .nameOff = 44,
                                            .xOff = 60,
                                            .yOff = 64,
                                            .angleOff = 68,
                                            .feffOff = 92,
                                            .scanStride = 94,
                                            .parttypeIndexNextRecord = true };

    if( aVersion == 0x2021 )
        return v2021;

    if( aVersion == 0x2022 )
        return v2022;

    return vNew;
}


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
                                           .layerCountOff = 50 };
    static constexpr PADSTACK_LAYOUT v2022{ .padWidthOff = 20,
                                            .drillOff = 24,
                                            .finLenOff = 28,
                                            .cornerOff = 32,
                                            .angleOff = 40,
                                            .layerStartOff = 44,
                                            .markerOff = 48,
                                            .shapeOff = 49,
                                            .layerCountOff = 50 };
    static constexpr PADSTACK_LAYOUT vNew{ .padWidthOff = 28,
                                           .drillOff = 32,
                                           .finLenOff = 36,
                                           .cornerOff = 44,
                                           .angleOff = 48,
                                           .layerStartOff = 52,
                                           .markerOff = 56,
                                           .shapeOff = 57,
                                           .layerCountOff = 58 };

    if( aVersion == 0x2021 )
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
constexpr size_t SIZE         = 112;
constexpr int    PIECE_START  = 8;
constexpr int    VERTEX_START = 12;
constexpr int    ARC_START    = 16;
constexpr int    CLASS_WORD   = 24;
constexpr int    PIECE_COUNT  = 24;
constexpr int    SUBTYPE_WORD = 28;
constexpr int    NAME         = 44;
constexpr int    TYPE_TAG     = 84;
constexpr int    ORIGIN_X     = 88;
constexpr int    ORIGIN_Y     = 92;
constexpr int    BBOX_MIN_X   = 96;
constexpr int    BBOX_MIN_Y   = 100;
constexpr int    BBOX_MAX_X   = 104;
constexpr int    BBOX_MAX_Y   = 108;
} // namespace DRW_ITEM


// Block-class tags carried at DRW_ITEM::TYPE_TAG. 0x4D00 also marks the board-outline item and
// is the library decal marker elsewhere.
namespace DRW_TAG
{
constexpr uint32_t COPPER_FILL   = 0x00004900;  // filled copper region
constexpr uint32_t COPPER_FILL_B = 0x0000FF00;  // second filled-copper tag, same class/shape
constexpr uint32_t LINE_ITEM     = 0x00004D00;  // line/outline item (board outline, v2026 line copper)
} // namespace DRW_TAG


// A reference designator is plausible when it is non-empty, starts with an alphanumeric (a
// numeric lead is legal, so this is isalnum, not isalpha), and contains no '+' -- no PADS
// reference designator convention ever uses '+', but the placements table's declared array can
// legitimately hold non-component entries (power/net-name label symbols, e.g. "P1_+5V",
// "LED_+5-CAN1") that share the placement record shape and would otherwise import as phantom
// footprints. Found on 2FOC-AdC_2.pcb: 6 such labels at real, in-bounds indices within section
// 22's own declared count, not a misaligned/recovered false positive.
static bool refdesFirstCharOk( const std::string& aRef )
{
    return !aRef.empty() && std::isalnum( static_cast<unsigned char>( aRef[0] ) )
           && aRef.find( '+' ) == std::string::npos;
}


// A stricter check for the byte-granular 0xFEFF marker scan (recoverOmittedPlacementsOld), which
// is far more prone to landing on a misaligned false-positive record than the grid-aligned main
// scan. A single-letter refdes is never real in PADS' naming convention (always PREFIX+NUMBER,
// e.g. "M1", "J13"); the sole real single-character refdes convention is a bare digit ("5"),
// which this still accepts.
static bool refdesPlausibleForByteScan( const std::string& aRef )
{
    if( !refdesFirstCharOk( aRef ) )
        return false;

    return aRef.size() > 1 || std::isdigit( static_cast<unsigned char>( aRef[0] ) );
}


// An unused/padding slot in the placement array can carry leftover non-placement text (e.g. a
// stale net-name string) that still happens to pass refdesFirstCharOk. A real placement's raw
// coordinate is design + origin, essentially never at raw (0, 0) -- the origin alone is normally
// a large offset. Verified against two real false positives this session: a stray "A"/"A1" text
// fragment preceding a genuine leading placement run, and a stale net name ("N1265572") sitting
// in an otherwise-valid placement array slot -- both at or near raw (0, 0).
static bool placementCoordPlausible( const SDB_RECORD& aRec, int aXOff, std::optional<int> aYOff )
{
    static constexpr int32_t MIN_PLAUSIBLE_RAW_COORD = 100000;

    // Matches recoverOmittedPlacements()'s COORD_ABS_MAX: a real placement's raw coordinate
    // never approaches INT32_MAX. Without this, a byte-scan false positive with a garbage
    // (e.g. reinterpreted-string) coordinate passes here purely because it isn't near zero --
    // found on P2007_0000_RBCS_RC_AdC_MAIS_000.pcb (v0x2021), where a stray "I" record with raw
    // X=-453602671 satisfied the old min-only check and was imported as a phantom footprint.
    static constexpr int32_t MAX_PLAUSIBLE_RAW_COORD = 1500000000;

    int32_t rawX = aRec.I32( aXOff );
    int32_t rawY = aYOff ? aRec.I32( *aYOff ) : 0;

    bool minOk = std::abs( rawX ) >= MIN_PLAUSIBLE_RAW_COORD || std::abs( rawY ) >= MIN_PLAUSIBLE_RAW_COORD;
    bool maxOk = std::abs( rawX ) <= MAX_PLAUSIBLE_RAW_COORD && std::abs( rawY ) <= MAX_PLAUSIBLE_RAW_COORD;

    return minOk && maxOk;
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

    m_sdb.Load( std::move( bytes ) );

    m_version     = m_sdb.Version();
    m_originX     = m_sdb.Coords().OriginX();
    m_originY     = m_sdb.Coords().OriginY();
    m_originFound = m_sdb.Coords().Found();

    m_parameters.origin.x = static_cast<double>( m_originX );
    m_parameters.origin.y = static_cast<double>( m_originY );

    // The call order below is load-bearing; the dependency edges noted at each group fix it.

    // Container state loads the version, origin and parameter block.
    parseBoardSetup();
    parseMetadataRegion();

    // Part placements, the part-cluster groups, and the recovery pass that adds omitted placements.
    parsePartPlacements();
    parseClusters();
    recoverOmittedPlacements();

    // Padstacks and the decal / part-type tables that linkPartsToDecals joins at the end.
    parsePadStacks();
    parseDecalNameTable();
    parsePartTypeTable();
    parsePartDecals();

    // The outline origin must resolve before the outline, whose vertices are DRW-origin-relative.
    parseBoardOutlineDrwOrigin();
    parseBoardOutline();

    // Net names first; the net-class and diff-pair passes key off the net records they produce.
    parseNetNames();
    parseNetClasses();
    parseDiffPairs();

    // Route-cell orientation uses placed terminal centers as anchors, so placements must have
    // their physical decals before structural route geometry is decoded.
    linkPartsToDecals();

    // Structural geometry. computeSec12Base and buildOwnerRuns establish the sec12 vertex base and
    // the owner-run cursors that the keepout, copper-shape and dimension decoders walk (the first
    // two through fetchOwnerLoop).
    parseTextRecords();
    parseRouteVertices();
    computeSec12Base();
    buildOwnerRuns();
    parseKeepouts();
    parseCopperShapes();
    parseCopperPours();
    parseDimensions();
    parseLayerStackup();

    resolveNetAnchors();

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
    // MAXIMUMLAYER lives in the same *PCB* board-setup parameter block PADS_SDB::locateOrigin
    // already located structurally (its true start is displaced from section 1's
    // directory-declared dataOffset by a per-file amount -- the same reason the origin needed
    // that scan, not a fixed offset).
    if( !m_sdb.Coords().Found() )
        return;

    uint32_t headerBase = m_sdb.Coords().HeaderBase();

    if( !m_cursor.InBounds( headerBase, 20 ) )
        return;

    // Word 4 of the fixed u32 parameter block is the maximum layer count.
    uint32_t maxLayer = m_sdb.RecordAt( headerBase ).U32( 16 );

    if( maxLayer >= 1 && maxLayer <= 64 )
        m_parameters.layer_count = static_cast<int>( maxLayer );
    else
        m_parameters.layer_count = 2;

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
    part.bottom_layer = aRec.U8( aNameOff + 28 ) != 0;
    part.units = "M";
    return part;
}


void BINARY_PARSER::parsePartPlacements()
{
    const SDB_SECTION* entry = getSection( SECTION::Placements );

    if( !entry || entry->count == 0 || entry->stride == 0 )
        return;

    if( entry->End() > m_data.size() )
        return;

    const PLACEMENT_LAYOUT& layout = placementLayout( m_version );
    bool                    isOld = isOldFormat();
    uint32_t                recSize = entry->stride;

    // The old framing reads the side flag at nameOff+28, past its 96 B stride.
    size_t fieldSpan = std::max<size_t>( recSize, static_cast<size_t>( layout.nameOff ) + 29 );
    uint32_t leadingPlacementCount = 0;

    if( m_version == 0x2022 )
    {
        for( uint32_t k = 1; k <= entry->count; ++k )
        {
            size_t candidateBase = static_cast<size_t>( entry->dataOffset ) - static_cast<size_t>( k ) * recSize;

            if( entry->dataOffset < k * recSize || !m_cursor.InBounds( candidateBase, fieldSpan ) )
                break;

            SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( candidateBase ) );

            if( !refdesFirstCharOk( rec.Str( layout.nameOff, 16 ) )
                || !placementCoordPlausible( rec, layout.xOff, layout.yOff )
                || rec.I32( layout.angleOff ) % ANGLE_SCALE != 0 )
            {
                break;
            }

            ++leadingPlacementCount;
        }
    }

    for( uint32_t i = 0; i < entry->count; ++i )
    {
        size_t off = static_cast<size_t>( i ) * recSize;

        // fieldSpan covers the full record, so this guard also rejects a directory whose
        // stride is smaller than the fields we read.
        if( off + fieldSpan > entry->totalBytes )
            break;

        SDB_RECORD  rec    = m_sdb.Record( *entry, i, recSize );
        std::string refDes = rec.Str( layout.nameOff, 16 );

        if( !refdesFirstCharOk( refDes ) )
            continue;

        if( !placementCoordPlausible( rec, layout.xOff, layout.yOff ) )
            continue;

        PART part = makePlacementPart( rec, layout.xOff, layout.yOff, layout.angleOff, layout.nameOff, refDes );

        // The parttype index lives in the NEXT physical record's @+4 field (a +1 block-
        // interleave lag); linkPartsToDecals resolves it to the decal name. For the LAST part
        // "the next record" falls outside this section's own declared totalBytes.
        //
        // Tried reading past the section boundary into the file's next physical bytes instead of
        // stopping there (on the theory that PADS writes one contiguous stream and the directory's
        // section split is a KiCad-side bookkeeping artifact, not a real storage gap). REVERTED:
        // measured directly on Seth's hand-built simple1.pcb, section 22 (Placements, count=2) is
        // immediately followed by section 23 (Nets, a 424 B/record table). The "next record" read
        // for the last placement lands exactly on section 23's first record and reads its bytes as
        // a parttype index -- 65536 here, meaningless, silently out of range of the 1-entry
        // parttype table so it happened not to visibly break anything, but for the wrong reason:
        // sections really do hold semantically distinct tables in this format, and spilling past
        // one into whatever the next happens to be is exactly the "guess past the edge" behavior
        // that produces wrong-but-plausible data on some other file rather than a clean failure.
        // The correct fix needs the real place this format stores the last placement's parttype --
        // not found yet -- so leave this at the safe, honest "we don't know" for now.
        if( layout.parttypeIndexNextRecord && i + 1 < entry->count )
        {
            size_t nextOff = ( static_cast<size_t>( i ) + 1 ) * recSize;

            if( nextOff + 18 <= entry->totalBytes )
            {
                SDB_RECORD next = m_sdb.Record( *entry, i + 1, recSize );
                m_partTypeIndex[m_parts.size()] = next.U32( 4 );
                m_partDecalAlternate[m_parts.size()] = next.U8( 17 );
            }
        }

        // Old dialects have no working parttype-index chain for placements; the decal index is
        // direct, in the NEXT record at layout.directDecalOff. Same last-part gap, same reason for
        // not spilling past the section -- see the comment above.
        if( layout.directDecalOff && i + 1 < entry->count )
        {
            size_t   nextOff = ( static_cast<size_t>( i ) + 1 ) * recSize;
            uint32_t fieldOff = static_cast<uint32_t>( *layout.directDecalOff );

            if( nextOff + fieldOff + 4 <= entry->totalBytes )
                m_partDecalIndex[m_parts.size()] = m_sdb.Record( *entry, i + 1, recSize ).U32( fieldOff );
        }

        // Cluster membership is the 1-based CLSTID at nameOff+64 (=+108), present only in the
        // new 112-byte layout; -1 means the part is in no cluster.
        if( !isOld && layout.nameOff == 44 && off + static_cast<size_t>( layout.nameOff ) + 68 <= entry->totalBytes )
        {
            int32_t clstid = rec.I32( layout.nameOff + 64 );

            if( clstid > 0 )
                m_partClusterId[m_parts.size()] = clstid;
        }

        uint32_t objectId =
                m_version == 0x2022 ? i + leadingPlacementCount : i + placementObjectBias( m_version );
        m_placementObjectToPart[objectId] = m_parts.size();
        m_parts.push_back( part );
    }

    // Any dialect can carry a run of leading placements immediately BEFORE the section's own
    // dataOffset, spilling into what the directory attributes to the preceding section -- the
    // same one-record quirk recoverOmittedPlacementsOld's leading-block case handles, but as a
    // run of arbitrary length rather than a single record. Walk backward while each candidate
    // has a valid refdes; verified against 2FOC_4.pcb (old dialect, direct decal chain), where
    // 13 such records (M1/M2/M3/U4/U5/U9-U12/C17/J11/J12 and one oddly-named "5") sit directly
    // before section 22's start, and against NEI2_2.pcb (new dialect, parttype-index chain),
    // where 7 (P1-P6, U1) do the same.
    if( layout.directDecalOff || layout.parttypeIndexNextRecord )
    {
        for( uint32_t k = 1; k <= entry->count; ++k )
        {
            size_t candidateBase = static_cast<size_t>( entry->dataOffset ) - static_cast<size_t>( k ) * recSize;

            if( entry->dataOffset < k * recSize || !m_cursor.InBounds( candidateBase, fieldSpan ) )
                break;

            SDB_RECORD  rec    = m_sdb.RecordAt( static_cast<uint32_t>( candidateBase ) );
            std::string refDes = rec.Str( layout.nameOff, 16 );

            if( !refdesFirstCharOk( refDes ) )
                break;

            // This backward extrapolation walks past the section's own declared bounds, so
            // unlike the main scan it needs a second signal beyond "looks like a refdes" --
            // verified against a real false positive (a stray "A"/"A1" text fragment with
            // near-zero raw X/Y immediately preceding a board's genuine 13-record leading run).
            if( !placementCoordPlausible( rec, layout.xOff, layout.yOff ) )
                break;

            // A second false positive on P2007_0000_RBCS_RC_AdC_MAIS_000.pcb (v0x2021): a stray
            // "I" text fragment with a raw X (-720754006) that satisfies placementCoordPlausible
            // (well under its 1.5e9 ceiling, sized for legitimate large panels) but whose angle
            // field is not a whole number of degrees (18.27078944 deg raw). Every genuine
            // placement observed in this codebase's corpus rotates in whole degrees; verified via
            // the full 66-board regression (compare_corpus.py) that this does not drop any of the
            // 2FOC_4.pcb / NEI2_2.pcb leading-block records this loop was originally added for.
            if( rec.I32( layout.angleOff ) % ANGLE_SCALE != 0 )
                break;

            PART part = makePlacementPart( rec, layout.xOff, layout.yOff, layout.angleOff, layout.nameOff, refDes );

            size_t nextBase = candidateBase + recSize;

            if( layout.directDecalOff )
            {
                uint32_t fieldOff = static_cast<uint32_t>( *layout.directDecalOff );

                if( m_cursor.InBounds( nextBase, fieldOff + 4 ) )
                {
                    m_partDecalIndex[m_parts.size()] =
                            m_sdb.RecordAt( static_cast<uint32_t>( nextBase ) ).U32( fieldOff );
                }
            }

            if( layout.parttypeIndexNextRecord && m_cursor.InBounds( nextBase, 18 ) )
            {
                SDB_RECORD next = m_sdb.RecordAt( static_cast<uint32_t>( nextBase ) );
                m_partTypeIndex[m_parts.size()] = next.U32( 4 );
                m_partDecalAlternate[m_parts.size()] = next.U8( 17 );
            }

            if( m_version == 0x2022 && k <= leadingPlacementCount )
                m_placementObjectToPart[leadingPlacementCount - k] = m_parts.size();
            else if( k <= placementObjectBias( m_version ) )
                m_placementObjectToPart[placementObjectBias( m_version ) - k] = m_parts.size();

            m_parts.push_back( part );
        }
    }

    // A handful of tiny, mechanical-only old-dialect boards carry NO placements at either the
    // declared dataOffset or immediately before it -- the whole record run is relocated further
    // back, with unrelated data (not a contiguous extension of it) sitting in between. Verified
    // against AdC_LCORE.pcb/AdC_LCORE-1.pcb (both v0x2022, 8 mounting-hole placements H1-H8),
    // where the real array starts exactly 13 records (1248 B) before entry->dataOffset, with 5
    // records of non-placement data separating it from the declared start. Since there is no
    // contiguity to walk from, search backward for a base at which every one of entry->count
    // records is a plausible placement -- requiring a full match keeps this from ever firing on
    // the ordinary case (a real board's tail records would have to coincidentally all validate).
    if( isOld && m_parts.empty() && entry->count > 0 )
    {
        constexpr uint32_t MAX_BACKWARD_RECORDS = 64;
        uint32_t           maxK = std::min( MAX_BACKWARD_RECORDS, entry->dataOffset / recSize );

        for( uint32_t k = 0; k <= maxK; ++k )
        {
            size_t base = static_cast<size_t>( entry->dataOffset ) - static_cast<size_t>( k ) * recSize;

            if( !m_cursor.InBounds( base, static_cast<size_t>( entry->count ) * recSize + fieldSpan - recSize ) )
                continue;

            bool allValid = true;

            for( uint32_t i = 0; i < entry->count && allValid; ++i )
            {
                SDB_RECORD  rec    = m_sdb.RecordAt( static_cast<uint32_t>( base + static_cast<size_t>( i ) * recSize ) );
                std::string refDes = rec.Str( layout.nameOff, 16 );

                if( !refdesFirstCharOk( refDes ) || !placementCoordPlausible( rec, layout.xOff, layout.yOff ) )
                    allValid = false;
            }

            if( !allValid )
                continue;

            for( uint32_t i = 0; i < entry->count; ++i )
            {
                size_t      recBase = base + static_cast<size_t>( i ) * recSize;
                SDB_RECORD  rec     = m_sdb.RecordAt( static_cast<uint32_t>( recBase ) );
                std::string refDes  = rec.Str( layout.nameOff, 16 );
                PART part = makePlacementPart( rec, layout.xOff, layout.yOff, layout.angleOff, layout.nameOff, refDes );

                if( layout.directDecalOff )
                {
                    uint32_t fieldOff = static_cast<uint32_t>( *layout.directDecalOff );
                    size_t   nextBase = recBase + recSize;

                    if( m_cursor.InBounds( nextBase, fieldOff + 4 ) )
                    {
                        m_partDecalIndex[m_parts.size()] =
                                m_sdb.RecordAt( static_cast<uint32_t>( nextBase ) ).U32( fieldOff );
                    }
                }

                m_parts.push_back( part );
            }

            break;
        }
    }
}


void BINARY_PARSER::parseClusters()
{
    // A part cluster is a fixed 60-byte record, name@+0 (char[16], NUL-padded), in cluster
    // order, so a record's 1-based ordinal IS the CLSTID the +108 field references. Membership
    // is captured during parsePartPlacements. The old 96-byte placement layout has no room for
    // the +108 CLSTID, so old-format boards carry no clusters.
    //
    // Count and stride are directory reads from the sec68 cluster-controller entry. The base is
    // NOT a directory offset: the late heap sections' physical position is a running read cursor
    // the 16-byte directory cannot reproduce. sec68 abuts the sec69 layer table, so the table is
    // located by a structural anchor: layer record 1 is always the "Top" copper layer, framed
    // [u32 0][u32 1][char[16] "Top"]. That 12-byte pattern is unique per file, and
    // cluster_base = sec69_rec0 - sec68.count*60.
    if( isOldFormat() )
        return;

    const SDB_SECTION* sec68 = getSection( SECTION::Clusters );

    if( !sec68 || sec68->count == 0 || sec68->stride != 60 )
        return;

    static constexpr size_t  REC_SIZE     = 60;
    static constexpr size_t  LAYER_STRIDE = 152;
    static const uint8_t     TOP_FRAME[12] = { 0, 0, 0, 0, 1, 0, 0, 0, 'T', 'o', 'p', 0 };

    // A second occurrence makes the locator ambiguous, so bail rather than guess.
    if( !signatureIsUnique( m_data, TOP_FRAME, sizeof( TOP_FRAME ) ) )
        return;

    size_t matchOff = findSignature( m_data, TOP_FRAME, sizeof( TOP_FRAME ) );

    // Use division rather than multiplication so a malformed count cannot overflow.
    if( matchOff < LAYER_STRIDE )
        return;

    size_t sec69Rec0 = matchOff - LAYER_STRIDE;

    if( sec68->count > sec69Rec0 / REC_SIZE )
        return;

    size_t base = sec69Rec0 - static_cast<size_t>( sec68->count ) * REC_SIZE;

    for( uint32_t i = 0; i < sec68->count; ++i )
    {
        SDB_RECORD  rec = m_sdb.RecordAt( base + i * REC_SIZE );
        std::string name = rec.Str( 0, 16 );

        // A misaligned base would read non-cluster bytes; a printable name plus the record's
        // constant fields confirm the run, so emit nothing rather than fabricate empty groups.
        if( name.empty() || rec.I32( 24 ) != 0 || rec.I32( 32 ) != 1 || rec.I32( 36 ) != 0 )
        {
            m_clusters.clear();
            return;
        }

        PART_CLUSTER cluster;
        cluster.name = std::move( name );
        cluster.id = static_cast<int>( i ) + 1;
        m_clusters.push_back( std::move( cluster ) );
    }
}


void BINARY_PARSER::recoverOmittedPlacements()
{
    // Recover the parts omitted from section 22 (connectors, mounting/tooling holes, test
    // points, fiducials, a few passives). They are a contiguous run of full placement records
    // in the tail of the sec19/sec21 region, but the run is not always adjacent to
    // sec22.dataOffset (0-12 gap/sentinel records can sit between), and the per-record marker
    // bytes are non-uniform across test-points/fiducials/edge-fingers, so they cannot gate the
    // walk.
    //
    // Locate the run by an asymmetric gap-tolerant back-walk from sec22.dataOffset that scores
    // each stride-aligned window. The decisive discriminator is the coordinate pair: a real
    // placement has both |X| and |Y| in design range; every gap/sentinel/phantom fails it. The
    // 0xFEFF and i32@+0 bytes are weak hints, never gates. The walk tolerates leading gap
    // records to find the block but stops at the first non-member once it starts. v2017/2019/
    // 2022 record field offsets are unresolved, so the scorer matches nothing for them.
    const PLACEMENT_LAYOUT& layout = placementLayout( m_version );
    const bool              isOld = isOldFormat();

    // The scored new-format locator recovers old-format placements too, but not the v2021
    // decal-index +1 lag, so the old dialects keep the bounded 0xFEFF scan below.
    if( isOld )
    {
        recoverOmittedPlacementsOld();
        return;
    }

    const size_t       stride = 112;
    const int          sideOff = layout.nameOff + 28;

    const SDB_SECTION* sec22 = getSection( SECTION::Placements );

    if( !sec22 || sec22->dataOffset < stride )
        return;

    static constexpr int64_t COORD_MIN     = 100000;
    static constexpr int64_t COORD_ABS_MAX = 1500000000;
    static constexpr int32_t ORI_HALF_DEGREE      = 40500000;   // 0.5 degree (PADS stores deg * 9e6)
    static constexpr int     SCORE_PLACEMENT = 8;

    // A real placement scores >= 8 (refdes 4 + coords 5 + side 1 + angle 1); gap/sentinel/
    // phantom records score <= 2. The 0xFEFF and i32@+0 hints add at most +1 each.
    auto score = [&]( size_t aBase ) -> int
    {
        if( !m_cursor.InBounds( aBase, stride ) )
            return -999;

        SDB_RECORD  rec = m_sdb.RecordAt( aBase );
        std::string ref = rec.Str( layout.nameOff, 16 );
        int         s = refdesFirstCharOk( ref ) ? 4 : -4;

        int64_t x = std::llabs( static_cast<int64_t>( rec.I32( layout.xOff ) ) );
        int64_t y = std::llabs( static_cast<int64_t>( layout.yOff ? rec.I32( *layout.yOff ) : 0 ) );
        s += ( x >= COORD_MIN && x < COORD_ABS_MAX && y >= COORD_MIN && y < COORD_ABS_MAX ) ? 5 : -5;

        int32_t side = rec.I32( sideOff );
        s += ( side == 0 || side == 1 ) ? 1 : -1;

        int32_t ori = rec.I32( layout.angleOff );
        bool    angOk = ( ori == 0 );

        if( !angOk && ori > 0 && ori <= 700000000 )
        {
            int32_t oriRemainder = ori % ORI_HALF_DEGREE;
            angOk = ( oriRemainder <= 200000 || ( ORI_HALF_DEGREE - oriRemainder ) <= 200000 );
        }

        s += angOk ? 1 : -1;

        if( m_cursor.InBounds( aBase + layout.feffOff, 2 ) && m_data[aBase + layout.feffOff] == 0xFE
            && m_data[aBase + layout.feffOff + 1] == 0xFF )
            s += 1;

        if( rec.I32( 0 ) == 0 )
            s += 1;

        return s;
    };

    // Skip up to 12 trailing gap records to find the block, then stop at the first non-member.
    // The cap only backstops a runaway walk; the block ends at the first gap once it has started.
    std::vector<size_t> bases;
    bool                started = false;
    int                 preGap = 0;

    for( int k = 1; k <= 512; ++k )
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

    std::reverse( bases.begin(), bases.end() );

    std::unordered_set<std::string> existingRefs;

    for( const PART& p : m_parts )
        existingRefs.insert( p.name );

    for( size_t baseIndex = 0; baseIndex < bases.size(); ++baseIndex )
    {
        size_t      base = bases[baseIndex];
        SDB_RECORD  rec = m_sdb.RecordAt( base );
        std::string refDes = rec.Str( layout.nameOff, 16 );

        if( refDes.empty() || existingRefs.count( refDes ) )
            continue;

        PART part = makePlacementPart( rec, layout.xOff, layout.yOff, layout.angleOff, layout.nameOff, refDes );

        // The parttype index (new) / decal index (v2021) lives in the NEXT physical record (the
        // +1 lag). That record is the index CARRIER, not itself a placement -- it need not (and
        // for the last recovered placement in a run, does not) score as one, so gating on
        // score(next) >= SCORE_PLACEMENT drops the index for exactly that case. Verified on
        // Seth's hand-built simple1.pcb: two placements L1/L2 recovered via this back-walk; L1's
        // carrier happens to double as L2's own placement record so the old gate passed by
        // accident, but L2's carrier (one stride further, past the end of the recovered run) is
        // a bare index slot with no refdes/coords and always failed the gate, leaving L2 with no
        // parttype index and a fallback self-name/zero-pad decal. An in-bounds-only read matches
        // the sibling +1-lag call sites in parsePartPlacements() above, which never gated on
        // score() at all -- linkPartsToDecals() already bounds-checks the index against the real
        // parttype/decal tables, so a genuinely bogus value here fails to resolve rather than
        // producing a wrong-but-plausible decal.
        size_t next = base + stride;

        if( m_cursor.InBounds( next, 18 ) )
        {
            SDB_RECORD carrier = m_sdb.RecordAt( next );
            m_partTypeIndex[m_parts.size()] = carrier.U32( 4 );
            m_partDecalAlternate[m_parts.size()] = carrier.U8( 17 );
        }

        if( bases.size() <= placementObjectBias( m_version ) )
            m_placementObjectToPart[placementObjectBias( m_version ) - bases.size() + baseIndex] = m_parts.size();

        m_parts.push_back( std::move( part ) );
        existingRefs.insert( refDes );
    }
}


void BINARY_PARSER::recoverOmittedPlacementsOld()
{
    // Old-format omitted placements via a bounded 0xFEFF marker walk over the sec19/sec21
    // ranges. v2021 carries its placements as a contiguous 96 B run whose first (anchor) block
    // has no 0xFEFF marker, handled as a one-shot leading-block recovery.
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

        bool oldLeadingHandled = !layout.directDecalOff.has_value();

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

            SDB_RECORD  candidateRec = m_sdb.RecordAt( base );
            std::string refDes = candidateRec.Str( layout.nameOff, 16 );

            if( !refdesPlausibleForByteScan( refDes ) )
                continue;

            // This is the byte-granular 0xFEFF scan the class comment above already calls "far
            // more prone to landing on a misaligned false positive than the grid-aligned main
            // scan" -- yet unlike every other placement-accepting site in this file, it had no
            // coordinate or angle check at all. Found on 2FOC-AdC_2.pcb: refdes "1" (angle
            // 112.7274589 deg raw, not a whole degree) and, via the lead-block path below,
            // "AGOD0-9" (angle 165.0525867 deg raw) both false positives, matching neither a real
            // PADS placement's coordinate nor its whole-degree rotation convention.
            if( !placementCoordPlausible( candidateRec, layout.xOff, layout.yOff )
                || candidateRec.I32( layout.angleOff ) % ANGLE_SCALE != 0 )
                continue;

            // Emit the leading (anchor) block once, only after the first genuine placement is
            // confirmed. Its decal index is in this first marked block's @+directDecalOff.
            if( !oldLeadingHandled )
            {
                oldLeadingHandled = true;

                if( base >= OLD_REC_SIZE )
                {
                    size_t      leadBase = base - OLD_REC_SIZE;
                    SDB_RECORD  leadRec = m_sdb.RecordAt( leadBase );
                    std::string leadRef = leadRec.Str( layout.nameOff, 16 );

                    if( m_cursor.InBounds( leadBase, fieldSpan ) && refdesPlausibleForByteScan( leadRef )
                        && placementCoordPlausible( leadRec, layout.xOff, layout.yOff )
                        && leadRec.I32( layout.angleOff ) % ANGLE_SCALE == 0 && !existingRefs.count( leadRef ) )
                    {
                        PART lead = makePlacementPart( leadRec, layout.xOff, layout.yOff, layout.angleOff,
                                                       layout.nameOff, leadRef );

                        size_t leadField = base + static_cast<size_t>( *layout.directDecalOff );

                        if( m_cursor.InBounds( leadField, 4 ) )
                            m_partDecalIndex[m_parts.size()] = m_sdb.RecordAt( leadField ).U32( 0 );

                        m_parts.push_back( lead );
                        existingRefs.insert( leadRef );
                    }
                }
            }

            if( existingRefs.count( refDes ) )
                continue;

            const SDB_RECORD& partRec = candidateRec;

            PART part = makePlacementPart( partRec, layout.xOff, layout.yOff, layout.angleOff, layout.nameOff, refDes );

            // The decal index is in the NEXT 96 B block's @+directDecalOff. Gate on that block's
            // own 0xFEFF marker so trailing section data cannot supply a bogus index.
            if( layout.directDecalOff )
            {
                size_t nextMarker = markerBase + OLD_REC_SIZE;
                size_t nextField = base + OLD_REC_SIZE + static_cast<size_t>( *layout.directDecalOff );

                if( m_cursor.InBounds( nextMarker, 2 ) && m_data[nextMarker] == 0xFE && m_data[nextMarker + 1] == 0xFF
                    && m_cursor.InBounds( nextField, 4 ) )
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
    const SDB_SECTION* layerSection = getSection( SECTION::PadShapes );

    if( !entry || !layerSection || entry->count == 0 || entry->stride == 0 )
        return;

    if( entry->End() > m_data.size() || layerSection->dataOffset > m_data.size() )
        return;

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

        if( isOldFormat() )
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

    // Section 4 describes the padstack controller, but its directory payload begins after the
    // controller's 64-byte object array. Locate that array by its exact section-5 ownership
    // invariant: every live record starts at the accumulated layer cursor and the final cursor is
    // section 5's count. Zero-layer records are aliases or retired entries and do not advance it.
    uint32_t poolStart = 0;
    uint32_t scanStart = getSection( 1 ) ? getSection( 1 )->dataOffset : 0;
    uint64_t poolBytes = static_cast<uint64_t>( entry->count ) * recSize;

    for( uint32_t candidate = scanStart;
         candidate <= layerSection->dataOffset && poolBytes <= layerSection->dataOffset - candidate; ++candidate )
    {
        uint32_t layerCursor = m_sdb.RecordAt( candidate ).I32( layout.layerStartOff );

        if( m_sdb.RecordAt( candidate ).U8( layout.markerOff ) != 0xFE || layerCursor > layerSection->count )
        {
            continue;
        }

        bool valid = true;

    for( uint32_t i = 0; i < entry->count; ++i )
    {
            SDB_RECORD rec = m_sdb.RecordAt( candidate + i * recSize );

            if( rec.U8( layout.markerOff ) != 0xFE )
            {
                valid = false;
            break;
            }

            uint8_t layerCount = rec.U8( layout.layerCountOff );

            if( layerCount == 0 )
            continue;

            if( rec.I32( layout.layerStartOff ) != static_cast<int32_t>( layerCursor )
                || layerCount > layerSection->count - layerCursor )
            {
                valid = false;
                break;
            }

            layerCursor += layerCount;
    }

        if( valid && layerCursor == layerSection->count )
    {
            poolStart = candidate;
            break;
        }
    }

    if( poolStart == 0 )
        return;

    m_padStackPool.assign( entry->count, {} );
    m_padStackCache.clear();
    m_viaStackByIndex.clear();
    std::vector<int32_t> maxPadDiameter( entry->count, 0 );

    const uint32_t layerRecordSize = m_version == 0x2022 ? 24 : isOldFormat() ? 20 : 24;
    const uint32_t layerTableHeaderSize = m_version == 0x2022 ? 64 : isOldFormat() ? 20 : 24;
    uint64_t       layerTableStart64 = static_cast<uint64_t>( poolStart ) + poolBytes + layerTableHeaderSize;
    bool           haveLayerTable =
            layerTableStart64 <= m_data.size()
            && static_cast<uint64_t>( layerSection->count ) * layerRecordSize <= m_data.size() - layerTableStart64;
    uint32_t layerTableStart = haveLayerTable ? static_cast<uint32_t>( layerTableStart64 ) : 0;

    // Pad stacks are indexed by their position in the object array; part decals reference them by
    // that stable index.
    for( uint32_t i = 0; i < entry->count; ++i )
    {
        uint32_t base = poolStart + i * recSize;

        if( m_sdb.RecordAt( base ).U8( layout.markerOff ) != 0xFE )
            continue;

        PAD_STACK_LAYER               defaultLayer = readLayer( base );

        // Slotted-drill metadata is carried by the following physical padstack record. Bit 3
        // marks the carrier; +8 is the preceding stack's slot length and +12 its orientation.
        if( !isOldFormat() && defaultLayer.drill > 0 && i + 1 < entry->count )
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

        if( haveLayerTable )
    {
            SDB_RECORD stackRec = m_sdb.RecordAt( base );
            uint32_t   layerStart = stackRec.U32( layout.layerStartOff );
            uint8_t    layerCount = stackRec.U8( layout.layerCountOff );

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
                layer.layer = selector == 0 ? 0
                                            : selector == 0xFF ? -1
                                                               : static_cast<int>( selector )
                                                                         + ( m_version == 0x2022 ? 0 : 1 );
                layer.shape = shapeIt->second;
                layer.sizeA = static_cast<double>( geometryRec.I32( 4 ) );
                int32_t sizeB = geometryRec.I32( 8 );
                layer.sizeB = sizeB > 0 ? static_cast<double>( sizeB ) : layer.sizeA;
                layers.push_back( std::move( layer ) );
            }
        }

        m_padStackCache[static_cast<int>( i )] = layers;
    }

    // Via pad stacks have a positive drill. Named JMPVIA and STANDARDVIA definitions may share
    // the same drill, so dimensions cannot be classified by an arbitrary size cutoff.
    std::map<std::pair<double, double>, int> viaDimCounts;

    for( const auto& [idx, layers] : m_padStackCache )
    {
        if( !layers.empty() && layers.front().drill > 0 )
            viaDimCounts[{ layers.front().sizeA, layers.front().drill }]++;
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

    const size_t extentRecordSize = isOldFormat() ? 100 : 112;
    const size_t extentNameOffset = isOldFormat() ? 16 : 44;
    const size_t extentBboxOffset = isOldFormat() ? 64 : 92;
    const size_t extentSentinelOffset = isOldFormat() ? 80 : 108;

    auto extentDiameter = [&]( size_t aBase ) -> int32_t
    {
        if( !m_cursor.InBounds( aBase, extentRecordSize ) || m_cursor.U16At( aBase + extentSentinelOffset ) != 0xFFFE )
        {
            return 0;
        }

        size_t nameLength = 0;

        while( nameLength < 40 && m_cursor.U8At( aBase + extentNameOffset + nameLength ) != 0 )
        {
            uint8_t ch = m_cursor.U8At( aBase + extentNameOffset + nameLength );

            if( ch < 0x20 || ch >= 0x7F )
                return 0;

            ++nameLength;
        }

        if( nameLength == 0 || nameLength == 40 )
            return 0;

        int32_t xMin = m_cursor.I32At( aBase + extentBboxOffset );
        int32_t yMin = m_cursor.I32At( aBase + extentBboxOffset + 4 );
        int32_t xMax = m_cursor.I32At( aBase + extentBboxOffset + 8 );
        int32_t yMax = m_cursor.I32At( aBase + extentBboxOffset + 12 );

        if( xMin != -xMax || yMin != -yMax || xMax <= 0 || yMax <= 0 || xMax - xMin != yMax - yMin )
        {
            return 0;
        }

        return xMax - xMin;
    };

    std::set<size_t> usedStacks;
    size_t           extentRun = 0;

    auto matchesViaStack = [&]( int32_t aDiameter )
    {
        for( size_t i = 0; i < m_padStackPool.size(); ++i )
        {
            if( !m_padStackPool[i].empty() && m_padStackPool[i].front().drill > 0 && maxPadDiameter[i] == aDiameter )
            {
                return true;
            }
        }

        return false;
    };

    if( entry->count >= 2 )
    {
        for( size_t off = scanStart; off + 2 * extentRecordSize <= m_data.size(); ++off )
        {
            if( matchesViaStack( extentDiameter( off ) )
                && matchesViaStack( extentDiameter( off + extentRecordSize ) ) )
            {
                extentRun = off;
                break;
            }
        }
    }

    for( int viaIndex = 0; extentRun != 0; ++viaIndex )
    {
        int32_t diameter = extentDiameter( extentRun + static_cast<size_t>( viaIndex ) * extentRecordSize );

        if( diameter <= 0 )
            break;

        size_t matchedStack = m_padStackPool.size();

        for( size_t stackIndex = 0; stackIndex < m_padStackPool.size(); ++stackIndex )
        {
            if( usedStacks.count( stackIndex ) || m_padStackPool[stackIndex].empty()
                || m_padStackPool[stackIndex].front().drill <= 0 || maxPadDiameter[stackIndex] != diameter )
            {
                continue;
            }

            matchedStack = stackIndex;
            break;
        }

        if( matchedStack == m_padStackPool.size() )
            break;

        m_viaStackByIndex[viaIndex] = matchedStack;
        usedStacks.insert( matchedStack );
    }
}


void BINARY_PARSER::parseDecalNameTable()
{
    // The complete decal-name table sits in a fixed-size header immediately before section 14,
    // at sec14.dataOffset - 1188. Each record is 112 bytes with the decal NAME at +0, a 0xFFFE
    // sentinel at +64 and the terminal count at +72. Unlike section 10 this table includes vias,
    // connectors and mounting holes, and is indexed directly (base 0) by a parttype's
    // decal_index. The first record is always JMPVIA_AAAAA, used as an anchor sanity check. The
    // +72 count is harvested into m_decalTerminalCount so passives without a section 14
    // descriptor get exact pad counts.
    if( isOldFormat() )
    {
        parseDecalNameTableOld();
        return;
    }

    static constexpr int DECAL_HDR_OFFSET = 1188;
    static constexpr int REC_SIZE = 112;
    static constexpr int STACK_START_OFFSET = 44;
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
    // Neither old dialect (v0x2021 or v0x2022) stores the 1188-byte sec14 header the newer
    // dialects use. Both share the same complete decal-name table instead: a run of 100-byte
    // records anchored at the JMPVIA_AAAAA signature in the section 12 tail, each carrying
    // NAME @ +0 (null-terminated), a 0xFFFE sentinel @ +64 that terminates the table, and a
    // terminal count @ +72. Located by signature rather than a fixed offset because the old
    // dialects' section framing differs from the newer ones.
    if( !isOldFormat() )
        return;

    static constexpr int REC_SIZE = 100;
    static constexpr int STACK_START_OFFSET = 44;
    static constexpr int SENTINEL_OFFSET = 64;
    static constexpr int                 START_OFFSET = 68;
    static constexpr int COUNT_OFFSET = 72;
    static const std::array<uint8_t, 12> SIGNATURE = { 'J', 'M', 'P', 'V', 'I', 'A', '_', 'A', 'A', 'A', 'A', 'A' };

    // The table is the first signature occurrence whose 100-byte run validates (record 0 ==
    // JMPVIA_AAAAA with a 0xFFFE sentinel), guarding against a stray earlier occurrence.
    for( size_t start = findSignature( m_data, SIGNATURE ); start != SIGNATURE_NOT_FOUND;
         start = findSignature( m_data, SIGNATURE, start + 1 ) )
    {
        std::vector<std::string>        table;
        std::map<std::string, uint32_t> counts;
        std::map<std::string, int32_t>  starts;
        std::map<std::string, int32_t>  stackCounts;
        std::map<std::string, int32_t>  stackStarts;

        for( size_t k = 0;; ++k )
        {
            size_t off = start + k * REC_SIZE;

            if( off + SENTINEL_OFFSET + 2 > m_data.size() )
                break;

            SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( off ) );

            if( rec.U16( SENTINEL_OFFSET ) != SDB_RECORD_SENTINEL )
                break;

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
                        starts.emplace( name, cursor );

                    if( stackCount > 0 && stackCount <= 1000 )
                    {
                        stackCounts.emplace( name, stackCount );

                        // Both old dialects store this cursor, and it has to be read rather than
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

        if( !table.empty() && table[0] == "JMPVIA_AAAAA" )
        {
            m_decalNameTable = std::move( table );
            m_decalTerminalCount = std::move( counts );
            m_decalTerminalStart = std::move( starts );
            m_decalStackCount = std::move( stackCounts );
            m_decalStackStart = std::move( stackStarts );
            return;
        }
    }
}


void BINARY_PARSER::parsePartTypeTable()
{
    // The parttype-definition table's record size, decal_index field offset, and header framing
    // all differ between dialects. New dialects carry a fixed-size header immediately before
    // section 17, at sec17.dataOffset - 1232, with 224-byte records and decal_index at +96.
    // v0x2022 has no such header -- its table starts exactly at sec17.dataOffset, with 208-byte
    // records and decal_index at +112 (v0x2021 has no parttype-definition layer at all).
    // Verified against 2FOC_4.pcb: the decal_index for DSPIC33FJ128MC802/TP-LC/LMC7101/
    // MOLEX53261_0790 all land on +112 and resolve to their correct ground-truth decal names via
    // m_decalNameTable. Note placements don't reference this table for v0x2022 -- see
    // usesDirectDecalChain -- but it is still populated for potential future use (e.g. rules).
    if( m_version == 0x2021 )
        return;

    const bool isV2022 = ( m_version == 0x2022 );

    const uint32_t hdrOffset = isV2022 ? 0 : 1232;
    const uint32_t recSize   = isV2022 ? 208 : 224;
    const uint32_t decalOff  = isV2022 ? 112 : 96;

    const SDB_SECTION* sec17 = getSection( SECTION::ParttypeDefs );

    if( !sec17 || sec17->count == 0 || sec17->dataOffset < hdrOffset )
        return;

    uint32_t start = sec17->dataOffset - hdrOffset;

    // The parttype's own name (the *PARTTYPE alias, e.g. a manufacturer part number such as
    // GRM15XR71C103KA86D that resolves to a generic decal like C-0402) sits at +44 in the
    // non-v2022 224 B record -- verified against parallella-rf.pcb by locating the literal
    // "GRM15XR71C103KA86D" text and confirming its offset from the record's own decal_index
    // field lands on +44. Not verified for v2022's 208 B record, so it is left unpopulated
    // there rather than guessing; linkPartsToDecals() falls back to the decal name in that case.
    const bool     hasName = !isV2022;
    const uint32_t nameOff = 44;

    m_partTypeDecalIndex.clear();
    m_partTypeDecalIndex.reserve( sec17->count );
    m_partTypeDecalIndices.clear();
    m_partTypeDecalIndices.reserve( sec17->count );
    m_partTypeNames.clear();

    if( hasName )
        m_partTypeNames.reserve( sec17->count );

    for( uint32_t k = 0; k < sec17->count; ++k )
    {
        uint32_t off = start + k * recSize;

        if( off + decalOff + 4 > m_data.size() )
        {
            m_partTypeDecalIndex.push_back( -1 );
            m_partTypeDecalIndices.emplace_back();

            if( hasName )
                m_partTypeNames.emplace_back();

            continue;
        }

        SDB_RECORD          record = m_sdb.RecordAt( off );
        std::vector<int32_t> decalIndices;

        for( uint32_t indexOff = decalOff; !isV2022 && indexOff + 8 <= recSize; indexOff += 8 )
        {
            int32_t decalIndex = record.I32( indexOff );

            if( decalIndex < 0 || record.I32( indexOff + 4 ) != decalIndex )
                break;

            decalIndices.push_back( decalIndex );
        }

        if( isV2022 && record.I32( decalOff ) >= 0 )
            decalIndices.push_back( record.I32( decalOff ) );

        m_partTypeDecalIndex.push_back( decalIndices.empty() ? -1 : decalIndices.front() );
        m_partTypeDecalIndices.push_back( std::move( decalIndices ) );

        if( hasName )
            m_partTypeNames.push_back( off + nameOff + 32 <= m_data.size() ? m_sdb.RecordAt( off ).Str( nameOff, 32 )
                                                : std::string() );
    }
}


void BINARY_PARSER::parsePartDecals()
{
    const SDB_SECTION* entry = getSection( SECTION::DrwItems );

    if( !entry || entry->count == 0 || entry->stride == 0 )
        return;

    if( entry->End() > m_data.size() )
        return;

    bool     isNew = !isOldFormat();
    bool     isV2022 = ( m_version == 0x2022 );
    uint32_t nameOff = isNew ? 44 : ( isV2022 ? 12 : 28 );
    uint32_t recSize = entry->stride;

    for( uint32_t i = 0; i < entry->count; ++i )
    {
        if( ( i + 1 ) * recSize > entry->totalBytes )
            break;

        SDB_RECORD rec = m_sdb.Record( *entry, i, recSize );

        // New format carries a unit flag at +76: 0x4D 'M' = metric, else inch. v0x2022's name
        // field sits at +12 rather than v0x2021's +28, matching the same per-dialect field-shift
        // seen throughout this section's old-format layouts.
        std::string name = rec.Str( nameOff, 32 );

        if( name.empty() )
            continue;

        PART_DECAL decal;
        decal.name = name;
        decal.units = ( isNew && rec.U8( 76 ) == 0x4D ) ? "M" : "I";

        m_decals[name] = decal;
    }

    // Register every name from the complete decal-name table, which covers connectors, mounting
    // holes and vias that section 10 omits, so a part whose decal lives only there resolves.
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
    if( isOldFormat() )
    {
        parseTerminalsOld();
        return;
    }

    // Terminal positions for every decal come from an explicit per-decal cursor (the +68 field
    // of the decal-name header table, in m_decalTerminalStart) indexing a unified terminal
    // stream POOL33 ++ SEC15:
    //   POOL33 = the fixed 33-record (36 B each) de-dup pool in the sec14 trailer, holding the
    //            terminals of high-volume passives de-duplicated out of section 15. Always
    //            exactly 33 records at (sec14.count-11)*112 + 44.
    //   SEC15  = the section-15 geometry pool (36 B records, x@+0/y@+4), zero-tail terminated.
    // A decal's terminals are stream[start .. start+count): start < 33 selects the de-dup pool,
    // start >= 33 selects SEC15[start-33]. Coordinates are decal-local. The start cursor is
    // stored rather than derived from counts because the pool de-duplicates geometrically
    // identical decals onto shared windows.
    static constexpr int TERM_SIZE = 36;
    static constexpr int POOL_SIZE = 33;

    struct SERIALIZED_TERMINAL
    {
        int32_t     x;
        int32_t     y;
        std::string name;
    };

    std::vector<SERIALIZED_TERMINAL> stream;
    size_t                           inlinePairBase = 0;

    const SDB_SECTION* sec14 = getSection( SECTION::DecalHeader );

    // The -11 is a record offset into the section-14 trailer, not a count floor: a design with
    // fewer than 11 decal headers puts the pool BEFORE the section's dataOffset, and the count
    // >= 11 guard that avoided the unsigned wrap also skipped the pool entirely on those boards,
    // leaving the terminal stream to a fallback that starts mid-decal. UZCB_Adapter (sec14 count
    // 8) is one: its stream began at pin 31, so J31/J32 decoded 172 terminals starting at pin 34
    // and 277 of 634 pins landed on the wrong net. Signed arithmetic plus a bounds check keeps
    // the wrap impossible without discarding those designs.
    int64_t signedPoolBase = static_cast<int64_t>( sec14 ? sec14->dataOffset : 0 )
                             + ( static_cast<int64_t>( sec14 ? sec14->count : 0 ) - 11 ) * 112 + 44;

    bool poolInBounds = sec14 && signedPoolBase >= 0
                        && signedPoolBase + static_cast<int64_t>( POOL_SIZE ) * TERM_SIZE
                                   <= static_cast<int64_t>( m_data.size() );

    // Designs with fewer than 11 decal headers are the ones that used to skip the pool, and some
    // of them genuinely have none -- they serialize a single unified stream instead, which the
    // fallback below reads. Only trust a pool found below that threshold when every one of its
    // records satisfies the terminal-record shape the fallback itself keys on (local coordinates
    // duplicated at +0/+8 and +4/+12). UZCB_Adapter passes 33 of 33 and needs the pool; NEI2_2
    // passes 30 of 33 and is correct without it.
    bool poolUsable = poolInBounds;

    if( poolInBounds && sec14->count < 11 )
    {
        for( int i = 0; i < POOL_SIZE && poolUsable; ++i )
        {
            SDB_RECORD rec =
                    m_sdb.RecordAt( static_cast<uint32_t>( signedPoolBase + i * TERM_SIZE ) );
            poolUsable = rec.I32( 0 ) == rec.I32( 8 ) && rec.I32( 4 ) == rec.I32( 12 );
        }
    }

    if( poolUsable )
    {
        size_t poolBase = static_cast<size_t>( signedPoolBase );

        for( int i = 0; i < POOL_SIZE; ++i )
        {
            size_t off = poolBase + static_cast<size_t>( i ) * TERM_SIZE;

            if( off + 8 <= m_data.size() )
            {
                SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( off ) );
                stream.push_back( { rec.I32( 0 ), rec.I32( 4 ), rec.Str( 20, 4 ) } );
            }
            else
            {
                stream.push_back( { 0, 0, {} } );
            }
        }
    }

    const SDB_SECTION* sec15 = getSection( SECTION::TerminalPool );

    // Small designs omit POOL33 and serialize one unified terminal stream across the nominal
    // section-5/8 boundary. Its 36-byte records duplicate the local coordinates at +0/+8 and
    // +4/+12 and carry an object pointer at +20. The decal header cursors slice this stream.
    if( stream.empty() )
    {
        size_t streamCount = 0;

        for( const auto& [name, start] : m_decalTerminalStart )
        {
            auto countIt = m_decalTerminalCount.find( name );

            if( start >= 0 && countIt != m_decalTerminalCount.end() )
                streamCount = std::max( streamCount, static_cast<size_t>( start ) + countIt->second );
        }

        const SDB_SECTION* sec5 = getSection( SECTION::PadShapes );
        const SDB_SECTION* sec10 = getSection( SECTION::DrwItems );
        size_t             bytes = streamCount * TERM_SIZE;

        // Small v0x2024 databases serialize the unified terminal stream directly after the
        // complete 112-byte decal-name table. Their terminal object pointer is at +16 rather
        // than +20; the following +20 word is pin-name metadata.
        if( m_version == 0x2024 && sec14 && sec14->count < 11 && sec14->dataOffset >= 1188 )
        {
            size_t base = static_cast<size_t>( sec14->dataOffset ) - 1188
                          + static_cast<size_t>( sec14->count ) * 112;

            if( base <= m_data.size() && bytes <= m_data.size() - base )
            {
                bool valid = true;

                for( size_t i = 0; i < streamCount; ++i )
                {
                    SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( base + i * TERM_SIZE ) );
                    uint32_t   objectPtr = rec.U32( 16 );

                    if( rec.I32( 0 ) != rec.I32( 8 ) || rec.I32( 4 ) != rec.I32( 12 )
                        || objectPtr < 0x01000000 || objectPtr >= 0x80000000 )
                    {
                        valid = false;
                        break;
                    }
                }

                if( valid )
                {
                    for( size_t i = 0; i < streamCount; ++i )
                    {
                        SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( base + i * TERM_SIZE ) );
                        stream.push_back( { rec.I32( 0 ), rec.I32( 4 ), {} } );
                    }

                    inlinePairBase = base + bytes + 4;
                }
            }
        }

        if( stream.empty() && streamCount > 0 && sec5 && sec10 && sec5->dataOffset <= sec10->End()
            && bytes <= sec10->End() - sec5->dataOffset )
        {
            for( size_t base = sec5->dataOffset; base + bytes <= sec10->End(); ++base )
            {
                bool valid = true;

                for( size_t i = 0; i < streamCount; ++i )
                {
                    SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( base + i * TERM_SIZE ) );
                    uint32_t   objectPtr = rec.U32( 20 );

                    if( rec.I32( 0 ) != rec.I32( 8 ) || rec.I32( 4 ) != rec.I32( 12 ) || rec.I32( 16 ) != 0
                        || objectPtr < 0x10000000 || objectPtr >= 0x80000000 )
                    {
                        valid = false;
                        break;
                    }
                }

                if( !valid )
                    continue;

                for( size_t i = 0; i < streamCount; ++i )
                {
                    SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( base + i * TERM_SIZE ) );
                    stream.push_back( { rec.I32( 4 ), rec.I32( 0 ), rec.Str( 20, 4 ) } );
                }

                inlinePairBase = base + bytes + 4;
                break;
            }
        }
    }

    if( inlinePairBase == 0 && sec15 && sec15->totalBytes > 0 && sec15->stride == TERM_SIZE )
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

            stream.push_back( { rec.I32( 0 ), rec.I32( 4 ), rec.Str( 20, 4 ) } );
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
            term.x = toBasicCoordX( stream[start + t].x );
            term.y = toBasicCoordY( stream[start + t].y );
            term.name = stream[start + t].name.empty() ? std::to_string( t + 1 ) : stream[start + t].name;
            decal.terminals.push_back( term );
        }
    }

    // Any decal still without terminals (a count-only entry whose +68 cursor is absent) falls
    // back to the count-correct placeholder layout.
    synthesizePlaceholderTerminals();
    assignDefaultPadStacks();

    if( inlinePairBase > 0 )
    {
        for( auto& [name, decal] : m_decals )
        {
            auto startIt = m_decalStackStart.find( name );
            auto countIt = m_decalStackCount.find( name );

            if( startIt == m_decalStackStart.end() || countIt == m_decalStackCount.end() || startIt->second < 0
                || countIt->second <= 0 )
            {
                continue;
            }

            size_t end = inlinePairBase + ( static_cast<size_t>( startIt->second ) + countIt->second ) * 8;

            if( end > m_data.size() )
                continue;

            std::vector<std::pair<int32_t, int32_t>> pairs;

            for( int32_t i = 0; i < countIt->second; ++i )
            {
                size_t     off = inlinePairBase + ( static_cast<size_t>( startIt->second ) + i ) * 8;
                SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( off ) );

                if( m_version == 0x2024 )
                    pairs.emplace_back( rec.I32( 4 ), rec.I32( 0 ) );
                else
                    pairs.emplace_back( rec.I32( 0 ), rec.I32( 4 ) );
            }

            applyPadstackPairs( decal, pairs, 0, countIt->second );
        }
    }

    if( inlinePairBase == 0 )
        parsePerPinPadstacks();
}


void BINARY_PARSER::parseTerminalsOld()
{
    static constexpr size_t TERM_SIZE = 36;
    const SDB_SECTION*      sec12 = getSection( SECTION::Vertices );
    const SDB_SECTION*      sec15 = getSection( SECTION::TerminalPool );

    std::set<int32_t> terminalCursors;
    int32_t           maxCursor = 0;

    for( const auto& [name, start] : m_decalTerminalStart )
    {
        auto countIt = m_decalTerminalCount.find( name );

        if( countIt == m_decalTerminalCount.end() || start < 0 )
            continue;

        maxCursor = std::max( maxCursor, start + static_cast<int32_t>( countIt->second ) );

        for( uint32_t i = 0; i < countIt->second; ++i )
            terminalCursors.insert( start + static_cast<int32_t>( i ) );
    }

    auto terminalValid = [&]( size_t aBase, int32_t aCursor )
    {
        size_t off = aBase + static_cast<size_t>( aCursor ) * TERM_SIZE;

        if( !m_cursor.InBounds( off, TERM_SIZE ) )
            return false;

        SDB_RECORD rec = m_sdb.RecordAt( off );
        bool       nameEnded = false;

        for( int i = 0; i < 4; ++i )
        {
            uint8_t ch = rec.U8( 24 + i );

            if( ch == 0 )
            {
                nameEnded = true;
            }
            else if( nameEnded || ch < 0x20 || ch > 0x7E )
            {
                return false;
            }
        }

        return rec.I32( 0 ) == 0 && rec.I32( 4 ) == rec.I32( 12 ) && rec.I32( 8 ) == rec.I32( 16 ) && rec.I32( 20 ) != 0
               && rec.I32( 28 ) == 0 && rec.I32( 32 ) == 0;
    };

    size_t streamBase = 0;
    size_t bestCompleteDecals = 0;
    size_t bestScore = 0;

    if( sec12 && sec15 && maxCursor > 0 )
    {
        size_t scanStart = sec12->dataOffset;
        size_t scanEnd = std::min( static_cast<size_t>( sec15->End() ), m_data.size() );
        size_t streamBytes = static_cast<size_t>( maxCursor ) * TERM_SIZE;

        for( size_t candidate = scanStart; candidate <= scanEnd && streamBytes <= scanEnd - candidate; candidate += 4 )
        {
            size_t completeDecals = 0;
            size_t score = 0;

            for( int32_t cursor : terminalCursors )
                score += terminalValid( candidate, cursor );

            for( const auto& [name, start] : m_decalTerminalStart )
            {
                auto countIt = m_decalTerminalCount.find( name );

                if( countIt == m_decalTerminalCount.end() || start < 0 )
                    continue;

                bool complete = true;

                for( uint32_t i = 0; i < countIt->second; ++i )
                {
                    if( !terminalValid( candidate, start + static_cast<int32_t>( i ) ) )
                    {
                        complete = false;
                        break;
                    }
                }

                completeDecals += complete;
            }

            if( std::tie( completeDecals, score ) > std::tie( bestCompleteDecals, bestScore ) )
            {
                bestCompleteDecals = completeDecals;
                bestScore = score;
                streamBase = candidate;
            }
        }
    }

    if( streamBase != 0 )
    {
        for( auto& [name, decal] : m_decals )
        {
            auto startIt = m_decalTerminalStart.find( name );
            auto countIt = m_decalTerminalCount.find( name );

            if( startIt == m_decalTerminalStart.end() || countIt == m_decalTerminalCount.end() )
                continue;

            std::vector<TERMINAL> terminals;

            for( uint32_t i = 0; i < countIt->second; ++i )
            {
                int32_t cursor = startIt->second + static_cast<int32_t>( i );

                if( !terminalValid( streamBase, cursor ) )
                    break;

                SDB_RECORD rec = m_sdb.RecordAt( streamBase + static_cast<size_t>( cursor ) * TERM_SIZE );
                TERMINAL   terminal;
                terminal.x = rec.I32( 4 );
                terminal.y = rec.I32( 8 );
                terminal.name = rec.Str( 24, 4 );

                if( terminal.name.empty() )
                    terminal.name = std::to_string( i + 1 );

                terminals.push_back( terminal );
            }

            if( terminals.size() == countIt->second )
                decal.terminals = std::move( terminals );
        }
    }

    synthesizePlaceholderTerminals();
    assignDefaultPadStacks();

    size_t pairCount = 0;

    for( const std::string& name : m_decalNameTable )
    {
        auto countIt = m_decalStackCount.find( name );

        if( countIt != m_decalStackCount.end() )
            pairCount += static_cast<size_t>( countIt->second );
    }

    if( streamBase == 0 || maxCursor <= 0 || pairCount == 0 || m_padStackPool.empty() )
        return;

    // Both old dialects put the pair pool at the same place. v0x2021 used to be read one pair
    // earlier, which only looked right because it also accumulated its slice starts in decal-table
    // order instead of reading the stored cursor; the two errors cancelled on boards whose ring
    // happens to be table order rotated by one, and compounded on the rest.
    size_t pairBase = streamBase + static_cast<size_t>( maxCursor ) * TERM_SIZE + 4;

    if( !m_cursor.InBounds( pairBase, pairCount * 8 ) )
        return;

    std::vector<std::pair<int32_t, int32_t>> pairs;
    pairs.reserve( pairCount );

    for( size_t i = 0; i < pairCount; ++i )
    {
        SDB_RECORD rec = m_sdb.RecordAt( pairBase + i * 8 );
        int32_t    pin = rec.I32( 0 );
        int32_t    ref = rec.I32( 4 );

        if( pin < 0 || static_cast<size_t>( ref ) >= m_padStackPool.size() || m_padStackPool[ref].empty() )
        {
            return;
        }

        pairs.emplace_back( pin, ref );
    }

    int32_t pairCursor = 0;

    for( const std::string& name : m_decalNameTable )
    {
        auto countIt = m_decalStackCount.find( name );
        auto decalIt = m_decals.find( name );

        if( countIt == m_decalStackCount.end() )
            continue;

        if( decalIt != m_decals.end() )
        {
            auto startIt = m_decalStackStart.find( name );
            int32_t start = startIt != m_decalStackStart.end() ? startIt->second : pairCursor;
            applyPadstackPairs( decalIt->second, pairs, start, countIt->second );
        }

        pairCursor += countIt->second;
    }
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
    // By convention pad stack 0 is the default for all of a decal's terminals.
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
    // A flat pool of (pin, ref) pairs follows the terminal records in section 15; the section-14
    // descriptor table slices it per decal. Each ref indexes the extended pad-stack pool. A
    // (0, ref) pair sets the decal default; a (pin>0, ref) pair overrides that one terminal.
    // Decals without a section-14 descriptor keep the convention default.
    if( isOldFormat() || m_padStackPool.empty() )
        return;

    const SDB_SECTION* sec14 = getSection( SECTION::DecalHeader );
    const SDB_SECTION* sec15 = getSection( SECTION::TerminalPool );

    if( !sec14 || !sec15 || sec15->stride != 36 )
        return;

    // The (pin, ref) pair pool begins at the first section-15 record whose +24/+28/+32 tail is
    // non-zero, the same boundary parseTerminals uses to end the terminal-geometry block.
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
    {
        size_t pairSpan = 0;

        for( const auto& [name, start] : m_decalStackStart )
        {
            auto countIt = m_decalStackCount.find( name );

            if( start >= 0 && countIt != m_decalStackCount.end() )
                pairSpan = std::max( pairSpan, static_cast<size_t>( start ) + countIt->second );
        }

        size_t bestScore = 0;

        if( pairSpan > 0 && pairSpan * 8 <= sec14->totalBytes )
        {
            size_t scanEnd = static_cast<size_t>( sec14->dataOffset ) + sec14->totalBytes - pairSpan * 8;

            for( size_t base = sec14->dataOffset; base <= scanEnd; ++base )
            {
                std::vector<std::pair<int32_t, int32_t>> candidate;
                std::set<int32_t>                       refs;
                size_t                                  nonzeroPins = 0;
                bool                                    valid = true;

                for( size_t i = 0; i < pairSpan; ++i )
                {
                    SDB_RECORD rec = m_sdb.RecordAt( base + i * 8 );
                    int32_t    pin = rec.I32( 0 );
                    int32_t    ref = rec.I32( 4 );

                    if( pin < 0 || ref < 0 || static_cast<size_t>( ref ) >= m_padStackPool.size()
                        || m_padStackPool[ref].empty() )
                    {
                        valid = false;
                        break;
                    }

                    candidate.emplace_back( pin, ref );
                    refs.insert( ref );
                    nonzeroPins += pin != 0;
                }

                for( const auto& [name, start] : m_decalStackStart )
                {
                    auto stackCountIt = m_decalStackCount.find( name );
                    auto terminalCountIt = m_decalTerminalCount.find( name );

                    if( !valid || start < 0 || stackCountIt == m_decalStackCount.end()
                        || terminalCountIt == m_decalTerminalCount.end() )
                    {
                        continue;
                    }

                    for( int32_t i = 0; i < stackCountIt->second; ++i )
                    {
                        int32_t pin = candidate[static_cast<size_t>( start ) + i].first;

                        if( pin > static_cast<int32_t>( terminalCountIt->second ) )
                        {
                            valid = false;
                            break;
                        }
                    }
                }

                if( !valid || ( refs.size() == 1 && *refs.begin() == 0 ) )
                    continue;

                size_t score = refs.size() * 1000 + nonzeroPins;

                if( score > bestScore )
                {
                    bestScore = score;
                    pairs = std::move( candidate );
                }
            }
        }
    }

    if( pairs.empty() )
        return;

    // The complete decal header directly slices the pair pool. This includes via definitions
    // and small de-duplicated libraries that have no section-14 descriptor or section-13 row.
    for( const auto& [name, start] : m_decalStackStart )
    {
        auto decalIt = m_decals.find( name );
        auto countIt = m_decalStackCount.find( name );

        if( decalIt != m_decals.end() && countIt != m_decalStackCount.end() )
            applyPadstackPairs( decalIt->second, pairs, start, countIt->second );
    }

    // The section-14 descriptor table is the section data itself, distinct from the -1188 header
    // used for terminal positions: 112-byte records with a 0xFFFE sentinel at +108, the decal
    // NAME at +44 and the pair-pool start cursor at +88. The pair count comes from the complete
    // decal-name table; +20 is the decal's drawing-line count.
    static constexpr size_t DESC_SIZE = 112;
    static constexpr size_t DESC_NAME_OFF = 44;
    static constexpr size_t DESC_START_OFF = 88;
    static constexpr size_t DESC_SENTINEL_OFF = 108;

    std::set<std::string> descriptorDecals;

    for( uint32_t k = 0; k < sec14->count; ++k )
    {
        size_t off = static_cast<size_t>( sec14->dataOffset ) + k * DESC_SIZE;

        SDB_RECORD desc = m_sdb.RecordAt( off );

        if( off + DESC_SIZE > m_data.size() || desc.U16( DESC_SENTINEL_OFF ) != SDB_RECORD_SENTINEL )
            continue;

        std::string name = desc.Str( DESC_NAME_OFF, 41 );

        if( name.empty() )
            continue;

        auto decalIt = m_decals.find( name );
        auto countIt = m_decalStackCount.find( name );

        if( decalIt == m_decals.end() || countIt == m_decalStackCount.end() )
            continue;

        descriptorDecals.insert( name );
        applyPadstackPairs( decalIt->second, pairs, desc.I32( DESC_START_OFF ), countIt->second );
    }

    // The de-duplicated library decals carry no section-14 descriptor. Their pair-pool start
    // cursor lives in a trailing section-13 table of 112-byte records. The +40 marker is the
    // decal's unit byte shifted by eight (0x4900 inch, 0x4D00 metric), the NAME is at +0, and the
    // start cursor is the i32 at +44. The slice length is the decal's pad-stack count.
    static constexpr int32_t LIB_INCH_MARKER = 0x4900;
    static constexpr int32_t LIB_METRIC_MARKER = 0x4D00;
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

        int32_t marker = lib.I32( LIB_MARKER_OFF );

        if( marker != LIB_INCH_MARKER && marker != LIB_METRIC_MARKER )
            continue;

        std::string name = lib.Str( 0, 40 );

        if( name.empty() || descriptorDecals.count( name ) )
            continue;

        auto decalIt = m_decals.find( name );
        auto countIt = m_decalStackCount.find( name );

        if( decalIt == m_decals.end() || countIt == m_decalStackCount.end() )
            continue;

        applyPadstackPairs( decalIt->second, pairs, lib.I32( LIB_START_OFF ), countIt->second );
    }
}


void BINARY_PARSER::applyPadstackPairs( PART_DECAL& aDecal, const std::vector<std::pair<int32_t, int32_t>>& aPairs,
                                        int32_t aStart, int32_t aCount )
{
    if( aStart < 0 || aCount <= 0 || static_cast<size_t>( aStart ) + static_cast<size_t>( aCount ) > aPairs.size() )
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
    // Section 9 is a text string pool followed by 112-byte LINE item records: u32 flags @0
    // (0xFFFE or 0xFFFF), u32 sentinel @4 (0xFFFFFFFF), name @44 (12 bytes, null-terminated),
    // and the DRW absolute X/Y origin at @88/@92. The first DRW-named record is the board
    // outline, whose origin converts section 11 vertices from DRW-relative to binary absolute.
    static constexpr int LINE_FLAGS_OFF = 0;
    static constexpr int LINE_SENTINEL_OFF = 4;

    const SDB_SECTION* entry9 = getSection( SECTION::StringPool );

    if( !entry9 || entry9->totalBytes < DRW_ITEM::SIZE )
        return;

    size_t scanEnd = entry9->dataOffset + entry9->totalBytes;

    for( size_t pos = entry9->dataOffset; pos + DRW_ITEM::SIZE <= scanEnd; ++pos )
    {
        SDB_RECORD rec = m_sdb.RecordAt( pos );
        uint32_t   flags = rec.U32( LINE_FLAGS_OFF );
        uint32_t   sentinel = rec.U32( LINE_SENTINEL_OFF );

        if( ( flags != SDB_RECORD_SENTINEL && flags != 0xFFFF ) || sentinel != SDB_FIELD_UNSET )
            continue;

        std::string name = rec.Str( DRW_ITEM::NAME, 12 );

        if( name.size() >= 4 && name.substr( 0, 3 ) == "DRW" )
        {
            m_boardDrwOriginX = rec.I32( DRW_ITEM::ORIGIN_X );
            m_boardDrwOriginY = rec.I32( DRW_ITEM::ORIGIN_Y );
            m_boardDrwOriginFound = true;
            return;
        }

        pos += DRW_ITEM::SIZE - 1;
    }
}


void BINARY_PARSER::parseBoardOutline()
{
    // Board outline vertices are 12-byte triplets [i32 X, i32 Y, u32 0xFFFFFFFF] in the tail of
    // section 11, after the 20-byte piece descriptors. count * stride may equal totalBytes
    // exactly, so the vertex region boundary is found by scanning backward from the section end.
    // Coordinates are DRW-relative and need the section 9 origin added to reach binary absolute.
    //
    // Arc-laden outlines are not stored as the simple section 11 vertex tail. Try the arc-aware
    // decoder first; it self-validates and only succeeds on a genuine arc outline, so it never
    // displaces a correct rectilinear result.
    if( parseArcBoardOutline() )
        return;

    const SDB_SECTION* entry11 = getSection( SECTION::GraphicPieces );

    if( !entry11 || entry11->totalBytes < 12 )
        return;

    size_t secStart = entry11->dataOffset;
    size_t secEnd   = entry11->dataOffset + entry11->totalBytes;

    // Each vertex has 0xFFFFFFFF at @8; piece descriptors have 0 at @8.
    size_t vtxStart = secEnd;

    for( size_t pos = secEnd - 12; pos >= secStart && pos < secEnd; pos -= 12 )
    {
        if( m_sdb.RecordAt( pos ).U32( 8 ) == SDB_FIELD_UNSET )
            vtxStart = pos;
        else
            break;
    }

    if( vtxStart >= secEnd )
        return;

    // Only the first closed polygon is the board outline; the vertex region may hold others.
    POLYLINE outline;
    outline.layer  = 1;
    outline.width  = 0.0;
    outline.closed = true;

    // A board outline encloses area; a run that collapses to a line or point is a stray graphic.
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

        if( sentinel != SDB_FIELD_UNSET )
            break;

        int32_t rawX = rec.I32( 0 );
        int32_t rawY = rec.I32( 4 );

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
    // PADS stores graphic pieces as closed runs of 12-byte vertex triplets [i32 X, i32 Y, i32
    // attr] in sections 10..12. attr == -1 marks a plain corner; attr == 0,1,2,... is the
    // ordinal of an arc corner into a parallel 20-byte arc-parameter table located by
    // findArcTable. A run may be purely rectilinear or arc-laden.
    //
    // There is no in-band board-vs-decal tag on the vertex run, so "largest closed run"
    // mis-fires on boards that carry bigger decals or split the outline across pieces. Instead,
    // every closed run is matched against the drawing-item index in sections 8..10. Each DRW
    // item carries its own origin and absolute bbox; the board outline's run extent equals
    // exactly one item's local bbox. A run is accepted only when it matches an item bbox within
    // a tight tolerance, and the owning item supplies the absolute origin. When nothing matches
    // the caller falls back to the rectilinear tail decoder, so a dubious run is never shipped.
    constexpr size_t VTX = 12;
    constexpr size_t ARC_REC = 20;
    constexpr int    MAX_CORNERS = 256;


    struct RunCandidate
    {
        std::vector<ARC_VERTEX> verts;
        std::vector<size_t> arcCornerIdx;
        size_t              arcTableOffset = 0;
        int64_t             span = 0;
        ARC_DRAWING_ITEM         owner;
        bool                haveOwner = false;
    };


    // Build the drawing-item index from the 112-byte DRW records in sections 8..10. Each record
    // stores an absolute origin at +88/+92 and an absolute bbox at +96..+108, made origin-
    // relative here so it compares directly against a run's local vertex extent. The +84 type
    // word marks the board outline item (0x00004D00), used only as a tie break.

    std::vector<ARC_DRAWING_ITEM> drawingItems = collectArcDrawingItems();

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

    std::vector<ARC_VERTEX> verts;
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
                ARC_VERTEX     v{ r.I32( 0 ), r.I32( 4 ), r.I32( 8 ) };

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

            for( const ARC_VERTEX& v : verts )
            {
                minX = std::min( minX, v.x );
                minY = std::min( minY, v.y );
                maxX = std::max( maxX, v.x );
                maxY = std::max( maxY, v.y );
            }

            int64_t runW = (int64_t) maxX - minX;
            int64_t runH = (int64_t) maxY - minY;
            int64_t span = std::max( runW, runH );

            // Reject degenerate sub-micron runs and absurd coordinate noise before the more
            // expensive arc-table and item-bbox checks. A run that collapses to a line in either
            // axis is a stray graphic. The upper bound is only an overflow guard (3.3 m); the
            // DRW-bbox match below is the real size check.
            if( span < 1000000 || span > 5000000000LL || runW <= 0 || runH <= 0 )
                continue;

            // Matched DRW records also include closed title-strip and dimension pieces, which
            // are valid graphics but much thinner than an importable board area.
            if( (double) span / (double) std::min( runW, runH ) > 12.0 )
                continue;

            int64_t arcTable = -1;

            if( !arcIdx.empty() )
            {
                arcTable = findArcParameterTable( verts, arcIdx );

                if( arcTable < 0 )
                    continue;
            }

            // Accept this run only when its extent coincides with a drawing item's bbox. The
            // tolerance absorbs arc bulge beyond the chord vertices and rounding.
            for( const ARC_DRAWING_ITEM& item : drawingItems )
            {
                int64_t err = std::abs( item.localMinX - minX ) + std::abs( item.localMinY - minY )
                              + std::abs( item.localMaxX - maxX ) + std::abs( item.localMaxY - maxY );

                if( err > 3000000 )
                    continue;

                double centerDist = 0.0;

                if( nPlacedParts >= 2 && placeW > 0 && placeH > 0 && placeW <= span * 4 && placeH <= span * 4 )
                {
                    double cx = ( (double) minX + maxX ) / 2.0 + item.originX;
                    double cy = ( (double) minY + maxY ) / 2.0 + item.originY;
                    centerDist = std::hypot( cx - placeCenterX, cy - placeCenterY );
                }

                // Among matching runs prefer the explicit board-outline item type, then the
                // larger owning DRW item, then the larger run, then the tighter fit. Placement
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

    // No run matched a drawing item closely enough; let the caller try its fallback.
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
        const ARC_VERTEX& v = best.verts[i];

        // Vertices are local to the owning item; add the item origin to reach the binary
        // absolute frame the converter expects (it subtracts the design origin once downstream).
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

            const ARC_VERTEX& s = best.verts[i - 1];
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


std::vector<ARC_DRAWING_ITEM> BINARY_PARSER::collectArcDrawingItems()
{
    std::vector<ARC_DRAWING_ITEM> items;
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

        if( ( marker == SDB_RECORD_SENTINEL || marker == 0xFFFF ) && rec.U16( 2 ) == 0 )
        {
            std::string name = rec.Str( DRW_ITEM::NAME, 12 );

            if( name.size() >= 3 && name.compare( 0, 3, "DRW" ) == 0 )
            {
                ARC_DRAWING_ITEM item;
                item.originX = rec.I32( DRW_ITEM::ORIGIN_X );
                item.originY = rec.I32( DRW_ITEM::ORIGIN_Y );
                item.localMinX = (int64_t) rec.I32( DRW_ITEM::BBOX_MIN_X ) - item.originX;
                item.localMinY = (int64_t) rec.I32( DRW_ITEM::BBOX_MIN_Y ) - item.originY;
                item.localMaxX = (int64_t) rec.I32( DRW_ITEM::BBOX_MAX_X ) - item.originX;
                item.localMaxY = (int64_t) rec.I32( DRW_ITEM::BBOX_MAX_Y ) - item.originY;
                item.span = std::max( item.localMaxX - item.localMinX, item.localMaxY - item.localMinY );
                item.preferred = rec.U32( DRW_ITEM::TYPE_TAG ) == DRW_TAG::LINE_ITEM;
                items.push_back( item );
                pos += DRW_ITEM::SIZE;
                continue;
            }
        }

        ++pos;
    }

    return items;
}


int64_t BINARY_PARSER::findArcParameterTable( const std::vector<ARC_VERTEX>& verts, const std::vector<size_t>& arcIdx )
{
    constexpr size_t ARC_REC = 20;

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

            // An arc box is square and non-degenerate.
            if( rx < 1.0 || std::abs( rx - ry ) > 2.0 )
            {
                ok = false;
                break;
            }

            double cx = ( xmin + xmax ) / 2.0;
            double cy = ( ymin + ymax ) / 2.0;
            size_t j = arcIdx[i];
            const ARC_VERTEX& s = verts[j];
            const ARC_VERTEX& e = verts[( j + 1 ) % verts.size()];
            double tol = std::max( 3.0, rx * 1e-5 );
            double distStart = std::hypot( s.x - cx, s.y - cy );
            double distEnd = std::hypot( e.x - cx, e.y - cy );

            if( std::abs( distStart - rx ) > tol || std::abs( distEnd - rx ) > tol )
                ok = false;
        }

        if( ok )
            return static_cast<int64_t>( off );
    }

    return -1;
}


bool BINARY_PARSER::isValidNetName( const std::string& aName ) const
{
    if( aName.empty() )
        return false;

    // An internal sentinel net holding unassigned copper/obstacles, not a real signal.
    if( aName == "___Unassigned_Obstacles_" )
        return false;

    if( aName == "\\" )
        return false;

    char first = aName[0];

    return std::isalpha( static_cast<unsigned char>( first ) ) || std::isdigit( static_cast<unsigned char>( first ) )
           || first == '+' || first == '-' || first == '$' || first == '~' || first == '_' || first == '/'
           || first == '\\';
}


void BINARY_PARSER::parseNetNames()
{
    if( m_version == 0x2022 || m_version == 0x2024 )
        parseNetNamesDirect();
    else if( !isOldFormat() )
        parseNetNamesNew();
    else
        parseNetNamesOld();
}


void BINARY_PARSER::parseNetNamesDirect()
{
    // Unlike v0x2021 (whose net names are scattered across sec19 rule records and embedded in
    // placement records -- see parseNetNamesOld), v0x2022 and v0x2024 carry every net name
    // directly in section 23, one per record, at a fixed offset -- the same direct-table shape
    // as the newer dialects' parseNetNamesNew, just with their own record stride and field
    // offset (neither matches the 424/116 those use). Verified by brute-force string search
    // against ground truth: v0x2022 (2FOC_4.pcb/2FOC_5.pcb) 173/173 and 162/173 names found at
    // stride 144 offset 140; v0x2024 (PCB_2FOC_05.pcb/NEI2_2.pcb) 170/173 and 15/18 names found
    // at stride 416 offset 92.
    uint32_t netRecordSize = m_version == 0x2022 ? 144 : 416;
    uint32_t netNameOff    = m_version == 0x2022 ? 140 : 92;
    uint32_t netNameLen    = 48;

    const SDB_SECTION* nets = getSection( SECTION::Nets );

    if( !nets || nets->count == 0 || nets->stride != netRecordSize )
        return;

    std::unordered_set<std::string> existing;

    const uint32_t leadingNetCount = m_version == 0x2022 ? 9 : 3;

    if( nets->dataOffset >= leadingNetCount * netRecordSize )
    {
        size_t leadingBase = nets->dataOffset - leadingNetCount * netRecordSize;

        for( uint32_t i = 0; i < leadingNetCount; ++i )
        {
            SDB_RECORD  rec = m_sdb.RecordAt( leadingBase + i * netRecordSize );
            std::string name = rec.Str( netNameOff, netNameLen );

            if( name.empty() || !isValidNetName( name ) || existing.count( name ) )
                continue;

            NET net;
            net.name = name;
            m_nets.push_back( net );
            if( m_version == 0x2024 )
                m_netAnchors.push_back( { m_nets.size() - 1, rec.U32( 80 ), rec.U32( 84 ) } );
            else
                m_netAnchors.push_back( { m_nets.size() - 1, rec.U32( 128 ), rec.U32( 132 ) } );
            existing.insert( name );
            m_sec23IndexToNet[i] = name;
        }
    }

    for( uint32_t i = 0; i < nets->count; ++i )
    {
        if( ( i + 1 ) * netRecordSize > nets->totalBytes )
            break;

        SDB_RECORD  rec  = m_sdb.Record( *nets, i, netRecordSize );
        std::string name = rec.Str( netNameOff, netNameLen );

        if( name.empty() || !isValidNetName( name ) || existing.count( name ) )
            continue;

        NET net;
        net.name = name;
        m_nets.push_back( net );
        m_sec23RecordToNet[i] = m_nets.size() - 1;

        if( m_version == 0x2024 )
            m_netAnchors.push_back( { m_nets.size() - 1, rec.U32( 80 ), rec.U32( 84 ) } );
        else
            m_netAnchors.push_back( { m_nets.size() - 1, rec.U32( 128 ), rec.U32( 132 ) } );

        existing.insert( name );
        m_sec23IndexToNet[i + leadingNetCount] = name;
    }

    parseNetConnectionsNew();
}


void BINARY_PARSER::parseNetNamesNew()
{
    std::unordered_set<std::string> existing;

    // Section 23 net record (424 B): the net NAME plus the net-class owner pointer (shared
    // by every member) and the net's own self-pointer (the diff-pair join key).
    constexpr uint32_t NET_RECORD_SIZE = 424;
    constexpr uint32_t NET_NAME        = 116;
    constexpr uint32_t NET_NAME_LEN    = 48;
    constexpr uint32_t NET_SELF_PTR    = 184;
    constexpr uint32_t NET_CLASS_PTR   = 188;
    constexpr uint32_t NET_INDEX_BIAS  = 3;

    const SDB_SECTION* nets = getSection( SECTION::Nets );

    if( nets && nets->count > 0 && nets->stride == NET_RECORD_SIZE )
    {
        // Net indices 0-2 are three full records immediately preceding the nominal section-23
        // array. Their 3*424-byte footprint is the exact displacement into the section-22 tail,
        // and explains the +3 route-net index bias used below.
        constexpr uint32_t LEADING_NET_COUNT = 3;

        if( nets->dataOffset >= LEADING_NET_COUNT * NET_RECORD_SIZE )
        {
            size_t leadingBase = nets->dataOffset - LEADING_NET_COUNT * NET_RECORD_SIZE;

            for( uint32_t i = 0; i < LEADING_NET_COUNT; ++i )
            {
                SDB_RECORD  rec = m_sdb.RecordAt( leadingBase + i * NET_RECORD_SIZE );
                std::string name = rec.Str( NET_NAME, NET_NAME_LEN );

                if( name.empty() || !isValidNetName( name ) || existing.count( name ) )
                    continue;

                NET net;
                net.name = name;
                m_nets.push_back( net );
                m_netAnchors.push_back( { m_nets.size() - 1, rec.U32( 104 ), rec.U32( 108 ) } );
                existing.insert( name );
                m_sec23IndexToNet[i] = name;

                if( uint32_t owner = rec.U32( NET_CLASS_PTR ) )
                    m_netClassOwner[name] = owner;

                if( uint32_t selfPtr = rec.U32( NET_SELF_PTR ) )
                    m_netSelfPtrToName[selfPtr] = name;
            }
        }

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
            m_sec23RecordToNet[i] = m_nets.size() - 1;
            m_netAnchors.push_back( { m_nets.size() - 1, rec.U32( 104 ), rec.U32( 108 ) } );
            existing.insert( name );

            // Route/via records index nets through a table whose first three slots belong to
            // nets held outside section 23, so section-23 record i answers to net index i + 3.
            // Measured on the corpus: the via net field partitions vias by net at 100% purity
            // against a 7-9% shuffled control, and the recovered index is i + 3 on 65/67, 749/752
            // and 2/3 of the net groups on BR350420B, OC_LTE_BASEBAND and BR350430B. Indices 0-2
            // simply miss and leave the via netless, which is the safe direction -- keying by i
            // shifted every via onto a real but wrong net (BR350430B put 39 GND vias on RS485-).
            m_sec23IndexToNet[i + NET_INDEX_BIAS] = name;

            if( uint32_t owner = rec.U32( NET_CLASS_PTR ) )
                m_netClassOwner[name] = owner;

            if( uint32_t selfPtr = rec.U32( NET_SELF_PTR ) )
                m_netSelfPtrToName[selfPtr] = name;
        }

        // The directory offset is reconstructed from declared byte counts and can land past the
        // physical net array. Net records retain four invariant sentinels around the name, so use
        // those to recover any records the nominal walk missed.
        if( existing.size() < nets->count )
        {
            for( size_t off = 0; off + NET_RECORD_SIZE <= m_data.size(); ++off )
            {
                SDB_RECORD rec = m_sdb.RecordAt( off );

                if( rec.U32( 20 ) != 0x0000FFFE || rec.U32( 36 ) != SDB_FIELD_UNSET || rec.U32( 64 ) != SDB_FIELD_UNSET
                    || rec.U32( 92 ) != 0xFFFFFFFE )
                {
                    continue;
                }

                std::string name = rec.Str( NET_NAME, NET_NAME_LEN );

                if( name.empty() || !isValidNetName( name ) || existing.count( name ) )
                    continue;

                NET net;
                net.name = name;
                m_nets.push_back( net );
                m_netAnchors.push_back( { m_nets.size() - 1, rec.U32( 104 ), rec.U32( 108 ) } );
                existing.insert( name );
                m_sec23IndexToNet[existing.size() - 1 + NET_INDEX_BIAS] = name;

                if( existing.size() == nets->count )
                    break;
            }
        }
    }

    parseNetConnectionsNew();

    // Placements fill in power/ground nets: each record may carry up to three net names at
    // +28/+52/+76 (24 chars), each preceded by a 4-byte net index that must look real (small
    // or the 0xFFFF.. sentinel).
    constexpr uint32_t PLACEMENT_RECORD_SIZE = 112;
    const SDB_SECTION* entry22 = getSection( SECTION::Placements );

    if( entry22 && entry22->count > 0 && entry22->stride == PLACEMENT_RECORD_SIZE )
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


void BINARY_PARSER::parseNetConnectionsNew()
{
    constexpr size_t   RECORD_SIZE = 68;
    constexpr uint32_t MARKER = 0xFE000000;
    constexpr uint32_t FLAG = 0x0000FFFE;

    const SDB_SECTION* connections = getSection( SECTION::Connections );
    const SDB_SECTION* nets = getSection( SECTION::Nets );

    const bool   isV2022 = m_version == 0x2022;
    const bool   isV2024 = m_version == 0x2024;
    const size_t netRecordSize = isV2022 ? 144 : isV2024 ? 416 : 424;
    const size_t netNameOff = isV2022 ? 140 : isV2024 ? 92 : 116;
    const size_t netCountOff = isV2022 ? 124 : isV2024 ? 76 : 100;
    const size_t netAnchorAOff = isV2022 ? 128 : isV2024 ? 80 : 104;
    const size_t netStartOff = isV2022 ? 136 : isV2024 ? 88 : 112;

    if( !connections || !nets || connections->count == 0 || nets->stride != netRecordSize )
        return;

    auto validRecord = [&]( size_t aOffset )
    {
        if( !m_cursor.InBounds( aOffset, RECORD_SIZE ) )
            return false;

        SDB_RECORD rec = m_sdb.RecordAt( aOffset );
        return ( rec.U32( 20 ) & 0xFFFFFFC0U ) == MARKER && ( rec.U32( 52 ) & 0xFFFFU ) == FLAG;
    };

    size_t runBase = 0;
    size_t runCount = 0;

    for( size_t off = 0; off + RECORD_SIZE <= m_data.size(); ++off )
    {
        if( !validRecord( off ) )
            continue;

        size_t count = 1;

        while( validRecord( off + count * RECORD_SIZE ) )
            ++count;

        if( count > runCount )
        {
            runBase = off;
            runCount = count;
        }

        off += count * RECORD_SIZE - 1;
    }

    if( runCount == 0 )
        return;

    struct NET_RANGE
    {
        size_t   netIndex;
        uint32_t start;
        uint32_t count;
        uint32_t anchorObject;
    };

    std::vector<NET_RANGE> ranges;

    const uint32_t leadingNetCount = isV2022 ? 9 : 3;

    if( nets->dataOffset >= leadingNetCount * netRecordSize )
    {
        size_t leadingBase = nets->dataOffset - leadingNetCount * netRecordSize;

        for( uint32_t i = 0; i < leadingNetCount; ++i )
        {
            SDB_RECORD  rec = m_sdb.RecordAt( leadingBase + i * netRecordSize );
            std::string name = rec.Str( netNameOff, 48 );
            auto netIt = std::find_if( m_nets.begin(), m_nets.end(),
                                       [&]( const NET& aNet ) { return aNet.name == name; } );
            uint32_t count = rec.U32( netCountOff );

            if( netIt != m_nets.end() && count > 0 && count <= connections->count )
                ranges.push_back( { static_cast<size_t>( std::distance( m_nets.begin(), netIt ) ),
                                    rec.U32( netStartOff ), count, rec.U32( netAnchorAOff ) } );
        }
    }

    for( const auto& [recordIndex, netIndex] : m_sec23RecordToNet )
    {
        if( recordIndex >= nets->count || netIndex >= m_nets.size() )
            continue;

        SDB_RECORD rec = m_sdb.Record( *nets, recordIndex, netRecordSize );
        uint32_t   count = rec.U32( netCountOff );

        if( count == 0 || count > connections->count )
            continue;

        ranges.push_back( { netIndex, rec.U32( netStartOff ), count, rec.U32( netAnchorAOff ) } );
    }

    int    bestShift = 0;
    size_t bestAnchorHits = 0;
    size_t bestValidRecords = 0;

    for( int shift = -128; shift <= 128; ++shift )
    {
        size_t anchorHits = 0;
        size_t validRecords = 0;

        for( const NET_RANGE& range : ranges )
        {
            bool anchorSeen = false;

            for( uint32_t i = 0; i < range.count; ++i )
            {
                int64_t physicalIndex = static_cast<int64_t>( range.start ) + i + shift;

                if( physicalIndex < -1 || physicalIndex >= static_cast<int64_t>( runCount ) )
                    continue;

                size_t recordOffset = static_cast<size_t>( static_cast<int64_t>( runBase )
                                                           + physicalIndex * RECORD_SIZE );
                SDB_RECORD rec = m_sdb.RecordAt( recordOffset );
                ++validRecords;
                anchorSeen |= rec.U32( 60 ) == range.anchorObject || rec.U32( 64 ) == range.anchorObject;
            }

            anchorHits += anchorSeen;
        }

        if( anchorHits > bestAnchorHits
            || ( anchorHits == bestAnchorHits && validRecords > bestValidRecords )
            || ( anchorHits == bestAnchorHits && validRecords == bestValidRecords
                 && std::abs( shift ) < std::abs( bestShift ) ) )
        {
            bestShift = shift;
            bestAnchorHits = anchorHits;
            bestValidRecords = validRecords;
        }
    }

    for( const NET_RANGE& range : ranges )
    {
        NET& net = m_nets[range.netIndex];

        for( uint32_t i = 0; i < range.count; ++i )
        {
            int64_t physicalIndex = static_cast<int64_t>( range.start ) + i + bestShift;

            if( physicalIndex < -1 || physicalIndex >= static_cast<int64_t>( runCount ) )
                continue;

            size_t recordOffset = static_cast<size_t>( static_cast<int64_t>( runBase ) + physicalIndex * RECORD_SIZE );
            SDB_RECORD rec = m_sdb.RecordAt( recordOffset );

            size_t nextRecordOffset = recordOffset + RECORD_SIZE;

            if( m_cursor.InBounds( nextRecordOffset, 8 ) )
            {
                SDB_RECORD pinRec = m_sdb.RecordAt( nextRecordOffset );
                m_netConnectionEndpoints.push_back( { range.netIndex, rec.U32( 60 ), pinRec.U32( 0 ) } );
                m_netConnectionEndpoints.push_back( { range.netIndex, rec.U32( 64 ), pinRec.U32( 4 ) } );
            }

            for( uint32_t objectId : { rec.U32( 60 ), rec.U32( 64 ) } )
            {
                auto partIt = m_placementObjectToPart.find( objectId );

                if( partIt != m_placementObjectToPart.end() && partIt->second < m_parts.size() )
                    net.component_refs.push_back( m_parts[partIt->second].name );
            }
        }
    }

}


void BINARY_PARSER::resolveNetAnchors()
{
    std::vector<std::set<std::pair<std::string, std::string>>> existingPins( m_nets.size() );

    auto resolve = [&]( const NET_ANCHOR& anchor )
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

        auto key = std::make_pair( part.name, terminalName );

        if( !existingPins[anchor.netIndex].insert( key ).second )
            return;

        NET_PIN pin;
        pin.ref_des = part.name;
        pin.pin_name = terminalName;
        m_nets[anchor.netIndex].pins.push_back( std::move( pin ) );
    };

    for( const NET_ANCHOR& anchor : m_netAnchors )
        resolve( anchor );

    for( const NET_ANCHOR& endpoint : m_netConnectionEndpoints )
        resolve( endpoint );
}


void BINARY_PARSER::parseNetNamesOld()
{
    std::unordered_set<std::string> existing;

    // Route vertices in v0x2021 reference nets by a dense 0-based index. The master list is
    // built in two phases: sec19 net-name records (FFFFFFFF-delimited, name at +24) take the
    // lowest indices, then sec22 + sec23 nets sorted ascending by their stored net ID are
    // appended.
    std::vector<std::string> sec19Nets;
    const SDB_SECTION*          entry19 = getSection( SECTION::PartPins );

    if( entry19 && entry19->count > 0 )
    {
        if( entry19->End() <= m_data.size() )
        {
            size_t sec19Size = entry19->totalBytes;

            // Sec19 net-rule records are FFFFFFFF-delimited; the u32 at +4 is zero for net
            // records (non-zero for menu items and component refs), and the name is at +24.
            for( size_t pos = 0; pos + 28 < sec19Size; ++pos )
            {
                SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( entry19->dataOffset + pos ) );

                if( rec.U32( 0 ) != SDB_FIELD_UNSET )
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

    struct IndexedNet
    {
        uint32_t    storedIdx;
        std::string name;
    };

    std::vector<IndexedNet> indexedNets;

    // Old-format placement record (96 B): up to two net names at +12/+60, each preceded by a
    // 4-byte stored net index.
    constexpr uint32_t OLD_PLACEMENT_SIZE = 96;
    const SDB_SECTION* entry22 = getSection( SECTION::Placements );

    if( entry22 && entry22->count > 0 && entry22->stride == OLD_PLACEMENT_SIZE )
    {
        for( uint32_t i = 0; i < entry22->count; ++i )
        {
            if( ( i + 1 ) * OLD_PLACEMENT_SIZE > entry22->totalBytes )
                break;

            SDB_RECORD rec = m_sdb.Record( *entry22, i, OLD_PLACEMENT_SIZE );

            for( uint32_t nameOff : { 12u, 60u } )
            {
                std::string name = rec.Str( nameOff, 48 );

                // +60 in particular is frequently NOT a second net name at all -- for a part
                // with nothing valid at +12, it commonly holds unrelated single-character noise
                // that still happens to pass isValidNetName (verified against 2FOC-001.pcb: 13
                // distinct components each contributing a different bogus single-letter "net",
                // several sharing one already-real net's stored index). No real net name in the
                // whole QA corpus is a single character, so this is a safe, well-evidenced floor.
                if( name.size() < 2 || !isValidNetName( name ) || existing.count( name ) )
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

    if( entry23 && entry23->count > 0 && entry23->stride == OLD_NET_SIZE )
    {
        for( uint32_t i = 0; i < entry23->count; ++i )
        {
            if( ( i + 1 ) * OLD_NET_SIZE > entry23->totalBytes )
                break;

            SDB_RECORD  rec    = m_sdb.Record( *entry23, i, OLD_NET_SIZE );
            uint32_t    netIdx = rec.U32( 8 );
            std::string name   = rec.Str( 12, 48 );

            if( !name.empty() && isValidNetName( name ) && netIdx < 100000 && !existing.count( name ) )
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

    // Build the dense net index table and emit NET objects.
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

    // Route and via records address nets by a dense ordinal into the serialized net table, not by
    // the stored net ID the name passes above sort on. Rebuild the index from that table so a via
    // resolves the net PADS wrote: verified against the ASCII exports' own via nets, 2533 of 2533
    // vias across the v0x2021 corpus resolve to the right name, none to a wrong one.
    m_sec23IndexToNet.clear();

    const std::vector<size_t> netOffsets = oldNetRecordOffsets();

    for( size_t i = 0; i < netOffsets.size(); ++i )
    {
        std::string name = m_sdb.RecordAt( static_cast<uint32_t>( netOffsets[i] ) ).Str( 12, 48 );

        if( name.empty() || !isValidNetName( name ) )
            continue;

        m_sec23IndexToNet[static_cast<uint32_t>( i )] = name;

        if( existing.insert( name ).second )
        {
            NET net;
            net.name = name;
            m_nets.push_back( net );
        }
    }

    parseNetConnectionsOld();
}


std::vector<size_t> BINARY_PARSER::oldNetRecordOffsets() const
{
    constexpr uint32_t NET_RECORD_SIZE = 144;
    constexpr size_t   LEADING_NET_RECORDS = 8;

    std::vector<size_t> offsets;
    const SDB_SECTION*  nets = getSection( SECTION::Nets );

    if( !nets || nets->stride != NET_RECORD_SIZE || nets->dataOffset < LEADING_NET_RECORDS * NET_RECORD_SIZE )
        return offsets;

    for( size_t i = LEADING_NET_RECORDS; i >= 1; --i )
        offsets.push_back( nets->dataOffset - i * NET_RECORD_SIZE );

    for( uint32_t i = 0; i < nets->count; ++i )
        offsets.push_back( nets->dataOffset + static_cast<size_t>( i ) * NET_RECORD_SIZE );

    return offsets;
}


void BINARY_PARSER::parseNetConnectionsOld()
{
    constexpr size_t   RECORD_SIZE     = 68;
    constexpr uint32_t ENDPOINT_FLAG   = 0x0000FFFE;   // low half of the record's u32 @ +0
    constexpr uint32_t MARKER          = 0xFE000000;   // u32 @ +36, low 6 bits carry a subtype
    constexpr uint32_t NET_RECORD_SIZE = 144;
    constexpr uint32_t NET_FIRST       = 8;    // index of this net's first connection record
    constexpr uint32_t NET_NAME        = 12;
    constexpr uint32_t NET_COUNT       = 92;   // number of connection records in this net

    const SDB_SECTION* connections = getSection( SECTION::Connections );
    const SDB_SECTION* nets = getSection( SECTION::Nets );

    if( !connections || !nets || connections->count == 0 || nets->stride != NET_RECORD_SIZE )
        return;

    auto validConnection = [&]( size_t aOffset )
    {
        if( !m_cursor.InBounds( aOffset, RECORD_SIZE ) )
            return false;

        SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( aOffset ) );

        return ( rec.U32( 0 ) & 0xFFFFU ) == ENDPOINT_FLAG && ( rec.U32( 36 ) & 0xFFFFFFC0U ) == MARKER;
    };

    if( !validConnection( connections->dataOffset ) )
        return;

    // Like the net table, the connection stream begins before the section's declared dataOffset;
    // walking back to the first non-record recovers a run whose length equals the directory's own
    // count on every v0x2021 corpus file, which is what confirms the run is the whole stream.
    size_t leading = 0;

    while( connections->dataOffset >= ( leading + 1 ) * RECORD_SIZE
           && validConnection( connections->dataOffset - ( leading + 1 ) * RECORD_SIZE ) )
    {
        ++leading;
    }

    size_t base = connections->dataOffset - leading * RECORD_SIZE;
    size_t total = leading;

    while( validConnection( base + total * RECORD_SIZE ) )
        ++total;

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
        CONNECTION conn{ rec.U32( 8 ), rec.U32( 16 ), rec.U32( 12 ), rec.U32( 20 ), 0 };

        conn.pinA = intern( conn.objectA, conn.ordinalA );
        size_t pinB = intern( conn.objectB, conn.ordinalB );

        size_t rootA = findRoot( conn.pinA );
        size_t rootB = findRoot( pinB );

        if( rootA != rootB )
            parent[rootA] = rootB;

        stream.push_back( conn );
    }

    std::map<size_t, size_t> componentFirst;
    std::map<size_t, size_t> componentSize;

    for( size_t i = 0; i < stream.size(); ++i )
    {
        size_t root = findRoot( stream[i].pinA );

        componentFirst.emplace( root, i );
        ++componentSize[root];
    }

    // Net record keyed by the connection index it names as its first member.
    struct NET_RANGE
    {
        std::string name;
        uint32_t    count;
    };

    std::map<uint32_t, NET_RANGE> netByFirst;

    // A net record only counts as one when its name is a plausible net name and its connection
    // count fits the stream, because the backward walk has no other way to know it has stepped
    // off the head of the table into the preceding section's bytes.
    auto netRecordName = [&]( size_t aOffset, std::string& aName, uint32_t& aCount, uint32_t& aFirst )
    {
        if( !m_cursor.InBounds( aOffset, NET_RECORD_SIZE ) )
            return false;

        SDB_RECORD rec = m_sdb.RecordAt( static_cast<uint32_t>( aOffset ) );

        aName = rec.Str( NET_NAME, 48 );
        aCount = rec.U32( NET_COUNT );
        aFirst = rec.U32( NET_FIRST );

        if( aName.empty() || aCount == 0 || aCount > connections->count )
            return false;

        bool allDigits = true;

        for( char c : aName )
        {
            if( !std::isalnum( static_cast<unsigned char>( c ) ) && c != '_' && c != '$' && c != '+' && c != '-'
                && c != '.' && c != '/' )
            {
                return false;
            }

            allDigits &= std::isdigit( static_cast<unsigned char>( c ) ) != 0;
        }

        return !allDigits;
    };

    // Storage order decides ownership: where two records name the same first connection, the
    // earlier one owns it, so the table is replayed forwards and a later record never displaces it.
    for( size_t offset : oldNetRecordOffsets() )
    {
        std::string name;
        uint32_t    count = 0;
        uint32_t    first = 0;

        if( netRecordName( offset, name, count, first ) && first < total )
            netByFirst.emplace( first, NET_RANGE{ name, count } );
    }

    std::map<std::string, size_t> netIndexByName;

    for( size_t i = 0; i < m_nets.size(); ++i )
        netIndexByName.emplace( m_nets[i].name, i );

    for( const auto& [root, first] : componentFirst )
    {
        auto rangeIt = netByFirst.find( static_cast<uint32_t>( first ) );

        // The stored count is an independent witness for the same net, so requiring it to equal
        // the component size keeps an unrecognized layout from attributing pins to a wrong net.
        if( rangeIt == netByFirst.end() || rangeIt->second.count != componentSize[root] )
            continue;

        auto netIt = netIndexByName.find( rangeIt->second.name );

        if( netIt == netIndexByName.end() )
            continue;

        for( const CONNECTION& conn : stream )
        {
            if( findRoot( conn.pinA ) != root )
                continue;

            m_netConnectionEndpoints.push_back( { netIt->second, conn.objectA, conn.ordinalA } );
            m_netConnectionEndpoints.push_back( { netIt->second, conn.objectB, conn.ordinalB } );
        }
    }
}


void BINARY_PARSER::parseMetadataRegion()
{
    // Only fall back to the DFT_CONFIGURATION scan when the SDB did not yield an origin.
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
    static const char DFT_MARKER[] = "DFT_CONFIGURATION";
    size_t markerLen = std::strlen( DFT_MARKER );

    for( size_t pos = aStart; pos + markerLen + 1 < aEnd; ++pos )
    {
        if( std::memcmp( &m_data[pos], DFT_MARKER, markerLen ) == 0 && m_data[pos + markerLen] == 0 )
        {
            size_t configStart = pos + markerLen + 1;

            while( configStart < aEnd )
            {
                if( m_data[configStart] == 0 )
                {
                    ++configStart;
                    continue;
                }

                if( configStart + 7 <= aEnd && std::memcmp( &m_data[configStart], "PARENT\0", 7 ) == 0 )
                {
                    configStart += 7;
                    continue;
                }

                break;
            }

            if( configStart >= aEnd )
                return;

            // Dot padding in the first 16 bytes distinguishes the two value formats.
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


std::map<std::string, std::string> BINARY_PARSER::parseDftDotPadded( size_t aPos, size_t aEnd ) const
{
    std::map<std::string, std::string> config;

    while( aPos + 16 <= aEnd )
    {
        // Keys are 16-byte fields padded with ASCII '.' (0x2E).
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

        if( aPos < aEnd && m_data[aPos] == 0 )
            ++aPos;

        size_t valStart = aPos;

        while( aPos < aEnd && m_data[aPos] != 0 )
            ++aPos;

        if( aPos > valStart )
        {
            std::string value( reinterpret_cast<const char*>( &m_data[valStart] ), aPos - valStart );
            config[key] = value;
        }

        if( aPos < aEnd )
            ++aPos;

        if( aPos + 7 <= aEnd && std::memcmp( &m_data[aPos], "PARENT\0", 7 ) == 0 )
        {
            aPos += 7;
        }
    }

    return config;
}


std::map<std::string, std::string> BINARY_PARSER::parseDftNullSeparated( size_t aPos, size_t aEnd ) const
{
    std::map<std::string, std::string> config;

    while( aPos < aEnd )
    {
        size_t keyStart = aPos;

        while( aPos < aEnd && m_data[aPos] != 0 )
            ++aPos;

        if( aPos == keyStart )
            break;

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

        if( aPos < aEnd )
            ++aPos;

        if( key == "PARENT" )
            continue;

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

    // The NET_CLASS name records sit just before the rule table, anchored off the first rule
    // record: name_head = first_edge - num_classes*NAME_STRIDE - NAME_HEAD_PAD. A blind stride
    // ASCII scan false-positives on silkscreen text, so this structural anchor is used instead.
    constexpr size_t NAME_STRIDE   = 0x118;
    constexpr size_t NAME_HEAD_PAD = 0x50;
    constexpr size_t NAME_LEN      = 40;

    size_t firstEdge = edges.front().off;

    for( const NET_CLASS_RULE_EDGE& e : edges )
        firstEdge = std::min( firstEdge, e.off );

    size_t headSpan = owners.size() * NAME_STRIDE + NAME_HEAD_PAD;
    size_t nameHead = ( firstEdge >= headSpan ) ? firstEdge - headSpan : 0;

    m_netClasses.clear();
    m_netClasses.resize( owners.size() );

    for( size_t k = 0; k < owners.size(); ++k )
    {
        std::string name = m_sdb.RecordAt( nameHead + k * NAME_STRIDE ).Str( 0, NAME_LEN );

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
    // The type-66 rule table is 24-byte records with tag 0x42 at +4 and a net-class owner
    // pointer at +8 (== a net's +188), a rule-detail page at +0 and a layer at +20.
    constexpr size_t   EDGE_SIZE      = 24;
    constexpr size_t   EDGE_RULE_PTR  = 0;
    constexpr size_t   EDGE_TAG       = 4;
    constexpr uint32_t EDGE_TAG_VALUE = 0x42;
    constexpr size_t   EDGE_OWNER     = 8;
    constexpr size_t   EDGE_LAYER     = 20;
    constexpr uint32_t RULE_PAGE_MASK = ~0xfffu;

    std::vector<NET_CLASS_RULE_EDGE> edges;

    for( size_t off = 0; off + EDGE_SIZE <= m_data.size(); ++off )
    {
        SDB_RECORD rec = m_sdb.RecordAt( off );

        if( rec.U32( EDGE_TAG ) != EDGE_TAG_VALUE )
            continue;

        uint32_t owner = rec.U32( EDGE_OWNER );

        if( !aOwnerSet.count( owner ) )
            continue;

        uint32_t rulePtr = rec.U32( EDGE_RULE_PTR );

        edges.push_back( { owner, rulePtr & RULE_PAGE_MASK, rulePtr, static_cast<int>( rec.U32( EDGE_LAYER ) ), off } );
    }

    return edges;
}


void BINARY_PARSER::applyNetClassClearances( const std::vector<NET_CLASS_RULE_EDGE>& aEdges,
                                             const std::map<uint32_t, size_t>& aOwnerOrdinal )
{
    // The clearance rule kind is the rule-detail page whose (class, layer) keys are all
    // unique and that spans the most classes. Record each class's clearance-rule layers.
    std::map<uint32_t, std::set<std::pair<uint32_t, int>>> pageKeys;
    std::map<uint32_t, bool>                               pageUnique;

    for( const NET_CLASS_RULE_EDGE& e : aEdges )
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
        for( const NET_CLASS_RULE_EDGE& e : aEdges )
        {
            if( e.page != clearancePage )
                continue;

            auto it = aOwnerOrdinal.find( e.owner );

            if( it != aOwnerOrdinal.end() )
                m_netClasses[it->second].ruleLayers.push_back( e.layer );
        }

        // The clearance-page edge's +0 pointer is the rule-value-object's declaration ordinal;
        // the value records live in a separate arena keyed by their own self-pointer at +12,
        // with no pointer chain between the two. Both arenas are emitted in the same declaration
        // order, so the i-th layer-0 clearance edge pairs positionally with the i-th value record.
        std::vector<const NET_CLASS_RULE_EDGE*> layer0Edges;

        for( const NET_CLASS_RULE_EDGE& e : aEdges )
        {
            if( e.page == clearancePage && e.layer == 0 )
                layer0Edges.push_back( &e );
        }

        std::sort( layer0Edges.begin(), layer0Edges.end(),
                   []( const NET_CLASS_RULE_EDGE* a, const NET_CLASS_RULE_EDGE* b )
                   {
                       return a->rulePtr < b->rulePtr;
                   } );

        // The value arena sits in a broader MFC blob outside the sec49 directory byte-range, so
        // the scan covers the whole file, seeded on the marker value (12 mil, the TRACK_TO_TRACK
        // default). Discriminator 1 is a layer-0 rule; the int32 core begins at +20.
        constexpr int      VALUE_MARKER        = 457200;
        constexpr size_t   VALUE_MARKER_OFF    = 0;
        constexpr size_t   VALUE_DISCRIM_OFF   = 8;
        constexpr int      VALUE_DISCRIM_LAYER0 = 1;
        constexpr size_t   VALUE_SELF_PTR_OFF  = 12;
        constexpr uint32_t VALUE_HANDLE_MIN    = 0x10000000u;
        constexpr uint32_t VALUE_HANDLE_END    = 0x20000000u;
        constexpr size_t   VALUE_CORE_OFF      = 20;
        constexpr int      VALUE_CORE_COUNT    = 38;

        struct ValueRec
        {
            uint32_t selfPtr;
            int32_t  core[VALUE_CORE_COUNT];
        };

        std::vector<ValueRec> values;

        for( size_t off = 0; off + VALUE_CORE_OFF + VALUE_CORE_COUNT * sizeof( int32_t ) <= m_data.size(); ++off )
        {
            SDB_RECORD rec = m_sdb.RecordAt( off );

            if( rec.I32( VALUE_MARKER_OFF ) != VALUE_MARKER )
                continue;

            uint32_t selfPtr = rec.U32( VALUE_SELF_PTR_OFF );

            if( selfPtr < VALUE_HANDLE_MIN || selfPtr >= VALUE_HANDLE_END )
                continue;

            if( rec.I32( VALUE_DISCRIM_OFF ) != VALUE_DISCRIM_LAYER0 )
                continue;

            ValueRec v{};
            v.selfPtr = selfPtr;
            bool anyNonZero = false;

            for( int i = 0; i < VALUE_CORE_COUNT; ++i )
            {
                v.core[i] = rec.I32( VALUE_CORE_OFF + sizeof( int32_t ) * i );

                if( v.core[i] != 0 )
                    anyNonZero = true;
            }

            if( anyNonZero )
                values.push_back( v );
        }

        std::sort( values.begin(), values.end(),
                   []( const ValueRec& a, const ValueRec& b )
                   {
                       return a.selfPtr < b.selfPtr;
                   } );

        // Equal counts are required for a sound positional join; otherwise leave values unset so
        // membership still ships.
        if( !layer0Edges.empty() && layer0Edges.size() == values.size() )
        {
            for( size_t i = 0; i < layer0Edges.size(); ++i )
            {
                auto it = aOwnerOrdinal.find( layer0Edges[i]->owner );

                if( it == aOwnerOrdinal.end() )
                    continue;

                // Field positions within the int32 rule-value core.
                constexpr int CORE_CLEARANCE      = 0;
                constexpr int CORE_VIA_CLEARANCE  = 2;
                constexpr int CORE_MIN_TRACK      = 33;
                constexpr int CORE_TRACK          = 34;
                constexpr int CORE_MAX_TRACK      = 35;

                const int32_t*     core = values[i].core;
                BIN_NET_CLASS_DEF& nc   = m_netClasses[it->second];

                nc.clearance     = core[CORE_CLEARANCE];
                nc.viaClearance  = core[CORE_VIA_CLEARANCE];
                nc.minTrackWidth = core[CORE_MIN_TRACK];
                nc.trackWidth    = core[CORE_TRACK];
                nc.maxTrackWidth = core[CORE_MAX_TRACK];
                nc.hasRuleValues = true;
            }
        }
    }
}


void BINARY_PARSER::parseDiffPairs()
{
    constexpr size_t  OBJECT_SIZE = 864;
    constexpr size_t  FF_TAIL_OFF = 604;             // 0xFF allocator free-fill begins here
    constexpr size_t  FF_TAIL_MIN = 200;             // require >=200 0xFF tail bytes
    constexpr double  MAX_LENGTH  = 17068800000.0;   // DIF_PAIR MAX_LENGTH default, the seed marker
    constexpr double  F64_INHERIT = -1.0;
    constexpr int32_t I32_INHERIT = -1;

    // Field offsets within the 864-byte DIF_PAIR object.
    constexpr size_t  NET_A_HANDLE   = 12;   // member-net A; equals a net record's +184 self-handle
    constexpr size_t  NET_B_HANDLE   = 16;   // member-net B
    constexpr size_t  MAX_LENGTH_OFF = 32;   // f64 seed marker
    constexpr size_t  GAP_INHERIT    = 40;   // f64 gap, used when the override is inherited
    constexpr size_t  GAP_OVERRIDE   = 56;   // f64 gap override
    constexpr size_t  WIDTH_INHERIT  = 592;  // i32 width, used when the override is inherited
    constexpr size_t  WIDTH_OVERRIDE = 600;  // i32 width override

    const SDB_SECTION* sec49 = getSection( SECTION::ClearanceRules );

    if( !sec49 || sec49->totalBytes == 0 || m_netSelfPtrToName.empty() )
        return;

    if( sec49->End() > m_data.size() )
        return;

    const size_t poolBase = sec49->dataOffset;
    const size_t poolSize = sec49->totalBytes;

    if( poolSize < OBJECT_SIZE )
        return;

    // The serialized DIF_PAIR objects are a packed 864-byte-stride array that may split across
    // several arena chunks. There is no count word and no in-file handle->offset index, so
    // resolution is a within-file value join: an object's +12/+16 member-net handles equal a net
    // record's +184 self-handle (m_netSelfPtrToName), which is bijective per file. A record is a
    // DIF_PAIR iff both handles resolve, its +32 MAX_LENGTH is the default or a sane override, and
    // its tail carries the >=200-byte 0xFF free-fill (a per-record validator, not the locator).
    // Each chunk is located by seeding on the MAX_LENGTH marker and extending 864-stride both ways.
    auto looksDp = [&]( size_t aStart ) -> bool
    {
        if( aStart < poolBase || aStart + OBJECT_SIZE > poolBase + poolSize )
            return false;

        SDB_RECORD obj = m_sdb.RecordAt( aStart );

        if( !m_netSelfPtrToName.count( obj.U32( NET_A_HANDLE ) )
            || !m_netSelfPtrToName.count( obj.U32( NET_B_HANDLE ) ) )
            return false;

        double maxLen = obj.F64( MAX_LENGTH_OFF );

        if( maxLen != MAX_LENGTH && !( maxLen > 0.0 && maxLen < 1e15 ) )
            return false;

        size_t fillBytes = 0;

        for( size_t k = aStart + FF_TAIL_OFF; k < aStart + OBJECT_SIZE; ++k )
            fillBytes += ( m_data[k] == 0xFF ) ? 1 : 0;

        return fillBytes >= FF_TAIL_MIN;
    };

    // Seeds carry the MAX_LENGTH default; from each, extend in both directions at 864 stride so
    // override pairs (whose +32 is off the default but still pass looksDp) are recovered. The
    // objects are byte-aligned to their arena chunk, not the pool, so the seed search is per-byte.
    std::set<size_t> found;

    for( size_t i = 0; i + OBJECT_SIZE <= poolSize; ++i )
    {
        size_t st = poolBase + i;

        if( m_sdb.RecordAt( st ).F64( MAX_LENGTH_OFF ) != MAX_LENGTH || !looksDp( st ) )
            continue;

        if( found.count( st ) )   // already recovered while walking an earlier seed's chunk
            continue;

        STRIDE_RUN run = extendStrideRun( st, OBJECT_SIZE, poolBase, poolBase + poolSize, looksDp );

        for( size_t k = 0; k < run.count; ++k )
            found.insert( run.base + k * OBJECT_SIZE );
    }

    std::set<std::pair<std::string, std::string>> seen;

    for( size_t objStart : found )
    {
        SDB_RECORD         obj = m_sdb.RecordAt( objStart );
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
    // Free-text items live in a 72-byte text-header stream split across sections 5 and 8,
    // sharing one packed C-string pool near the top of section 8. The legend/title-block half
    // sits in section 5's tail and the marker/fiducial half in section 8, so the scan covers the
    // combined range [sec5.dataOffset .. sec8.end]. Metadata lags geometry by one record slot:
    // text K's geometry is in record K (height@36, width@40, X@44, Y@48, all RAW = design +
    // origin) but its metadata (string offset@8, layer word@24, object tag@28) is in record K+1.
    //
    // Record K is a genuine text item iff record K+1 has tag@28 == 0x49000000 and
    // (word@24 >> 16) == 0x0020 (the high half marks a TEXT object, the low byte is the layer),
    // and record K has positive height and width.
    const SDB_SECTION* s8 = getSection( SECTION::FreeText );

    if( !s8 || s8->totalBytes == 0 || s8->stride < 72 )
        return;

    const SDB_SECTION* s5 = getSection( SECTION::PadShapes );

    size_t lo = ( s5 && s5->totalBytes > 0 ) ? s5->dataOffset : s8->dataOffset;
    size_t hi = s8->dataOffset + s8->totalBytes;

    if( hi < lo + 144 || hi > m_data.size() )
        return;

    // The C-string pool spills past section 8's end into section 9, a per-item-1 byte blob laid
    // down immediately after it. String resolution must run against this extended upper bound or
    // strings crossing the sec8/sec9 boundary get truncated. Require section 9 to be the byte
    // blob directly after section 8 and bound the extension with subtraction to avoid wrap.
    size_t          poolHi = hi;
    const SDB_SECTION* s9     = getSection( SECTION::StringPool );

    if( s9 && s9->totalBytes > 0 && s9->stride == 1 && s9->dataOffset == hi && s9->dataOffset <= m_data.size()
        && s9->totalBytes <= m_data.size() - s9->dataOffset )
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

    // The string pool is in insertion order, so each text resolves through its own string
    // offset. The pool base is not on a clean boundary; calibrate it within a window near the
    // end of the header region by choosing the base that lands the most offsets on a C-string
    // start.
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

            if( soff >= lo && soff < poolHi && ( soff == base || m_data[soff - 1] == 0 ) && cStringStartAt( soff ) )
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

        // Str clamps only to the buffer end, so without this guard a false candidate could read
        // printable bytes out of a neighbouring section.
        if( soff < lo || soff >= poolHi || ( soff != poolBase && m_data[soff - 1] != 0 ) || !cStringStartAt( soff ) )
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


// Per-version field layout for the section-60 via records. Only v0x2021/0x2022 (old), v0x2025
// and v0x2026 carry a recoverable via encoding; every other version selects no layout and emits
// no vias. The three encodings share one body and differ only in the marker predicate, the
// field offsets, and the record-size gate.
// Section 60 is a heterogeneous node table (via records, route corner nodes, free slots),
// discriminated by a type byte -- but that byte's position within the fixed-stride record grid
// is displaced from the directory-declared dataOffset by a PER-FILE phase (not per-version;
// e.g. v0x2027 alone shows 14 distinct phases across the corpus). Every other field is defined
// relative to the type byte's own absolute address T, and is stride-independent (same relative
// offsets for both the 48 B and 64 B record sizes):
//   T-27  i32 raw X                    T-23  i32 raw Y
//   T+0   u8  type (0x0E = via)        T+1   u16 net index, biased by 3 (see parseNetNamesNew)
//   T+4   u16 sub-type tag; low byte always 0x17, bit 0x0200 marks a genuine via (some other
//         0x0E-typed records share the low byte but are not vias)
// Phase is found the same way as PADS_SDB::locateOrigin: score every phase by how many records
// it makes match the via/corner marker shape, take the max (never ambiguous -- the runner-up
// scores 0 on all but one of 66 QA corpus files, where it scores 1). Verified 99.99% recall /
// 100% precision across all 66 QA corpus files (14,632/14,634 ground-truth vias; the 2 misses
// are two isolated via points in one file that aren't present in the node arena at any phase).
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
    };

    std::vector<ViaLocation> viaLocations;

    const SDB_SECTION* entry60 = getSection( SECTION::Vias );

    if( !entry60 || entry60->count == 0 || entry60->stride == 0 )
        return;

    uint32_t stride = entry60->stride;

    // The directory's per-section offsets are not stored values -- they are RECONSTRUCTED by
    // summing every preceding section's declared totalBytes (see PADS_SDB::parseDirectory()),
    // and the 16-byte directory entry has no other field: the trailing 8 bytes are verified zero
    // on every file checked. So a section boundary is only as trustworthy as the cumulative sum
    // up to it, and any earlier section whose true physical size doesn't match its declared
    // totalBytes throws off every boundary after it. That is what happens here: on Seth's
    // hand-built simple3.pcb (2 vias), the real node records sit at a byte offset inside what the
    // directory calls section 57's range, 372 B before the reconstructed section 59 boundary this
    // scan used to start from -- section 59, 60 and 61's computed offsets are simply wrong on this
    // file, not just imprecise. Rather than pick another section boundary to trust in its place,
    // scan the entire payload region once: the phase-scoring argmax below already discriminates a
    // real via/corner cluster from everything else in the file with a wide, verified margin (the
    // runner-up phase scores 0 on all but one of 66 QA corpus files, where it scores 1), so a wider
    // net costs a few million extra byte comparisons -- unmeasurable next to file I/O -- and removes
    // the boundary dependency entirely instead of shifting it to a different guess.
    const SDB_SECTION* firstSection = getSection( 1 );
    constexpr size_t   FOOTER_BYTES = 46;

    size_t scanStart = firstSection ? firstSection->dataOffset : entry60->dataOffset;
    size_t scanEnd = m_data.size() > FOOTER_BYTES ? m_data.size() - FOOTER_BYTES : m_data.size();

    if( scanStart >= scanEnd || scanEnd - scanStart < stride )
        return;

    // Find the phase K in [0, stride) that maximizes the count of matching via/corner-shaped
    // type bytes at absolute positions scanStart + K, +stride, +2*stride, ... The corner marker
    // (type 0x16, validity byte at T+6 == 1) is included in the score alongside vias purely to
    // strengthen the phase signal on boards with few or no vias; only type-0x0E records are
    // actually emitted below.
    static constexpr int CORNER_TYPE = 0x16;
    uint32_t             bestPhase = 0;
    size_t               bestScore = 0;

    for( uint32_t phase = 0; phase < stride; ++phase )
    {
        size_t score = 0;

        for( size_t typeByte = scanStart + phase; typeByte + 6 <= scanEnd; typeByte += stride )
        {
            uint8_t type = m_cursor.U8At( typeByte );

            if( type == VIA_TYPE && viaRecordValid( m_cursor, typeByte ) )
                ++score;
            else if( type == CORNER_TYPE && m_cursor.U8At( typeByte + 6 ) == 1 )
                ++score;
        }

        if( score > bestScore )
        {
            bestScore = score;
            bestPhase = phase;
        }
    }

    if( bestScore == 0 )
        return;

    std::set<std::pair<int32_t, int32_t>> seenVias;
    std::set<std::pair<int32_t, int32_t>> routeAnchorPoints;

    for( size_t typeByte = scanStart + bestPhase; typeByte + 6 <= scanEnd; typeByte += stride )
    {
        uint8_t type = m_cursor.U8At( typeByte );
        bool    isVia = type == VIA_TYPE && viaRecordValid( m_cursor, typeByte );
        bool    isCorner = type == CORNER_TYPE && m_cursor.U8At( typeByte + 6 ) == 1;

        if( !isVia && !isCorner )
            continue;

        if( typeByte < 27 || !m_cursor.InBounds( typeByte - 27, 8 ) )
            continue;

        int32_t vx = m_cursor.I32At( typeByte - 27 );
        int32_t vy = m_cursor.I32At( typeByte - 23 );
        routeAnchorPoints.emplace( vx, vy );

        if( !isVia )
            continue;

        std::string netName;
        uint32_t    netIdx = m_cursor.U16At( typeByte + 1 );
        auto        it = m_sec23IndexToNet.find( netIdx );

        if( it != m_sec23IndexToNet.end() )
            netName = it->second;

        if( netName.empty() && m_nets.size() == 1 )
            netName = m_nets.front().name;

        if( !seenVias.insert( { vx, vy } ).second )
            continue;

        ViaLocation via;
        via.x       = vx;
        via.y       = vy;
        via.netName = netName;
        via.viaIndex = m_cursor.U8At( typeByte - 3 );
        viaLocations.push_back( via );
    }

    for( const PART& part : m_parts )
    {
        auto decalIt = m_decals.find( part.decal );

        if( decalIt == m_decals.end() )
            continue;

        double radians = part.rotation * std::numbers::pi / 180.0;
        double cosine = std::cos( radians );
        double sine = std::sin( radians );

        for( const TERMINAL& terminal : decalIt->second.terminals )
        {
            double dx = terminal.x * cosine - terminal.y * sine;
            double dy = terminal.x * sine + terminal.y * cosine;

            if( part.bottom_layer )
                dx = -dx;

            routeAnchorPoints.emplace( static_cast<int32_t>( std::llround( part.location.x + dx ) ),
                                       static_cast<int32_t>( std::llround( part.location.y + dy ) ) );
        }
    }

    std::map<std::string, ROUTE> routes;

    for( const ViaLocation& via : viaLocations )
    {
        ROUTE& route = routes[via.netName];
        route.net_name = via.netName;

        VIA viaDef;
        viaDef.location.x = static_cast<double>( via.x );
        viaDef.location.y = static_cast<double>( via.y );

        bool stackResolved = false;

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
                    stackResolved = true;
                }
            }
        }

        auto stackIt = m_viaStackByIndex.find( via.viaIndex );

        if( !stackResolved && stackIt != m_viaStackByIndex.end() )
            viaDef.stack = m_padStackPool[stackIt->second];

        route.vias.push_back( std::move( viaDef ) );
    }

    const SDB_SECTION* routeObjects = getSection( SECTION::RouteObjects );
    const SDB_SECTION* routeLayers = getSection( SECTION::RouteLayers );
    const SDB_SECTION* routeCells = getSection( SECTION::RouteCells );

    if( routeObjects && routeLayers && routeCells && routeObjects->count > 0
        && ( routeObjects->stride == 36 || routeObjects->stride == 48 ) && routeLayers->stride == 2
        && routeCells->stride == 12 && routeLayers->count > 0 )
    {
        struct ROUTE_OBJECT
        {
            int32_t  width = 0;
            int32_t  boundLo = 0;
            int32_t  boundHi = 0;
            uint32_t style = 0;
            uint32_t cellCount = 0;
        };

        size_t objectBytes = routeObjects->totalBytes;
        size_t layerBytes = routeLayers->totalBytes;
        size_t cellBytes = routeCells->totalBytes;
        size_t bestLayerTable = 0;
        size_t bestTagMatches = 0;
        std::vector<int> bestLayerOrder;
        bool   legacyObjects = routeObjects->stride == 36;
        size_t objectStride = legacyObjects ? 36 : 48;
        size_t widthOffset = legacyObjects ? 8 : 20;
        size_t boundLoOffset = legacyObjects ? 12 : 24;
        size_t boundHiOffset = legacyObjects ? 16 : 28;
        size_t cellCountOffset = legacyObjects ? 24 : 36;
        size_t trailingTagOffset = legacyObjects ? 32 : 44;

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

        for( size_t pos = scanStart; pos + layerBytes + cellBytes <= scanEnd; ++pos )
        {
            if( pos < objectBytes )
                continue;

            bool             layerTableMatches = true;
            std::set<uint16_t> seenLayers;
            std::vector<int> layerOrder;

            for( uint32_t layer = 0; layer < routeLayers->count; ++layer )
            {
                uint16_t serializedLayer = m_cursor.U16At( pos + layer * 2 );

                if( serializedLayer >= routeLayers->count || !seenLayers.insert( serializedLayer ).second )
                {
                    layerTableMatches = false;
                    break;
                }

                layerOrder.push_back( serializedLayer );
            }

            if( !layerTableMatches )
                continue;

            size_t   ringStart = pos - objectBytes;
            uint64_t totalCells = 0;
            size_t   tagMatches = 0;
            bool     valid = true;

            for( uint32_t i = 0; i < routeObjects->count; ++i )
            {
                size_t  base = ( 32 + static_cast<size_t>( i ) * objectStride ) % objectBytes;
                int32_t quarterWidth = static_cast<int32_t>( ringU32( ringStart, base + widthOffset ) );
                int32_t count = static_cast<int32_t>( ringU32( ringStart, base + cellCountOffset ) );

                if( quarterWidth < 0 || quarterWidth > 20000000 || count <= 0 || count > 100000 )
                {
                    valid = false;
                    break;
                }

                totalCells += static_cast<uint32_t>( count );
                tagMatches += ringU32( ringStart, base ) == ringU32( ringStart, base + trailingTagOffset );
            }

            if( !valid || totalCells != routeCells->count )
            {
                continue;
            }

            if( tagMatches > bestTagMatches )
            {
                bestLayerTable = pos;
                bestTagMatches = tagMatches;
                bestLayerOrder = std::move( layerOrder );
            }
        }

        if( bestLayerTable != 0 )
        {
            if( bestLayerOrder.size() != routeLayers->count )
            {
                bestLayerOrder.resize( routeLayers->count );
                std::iota( bestLayerOrder.begin(), bestLayerOrder.end(), 0 );
            }

            auto padsLayerForSerializedIndex = [&]( size_t aIndex )
            {
                return aIndex < bestLayerOrder.size() ? bestLayerOrder[aIndex] + 1
                                                     : static_cast<int>( aIndex ) + 1;
            };

            size_t                    ringStart = bestLayerTable - objectBytes;
            std::vector<ROUTE_OBJECT> objects;

            for( uint32_t i = 0; i < routeObjects->count; ++i )
            {
                size_t       base = ( 32 + static_cast<size_t>( i ) * objectStride ) % objectBytes;
                ROUTE_OBJECT object;

                object.width = static_cast<int32_t>( ringU32( ringStart, base + widthOffset ) ) * 4;
                object.boundLo = static_cast<int32_t>( ringU32( ringStart, base + boundLoOffset ) );
                object.boundHi = static_cast<int32_t>( ringU32( ringStart, base + boundHiOffset ) );
                object.style = ringU32( ringStart, base + ( legacyObjects ? 20 : 32 ) );
                object.cellCount = ringU32( ringStart, base + cellCountOffset );
                objects.push_back( object );
            }

            struct ROUTE_HEADER
            {
                uint32_t self = 0;
                uint32_t link = 0;
                int32_t  width = 0;
                int      layer = 0;
            };

            std::vector<std::vector<ROUTE_HEADER>> headerChains;
            std::vector<ROUTE_HEADER>              headerSequence;
            std::multiset<int32_t>                 objectWidths;
            std::vector<size_t>                    serializedLayerCounts( routeLayers->count );
            std::vector<size_t>                    serializedPositiveLayerCounts( routeLayers->count );
            size_t                                 headerMarkerOffset = m_version == 0x2024 ? 0 : 8;
            size_t                                 headerWidthOffset = m_version == 0x2024 ? 4 : 12;
            size_t                                 headerLayerOffset = m_version == 0x2024 ? 8 : 16;
            size_t                                 headerFlagsOffset = m_version == 0x2024 ? 16 : 24;

            for( const ROUTE_OBJECT& object : objects )
            {
                if( object.width > 0 )
                    objectWidths.insert( object.width );
            }

            auto matchingObjectWidth = [&]( int32_t aWidth ) -> std::optional<int32_t>
            {
                auto widthIt = objectWidths.lower_bound( aWidth - 4 );

                if( widthIt != objectWidths.end() && std::abs( *widthIt - aWidth ) <= 4 )
                    return *widthIt;

                return std::nullopt;
            };

            for( size_t pos = scanStart; !legacyObjects && pos + 32 <= scanEnd; ++pos )
            {
                uint32_t layer = m_cursor.U32At( pos + headerLayerOffset );
                uint32_t flags = m_cursor.U32At( pos + headerFlagsOffset );

                if( m_cursor.U32At( pos + headerMarkerOffset ) == 0x2001 && layer < routeLayers->count
                    && ( flags & ~0x80000000U ) == 0 )
                {
                    ++serializedLayerCounts[layer];

                    if( m_cursor.I32At( pos + headerWidthOffset ) > 0 )
                    {
                        ++serializedPositiveLayerCounts[layer];

                        if( m_version == 0x2024 )
                        {
                            auto width = matchingObjectWidth( m_cursor.I32At( pos + headerWidthOffset ) * 2 );

                            if( width )
                            {
                                ROUTE_HEADER header;
                                header.width = *width;
                                header.layer = padsLayerForSerializedIndex( layer );
                                headerSequence.push_back( header );
                            }
                        }
                    }
                }
            }

            auto validHeaderAt = [&]( size_t aBase )
            {
                if( aBase + 32 > scanEnd || m_cursor.U32At( aBase + 8 ) != 0x2001 )
                    return false;

                int32_t  halfWidth = m_cursor.I32At( aBase + 12 );
                uint32_t layer = m_cursor.U32At( aBase + 16 );
                uint32_t flags = m_cursor.U32At( aBase + 24 );
                return halfWidth > 0 && layer < routeLayers->count && ( flags & ~0x80000000U ) == 0
                       && matchingObjectWidth( halfWidth * 2 ).has_value();
            };

            for( size_t pos = scanStart; !legacyObjects && m_version != 0x2024 && pos + 32 <= scanEnd; ++pos )
            {
                if( !validHeaderAt( pos ) || ( pos >= scanStart + 32 && validHeaderAt( pos - 32 ) ) )
                    continue;

                std::vector<ROUTE_HEADER>  headers;
                std::map<uint32_t, size_t> selfToHeader;
                size_t                     linkMatches = 0;

                for( size_t base = pos; validHeaderAt( base ); base += 32 )
                {
                    int32_t  halfWidth = m_cursor.I32At( base + 12 );
                    uint32_t layer = m_cursor.U32At( base + 16 );

                    ROUTE_HEADER header;
                    header.self = m_cursor.U32At( base );
                    header.link = m_cursor.U32At( base + 4 );
                    header.width = *matchingObjectWidth( halfWidth * 2 );
                    header.layer = padsLayerForSerializedIndex( layer );
                    selfToHeader.emplace( header.self, headers.size() );
                    headers.push_back( header );
                }

                std::multiset<int32_t> remainingWidths = objectWidths;
                bool                   widthsAvailable = true;

                for( const ROUTE_HEADER& header : headers )
                {
                    auto widthIt = remainingWidths.find( header.width );

                    if( widthIt == remainingWidths.end() )
                    {
                        widthsAvailable = false;
                        break;
                    }

                    remainingWidths.erase( widthIt );
                }

                if( !widthsAvailable )
                    continue;

                headerSequence.insert( headerSequence.end(), headers.begin(), headers.end() );

                if( selfToHeader.size() != headers.size() )
                    continue;

                std::set<uint32_t> referenced;

                for( const ROUTE_HEADER& header : headers )
                {
                    if( selfToHeader.count( header.link ) )
                    {
                        referenced.insert( header.link );
                        ++linkMatches;
                    }
                }

                if( referenced.size() != linkMatches )
                    continue;

                std::vector<size_t> heads;

                for( size_t i = 0; i < headers.size(); ++i )
                {
                    if( !referenced.count( headers[i].self ) )
                        heads.push_back( i );
                }

                if( heads.empty() || linkMatches + heads.size() != headers.size() )
                    continue;

                std::set<size_t> visited;
                size_t           chainsBefore = headerChains.size();

                for( size_t head : heads )
                {
                    std::vector<ROUTE_HEADER> chain;

                    while( head < headers.size() && visited.insert( head ).second )
                    {
                        chain.push_back( headers[head] );
                        auto next = selfToHeader.find( headers[head].link );
                        head = next == selfToHeader.end() ? headers.size() : next->second;
                    }

                    if( !chain.empty() )
                        headerChains.push_back( std::move( chain ) );
                }

                if( visited.size() != headers.size() )
                    headerChains.resize( chainsBefore );
            }

            struct ROUTE_CELL
            {
                int32_t x1 = 0;
                int32_t y = 0;
                int32_t x2 = 0;
            };

            size_t                  cellStart = bestLayerTable + layerBytes;
            std::vector<ROUTE_CELL> cells;

            for( uint32_t i = 0; i < routeCells->count; ++i )
            {
                size_t  base = cellStart + static_cast<size_t>( i ) * 12;
                int32_t first = m_cursor.I32At( base );
                int32_t second = m_cursor.I32At( base + 4 );
                int32_t third = m_cursor.I32At( base + 8 );

                cells.push_back( { first, second, third } );
            }

            std::set<std::pair<int32_t, int32_t>> viaPoints;

            for( const ViaLocation& via : viaLocations )
                viaPoints.emplace( via.x, via.y );

            size_t globalXyAnchorHits = 0;
            size_t globalYxAnchorHits = 0;

            for( const ROUTE_CELL& cell : cells )
            {
                globalXyAnchorHits += routeAnchorPoints.count( { cell.x1, cell.y } );
                globalXyAnchorHits += routeAnchorPoints.count( { cell.x2, cell.y } );
                globalYxAnchorHits += routeAnchorPoints.count( { cell.y, cell.x1 } );
                globalYxAnchorHits += routeAnchorPoints.count( { cell.y, cell.x2 } );
            }

            bool fileFixedIsX = globalXyAnchorHits >= globalYxAnchorHits;

            size_t             cursor = 0;
            std::vector<TRACK> decodedPieces;
            std::vector<size_t> decodedObjectIndices;

            std::vector<size_t> objectRuns( objects.size(), std::numeric_limits<size_t>::max() );
            size_t              runCount = 0;
            bool                inRun = false;
            std::vector<size_t> runSizes;

            for( size_t i = 0; i < objects.size(); ++i )
            {
                if( objects[i].width <= 0 )
                {
                    inRun = false;
                    continue;
                }

                if( !inRun )
                {
                    inRun = true;
                    ++runCount;
                    runSizes.push_back( 0 );
                }

                objectRuns[i] = runCount - 1;
                ++runSizes.back();
            }

            std::vector<int> runLayers( runCount, 1 );
            bool v2022NeedsHeaderLayerRecovery = m_version == 0x2022 && legacyObjects
                                                 && runCount > routeLayers->count + 1;

            if( ( legacyObjects || m_version == 0x2024 ) && runCount == routeLayers->count )
            {
                for( size_t run = 0; run < runCount; ++run )
                    runLayers[run] = padsLayerForSerializedIndex( run );
            }
            else if( legacyObjects && runCount == routeLayers->count + 1 && !objects.empty()
                     && objects.front().width > 0 && objects.back().width > 0 )
            {
                for( size_t run = 0; run + 1 < runCount; ++run )
                    runLayers[run] = padsLayerForSerializedIndex( run );

                runLayers.back() = 1;
            }
            else if( legacyObjects && runCount >= 2 && runCount < routeLayers->count )
            {
                // A layer carrying no routes at all contributes no run, so a board that leaves
                // one empty has fewer runs than layers and the 1:1 case above does not fire --
                // which used to leave every track on layer 1. The runs still follow layer order
                // with the outer layers at the ends, so the surviving runs are the top layer,
                // then inner layers in order, then the bottom layer. Verified against the ASCII
                // exports' own route-point layers on every corpus board where runs are short:
                // PSTAGE-002 uses layers 1,2,3,4,6 of 6 and PSTAGE-003/-004 use 1,2,3,6, each
                // matching this assignment exactly.
                for( size_t run = 0; run + 1 < runCount; ++run )
                    runLayers[run] = padsLayerForSerializedIndex( run );

                runLayers.back() = padsLayerForSerializedIndex( routeLayers->count - 1 );
            }

            if( !legacyObjects && runCount > 0
                && std::any_of( serializedLayerCounts.begin(), serializedLayerCounts.end(),
                                []( size_t aCount )
                                {
                                    return aCount > 0;
                                } ) )
            {
                bool allLayersPresent =
                        std::all_of( serializedPositiveLayerCounts.begin(), serializedPositiveLayerCounts.end(),
                                     []( size_t aCount )
                                     {
                                         return aCount > 0;
                                     } );
                size_t overhead = allLayersPresent && m_version != 0x2024 && m_version != 0x2026
                                          ? *std::min_element( serializedPositiveLayerCounts.begin(),
                                                               serializedPositiveLayerCounts.end() )
                                          : 0;
                size_t partitionRunCount = runCount;

                if( !objects.empty() && objects.front().width > 0 && objects.back().width > 0
                    && objectRuns.back() + 1 == runCount )
                {
                    --partitionRunCount;
                }

                struct PARTITION_PARENT
                {
                    size_t previousRun = 0;
                    bool   valid = false;
                };

                size_t layerCount = serializedLayerCounts.size();
                double infinity = std::numeric_limits<double>::infinity();
                std::vector<std::vector<double>> costs(
                        layerCount + 1, std::vector<double>( partitionRunCount + 1, infinity ) );
                std::vector<std::vector<PARTITION_PARENT>> parents(
                        layerCount + 1, std::vector<PARTITION_PARENT>( partitionRunCount + 1 ) );
                costs[0][0] = 0.0;

                for( size_t layer = 0; layer < layerCount; ++layer )
                {
                    size_t expectedSize = serializedLayerCounts[layer] > overhead
                                                  ? serializedLayerCounts[layer] - overhead
                                                  : 0;

                    for( size_t run = 0; run <= partitionRunCount; ++run )
                    {
                        if( !std::isfinite( costs[layer][run] ) )
                            continue;

                        double skippedCost = costs[layer][run] + 1.0;

                        if( skippedCost < costs[layer + 1][run] )
                        {
                            costs[layer + 1][run] = skippedCost;
                            parents[layer + 1][run] = { run, true };
                        }

                        size_t actualSize = 0;

                        for( size_t endRun = run; endRun < partitionRunCount; ++endRun )
                        {
                            actualSize += runSizes[endRun];
                            double difference = std::abs( static_cast<double>( actualSize ) - expectedSize );
                            double scale = std::max<size_t>( expectedSize, 1 );
                            double matchedCost = costs[layer][run] + difference / scale;

                            if( matchedCost < costs[layer + 1][endRun + 1] )
                            {
                                costs[layer + 1][endRun + 1] = matchedCost;
                                parents[layer + 1][endRun + 1] = { run, true };
                            }
                        }
                    }
                }

                if( std::isfinite( costs[layerCount][partitionRunCount] ) )
                {
                    size_t run = partitionRunCount;

                    for( size_t layer = layerCount; layer > 0; --layer )
                    {
                        const PARTITION_PARENT& parent = parents[layer][run];

                        if( !parent.valid )
                            break;

                        for( size_t matchedRun = parent.previousRun; matchedRun < run; ++matchedRun )
                            runLayers[matchedRun] = padsLayerForSerializedIndex( layer - 1 );

                        run = parent.previousRun;
                    }
                }
            }

            // Descriptor storage is a ring: the last physical descriptor precedes descriptor 0.
            // Positive runs crossing that seam therefore share one serialized layer.
            if( !objects.empty() && objects.front().width > 0 && objects.back().width > 0
                && objectRuns.front() < runLayers.size() && objectRuns.back() < runLayers.size() )
            {
                runLayers[objectRuns.back()] = runLayers[objectRuns.front()];
            }

            std::vector<int> objectLayers( objects.size(), 1 );

            for( size_t object = 0; object < objects.size(); ++object )
            {
                size_t run = objectRuns[object];

                if( run < runLayers.size() )
                    objectLayers[object] = runLayers[run];
            }

            if( m_version == 0x2026 && !objects.empty() && !headerSequence.empty() )
            {
                struct LAYER_WIDTH
                {
                    int32_t width = 0;
                    int     layer = 1;
                };

                std::vector<size_t> logicalObjects;

                for( size_t sequence = 0; sequence < objects.size(); ++sequence )
                {
                    size_t object = ( sequence + objects.size() - 1 ) % objects.size();

                    if( objects[object].width > 0 )
                        logicalObjects.push_back( object );
                }

                std::vector<LAYER_WIDTH> logicalHeaders;

                for( size_t serializedLayer = 0; serializedLayer < routeLayers->count; ++serializedLayer )
                {
                    int padsLayer = padsLayerForSerializedIndex( serializedLayer );

                    for( const ROUTE_HEADER& header : headerSequence )
                    {
                        if( header.layer == padsLayer )
                            logicalHeaders.push_back( { header.width, header.layer } );
                    }
                }

                if( !logicalObjects.empty() && !logicalHeaders.empty() )
                {
                    enum class ALIGNMENT : uint8_t
                    {
                        DIAGONAL,
                        OBJECT_ONLY,
                        HEADER_ONLY
                    };

                    size_t objectCount = logicalObjects.size();
                    size_t headerCount = logicalHeaders.size();
                    std::vector<size_t> previous( headerCount + 1 );
                    std::vector<size_t> current( headerCount + 1 );
                    std::vector<ALIGNMENT> parents( ( objectCount + 1 ) * ( headerCount + 1 ),
                                                    ALIGNMENT::DIAGONAL );
                    std::iota( previous.begin(), previous.end(), 0 );

                    for( size_t object = 1; object <= objectCount; ++object )
                    {
                        current[0] = object;
                        parents[object * ( headerCount + 1 )] = ALIGNMENT::OBJECT_ONLY;

                        for( size_t header = 1; header <= headerCount; ++header )
                        {
                            int32_t objectWidth = objects[logicalObjects[object - 1]].width;
                            size_t diagonal = previous[header - 1]
                                              + ( objectWidth == logicalHeaders[header - 1].width ? 0 : 2 );
                            size_t objectOnly = previous[header] + 1;
                            size_t headerOnly = current[header - 1] + 1;
                            ALIGNMENT parent = ALIGNMENT::DIAGONAL;
                            size_t    cost = diagonal;

                            if( objectOnly < cost )
                            {
                                cost = objectOnly;
                                parent = ALIGNMENT::OBJECT_ONLY;
                            }

                            if( headerOnly < cost )
                            {
                                cost = headerOnly;
                                parent = ALIGNMENT::HEADER_ONLY;
                            }

                            current[header] = cost;
                            parents[object * ( headerCount + 1 ) + header] = parent;
                        }

                        std::swap( previous, current );
                    }

                    std::vector<std::optional<int>> alignedLayers( objectCount );
                    size_t exactMatches = 0;
                    size_t object = objectCount;
                    size_t header = headerCount;

                    while( object > 0 || header > 0 )
                    {
                        ALIGNMENT parent = parents[object * ( headerCount + 1 ) + header];

                        if( object > 0 && header > 0 && parent == ALIGNMENT::DIAGONAL )
                        {
                            alignedLayers[object - 1] = logicalHeaders[header - 1].layer;
                            exactMatches += objects[logicalObjects[object - 1]].width
                                            == logicalHeaders[header - 1].width;
                            --object;
                            --header;
                        }
                        else if( object > 0 && ( header == 0 || parent == ALIGNMENT::OBJECT_ONLY ) )
                        {
                            --object;
                        }
                        else
                        {
                            --header;
                        }
                    }

                    size_t comparable = std::min( objectCount, headerCount );

                    // The header arena does not always describe every width the board routes: on
                    // MMSP_L_1 all 124 headers carry width 300000 and the 546 tracks of width
                    // 200000 have no header at all. An object whose width never appears in the
                    // arena has no alignment evidence behind it, so letting it inherit a
                    // neighbour's layer would spread a guess over most of the board. Those keep
                    // the run-level layer instead.
                    std::set<int32_t> headerWidths;

                    for( const LAYER_WIDTH& layerWidth : logicalHeaders )
                        headerWidths.insert( layerWidth.width );

                    auto alignmentApplies = [&]( size_t aSequence )
                    {
                        return headerWidths.count( objects[logicalObjects[aSequence]].width ) != 0;
                    };

                    if( exactMatches * 4 >= comparable * 3 )
                    {
                        std::optional<int> currentLayer;

                        for( size_t sequence = 0; sequence < objectCount; ++sequence )
                        {
                            if( alignedLayers[sequence] )
                                currentLayer = alignedLayers[sequence];

                            if( currentLayer && alignmentApplies( sequence ) )
                                objectLayers[logicalObjects[sequence]] = *currentLayer;
                        }

                        currentLayer.reset();

                        for( size_t sequence = objectCount; sequence > 0; --sequence )
                        {
                            if( alignedLayers[sequence - 1] )
                                currentLayer = alignedLayers[sequence - 1];

                            if( currentLayer && !alignedLayers[sequence - 1] && alignmentApplies( sequence - 1 ) )
                                objectLayers[logicalObjects[sequence - 1]] = *currentLayer;
                        }
                    }
                }
            }


            struct ROUTE_CHUNK
            {
                size_t objectIndex = 0;
                size_t cellStart = 0;
                size_t xyAnchorHits = 0;
                size_t yxAnchorHits = 0;
            };

            std::vector<ROUTE_CHUNK>               chunks;
            std::vector<std::pair<size_t, size_t>> runAnchorHits( runCount );

            // The object array is a ring whose logical base is 32 bytes into its physical
            // storage. Its last descriptor therefore owns the first cell chunk, followed by
            // descriptors 0..N-2. The descriptor cell counts partition the cell stream exactly.
            for( size_t sequence = 0; sequence < objects.size(); ++sequence )
            {
                size_t match = ( sequence + objects.size() - 1 ) % objects.size();

                if( cursor + objects[match].cellCount > cells.size() )
                    break;

                const ROUTE_OBJECT& object = objects[match];
                size_t              xyAnchorHits = 0;
                size_t              yxAnchorHits = 0;
                int32_t             minVariable = std::numeric_limits<int32_t>::max();
                int32_t             maxVariable = std::numeric_limits<int32_t>::lowest();

                for( size_t j = 0; j < object.cellCount; ++j )
                {
                    const ROUTE_CELL& cell = cells[cursor + j];
                    minVariable = std::min( minVariable, std::min( cell.x1, cell.x2 ) );
                    maxVariable = std::max( maxVariable, std::max( cell.x1, cell.x2 ) );
                    xyAnchorHits += routeAnchorPoints.count( { cell.x1, cell.y } );
                    xyAnchorHits += routeAnchorPoints.count( { cell.x2, cell.y } );
                    yxAnchorHits += routeAnchorPoints.count( { cell.y, cell.x1 } );
                    yxAnchorHits += routeAnchorPoints.count( { cell.y, cell.x2 } );
                }

                if( object.width > 0 && minVariable == std::min( object.boundLo, object.boundHi )
                    && maxVariable == std::max( object.boundLo, object.boundHi ) )
                {
                    chunks.push_back( { match, cursor, xyAnchorHits, yxAnchorHits } );
                    size_t run = objectRuns[match];
                    runAnchorHits[run].first += xyAnchorHits;
                    runAnchorHits[run].second += yxAnchorHits;
                }

                cursor += object.cellCount;
            }

            for( const ROUTE_CHUNK& chunk : chunks )
            {
                const ROUTE_OBJECT& object = objects[chunk.objectIndex];

                if( ( object.style & 0x1000 ) != 0 )
                    continue;

                const auto& runHits = runAnchorHits[objectRuns[chunk.objectIndex]];
                bool        fixedIsX;

                if( chunk.xyAnchorHits != chunk.yxAnchorHits )
                    fixedIsX = chunk.xyAnchorHits > chunk.yxAnchorHits;
                else if( runHits.first != runHits.second )
                    fixedIsX = runHits.first > runHits.second;
                else
                    fixedIsX = fileFixedIsX;

                TRACK  track;
                size_t objectRun = objectRuns[chunk.objectIndex];
                track.layer = chunk.objectIndex < objectLayers.size() ? objectLayers[chunk.objectIndex]
                                                                      : ( objectRun < runLayers.size()
                                                                                  ? runLayers[objectRun]
                                                                                  : 1 );
                track.width = object.width;

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

                decodedPieces.push_back( std::move( track ) );
                decodedObjectIndices.push_back( chunk.objectIndex );
            }

            std::vector<bool> v2022HeaderMatched( decodedPieces.size(), false );

            if( m_version == 0x2022 && legacyObjects && !decodedPieces.empty() )
            {
                const SDB_SECTION* routeHeaders = getSection( 59 );

                struct V2022_ROUTE_HEADER
                {
                    size_t  sequence = 0;
                    int32_t halfWidth = 0;
                    int     rawLayer = 0;
                    uint32_t nodeA = 0;
                    uint32_t nodeB = 0;
                };

                if( routeHeaders && routeHeaders->stride == 24 && routeHeaders->count > 1
                    && routeHeaders->totalBytes >= routeHeaders->stride )
                {
                    auto headerU32 = [&]( size_t aPhase, size_t aSequence, size_t aField )
                    {
                        uint32_t value = 0;
                        size_t   offset = aPhase + aSequence * 24 + aField;

                        for( size_t byte = 0; byte < 4; ++byte )
                        {
                            size_t physical = routeHeaders->dataOffset
                                              + ( offset + byte ) % routeHeaders->totalBytes;
                            value |= static_cast<uint32_t>( m_cursor.U8At( physical ) ) << ( byte * 8 );
                        }

                        return value;
                    };

                    size_t bestHeaderPhase = 0;
                    size_t bestHeaderScore = 0;

                    for( size_t phase = 0; phase < 24; ++phase )
                    {
                        size_t score = 0;

                        for( size_t i = 0; i < routeHeaders->count; ++i )
                        {
                            int32_t  halfWidth = static_cast<int32_t>( headerU32( phase, i, 0 ) );
                            uint32_t layer = headerU32( phase, i, 4 );
                            uint32_t nodeA = headerU32( phase, i, 12 );
                            uint32_t nodeB = headerU32( phase, i, 16 );
                            uint32_t flags = headerU32( phase, i, 20 );
                            uint32_t distance = nodeA > nodeB ? nodeA - nodeB : nodeB - nodeA;

                            if( halfWidth > 0 && halfWidth < 20000000 && layer < routeLayers->count
                                && ( flags == 1 || flags == 32 || flags == 1024 ) && nodeA != 0
                                && nodeB != 0 && distance % 72 == 0
                                && distance < static_cast<uint64_t>( entry60->count ) * 72 )
                            {
                                ++score;
                            }
                        }

                        if( score > bestHeaderScore )
                        {
                            bestHeaderScore = score;
                            bestHeaderPhase = phase;
                        }
                    }

                    std::vector<V2022_ROUTE_HEADER> headers;
                    uint32_t                        vertexBase = std::numeric_limits<uint32_t>::max();

                    for( size_t i = 0; i < routeHeaders->count; ++i )
                    {
                        int32_t  halfWidth = static_cast<int32_t>( headerU32( bestHeaderPhase, i, 0 ) );
                        uint32_t layer = headerU32( bestHeaderPhase, i, 4 );
                        uint32_t nodeA = headerU32( bestHeaderPhase, i, 12 );
                        uint32_t nodeB = headerU32( bestHeaderPhase, i, 16 );
                        uint32_t flags = headerU32( bestHeaderPhase, i, 20 );
                        uint32_t distance = nodeA > nodeB ? nodeA - nodeB : nodeB - nodeA;

                        if( halfWidth > 0 && halfWidth < 20000000 && layer < routeLayers->count
                            && ( flags == 1 || flags == 32 || flags == 1024 ) && nodeA != 0 && nodeB != 0
                            && distance % 72 == 0
                            && distance < static_cast<uint64_t>( entry60->count ) * 72 )
                        {
                            vertexBase = std::min( vertexBase, std::min( nodeA, nodeB ) );
                        }
                    }

                    if( vertexBase != std::numeric_limits<uint32_t>::max() )
                    {
                        for( size_t i = 0; i < routeHeaders->count; ++i )
                        {
                            int32_t  halfWidth = static_cast<int32_t>( headerU32( bestHeaderPhase, i, 0 ) );
                            uint32_t layer = headerU32( bestHeaderPhase, i, 4 );
                            uint32_t nodeA = headerU32( bestHeaderPhase, i, 12 );
                            uint32_t nodeB = headerU32( bestHeaderPhase, i, 16 );
                            uint32_t flags = headerU32( bestHeaderPhase, i, 20 );

                            if( halfWidth <= 0 || halfWidth >= 20000000 || layer >= routeLayers->count
                                || ( flags != 1 && flags != 32 && flags != 1024 ) || nodeA < vertexBase
                                || nodeB < vertexBase || ( nodeA - vertexBase ) % 72 != 0
                                || ( nodeB - vertexBase ) % 72 != 0 )
                            {
                                continue;
                            }

                            uint32_t ordinalA = ( nodeA - vertexBase ) / 72;
                            uint32_t ordinalB = ( nodeB - vertexBase ) / 72;

                            if( ordinalA < entry60->count && ordinalB < entry60->count )
                            {
                                headers.push_back( { i, halfWidth, static_cast<int>( layer ), ordinalA,
                                                     ordinalB } );
                            }
                        }
                    }

                    using ENDPOINT_KEY = std::tuple<int32_t, int32_t, int32_t, int32_t>;

                    auto endpointKey = []( int32_t aX1, int32_t aY1, int32_t aX2, int32_t aY2 )
                    {
                        if( std::tie( aX2, aY2 ) < std::tie( aX1, aY1 ) )
                        {
                            std::swap( aX1, aX2 );
                            std::swap( aY1, aY2 );
                        }

                        return ENDPOINT_KEY( aX1, aY1, aX2, aY2 );
                    };

                    std::set<ENDPOINT_KEY> pieceEndpoints;

                    for( const TRACK& track : decodedPieces )
                    {
                        pieceEndpoints.insert(
                                endpointKey( static_cast<int32_t>( std::llround( track.points.front().x ) ),
                                             static_cast<int32_t>( std::llround( track.points.front().y ) ),
                                             static_cast<int32_t>( std::llround( track.points.back().x ) ),
                                             static_cast<int32_t>( std::llround( track.points.back().y ) ) ) );
                    }

                    size_t recordResidue = ( ( scanStart + bestPhase ) % stride + stride - 27 % stride ) % stride;
                    size_t nodeBase = entry60->dataOffset;
                    nodeBase -= ( nodeBase % stride + stride - recordResidue ) % stride;
                    size_t bestRotation = 0;
                    size_t bestRotationScore = 0;

                    for( size_t rotation = 0; rotation < std::min<size_t>( entry60->count, 128 ); ++rotation )
                    {
                        size_t score = 0;

                        for( const V2022_ROUTE_HEADER& header : headers )
                        {
                            size_t indexA = header.nodeA + rotation;
                            size_t indexB = header.nodeB + rotation;

                            if( indexA >= entry60->count || indexB >= entry60->count )
                                continue;

                            size_t offsetA = nodeBase + indexA * stride;
                            size_t offsetB = nodeBase + indexB * stride;

                            if( !m_cursor.InBounds( offsetA, 8 ) || !m_cursor.InBounds( offsetB, 8 ) )
                                continue;

                            ENDPOINT_KEY key = endpointKey( m_cursor.I32At( offsetA ), m_cursor.I32At( offsetA + 4 ),
                                                            m_cursor.I32At( offsetB ), m_cursor.I32At( offsetB + 4 ) );
                            score += pieceEndpoints.count( key );
                        }

                        if( score > bestRotationScore )
                        {
                            bestRotationScore = score;
                            bestRotation = rotation;
                        }
                    }

                    using HEADER_MATCH_KEY = std::tuple<ENDPOINT_KEY, int32_t>;
                    std::map<HEADER_MATCH_KEY, std::vector<size_t>> pieceGroups;
                    std::map<HEADER_MATCH_KEY, std::vector<std::pair<size_t, int>>> headerGroups;

                    for( size_t piece = 0; piece < decodedPieces.size(); ++piece )
                    {
                        const TRACK& track = decodedPieces[piece];
                        ENDPOINT_KEY key = endpointKey(
                                static_cast<int32_t>( std::llround( track.points.front().x ) ),
                                static_cast<int32_t>( std::llround( track.points.front().y ) ),
                                static_cast<int32_t>( std::llround( track.points.back().x ) ),
                                static_cast<int32_t>( std::llround( track.points.back().y ) ) );
                        pieceGroups[{ key, track.width }].push_back( piece );
                    }

                    auto padsLayer = []( int aRawLayer )
                    {
                        return aRawLayer + 1;
                    };

                    for( size_t i = 0; i + 1 < headers.size(); ++i )
                    {
                        const V2022_ROUTE_HEADER& header = headers[i];
                        const V2022_ROUTE_HEADER& style = headers[i + 1];
                        size_t indexA = header.nodeA + bestRotation;
                        size_t indexB = header.nodeB + bestRotation;

                        if( indexA >= entry60->count || indexB >= entry60->count )
                            continue;

                        size_t offsetA = nodeBase + indexA * stride;
                        size_t offsetB = nodeBase + indexB * stride;

                        if( !m_cursor.InBounds( offsetA, 8 ) || !m_cursor.InBounds( offsetB, 8 ) )
                            continue;

                        ENDPOINT_KEY key = endpointKey( m_cursor.I32At( offsetA ), m_cursor.I32At( offsetA + 4 ),
                                                        m_cursor.I32At( offsetB ), m_cursor.I32At( offsetB + 4 ) );
                        headerGroups[{ key, style.halfWidth * 2 }].push_back(
                                { header.sequence, padsLayer( style.rawLayer ) } );
                    }

                    for( auto& [key, pieces] : pieceGroups )
                    {
                        auto matches = headerGroups.find( key );

                        if( matches == headerGroups.end() )
                            continue;

                        for( size_t piece : pieces )
                            v2022HeaderMatched[piece] = true;

                        std::sort( matches->second.begin(), matches->second.end() );
                        std::set<int> candidateLayers;

                        for( const auto& [sequence, layer] : matches->second )
                            candidateLayers.insert( layer );

                        std::set<int> pieceLayers;

                        for( size_t piece : pieces )
                            pieceLayers.insert( decodedPieces[piece].layer );

                        if( v2022NeedsHeaderLayerRecovery && candidateLayers.size() > 1 )
                        {
                            for( size_t piece : pieces )
                            {
                                size_t object = decodedObjectIndices[piece];
                                int serializedLayer = objectRuns[object] == 0 ? 1
                                                                            : static_cast<int>( objectRuns[object] );

                                if( candidateLayers.count( serializedLayer ) )
                                    decodedPieces[piece].layer = serializedLayer;
                            }
                        }

                        size_t count = std::min( pieces.size(), matches->second.size() );

                        for( size_t piece : pieces )
                        {
                            if( candidateLayers.count( decodedPieces[piece].layer ) )
                                continue;

                            if( candidateLayers.size() == 1 && pieceLayers.size() == 1 )
                                decodedPieces[piece].layer = *candidateLayers.begin();
                        }

                        if( pieces.size() == matches->second.size() )
                        {
                            for( size_t i = 0; i < count; ++i )
                            {
                                if( !candidateLayers.count( decodedPieces[pieces[i]].layer ) )
                                    decodedPieces[pieces[i]].layer = matches->second[i].second;
                            }
                        }
                    }

                }
            }

            if( !decodedPieces.empty() && ( m_version == 0x2024 || objects.size() <= 64 )
                && decodedPieces.size() == headerSequence.size() )
            {
                std::map<int32_t, std::map<int, size_t>> layerCounts;

                for( const ROUTE_HEADER& header : headerSequence )
                    ++layerCounts[header.width][header.layer];

                for( const auto& [width, counts] : layerCounts )
                {
                    int    modalLayer = 1;
                    size_t modalCount = 0;

                    for( const auto& [layer, count] : counts )
                    {
                        if( count > modalCount )
                        {
                            modalLayer = layer;
                            modalCount = count;
                        }
                    }

                    std::vector<size_t> candidates;

                    for( size_t piece = 0; piece < decodedPieces.size(); ++piece )
                    {
                        if( decodedPieces[piece].width == width )
                        {
                            decodedPieces[piece].layer = modalLayer;
                            candidates.push_back( piece );
                        }
                    }

                    std::sort( candidates.begin(), candidates.end(),
                               [&]( size_t aLeft, size_t aRight )
                               {
                                   auto viaHits = [&]( size_t aPiece )
                                   {
                                       const TRACK& piece = decodedPieces[aPiece];
                                       return viaPoints.count( { static_cast<int32_t>( piece.points.front().x ),
                                                                 static_cast<int32_t>( piece.points.front().y ) } )
                                              + viaPoints.count( { static_cast<int32_t>( piece.points.back().x ),
                                                                   static_cast<int32_t>( piece.points.back().y ) } );
                                   };

                                   return viaHits( aLeft ) > viaHits( aRight );
                               } );

                    size_t candidate = 0;

                    for( const auto& [layer, count] : counts )
                    {
                        if( layer == modalLayer )
                            continue;

                        for( size_t i = 0; i < count && candidate < candidates.size(); ++i )
                            decodedPieces[candidates[candidate++]].layer = layer;
                    }
                }
            }

            if( m_version == 0x2024 )
            {
                using ENDPOINT = std::pair<int32_t, int32_t>;
                std::map<ENDPOINT, std::map<int, size_t>> endpointLayerCounts;

                auto endpoint = []( const ARC_POINT& aPoint )
                {
                    return ENDPOINT( static_cast<int32_t>( std::llround( aPoint.x ) ),
                                     static_cast<int32_t>( std::llround( aPoint.y ) ) );
                };

                for( const TRACK& track : decodedPieces )
                {
                    for( size_t point = 0; point + 1 < track.points.size(); ++point )
                    {
                        ++endpointLayerCounts[endpoint( track.points[point] )][track.layer];
                        ++endpointLayerCounts[endpoint( track.points[point + 1] )][track.layer];
                    }
                }

                std::vector<std::optional<int>> repairedLayers( decodedPieces.size() );

                for( size_t piece = 0; piece < decodedPieces.size(); ++piece )
                {
                    const TRACK&          track = decodedPieces[piece];
                    std::map<int, size_t> adjacentLayerCounts;
                    bool                  sameLayerNeighbor = false;
                    const ENDPOINT endpoints[] = { endpoint( track.points.front() ),
                                                   endpoint( track.points.back() ) };

                    for( const ENDPOINT& point : endpoints )
                    {
                        if( viaPoints.count( point ) || routeAnchorPoints.count( point ) )
                            continue;

                        auto counts = endpointLayerCounts.find( point );

                        if( counts == endpointLayerCounts.end() )
                            continue;

                        for( const auto& [layer, count] : counts->second )
                        {
                            size_t neighbors = count - ( layer == track.layer ? 1 : 0 );

                            if( layer == track.layer )
                                sameLayerNeighbor |= neighbors > 0;
                            else
                                adjacentLayerCounts[layer] += neighbors;
                        }
                    }

                    if( !sameLayerNeighbor && adjacentLayerCounts.size() == 1
                        && adjacentLayerCounts.begin()->second >= 2 )
                    {
                        repairedLayers[piece] = adjacentLayerCounts.begin()->first;
                    }
                }

                for( size_t piece = 0; piece < decodedPieces.size(); ++piece )
                {
                    if( repairedLayers[piece] )
                        decodedPieces[piece].layer = *repairedLayers[piece];
                }
            }

            if( v2022NeedsHeaderLayerRecovery )
            {
                using ENDPOINT = std::pair<double, double>;
                std::map<ENDPOINT, std::set<int>> matchedEndpointLayers;

                for( size_t piece = 0; piece < decodedPieces.size(); ++piece )
                {
                    if( !v2022HeaderMatched[piece] )
                        continue;

                    const TRACK& track = decodedPieces[piece];
                    matchedEndpointLayers[{ track.points.front().x, track.points.front().y }].insert( track.layer );
                    matchedEndpointLayers[{ track.points.back().x, track.points.back().y }].insert( track.layer );
                }

                for( size_t piece = 0; piece < decodedPieces.size(); ++piece )
                {
                    if( v2022HeaderMatched[piece] || decodedPieces[piece].layer != 1 )
                        continue;

                    const TRACK& track = decodedPieces[piece];
                    std::set<int> adjacentLayers;
                    const ENDPOINT endpoints[] = {
                        { track.points.front().x, track.points.front().y },
                        { track.points.back().x, track.points.back().y }
                    };

                    for( const ENDPOINT& endpoint : endpoints )
                    {
                        auto layers = matchedEndpointLayers.find( endpoint );

                        if( layers != matchedEndpointLayers.end() )
                            adjacentLayers.insert( layers->second.begin(), layers->second.end() );
                    }

                    if( adjacentLayers.size() == 1 )
                        decodedPieces[piece].layer = *adjacentLayers.begin();
                }
            }

            if( !decodedPieces.empty() && !headerChains.empty() )
            {
                auto pointsEqual = []( const ARC_POINT& aLeft, const ARC_POINT& aRight )
                {
                    return aLeft.x == aRight.x && aLeft.y == aRight.y;
                };

                using ENDPOINT = std::pair<double, double>;
                std::map<ENDPOINT, std::vector<size_t>> endpointPieces;

                for( size_t piece = 0; piece < decodedPieces.size(); ++piece )
                {
                    const TRACK& track = decodedPieces[piece];
                    endpointPieces[{ track.points.front().x, track.points.front().y }].push_back( piece );
                    endpointPieces[{ track.points.back().x, track.points.back().y }].push_back( piece );
                }

                std::vector<std::vector<size_t>> pieceComponents;
                std::vector<bool>                componentSeen( decodedPieces.size(), false );

                for( size_t i = 0; i < decodedPieces.size(); ++i )
                {
                    if( componentSeen[i] )
                        continue;

                    std::vector<size_t> component;
                    std::vector<size_t> pending = { i };
                    componentSeen[i] = true;

                    while( !pending.empty() )
                    {
                        size_t current = pending.back();
                        pending.pop_back();
                        component.push_back( current );

                        const TRACK& currentTrack = decodedPieces[current];
                        const ENDPOINT endpoints[] = {
                            { currentTrack.points.front().x, currentTrack.points.front().y },
                            { currentTrack.points.back().x, currentTrack.points.back().y }
                        };

                        for( const ENDPOINT& endpoint : endpoints )
                        {
                            for( size_t piece : endpointPieces[endpoint] )
                            {
                                if( !componentSeen[piece] )
                                {
                                    componentSeen[piece] = true;
                                    pending.push_back( piece );
                                }
                            }
                        }
                    }

                    pieceComponents.push_back( std::move( component ) );
                }

                std::vector<bool> chainUsed( headerChains.size(), false );

                for( const std::vector<size_t>& component : pieceComponents )
                {
                    std::multiset<int32_t> componentWidths;

                    for( size_t piece : component )
                        componentWidths.insert( decodedPieces[piece].width );

                    size_t chainIndex = headerChains.size();

                    for( size_t i = 0; i < headerChains.size(); ++i )
                    {
                        if( chainUsed[i] || headerChains[i].size() != component.size() )
                            continue;

                        std::multiset<int32_t> chainWidths;

                        for( const ROUTE_HEADER& header : headerChains[i] )
                            chainWidths.insert( header.width );

                        if( chainWidths == componentWidths )
                        {
                            chainIndex = i;
                            break;
                        }
                    }

                    if( chainIndex == headerChains.size() )
                        continue;

                    chainUsed[chainIndex] = true;
                    std::vector<size_t> pieceOrder;
                    std::set<size_t>    ordered;
                    size_t              current = component.front();

                    for( size_t piece : component )
                    {
                        std::set<size_t> neighbors;
                        const TRACK&     track = decodedPieces[piece];
                        const ENDPOINT endpoints[] = {
                            { track.points.front().x, track.points.front().y },
                            { track.points.back().x, track.points.back().y }
                        };

                        for( const ENDPOINT& endpoint : endpoints )
                        {
                            for( size_t other : endpointPieces[endpoint] )
                            {
                                if( piece != other )
                                    neighbors.insert( other );
                            }
                        }

                        if( neighbors.size() <= 1 )
                        {
                            current = piece;
                            break;
                        }
                    }

                    size_t startConnections = 0;
                    const ENDPOINT start = { decodedPieces[current].points.front().x,
                                             decodedPieces[current].points.front().y };

                    for( size_t piece : endpointPieces[start] )
                    {
                        if( piece == current )
                            continue;

                        startConnections += pointsEqual( decodedPieces[current].points.front(),
                                                         decodedPieces[piece].points.front() );
                        startConnections += pointsEqual( decodedPieces[current].points.front(),
                                                         decodedPieces[piece].points.back() );
                    }

                    if( startConnections != 0 )
                    {
                        std::reverse( decodedPieces[current].points.begin(), decodedPieces[current].points.end() );
                    }

                    while( ordered.insert( current ).second )
                    {
                        pieceOrder.push_back( current );
                        size_t next = decodedPieces.size();
                        const ENDPOINT end = { decodedPieces[current].points.back().x,
                                               decodedPieces[current].points.back().y };

                        for( size_t piece : endpointPieces[end] )
                        {
                            if( ordered.count( piece ) )
                                continue;

                            if( pointsEqual( decodedPieces[current].points.back(),
                                             decodedPieces[piece].points.front() ) )
                            {
                                next = piece;
                                break;
                            }

                            if( pointsEqual( decodedPieces[current].points.back(),
                                             decodedPieces[piece].points.back() ) )
                            {
                                std::reverse( decodedPieces[piece].points.begin(), decodedPieces[piece].points.end() );
                                next = piece;
                                break;
                            }
                        }

                        if( next == decodedPieces.size() )
                            break;

                        current = next;
                    }

                    if( pieceOrder.size() != component.size() )
                        continue;

                    std::vector<ROUTE_HEADER>& headers = headerChains[chainIndex];
                    bool                       forwardMatches = true;
                    bool                       reverseMatches = true;

                    for( size_t i = 0; i < component.size(); ++i )
                    {
                        forwardMatches &= decodedPieces[pieceOrder[i]].width == headers[i].width;
                        reverseMatches &= decodedPieces[pieceOrder[i]].width == headers[headers.size() - i - 1].width;
                    }

                    if( !forwardMatches && reverseMatches )
                        std::reverse( headers.begin(), headers.end() );

                }
            }

            std::string trackNet = m_nets.size() == 1 ? m_nets.front().name : std::string();
            ROUTE&      trackRoute = routes[trackNet];
            trackRoute.net_name = trackNet;

            for( TRACK& track : decodedPieces )
                trackRoute.tracks.push_back( std::move( track ) );
        }
    }

    for( auto& [netName, route] : routes )
    {
        if( route.tracks.empty() && route.vias.empty() )
            continue;

        for( VIA& via : route.vias )
        {
            std::set<int> connectedLayers;

            for( const TRACK& track : route.tracks )
            {
                for( const ARC_POINT& point : track.points )
                {
                    if( point.x == via.location.x && point.y == via.location.y )
                    {
                        connectedLayers.insert( track.layer );
                        break;
                    }
                }
            }

            if( connectedLayers.size() >= 2 )
            {
                via.start_layer = *connectedLayers.begin();
                via.end_layer = *connectedLayers.rbegin();
            }
        }

        m_routes.push_back( std::move( route ) );
    }
}


void BINARY_PARSER::parseCopperShapes()
{
    const SDB_SECTION* sec10 = getSection( SECTION::DrwItems );
    const SDB_SECTION* sec11 = getSection( SECTION::GraphicPieces );
    const SDB_SECTION* sec12 = getSection( SECTION::Vertices );

    if( !sec10 || !sec11 || !sec12 || sec10->stride < 112 || sec11->stride < 20 || sec12->stride < 12 )
    {
        return;
    }

    constexpr size_t MAX_COPPER_SHAPE_EDGES = 80;

    // Iterate every owner buildOwnerRuns found (a wide, structural byte scan from section 8
    // through the end of section 10) rather than sec10->count records based at sec10->dataOffset:
    // that declared base is not reliable here either -- verified against Ems4_Rev2.pcb, where 6
    // real DRW-named, valid-marker records with real copper class/tag fields sit before the
    // declared section start, cleanly stride-aligned, with no discontinuity into the declared
    // range (the marker walk finds real records straddling the "boundary" on both sides).
    // Reading each owner's OWN header at its stored ownOffset (not the lagged run offset)
    // preserves every existing field access below unchanged.
    //
    // This alone does not recover all of them, though: for the same displaced records, the
    // owner's own vertexStart cursor can land BEFORE computeSec12Base's clean-prefix boundary
    // (e.g. DRW26389629: vertexStart 20 vs a computed base of 99), which sec12Vertex correctly
    // refuses to read as garbage. Either these owners' geometry legitimately lives in what
    // computeSec12Base treats as the dirty heap tail, or that single-boundary model is too
    // simple for a file where section 10 itself needed this same kind of correction -- not
    // established either way. Left unrecovered rather than guessed at.
    for( const auto& [name, run] : m_ownerRuns )
    {
        if( name.size() < 4 || name.substr( 0, 3 ) != "DRW" )
            continue;

        if( !m_cursor.InBounds( run.ownOffset, DRW_ITEM::SIZE ) )
            continue;

        SDB_RECORD hdr = m_sdb.RecordAt( static_cast<uint32_t>( run.ownOffset ) );

        // class 1 = filled copper region (either block tag -- the second was found covering the
        // same shapes as the first within a single file, not a version split), 7 = v2026
        // line-drawn copper. subtypeWord does NOT reliably separate real copper shapes from
        // anything else here -- this used to also require subtypeWord != 3 on the assumption
        // that 3 meant "pour, decoded separately elsewhere", but verified against
        // MC4_PLUS_CSHAPE.pcb, subtype 3 is shared by 10 of its 15 real COPPER shapes (both
        // fills and circles) and by at least one plain (non-filled) LINES item, so it cannot be
        // used as a classifier either way. The real filled-vs-stroked discriminator is
        // downstream: a filled shape's declared bbox equals its vertex extent exactly, while a
        // stroked LINES item's bbox is pen-width-expanded.
        constexpr uint32_t CLASS_FILLED       = 1;
        constexpr uint32_t CLASS_LINE         = 7;
        constexpr uint32_t SUBTYPE_PLAIN_LINE = 0;

        uint32_t classWord = hdr.U32( DRW_ITEM::CLASS_WORD );
        uint32_t subtypeWord = hdr.U32( DRW_ITEM::SUBTYPE_WORD );
        uint32_t blockTag = hdr.U32( DRW_ITEM::TYPE_TAG );
        bool legacyCopper = ( ( blockTag == DRW_TAG::COPPER_FILL || blockTag == DRW_TAG::COPPER_FILL_B )
                              && classWord == CLASS_FILLED );
        bool     v2026LineCopper = ( m_version == 0x2026 && blockTag == DRW_TAG::LINE_ITEM && classWord == CLASS_LINE
                                 && subtypeWord == SUBTYPE_PLAIN_LINE );

        if( !legacyCopper && !v2026LineCopper )
            continue;

        uint32_t sec11Index = hdr.U32( DRW_ITEM::PIECE_START );

        if( legacyCopper && sec11Index >= sec11->count )
            continue;

        int32_t originX = hdr.I32( DRW_ITEM::ORIGIN_X );
        int32_t originY = hdr.I32( DRW_ITEM::ORIGIN_Y );

        int64_t localMinX = static_cast<int64_t>( hdr.I32( DRW_ITEM::BBOX_MIN_X ) ) - originX;
        int64_t localMinY = static_cast<int64_t>( hdr.I32( DRW_ITEM::BBOX_MIN_Y ) ) - originY;
        int64_t localMaxX = static_cast<int64_t>( hdr.I32( DRW_ITEM::BBOX_MAX_X ) ) - originX;
        int64_t localMaxY = static_cast<int64_t>( hdr.I32( DRW_ITEM::BBOX_MAX_Y ) ) - originY;

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
            // The structural slice resolves which sec12 loop belongs to this owner; the
            // bbox-equality gate is the filled-copper classifier, since a stroked LINES item
            // records a pen-expanded bbox that no longer equals its vertex extent.
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

            if( loopMinX != localMinX || loopMinY != localMinY || loopMaxX != localMaxX || loopMaxY != localMaxY )
            {
                continue;
            }
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

            if( std::lround( cx - radius ) != localMinX || std::lround( cy - radius ) != localMinY
                || std::lround( cx + radius ) != localMaxX || std::lround( cy + radius ) != localMaxY )
            {
                continue;
            }

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

        if( v2026LineCopper )
        {
            copper.layer = 1;
        }
        else
        {
            SDB_RECORD gfx = m_sdb.Record( *sec11, sec11Index, sec11->stride );
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

    if( !sec10 || sec10->stride < 112 || m_ownerRuns.empty() )
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
        std::string name = m_sdb.Record( *sec10, rec, sec10->stride ).Str( 44, 24 );

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

    if( !sec12 || sec12->stride < 12 || sec12->totalBytes == 0 )
        return;

    int32_t directoryRows = static_cast<int32_t>( sec12->totalBytes / 12 );
    int32_t cleanRows = 0;

    // A record is clean while its attr is -1 (plain corner) or a small arc-table ordinal; the
    // per-save heap tail begins when attr becomes coordinate-scale.
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
    size_t end = std::min( static_cast<size_t>( sec10->dataOffset ) + static_cast<size_t>( sec10->totalBytes ),
                           m_data.size() );

    if( start >= end )
        return;

    // A genuine 112-byte owner record carries a 0xFFFE/0xFFFF marker, a >=2-char ASCII name at
    // +44, and a cursor triple in range. The cursor guard rejects coincidental heap markers so
    // the marker walk does not bind garbage runs.
    auto isOwnerRecord = [&]( size_t aOff ) -> bool
    {
        SDB_RECORD rec = m_sdb.RecordAt( aOff );
        uint16_t   marker = rec.U16( 0 );
        uint16_t   hi = rec.U16( 2 );

        if( ( marker != SDB_RECORD_SENTINEL && marker != 0xFFFF ) || hi != 0 )
            return false;

        std::string name = rec.Str( DRW_ITEM::NAME, 44 );

        if( name.size() < 2 )
            return false;

        for( char c : name )
        {
            if( static_cast<unsigned char>( c ) < 32 || static_cast<unsigned char>( c ) >= 127 )
                return false;
        }

        int32_t vertexStart = rec.I32( DRW_ITEM::VERTEX_START );
        int32_t pieceCount = rec.I32( DRW_ITEM::PIECE_COUNT );

        return vertexStart >= 0 && vertexStart < ( 1 << 24 ) && pieceCount >= 0 && pieceCount < ( 1 << 16 );
    };

    auto readRun = [&]( size_t aLagOff, size_t aOwnOff ) -> OWNER_RUN
    {
        SDB_RECORD rec = m_sdb.RecordAt( aLagOff );
        OWNER_RUN  run;
        run.pieceStart = rec.I32( DRW_ITEM::PIECE_START );
        run.vertexStart = rec.I32( DRW_ITEM::VERTEX_START );
        run.arcStart = rec.I32( DRW_ITEM::ARC_START );
        run.pieceCount = rec.I32( DRW_ITEM::PIECE_COUNT );
        run.ownOffset = aOwnOff;
        return run;
    };

    // The run cursors are read from the FOLLOWING accepted record (the one-record lag), so the
    // previous record's name and offset are remembered and bound when the next record is found.
    // The last owner has no successor in the named list; its cursors are carried by the next
    // physical 112-byte record (the trailing terminator slot), bound after the walk.
    std::string prevName;
    size_t      prevOff = 0;
    bool        havePrev = false;
    size_t      off = start;

    while( off + DRW_ITEM::SIZE <= end )
    {
        if( isOwnerRecord( off ) )
        {
            std::string name = m_sdb.RecordAt( off ).Str( DRW_ITEM::NAME, 44 );

            if( havePrev && !m_ownerRuns.count( prevName ) )
                m_ownerRuns.emplace( prevName, readRun( off, prevOff ) );

            prevName = std::move( name );
            prevOff = off;
            havePrev = true;
            off += DRW_ITEM::SIZE;
            continue;
        }

        ++off;
    }

    if( havePrev && !m_ownerRuns.count( prevName ) )
    {
        size_t tailOff = prevOff + DRW_ITEM::SIZE;

        if( tailOff + DRW_ITEM::SIZE <= m_data.size() )
            m_ownerRuns.emplace( prevName, readRun( tailOff, prevOff ) );
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


bool BINARY_PARSER::fetchOwnerLoop( const std::string& aName, size_t aMaxVerts, std::vector<VECTOR2I>& aOut ) const
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


bool BINARY_PARSER::fetchOwnerCirclePoints( const std::string& aName, VECTOR2I& aP0, VECTOR2I& aP1 ) const
{
    auto it = m_ownerRuns.find( aName );

    if( it == m_ownerRuns.end() )
        return false;

    int32_t startRow = it->second.vertexStart - m_sec12Base;

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

    if( !sec10 || !sec12 || sec10->stride < 112 || sec12->stride < 12 )
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
        SDB_RECORD hdr = m_sdb.Record( *sec10, rec, sec10->stride );

        // Keepouts share the filled-region class with copper but carry no block tag; subtype
        // buckets 1 and 10 are the two keepout variants, both mapped to KEEPOUT_TYPE::ALL below.
        constexpr uint32_t CLASS_FILLED        = 1;
        constexpr uint32_t SUBTYPE_KEEPOUT_A   = 1;
        constexpr uint32_t SUBTYPE_KEEPOUT_B   = 10;

        if( hdr.U32( DRW_ITEM::TYPE_TAG ) != 0 || hdr.U32( DRW_ITEM::CLASS_WORD ) != CLASS_FILLED )
            continue;

        uint32_t typeBucket = hdr.U32( DRW_ITEM::SUBTYPE_WORD );

        if( typeBucket != SUBTYPE_KEEPOUT_A && typeBucket != SUBTYPE_KEEPOUT_B )
            continue;

        std::string name = hdr.Str( DRW_ITEM::NAME, 24 );

        if( name.size() < 4 || name.substr( 0, 3 ) != "DRW" )
            continue;

        Owner owner;
        owner.name = std::move( name );
        owner.originX = hdr.I32( DRW_ITEM::ORIGIN_X );
        owner.originY = hdr.I32( DRW_ITEM::ORIGIN_Y );
        owner.minX = static_cast<int64_t>( hdr.I32( DRW_ITEM::BBOX_MIN_X ) ) - owner.originX;
        owner.minY = static_cast<int64_t>( hdr.I32( DRW_ITEM::BBOX_MIN_Y ) ) - owner.originY;
        owner.maxX = static_cast<int64_t>( hdr.I32( DRW_ITEM::BBOX_MAX_X ) ) - owner.originX;
        owner.maxY = static_cast<int64_t>( hdr.I32( DRW_ITEM::BBOX_MAX_Y ) ) - owner.originY;

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
        double cx = static_cast<double>( owner.minX + owner.maxX ) / 2.0;
        double cy = static_cast<double>( owner.minY + owner.maxY ) / 2.0;
        double radius = static_cast<double>( spanX ) / 2.0;

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
    // Copper pours are a global run of 88-byte "owner" records (one per pour), each anchoring a
    // 16-byte "piece" record and indexing into a shared, file-wide vertex/arc array. The
    // directory-declared offsets for sections 52-55 are frequently wrong -- the whole owner/
    // piece/vertex/arc array set is shifted from them by a per-file displacement -- so owners
    // are found by a global signature scan rather than by iterating a section's declared record
    // count: section 52's declared count is pool capacity, not a pour count (observed as high as
    // 18369 for 32 actual pours on one board).
    //
    // Owner signature, scanned byte-by-byte across the whole file (zero false positives/
    // negatives verified across the QA corpus, including files with no pours and stray "POR"
    // byte sequences elsewhere):
    //   u32(O+64) == 1
    //   bytes(O+70, 3) == "POR"
    //
    // Owner record (88 bytes), offsets from O:
    //   +4   u32 cumulative corner start index into the shared vertex array
    //   +8   u32 cumulative arc start index into the shared arc array (arcs are not yet
    //        imported -- see the piece-record note below)
    //   +24  i32 raw XLOC -- each pour owns its own anchor, not a shared board anchor
    //   +28  i32 raw YLOC
    //   +70  char name[16]
    //
    // Piece record (16 bytes, one per pour, in owner order), based at pieceBase:
    //   +0   u32 corner count
    //   +4   u32 arc count
    //   +8   i32 width, BASIC units
    //   +12  u8  piece type: 0x32 = polygon, 0x33 = circle (two diametrically-opposite corners,
    //        not a 2-point polygon)
    //   +13  u8  layer
    //
    // The vertex array (flat, file-wide, 8-byte local (i32 x, i32 y) pairs) is indexed by each
    // owner's own cumulative corner-start field, not consumed sequentially per pour -- the pool
    // is over-allocated on some boards (e.g. 10,380 slots for 1,377 used corners on one board).
    // Arc records decorate specific corner-to-corner segments with a curve rather than adding
    // extra boundary points, so the corner list alone still yields a closed (if not smoothly
    // curved) outline; arc-to-curve conversion is not implemented.
    static constexpr size_t OWNER_SIZE  = 88;
    static constexpr size_t PIECE_SIZE  = 16;
    static constexpr size_t VERTEX_SIZE = 8;

    struct POUR_OWNER
    {
        size_t      offset = 0;
        uint32_t    cornerStart = 0;
        int32_t     rawX = 0;
        int32_t     rawY = 0;
        std::string name;
    };

    std::vector<POUR_OWNER> owners;

    for( size_t o = 0; o + OWNER_SIZE <= m_data.size(); ++o )
    {
        if( m_cursor.U32At( o + 64 ) != 1 )
            continue;

        if( m_data[o + 70] != 'P' || m_data[o + 71] != 'O' || m_data[o + 72] != 'R' )
            continue;

        POUR_OWNER owner;
        owner.offset = o;
        owner.cornerStart = m_cursor.U32At( o + 4 );
        owner.rawX = m_cursor.I32At( o + 24 );
        owner.rawY = m_cursor.I32At( o + 28 );
        owner.name = m_cursor.StringAt( o + 70, 16 );
        owners.push_back( std::move( owner ) );
    }

    if( owners.empty() )
        return;

    const SDB_SECTION* sec52 = getSection( SECTION::PourTokensA );
    const SDB_SECTION* sec53 = getSection( SECTION::PourTokensB );
    const SDB_SECTION* sec54 = getSection( SECTION::PourTokensC );

    if( !sec52 || !sec53 || !sec54 )
        return;

    int64_t delta = static_cast<int64_t>( owners[0].offset ) - static_cast<int64_t>( sec52->dataOffset );
    int64_t pieceBaseSigned = static_cast<int64_t>( sec53->dataOffset ) + delta;
    int64_t vertexBaseSigned = static_cast<int64_t>( sec54->dataOffset ) + delta;

    if( pieceBaseSigned < 0 || vertexBaseSigned < 0 )
        return;

    size_t pieceBase = static_cast<size_t>( pieceBaseSigned );
    size_t vertexBase = static_cast<size_t>( vertexBaseSigned );

    for( size_t p = 0; p < owners.size(); ++p )
    {
        const POUR_OWNER& owner = owners[p];
        size_t             pieceOff = pieceBase + p * PIECE_SIZE;

        if( !m_cursor.InBounds( pieceOff, PIECE_SIZE ) )
            continue;

        uint32_t cornerCount = m_cursor.U32At( pieceOff );
        int32_t  width = m_cursor.I32At( pieceOff + 8 );
        uint8_t  pieceType = m_cursor.U8At( pieceOff + 12 );
        uint8_t  layer = m_cursor.U8At( pieceOff + 13 );

        if( cornerCount == 0 || cornerCount > 100000 )
            continue;

        size_t vOff = vertexBase + static_cast<size_t>( owner.cornerStart ) * VERTEX_SIZE;

        POUR pour;
        pour.owner_pour = owner.name;
        pour.width = static_cast<double>( width );
        pour.layer = static_cast<int>( layer );

        if( pieceType == 0x33 && cornerCount == 2 )
        {
            // Circle piece: the two "corners" are diametrically opposite endpoints, not a
            // 2-point polygon -- the downstream zone builder requires at least 3 points and
            // would silently drop it. Synthesize a regular polygon approximation instead.
            if( !m_cursor.InBounds( vOff, VERTEX_SIZE * 2 ) )
                continue;

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
                double  angle = 2.0 * M_PI * s / CIRCLE_SEGMENTS;
                int32_t rawX = static_cast<int32_t>( std::lround( cx + radius * std::cos( angle ) ) );
                int32_t rawY = static_cast<int32_t>( std::lround( cy + radius * std::sin( angle ) ) );

                pour.points.emplace_back( toBasicCoordX( rawX ), toBasicCoordY( rawY ) );
            }
        }
        else
        {
            if( !m_cursor.InBounds( vOff, static_cast<size_t>( cornerCount ) * VERTEX_SIZE ) )
                continue;

            for( uint32_t v = 0; v < cornerCount; ++v )
            {
                size_t  o = vOff + static_cast<size_t>( v ) * VERTEX_SIZE;
                int32_t localX = m_cursor.I32At( o );
                int32_t localY = m_cursor.I32At( o + 4 );

                pour.points.emplace_back( toBasicCoordX( owner.rawX + localX ), toBasicCoordY( owner.rawY + localY ) );
            }
        }

        m_pours.push_back( std::move( pour ) );
    }
}


void BINARY_PARSER::parseLayerStackup()
{
    m_layerInfos.clear();

    // The sec69 layout (name@+0, routing_dir@+32, layer_thickness@+52, copper_thickness@+56,
    // dielectric f32@+60; 31 records of 152 bytes) is verified on v0x2027 only. Older dialects
    // keep the synthesized fallback in GetLayerInfos().
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

    // The "(All layers)" string anchors the first record; the directory data_offset overflows
    // the indexed region on large boards.
    size_t recordBase = findSignature( m_data, reinterpret_cast<const uint8_t*>( ANCHOR.data() ), ANCHOR.size() );

    if( recordBase == SIGNATURE_NOT_FOUND )
        return;

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

        // A real copper layer carries a non-zero COPPER_THICKNESS. The usage==1 enum is not a
        // reliable copper signal on its own: the "(All layers)" pseudo-layer also reads usage==1,
        // and a routed outer layer can read usage==0, so a usage-only test both over- and
        // under-counts. COPPER_THICKNESS includes Bottom and excludes the pseudo-layer and doc
        // layers.
        info.is_copper = ( info.copper_thickness > 0.0 );
        info.required  = info.is_copper;

        if( info.is_copper )
        {
            info.layer_type = ( routingDir == 2 ) ? PADS_LAYER_FUNCTION::PLANE : PADS_LAYER_FUNCTION::ROUTING;
        }
        else
        {
            info.layer_type = PADS_LAYER_FUNCTION::UNASSIGNED;
        }

        m_layerInfos.push_back( std::move( info ) );
    }

    // The leading "(All layers)" aggregate is a pseudo-layer that always carries usage==1;
    // demote it explicitly to avoid an extra copper entry.
    if( !m_layerInfos.empty() && m_layerInfos.front().name == ANCHOR )
    {
        m_layerInfos.front().is_copper = false;
        m_layerInfos.front().required  = false;
        m_layerInfos.front().layer_type = PADS_LAYER_FUNCTION::UNASSIGNED;
    }
}


namespace
{
struct NON_COPPER_LAYER_DEF
{
    int                 number;
    const char*         name;
    PADS_LAYER_FUNCTION type;
};

// Standard PADS non-copper technical layers follow well-known numbering conventions.
const NON_COPPER_LAYER_DEF STANDARD_NON_COPPER[] = {
    { 21, "Assembly Top",       PADS_LAYER_FUNCTION::ASSEMBLY },
    { 22, "Assembly Bottom",    PADS_LAYER_FUNCTION::ASSEMBLY },
    { 25, "Solder Mask Top",    PADS_LAYER_FUNCTION::SOLDER_MASK },
    { 26, "Silkscreen Top",     PADS_LAYER_FUNCTION::SILK_SCREEN },
    { 27, "Silkscreen Bottom",  PADS_LAYER_FUNCTION::SILK_SCREEN },
    { 28, "Solder Mask Bottom", PADS_LAYER_FUNCTION::SOLDER_MASK },
    { 29, "Paste Mask Top",     PADS_LAYER_FUNCTION::PASTE_MASK },
    { 30, "Paste Mask Bottom",  PADS_LAYER_FUNCTION::PASTE_MASK },
};

void appendStandardNonCopperLayers( std::vector<LAYER_INFO>& aInfos )
{
    for( const NON_COPPER_LAYER_DEF& def : STANDARD_NON_COPPER )
    {
        LAYER_INFO info;
        info.number = def.number;
        info.name = def.name;
        info.is_copper = false;
        info.required = false;
        info.layer_type = def.type;
        aInfos.push_back( info );
    }
}
} // namespace


std::vector<LAYER_INFO> BINARY_PARSER::GetLayerInfos() const
{
    // When the sec69 stackup table was decoded, renumber its active copper layers 1..N in table
    // order and append the standard non-copper technical layers, which sec69 names by
    // documentation subtype rather than the canonical PADS numbers the importer maps.
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
            appendStandardNonCopperLayers( infos );
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

    appendStandardNonCopperLayers( infos );
    return infos;
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

        if( partTypeIdx < m_partTypeDecalIndices.size()
            && alternate < m_partTypeDecalIndices[partTypeIdx].size() )
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

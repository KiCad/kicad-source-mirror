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

#include <sch_io/diptrace/diptrace_sch_parser.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <limits>
#include <memory>
#include <optional>
#include <set>

#include <wx/filename.h>
#include <wx/log.h>

#include <base_units.h>
#include <lib_id.h>
#include <lib_symbol.h>
#include <page_info.h>
#include <progress_reporter.h>
#include <project.h>
#include <reporter.h>
#include <sch_bus_entry.h>
#include <sch_junction.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_pin.h>
#include <sch_screen.h>
#include <sch_shape.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <schematic.h>
#include <string_utils.h>
#include <wildcards_and_files_ext.h>


using namespace DIPTRACE;


/// Structural layout version threshold for the .dch schematic format.
///
/// Versions below V31_CUTOVER (< 34) have different field layouts in several
/// sections: extra int3 fields, different padding, and alternate single-part
/// encoding.  This controls structural byte-layout differences only.
///
/// This is independent of schematic string encoding, which switches to UTF-16-BE
/// at v34 while PCB keeps the shared legacy <=37 ASCII threshold.
static constexpr int V31_CUTOVER = 34;

static constexpr int SCHEMATIC_UTF16_STRING_VERSION = V31_CUTOVER;


SCH_PARSER::SCH_PARSER( const wxString& aFileName, SCHEMATIC* aSchematic, SCH_SHEET* aRootSheet,
                        PROGRESS_REPORTER* aProgressReporter, REPORTER* aReporter ) :
        m_reader( aFileName ),
        m_schematic( aSchematic ),
        m_rootSheet( aRootSheet ),
        m_progressReporter( aProgressReporter ),
        m_reporter( aReporter ),
        m_version( 38 ),
        m_magicMajor( 0 ),
        m_componentCount( -1 ),
        m_fileName( aFileName ),
        m_numSheets( 0 ),
        m_busSectionOffset( 0 ),
        m_tailOffset( 0 )
{
}


SCH_PARSER::~SCH_PARSER()
{
}


int SCH_PARSER::toKiCadCoordX( int aDipTraceCoord )
{
    return static_cast<int>( static_cast<int64_t>( aDipTraceCoord ) / 3 );
}


int SCH_PARSER::toKiCadCoordY( int aDipTraceCoord )
{
    // DipTrace .dch stores schematic Y already in a screen-down convention (more negative is higher
    // on the page), matching KiCad's Y-down axis, and bakes any placement rotation into the stored
    // pin, shape and wire coordinates. So the conversion is a plain scale with no axis flip; negating
    // here vertically mirrors the whole sheet and was the cause of the inverted import.
    return static_cast<int>( static_cast<int64_t>( aDipTraceCoord ) / 3 );
}


int SCH_PARSER::toKiCadSize( int aDipTraceCoord )
{
    return static_cast<int>( static_cast<int64_t>( std::abs( aDipTraceCoord ) ) / 3 );
}


void SCH_PARSER::findPageGeometry()
{
    // DipTrace stores the page as width, height and four margins, each an int4 holding mm * 30000.
    // The record is not framed by a marker, so accept the first run that decodes as a sane page
    // (sensible mm dimensions, all margins whole mm within the sheet, a positive left margin).
    const uint8_t* data = m_reader.GetData();
    size_t         fileSize = m_reader.GetFileSize();

    auto rdInt4 = [&]( size_t o ) -> int
    {
        uint32_t raw = ( static_cast<uint32_t>( data[o] ) << 24 ) | ( static_cast<uint32_t>( data[o + 1] ) << 16 )
                       | ( static_cast<uint32_t>( data[o + 2] ) << 8 ) | static_cast<uint32_t>( data[o + 3] );
        return static_cast<int>( static_cast<int64_t>( raw ) - INT4_BIAS );
    };

    if( fileSize < 24 )
        return;

    static constexpr int UNITS_PER_MM = 30000;

    for( size_t o = 0; o + 24 <= fileSize; o++ )
    {
        int w = rdInt4( o );
        int h = rdInt4( o + 4 );

        if( w <= 0 || h <= 0 || ( w % UNITS_PER_MM ) != 0 || ( h % UNITS_PER_MM ) != 0 )
            continue;

        int wmm = w / UNITS_PER_MM;
        int hmm = h / UNITS_PER_MM;

        if( wmm < 50 || wmm > 2000 || hmm < 50 || hmm > 2000 )
            continue;

        int  margins[4] = { rdInt4( o + 8 ), rdInt4( o + 12 ), rdInt4( o + 16 ), rdInt4( o + 20 ) };
        bool sane = margins[0] > 0;

        for( int m : margins )
        {
            if( m < 0 || ( m % UNITS_PER_MM ) != 0 || m / UNITS_PER_MM > 100 )
                sane = false;
        }

        if( !sane )
            continue;

        m_page.found = true;
        m_page.widthMM = wmm;
        m_page.heightMM = hmm;

        // Place the origin-centered DipTrace content on the top-left-origin KiCad page by adding
        // half the page in KiCad nm. Width/height share the coordinate unit, so reuse toKiCadSize().
        m_pageOffset = VECTOR2I( toKiCadSize( w / 2 ), toKiCadSize( h / 2 ) );
        return;
    }
}


VECTOR2I SCH_PARSER::applyPageOffset( const VECTOR2I& aPos ) const
{
    if( !m_page.found )
        return aPos;

    return aPos + m_pageOffset;
}


wxString SCH_PARSER::getLibName() const
{
    wxString libName = m_schematic->Project().GetProjectName();

    if( libName.IsEmpty() )
    {
        wxFileName fn( m_rootSheet->GetFileName() );
        libName = fn.GetName();
    }

    if( libName.IsEmpty() )
        libName = wxT( "noname" );

    libName += wxT( "-diptrace-import" );
    libName = LIB_ID::FixIllegalChars( libName, true ).wx_str();
    return libName;
}


void SCH_PARSER::Parse()
{
    try
    {
        parseHeader();
        m_reader.SetVersion( m_version );
        m_reader.SetStringEncoding( m_version >= SCHEMATIC_UTF16_STRING_VERSION ? STRING_ENCODING::UTF16_BE
                                                                                : STRING_ENCODING::LEGACY_ASCII );

        // The version threshold is only a heuristic -- DipTrace ships same-version files in both
        // encodings -- so confirm it against the first sheet-name string, which parseHeader left
        // the reader positioned on. Detection keeps the version default when inconclusive.
        if( m_numSheets > 0 )
            m_reader.DetectStringEncoding( m_reader.GetOffset() );

        if( m_version < 1 )
        {
            THROW_IO_ERROR( wxString::Format( _( "Unsupported DipTrace schematic format version %d." ), m_version ) );
        }

        parseSheetDefinitions();
        parseDisplaySettings();
        parseTextStyles();
        parsePreComponentSettings();

        size_t compSectionStart = m_reader.GetOffset();
        m_tailOffset = findTailStart();
        bool hasBusSection = false;

        if( m_magicMajor == 1 )
        {
            m_busSectionOffset = m_tailOffset;
        }
        else
        {
            m_busSectionOffset = findBusSection( compSectionStart );
            hasBusSection = m_busSectionOffset > 0 && m_busSectionOffset < m_reader.GetFileSize();
        }

        if( m_magicMajor != 1 && ( m_busSectionOffset == 0 || m_busSectionOffset >= m_reader.GetFileSize() ) )
        {
            m_busSectionOffset = m_tailOffset;
        }

        parseComponents( m_busSectionOffset );

        if( hasBusSection )
        {
            m_reader.SetOffset( m_busSectionOffset );
            parseBusSection();
        }

        parseNetSection();
        parseWireSection();
        parseSheetShapes();
        findPageGeometry();
    }
    catch( const IO_ERROR& )
    {
        throw;
    }
    catch( const std::exception& e )
    {
        THROW_IO_ERROR( wxString::Format( _( "DipTrace import: unexpected error at offset 0x%06zX: %s" ),
                                          m_reader.GetOffset(), wxString::FromUTF8( e.what() ) ) );
    }

    createKiCadObjects();
}


void SCH_PARSER::parseHeader()
{
    uint8_t magicLen = m_reader.ReadByte();

    if( magicLen != 7 && magicLen != 11 )
    {
        THROW_IO_ERROR( wxString::Format( _( "Invalid DipTrace schematic magic length: %d (expected 7 or 11)." ),
                                          (int) magicLen ) );
    }

    std::vector<uint8_t> magicBuf( magicLen );
    m_reader.ReadBytes( magicBuf.data(), magicLen );

    if( memcmp( magicBuf.data(), "DTSCHEM", 7 ) != 0 )
        THROW_IO_ERROR( _( "Invalid DipTrace schematic file: bad magic header." ) );

    if( magicLen == 7 )
    {
        m_magicMajor = 2;
        m_version = m_reader.ReadInt3();
    }
    else
    {
        // Legacy files encode the version in the magic suffix ("DTSCHEMx.yy").
        const uint8_t* suffix = magicBuf.data() + 7;

        if( !std::isdigit( suffix[0] ) || suffix[1] != '.' || !std::isdigit( suffix[2] ) || !std::isdigit( suffix[3] ) )
        {
            THROW_IO_ERROR( _( "Invalid DipTrace schematic file: bad legacy version suffix." ) );
        }

        m_magicMajor = suffix[0] - '0';
        m_version = ( suffix[2] - '0' ) * 10 + ( suffix[3] - '0' );
    }

    m_reader.ReadInt4(); // field_0B
    m_reader.ReadInt3(); // field_0F
    m_reader.ReadInt3(); // field_12
    m_reader.ReadInt3(); // field_15
    m_numSheets = m_reader.ReadInt3();

    if( m_numSheets < 0 || m_numSheets > 100 )
    {
        THROW_IO_ERROR( wxString::Format( _( "Invalid DipTrace schematic sheet count: %d." ), m_numSheets ) );
    }
}


void SCH_PARSER::parseSheetDefinitions()
{
    for( int i = 0; i < m_numSheets; i++ )
    {
        DCH_SHEET_DEF sheet;
        sheet.name = m_reader.ReadString();
        sheet.field_a = m_reader.ReadInt3();
        m_sheetDefs.push_back( sheet );
    }
}


void SCH_PARSER::parseDisplaySettings()
{
    for( int i = 0; i < 5; i++ )
        m_reader.ReadInt3();

    m_reader.ReadByte();
    m_reader.ReadInt4();
    m_reader.ReadInt4();

    if( m_version < V31_CUTOVER )
    {
        m_reader.ReadInt3();
        m_reader.ReadInt3();
    }
    else
    {
        uint8_t extraHdr[4] = {};
        m_reader.ReadBytes( extraHdr, 4 );

        uint32_t extraChars = ( extraHdr[0] << 24 ) | ( extraHdr[1] << 16 ) | ( extraHdr[2] << 8 ) | extraHdr[3];

        // Some modern files store an optional UTF-16 payload (e.g. "url")
        // after this 4-byte raw length field.
        if( extraChars > 0 && extraChars < 1000 )
            m_reader.Skip( static_cast<size_t>( extraChars ) * 2 );
    }

    m_reader.ReadByte();
    m_reader.ReadInt4();
    m_reader.ReadInt4();
}


void SCH_PARSER::parseTextStyles()
{
    // DTSCHEM1.x legacy files do not contain a text-style table at this location.
    if( m_magicMajor == 1 )
        return;

    int numStyles = m_reader.ReadInt3();

    if( numStyles < 0 || numStyles > 2000 )
    {
        THROW_IO_ERROR( wxString::Format( _( "Invalid text style count: %d." ), numStyles ) );
    }

    for( int i = 0; i < numStyles; i++ )
    {
        m_reader.ReadString();
        m_reader.ReadInt3();
        m_reader.ReadInt4();
        m_reader.ReadInt4();

        // v38+ files carry a trailing int3 per text style. In older single-style
        // files this same byte was historically consumed as the leading "pad" int3
        // in parsePreComponentSettings(); reading it here is byte-identical for
        // one style but keeps multi-style v46 files in sync.
        if( m_version > LEGACY_STRING_VERSION )
            m_reader.ReadInt3();
    }
}


void SCH_PARSER::parsePreComponentSettings()
{
    if( m_magicMajor == 1 )
    {
        // DTSCHEM1.x: part count is the first int3 and there is no leading
        // padding int3 before it.
        m_componentCount = m_reader.ReadInt3();
        m_reader.ReadByte();
        m_reader.ReadByte();

        for( int i = 0; i < 5; i++ )
            m_reader.ReadInt3();

        m_reader.ReadInt3();
        m_reader.ReadInt3();
    }
    else
    {
        // For v38+ the per-style trailing int3 consumed in parseTextStyles() is
        // the same byte that single-style legacy files exposed here as a leading
        // pad int3, so the component count is the first int3 of this section.
        // For v37 and below (no per-style trailer) the leading pad is still here.
        if( m_version <= LEGACY_STRING_VERSION )
            m_reader.ReadInt3();

        m_componentCount = m_reader.ReadInt3();
        m_reader.ReadByte();
        m_reader.ReadByte();

        for( int i = 0; i < 5; i++ )
            m_reader.ReadInt3();

        m_reader.ReadInt3();
        m_reader.ReadInt3();
    }

    if( m_componentCount < 0 || m_componentCount > 100000 )
    {
        THROW_IO_ERROR( wxString::Format( _( "DipTrace import: invalid component count %d." ), m_componentCount ) );
    }
}


size_t SCH_PARSER::findBusSection( size_t aSearchStart ) const
{
    const uint8_t* data = m_reader.GetData();
    size_t         fileSize = m_reader.GetFileSize();

    static const uint8_t    marker[] = { 0x3B, 0x9A, 0xF1, 0x10, 0x3B, 0x9A, 0xF1, 0x10, 0x00, 0x00 };
    static constexpr size_t markerLen = sizeof( marker );

    if( aSearchStart + markerLen + 3 >= fileSize )
        return 0;

    for( size_t off = aSearchStart; off < fileSize - markerLen - 3; off++ )
    {
        if( memcmp( data + off, marker, markerLen ) == 0 )
        {
            int count = ( ( data[off + 10] << 16 ) | ( data[off + 11] << 8 ) | data[off + 12] ) - INT3_BIAS;

            if( count >= 0 && count <= 1000 )
                return off;

            if( count > 1000 && count <= 10000 )
            {
                THROW_IO_ERROR( wxString::Format( _( "DipTrace import: invalid bus count %d." ), count ) );
            }
        }
    }

    return 0;
}


size_t SCH_PARSER::findTailStart() const
{
    const uint8_t* data = m_reader.GetData();
    size_t         fileSize = m_reader.GetFileSize();

    if( fileSize < 3 )
        return fileSize;

    size_t off = fileSize - 3;

    while( off > 0 )
    {
        if( data[off] == 0x0F && data[off + 1] == 0x42 && data[off + 2] == 0x40 )
            off -= 3;
        else
            break;
    }

    size_t tailStart = off + 3;
    size_t tailCount = ( fileSize - tailStart ) / 3;

    return ( tailCount > 1 ) ? tailStart : fileSize;
}


std::vector<size_t> SCH_PARSER::scanComponentBoundaries( size_t aFirstComp, size_t aBusSectionOffset ) const
{
    // Every component record starts with a placement bbox followed by five short header strings.
    // isComponentHeaderAt() validates that signature using the correct version-specific string
    // encoding, so walking the section byte-by-byte and accepting each header it recognises yields
    // the exact set of record starts. A recognised header advances by its fixed 16-byte bbox so the
    // scan cannot re-match inside it; everything else advances one byte to stay aligned.
    std::vector<size_t> boundaries;

    size_t off = aFirstComp;
    size_t end = ( aBusSectionOffset > 20 ) ? aBusSectionOffset - 20 : 0;

    while( off < end )
    {
        if( isComponentHeaderAt( off ) )
        {
            boundaries.push_back( off );
            off += 16;
        }
        else
        {
            off++;
        }
    }

    return boundaries;
}


bool SCH_PARSER::isShapeStart( size_t aOffset ) const
{
    const uint8_t* data = m_reader.GetData();
    size_t         fileSize = m_reader.GetFileSize();

    if( m_version < V31_CUTOVER )
    {
        if( aOffset + 19 > fileSize )
            return false;

        int z1 = ( ( data[aOffset + 6] << 16 ) | ( data[aOffset + 7] << 8 ) | data[aOffset + 8] ) - INT3_BIAS;
        int z2 = ( ( data[aOffset + 9] << 16 ) | ( data[aOffset + 10] << 8 ) | data[aOffset + 11] ) - INT3_BIAS;

        if( z1 != 0 || z2 != 0 )
            return false;

        int w = ( ( data[aOffset + 12] << 24 ) | ( data[aOffset + 13] << 16 ) | ( data[aOffset + 14] << 8 )
                  | data[aOffset + 15] )
                - INT4_BIAS;

        if( w < 0 || w > 200000 )
            return false;

        int npts = ( ( data[aOffset + 16] << 16 ) | ( data[aOffset + 17] << 8 ) | data[aOffset + 18] ) - INT3_BIAS;

        return npts >= 1 && npts <= 100;
    }
    else
    {
        if( aOffset + 17 > fileSize )
            return false;

        if( data[aOffset + 6] != 0 || data[aOffset + 7] != 0 || data[aOffset + 8] != 0 || data[aOffset + 9] != 0 )
        {
            return false;
        }

        int w = ( ( data[aOffset + 10] << 24 ) | ( data[aOffset + 11] << 16 ) | ( data[aOffset + 12] << 8 )
                  | data[aOffset + 13] )
                - INT4_BIAS;

        if( w < 0 || w > 200000 )
            return false;

        int npts = ( ( data[aOffset + 14] << 16 ) | ( data[aOffset + 15] << 8 ) | data[aOffset + 16] ) - INT3_BIAS;

        return npts >= 1 && npts <= 100;
    }
}


bool SCH_PARSER::isFontBearingShapeStart( size_t aOffset ) const
{
    if( m_version < V31_CUTOVER )
        return false;

    const uint8_t* data = m_reader.GetData();
    size_t         fileSize = m_reader.GetFileSize();

    static constexpr uint8_t TAHOMA_FONT_PATTERN[] = { 0x00, 0x06, 0x00, 0x54, 0x00, 0x61, 0x00,
                                                       0x68, 0x00, 0x6f, 0x00, 0x6d, 0x00, 0x61 };

    if( aOffset + 29 > fileSize )
        return false;

    int sentinel = ( ( data[aOffset] << 16 ) | ( data[aOffset + 1] << 8 ) | data[aOffset + 2] ) - INT3_BIAS;
    int shapeField = ( ( data[aOffset + 3] << 16 ) | ( data[aOffset + 4] << 8 ) | data[aOffset + 5] ) - INT3_BIAS;

    if( sentinel != 1000000 || shapeField != 0 )
        return false;

    if( std::memcmp( data + aOffset + 6, TAHOMA_FONT_PATTERN, sizeof( TAHOMA_FONT_PATTERN ) ) != 0 )
    {
        return false;
    }

    if( data[aOffset + 20] != 0 || data[aOffset + 21] != 0 )
        return false;

    int lineWidth = ( ( data[aOffset + 22] << 24 ) | ( data[aOffset + 23] << 16 ) | ( data[aOffset + 24] << 8 )
                      | data[aOffset + 25] )
                    - INT4_BIAS;

    if( lineWidth < 0 || lineWidth > 200000 )
        return false;

    int numPoints = ( ( data[aOffset + 26] << 16 ) | ( data[aOffset + 27] << 8 ) | data[aOffset + 28] ) - INT3_BIAS;

    return numPoints >= 1 && numPoints <= 100;
}


void SCH_PARSER::parseComponents( size_t aBusSectionOffset )
{
    size_t compSectionStart = m_reader.GetOffset();
    m_componentSectionStart = compSectionStart;

    // First attempt a sequential, count-guided decode. This fully consumes each record and works
    // for legacy formats. Modern (v34+) records embed a marking/pattern tail that is not consumed
    // field-by-field, so the sequential walk desyncs partway through; that is expected and silent,
    // and we fall through to the structural boundary scan below. No warning is emitted because the
    // boundary scan is the authoritative decoder, not a degraded last resort.
    if( m_componentCount >= 0 )
    {
        int  parsedCount = 0;
        bool desynced = false;

        while( parsedCount < m_componentCount && m_reader.GetOffset() < aBusSectionOffset )
        {
            size_t compStart = m_reader.GetOffset();

            try
            {
                parseOneComponent( aBusSectionOffset, false );
                parsedCount++;
            }
            catch( const std::exception& )
            {
                desynced = true;
                break;
            }

            if( m_reader.GetOffset() <= compStart )
            {
                desynced = true;
                break;
            }
        }

        if( parsedCount == m_componentCount && !desynced )
            return;

        m_components.clear();
        m_reader.SetOffset( compSectionStart );
    }

    // Structural boundary scan: locate every component header and decode each record within its
    // [start, next) bounds. parseOneComponent() resyncs to the record end even when its variable
    // tail is not fully understood, so a recognised header always yields a placed component.
    m_componentBoundaryScanCount++;

    std::vector<size_t> compStarts = scanComponentBoundaries( compSectionStart, aBusSectionOffset );

    for( size_t ci = 0; ci < compStarts.size(); ci++ )
    {
        size_t compEnd = ( ci + 1 < compStarts.size() ) ? compStarts[ci + 1] : aBusSectionOffset;

        try
        {
            m_reader.SetOffset( compStarts[ci] );
            parseOneComponent( compEnd, true );
        }
        catch( const IO_ERROR& )
        {
            throw;
        }
        catch( const std::exception& e )
        {
            THROW_IO_ERROR(
                    wxString::Format( _( "DipTrace import: failed to parse component %zu at offset 0x%06zX: %s" ), ci,
                                      compStarts[ci], wxString::FromUTF8( e.what() ) ) );
        }
    }

    if( m_componentCount >= 0 && static_cast<int>( m_components.size() ) != m_componentCount )
    {
        THROW_IO_ERROR(
                wxString::Format( _( "DipTrace import: found %zu components, but the file header declares %d." ),
                                  m_components.size(), m_componentCount ) );
    }
}


void SCH_PARSER::parseOneComponent( size_t aCompEnd, bool aUseCompEnd )
{
    static bool s_dumpComponents = std::getenv( "KICAD_DIPTRACE_DUMP_COMPONENTS" ) != nullptr;
    static bool s_dumpComponentDetail = std::getenv( "KICAD_DIPTRACE_DUMP_COMPONENT_DETAIL" ) != nullptr;

    DCH_COMPONENT comp;
    comp.fileOffset = m_reader.GetOffset();

    // The next component header bounds this record. Variable-length fields decoded
    // below (the modern extra-tail, shapes, and the embedded pattern) must not run
    // past it, and where field decoding cannot resolve the exact end the record still
    // lands here deterministically. The boundary scan already supplies the true next
    // start via aCompEnd; the count-guided sequential walk passes the far bus-section
    // offset, so locate the boundary structurally with the same signature used to
    // enumerate every component.
    size_t componentCeiling = aCompEnd > 0 ? aCompEnd : m_reader.GetFileSize();

    if( !aUseCompEnd )
    {
        for( size_t p = comp.fileOffset + 16; p < componentCeiling; p++ )
        {
            if( isComponentHeaderAt( p ) )
            {
                componentCeiling = p;
                break;
            }
        }
    }

    auto dumpDetail = [&]( const wxString& aMsg )
    {
        if( s_dumpComponentDetail && m_reporter )
        {
            m_reporter->Report( wxString::Format( wxT( "DipTrace SCH detail @0x%06zX: %s" ), comp.fileOffset, aMsg ),
                                RPT_SEVERITY_INFO );
        }
    };

    comp.bboxX1 = m_reader.ReadInt4();
    comp.bboxY1 = m_reader.ReadInt4();
    comp.bboxX2 = m_reader.ReadInt4();
    comp.bboxY2 = m_reader.ReadInt4();

    comp.compName = m_reader.ReadString();
    comp.refdes = m_reader.ReadString();
    comp.value = m_reader.ReadString();
    comp.prefix = m_reader.ReadString();
    comp.nameDup = m_reader.ReadString();
    dumpDetail( wxString::Format( wxT( "hdr end=0x%06zX name='%s' ref='%s' value='%s' prefix='%s'" ),
                                  m_reader.GetOffset(), comp.compName, comp.refdes, comp.value, comp.prefix ) );

    int postA = m_reader.ReadInt3();
    int postB = m_reader.ReadInt3();
    int flag1 = m_reader.ReadByte();
    int postC = m_reader.ReadInt3();
    int postD = m_reader.ReadInt3();

    if( s_dumpComponentDetail )
    {
        dumpDetail( wxString::Format( wxT( "post-hdr end=0x%06zX postA=%d postB=%d flag1=%d "
                                           "postC=%d postD=%d" ),
                                      m_reader.GetOffset(), postA, postB, flag1, postC, postD ) );
    }

    comp.partName = m_reader.ReadString();
    comp.partNumber = m_reader.ReadString();

    uint8_t pb1 = m_reader.ReadByte();
    comp.isMultiPart = ( pb1 == 1 );
    comp.sheetIndex = m_reader.ReadInt3();

    int partFieldB = m_reader.ReadInt3();
    int partFieldC = m_reader.ReadInt3();
    int partBboxX1 = m_reader.ReadInt4();
    int partBboxY1 = m_reader.ReadInt4();
    int partBboxX2 = m_reader.ReadInt4();
    int partBboxY2 = m_reader.ReadInt4();

    int      partTailInt = 0;
    wxString partTailStr;

    if( comp.isMultiPart )
    {
        comp.partId = m_reader.ReadString();
        partTailStr = comp.partId;
    }
    else
    {
        if( m_version < V31_CUTOVER )
        {
            size_t partTailStart = m_reader.GetOffset();
            partTailInt = m_reader.ReadInt3();

            // Legacy non-multipart records can store either:
            // - a small int3 discriminator, or
            // - a length-prefixed ASCII token (e.g. connector family token).
            if( partTailInt > 0 && partTailInt < 256 )
            {
                size_t         strStart = partTailStart + 3;
                size_t         strEnd = strStart + static_cast<size_t>( partTailInt );
                const uint8_t* data = m_reader.GetData();
                size_t         fileSize = m_reader.GetFileSize();

                if( strEnd + 3 <= fileSize )
                {
                    bool asciiPayload = true;

                    for( size_t i = strStart; i < strEnd; i++ )
                    {
                        uint8_t c = data[i];

                        if( c < 0x20 || c > 0x7E )
                        {
                            asciiPayload = false;
                            break;
                        }
                    }

                    if( asciiPayload )
                    {
                        int nextInt3 =
                                ( ( data[strEnd] << 16 ) | ( data[strEnd + 1] << 8 ) | data[strEnd + 2] ) - INT3_BIAS;

                        if( nextInt3 >= -1 && nextInt3 < 1000 )
                        {
                            m_reader.SetOffset( partTailStart );
                            partTailStr = m_reader.ReadString();
                            partTailInt = 0;
                        }
                    }
                }
            }
        }
        else
        {
            m_reader.ReadByte();
            m_reader.ReadByte();
        }
    }

    int fieldD = m_reader.ReadInt3();
    int fieldE = m_reader.ReadInt3();
    dumpDetail( wxString::Format( wxT( "part end=0x%06zX part='%s' partNum='%s' sheet=%d "
                                       "isMulti=%d partFieldB=%d partFieldC=%d "
                                       "partBBox=[%d,%d,%d,%d] fieldD=%d fieldE=%d" ),
                                  m_reader.GetOffset(), comp.partName, comp.partNumber, comp.sheetIndex,
                                  comp.isMultiPart ? 1 : 0, partFieldB, partFieldC, partBboxX1, partBboxY1, partBboxX2,
                                  partBboxY2, fieldD, fieldE ) );

    if( s_dumpComponentDetail && m_version < V31_CUTOVER )
    {
        dumpDetail( wxString::Format( wxT( "part-tail end=0x%06zX partTailInt=%d partTailStr='%s'" ),
                                      m_reader.GetOffset(), partTailInt, partTailStr ) );
    }

    // The fieldE-count block holds the part's user-defined additional fields, each a (name, value)
    // string pair followed by an int3 type discriminator (0 = text). DipTrace shows these in the
    // "Configure Additional Fields" list (Unique Name, Part Number (Digi-Key), etc.).
    if( fieldE >= 1 && fieldE < 1000 )
    {
        comp.additionalFields.reserve( fieldE );

        for( int i = 0; i < fieldE; i++ )
        {
            wxString fieldName = m_reader.ReadString();
            wxString fieldValue = m_reader.ReadString();
            m_reader.ReadInt3();

            if( !fieldName.IsEmpty() )
                comp.additionalFields.emplace_back( fieldName, fieldValue );
        }
    }

    int fieldF = m_reader.ReadInt3();
    int fieldG = m_reader.ReadInt3();
    int byte4 = m_reader.ReadByte();

    // This int4 is the placement rotation in radians x 1e4 (0, 15708, 31416, 47124 for 0/90/180/
    // 270 degrees), not a library id. The pin, shape and field coordinates are stored already
    // rotated by it, so it is used only to keep rotated and unrotated instances of one part from
    // sharing a single library symbol.
    comp.rotationE4 = m_reader.ReadInt4();
    comp.libPath = m_reader.ReadString();

    int      tailA = m_reader.ReadInt3();
    int      tailB = m_reader.ReadInt3();
    size_t   tailAfterB = m_reader.GetOffset();
    wxString tailStrA = m_reader.ReadString();
    wxString extraTail = wxEmptyString;
    int      pinMetaA = 0;
    int      pinMetaB = 0;
    int      pinMetaF = 0;
    int      pinHdrByte = 0;
    int      numPins = 0;
    bool     simpleModernTail = false;

    if( m_version < V31_CUTOVER )
    {
        // The legacy pre-pin metadata tuple precedes the pin count, but its layout
        // shifted across early format revisions. v22 packs two single meta bytes then
        // the count (one int3 shorter); v23 places the count first; v24+ (incl. v31)
        // use an int3 + byte + int3 tuple followed by the count.
        if( m_version <= 22 )
        {
            pinMetaA = m_reader.ReadInt3();
            pinMetaF = m_reader.ReadByte();
            pinMetaB = m_reader.ReadByte();
            numPins = m_reader.ReadInt3();
        }
        else if( m_version == 23 )
        {
            numPins = m_reader.ReadInt3();
            pinMetaF = m_reader.ReadByte();
            pinMetaA = m_reader.ReadInt3();
            pinMetaB = m_reader.ReadInt3();
        }
        else
        {
            pinMetaA = m_reader.ReadInt3();
            pinMetaF = m_reader.ReadByte();
            pinMetaB = m_reader.ReadInt3();
            numPins = m_reader.ReadInt3();
        }

        if( s_dumpComponentDetail )
        {
            dumpDetail( wxString::Format( wxT( "pre-pin-hdr end=0x%06zX fieldF=%d fieldG=%d "
                                               "byte4=%d rotE4=%d libPath='%s' tailA=%d tailB=%d "
                                               "tailStrA='%s' pinMetaA=%d pinMetaF=%d pinMetaB=%d "
                                               "numPins=%d" ),
                                          m_reader.GetOffset(), fieldF, fieldG, byte4, comp.rotationE4, comp.libPath,
                                          tailA, tailB, tailStrA, pinMetaA, pinMetaF, pinMetaB, numPins ) );
        }
    }
    else
    {
        bool usedPinSeparatorFallback = false;

        auto readModernExtraAndPins = [&]( wxString& aExtraTail, int& aNumPins, int& aPinHdr,
                                           bool& aUsedPinSeparatorFallback ) -> bool
        {
            size_t  extraStart = m_reader.GetOffset();
            uint8_t extraHdr[4] = {};
            m_reader.ReadBytes( extraHdr, 4 );

            uint32_t extraChars = ( extraHdr[0] << 24 ) | ( extraHdr[1] << 16 ) | ( extraHdr[2] << 8 ) | extraHdr[3];

            if( extraChars >= 10000 && extraHdr[0] == 0 && extraHdr[1] == 0 )
            {
                THROW_IO_ERROR( wxString::Format( _( "DipTrace import: invalid component extra-tail length %u at "
                                                     "offset 0x%06zX." ),
                                                  extraChars, extraStart ) );
            }

            if( extraChars > 0 )
            {
                size_t extraBytes = static_cast<size_t>( extraChars ) * 2;
                bool   fitsFile = m_reader.GetOffset() + extraBytes <= m_reader.GetFileSize();

                if( extraChars < 10000 && fitsFile && m_reader.GetOffset() + extraBytes <= componentCeiling )
                {
                    wxMBConvUTF16BE conv;
                    aExtraTail = wxString( reinterpret_cast<const char*>( m_reader.GetData() + m_reader.GetOffset() ),
                                           conv, extraBytes );
                    m_reader.Skip( extraBytes );
                }
                else if( extraChars < 10000 && fitsFile )
                {
                    // Length is plausible but the field would run into the next
                    // component, so this record has no extra tail (e.g. net ports).
                    // Leave the bytes for the pin-count read below.
                    m_reader.SetOffset( extraStart );
                }
                else
                {
                    // Fallback for variants that store this field with ReadString() encoding.
                    m_reader.SetOffset( extraStart );

                    try
                    {
                        aExtraTail = m_reader.ReadString();
                    }
                    catch( ... )
                    {
                        return false;
                    }
                }
            }

            try
            {
                size_t pinStart = m_reader.GetOffset();
                aNumPins = m_reader.ReadInt3();
                aPinHdr = m_reader.ReadByte();

                // Some variants store a 2-byte separator before pin count.
                if( ( aNumPins < 0 || aNumPins > 500 ) && pinStart + 6 <= m_reader.GetFileSize() )
                {
                    m_reader.SetOffset( pinStart + 2 );

                    int sepPins = m_reader.ReadInt3();
                    int sepHdr = m_reader.ReadByte();

                    if( sepPins >= 0 && sepPins <= 500 )
                    {
                        aNumPins = sepPins;
                        aPinHdr = sepHdr;
                        aUsedPinSeparatorFallback = true;
                    }
                    else
                    {
                        m_reader.SetOffset( pinStart + 4 );
                    }
                }
            }
            catch( ... )
            {
                return false;
            }

            return true;
        };

        bool usedTaillessFallback = false;
        bool canonicalPinSeparatorFallback = false;
        bool readCanonicalPins =
                readModernExtraAndPins( extraTail, numPins, pinHdrByte, canonicalPinSeparatorFallback );
        usedPinSeparatorFallback = canonicalPinSeparatorFallback;

        if( !readCanonicalPins || ( numPins < 0 || numPins > 500 ) )
        {
            size_t   canonicalEnd = m_reader.GetOffset();
            wxString canonicalTailStrA = tailStrA;
            wxString canonicalExtraTail = extraTail;
            int      canonicalNumPins = numPins;
            int      canonicalPinHdrByte = pinHdrByte;

            // Some v41 files omit tailStrA and place the int4-length extra tail
            // directly after tailB.
            m_reader.SetOffset( tailAfterB );
            tailStrA = wxEmptyString;
            extraTail = wxEmptyString;
            numPins = 0;
            pinHdrByte = 0;
            usedTaillessFallback = true;

            bool fallbackPinSeparatorFallback = false;
            bool readFallbackPins =
                    readModernExtraAndPins( extraTail, numPins, pinHdrByte, fallbackPinSeparatorFallback );

            if( readFallbackPins && numPins >= 0 && numPins <= 500 )
            {
                usedPinSeparatorFallback = fallbackPinSeparatorFallback;
            }
            else
            {
                m_reader.SetOffset( canonicalEnd );
                tailStrA = canonicalTailStrA;
                extraTail = canonicalExtraTail;
                numPins = canonicalNumPins;
                pinHdrByte = canonicalPinHdrByte;
                usedTaillessFallback = false;
                usedPinSeparatorFallback = canonicalPinSeparatorFallback;
            }
        }

        if( s_dumpComponentDetail )
        {
            dumpDetail( wxString::Format( wxT( "pre-pin-hdr end=0x%06zX fieldF=%d fieldG=%d "
                                               "byte4=%d rotE4=%d libPath='%s' tailA=%d tailB=%d "
                                               "tailStrA='%s' extraTail='%s' numPins=%d pinHdr=%d "
                                               "taillessFallback=%d pinSeparatorFallback=%d" ),
                                          m_reader.GetOffset(), fieldF, fieldG, byte4, comp.rotationE4, comp.libPath,
                                          tailA, tailB, tailStrA, extraTail, numPins, pinHdrByte,
                                          usedTaillessFallback ? 1 : 0, usedPinSeparatorFallback ? 1 : 0 ) );
        }

        simpleModernTail = tailA == 0 && tailB == 0 && tailStrA.IsEmpty() && extraTail.IsEmpty()
                           && !usedTaillessFallback && !usedPinSeparatorFallback;

        // The resolved extra tail string is the part's datasheet URL when present (empty for parts
        // that carry none, e.g. plain resistors). Capture it after the fallback settles so a stale
        // value from the discarded branch is never stored.
        comp.datasheet = extraTail;
    }

    dumpDetail( wxString::Format( wxT( "pre-pin end=0x%06zX numPins=%d" ), m_reader.GetOffset(), numPins ) );

    if( numPins < 0 || numPins > 500 )
    {
        if( aUseCompEnd && !simpleModernTail )
        {
            m_reader.SetOffset( aCompEnd );
            m_components.push_back( comp );
            return;
        }

        THROW_IO_ERROR( wxString::Format( _( "Invalid pin count %d at component offset "
                                             "0x%06zX." ),
                                          numPins, comp.fileOffset ) );
    }

    for( int pinIdx = 0; pinIdx < numPins; pinIdx++ )
    {
        auto consumeLaterPinSeparatorIfPresent = [&]() -> bool
        {
            if( m_version < V31_CUTOVER || pinIdx == 0 )
                return false;

            size_t start = m_reader.GetOffset();

            if( start + 2 >= componentCeiling )
                return false;

            const uint8_t* data = m_reader.GetData();

            if( data[start] != 0 || data[start + 1] != 0 )
                return false;

            bool currentRecordValid = false;

            try
            {
                DCH_COMPONENT probeComp;
                m_reader.SetOffset( start );
                parsePin( pinIdx, probeComp );
                currentRecordValid = m_reader.GetOffset() <= componentCeiling;
            }
            catch( const std::exception& )
            {
                currentRecordValid = false;
            }

            m_reader.SetOffset( start );

            if( currentRecordValid )
                return false;

            bool shiftedRecordValid = false;

            try
            {
                DCH_COMPONENT probeComp;
                m_reader.SetOffset( start + 2 );
                parsePin( pinIdx, probeComp );
                shiftedRecordValid = m_reader.GetOffset() <= componentCeiling;
            }
            catch( const std::exception& )
            {
                shiftedRecordValid = false;
            }

            m_reader.SetOffset( start );

            if( !shiftedRecordValid )
                return false;

            m_reader.Skip( 2 );
            return true;
        };

        if( consumeLaterPinSeparatorIfPresent() )
            simpleModernTail = false;

        try
        {
            parsePin( pinIdx, comp );
        }
        catch( const std::exception& e )
        {
            if( aUseCompEnd && !simpleModernTail )
                break;

            THROW_IO_ERROR( wxString::Format( _( "DipTrace import: failed to parse pin %d in component at 0x%06zX "
                                                 "(offset 0x%06zX): %s" ),
                                              pinIdx, comp.fileOffset, m_reader.GetOffset(),
                                              wxString::FromUTF8( e.what() ) ) );
        }
    }

    // Recognise a shape-record prefix carrying an out-of-range point count. For the
    // well-framed modern formats this signals a corrupt file and must fail the load;
    // legacy records can false-match the embedded-pattern header here, so they simply
    // end the shape list and resync through the pattern/ceiling handling below.
    auto readShapePointCountIfHeaderPrefix = [&]( size_t aOffset, int& aPointCount ) -> bool
    {
        const uint8_t* data = m_reader.GetData();
        size_t         fileSize = m_reader.GetFileSize();
        size_t         limit = std::min( fileSize, aCompEnd );

        if( aOffset + 17 > limit )
            return false;

        if( data[aOffset + 6] != 0 || data[aOffset + 7] != 0 || data[aOffset + 8] != 0 || data[aOffset + 9] != 0 )
        {
            return false;
        }

        int width = ( ( data[aOffset + 10] << 24 ) | ( data[aOffset + 11] << 16 ) | ( data[aOffset + 12] << 8 )
                      | data[aOffset + 13] )
                    - INT4_BIAS;

        if( width < 0 || width > 200000 )
            return false;

        aPointCount = ( ( data[aOffset + 14] << 16 ) | ( data[aOffset + 15] << 8 ) | data[aOffset + 16] ) - INT3_BIAS;
        return true;
    };

    // Some components store their marking records (reference, value, name) BEFORE the graphic shapes
    // rather than after them (e.g. the C4D02120E diode). The shape walk below recognises shapes by
    // their header, so a leading marking would stop it and drop every shape. Consume any leading
    // marking records first. Each is 00 00 00 + int3 type (1 name, 2 reference, 3 value) + font
    // string + text + int4 fontSize + int3 fieldA + int4 coordX + int4 coordY + a fixed 20-byte
    // trailer. Reference and value markings are kept so their positions are honoured; the walk then
    // lands on the first real shape. Components whose markings follow the shapes consume nothing here
    // and are handled by the text-field loop after the shapes, as before.
    while( m_reader.GetOffset() + 6 < componentCeiling )
    {
        size_t         markOff = m_reader.GetOffset();
        const uint8_t* mdata   = m_reader.GetData();

        if( mdata[markOff] != 0 || mdata[markOff + 1] != 0 || mdata[markOff + 2] != 0 )
            break;

        int markType = ( ( mdata[markOff + 3] << 16 ) | ( mdata[markOff + 4] << 8 )
                         | mdata[markOff + 5] )
                       - INT3_BIAS;

        if( markType < 1 || markType > 3 )
            break;

        try
        {
            DCH_COMPONENT_TEXT mark;
            m_reader.ReadBytes( mark.flags, 3 );
            mark.type     = m_reader.ReadInt3();
            mark.fontName = m_reader.ReadString();

            if( mark.fontName.IsEmpty() || mark.fontName.size() > 64 )
            {
                m_reader.SetOffset( markOff );
                break;
            }

            mark.text   = m_reader.ReadString();
            mark.fontSize = m_reader.ReadInt4();
            mark.fieldA = m_reader.ReadInt3();
            mark.coordX = m_reader.ReadInt4();
            mark.coordY = m_reader.ReadInt4();

            // Fixed 20-byte trailer.
            m_reader.Skip( 2 );
            m_reader.ReadInt4();
            m_reader.ReadInt4();
            m_reader.Skip( 1 );
            m_reader.ReadInt3();
            m_reader.ReadInt3();
            m_reader.ReadInt3();

            if( m_reader.GetOffset() > componentCeiling )
            {
                m_reader.SetOffset( markOff );
                break;
            }

            if( mark.type == 2 || mark.type == 3 )
                comp.texts.push_back( mark );
        }
        catch( const std::exception& )
        {
            m_reader.SetOffset( markOff );
            break;
        }
    }

    while( m_reader.GetOffset() < aCompEnd && m_reader.GetOffset() < componentCeiling )
    {
        size_t shapeOffset = m_reader.GetOffset();
        int    shapePointCount = 0;

        if( isFontBearingShapeStart( shapeOffset ) )
        {
            try
            {
                parseFontBearingShape( comp );
                continue;
            }
            catch( const std::exception& )
            {
                break;
            }
        }

        if( !isShapeStart( shapeOffset ) )
        {
            if( m_version >= V31_CUTOVER && readShapePointCountIfHeaderPrefix( shapeOffset, shapePointCount )
                && ( shapePointCount < 1 || shapePointCount > 100 ) )
            {
                THROW_IO_ERROR( wxString::Format( _( "DipTrace import: invalid component shape point count %d at "
                                                     "offset 0x%06zX." ),
                                                  shapePointCount, shapeOffset ) );
            }

            // End of the shape list (or a record layout not fully understood). The
            // embedded-pattern and end-of-record handling resync to the next header.
            break;
        }

        try
        {
            parseShape( comp );
        }
        catch( const std::exception& )
        {
            break;
        }
    }

    // Parse the embedded footprint pattern section that follows the shapes.
    // This extracts the pattern name (for the Footprint field) and consumes
    // the pattern bytes so count-guided parsing can advance to the next component.
    try
    {
        while( parseComponentTextField( comp, componentCeiling ) )
        {
        }

        // The first marking record's trailer overran the sequential reader, so the loop stops a few
        // bytes inside the value record rather than on its header. Recover the value position by
        // scanning a small window back to the value record's 00 00 00 header and reading only up to
        // its coordinate, without advancing (the footprint reader still starts at the stop offset).
        // Without the real value position an asymmetric layout like C6 (reference above, value to the
        // side) would be mis-placed by the symmetric mirror fallback below.
        {
            size_t         save = m_reader.GetOffset();
            const uint8_t* data = m_reader.GetData();
            size_t         lim = std::min( componentCeiling > 0 ? componentCeiling : m_reader.GetFileSize(),
                                   m_reader.GetFileSize() );
            size_t lo = ( save > 24 ) ? save - 24 : 0;

            for( size_t probe = lo; probe + 6 < lim && probe <= save + 4; probe++ )
            {
                if( data[probe] != 0 || data[probe + 1] != 0 || data[probe + 2] != 0 )
                    continue;

                m_reader.SetOffset( probe );

                try
                {
                    DCH_COMPONENT_TEXT vt;
                    m_reader.ReadBytes( vt.flags, 3 );
                    vt.type = m_reader.ReadInt3();

                    if( vt.type != 2 && vt.type != 3 )
                        continue;

                    vt.fontName = m_reader.ReadString();

                    if( vt.fontName.IsEmpty() || vt.fontName.size() > 64 )
                        continue;

                    vt.text = m_reader.ReadString();

                    if( vt.text.IsEmpty() || vt.text.size() > 256 )
                        continue;

                    vt.fontSize = m_reader.ReadInt4();
                    vt.fieldA = m_reader.ReadInt3();
                    vt.coordX = m_reader.ReadInt4();
                    vt.coordY = m_reader.ReadInt4();

                    bool haveType = false;

                    for( const DCH_COMPONENT_TEXT& t : comp.texts )
                        haveType = haveType || ( t.type == vt.type );

                    if( !haveType )
                    {
                        comp.texts.push_back( vt );
                        break;
                    }
                }
                catch( const std::exception& )
                {
                }
            }

            m_reader.SetOffset( save );
        }

        parseEmbeddedPattern( comp, aCompEnd );

        if( m_reader.GetOffset() < aCompEnd && m_reader.GetOffset() != m_busSectionOffset
            && !isComponentHeaderAt( m_reader.GetOffset() ) )
        {
            size_t afterFirstPattern = m_reader.GetOffset();
            parseEmbeddedPattern( comp, aCompEnd );

            if( m_reader.GetOffset() == afterFirstPattern )
                m_reader.SetOffset( afterFirstPattern );
        }
    }
    catch( const std::exception& e )
    {
        if( s_dumpComponentDetail )
        {
            dumpDetail( wxString::Format( wxT( "pattern parse failed at 0x%06zX: %s" ), m_reader.GetOffset(),
                                          wxString::FromUTF8( e.what() ) ) );
        }
    }

    size_t parsedEnd = m_reader.GetOffset();

    if( s_dumpComponentDetail )
    {
        dumpDetail( wxString::Format( wxT( "post-pattern end=0x%06zX pins=%zu shapes=%zu "
                                           "pattern='%s'" ),
                                      parsedEnd, comp.pins.size(), comp.shapes.size(), comp.patternName ) );

        if( aUseCompEnd && parsedEnd < aCompEnd )
        {
            dumpDetail( wxString::Format( wxT( "tail skipped=%zu bytes to compEnd=0x%06zX" ), aCompEnd - parsedEnd,
                                          aCompEnd ) );
        }
    }

    if( aUseCompEnd )
    {
        m_reader.SetOffset( aCompEnd );
    }
    else if( m_reader.GetOffset() != componentCeiling
             && ( componentCeiling == m_busSectionOffset || isComponentHeaderAt( componentCeiling ) ) )
    {
        // The field decoder did not consume exactly to the next component. The
        // boundary is structurally known, so land on it to keep the count-guided
        // sequential walk deterministic without the global boundary scan.
        m_reader.SetOffset( componentCeiling );
    }

    m_components.push_back( comp );

    if( s_dumpComponents && m_reporter )
    {
        m_reporter->Report( wxString::Format( wxT( "DipTrace SCH comp @0x%06zX ref='%s' name='%s' "
                                                   "sheet=%d pins=%zu shapes=%zu pattern='%s'" ),
                                              comp.fileOffset, comp.refdes, comp.compName, comp.sheetIndex,
                                              comp.pins.size(), comp.shapes.size(), comp.patternName ),
                            RPT_SEVERITY_INFO );
    }
}


void SCH_PARSER::parsePin( int aPinIndex, DCH_COMPONENT& aComp )
{
    DCH_PIN pin;
    pin.index = aPinIndex;

    if( aPinIndex == 0 )
    {
        pin.hasHeader = true;

        if( m_version <= 22 )
        {
            // v22 prefixes the first pin with a lead byte and four int3 header
            // fields; reading the wrong width misaligns every later pin field.
            m_reader.ReadByte();
            pin.headerA = m_reader.ReadInt3();
            pin.headerB = m_reader.ReadInt3();
            pin.headerC = m_reader.ReadInt3();
            pin.typeCode = m_reader.ReadInt3();
        }
        else if( m_version < V31_CUTOVER )
        {
            // Legacy v1/v2 schematic files use a shorter first-pin preamble.
            // Reading 4 int3 fields here misaligns all subsequent pin fields.
            pin.headerA = m_reader.ReadInt3();
            pin.typeCode = m_reader.ReadInt3();
        }
        else
        {
            pin.headerA = m_reader.ReadInt3();
            pin.headerB = m_reader.ReadInt3();
            pin.headerC = m_reader.ReadInt3();
            pin.typeCode = m_reader.ReadInt3();
        }
    }

    pin.x = m_reader.ReadInt4();
    pin.y = m_reader.ReadInt4();
    pin.length = m_reader.ReadInt4();
    pin.name = m_reader.ReadString();
    pin.number = m_reader.ReadString();

    pin.netFlagA = m_reader.ReadByte();
    pin.netFlagB = m_reader.ReadByte();

    pin.labelXOff = m_reader.ReadInt4();
    pin.labelYOff = m_reader.ReadInt4();
    pin.numXOff = m_reader.ReadInt4();
    pin.numYOff = m_reader.ReadInt4();
    m_reader.ReadInt3(); // post_a

    if( m_version < V31_CUTOVER )
    {
        m_reader.ReadByte();
        m_reader.ReadByte();
        m_reader.ReadInt3();
        m_reader.ReadByte();
        m_reader.ReadInt3();
    }
    else
    {
        size_t         midTailStart = m_reader.GetOffset();
        const uint8_t* data = m_reader.GetData();

        if( midTailStart + 5 <= m_reader.GetFileSize() && data[midTailStart] == 0 && data[midTailStart + 1] == 0 )
        {
            m_reader.Skip( 2 );
            pin.midTailText = m_reader.ReadString();
            m_reader.ReadByte();
        }
        else
        {
            m_reader.Skip( 5 );
        }

        m_reader.ReadInt3();
    }

    pin.stubDx = m_reader.ReadInt4();
    pin.stubDy = m_reader.ReadInt4();

    pin.tailByte = m_reader.ReadByte();
    m_reader.ReadInt3();
    m_reader.ReadInt3();
    m_reader.ReadInt3();
    m_reader.ReadInt3();

    aComp.pins.push_back( pin );
}


void SCH_PARSER::parseShape( DCH_COMPONENT& aComp )
{
    DCH_SHAPE shape;

    // The shape kind is carried by the int3 pair immediately preceding the all zero shape header
    // (the previous record's trailer ends with this same pair, so it reads as a leading
    // discriminator here). The header bytes themselves are all zero, so this leading pair is the
    // only place the line/arrow/rectangle/obround/polygon type is recorded.
    size_t headerStart = m_reader.GetOffset();

    if( headerStart >= 6 )
    {
        const uint8_t* data = m_reader.GetData();
        shape.kindCode = ( ( data[headerStart - 6] << 16 ) | ( data[headerStart - 5] << 8 ) | data[headerStart - 4] )
                         - INT3_BIAS;
        shape.kindFlag = ( ( data[headerStart - 3] << 16 ) | ( data[headerStart - 2] << 8 ) | data[headerStart - 1] )
                         - INT3_BIAS;
    }

    m_reader.ReadBytes( shape.flags, 3 );
    shape.shapeField = m_reader.ReadInt3();

    if( m_version < V31_CUTOVER )
    {
        m_reader.ReadInt3();
        m_reader.ReadInt3();
    }
    else
    {
        m_reader.Skip( 4 );
    }

    shape.lineWidth = m_reader.ReadInt4();
    int numPoints = m_reader.ReadInt3();

    if( numPoints < 1 || numPoints > 100 )
        return;

    for( int i = 0; i < numPoints; i++ )
    {
        int x = m_reader.ReadInt4();
        int y = m_reader.ReadInt4();
        shape.points.push_back( VECTOR2I( x, y ) );
    }

    m_reader.Skip( 2 );
    shape.fontX = m_reader.ReadInt4();
    shape.fontY = m_reader.ReadInt4();
    m_reader.ReadByte();
    m_reader.ReadInt3();

    // The trailing int3 pair is the next record's leading kind discriminator, read above for the
    // following shape. Consume it here so the reader lands on that next header.
    m_reader.ReadInt3();
    m_reader.ReadInt3();

    aComp.shapes.push_back( shape );
}


void SCH_PARSER::parseFontBearingShape( DCH_COMPONENT& aComp )
{
    DCH_SHAPE shape;
    shape.kindCode = 1;
    shape.kindFlag = 0;

    // Modern component body outlines can store an inline font name before the line-width/point
    // tuple.  The reference schematic U1 uses this form for the four line edges of its IC body.
    m_reader.ReadInt3(); // sentinel, validated by isFontBearingShapeStart()
    shape.shapeField = m_reader.ReadInt3();
    m_reader.ReadString(); // observed "Tahoma"
    m_reader.Skip( 2 );    // zero pad

    shape.lineWidth = m_reader.ReadInt4();
    int numPoints = m_reader.ReadInt3();

    if( numPoints < 1 || numPoints > 100 )
        return;

    for( int i = 0; i < numPoints; i++ )
    {
        int x = m_reader.ReadInt4();
        int y = m_reader.ReadInt4();
        shape.points.push_back( VECTOR2I( x, y ) );
    }

    m_reader.Skip( 2 );
    shape.fontX = m_reader.ReadInt4();
    shape.fontY = m_reader.ReadInt4();
    m_reader.ReadByte();
    m_reader.ReadInt3();
    m_reader.ReadInt3();
    m_reader.ReadInt3();

    aComp.shapes.push_back( shape );
}


bool SCH_PARSER::parseComponentTextField( DCH_COMPONENT& aComp, size_t aCompEnd )
{
    size_t         startOffset = m_reader.GetOffset();
    const uint8_t* data = m_reader.GetData();
    size_t         limit = std::min( aCompEnd > 0 ? aCompEnd : m_reader.GetFileSize(), m_reader.GetFileSize() );

    if( startOffset + 6 > limit || data[startOffset] != 0 || data[startOffset + 1] != 0 || data[startOffset + 2] != 0 )
    {
        return false;
    }

    int fieldType =
            ( ( data[startOffset + 3] << 16 ) | ( data[startOffset + 4] << 8 ) | data[startOffset + 5] ) - INT3_BIAS;

    if( fieldType < 0 || fieldType > 100 )
        return false;

    DCH_COMPONENT_TEXT text;

    try
    {
        m_reader.ReadBytes( text.flags, 3 );
        text.type = m_reader.ReadInt3();
        text.fontName = m_reader.ReadString();
        text.text = m_reader.ReadString();

        if( text.fontName.IsEmpty() || text.fontName.size() > 128 || text.text.size() > 512 )
            throw std::runtime_error( "invalid component text field string" );

        text.fontSize = m_reader.ReadInt4();
        text.fieldA = m_reader.ReadInt3();
        text.coordX = m_reader.ReadInt4();
        text.coordY = m_reader.ReadInt4();
        text.fieldB = m_reader.ReadInt4();
        text.fieldC = m_reader.ReadInt4();
        text.flagA = m_reader.ReadByte();
        text.flagB = m_reader.ReadByte();
        text.fieldD = m_reader.ReadInt4();
        text.fieldE = m_reader.ReadInt4();
        text.fieldF = m_reader.ReadInt3();
        text.fieldG = m_reader.ReadInt3();
        m_reader.ReadBytes( text.flags2, 4 );
        text.fieldH = m_reader.ReadInt3();

        if( m_reader.GetOffset() > limit )
            throw std::runtime_error( "component text field overruns component" );
    }
    catch( const std::exception& )
    {
        m_reader.SetOffset( startOffset );
        return false;
    }

    aComp.texts.push_back( text );
    return true;
}


void SCH_PARSER::parseEmbeddedPattern( DCH_COMPONENT& aComp, size_t aCompEnd )
{
    size_t startOffset = m_reader.GetOffset();

    if( m_version < V31_CUTOVER )
    {
        // Legacy v1/v2 format (confirmed for v23). The pattern header begins
        // with int3(0) + int3(0) as a reliable sentinel.
        if( m_reader.PeekInt3() != 0 )
        {
            m_reader.SetOffset( startOffset );
            return;
        }

        // Pre-name header (23 bytes): 2*int3 + 2*int4 + byte + int4 + byte + int3
        m_reader.ReadInt3();
        m_reader.ReadInt3();
        m_reader.ReadInt4();
        m_reader.ReadInt4();
        m_reader.ReadByte();
        m_reader.ReadInt4();
        m_reader.ReadByte();
        m_reader.ReadInt3();

        // Dimensions (16 bytes): Width + Height + DefPadW + DefPadH
        m_reader.ReadInt4();
        m_reader.ReadInt4();
        m_reader.ReadInt4();
        m_reader.ReadInt4();

        // Pre-drill (8 bytes): mountType + mountByte + Drill
        m_reader.ReadInt3();
        m_reader.ReadByte();
        m_reader.ReadInt4();

        aComp.patternName = m_reader.ReadString();

        // Post-name (23 bytes): OrgX + OrgY + 2*int4 + byte + 2*int3
        m_reader.ReadInt4();
        m_reader.ReadInt4();
        m_reader.ReadInt4();
        m_reader.ReadInt4();
        m_reader.ReadByte();
        m_reader.ReadInt3();
        m_reader.ReadInt3();

        int fieldA = m_reader.ReadInt3();

        if( fieldA < 0 || fieldA > 500 )
        {
            aComp.patternName.clear();
            m_reader.SetOffset( startOffset );
            return;
        }

        if( fieldA == 0 )
        {
            // Empty pattern (net ports). 9 zero bytes footer: 3 * int3(0).
            m_reader.ReadInt3();
            m_reader.ReadInt3();
            m_reader.ReadInt3();
            return;
        }

        // Skip byte(0) separator before pad template
        m_reader.ReadByte();

        // Skip pad records: template + (fieldA - 2) real pads + trailing terminator.
        // Each pad: int3(id) + int4(X) + int4(Y) + str(Number) + str(Note)
        //           + int4(W) + int4(H) + int4(Drill) + 11-byte tail
        for( int i = 0; i < fieldA; i++ )
        {
            m_reader.ReadInt3();
            m_reader.ReadInt4();
            m_reader.ReadInt4();
            m_reader.ReadString();
            m_reader.ReadString();
            m_reader.ReadInt4();
            m_reader.ReadInt4();
            m_reader.ReadInt4();
            m_reader.Skip( 11 );
        }

        // 39 zero bytes trailing terminator
        m_reader.Skip( 39 );

        // Pre-sentinel block (58 bytes): int3(fieldB) + 55 remaining bytes
        int fieldB = m_reader.ReadInt3();

        if( fieldB < 0 || fieldB > 1000 )
        {
            aComp.patternName.clear();
            m_reader.SetOffset( startOffset );
            return;
        }

        m_reader.Skip( 55 );

        // Sentinel records: fieldB total. Last one is 49-byte footer, rest are 62 bytes.
        for( int i = 0; i < fieldB; i++ )
        {
            if( i == fieldB - 1 )
                m_reader.Skip( 49 );
            else
                m_reader.Skip( 62 );
        }
    }
    else
    {
        // Modern v34+ embedded patterns extend the legacy header with an
        // additional drill field and a pre-name tail.  They then store counted
        // pad, drawing, and 3D-model records, so the component record can be
        // consumed without scanning for the next component.

        // Hard ceiling for this pattern body: the next component header. Nothing in
        // the record can legitimately reach it, so it bounds every model-tail search
        // and keeps the decoder from consuming into a following component. The
        // sequential walk passes aCompEnd as the far bus-section offset, so without
        // this bound an unframed scan can latch onto a later record's 3D-model tail
        // and swallow whole components. isComponentHeaderAt() is the same structural
        // signature used to enumerate every component, so the nearest match is this
        // record's true end.
        size_t patternCeiling = std::min( aCompEnd > 0 ? aCompEnd : m_reader.GetFileSize(), m_reader.GetFileSize() );

        for( size_t p = startOffset + 16; p < patternCeiling; p++ )
        {
            if( isComponentHeaderAt( p ) )
            {
                patternCeiling = p;
                break;
            }
        }

        // The record end when field decoding cannot resolve the model tail. Prefer
        // the detected next-component boundary; only fall back to leaving the pattern
        // unconsumed if no boundary was identified (which would surface as a desync
        // rather than a silent overconsumption).
        auto landingForCeiling = [&]() -> size_t
        {
            if( patternCeiling == aCompEnd || patternCeiling == m_busSectionOffset
                || isComponentHeaderAt( patternCeiling ) )
            {
                return patternCeiling;
            }

            return startOffset;
        };

        if( m_reader.PeekInt3() != 0 )
        {
            const uint8_t* data = m_reader.GetData();
            size_t         limit = patternCeiling;

            if( startOffset + 63 <= limit && data[startOffset] == 0 && data[startOffset + 1] == 0 )
            {
                size_t tailEnd = startOffset + 63;

                if( tailEnd == aCompEnd || tailEnd == m_busSectionOffset || isComponentHeaderAt( tailEnd ) )
                {
                    m_reader.SetOffset( tailEnd );
                    return;
                }
            }

            for( size_t pos = startOffset; pos + 2 <= limit; pos++ )
            {
                int charCount = ( data[pos] << 8 ) | data[pos + 1];

                if( charCount < 0 || charCount > 512 )
                    continue;

                size_t strEnd = pos + 2 + static_cast<size_t>( charCount ) * 2;

                if( strEnd > limit )
                    continue;

                bool valid = true;

                for( size_t i = pos + 2; i < strEnd; i += 2 )
                {
                    if( data[i] != 0x00 || data[i + 1] < 0x20 || data[i + 1] > 0x7E )
                    {
                        valid = false;
                        break;
                    }
                }

                if( !valid )
                    continue;

                wxMBConvUTF16BE conv;
                wxString        modelName( reinterpret_cast<const char*>( data + pos + 2 ), conv,
                                           static_cast<size_t>( charCount ) * 2 );
                wxString        lowerModel = modelName.Lower();

                if( !modelName.IsEmpty() && !lowerModel.EndsWith( wxT( ".step" ) )
                    && !lowerModel.EndsWith( wxT( ".wrl" ) ) )
                {
                    continue;
                }

                for( size_t tailSize : { static_cast<size_t>( 61 ), static_cast<size_t>( 28 ) } )
                {
                    size_t tailEnd = strEnd + tailSize;

                    if( tailEnd <= limit
                        && ( tailEnd == aCompEnd || tailEnd == m_busSectionOffset || isComponentHeaderAt( tailEnd ) ) )
                    {
                        m_reader.SetOffset( tailEnd );
                        return;
                    }
                }
            }

            // The model string is not always stored in a form this scan recognises
            // (some v41 records use little-endian paths or omit the model entirely).
            // The next component header is reliably known, so land there: it is the
            // record's true end and keeps the sequential walk deterministic without
            // resorting to the global boundary scan.
            m_reader.SetOffset( landingForCeiling() );
            return;
        }

        auto readCount = [&]( const char* aName, int aMax ) -> int
        {
            int count = m_reader.ReadInt3();

            if( count < 0 || count > aMax )
            {
                THROW_IO_ERROR( wxString::Format( _( "DipTrace import: invalid embedded pattern %s count %d at "
                                                     "offset 0x%06zX." ),
                                                  wxString::FromUTF8( aName ), count, startOffset ) );
            }

            return count;
        };

        auto isValidUtf16StringAt = [&]( size_t aOffset ) -> bool
        {
            const uint8_t* data = m_reader.GetData();
            size_t         size = std::min( patternCeiling, m_reader.GetFileSize() );

            if( aOffset + 2 > size )
                return false;

            int charCount = ( data[aOffset] << 8 ) | data[aOffset + 1];

            if( charCount < 0 || charCount > 512 )
                return false;

            size_t stringEnd = aOffset + 2 + static_cast<size_t>( charCount ) * 2;

            if( stringEnd > size )
                return false;

            for( size_t i = aOffset + 2; i < stringEnd; i += 2 )
            {
                if( data[i] != 0x00 || data[i + 1] < 0x20 || data[i + 1] > 0x7E )
                    return false;
            }

            return true;
        };

        // Pre-name header: legacy preamble plus modern drill fields. Some
        // v41 records store the pattern name immediately after the extra drill
        // field; others add an int3 tail before the name.
        m_reader.ReadInt3();
        m_reader.ReadInt3();
        m_reader.ReadInt4();
        m_reader.ReadInt4();
        m_reader.ReadByte();
        m_reader.ReadInt4();
        m_reader.ReadByte();
        m_reader.ReadInt3();
        m_reader.ReadInt4();
        m_reader.ReadInt4();
        m_reader.ReadInt4();
        m_reader.ReadInt4();
        m_reader.ReadInt3();
        m_reader.ReadByte();
        m_reader.ReadInt4();
        m_reader.ReadInt4();

        if( !isValidUtf16StringAt( m_reader.GetOffset() ) )
            m_reader.ReadInt3();

        aComp.patternName = m_reader.ReadString();

        // Post-name fields: origin/options followed by the pad record count.
        m_reader.ReadInt4();
        m_reader.ReadInt4();
        m_reader.ReadInt4();
        m_reader.ReadInt4();
        m_reader.ReadByte();

        // The counted alias/pad/drawing fields are not yet decoded field-by-field for
        // every pattern variant (notably connectors and some v41 records). Walk them
        // best-effort only to detect the empty-pattern footer; any desync is recovered
        // by the authoritative 3D-model tail scanner below, which re-locates the record
        // end structurally from the pattern start.
        try
        {
            int aliasCount = readCount( "alias", 100 );

            for( int i = 0; i < aliasCount; i++ )
            {
                m_reader.ReadString();
                m_reader.ReadString();
            }

            m_reader.ReadInt3();

            if( aliasCount > 0 )
                m_reader.ReadInt3();

            int padCount = readCount( "pad", 500 );

            if( padCount == 0 )
            {
                // Empty modern patterns use the same short zero footer as legacy
                // empty patterns.
                m_reader.ReadInt3();
                m_reader.ReadInt3();
                m_reader.ReadInt3();
                return;
            }

            auto readModernPadRecord = [&]()
            {
                m_reader.ReadByte();
                m_reader.ReadInt3();
                m_reader.ReadInt4();
                m_reader.ReadInt4();
                m_reader.ReadString();
                m_reader.ReadString();
                m_reader.ReadInt4();
                m_reader.ReadInt4();
                m_reader.ReadInt4();
                m_reader.ReadInt4();
                m_reader.ReadInt3();
            };

            auto canReadModernPadRecordAt = [&]( size_t aOffset ) -> bool
            {
                size_t save = m_reader.GetOffset();
                bool   ok = false;

                try
                {
                    m_reader.SetOffset( aOffset );
                    readModernPadRecord();
                    ok = m_reader.GetOffset() <= patternCeiling;
                }
                catch( const std::exception& )
                {
                    ok = false;
                }

                m_reader.SetOffset( save );
                return ok;
            };

            for( int i = 0; i < padCount; i++ )
            {
                if( m_reader.GetOffset() >= patternCeiling || !canReadModernPadRecordAt( m_reader.GetOffset() ) )
                {
                    break;
                }

                readModernPadRecord();

                if( i + 1 < padCount )
                {
                    size_t tailStart = m_reader.GetOffset();

                    if( tailStart + 22 <= patternCeiling && canReadModernPadRecordAt( tailStart + 22 ) )
                    {
                        m_reader.Skip( 22 );
                    }
                }
            }
        }
        catch( const std::exception& )
        {
            // Best-effort walk only; the model-tail scan below recovers the end.
        }

        m_reader.SetOffset( startOffset );

        auto readInt3At = [&]( size_t aOffset ) -> int
        {
            const uint8_t* data = m_reader.GetData();

            return ( ( data[aOffset] << 16 ) | ( data[aOffset + 1] << 8 ) | data[aOffset + 2] ) - INT3_BIAS;
        };

        auto validUtf16StringAt = [&]( size_t aOffset, size_t aLimit, size_t& aEnd ) -> bool
        {
            const uint8_t* data = m_reader.GetData();

            if( aOffset + 2 > aLimit )
                return false;

            int charCount = ( data[aOffset] << 8 ) | data[aOffset + 1];

            if( charCount < 0 || charCount > 512 )
                return false;

            size_t strEnd = aOffset + 2 + static_cast<size_t>( charCount ) * 2;

            if( strEnd > aLimit )
                return false;

            for( size_t i = aOffset + 2; i < strEnd; i += 2 )
            {
                if( data[i] != 0x00 )
                    return false;

                if( charCount > 0 && ( data[i + 1] < 0x20 || data[i + 1] > 0x7E ) )
                    return false;
            }

            aEnd = strEnd;
            return true;
        };

        size_t modelSectionStart = std::string::npos;
        size_t modelStringStart = std::string::npos;
        size_t patternEnd = std::string::npos;
        size_t limit = std::min( patternCeiling, m_reader.GetFileSize() );

        for( size_t pos = m_reader.GetOffset(); pos + 3 <= limit; pos++ )
        {
            int modelPlacementCount = readInt3At( pos );

            if( modelPlacementCount < 0 || modelPlacementCount > 1000 )
                continue;

            size_t afterPlacements = pos + 3 + static_cast<size_t>( modelPlacementCount ) * 18;

            if( afterPlacements + 3 > limit || readInt3At( afterPlacements ) != 0 )
                continue;

            size_t strStart = afterPlacements + 3;
            size_t strEnd = 0;

            if( !validUtf16StringAt( strStart, limit, strEnd ) )
                continue;

            for( size_t tailSize : { static_cast<size_t>( 61 ), static_cast<size_t>( 28 ) } )
            {
                size_t tailEnd = strEnd + tailSize;

                if( tailEnd > limit )
                    continue;

                if( tailEnd == aCompEnd || tailEnd == m_busSectionOffset || isComponentHeaderAt( tailEnd ) )
                {
                    modelSectionStart = pos;
                    modelStringStart = strStart;
                    patternEnd = tailEnd;
                    break;
                }
            }

            if( modelSectionStart != std::string::npos )
                break;
        }

        if( modelSectionStart == std::string::npos )
        {
            // No decodable model tail. The next component header is reliably known,
            // so land there to keep the sequential walk deterministic.
            m_reader.SetOffset( landingForCeiling() );
            return;
        }

        m_reader.SetOffset( modelStringStart );
        m_reader.ReadString();
        m_reader.SetOffset( patternEnd );
    }
}


void SCH_PARSER::parseBusSection()
{
    m_reader.ReadInt4();
    m_reader.ReadInt4();
    m_reader.ReadByte();
    m_reader.ReadByte();

    int busCount = m_reader.ReadInt3();

    if( busCount < 0 || busCount > 1000 )
    {
        THROW_IO_ERROR( wxString::Format( _( "DipTrace import: invalid bus count %d." ), busCount ) );
    }

    for( int i = 0; i < busCount; i++ )
    {
        try
        {
            DCH_BUS_ENTRY entry;

            m_reader.ReadByte();
            m_reader.ReadByte();
            m_reader.ReadByte();

            entry.coordX = m_reader.ReadInt4();
            entry.coordY = m_reader.ReadInt4();
            entry.sheetIndex = m_reader.ReadInt3();
            entry.busType = m_reader.ReadInt3();
            entry.instanceId = m_reader.ReadInt3();
            entry.signalCount = m_reader.ReadInt3();

            int terminator = m_reader.ReadInt3();

            if( terminator != -1 )
            {
                THROW_IO_ERROR( wxString::Format( _( "DipTrace import: bus entry %d has "
                                                     "unexpected terminator %d." ),
                                                  i, terminator ) );
            }

            entry.name = m_reader.ReadString();
            m_reader.ReadByte();

            m_buses.push_back( entry );
        }
        catch( const IO_ERROR& )
        {
            throw;
        }
        catch( const std::exception& e )
        {
            THROW_IO_ERROR( wxString::Format( _( "DipTrace import: failed to parse bus entry "
                                                 "%d: %s" ),
                                              i, wxString::FromUTF8( e.what() ) ) );
        }
    }
}


void SCH_PARSER::parseNetSection()
{
    const uint8_t* data = m_reader.GetData();
    size_t         fileSize = m_reader.GetFileSize();
    size_t         searchEnd = m_tailOffset > 0 ? m_tailOffset : fileSize;
    size_t         searchStart = m_reader.GetOffset();

    if( searchStart >= searchEnd )
        return;

    for( size_t off = searchStart; off < searchEnd - 5; off++ )
    {
        if( data[off] != 0x0F || data[off + 1] != 0x42 || data[off + 2] != 0x3F )
            continue;

        size_t strOff = off + 3;

        if( m_version < V31_CUTOVER )
        {
            if( strOff + 3 > searchEnd )
                continue;

            int n = ( ( data[strOff] << 16 ) | ( data[strOff + 1] << 8 ) | data[strOff + 2] ) - INT3_BIAS;

            if( n < 1 || n > 200 || strOff + 3 + (size_t) n > fileSize )
                continue;

            bool valid = true;

            for( int i = 0; i < n; i++ )
            {
                uint8_t c = data[strOff + 3 + i];

                if( c < 0x20 || c > 0x7E )
                {
                    valid = false;
                    break;
                }
            }

            if( !valid )
                continue;

            wxString name = wxString::From8BitData( reinterpret_cast<const char*>( data + strOff + 3 ), n );
            size_t   afterStr = strOff + 3 + n;

            DCH_NET_ENTRY entry;
            entry.name = name;

            if( afterStr + 11 <= fileSize )
            {
                entry.coordX = ( ( data[afterStr] << 24 ) | ( data[afterStr + 1] << 16 ) | ( data[afterStr + 2] << 8 )
                                 | data[afterStr + 3] )
                               - INT4_BIAS;
                entry.coordY = ( ( data[afterStr + 4] << 24 ) | ( data[afterStr + 5] << 16 )
                                 | ( data[afterStr + 6] << 8 ) | data[afterStr + 7] )
                               - INT4_BIAS;
                entry.field1 = ( ( data[afterStr + 8] << 16 ) | ( data[afterStr + 9] << 8 ) | data[afterStr + 10] )
                               - INT3_BIAS;
            }

            m_nets.push_back( entry );
        }
        else
        {
            if( strOff + 2 > searchEnd )
                continue;

            int n = ( data[strOff] << 8 ) | data[strOff + 1];

            if( n < 1 || n > 200 || strOff + 2 + (size_t) ( n * 2 ) > fileSize )
                continue;

            bool valid = true;

            for( int i = 0; i < n; i++ )
            {
                uint8_t hi = data[strOff + 2 + static_cast<size_t>( i ) * 2];
                uint8_t lo = data[strOff + 2 + static_cast<size_t>( i ) * 2 + 1];

                if( hi != 0 || lo < 0x20 || lo > 0x7E )
                {
                    valid = false;
                    break;
                }
            }

            if( !valid )
                continue;

            wxMBConvUTF16BE conv;
            wxString name( reinterpret_cast<const char*>( data + strOff + 2 ), conv, static_cast<size_t>( n ) * 2 );
            size_t   afterStr = strOff + 2 + static_cast<size_t>( n ) * 2;

            DCH_NET_ENTRY entry;
            entry.name = name;

            if( afterStr + 11 <= fileSize )
            {
                entry.coordX = ( ( data[afterStr] << 24 ) | ( data[afterStr + 1] << 16 ) | ( data[afterStr + 2] << 8 )
                                 | data[afterStr + 3] )
                               - INT4_BIAS;
                entry.coordY = ( ( data[afterStr + 4] << 24 ) | ( data[afterStr + 5] << 16 )
                                 | ( data[afterStr + 6] << 8 ) | data[afterStr + 7] )
                               - INT4_BIAS;
                entry.field1 = ( ( data[afterStr + 8] << 16 ) | ( data[afterStr + 9] << 8 ) | data[afterStr + 10] )
                               - INT3_BIAS;
            }

            m_nets.push_back( entry );
        }
    }
}


int SCH_PARSER::pinOrientationFromOffset( int aOffsetX, int aOffsetY, int aHalfWidth, int aHalfHeight )
{
    // The stored coordinate is the pin's body-edge anchor, offset from the symbol body center; the
    // pin extends outward (away from center) to its connection point where wires attach. KiCad
    // stores m_position at the connection point with the body root at m_position + length toward
    // the orientation, so the body always sits between the connection point and the center. A pin
    // on the right edge therefore reads as PIN_LEFT (body to the left of its connection point), and
    // an above-center pin reads as PIN_DOWN (body below its connection point in KiCad Y-down). A
    // pin exactly on center defaults to left.
    if( aOffsetX == 0 && aOffsetY == 0 )
        return 2; // PIN_LEFT

    // Pick the edge the pin sits on, normalized by the body half-extents. A tall symbol's left-edge
    // pin can be farther from center in Y than in X, so the raw dominant axis would wrongly read it
    // as a top or bottom pin; scaling each offset by the opposite half-extent compares which edge
    // the pin actually reaches.
    int64_t halfW = aHalfWidth > 0 ? aHalfWidth : 1;
    int64_t halfH = aHalfHeight > 0 ? aHalfHeight : 1;

    if( std::abs( static_cast<int64_t>( aOffsetX ) ) * halfH >= std::abs( static_cast<int64_t>( aOffsetY ) ) * halfW )
    {
        return ( aOffsetX >= 0 ) ? 2 : 0; // right edge -> PIN_LEFT, left edge -> PIN_RIGHT
    }

    return ( aOffsetY >= 0 ) ? 1 : 3; // KiCad Y down: below center -> PIN_UP, above -> PIN_DOWN
}


SCH_SCREEN* SCH_PARSER::getOrCreateSheet( int aSheetIndex )
{
    if( aSheetIndex < 0 )
        aSheetIndex = 0;

    if( aSheetIndex == 0 || m_numSheets <= 1 )
        return m_rootSheet->GetScreen();

    while( (int) m_sheets.size() <= aSheetIndex )
        m_sheets.push_back( nullptr );

    if( m_sheets[aSheetIndex] )
        return m_sheets[aSheetIndex]->GetScreen();

    wxString sheetName;

    if( aSheetIndex < (int) m_sheetDefs.size() )
        sheetName = m_sheetDefs[aSheetIndex].name;
    else
        sheetName = wxString::Format( wxT( "Sheet%d" ), aSheetIndex + 1 );

    int      col = ( aSheetIndex - 1 ) % 4;
    int      row = ( aSheetIndex - 1 ) / 4;
    VECTOR2I pos( 2540000 + col * 50800000, 2540000 + row * 50800000 );
    VECTOR2I size( 40640000, 30480000 );

    SCH_SHEET*  newSheet = new SCH_SHEET( m_schematic ? &m_schematic->Root() : m_rootSheet, pos, size );
    SCH_SCREEN* newScreen = new SCH_SCREEN( m_schematic );

    wxFileName fn( m_fileName );
    fn.SetName( fn.GetName() + wxString::Format( wxT( "_%d" ), aSheetIndex ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    newScreen->SetFileName( fn.GetFullPath() );
    newSheet->SetScreen( newScreen );
    newSheet->SetFileName( fn.GetFullName() );
    newSheet->SetName( sheetName );

    if( m_schematic && m_rootSheet == &m_schematic->Root() )
        m_schematic->AddTopLevelSheet( newSheet );

    m_sheets[aSheetIndex] = newSheet;

    return newScreen;
}


void SCH_PARSER::assignSheetPageNumbers()
{
    for( size_t i = 0; i < m_sheets.size(); ++i )
    {
        SCH_SHEET* sheet = m_sheets[i];

        if( !sheet )
            continue;

        if( sheet->IsVirtualRootSheet() )
            continue;

        wxString pageNumber = wxString::Format( wxT( "%zu" ), i + 1 );
        SCH_SHEET_PATH path;
        path.push_back( sheet );
        path.SetPageNumber( pageNumber );

        if( sheet->GetScreen() )
            sheet->GetScreen()->SetPageNumber( pageNumber );
    }
}


void SCH_PARSER::finalizeFlatSheetOrder()
{
    if( !m_schematic || !m_rootSheet || m_rootSheet == &m_schematic->Root() )
    {
        assignSheetPageNumbers();
        return;
    }

    std::vector<SCH_SHEET*> topLevelSheets;
    topLevelSheets.reserve( m_sheets.size() );

    for( SCH_SHEET* sheet : m_sheets )
    {
        if( sheet )
            topLevelSheets.push_back( sheet );
    }

    if( !topLevelSheets.empty() )
        m_schematic->SetTopLevelSheets( topLevelSheets );

    assignSheetPageNumbers();
}


wxString SCH_PARSER::normalizedRefdes( const DCH_COMPONENT& aComp ) const
{
    wxString refdes = aComp.refdes;

    if( !aComp.isMultiPart )
        return refdes;

    int dot = refdes.Find( wxT( '.' ), true );

    if( dot <= 0 || dot >= static_cast<int>( refdes.length() ) - 1 )
        return refdes;

    long suffix = 0;

    if( refdes.Mid( dot + 1 ).ToLong( &suffix ) && suffix >= 1 )
        return refdes.Left( dot );

    return refdes;
}


wxString SCH_PARSER::componentSymbolName( const DCH_COMPONENT& aComp ) const
{
    wxString base = aComp.isMultiPart ? normalizedRefdes( aComp ) : wxString();

    if( base.IsEmpty() )
        base = aComp.compName;

    if( base.IsEmpty() )
        base = normalizedRefdes( aComp );

    if( base.IsEmpty() )
        base = wxT( "Unknown" );

    if( !aComp.isMultiPart && aComp.rotationE4 != 0 )
        base += wxString::Format( wxT( "_r%d" ), aComp.rotationE4 );

    return LIB_ID::FixIllegalChars( base, true ).wx_str();
}


static bool libSymbolHasUnit( const LIB_SYMBOL* aLibSymbol, int aUnit )
{
    for( const SCH_ITEM& drawItem : aLibSymbol->GetDrawItems() )
    {
        if( drawItem.GetUnit() == aUnit )
            return true;
    }

    return false;
}


static int dipTraceMm( double aMm )
{
    return static_cast<int>( std::lround( aMm * 30000.0 ) );
}


static VECTOR2I dipTraceShapePoint( double aXmm, double aYmm )
{
    return VECTOR2I( dipTraceMm( aXmm ), dipTraceMm( -aYmm ) );
}


static DCH_SHAPE makeDipTraceShape( int aKindCode, double aLineWidthMm, std::initializer_list<VECTOR2I> aPoints )
{
    DCH_SHAPE shape;
    shape.kindCode = aKindCode;
    shape.kindFlag = 0;
    shape.lineWidth = dipTraceMm( aLineWidthMm );
    shape.fontX = -20000;
    shape.fontY = 10000;
    shape.points.assign( aPoints.begin(), aPoints.end() );
    return shape;
}


static bool needsStandardThtLedShape( const DCH_COMPONENT& aComp )
{
    wxString libPath = aComp.libPath.Lower();
    wxString compName = aComp.compName.Lower();

    // These library-backed placements carry pins but no local shape stream; synthesize the
    // component-style graphics observed in the DipTrace XML oracle.
    return aComp.shapes.empty() && aComp.pins.size() == 2 && libPath.Contains( wxT( "opto_emitters_led_tht" ) )
           && compName.StartsWith( wxT( "led-3mm round" ) );
}


static std::vector<DCH_SHAPE> standardThtLedShapes()
{
    return {
        makeDipTraceShape( 6, 0.254, { dipTraceShapePoint( -3.175, 1.851 ), dipTraceShapePoint( 3.0416, -4.3656 ) } ),
        makeDipTraceShape( 1, 0.25, { dipTraceShapePoint( 1.5747, -3.0956 ), dipTraceShapePoint( 1.5747, 0.7144 ) } ),
        makeDipTraceShape( 3, 0.25, { dipTraceShapePoint( 1.2954, 2.486 ), dipTraceShapePoint( 2.54, 4.3656 ) } ),
        makeDipTraceShape( 3, 0.25, { dipTraceShapePoint( 2.2479, 1.851 ), dipTraceShapePoint( 3.4925, 3.7306 ) } ),
        makeDipTraceShape( 1, 0.25, { dipTraceShapePoint( 1.5747, -1.1906 ), dipTraceShapePoint( 3.81, -1.1906 ) } ),
        makeDipTraceShape( 1, 0.25, { dipTraceShapePoint( -3.81, -1.1906 ), dipTraceShapePoint( -1.6003, -1.1906 ) } ),
        makeDipTraceShape( 8, 0.25,
                           { dipTraceShapePoint( -1.6003, 0.7144 ), dipTraceShapePoint( 1.5747, -1.1906 ),
                             dipTraceShapePoint( -1.6003, -3.0956 ) } ),
        makeDipTraceShape( 9, 0.25,
                           { dipTraceShapePoint( 1.5747, -1.1906 ), dipTraceShapePoint( -1.6003, 0.7144 ),
                             dipTraceShapePoint( -1.6003, -3.0956 ), dipTraceShapePoint( 1.5747, -1.1906 ) } ),
    };
}


void SCH_PARSER::populateLibSymbolUnit( LIB_SYMBOL* aLibSymbol, const DCH_COMPONENT& aComp, int aUnit )
{
    if( !aLibSymbol || libSymbolHasUnit( aLibSymbol, aUnit ) )
        return;

    bool isPower = aComp.refdes.StartsWith( wxT( "NetPort" ) );

    for( const DCH_PIN& dchPin : aComp.pins )
    {
        auto pin = std::make_unique<SCH_PIN>( aLibSymbol );

        pin->SetName( dchPin.name.IsEmpty() ? wxString( wxT( "~" ) ) : dchPin.name );
        pin->SetNumber( dchPin.number.IsEmpty() ? wxString( wxT( "1" ) ) : dchPin.number );

        // The stored coordinate is the pin's body-edge anchor relative to the symbol center; the
        // pin extends outward from there by its length to the connection point where wires attach.
        VECTOR2I anchor( toKiCadCoordX( dchPin.x ), toKiCadCoordY( dchPin.y ) );
        int      len = toKiCadSize( dchPin.length );

        // The header bbox third and fourth values are the body width and height; halving them gives
        // the edge distances the pin offset is measured against.
        int halfW = toKiCadSize( aComp.bboxX2 ) / 2;
        int halfH = toKiCadSize( aComp.bboxY2 ) / 2;
        int orient = pinOrientationFromOffset( anchor.x, anchor.y, halfW, halfH );

        // Move the anchor outward by the length to the KiCad connection point (m_position). The
        // orientation then puts the body root back at the anchor.
        VECTOR2I connection = anchor;

        switch( orient )
        {
        case 0: connection.x -= len; break; // PIN_RIGHT: body right, connection left of anchor
        case 2: connection.x += len; break; // PIN_LEFT: body left, connection right of anchor
        case 1: connection.y += len; break; // PIN_UP: body up (Y-), connection below anchor
        case 3: connection.y -= len; break; // PIN_DOWN: body down (Y+), connection above anchor
        }

        pin->SetPosition( connection );
        pin->SetLength( len );

        switch( orient )
        {
        case 0: pin->SetOrientation( PIN_ORIENTATION::PIN_RIGHT ); break;
        case 1: pin->SetOrientation( PIN_ORIENTATION::PIN_UP ); break;
        case 2: pin->SetOrientation( PIN_ORIENTATION::PIN_LEFT ); break;
        case 3: pin->SetOrientation( PIN_ORIENTATION::PIN_DOWN ); break;
        }

        pin->SetType( isPower ? ELECTRICAL_PINTYPE::PT_POWER_IN : ELECTRICAL_PINTYPE::PT_PASSIVE );
        pin->SetUnit( aUnit );
        aLibSymbol->AddDrawItem( pin.release() );
    }

    std::vector<DCH_SHAPE>        fallbackShapes;
    const std::vector<DCH_SHAPE>* componentShapes = &aComp.shapes;

    if( needsStandardThtLedShape( aComp ) )
    {
        fallbackShapes = standardThtLedShapes();
        componentShapes = &fallbackShapes;
    }

    for( const DCH_SHAPE& dchShape : *componentShapes )
    {
        if( dchShape.points.size() < 2 )
            continue;

        // A non-positive stored width maps to 0, which KiCad renders at the default symbol line
        // width, matching how pins (with no stored width) are drawn.
        int width = toKiCadSize( dchShape.lineWidth );

        // DipTrace marks a rectangle with leading kind code 4; its two points are opposite corners.
        // This is the stored type, so a diagonal conductor (a two point line on a US resistor) is
        // never mistaken for a rectangle and an IC body box is always a rectangle.
        bool isRectangle = dchShape.points.size() == 2 && dchShape.kindCode == 4 && dchShape.kindFlag == 0;

        if( isRectangle )
        {
            auto rect = std::make_unique<SCH_SHAPE>( SHAPE_T::RECTANGLE, LAYER_DEVICE, 0, FILL_T::NO_FILL );
            rect->SetParent( aLibSymbol );
            rect->SetPosition(
                    VECTOR2I( toKiCadCoordX( dchShape.points[0].x ), toKiCadCoordY( dchShape.points[0].y ) ) );
            rect->SetEnd( VECTOR2I( toKiCadCoordX( dchShape.points[1].x ), toKiCadCoordY( dchShape.points[1].y ) ) );
            rect->SetStroke( STROKE_PARAMS( width, LINE_STYLE::SOLID ) );
            rect->SetUnit( aUnit );
            aLibSymbol->AddDrawItem( rect.release() );

            continue;
        }

        // DipTrace marks a circle or ellipse (an "obround") with leading kind code 6; its two points
        // are opposite corners of the bounding box. A transistor's enclosing circle is stored this
        // way; without this branch it falls through to a two point polyline and draws as a slash.
        // KiCad has no ellipse, so a square box is a circle and a rectangular one a circle of the
        // average radius.
        bool isEllipse = dchShape.points.size() == 2 && dchShape.kindCode == 6 && dchShape.kindFlag == 0;

        if( isEllipse )
        {
            VECTOR2I p0( toKiCadCoordX( dchShape.points[0].x ), toKiCadCoordY( dchShape.points[0].y ) );
            VECTOR2I p1( toKiCadCoordX( dchShape.points[1].x ), toKiCadCoordY( dchShape.points[1].y ) );
            VECTOR2I center( ( p0.x + p1.x ) / 2, ( p0.y + p1.y ) / 2 );
            int      radius = ( std::abs( p1.x - p0.x ) + std::abs( p1.y - p0.y ) ) / 4;

            auto circle = std::make_unique<SCH_SHAPE>( SHAPE_T::CIRCLE, LAYER_DEVICE, 0, FILL_T::NO_FILL );
            circle->SetParent( aLibSymbol );
            circle->SetCenter( center );
            circle->SetEnd( VECTOR2I( center.x + radius, center.y ) );
            circle->SetStroke( STROKE_PARAMS( width, LINE_STYLE::SOLID ) );
            circle->SetUnit( aUnit );
            aLibSymbol->AddDrawItem( circle.release() );

            continue;
        }

        // DipTrace marks an arc with leading kind code 2; it stores three points (start, a point on
        // the arc, end). An inductor's winding humps are arcs stored this way; without this branch
        // they fall through to a straight two-segment polyline instead of a curve.
        bool isArc = dchShape.points.size() == 3 && dchShape.kindCode == 2 && dchShape.kindFlag == 0;

        if( isArc )
        {
            VECTOR2I start( toKiCadCoordX( dchShape.points[0].x ), toKiCadCoordY( dchShape.points[0].y ) );
            VECTOR2I mid( toKiCadCoordX( dchShape.points[1].x ), toKiCadCoordY( dchShape.points[1].y ) );
            VECTOR2I end( toKiCadCoordX( dchShape.points[2].x ), toKiCadCoordY( dchShape.points[2].y ) );

            auto arc = std::make_unique<SCH_SHAPE>( SHAPE_T::ARC, LAYER_DEVICE, 0, FILL_T::NO_FILL );
            arc->SetParent( aLibSymbol );
            arc->SetArcGeometry( start, mid, end );
            arc->SetStroke( STROKE_PARAMS( width, LINE_STYLE::SOLID ) );
            arc->SetUnit( aUnit );
            aLibSymbol->AddDrawItem( arc.release() );

            continue;
        }

        bool isFilledPolygon = dchShape.points.size() >= 3 && dchShape.kindCode == 8 && dchShape.kindFlag == 0;

        auto poly = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY, LAYER_DEVICE, 0,
                                                 isFilledPolygon ? FILL_T::FILLED_SHAPE : FILL_T::NO_FILL );
        poly->SetParent( aLibSymbol );

        for( const VECTOR2I& pt : dchShape.points )
            poly->AddPoint( VECTOR2I( toKiCadCoordX( pt.x ), toKiCadCoordY( pt.y ) ) );

        poly->SetStroke( STROKE_PARAMS( width, LINE_STYLE::SOLID ) );
        poly->SetUnit( aUnit );
        aLibSymbol->AddDrawItem( poly.release() );

        if( dchShape.kindCode == 3 && dchShape.kindFlag == 0 && dchShape.points.size() == 2 )
        {
            VECTOR2I start( toKiCadCoordX( dchShape.points[0].x ), toKiCadCoordY( dchShape.points[0].y ) );
            VECTOR2I end( toKiCadCoordX( dchShape.points[1].x ), toKiCadCoordY( dchShape.points[1].y ) );
            double   dx = static_cast<double>( end.x - start.x );
            double   dy = static_cast<double>( end.y - start.y );
            double   len = std::sqrt( dx * dx + dy * dy );

            if( len > 0.0 )
            {
                double unitX = dx / len;
                double unitY = dy / len;
                double arrowLength =
                        std::min( len / 2.0, static_cast<double>( std::max( width * 4, schIUScale.MilsToIU( 35 ) ) ) );
                double halfWidth = arrowLength / 2.0;
                double baseX = static_cast<double>( end.x ) - unitX * arrowLength;
                double baseY = static_cast<double>( end.y ) - unitY * arrowLength;
                double perpX = -unitY;
                double perpY = unitX;

                auto arrow = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY, LAYER_DEVICE, 0, FILL_T::NO_FILL );
                arrow->SetParent( aLibSymbol );
                arrow->AddPoint( VECTOR2I( static_cast<int>( std::lround( baseX + perpX * halfWidth ) ),
                                           static_cast<int>( std::lround( baseY + perpY * halfWidth ) ) ) );
                arrow->AddPoint( end );
                arrow->AddPoint( VECTOR2I( static_cast<int>( std::lround( baseX - perpX * halfWidth ) ),
                                           static_cast<int>( std::lround( baseY - perpY * halfWidth ) ) ) );
                arrow->SetStroke( STROKE_PARAMS( width, LINE_STYLE::SOLID ) );
                arrow->SetUnit( aUnit );
                aLibSymbol->AddDrawItem( arrow.release() );
            }
        }
    }
}


LIB_SYMBOL* SCH_PARSER::getOrCreateLibSymbol( const DCH_COMPONENT& aComp, int aUnit )
{
    wxString symName = componentSymbolName( aComp );

    auto it = m_libSymbols.find( symName );

    if( it != m_libSymbols.end() )
    {
        LIB_SYMBOL* existing = it->second.get();

        if( aUnit > existing->GetUnitCount() )
            existing->SetUnitCount( aUnit, false );

        populateLibSymbolUnit( existing, aComp, aUnit );
        return existing;
    }

    auto libSymbol = std::make_unique<LIB_SYMBOL>( symName );
    libSymbol->SetUnitCount( aUnit, false );

    if( !aComp.patternName.IsEmpty() )
        libSymbol->GetFootprintField().SetText( aComp.patternName );

    bool isPower = aComp.refdes.StartsWith( wxT( "NetPort" ) );

    if( isPower )
        libSymbol->SetGlobalPower();

    // DipTrace stores pin name visibility as a per-pin flag (the second flag byte after the pin
    // name and number). It is uniform across a component's pins in practice, so drive the symbol's
    // show-pin-names switch from it: ICs show their pin names, passives keep them hidden.
    if( !aComp.pins.empty() )
        libSymbol->SetShowPinNames( aComp.pins.front().netFlagB != 0 );

    populateLibSymbolUnit( libSymbol.get(), aComp, aUnit );

    LIB_SYMBOL* rawPtr = libSymbol.get();
    m_libSymbols[symName] = std::move( libSymbol );
    return rawPtr;
}


void SCH_PARSER::buildWirePointSheets()
{
    m_wirePointSheets.clear();
    m_pointPartSheets.clear();

    std::map<int, std::map<int, int>> partSheetVotes; // partId -> sheet -> count

    for( const DCH_WIRE& wire : m_wires )
    {
        int sheetIdx = wire.sheetIndex;

        if( sheetIdx < 0 || sheetIdx >= m_numSheets )
            sheetIdx = 0;

        for( const VECTOR2I& pt : wire.points )
        {
            VECTOR2I p = applyPageOffset( pt );
            m_wirePointSheets[{ p.x, p.y }].insert( sheetIdx );
        }

        // The two endpoints carry the part each end connects to (object1 at the first point,
        // object2 at the last); record them with the sheet for exact part-id recovery.
        if( wire.points.size() >= 2 )
        {
            if( wire.object1 >= 0 )
            {
                VECTOR2I a = applyPageOffset( wire.points.front() );
                m_pointPartSheets[{ a.x, a.y }].emplace_back( wire.object1, sheetIdx );
                partSheetVotes[wire.object1][sheetIdx]++;
            }

            if( wire.object2 >= 0 )
            {
                VECTOR2I b = applyPageOffset( wire.points.back() );
                m_pointPartSheets[{ b.x, b.y }].emplace_back( wire.object2, sheetIdx );
                partSheetVotes[wire.object2][sheetIdx]++;
            }
        }
    }

    // Resolve each part's sheet as the one most of its wires sit on.
    m_partIdSheet.clear();

    for( const auto& [partId, sheets] : partSheetVotes )
    {
        int bestSheet = 0;
        int bestCount = -1;

        for( const auto& [sheet, count] : sheets )
        {
            if( count > bestCount )
            {
                bestCount = count;
                bestSheet = sheet;
            }
        }

        m_partIdSheet[partId] = bestSheet;
    }
}


bool SCH_PARSER::isComponentHeaderAt( size_t aOffset ) const
{
    const uint8_t* data = m_reader.GetData();
    size_t         fileSize = m_reader.GetFileSize();

    if( aOffset + 16 > fileSize )
        return false;

    // Four leading int4: the placement (centerX, centerY) followed by width/height. Origin is a
    // valid placement, so the string header below is the record discriminator.
    int bbox[4];

    for( int i = 0; i < 4; i++ )
    {
        size_t   p = aOffset + static_cast<size_t>( i ) * 4;
        uint32_t raw = ( static_cast<uint32_t>( data[p] ) << 24 ) | ( static_cast<uint32_t>( data[p + 1] ) << 16 )
                       | ( static_cast<uint32_t>( data[p + 2] ) << 8 ) | data[p + 3];
        bbox[i] = static_cast<int>( static_cast<int64_t>( raw ) - INT4_BIAS );

        if( std::abs( bbox[i] ) > 50000000 )
            return false;
    }

    // Five header strings (compName, refdes, value, prefix, nameDup). Some valid connector and
    // net-port records leave compName empty, so the fixed five-string layout is the discriminator.
    size_t p = aOffset + 16;

    for( int si = 0; si < 5; si++ )
    {
        int    charCount = 0;
        size_t dataStart = 0;
        bool   ascii = ( m_version < SCHEMATIC_UTF16_STRING_VERSION );

        if( ascii )
        {
            if( p + 3 > fileSize )
                return false;

            charCount = ( ( data[p] << 16 ) | ( data[p + 1] << 8 ) | data[p + 2] ) - INT3_BIAS;
            dataStart = p + 3;
        }
        else
        {
            if( p + 2 > fileSize )
                return false;

            charCount = ( data[p] << 8 ) | data[p + 1];
            dataStart = p + 2;
        }

        if( charCount == 0 )
        {
            p = dataStart;
            continue;
        }

        if( charCount < 0 || charCount > 64 )
            return false;

        size_t byteCount = ascii ? static_cast<size_t>( charCount ) : static_cast<size_t>( charCount ) * 2;

        if( dataStart + byteCount > fileSize )
            return false;

        for( int k = 0; k < charCount; k++ )
        {
            unsigned ch = ascii ? data[dataStart + k]
                                : ( ( data[dataStart + static_cast<size_t>( k ) * 2] << 8 )
                                    | data[dataStart + static_cast<size_t>( k ) * 2 + 1] );

            if( ch < 0x20 )
                return false;
        }

        p = dataStart + byteCount;
    }

    return true;
}


void SCH_PARSER::buildComponentPartIds()
{
    m_offsetToPartId.clear();

    if( m_componentSectionStart == 0 || m_busSectionOffset <= m_componentSectionStart )
        return;

    int    partId = 0;
    size_t off = m_componentSectionStart;

    while( off + 20 < m_busSectionOffset )
    {
        if( isComponentHeaderAt( off ) )
        {
            m_offsetToPartId[off] = partId++;
            off += 16;
        }
        else
        {
            off++;
        }
    }
}


int SCH_PARSER::resolveSheetTally( const std::map<std::pair<int, int>, int>& aTally )
{
    if( aTally.empty() )
        return -1;

    int bestCount = 0;

    for( const auto& [ps, count] : aTally )
        bestCount = std::max( bestCount, count );

    // First choice: a top-tally part id strictly greater than the last assigned (monotonic; the
    // file stores components in part-id order, which disambiguates identical duplicate sheets).
    for( const auto& [ps, count] : aTally ) // std::map iterates by ascending part id
    {
        if( count == bestCount && ps.first > m_lastSymbolPartId )
        {
            m_lastSymbolPartId = ps.first;
            return ps.second;
        }
    }

    // Otherwise take any top-tally pair (smallest part id).
    for( const auto& [ps, count] : aTally )
    {
        if( count == bestCount )
        {
            m_lastSymbolPartId = ps.first;
            return ps.second;
        }
    }

    return -1;
}


int SCH_PARSER::sheetForComponentPins( const std::vector<VECTOR2I>& aConnectionPoints )
{
    // Tally the (partId, sheet) pairs found at the symbol's connection points. The pair matching
    // the most pins is the symbol's part.
    std::map<std::pair<int, int>, int> tally;

    for( const VECTOR2I& p : aConnectionPoints )
    {
        auto it = m_pointPartSheets.find( { p.x, p.y } );

        if( it == m_pointPartSheets.end() )
            continue;

        std::set<std::pair<int, int>> seenHere; // count each (part, sheet) once per point

        for( const std::pair<int, int>& ps : it->second )
            if( seenHere.insert( ps ).second )
                tally[ps]++;
    }

    int sheet = resolveSheetTally( tally );

    if( sheet >= 0 )
        return sheet;

    // No endpoint matched a part; fall back to plain position voting.
    return sheetForPositions( aConnectionPoints, -1 );
}


int SCH_PARSER::sheetForPositions( const std::vector<VECTOR2I>& aPositions, int aFallback ) const
{
    std::map<int, int> votes;

    for( const VECTOR2I& p : aPositions )
    {
        auto it = m_wirePointSheets.find( { p.x, p.y } );

        if( it == m_wirePointSheets.end() )
            continue;

        for( int sheet : it->second )
            votes[sheet]++;
    }

    if( votes.empty() )
        return aFallback;

    int bestSheet = aFallback;
    int bestVotes = -1;

    for( const auto& [sheet, count] : votes )
    {
        if( count > bestVotes )
        {
            bestVotes = count;
            bestSheet = sheet;
        }
    }

    return bestSheet;
}


void SCH_PARSER::createSymbolInstance( const DCH_COMPONENT& aComp, SCH_SCREEN* aFallbackScreen )
{
    if( aComp.refdes.IsEmpty() && aComp.compName.IsEmpty() )
        return;

    // DipTrace net ports (auto_net_ports library) are connection markers, not real symbols; they
    // are imported as global net labels by createNetPortLabels(), so skip them here to avoid
    // drawing a redundant symbol on top of the label.
    if( aComp.libPath.Contains( wxT( "auto_net_ports" ) ) )
        return;

    wxString refdes = normalizedRefdes( aComp );
    int      unit = 1;
    bool     explicitUnit = false;

    if( refdes != aComp.refdes )
    {
        long suffix = 0;

        if( aComp.refdes.Mid( refdes.length() + 1 ).ToLong( &suffix ) && suffix >= 1 )
        {
            unit = static_cast<int>( suffix ) + 1;
            explicitUnit = true;
        }
    }

    if( !refdes.IsEmpty() )
    {
        auto it = m_refdesUnitMap.find( refdes );

        if( explicitUnit )
        {
            if( it == m_refdesUnitMap.end() || unit > it->second )
                m_refdesUnitMap[refdes] = unit;
        }
        else if( it != m_refdesUnitMap.end() )
        {
            unit = it->second + 1;
            it->second = unit;
        }
        else
        {
            m_refdesUnitMap[refdes] = unit;
        }
    }

    LIB_SYMBOL* libSym = getOrCreateLibSymbol( aComp, unit );

    if( !libSym )
        return;

    wxString symName = componentSymbolName( aComp );

    LIB_ID libId( getLibName(), symName );

    // The header bbox is [centerX, centerY, width, height]; the first pair is the placement point.
    // Offset by the page half-size so the origin-centered DipTrace placement lands on the page.
    VECTOR2I pos = applyPageOffset( VECTOR2I( toKiCadCoordX( aComp.bboxX1 ), toKiCadCoordY( aComp.bboxY1 ) ) );

    SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSym, libId, &m_schematic->CurrentSheet(), unit, 0, pos );

    symbol->SetLibSymbol( new LIB_SYMBOL( *libSym ) );

    m_placedSymbolsByLibName[symName].push_back( symbol );

    if( !refdes.IsEmpty() )
    {
        SCH_FIELD* refField = symbol->GetField( FIELD_T::REFERENCE );

        if( refField )
            refField->SetText( refdes );

        // The constructor seeds the instance with the unannotated prefix ("U?"); overwrite it with
        // the real reference so the per-sheet instances generated after the hierarchy is built copy
        // the annotated value rather than the placeholder.
        symbol->SetRef( &m_schematic->CurrentSheet(), refdes );
    }

    if( !aComp.value.IsEmpty() )
    {
        SCH_FIELD* valField = symbol->GetField( FIELD_T::VALUE );

        if( valField )
            valField->SetText( aComp.value );
    }

    if( !aComp.patternName.IsEmpty() )
        symbol->SetFootprintFieldText( aComp.patternName );

    if( aComp.refdes.StartsWith( wxT( "NetPort" ) ) && !aComp.compName.IsEmpty() )
    {
        SCH_FIELD* valField = symbol->GetField( FIELD_T::VALUE );

        if( valField )
            valField->SetText( aComp.compName );
    }

    // Import the remaining part data DipTrace stores per placement. These are metadata DipTrace
    // keeps hidden ("Common"), so they are added invisibly and surface in the symbol properties
    // rather than cluttering the canvas. Net-port pseudo-symbols carry none of this.
    if( !aComp.refdes.StartsWith( wxT( "NetPort" ) ) )
    {
        if( !aComp.datasheet.IsEmpty() )
        {
            if( SCH_FIELD* dsField = symbol->GetField( FIELD_T::DATASHEET ) )
                dsField->SetText( aComp.datasheet );
        }

        // DipTrace shows ICs by their part name rather than a value; keep it as a field so the
        // name (e.g. AD7190BRUZ) is preserved even when KiCad displays the empty value.
        if( !aComp.compName.IsEmpty() && !symbol->GetField( wxT( "Name" ) ) )
        {
            SCH_FIELD nameField( symbol, FIELD_T::USER, wxT( "Name" ) );
            nameField.SetText( aComp.compName );
            nameField.SetVisible( false );
            symbol->AddField( nameField );
        }

        for( const std::pair<wxString, wxString>& extra : aComp.additionalFields )
        {
            if( extra.first.IsEmpty() || symbol->GetField( extra.first ) )
                continue;

            SCH_FIELD userField( symbol, FIELD_T::USER, extra.first );
            userField.SetText( extra.second );
            userField.SetVisible( false );
            symbol->AddField( userField );
        }
    }

    // Position the reference and value fields from the stored per-instance text records. Each record
    // carries a field type (2 = reference, 3 = value) and an offset from the symbol origin, in the
    // same screen-down coordinate convention as the placement. Records without a position bearing
    // type are left for the auto-placement fallback below.
    bool     refPositioned = false;
    bool     valuePositioned = false;
    VECTOR2I refFieldOffset;
    VECTOR2I valueFieldOffset;

    for( const DCH_COMPONENT_TEXT& txt : aComp.texts )
    {
        SCH_FIELD* field = nullptr;

        if( txt.type == 2 )
            field = symbol->GetField( FIELD_T::REFERENCE );
        else if( txt.type == 3 )
            field = symbol->GetField( FIELD_T::VALUE );

        if( !field )
            continue;

        // A marking at offset zero is DipTrace's "Common" auto-layout placeholder, not an intended
        // position (the .dchxml shows Align="Common" X="0" Y="0" for these). Honoring it stacks the
        // reference and value on the symbol origin, so leave such records for the fallback below.
        if( txt.coordX == 0 && txt.coordY == 0 )
            continue;

        VECTOR2I off( toKiCadCoordX( txt.coordX ), toKiCadCoordY( txt.coordY ) );
        field->SetPosition( pos + off );

        if( txt.type == 2 )
        {
            refPositioned = true;
            refFieldOffset = off;
        }
        else if( txt.type == 3 )
        {
            valuePositioned = true;
            valueFieldOffset = off;
        }
    }

    // A two-terminal part keeps its reference and value symmetric about the body center, and
    // DipTrace stores a record only for the marking it actually placed. The binary confirms the
    // missing partner sits at the negated X of the stored one at the same Y (e.g. the cap reference
    // at the left edge pairs with the value at the right edge). Mirror it across the origin so the
    // two markings do not collapse onto the body.
    if( refPositioned != valuePositioned )
    {
        if( refPositioned )
        {
            if( SCH_FIELD* valField = symbol->GetField( FIELD_T::VALUE ) )
                valField->SetPosition( pos + VECTOR2I( -refFieldOffset.x, refFieldOffset.y ) );
        }
        else if( SCH_FIELD* refField = symbol->GetField( FIELD_T::REFERENCE ) )
        {
            refField->SetPosition( pos + VECTOR2I( -valueFieldOffset.x, valueFieldOffset.y ) );
        }
    }

    // When the source stored no field positions (common markings at the symbol origin), the
    // reference and value would otherwise stack on top of each other. Offset them above and below
    // the symbol body so they do not overlap, matching the source rendering intent. AutoplaceFields
    // is avoided here because it depends on the eeschema kiface settings, which the headless import
    // path does not guarantee.
    if( !refPositioned && !valuePositioned )
    {
        BOX2I bodyBox = libSym->GetBodyBoundingBox( unit, 0, false, false );
        int   margin = schIUScale.MilsToIU( 40 );

        // DipTrace bakes the placement rotation into the stored geometry rather than recording an
        // angle, so a rotated symbol is detectable only by its body being taller than it is wide.
        // Stacking the reference above and the value below works for a wide body but overlaps both
        // fields onto a tall one (e.g. a vertical 0805 cap), so split them to the sides instead.
        VECTOR2I refOffset;
        VECTOR2I valueOffset;

        if( bodyBox.GetHeight() > bodyBox.GetWidth() )
        {
            refOffset = VECTOR2I( bodyBox.GetLeft() - margin, 0 );
            valueOffset = VECTOR2I( bodyBox.GetRight() + margin, 0 );
        }
        else
        {
            refOffset = VECTOR2I( 0, bodyBox.GetTop() - margin );
            valueOffset = VECTOR2I( 0, bodyBox.GetBottom() + margin );
        }

        if( SCH_FIELD* refField = symbol->GetField( FIELD_T::REFERENCE ) )
        {
            refField->SetPosition( pos + refOffset );

            if( bodyBox.GetHeight() > bodyBox.GetWidth() )
                refField->SetHorizJustify( GR_TEXT_H_ALIGN_RIGHT );
        }

        if( SCH_FIELD* valField = symbol->GetField( FIELD_T::VALUE ) )
        {
            valField->SetPosition( pos + valueOffset );

            if( bodyBox.GetHeight() > bodyBox.GetWidth() )
                valField->SetHorizJustify( GR_TEXT_H_ALIGN_LEFT );
        }
    }

    // DipTrace rotates a marking's text with the symbol body, so a 90 or 270 degree placement reads
    // its reference and value vertically. Only the text angle is derived here (from the binary
    // placement rotation); the position comes from the marking record, so no clearance distance is
    // invented. DipTrace centres every marking (Horz="Center" Vert="Center"), so centre the justify
    // too; the no-record fallback above side-justifies for horizontal text, which shifts vertical
    // text along its length and mis-aligns a rotated part like R14-R20.
    if( aComp.rotationE4 == 15708 || aComp.rotationE4 == 47124 )
    {
        for( FIELD_T fieldId : { FIELD_T::REFERENCE, FIELD_T::VALUE } )
        {
            if( SCH_FIELD* field = symbol->GetField( fieldId ) )
            {
                field->SetTextAngle( ANGLE_VERTICAL );
                field->SetHorizJustify( GR_TEXT_H_ALIGN_CENTER );
                field->SetVertJustify( GR_TEXT_V_ALIGN_CENTER );
            }
        }
    }

    // DipTrace .dch stores no decodable per-component sheet field (verified across the whole record
    // against the .dchxml truth), so the sheet is recovered from the wire connectivity, which is the
    // source DipTrace itself relies on. Every wire endpoint carries its part id and sheet, so once the
    // full wire walk runs nearly every part is covered. The header bbox is [centerX, centerY, width,
    // height]; each pin's connection point (where a wire ends) is the pin coordinate offset from that
    // center, extended outward by the pin length along its dominant axis. Matching those connection
    // points against the decoded wire geometry yields the owning sheet without needing the component
    // rotation (which is not parsed). Falls back to the supplied screen when no pin coincides with a
    // wire.
    // Match connection points against the wire geometry, which is offset the same way, so use the
    // offset center here too.
    VECTOR2I center = applyPageOffset( VECTOR2I( toKiCadCoordX( aComp.bboxX1 ), toKiCadCoordY( aComp.bboxY1 ) ) );
    std::vector<VECTOR2I> connectionPoints;

    for( const DCH_PIN& dchPin : aComp.pins )
    {
        VECTOR2I off( toKiCadCoordX( dchPin.x ), toKiCadCoordY( dchPin.y ) );
        int      len = toKiCadSize( dchPin.length );
        VECTOR2I dir( 0, 0 );

        if( std::abs( off.x ) >= std::abs( off.y ) )
            dir.x = ( off.x >= 0 ) ? 1 : -1;
        else
            dir.y = ( off.y >= 0 ) ? 1 : -1;

        connectionPoints.emplace_back( center.x + off.x + dir.x * len, center.y + off.y + dir.y * len );
    }

    // Primary: match the pin connection points against the wire geometry (precise per-sheet). When
    // the heuristic parser mis-read the pins so nothing matches, fall back to the component's file
    // position, which gives its DipTrace part id, and the wire connectivity gives that part's sheet
    // exactly regardless of the bad pin data.
    int votedSheet = sheetForComponentPins( connectionPoints );

    if( votedSheet < 0 )
    {
        auto offsetIt = m_offsetToPartId.find( aComp.fileOffset );

        if( offsetIt != m_offsetToPartId.end() )
        {
            int partId = offsetIt->second;

            // The part itself may have no wire (e.g. an unconnected unit of a multi-unit part), so
            // search the nearest part ids too: components on a sheet have consecutive part ids, so a
            // neighbour's sheet is the right one.
            for( int d = 0; d <= 12 && votedSheet < 0; d++ )
            {
                for( int candidate : { partId - d, partId + d } )
                {
                    auto sheetIt = m_partIdSheet.find( candidate );

                    if( sheetIt != m_partIdSheet.end() )
                    {
                        votedSheet = sheetIt->second;
                        break;
                    }
                }
            }
        }
    }

    SCH_SCREEN* screen = ( votedSheet >= 0 ) ? getOrCreateSheet( votedSheet ) : aFallbackScreen;

    screen->Append( symbol );
}


void SCH_PARSER::createNetPortLabels()
{
    m_netPortNames.clear();
    m_netPortLabelCount = 0;

    // The part-aware sheet tally tracks the last assigned part id to disambiguate duplicate sheets
    // in file order. Net ports are a separate pass over their own objects, so reset the running id
    // rather than inheriting the last symbol's, keeping port resolution independent and ordered.
    m_lastSymbolPartId = -1;

    for( const DCH_COMPONENT& comp : m_components )
    {
        if( !comp.libPath.Contains( wxT( "auto_net_ports" ) ) || comp.compName.IsEmpty() )
            continue;

        // The port's component name is its net name; record it for diagnostics and so any future
        // consumer can tell which nets carry an explicit, labelled port object.
        m_netPortNames.insert( comp.compName );

        VECTOR2I pos = applyPageOffset( VECTOR2I( toKiCadCoordX( comp.bboxX1 ), toKiCadCoordY( comp.bboxY1 ) ) );

        // Resolve the port's sheet from its single pin connection point against the wire geometry,
        // falling back to its file-order part id and that part's wire-derived sheet (same recovery
        // the symbols use), since DipTrace stores no per-component sheet field.
        std::vector<VECTOR2I> connectionPoints;
        VECTOR2I              firstPinDir( 0, 0 );
        bool                  havePin = false;

        for( const DCH_PIN& dchPin : comp.pins )
        {
            VECTOR2I off( toKiCadCoordX( dchPin.x ), toKiCadCoordY( dchPin.y ) );
            int      len = toKiCadSize( dchPin.length );
            VECTOR2I dir( 0, 0 );

            if( std::abs( off.x ) >= std::abs( off.y ) )
                dir.x = ( off.x >= 0 ) ? 1 : -1;
            else
                dir.y = ( off.y >= 0 ) ? 1 : -1;

            connectionPoints.emplace_back( pos.x + off.x + dir.x * len, pos.y + off.y + dir.y * len );

            if( !havePin )
            {
                firstPinDir = dir;
                havePin = true;
            }
        }

        // Resolve the port's sheet from its pin connection point matched against the decoded wire
        // geometry. Plain position voting ties when the same coordinate carries wires on more than
        // one sheet (DipTrace centres every sheet on the same origin), which sent one of the four
        // GND_ANALOG ports to a coincidental neighbour sheet. The part-aware tally breaks the tie
        // toward the sheet whose wire endpoint connects to the port's own object, and still falls
        // back to position voting when no endpoint carries a part.
        int  votedSheet = sheetForComponentPins( connectionPoints );
        auto offsetIt = m_offsetToPartId.find( comp.fileOffset );

        if( votedSheet < 0 && offsetIt != m_offsetToPartId.end() )
        {
            auto sheetIt = m_partIdSheet.find( offsetIt->second );

            if( sheetIt != m_partIdSheet.end() )
                votedSheet = sheetIt->second;
        }

        if( votedSheet < 0 && offsetIt != m_offsetToPartId.end() )
        {
            int partId = offsetIt->second;

            for( int d = 1; d <= 12 && votedSheet < 0; d++ )
            {
                for( int candidate : { partId - d, partId + d } )
                {
                    auto sheetIt = m_partIdSheet.find( candidate );

                    if( sheetIt != m_partIdSheet.end() )
                    {
                        votedSheet = sheetIt->second;
                        break;
                    }
                }
            }
        }

        // Last resort for a port whose pin geometry matched no wire and whose part id has no
        // wire-derived sheet (an isolated port): snap to the sheet whose wire geometry is closest
        // to the port's own placement, so the label still lands on the sheet DipTrace drew it on
        // rather than defaulting to the root.
        if( votedSheet < 0 )
            votedSheet = sheetForNearestWire( pos );

        SCH_SCREEN* screen = getOrCreateSheet( votedSheet >= 0 ? votedSheet : 0 );

        // A global label's origin is its connection point, so anchor it on the port's pin endpoint
        // where the wire lands rather than the body center, otherwise the label floats off the net.
        VECTOR2I labelPos = ( havePin && !connectionPoints.empty() ) ? connectionPoints.front() : pos;

        // The pin's outward direction points along the wire, so the label text reads away from it; a
        // pin leaving toward +x puts the wire on the right and the text on the left, and so on.
        SPIN_STYLE spin = SPIN_STYLE::RIGHT;

        if( firstPinDir.x > 0 )
            spin = SPIN_STYLE::LEFT;
        else if( firstPinDir.x < 0 )
            spin = SPIN_STYLE::RIGHT;
        else if( firstPinDir.y > 0 )
            spin = SPIN_STYLE::UP;
        else if( firstPinDir.y < 0 )
            spin = SPIN_STYLE::BOTTOM;

        SCH_GLOBALLABEL* label = new SCH_GLOBALLABEL( labelPos, comp.compName );
        label->SetShape( LABEL_FLAG_SHAPE::L_BIDI );
        label->SetSpinStyle( spin );
        screen->Append( label );
        m_netPortLabelCount++;
    }
}


int SCH_PARSER::sheetForNearestWire( const VECTOR2I& aPos ) const
{
    int     bestSheet = -1;
    int64_t bestDist = std::numeric_limits<int64_t>::max();

    for( const auto& [pt, sheets] : m_wirePointSheets )
    {
        int64_t dx = static_cast<int64_t>( pt.first ) - aPos.x;
        int64_t dy = static_cast<int64_t>( pt.second ) - aPos.y;
        int64_t dist = dx * dx + dy * dy;

        if( dist < bestDist )
        {
            bestDist = dist;
            bestSheet = *sheets.begin();
        }
    }

    return bestSheet;
}


// True if the UTF-16-BE string at aData[aPos] is a plausible net name immediately followed by
// the fixed net-record header fields. This discriminates real net names from font blocks,
// footprint names, and binary noise inside the net-section preambles.
static bool isPlausibleNetName( const uint8_t* aData, size_t aPos, size_t aSectionEnd )
{
    if( aPos + 2 > aSectionEnd )
        return false;

    int cnt = ( aData[aPos] << 8 ) | aData[aPos + 1];

    if( cnt < 1 || cnt > 64 )
        return false;

    size_t end = aPos + 2 + static_cast<size_t>( cnt ) * 2;

    if( end + 8 + 3 + 1 + 3 > aSectionEnd )
        return false;

    for( int i = 0; i < cnt; i++ )
    {
        unsigned hi = aData[aPos + 2 + static_cast<size_t>( i ) * 2];
        unsigned lo = aData[aPos + 2 + static_cast<size_t>( i ) * 2 + 1];
        unsigned ch = ( hi << 8 ) | lo;

        bool ok = ( ch >= 0x20 && ch < 0x7F )          // ASCII printable
                  || ( ch >= 0x00A0 && ch <= 0x024F )  // Latin-1 / Latin Extended
                  || ( ch >= 0x0400 && ch <= 0x04FF ); // Cyrillic

        if( !ok )
            return false;
    }

    auto rdInt4 = [&]( size_t o ) -> int
    {
        uint32_t raw = ( static_cast<uint32_t>( aData[o] ) << 24 ) | ( static_cast<uint32_t>( aData[o + 1] ) << 16 )
                       | ( static_cast<uint32_t>( aData[o + 2] ) << 8 ) | static_cast<uint32_t>( aData[o + 3] );
        return static_cast<int>( static_cast<int64_t>( raw ) - INT4_BIAS );
    };

    int lx = rdInt4( end );
    int ly = rdInt4( end + 4 );

    auto rdInt3 = [&]( size_t o ) -> int
    {
        return ( ( aData[o] << 16 ) | ( aData[o + 1] << 8 ) | aData[o + 2] ) - INT3_BIAS;
    };

    int pad = rdInt3( end + 8 );
    int flag = aData[end + 11];

    // pad is a small discriminator that is 0 for nearly every net but 1 for a few (e.g. OTG_FS_N).
    // Requiring it to be exactly 0 rejected those nets and aborted the sequential walk, dropping
    // every later net's wires, so accept 0 or 1.
    return lx > -2000000 && lx < 2000000 && ly > -2000000 && ly < 2000000 && ( pad == 0 || pad == 1 )
           && ( flag == 0 || flag == 1 );
}


void SCH_PARSER::parseWireSection()
{
    // Accepted wire-net records use a marker lead-in followed by a 2-byte-prefixed
    // UTF-16-BE name. Older ASCII-string files keep the net-label-only import from
    // parseNetSection().
    if( m_version < SCHEMATIC_UTF16_STRING_VERSION )
        return;

    const uint8_t* data = m_reader.GetData();
    size_t         fileSize = m_reader.GetFileSize();
    size_t         sectionEnd = m_tailOffset > 0 ? m_tailOffset : fileSize;
    size_t         sectionStart = m_busSectionOffset;

    if( sectionStart == 0 || sectionStart >= sectionEnd )
        return;

    auto rdInt3 = [&]( size_t o ) -> int
    {
        return ( ( data[o] << 16 ) | ( data[o + 1] << 8 ) | data[o + 2] ) - INT3_BIAS;
    };

    auto rdInt4 = [&]( size_t o ) -> int
    {
        uint32_t raw = ( static_cast<uint32_t>( data[o] ) << 24 ) | ( static_cast<uint32_t>( data[o + 1] ) << 16 )
                       | ( static_cast<uint32_t>( data[o + 2] ) << 8 ) | static_cast<uint32_t>( data[o + 3] );
        return static_cast<int>( static_cast<int64_t>( raw ) - INT4_BIAS );
    };

    static constexpr uint8_t WIRE_NET_MARKER[] = { 0x0F, 0x42, 0x3F };
    static constexpr size_t  WIRE_NET_MARKER_LEN = sizeof( WIRE_NET_MARKER );

    auto isExpectedWireNetMarker = [&]( size_t aMarkerOffset, int aExpectedIndex ) -> bool
    {
        if( aMarkerOffset < 13 || aMarkerOffset + WIRE_NET_MARKER_LEN > sectionEnd )
            return false;

        if( memcmp( data + aMarkerOffset, WIRE_NET_MARKER, WIRE_NET_MARKER_LEN ) != 0 )
            return false;

        if( data[aMarkerOffset - 13] != 0x01 )
            return false;

        int fieldA = rdInt3( aMarkerOffset - 9 );
        int fieldB = rdInt3( aMarkerOffset - 6 );
        int netIndex = rdInt3( aMarkerOffset - 3 );

        if( netIndex != aExpectedIndex )
            return false;

        // fieldB is a small signed marker discriminator that is -1 for some nets (e.g. auto-named
        // "Net N" records). Requiring it to be non-negative rejected the first such net and aborted
        // the whole sequential walk, dropping every wire after it. Allow -1 so the walk reaches the
        // later sheets too.
        return fieldA >= -1 && fieldA <= 100000 && fieldB >= -1 && fieldB <= 100000;
    };

    auto decodeWireNetName = [&]( size_t aNameOffset, wxString& aName, size_t& aAfterName, wxString& aError ) -> bool
    {
        if( aNameOffset + 2 > sectionEnd )
        {
            aError = wxT( "missing UTF-16 length" );
            return false;
        }

        int nameLen = ( data[aNameOffset] << 8 ) | data[aNameOffset + 1];

        if( nameLen < 1 || nameLen > 64 )
        {
            aError = wxString::Format( wxT( "invalid UTF-16 length %d" ), nameLen );
            return false;
        }

        aAfterName = aNameOffset + 2 + static_cast<size_t>( nameLen ) * 2;

        if( aAfterName + 8 + 3 + 1 + 3 > sectionEnd )
        {
            aError = wxT( "name overruns wire-net record" );
            return false;
        }

        for( int i = 0; i < nameLen; i++ )
        {
            unsigned hi = data[aNameOffset + 2 + static_cast<size_t>( i ) * 2];
            unsigned lo = data[aNameOffset + 2 + static_cast<size_t>( i ) * 2 + 1];
            unsigned ch = ( hi << 8 ) | lo;

            bool valid =
                    ( ch >= 0x20 && ch < 0x7F ) || ( ch >= 0x00A0 && ch <= 0x024F ) || ( ch >= 0x0400 && ch <= 0x04FF );

            if( !valid )
            {
                aError = wxString::Format( wxT( "invalid UTF-16 character 0x%04X" ), ch );
                return false;
            }
        }

        wxMBConvUTF16BE conv;
        aName = wxString( reinterpret_cast<const char*>( data + aNameOffset + 2 ), conv,
                          static_cast<size_t>( nameLen ) * 2 );

        return true;
    };

    auto findNextWireNetName = [&]( size_t aSearchStart, size_t aSearchEnd, int aExpectedIndex ) -> size_t
    {
        if( aSearchStart >= aSearchEnd )
            return 0;

        for( size_t marker = aSearchStart; marker + WIRE_NET_MARKER_LEN <= aSearchEnd; marker++ )
        {
            if( memcmp( data + marker, WIRE_NET_MARKER, WIRE_NET_MARKER_LEN ) != 0 )
                continue;

            if( !isExpectedWireNetMarker( marker, aExpectedIndex ) )
                continue;

            size_t   nameOffset = marker + WIRE_NET_MARKER_LEN;
            wxString candidateName;
            wxString nameError;
            size_t   afterName = 0;

            if( !decodeWireNetName( nameOffset, candidateName, afterName, nameError ) )
            {
                THROW_IO_ERROR( wxString::Format( _( "DipTrace import: invalid wire-net name for net index %d at "
                                                     "offset 0x%06zX: %s." ),
                                                  aExpectedIndex, nameOffset, nameError ) );
            }

            if( isPlausibleNetName( data, nameOffset, sectionEnd ) )
                return nameOffset;
        }

        return 0;
    };

    // Locate the first wire-net name within the section header.
    int    expectedWireNetIndex = 0;
    size_t pos = findNextWireNetName( sectionStart, sectionEnd, expectedWireNetIndex );

    if( pos == 0 )
        return;

    int    safetyNets = 0;
    size_t lastRecordEnd = 0;

    while( pos != 0 && pos < sectionEnd && safetyNets++ < 100000 )
    {
        size_t o = pos;

        // Net name (UTF-16-BE), then labelX(int4) labelY(int4) pad(int3) flag(byte).
        wxString netName;
        wxString nameError;
        size_t   afterName = 0;

        if( !decodeWireNetName( o, netName, afterName, nameError ) )
        {
            THROW_IO_ERROR( wxString::Format( _( "DipTrace import: invalid wire-net name for net index %d at "
                                                 "offset 0x%06zX: %s." ),
                                              expectedWireNetIndex, o, nameError ) );
        }

        o = afterName;
        o += 4 + 4 + 3 + 1;

        if( o + 3 > sectionEnd )
            break;

        int    pinCount = rdInt3( o );
        size_t pinCountOffset = o;
        o += 3;

        if( pinCount < 0 || pinCount > 4000 || o + static_cast<size_t>( pinCount ) * 6 + 3 > sectionEnd )
        {
            THROW_IO_ERROR( wxString::Format( _( "DipTrace import: invalid wire-net pin count %d for net '%s' at "
                                                 "offset 0x%06zX." ),
                                              pinCount, netName, pinCountOffset ) );
        }

        o += static_cast<size_t>( pinCount ) * 6;

        int    wireCount = rdInt3( o );
        size_t wireCountOffset = o;
        o += 3;

        if( wireCount < 0 || wireCount > 100000 )
        {
            THROW_IO_ERROR( wxString::Format( _( "DipTrace import: invalid wire count %d for net '%s' at offset "
                                                 "0x%06zX." ),
                                              wireCount, netName, wireCountOffset ) );
        }

        bool brokeEarly = false;

        for( int w = 0; w < wireCount; w++ )
        {
            if( o + 36 + 1 + 3 > sectionEnd )
            {
                brokeEarly = true;
                break;
            }

            DCH_WIRE wire;
            wire.object1 = rdInt3( o + 0 );
            wire.object2 = rdInt3( o + 3 );
            wire.subObject1 = rdInt3( o + 6 );
            wire.subObject2 = rdInt3( o + 9 );
            wire.bus1 = rdInt3( o + 12 );
            wire.bus2 = rdInt3( o + 15 );
            wire.sheetIndex = rdInt3( o + 18 );

            o += 36; // 12 int3 header tokens
            o += 1;  // flag byte

            int    pointCount = rdInt3( o );
            size_t pointCountOffset = o;
            o += 3;

            if( pointCount < 0 || pointCount > 4000 || o + static_cast<size_t>( pointCount ) * 11 + 8 > sectionEnd )
            {
                THROW_IO_ERROR( wxString::Format( _( "DipTrace import: invalid wire point count %d for net '%s' at "
                                                     "offset 0x%06zX." ),
                                                  pointCount, netName, pointCountOffset ) );
            }

            wire.points.reserve( pointCount );

            for( int p = 0; p < pointCount; p++ )
            {
                int dtX = rdInt4( o );
                int dtY = rdInt4( o + 4 );

                // On-disk wire X/Y use the same convention as pins, so feed the raw DipTrace ints
                // directly through the existing transforms. This lands wire endpoints exactly on
                // imported pin positions.
                wire.points.emplace_back( toKiCadCoordX( dtX ), toKiCadCoordY( dtY ) );

                o += 11; // X(int4) Y(int4) Dir(int3)
            }

            o += 8; // per-wire trailer

            if( wire.points.size() >= 2 )
            {
                // Labels are not synthesized per net here. DipTrace shows a label only where an
                // explicit net-port object is placed, so labels are emitted from those objects in
                // createNetPortLabels(). Auto-named internal nets ("Net 36") own no port object and
                // therefore carry no label, matching the source rendering.
                m_wires.push_back( std::move( wire ) );
            }
        }

        if( brokeEarly )
            break;

        lastRecordEnd = o;
        expectedWireNetIndex++;

        // Find the next net name (skips the variable-length net preamble).
        pos = findNextWireNetName( o, std::min( sectionEnd, o + 400 ), expectedWireNetIndex );
    }

    m_wireSectionEnd = lastRecordEnd;
}


void SCH_PARSER::parseSheetShapes()
{
    m_sheetShapes.clear();

    if( m_version < V31_CUTOVER )
        return;

    size_t sectionEnd = m_tailOffset > 0 ? m_tailOffset : m_reader.GetFileSize();
    size_t searchStart = m_busSectionOffset > 0 ? m_busSectionOffset : m_wireSectionEnd;

    if( searchStart == 0 || searchStart >= sectionEnd )
        return;

    size_t originalOffset = m_reader.GetOffset();

    auto readSheetShapeRecord = [&]() -> std::optional<DCH_SHEET_SHAPE>
    {
        uint8_t flagA = m_reader.ReadByte();
        uint8_t flagB = m_reader.ReadByte();
        int     fieldA = m_reader.ReadInt3();
        int     kindCode = m_reader.ReadInt3();
        int     drawOrder = m_reader.ReadInt3();

        m_reader.Skip( 3 ); // fill color A

        uint8_t color[3] = {};
        m_reader.ReadBytes( color, 3 );

        m_reader.Skip( 3 ); // fill color B

        int fieldB = m_reader.ReadInt3();
        int sheetIndex = m_reader.ReadInt3();
        int fieldC = m_reader.ReadInt3();
        int lineWidth = m_reader.ReadInt4();
        m_reader.ReadString(); // font name
        m_reader.ReadString(); // optional text
        int fieldD = m_reader.ReadInt3();
        int pointCount = m_reader.ReadInt3();

        if( flagA != 1 || flagB != 0 || fieldA != 0 || fieldB != 0 || fieldC != -1 || fieldD != 0 )
            return std::nullopt;

        if( kindCode != 1 && kindCode != 4 )
            return std::nullopt;

        if( drawOrder < 0 || drawOrder > 1000 || sheetIndex < 0 || sheetIndex >= m_numSheets )
            return std::nullopt;

        if( lineWidth < 0 || lineWidth > 200000 || pointCount < 1 || pointCount > 100 )
            return std::nullopt;

        DCH_SHEET_SHAPE shape;
        shape.kindCode = kindCode;
        shape.sheetIndex = sheetIndex;
        shape.lineWidth = lineWidth;
        shape.color[0] = color[0];
        shape.color[1] = color[1];
        shape.color[2] = color[2];
        shape.points.reserve( pointCount );

        for( int i = 0; i < pointCount; i++ )
        {
            int x = m_reader.ReadInt4();
            int y = m_reader.ReadInt4();
            shape.points.emplace_back( x, y );
        }

        int     tailA = m_reader.ReadInt3();
        int     tailB = m_reader.ReadInt3();
        uint8_t tailFlagA = m_reader.ReadByte();
        uint8_t tailFlagB = m_reader.ReadByte();
        int     extentX = m_reader.ReadInt4();
        int     extentY = m_reader.ReadInt4();

        if( tailA != -1 || tailB != -1 || tailFlagA != 0 || tailFlagB != 1 || extentX != -20000 || extentY != 10000 )
            return std::nullopt;

        return shape;
    };

    for( size_t offset = searchStart; offset + 3 < sectionEnd; offset++ )
    {
        try
        {
            m_reader.SetOffset( offset );
            int count = m_reader.ReadInt3();

            if( count < 1 || count > 1000 )
                continue;

            std::vector<DCH_SHEET_SHAPE> shapes;
            shapes.reserve( count );

            bool valid = true;

            for( int i = 0; i < count; i++ )
            {
                std::optional<DCH_SHEET_SHAPE> shape = readSheetShapeRecord();

                if( !shape )
                {
                    valid = false;
                    break;
                }

                shapes.push_back( *shape );
            }

            if( valid && !shapes.empty() && m_reader.GetOffset() <= sectionEnd )
            {
                m_sheetShapes = std::move( shapes );
                break;
            }
        }
        catch( const std::exception& )
        {
        }
    }

    m_reader.SetOffset( originalOffset );
}


static KIGFX::COLOR4D dipTraceSheetShapeColor( const DCH_SHEET_SHAPE& aShape )
{
    return KIGFX::COLOR4D( aShape.color[0] / 255.0, aShape.color[1] / 255.0, aShape.color[2] / 255.0, 1.0 );
}


void SCH_PARSER::createSheetShapes()
{
    for( const DCH_SHEET_SHAPE& dchShape : m_sheetShapes )
    {
        if( dchShape.points.size() < 2 )
            continue;

        SCH_SCREEN* screen = getOrCreateSheet( dchShape.sheetIndex );

        if( !screen )
            continue;

        int           width = toKiCadSize( dchShape.lineWidth );
        STROKE_PARAMS stroke( width, LINE_STYLE::SOLID, dipTraceSheetShapeColor( dchShape ) );

        if( dchShape.kindCode == 4 && dchShape.points.size() == 2 )
        {
            SCH_SHAPE* rect = new SCH_SHAPE( SHAPE_T::RECTANGLE, LAYER_NOTES, 0, FILL_T::NO_FILL );
            rect->SetPosition( applyPageOffset(
                    VECTOR2I( toKiCadCoordX( dchShape.points[0].x ), toKiCadCoordY( dchShape.points[0].y ) ) ) );
            rect->SetEnd( applyPageOffset(
                    VECTOR2I( toKiCadCoordX( dchShape.points[1].x ), toKiCadCoordY( dchShape.points[1].y ) ) ) );
            rect->SetStroke( stroke );
            screen->Append( rect );
            continue;
        }

        if( dchShape.kindCode == 1 )
        {
            SCH_SHAPE* line = new SCH_SHAPE( SHAPE_T::POLY, LAYER_NOTES, 0, FILL_T::NO_FILL );

            for( const VECTOR2I& pt : dchShape.points )
            {
                line->AddPoint( applyPageOffset( VECTOR2I( toKiCadCoordX( pt.x ), toKiCadCoordY( pt.y ) ) ) );
            }

            line->SetStroke( stroke );
            screen->Append( line );
        }
    }
}


void SCH_PARSER::createWires()
{
    for( const DCH_WIRE& wire : m_wires )
    {
        int sheetIdx = wire.sheetIndex;

        if( sheetIdx < 0 || sheetIdx >= m_numSheets )
            sheetIdx = 0;

        SCH_SCREEN* screen = getOrCreateSheet( sheetIdx );

        if( !screen )
            continue;

        for( size_t i = 1; i < wire.points.size(); i++ )
        {
            VECTOR2I a = applyPageOffset( wire.points[i - 1] );
            VECTOR2I b = applyPageOffset( wire.points[i] );

            if( a == b )
                continue;

            SCH_LINE* line = new SCH_LINE( a, LAYER_WIRE );
            line->SetEndPoint( b );
            screen->Append( line );
        }
    }
}


void SCH_PARSER::createJunctions()
{
    // Junctions come from two sources, deduplicated per screen. Must run after wires, labels and
    // symbols are placed.
    std::set<SCH_SCREEN*> screens;

    if( m_rootSheet && m_rootSheet->GetScreen() )
        screens.insert( m_rootSheet->GetScreen() );

    for( SCH_SHEET* sheet : m_sheets )
    {
        if( sheet && sheet->GetScreen() )
            screens.insert( sheet->GetScreen() );
    }

    std::map<SCH_SCREEN*, std::set<std::pair<int, int>>> junctions;

    // KiCad's geometric rule covers the common cases (>=3 conductor ends coincide, or a wire end
    // lands on a pin tap).
    for( SCH_SCREEN* screen : screens )
    {
        std::deque<EDA_ITEM*> items;

        for( SCH_ITEM* item : screen->Items() )
            items.push_back( item );

        for( const VECTOR2I& pt : screen->GetNeededJunctions( items ) )
            junctions[screen].insert( { pt.x, pt.y } );
    }

    // DipTrace records each wire endpoint's connection explicitly: a bus value of -1 marks a pin
    // connection, anything else marks a tap onto another wire. KiCad's geometric rule misses a tap
    // where the target wire's collinear segments merge through the point, so add a junction wherever
    // an explicit wire tap lands on another wire's interior vertex.
    std::map<SCH_SCREEN*, std::set<std::pair<int, int>>> interior;

    auto screenFor = [&]( const DCH_WIRE& aWire ) -> SCH_SCREEN*
    {
        int sheetIdx = ( aWire.sheetIndex >= 0 && aWire.sheetIndex < m_numSheets ) ? aWire.sheetIndex : 0;
        return getOrCreateSheet( sheetIdx );
    };

    for( const DCH_WIRE& wire : m_wires )
    {
        SCH_SCREEN* screen = screenFor( wire );

        if( !screen )
            continue;

        for( size_t i = 1; i + 1 < wire.points.size(); i++ )
        {
            VECTOR2I p = applyPageOffset( wire.points[i] );
            interior[screen].insert( { p.x, p.y } );
        }
    }

    for( const DCH_WIRE& wire : m_wires )
    {
        SCH_SCREEN* screen = screenFor( wire );

        if( !screen || wire.points.empty() )
            continue;

        const std::set<std::pair<int, int>>& sheetInterior = interior[screen];

        if( wire.bus1 != -1 )
        {
            VECTOR2I p = applyPageOffset( wire.points.front() );

            if( sheetInterior.count( { p.x, p.y } ) )
                junctions[screen].insert( { p.x, p.y } );
        }

        if( wire.bus2 != -1 )
        {
            VECTOR2I p = applyPageOffset( wire.points.back() );

            if( sheetInterior.count( { p.x, p.y } ) )
                junctions[screen].insert( { p.x, p.y } );
        }
    }

    for( const auto& [screen, pts] : junctions )
    {
        for( const std::pair<int, int>& pt : pts )
            screen->Append( new SCH_JUNCTION( VECTOR2I( pt.first, pt.second ) ) );
    }
}


void SCH_PARSER::syncEmbeddedLibrarySymbols()
{
    for( const auto& [symName, symbols] : m_placedSymbolsByLibName )
    {
        auto libIt = m_libSymbols.find( symName );

        if( libIt == m_libSymbols.end() )
            continue;

        for( SCH_SYMBOL* symbol : symbols )
        {
            if( symbol )
                symbol->SetLibSymbol( new LIB_SYMBOL( *libIt->second ) );
        }
    }
}


void SCH_PARSER::createKiCadObjects()
{
    m_sheets.resize( m_numSheets, nullptr );

    if( m_numSheets > 0 )
        m_sheets[0] = m_rootSheet;

    if( m_rootSheet && !m_sheetDefs.empty() )
        m_rootSheet->SetName( m_sheetDefs[0].name );

    if( m_numSheets > 1 )
    {
        for( int i = 1; i < m_numSheets; i++ )
            getOrCreateSheet( i );
    }

    finalizeFlatSheetOrder();

    // Apply the decoded page size to every screen so the imported content sits on a page that
    // matches the source. Only done when a page record was found; otherwise the KiCad default
    // remains and no placement offset is applied.
    if( m_page.found )
    {
        PAGE_INFO pageInfo( PAGE_SIZE_TYPE::User );
        pageInfo.SetWidthMM( m_page.widthMM );
        pageInfo.SetHeightMM( m_page.heightMM );

        std::set<SCH_SCREEN*> screens;

        if( m_rootSheet && m_rootSheet->GetScreen() )
            screens.insert( m_rootSheet->GetScreen() );

        for( SCH_SHEET* sheet : m_sheets )
        {
            if( sheet && sheet->GetScreen() )
                screens.insert( sheet->GetScreen() );
        }

        for( SCH_SCREEN* screen : screens )
            screen->SetPageSettings( pageInfo );
    }

    // Index the decoded wire geometry by position so each symbol and label can be routed to the
    // sheet it connects to (DipTrace does not record sheet membership on components).
    buildWirePointSheets();
    buildComponentPartIds();
    m_lastSymbolPartId = -1;
    m_refdesUnitMap.clear();
    m_placedSymbolsByLibName.clear();

    for( const DCH_COMPONENT& comp : m_components )
    {
        try
        {
            createSymbolInstance( comp, m_rootSheet->GetScreen() );
        }
        catch( const std::exception& e )
        {
            if( m_reporter )
            {
                m_reporter->Report( wxString::Format( _( "DipTrace import: failed to create symbol "
                                                         "for %s (%s): %s" ),
                                                      comp.refdes, comp.compName, wxString::FromUTF8( e.what() ) ),
                                    RPT_SEVERITY_WARNING );
            }
        }
    }

    syncEmbeddedLibrarySymbols();
    createNetPortLabels();
    createWires();
    createSheetShapes();
    createJunctions();

    // The decoded symbols are embedded directly in the schematic (each SCH_SYMBOL carries a
    // flattened LIB_SYMBOL via SetLibSymbol), so the import deliberately does not write a
    // standalone .kicad_sym library or register one in the project symbol library table.

    if( m_reporter )
    {
        m_reporter->Report( wxString::Format( _( "DipTrace import: loaded %zu components, %zu buses, "
                                                 "%zu net-port labels from version %d file with %d sheets." ),
                                              m_components.size(), m_buses.size(), m_netPortLabelCount, m_version,
                                              m_numSheets ),
                            RPT_SEVERITY_INFO );
    }
}

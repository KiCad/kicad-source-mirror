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
#include <cstdlib>
#include <cstring>
#include <deque>
#include <memory>
#include <set>

#include <wx/filename.h>
#include <wx/log.h>

#include <lib_id.h>
#include <lib_symbol.h>
#include <progress_reporter.h>
#include <project.h>
#include <project_sch.h>
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
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <libraries/symbol_library_adapter.h>


using namespace DIPTRACE;


/// Structural layout version threshold for the .dch schematic format.
///
/// Versions below V31_CUTOVER (< 34) have different field layouts in several
/// sections: extra int3 fields, different padding, and alternate single-part
/// encoding.  This controls structural byte-layout differences only.
///
/// This is independent of LEGACY_STRING_VERSION (37) in the shared binary
/// reader, which controls string encoding (ASCII vs UTF-16-BE).  A file can
/// be above V31_CUTOVER (modern layout) but still at or below
/// LEGACY_STRING_VERSION (legacy strings), e.g. versions 34-37.
static constexpr int V31_CUTOVER = 34;


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
    return static_cast<int>( static_cast<int64_t>( aDipTraceCoord ) * 100 / 3 );
}


int SCH_PARSER::toKiCadCoordY( int aDipTraceCoord )
{
    return static_cast<int>( static_cast<int64_t>( -aDipTraceCoord ) * 100 / 3 );
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

        if( m_version < 1 )
        {
            THROW_IO_ERROR( wxString::Format(
                    _( "Unsupported DipTrace schematic format version %d." ),
                    m_version ) );
        }

        parseSheetDefinitions();
        parseDisplaySettings();
        parseTextStyles();
        parsePreComponentSettings();

        size_t compSectionStart = m_reader.GetOffset();
        m_busSectionOffset = findBusSection( compSectionStart );
        m_tailOffset = findTailStart();

        if( m_busSectionOffset == 0 || m_busSectionOffset >= m_reader.GetFileSize() )
        {
            if( m_reporter )
            {
                m_reporter->Report( _( "DipTrace import: could not locate bus section; "
                                       "using tail offset as boundary." ),
                                    RPT_SEVERITY_WARNING );
            }

            m_busSectionOffset = m_tailOffset;
        }

        parseComponents( m_busSectionOffset );

        if( m_busSectionOffset > 0 && m_busSectionOffset < m_reader.GetFileSize() )
        {
            m_reader.SetOffset( m_busSectionOffset );
            parseBusSection();
        }

        parseNetSection();
        parseWireSection();
    }
    catch( const IO_ERROR& )
    {
        throw;
    }
    catch( const std::exception& e )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "DipTrace import: unexpected error at offset 0x%06zX: %s" ),
                m_reader.GetOffset(), wxString::FromUTF8( e.what() ) ) );
    }

    createKiCadObjects();
}


void SCH_PARSER::parseHeader()
{
    uint8_t magicLen = m_reader.ReadByte();

    if( magicLen != 7 && magicLen != 11 )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "Invalid DipTrace schematic magic length: %d (expected 7 or 11)." ),
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
        std::string magicSuffix( reinterpret_cast<const char*>( magicBuf.data() + 7 ),
                                 magicLen - 7 );
        int         parsedMajor = 0;
        int         parsedMinor = 0;
        size_t      dotPos = magicSuffix.find( '.' );

        if( dotPos != std::string::npos )
        {
            std::string major = magicSuffix.substr( 0, dotPos );
            std::string minor = magicSuffix.substr( dotPos + 1 );

            bool validMajor = !major.empty() && std::all_of( major.begin(), major.end(),
                                                             []( unsigned char c ) {
                                                                 return std::isdigit( c ) != 0;
                                                             } );
            bool validMinor = !minor.empty() && std::all_of( minor.begin(), minor.end(),
                                                             []( unsigned char c ) {
                                                                 return std::isdigit( c ) != 0;
                                                             } );

            if( validMajor )
                parsedMajor = std::stoi( major );

            if( validMinor )
            {
                parsedMinor = std::stoi( minor );
            }
        }

        if( parsedMajor <= 0 || parsedMajor > 9 )
            parsedMajor = 2;

        if( parsedMinor <= 0 || parsedMinor > LEGACY_STRING_VERSION )
            parsedMinor = 33;

        m_magicMajor = parsedMajor;
        m_version = parsedMinor;
    }

    m_reader.ReadInt4();  // field_0B
    m_reader.ReadInt3();  // field_0F
    m_reader.ReadInt3();  // field_12
    m_reader.ReadInt3();  // field_15
    m_numSheets = m_reader.ReadInt3();

    if( m_numSheets < 0 || m_numSheets > 100 )
    {
        THROW_IO_ERROR( wxString::Format(
                _( "Invalid DipTrace schematic sheet count: %d." ), m_numSheets ) );
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

        uint32_t extraChars = ( extraHdr[0] << 24 ) | ( extraHdr[1] << 16 ) | ( extraHdr[2] << 8 )
                              | extraHdr[3];

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
        THROW_IO_ERROR( wxString::Format(
                _( "Invalid text style count: %d." ), numStyles ) );
    }

    for( int i = 0; i < numStyles; i++ )
    {
        m_reader.ReadString();
        m_reader.ReadInt3();
        m_reader.ReadInt4();
        m_reader.ReadInt4();

        // v38+ (UTF-16) files carry a trailing int3 per text style. In older
        // single-style files this same byte was historically consumed as the
        // leading "pad" int3 in parsePreComponentSettings(); reading it here is
        // byte-identical for one style but keeps multi-style v46 files in sync.
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
        if( m_reporter )
        {
            m_reporter->Report( wxString::Format(
                                        _( "DipTrace import: invalid component count %d; "
                                           "falling back to boundary scan." ),
                                        m_componentCount ),
                                RPT_SEVERITY_WARNING );
        }

        m_componentCount = -1;
    }
}


size_t SCH_PARSER::findBusSection( size_t aSearchStart ) const
{
    const uint8_t* data     = m_reader.GetData();
    size_t         fileSize = m_reader.GetFileSize();

    static const uint8_t marker[] = { 0x3B, 0x9A, 0xF1, 0x10, 0x3B, 0x9A, 0xF1, 0x10,
                                      0x00, 0x00 };
    static constexpr size_t markerLen = sizeof( marker );

    if( aSearchStart + markerLen + 3 >= fileSize )
        return 0;

    for( size_t off = aSearchStart; off < fileSize - markerLen - 3; off++ )
    {
        if( memcmp( data + off, marker, markerLen ) == 0 )
        {
            int count = ( ( data[off + 10] << 16 ) | ( data[off + 11] << 8 )
                          | data[off + 12] )
                        - INT3_BIAS;

            if( count >= 0 && count <= 200 )
                return off;
        }
    }

    return 0;
}


size_t SCH_PARSER::findTailStart() const
{
    const uint8_t* data     = m_reader.GetData();
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


std::vector<size_t> SCH_PARSER::scanComponentBoundaries( size_t aFirstComp,
                                                         size_t aBusSectionOffset ) const
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
    const uint8_t* data     = m_reader.GetData();
    size_t         fileSize = m_reader.GetFileSize();

    if( m_version < V31_CUTOVER )
    {
        if( aOffset + 19 > fileSize )
            return false;

        int z1 = ( ( data[aOffset + 6] << 16 ) | ( data[aOffset + 7] << 8 )
                   | data[aOffset + 8] )
                 - INT3_BIAS;
        int z2 = ( ( data[aOffset + 9] << 16 ) | ( data[aOffset + 10] << 8 )
                   | data[aOffset + 11] )
                 - INT3_BIAS;

        if( z1 != 0 || z2 != 0 )
            return false;

        int w = ( ( data[aOffset + 12] << 24 ) | ( data[aOffset + 13] << 16 )
                  | ( data[aOffset + 14] << 8 ) | data[aOffset + 15] )
                - INT4_BIAS;

        if( w < 0 || w > 200000 )
            return false;

        int npts = ( ( data[aOffset + 16] << 16 ) | ( data[aOffset + 17] << 8 )
                     | data[aOffset + 18] )
                   - INT3_BIAS;

        return npts >= 1 && npts <= 100;
    }
    else
    {
        if( aOffset + 17 > fileSize )
            return false;

        if( data[aOffset + 6] != 0 || data[aOffset + 7] != 0 || data[aOffset + 8] != 0
            || data[aOffset + 9] != 0 )
        {
            return false;
        }

        int w = ( ( data[aOffset + 10] << 24 ) | ( data[aOffset + 11] << 16 )
                  | ( data[aOffset + 12] << 8 ) | data[aOffset + 13] )
                - INT4_BIAS;

        if( w < 0 || w > 200000 )
            return false;

        int npts = ( ( data[aOffset + 14] << 16 ) | ( data[aOffset + 15] << 8 )
                     | data[aOffset + 16] )
                   - INT3_BIAS;

        return npts >= 1 && npts <= 100;
    }
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
        bool desynced    = false;

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
    std::vector<size_t> compStarts =
            scanComponentBoundaries( compSectionStart, aBusSectionOffset );

    for( size_t ci = 0; ci < compStarts.size(); ci++ )
    {
        size_t compEnd =
                ( ci + 1 < compStarts.size() ) ? compStarts[ci + 1] : aBusSectionOffset;

        try
        {
            m_reader.SetOffset( compStarts[ci] );
            parseOneComponent( compEnd, true );
        }
        catch( const std::exception& e )
        {
            if( m_reporter )
            {
                m_reporter->Report(
                        wxString::Format( _( "DipTrace import: failed to parse component "
                                             "%zu at offset 0x%06zX: %s" ),
                                          ci, compStarts[ci], wxString::FromUTF8( e.what() ) ),
                        RPT_SEVERITY_WARNING );
            }
        }
    }

    // Surface a single diagnostic if the recovered placement count diverges materially from the
    // count the file's own header advertised, which would indicate the header signature missed or
    // over-matched records.
    if( m_componentCount > 0 && m_reporter )
    {
        int found = static_cast<int>( m_components.size() );

        if( std::abs( found - m_componentCount ) > m_componentCount / 20 )
        {
            m_reporter->Report(
                    wxString::Format( _( "DipTrace import: recovered %d of %d components declared "
                                         "in the file header." ),
                                      found, m_componentCount ),
                    RPT_SEVERITY_WARNING );
        }
    }
}


void SCH_PARSER::parseOneComponent( size_t aCompEnd, bool aUseCompEnd )
{
    static bool s_dumpComponents =
            std::getenv( "KICAD_DIPTRACE_DUMP_COMPONENTS" ) != nullptr;
    static bool s_dumpComponentDetail =
            std::getenv( "KICAD_DIPTRACE_DUMP_COMPONENT_DETAIL" ) != nullptr;

    DCH_COMPONENT comp;
    comp.fileOffset = m_reader.GetOffset();

    auto dumpDetail = [&]( const wxString& aMsg )
    {
        if( s_dumpComponentDetail && m_reporter )
        {
            m_reporter->Report(
                    wxString::Format( wxT( "DipTrace SCH detail @0x%06zX: %s" ),
                                      comp.fileOffset, aMsg ),
                    RPT_SEVERITY_INFO );
        }
    };

    comp.bboxX1 = m_reader.ReadInt4();
    comp.bboxY1 = m_reader.ReadInt4();
    comp.bboxX2 = m_reader.ReadInt4();
    comp.bboxY2 = m_reader.ReadInt4();

    comp.compName = m_reader.ReadString();
    comp.refdes   = m_reader.ReadString();
    comp.value    = m_reader.ReadString();
    comp.prefix   = m_reader.ReadString();
    comp.nameDup  = m_reader.ReadString();
    dumpDetail( wxString::Format( wxT( "hdr end=0x%06zX name='%s' ref='%s' value='%s' prefix='%s'" ),
                                  m_reader.GetOffset(), comp.compName, comp.refdes, comp.value,
                                  comp.prefix ) );

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

    comp.partName   = m_reader.ReadString();
    comp.partNumber = m_reader.ReadString();

    uint8_t pb1 = m_reader.ReadByte();
    comp.isMultiPart = ( pb1 == 1 );
    comp.sheetIndex  = m_reader.ReadInt3();

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
        comp.partId  = m_reader.ReadString();
        partTailStr  = comp.partId;
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
                size_t         strEnd   = strStart + static_cast<size_t>( partTailInt );
                const uint8_t* data     = m_reader.GetData();
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
                        int nextInt3 = ( ( data[strEnd] << 16 ) | ( data[strEnd + 1] << 8 )
                                         | data[strEnd + 2] ) - INT3_BIAS;

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
                                  m_reader.GetOffset(), comp.partName, comp.partNumber,
                                  comp.sheetIndex, comp.isMultiPart ? 1 : 0, partFieldB,
                                  partFieldC, partBboxX1, partBboxY1, partBboxX2, partBboxY2,
                                  fieldD, fieldE ) );

    if( s_dumpComponentDetail && m_version < V31_CUTOVER )
    {
        dumpDetail( wxString::Format( wxT( "part-tail end=0x%06zX partTailInt=%d partTailStr='%s'" ),
                                      m_reader.GetOffset(), partTailInt, partTailStr ) );
    }

    if( fieldE >= 1 && fieldE < 1000 )
    {
        for( int i = 0; i < fieldE; i++ )
        {
            m_reader.ReadString();
            m_reader.ReadString();
            m_reader.ReadInt3();
        }
    }

    int fieldF = m_reader.ReadInt3();
    int fieldG = m_reader.ReadInt3();
    int byte4  = m_reader.ReadByte();
    comp.libId = m_reader.ReadInt4();
    comp.libPath = m_reader.ReadString();

    int      tailA     = m_reader.ReadInt3();
    int      tailB     = m_reader.ReadInt3();
    size_t   tailAfterB = m_reader.GetOffset();
    wxString tailStrA  = m_reader.ReadString();
    wxString extraTail = wxEmptyString;
    int      pinMetaA  = 0;
    int      pinMetaB  = 0;
    int      pinMetaF  = 0;
    int      pinHdrByte = 0;
    int      numPins   = 0;

    if( m_version < V31_CUTOVER )
    {
        // Legacy v1/v2 uses an int3+byte+int3 pre-pin metadata tuple.
        // This must be consumed before the pin count.
        pinMetaA = m_reader.ReadInt3();
        pinMetaF = m_reader.ReadByte();
        pinMetaB = m_reader.ReadInt3();
        numPins  = m_reader.ReadInt3();

        if( s_dumpComponentDetail )
        {
            dumpDetail( wxString::Format( wxT( "pre-pin-hdr end=0x%06zX fieldF=%d fieldG=%d "
                                               "byte4=%d libId=%d libPath='%s' tailA=%d tailB=%d "
                                               "tailStrA='%s' pinMetaA=%d pinMetaF=%d pinMetaB=%d "
                                               "numPins=%d" ),
                                          m_reader.GetOffset(), fieldF, fieldG, byte4, comp.libId,
                                          comp.libPath, tailA, tailB, tailStrA, pinMetaA, pinMetaF,
                                          pinMetaB, numPins ) );
        }
    }
    else
    {
        auto readModernExtraAndPins = [&]( wxString& aExtraTail, int& aNumPins, int& aPinHdr ) -> bool
        {
            size_t extraStart = m_reader.GetOffset();
            uint8_t extraHdr[4] = {};
            m_reader.ReadBytes( extraHdr, 4 );

            uint32_t extraChars = ( extraHdr[0] << 24 ) | ( extraHdr[1] << 16 )
                                  | ( extraHdr[2] << 8 ) | extraHdr[3];

            if( extraChars > 0 )
            {
                size_t extraBytes = static_cast<size_t>( extraChars ) * 2;

                if( extraChars < 10000
                    && m_reader.GetOffset() + extraBytes <= m_reader.GetFileSize() )
                {
                    wxMBConvUTF16BE conv;
                    aExtraTail = wxString(
                            reinterpret_cast<const char*>( m_reader.GetData()
                                                           + m_reader.GetOffset() ),
                            conv, extraBytes );
                    m_reader.Skip( extraBytes );
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
                aPinHdr  = m_reader.ReadByte();

                // Some variants store a 2-byte separator before pin count.
                if( ( aNumPins < 0 || aNumPins > 500 ) && pinStart + 6 <= m_reader.GetFileSize() )
                {
                    m_reader.SetOffset( pinStart + 2 );

                    int sepPins = m_reader.ReadInt3();
                    int sepHdr  = m_reader.ReadByte();

                    if( sepPins >= 0 && sepPins <= 500 )
                    {
                        aNumPins = sepPins;
                        aPinHdr  = sepHdr;
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

        if( !readModernExtraAndPins( extraTail, numPins, pinHdrByte )
            || ( numPins < 0 || numPins > 500 ) )
        {
            // Some v41 files omit tailStrA and place the int4-length extra tail
            // directly after tailB.
            m_reader.SetOffset( tailAfterB );
            tailStrA = wxEmptyString;
            extraTail = wxEmptyString;
            numPins = 0;
            pinHdrByte = 0;
            usedTaillessFallback = true;
            readModernExtraAndPins( extraTail, numPins, pinHdrByte );
        }

        if( s_dumpComponentDetail )
        {
            dumpDetail( wxString::Format( wxT( "pre-pin-hdr end=0x%06zX fieldF=%d fieldG=%d "
                                               "byte4=%d libId=%d libPath='%s' tailA=%d tailB=%d "
                                               "tailStrA='%s' extraTail='%s' numPins=%d pinHdr=%d "
                                               "taillessFallback=%d" ),
                                          m_reader.GetOffset(), fieldF, fieldG, byte4, comp.libId,
                                          comp.libPath, tailA, tailB, tailStrA, extraTail,
                                          numPins, pinHdrByte,
                                          usedTaillessFallback ? 1 : 0 ) );
        }
    }

    dumpDetail( wxString::Format( wxT( "pre-pin end=0x%06zX numPins=%d" ),
                                  m_reader.GetOffset(), numPins ) );

    if( numPins < 0 || numPins > 500 )
    {
        if( aUseCompEnd )
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
        try
        {
            parsePin( pinIdx, comp );
        }
        catch( const std::exception& e )
        {
            if( s_dumpComponentDetail && m_reporter )
            {
                m_reporter->Report(
                        wxString::Format( wxT( "DipTrace SCH detail @0x%06zX: pin %d parse "
                                               "failed at 0x%06zX: %s" ),
                                          comp.fileOffset, pinIdx, m_reader.GetOffset(),
                                          wxString::FromUTF8( e.what() ) ),
                        RPT_SEVERITY_INFO );
            }

            if( !aUseCompEnd )
                throw;

            break;
        }
    }

    while( m_reader.GetOffset() < aCompEnd && isShapeStart( m_reader.GetOffset() ) )
    {
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
        parseEmbeddedPattern( comp, aCompEnd );
    }
    catch( const std::exception& e )
    {
        if( s_dumpComponentDetail )
        {
            dumpDetail( wxString::Format( wxT( "pattern parse failed at 0x%06zX: %s" ),
                                          m_reader.GetOffset(),
                                          wxString::FromUTF8( e.what() ) ) );
        }
    }

    size_t parsedEnd = m_reader.GetOffset();

    if( s_dumpComponentDetail )
    {
        dumpDetail( wxString::Format( wxT( "post-pattern end=0x%06zX pins=%zu shapes=%zu "
                                           "pattern='%s'" ),
                                      parsedEnd, comp.pins.size(), comp.shapes.size(),
                                      comp.patternName ) );

        if( aUseCompEnd && parsedEnd < aCompEnd )
        {
            dumpDetail( wxString::Format( wxT( "tail skipped=%zu bytes to compEnd=0x%06zX" ),
                                          aCompEnd - parsedEnd, aCompEnd ) );
        }
    }

    if( aUseCompEnd )
        m_reader.SetOffset( aCompEnd );

    m_components.push_back( comp );

    if( s_dumpComponents && m_reporter )
    {
        m_reporter->Report(
                wxString::Format( wxT( "DipTrace SCH comp @0x%06zX ref='%s' name='%s' "
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

        if( m_version < V31_CUTOVER )
        {
            // Legacy v1/v2 schematic files use a shorter first-pin preamble.
            // Reading 4 int3 fields here misaligns all subsequent pin fields.
            pin.headerA  = m_reader.ReadInt3();
            pin.typeCode = m_reader.ReadInt3();
        }
        else
        {
            pin.headerA  = m_reader.ReadInt3();
            pin.headerB  = m_reader.ReadInt3();
            pin.headerC  = m_reader.ReadInt3();
            pin.typeCode = m_reader.ReadInt3();
        }
    }

    pin.x      = m_reader.ReadInt4();
    pin.y      = m_reader.ReadInt4();
    pin.length = m_reader.ReadInt4();
    pin.name   = m_reader.ReadString();
    pin.number = m_reader.ReadString();

    pin.netFlagA = m_reader.ReadByte();
    pin.netFlagB = m_reader.ReadByte();

    pin.labelXOff = m_reader.ReadInt4();
    pin.labelYOff = m_reader.ReadInt4();
    pin.numXOff   = m_reader.ReadInt4();
    pin.numYOff   = m_reader.ReadInt4();
    m_reader.ReadInt3();  // post_a

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
        m_reader.Skip( 5 );
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
    m_reader.ReadInt3();
    m_reader.ReadInt3();

    aComp.shapes.push_back( shape );
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
        // Modern format (v34+). The embedded pattern section is much larger than
        // the legacy format, containing full library metadata (3D models, library
        // references, etc.). Rather than parsing field-by-field, we locate the
        // pattern name by scanning for a reliable anchor: the UTF-16-BE string
        // "Case Info" which always appears at a known structural distance after
        // the pattern name.
        //
        // Layout before "Case Info":
        //   str(patternName) + 4*int4 + byte + N*int3 + int3(18) + uint16(9) + "Case Info"
        //
        // The int3(18) is always at ciPos-5, but N varies (typically 0 for
        // non-empty patterns, 2 for empty patterns). We anchor on int3(18)
        // and try several gap sizes.

        const uint8_t* data = m_reader.GetData();

        // "Case Info" in UTF-16-BE
        static const uint8_t kCaseInfo[] = {
            0x00, 0x43, 0x00, 0x61, 0x00, 0x73, 0x00, 0x65,
            0x00, 0x20, 0x00, 0x49, 0x00, 0x6E, 0x00, 0x66, 0x00, 0x6F
        };
        static constexpr size_t kCaseInfoLen = sizeof( kCaseInfo );

        // Limit the search to the current component's boundary so we don't
        // accidentally match a "Case Info" from the next component.
        // Also cap at 16 KB from the current position since the largest observed
        // component record is ~14.5 KB.
        static constexpr size_t kMaxPatternSearch = 16384;
        size_t searchEnd = std::min( startOffset + kMaxPatternSearch,
                                     aCompEnd > 0 ? aCompEnd : m_reader.GetFileSize() );
        size_t ciPos     = m_reader.FindPattern( kCaseInfo, kCaseInfoLen,
                                                 startOffset, searchEnd );

        if( ciPos == std::string::npos || ciPos < startOffset + 30 )
            return;

        // int3(18) anchor is always 5 bytes before "Case Info" text
        size_t anchor = ciPos - 5;

        // The gap between pattern name end and anchor is:
        //   4*int4(16) + byte(1) + N*int3(N*3) = 17 + N*3
        // where N is 0, 1, or 2 extra int3 fields.
        static constexpr int kMaxExtraInt3 = 4;
        static constexpr int kBaseGap      = 17;   // 4*int4 + byte

        for( int extraInt3s = 0; extraInt3s <= kMaxExtraInt3; extraInt3s++ )
        {
            size_t nameEnd = anchor - kBaseGap - static_cast<size_t>( extraInt3s ) * 3;

            if( nameEnd < startOffset + 2 )
                break;

            // Try to find a valid UTF-16-BE string ending at nameEnd
            for( int tryLen = 0; tryLen <= 50; tryLen++ )
            {
                size_t strStart = nameEnd - static_cast<size_t>( tryLen ) * 2 - 2;

                if( strStart < startOffset )
                    break;

                int charCount = ( data[strStart] << 8 ) | data[strStart + 1];

                if( charCount != tryLen )
                    continue;

                if( charCount == 0 )
                    break;  // empty string = no pattern name

                size_t strDataStart = strStart + 2;
                size_t strDataEnd   = strDataStart + static_cast<size_t>( charCount ) * 2;

                if( strDataEnd != nameEnd )
                    continue;

                bool valid = true;

                for( size_t i = strDataStart; i < strDataEnd; i += 2 )
                {
                    if( data[i] != 0x00 || data[i + 1] < 0x20 || data[i + 1] > 0x7E )
                    {
                        valid = false;
                        break;
                    }
                }

                if( valid )
                {
                    wxMBConvUTF16BE conv;
                    aComp.patternName = wxString(
                            reinterpret_cast<const char*>( data + strDataStart ),
                            conv, static_cast<size_t>( charCount ) * 2 );
                }

                break;
            }

            if( !aComp.patternName.IsEmpty() )
                break;
        }
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
        return;

    for( int i = 0; i < busCount; i++ )
    {
        try
        {
            DCH_BUS_ENTRY entry;

            m_reader.ReadByte();
            m_reader.ReadByte();
            m_reader.ReadByte();

            entry.coordX      = m_reader.ReadInt4();
            entry.coordY      = m_reader.ReadInt4();
            entry.sheetIndex  = m_reader.ReadInt3();
            entry.busType     = m_reader.ReadInt3();
            entry.instanceId  = m_reader.ReadInt3();
            entry.signalCount = m_reader.ReadInt3();

            int terminator = m_reader.ReadInt3();

            if( terminator != -1 )
            {
                if( m_reporter )
                {
                    m_reporter->Report(
                            wxString::Format( _( "DipTrace import: bus entry %d has "
                                                 "unexpected terminator %d." ),
                                              i, terminator ),
                            RPT_SEVERITY_WARNING );
                }

                break;
            }

            entry.name = m_reader.ReadString();
            m_reader.ReadByte();

            m_buses.push_back( entry );
        }
        catch( const std::exception& e )
        {
            if( m_reporter )
            {
                m_reporter->Report(
                        wxString::Format( _( "DipTrace import: failed to parse bus entry "
                                             "%d: %s" ),
                                          i, wxString::FromUTF8( e.what() ) ),
                        RPT_SEVERITY_WARNING );
            }

            break;
        }
    }
}


void SCH_PARSER::parseNetSection()
{
    const uint8_t* data      = m_reader.GetData();
    size_t         fileSize  = m_reader.GetFileSize();
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

            int n = ( ( data[strOff] << 16 ) | ( data[strOff + 1] << 8 ) | data[strOff + 2] )
                    - INT3_BIAS;

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

            wxString name = wxString::From8BitData(
                    reinterpret_cast<const char*>( data + strOff + 3 ), n );
            size_t afterStr = strOff + 3 + n;

            DCH_NET_ENTRY entry;
            entry.name = name;

            if( afterStr + 11 <= fileSize )
            {
                entry.coordX =
                        ( ( data[afterStr] << 24 ) | ( data[afterStr + 1] << 16 )
                          | ( data[afterStr + 2] << 8 ) | data[afterStr + 3] )
                        - INT4_BIAS;
                entry.coordY =
                        ( ( data[afterStr + 4] << 24 ) | ( data[afterStr + 5] << 16 )
                          | ( data[afterStr + 6] << 8 ) | data[afterStr + 7] )
                        - INT4_BIAS;
                entry.field1 =
                        ( ( data[afterStr + 8] << 16 ) | ( data[afterStr + 9] << 8 )
                          | data[afterStr + 10] )
                        - INT3_BIAS;
            }

            m_nets.push_back( entry );
        }
        else
        {
            if( strOff + 2 > searchEnd )
                continue;

            int n = ( data[strOff] << 8 ) | data[strOff + 1];

            if( n < 1 || n > 200 || strOff + 2 + (size_t)( n * 2 ) > fileSize )
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
            wxString        name( reinterpret_cast<const char*>( data + strOff + 2 ), conv,
                                  static_cast<size_t>( n ) * 2 );
            size_t          afterStr = strOff + 2 + static_cast<size_t>( n ) * 2;

            DCH_NET_ENTRY entry;
            entry.name = name;

            if( afterStr + 11 <= fileSize )
            {
                entry.coordX =
                        ( ( data[afterStr] << 24 ) | ( data[afterStr + 1] << 16 )
                          | ( data[afterStr + 2] << 8 ) | data[afterStr + 3] )
                        - INT4_BIAS;
                entry.coordY =
                        ( ( data[afterStr + 4] << 24 ) | ( data[afterStr + 5] << 16 )
                          | ( data[afterStr + 6] << 8 ) | data[afterStr + 7] )
                        - INT4_BIAS;
                entry.field1 =
                        ( ( data[afterStr + 8] << 16 ) | ( data[afterStr + 9] << 8 )
                          | data[afterStr + 10] )
                        - INT3_BIAS;
            }

            m_nets.push_back( entry );
        }
    }
}


int SCH_PARSER::pinOrientationFromStub( int aStubDx, int aStubDy )
{
    if( aStubDx < 0 )       return 2;  // PIN_LEFT
    else if( aStubDx > 0 )  return 0;  // PIN_RIGHT
    else if( aStubDy > 0 )  return 1;  // PIN_UP
    else if( aStubDy < 0 )  return 3;  // PIN_DOWN
    return 0;
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

    SCH_SHEET*  newSheet  = new SCH_SHEET( m_rootSheet, pos, size );
    SCH_SCREEN* newScreen = new SCH_SCREEN( m_schematic );

    wxFileName fn( m_fileName );
    fn.SetName( fn.GetName() + wxString::Format( wxT( "_%d" ), aSheetIndex ) );
    fn.SetExt( FILEEXT::KiCadSchematicFileExtension );

    newScreen->SetFileName( fn.GetFullPath() );
    newSheet->SetScreen( newScreen );
    newSheet->SetFileName( fn.GetFullName() );
    newSheet->SetName( sheetName );

    m_rootSheet->GetScreen()->Append( newSheet );
    m_sheets[aSheetIndex] = newSheet;

    return newScreen;
}


LIB_SYMBOL* SCH_PARSER::getOrCreateLibSymbol( const DCH_COMPONENT& aComp, int aUnit )
{
    wxString symName = aComp.compName;

    if( symName.IsEmpty() )
        symName = aComp.refdes;

    if( symName.IsEmpty() )
        symName = wxT( "Unknown" );

    symName = LIB_ID::FixIllegalChars( symName, true ).wx_str();

    auto it = m_libSymbols.find( symName );

    if( it != m_libSymbols.end() )
    {
        LIB_SYMBOL* existing = it->second.get();

        if( aUnit > existing->GetUnitCount() )
            existing->SetUnitCount( aUnit, true );

        return existing;
    }

    auto libSymbol = std::make_unique<LIB_SYMBOL>( symName );
    libSymbol->SetUnitCount( aUnit, true );

    if( !aComp.patternName.IsEmpty() )
        libSymbol->GetFootprintField().SetText( aComp.patternName );

    bool isPower = aComp.refdes.StartsWith( wxT( "NetPort" ) );

    if( isPower )
        libSymbol->SetGlobalPower();

    for( const DCH_PIN& dchPin : aComp.pins )
    {
        auto pin = std::make_unique<SCH_PIN>( libSymbol.get() );

        pin->SetName( dchPin.name.IsEmpty() ? wxString( wxT( "~" ) ) : dchPin.name );
        pin->SetNumber( dchPin.number.IsEmpty() ? wxString( wxT( "1" ) ) : dchPin.number );
        pin->SetPosition( VECTOR2I( toKiCadCoordX( dchPin.x ),
                                    toKiCadCoordY( dchPin.y ) ) );
        pin->SetLength( static_cast<int>( static_cast<int64_t>( abs( dchPin.length ) ) * 100 / 3 ) );

        int orient = pinOrientationFromStub( dchPin.stubDx, dchPin.stubDy );

        switch( orient )
        {
        case 0: pin->SetOrientation( PIN_ORIENTATION::PIN_RIGHT ); break;
        case 1: pin->SetOrientation( PIN_ORIENTATION::PIN_UP );    break;
        case 2: pin->SetOrientation( PIN_ORIENTATION::PIN_LEFT );  break;
        case 3: pin->SetOrientation( PIN_ORIENTATION::PIN_DOWN );  break;
        }

        pin->SetType( isPower ? ELECTRICAL_PINTYPE::PT_POWER_IN
                               : ELECTRICAL_PINTYPE::PT_PASSIVE );
        pin->SetUnit( aUnit );
        libSymbol->AddDrawItem( pin.release() );
    }

    for( const DCH_SHAPE& dchShape : aComp.shapes )
    {
        if( dchShape.points.size() < 2 )
            continue;

        auto poly = std::make_unique<SCH_SHAPE>( SHAPE_T::POLY, LAYER_DEVICE, 0,
                                                   FILL_T::NO_FILL );
        poly->SetParent( libSymbol.get() );

        for( const VECTOR2I& pt : dchShape.points )
            poly->AddPoint( VECTOR2I( toKiCadCoordX( pt.x ), toKiCadCoordY( pt.y ) ) );

        int width = static_cast<int>( static_cast<int64_t>( dchShape.lineWidth ) * 100 / 3 );

        if( width <= 0 )
            width = 1;

        poly->SetStroke( STROKE_PARAMS( width, LINE_STYLE::SOLID ) );
        poly->SetUnit( aUnit );
        libSymbol->AddDrawItem( poly.release() );
    }

    LIB_SYMBOL* rawPtr = libSymbol.get();
    m_libSymbols[symName] = std::move( libSymbol );
    return rawPtr;
}


void SCH_PARSER::buildWirePointSheets()
{
    m_wirePointSheets.clear();
    m_pointPartSheets.clear();

    std::map<int, std::map<int, int>> partSheetVotes;   // partId -> sheet -> count

    for( const DCH_WIRE& wire : m_wires )
    {
        int sheetIdx = wire.sheetIndex;

        if( sheetIdx < 0 || sheetIdx >= m_numSheets )
            sheetIdx = 0;

        for( const VECTOR2I& pt : wire.points )
            m_wirePointSheets[{ pt.x, pt.y }].insert( sheetIdx );

        // The two endpoints carry the part each end connects to (object1 at the first point,
        // object2 at the last); record them with the sheet for exact part-id recovery.
        if( wire.points.size() >= 2 )
        {
            if( wire.object1 >= 0 )
            {
                const VECTOR2I& a = wire.points.front();
                m_pointPartSheets[{ a.x, a.y }].emplace_back( wire.object1, sheetIdx );
                partSheetVotes[wire.object1][sheetIdx]++;
            }

            if( wire.object2 >= 0 )
            {
                const VECTOR2I& b = wire.points.back();
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
    const uint8_t* data     = m_reader.GetData();
    size_t         fileSize = m_reader.GetFileSize();

    if( aOffset + 16 > fileSize )
        return false;

    // Four leading int4: the placement (centerX, centerY) followed by width/height. Reject values
    // that are out of range or essentially at the origin (which only mid-record noise produces).
    int bbox[4];

    for( int i = 0; i < 4; i++ )
    {
        size_t   p = aOffset + static_cast<size_t>( i ) * 4;
        uint32_t raw = ( static_cast<uint32_t>( data[p] ) << 24 )
                       | ( static_cast<uint32_t>( data[p + 1] ) << 16 )
                       | ( static_cast<uint32_t>( data[p + 2] ) << 8 ) | data[p + 3];
        bbox[i] = static_cast<int>( static_cast<int64_t>( raw ) - INT4_BIAS );

        if( std::abs( bbox[i] ) > 50000000 )
            return false;
    }

    if( std::abs( bbox[0] ) <= 50000 && std::abs( bbox[1] ) <= 50000 )
        return false;

    // Five header strings (compName, refdes, value, prefix, nameDup). Each is empty or a short
    // printable token; the first (compName) must be non-empty.
    size_t p = aOffset + 16;

    for( int si = 0; si < 5; si++ )
    {
        int charCount = 0;
        size_t dataStart = 0;
        bool   ascii = ( m_version <= LEGACY_STRING_VERSION );

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
            if( si == 0 )
                return false;

            p = dataStart;
            continue;
        }

        if( charCount < 0 || charCount > 64 )
            return false;

        size_t byteCount = ascii ? static_cast<size_t>( charCount )
                                 : static_cast<size_t>( charCount ) * 2;

        if( dataStart + byteCount > fileSize )
            return false;

        for( int k = 0; k < charCount; k++ )
        {
            unsigned ch = ascii
                                  ? data[dataStart + k]
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
    for( const auto& [ps, count] : aTally )   // std::map iterates by ascending part id
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

        std::set<std::pair<int, int>> seenHere;   // count each (part, sheet) once per point

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
    // are imported as global net labels by createNetLabels(), so skip them here to avoid drawing a
    // redundant symbol on top of the label.
    if( aComp.libPath.Contains( wxT( "auto_net_ports" ) ) )
        return;

    int unit = 1;

    if( !aComp.refdes.IsEmpty() )
    {
        auto it = m_refdesUnitMap.find( aComp.refdes );

        if( it != m_refdesUnitMap.end() )
        {
            unit = it->second + 1;
            it->second = unit;
        }
        else
        {
            m_refdesUnitMap[aComp.refdes] = 1;
        }
    }

    LIB_SYMBOL* libSym = getOrCreateLibSymbol( aComp, unit );

    if( !libSym )
        return;

    wxString symName = LIB_ID::FixIllegalChars(
                               aComp.compName.IsEmpty() ? aComp.refdes : aComp.compName, true )
                               .wx_str();

    LIB_ID libId( getLibName(), symName );

    // The header bbox is [centerX, centerY, width, height]; the first pair is the placement point.
    VECTOR2I pos( toKiCadCoordX( aComp.bboxX1 ), toKiCadCoordY( aComp.bboxY1 ) );

    SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSym, libId, &m_schematic->CurrentSheet(), unit, 0,
                                          pos );

    symbol->SetLibSymbol( new LIB_SYMBOL( *libSym ) );

    if( !aComp.refdes.IsEmpty() )
    {
        SCH_FIELD* refField = symbol->GetField( FIELD_T::REFERENCE );

        if( refField )
            refField->SetText( aComp.refdes );
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

    // DipTrace stores no per-component sheet field; recover it from connectivity. The header bbox
    // is [centerX, centerY, width, height]; each pin's connection point (where a wire ends) is the
    // pin coordinate offset from that center, extended outward by the pin length along its dominant
    // axis. Matching those connection points against the decoded wire geometry yields the owning
    // sheet without needing the component rotation (which is not parsed). Falls back to the supplied
    // screen when no pin coincides with a wire.
    VECTOR2I              center( toKiCadCoordX( aComp.bboxX1 ), toKiCadCoordY( aComp.bboxY1 ) );
    std::vector<VECTOR2I> connectionPoints;

    for( const DCH_PIN& dchPin : aComp.pins )
    {
        VECTOR2I off( toKiCadCoordX( dchPin.x ), toKiCadCoordY( dchPin.y ) );
        int      len = static_cast<int>( static_cast<int64_t>( std::abs( dchPin.length ) ) * 100 / 3 );
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


void SCH_PARSER::createNetLabels()
{
    // Labels are derived from the net/wire section (one per net per sheet, anchored on a wire),
    // which carries reliable positions and sheet membership. The legacy net-name marker scan
    // (m_nets) yields only a constant placeholder position, so it is not used for placement.
    for( const DCH_NET_LABEL& net : m_netLabels )
    {
        if( net.name.IsEmpty() )
            continue;

        SCH_SCREEN* screen = getOrCreateSheet( net.sheetIndex );

        SCH_GLOBALLABEL* label = new SCH_GLOBALLABEL( net.pos, net.name );
        label->SetShape( LABEL_FLAG_SHAPE::L_BIDI );
        screen->Append( label );
    }
}


// True if the UTF-16-BE string at aData[aPos] is a plausible net name immediately followed by a
// labelX/labelY pair that look like real coordinates. This discriminates real net names from
// 'Tahoma' font blocks and binary noise inside the net-section preambles.
static bool isPlausibleNetName( const uint8_t* aData, size_t aPos, size_t aSectionEnd )
{
    if( aPos + 2 > aSectionEnd )
        return false;

    int cnt = ( aData[aPos] << 8 ) | aData[aPos + 1];

    if( cnt < 1 || cnt > 64 )
        return false;

    size_t end = aPos + 2 + static_cast<size_t>( cnt ) * 2;

    if( end + 8 > aSectionEnd )
        return false;

    for( int i = 0; i < cnt; i++ )
    {
        unsigned hi = aData[aPos + 2 + static_cast<size_t>( i ) * 2];
        unsigned lo = aData[aPos + 2 + static_cast<size_t>( i ) * 2 + 1];
        unsigned ch = ( hi << 8 ) | lo;

        bool ok = ( ch >= 0x20 && ch < 0x7F )            // ASCII printable
                  || ( ch >= 0x00A0 && ch <= 0x024F )    // Latin-1 / Latin Extended
                  || ( ch >= 0x0400 && ch <= 0x04FF );   // Cyrillic

        if( !ok )
            return false;
    }

    auto rdInt4 = [&]( size_t o ) -> int
    {
        uint32_t raw = ( static_cast<uint32_t>( aData[o] ) << 24 )
                       | ( static_cast<uint32_t>( aData[o + 1] ) << 16 )
                       | ( static_cast<uint32_t>( aData[o + 2] ) << 8 )
                       | static_cast<uint32_t>( aData[o + 3] );
        return static_cast<int>( static_cast<int64_t>( raw ) - INT4_BIAS );
    };

    int lx = rdInt4( end );
    int ly = rdInt4( end + 4 );

    return lx > -2000000 && lx < 2000000 && ly > -2000000 && ly < 2000000;
}


void SCH_PARSER::parseWireSection()
{
    // Only the modern UTF-16 string format carries the structured wire layout decoded here; the
    // net-name scan below assumes 2-byte-prefixed UTF-16-BE strings. Legacy (<= v37) files use
    // ASCII strings and keep the net-label-only import from parseNetSection().
    if( m_version <= LEGACY_STRING_VERSION )
        return;

    const uint8_t* data         = m_reader.GetData();
    size_t         fileSize     = m_reader.GetFileSize();
    size_t         sectionEnd   = m_tailOffset > 0 ? m_tailOffset : fileSize;
    size_t         sectionStart = m_busSectionOffset;

    if( sectionStart == 0 || sectionStart >= sectionEnd )
        return;

    auto rdInt3 = [&]( size_t o ) -> int
    {
        return ( ( data[o] << 16 ) | ( data[o + 1] << 8 ) | data[o + 2] ) - INT3_BIAS;
    };

    auto rdInt4 = [&]( size_t o ) -> int
    {
        uint32_t raw = ( static_cast<uint32_t>( data[o] ) << 24 )
                       | ( static_cast<uint32_t>( data[o + 1] ) << 16 )
                       | ( static_cast<uint32_t>( data[o + 2] ) << 8 )
                       | static_cast<uint32_t>( data[o + 3] );
        return static_cast<int>( static_cast<int64_t>( raw ) - INT4_BIAS );
    };

    // Locate the first net name within the section header.
    size_t pos = 0;

    for( size_t p = sectionStart; p + 2 < sectionEnd; p++ )
    {
        if( isPlausibleNetName( data, p, sectionEnd ) )
        {
            pos = p;
            break;
        }
    }

    if( pos == 0 )
        return;

    int                                  safetyNets = 0;
    std::set<std::pair<wxString, int>>    seenNetSheets;

    while( pos != 0 && pos < sectionEnd && safetyNets++ < 100000 )
    {
        size_t o = pos;

        // Net name (UTF-16-BE), then labelX(int4) labelY(int4) pad(int3) flag(byte).
        int      nameLen = ( data[o] << 8 ) | data[o + 1];
        wxString netName;

        if( nameLen > 0 && o + 2 + static_cast<size_t>( nameLen ) * 2 <= sectionEnd )
        {
            wxMBConvUTF16BE conv;
            netName = wxString( reinterpret_cast<const char*>( data + o + 2 ), conv,
                                static_cast<size_t>( nameLen ) * 2 );
        }

        o += 2 + static_cast<size_t>( nameLen ) * 2;
        o += 4 + 4 + 3 + 1;

        if( o + 3 > sectionEnd )
            break;

        int pinCount = rdInt3( o );
        o += 3;

        if( pinCount < 0 || pinCount > 4000 || o + static_cast<size_t>( pinCount ) * 6 + 3 > sectionEnd )
            break;

        o += static_cast<size_t>( pinCount ) * 6;

        int wireCount = rdInt3( o );
        o += 3;

        if( wireCount < 0 || wireCount > 100000 )
            break;

        bool brokeEarly = false;

        for( int w = 0; w < wireCount; w++ )
        {
            if( o + 36 + 1 + 3 > sectionEnd )
            {
                brokeEarly = true;
                break;
            }

            DCH_WIRE wire;
            wire.object1    = rdInt3( o + 0 );
            wire.object2    = rdInt3( o + 3 );
            wire.subObject1 = rdInt3( o + 6 );
            wire.subObject2 = rdInt3( o + 9 );
            wire.bus1       = rdInt3( o + 12 );
            wire.bus2       = rdInt3( o + 15 );
            wire.sheetIndex = rdInt3( o + 18 );

            o += 36;   // 12 int3 header tokens
            o += 1;    // flag byte

            int pointCount = rdInt3( o );
            o += 3;

            if( pointCount < 0 || pointCount > 4000
                || o + static_cast<size_t>( pointCount ) * 11 + 8 > sectionEnd )
            {
                brokeEarly = true;
                break;
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

                o += 11;   // X(int4) Y(int4) Dir(int3)
            }

            o += 8;   // per-wire trailer

            if( wire.points.size() >= 2 )
            {
                // Emit one net-name label per sheet the net appears on, anchored at a wire
                // endpoint so it labels the actual conductor (DipTrace net ports have no usable
                // stored position).
                int sheetIdx = wire.sheetIndex;

                if( sheetIdx < 0 || sheetIdx >= m_numSheets )
                    sheetIdx = 0;

                if( !netName.IsEmpty()
                    && seenNetSheets.insert( { netName, sheetIdx } ).second )
                {
                    m_netLabels.push_back( { netName, wire.points.front(), sheetIdx } );
                }

                m_wires.push_back( std::move( wire ) );
            }
        }

        if( brokeEarly )
            break;

        // Find the next net name (skips the variable-length net preamble).
        size_t next = 0;

        for( size_t q = o; q + 2 < sectionEnd && q < o + 400; q++ )
        {
            if( isPlausibleNetName( data, q, sectionEnd ) )
            {
                next = q;
                break;
            }
        }

        pos = next;
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
            const VECTOR2I& a = wire.points[i - 1];
            const VECTOR2I& b = wire.points[i];

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
    // DipTrace stores no explicit junctions; reuse KiCad's own rule (>=3 conductor ends coincide,
    // or a wire end lands on another wire's interior, including symbol-pin taps). Must run after
    // wires, labels and symbols are placed.
    std::set<SCH_SCREEN*> screens;

    if( m_rootSheet && m_rootSheet->GetScreen() )
        screens.insert( m_rootSheet->GetScreen() );

    for( SCH_SHEET* sheet : m_sheets )
    {
        if( sheet && sheet->GetScreen() )
            screens.insert( sheet->GetScreen() );
    }

    for( SCH_SCREEN* screen : screens )
    {
        std::deque<EDA_ITEM*> items;

        for( SCH_ITEM* item : screen->Items() )
            items.push_back( item );

        for( const VECTOR2I& pt : screen->GetNeededJunctions( items ) )
        {
            SCH_JUNCTION* junction = new SCH_JUNCTION( pt );
            screen->Append( junction );
        }
    }
}


void SCH_PARSER::createKiCadObjects()
{
    m_sheets.resize( m_numSheets, nullptr );

    if( m_numSheets > 0 )
        m_sheets[0] = m_rootSheet;

    if( m_numSheets > 1 )
    {
        for( int i = 1; i < m_numSheets; i++ )
            getOrCreateSheet( i );
    }

    // Index the decoded wire geometry by position so each symbol and label can be routed to the
    // sheet it connects to (DipTrace does not record sheet membership on components).
    buildWirePointSheets();
    buildComponentPartIds();
    m_lastSymbolPartId = -1;

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
                m_reporter->Report(
                        wxString::Format( _( "DipTrace import: failed to create symbol "
                                             "for %s (%s): %s" ),
                                          comp.refdes, comp.compName,
                                          wxString::FromUTF8( e.what() ) ),
                        RPT_SEVERITY_WARNING );
            }
        }
    }

    createNetLabels();
    createWires();
    createJunctions();

    if( !m_libSymbols.empty() )
    {
        wxFileName libFileName;
        wxString   projectPath = m_schematic->Project().GetProjectPath();

        // In headless/library-less import there is no project on disk, so the project
        // path is empty and Assign() would yield a relative path. The sexpr library
        // cache rejects relative paths (wxCHECK m_libFileName.IsAbsolute()), so anchor
        // the library to the absolute root-sheet directory instead.
        if( projectPath.IsEmpty() )
        {
            libFileName.Assign( m_rootSheet->GetFileName() );
            libFileName.SetName( getLibName() );
            libFileName.SetExt( FILEEXT::KiCadSymbolLibFileExtension );
            libFileName.MakeAbsolute();
        }
        else
        {
            libFileName.Assign( projectPath, getLibName(),
                                FILEEXT::KiCadSymbolLibFileExtension );
        }

        bool libSaved = false;

        // Step 1: Save the symbol library file. This is independent of table registration
        // and should succeed even in headless/CLI contexts.
        try
        {
            IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

            if( pi )
            {
                // CreateLibrary refuses to overwrite. The import-generated library is named after
                // the schematic and is wholly derived from it, so a stale copy from a previous
                // import of the same file must be replaced rather than aborting the save.
                if( libFileName.FileExists() )
                    wxRemoveFile( libFileName.GetFullPath() );

                pi->CreateLibrary( libFileName.GetFullPath() );

                std::map<std::string, UTF8> properties;
                properties.emplace( SCH_IO_KICAD_SEXPR::PropBuffering, wxEmptyString );

                for( auto& [name, libSym] : m_libSymbols )
                {
                    pi->SaveSymbol( libFileName.GetFullPath(),
                                    new LIB_SYMBOL( *libSym.get() ), &properties );
                }

                pi->SaveLibrary( libFileName.GetFullPath() );
                libSaved = true;
            }
        }
        catch( const std::exception& e )
        {
            if( m_reporter )
            {
                m_reporter->Report(
                        wxString::Format( _( "DipTrace import: failed to save symbol "
                                             "library: %s" ),
                                          wxString::FromUTF8( e.what() ) ),
                        RPT_SEVERITY_WARNING );
            }
        }

        // Step 2: Register the library in the project symbol library table. This may fail
        // in headless/import contexts where the UI adapter is not available. The symbols
        // are already embedded in the schematic, so the library file is supplementary.
        if( libSaved )
        {
            try
            {
                SYMBOL_LIBRARY_ADAPTER* adapter =
                        PROJECT_SCH::SymbolLibAdapter( &m_schematic->Project() );

                if( adapter )
                {
                    LIBRARY_TABLE* table = adapter->ProjectTable().value_or( nullptr );

                    if( table && !table->HasRow( getLibName() ) )
                    {
                        // ${KIPRJMOD} is undefined without a project on disk, so point the
                        // row at the absolute file actually saved above in that case.
                        wxString libTableUri =
                                projectPath.IsEmpty()
                                        ? libFileName.GetFullPath()
                                        : wxT( "${KIPRJMOD}/" ) + libFileName.GetFullName();

                        LIBRARY_TABLE_ROW& row = table->InsertRow();
                        row.SetNickname( getLibName() );
                        row.SetURI( libTableUri );
                        row.SetType( "KiCad" );
                        table->Save();

                        adapter->LoadOne( getLibName() );
                    }
                }
                else
                {
                    if( m_reporter )
                    {
                        m_reporter->Report(
                                _( "DipTrace import: symbol library saved but could not "
                                   "register in project library table (no adapter available)." ),
                                RPT_SEVERITY_WARNING );
                    }
                }
            }
            catch( const std::exception& e )
            {
                if( m_reporter )
                {
                    m_reporter->Report(
                            wxString::Format(
                                    _( "DipTrace import: symbol library saved but failed "
                                       "to register in project library table: %s" ),
                                    wxString::FromUTF8( e.what() ) ),
                            RPT_SEVERITY_WARNING );
                }
            }
        }
    }

    if( m_reporter )
    {
        m_reporter->Report(
                wxString::Format( _( "DipTrace import: loaded %zu components, %zu buses, "
                                     "%zu net labels from version %d file with %d sheets." ),
                                  m_components.size(), m_buses.size(), m_netLabels.size(), m_version,
                                  m_numSheets ),
                RPT_SEVERITY_INFO );
    }
}

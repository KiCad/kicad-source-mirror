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
#include <memory>
#include <regex>

#include <wx/filename.h>
#include <wx/log.h>

#include <lib_id.h>
#include <lib_symbol.h>
#include <progress_reporter.h>
#include <project.h>
#include <project_sch.h>
#include <reporter.h>
#include <sch_bus_entry.h>
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
            m_reader.Skip( extraChars * 2 );
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


bool SCH_PARSER::isValidRefdes( const wxString& aRefdes )
{
    if( aRefdes.IsEmpty() )
        return true;

    if( aRefdes.StartsWith( wxT( "NetPort" ) ) )
        return true;

    static const std::regex refdesRegex( "^[A-Z]{1,4}[0-9]{1,4}[A-Za-z]?$" );
    return std::regex_match( aRefdes.ToStdString(), refdesRegex );
}


std::vector<size_t> SCH_PARSER::scanComponentBoundaries( size_t aFirstComp,
                                                         size_t aBusSectionOffset ) const
{
    const uint8_t* data     = m_reader.GetData();
    size_t         fileSize = m_reader.GetFileSize();
    int            version  = m_reader.GetVersion();

    std::vector<size_t> boundaries;
    boundaries.push_back( aFirstComp );

    size_t off = aFirstComp + 16;

    while( off < aBusSectionOffset - 20 )
    {
        bool ok = true;

        for( int i = 0; i < 4; i++ )
        {
            size_t p = off + i * 4;

            if( p + 4 > fileSize )
            {
                ok = false;
                break;
            }

            int raw = ( data[p] << 24 ) | ( data[p + 1] << 16 ) | ( data[p + 2] << 8 )
                      | data[p + 3];

            if( abs( raw - INT4_BIAS ) > 50000000 )
            {
                ok = false;
                break;
            }
        }

        if( ok )
        {
            size_t   pos = off + 16;
            int      nonEmpty = 0;
            bool     valid = true;
            wxString strings[5];

            for( int si = 0; si < 5; si++ )
            {
                if( version < V31_CUTOVER )
                {
                    if( pos + 3 > fileSize )
                    {
                        valid = false;
                        break;
                    }

                    int n = ( ( data[pos] << 16 ) | ( data[pos + 1] << 8 ) | data[pos + 2] )
                            - INT3_BIAS;
                    pos += 3;

                    if( n < 0 || n > 5000 )
                    {
                        if( n != 0 ) { valid = false; break; }
                        continue;
                    }

                    if( pos + (size_t) n > fileSize )
                    {
                        valid = false;
                        break;
                    }

                    if( n > 0 )
                    {
                        strings[si] = wxString::From8BitData(
                                reinterpret_cast<const char*>( data + pos ), n );
                        pos += n;
                        nonEmpty++;
                    }
                }
                else
                {
                    if( pos + 2 > fileSize )
                    {
                        valid = false;
                        break;
                    }

                    int n = ( data[pos] << 8 ) | data[pos + 1];
                    pos += 2;

                    if( n < 0 || n > 5000 )
                    {
                        if( n != 0 ) { valid = false; break; }
                        continue;
                    }

                    if( pos + (size_t)( n * 2 ) > fileSize )
                    {
                        valid = false;
                        break;
                    }

                    if( n > 0 )
                    {
                        wxMBConvUTF16BE conv;
                        strings[si] = wxString( reinterpret_cast<const char*>( data + pos ),
                                                conv, n * 2 );
                        pos += n * 2;
                        nonEmpty++;
                    }
                }
            }

            if( valid && nonEmpty >= 2 )
            {
                bool accepted = false;

                if( strings[0] == strings[4] && isValidRefdes( strings[1] ) )
                    accepted = true;

                if( !accepted && !strings[1].IsEmpty() && !strings[0].IsEmpty()
                    && isValidRefdes( strings[1] ) )
                {
                    accepted = true;
                }

                if( accepted )
                {
                    boundaries.push_back( off );
                    off += 16;
                    continue;
                }
            }
        }

        off++;
    }

    std::sort( boundaries.begin(), boundaries.end() );
    boundaries.erase( std::unique( boundaries.begin(), boundaries.end() ), boundaries.end() );
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

    if( m_componentCount >= 0 )
    {
        int parsedCount = 0;

        while( parsedCount < m_componentCount && m_reader.GetOffset() < aBusSectionOffset )
        {
            size_t compStart = m_reader.GetOffset();

            try
            {
                parseOneComponent( aBusSectionOffset, false );
                parsedCount++;
            }
            catch( const std::exception& e )
            {
                if( m_reporter )
                {
                    m_reporter->Report(
                            wxString::Format( _( "DipTrace import: failed to parse component "
                                                 "%d/%d at offset 0x%06zX: %s" ),
                                              parsedCount + 1, m_componentCount, compStart,
                                              wxString::FromUTF8( e.what() ) ),
                            RPT_SEVERITY_WARNING );
                }

                break;
            }

            if( m_reader.GetOffset() <= compStart )
                break;
        }

        if( parsedCount == m_componentCount )
            return;

        if( m_reporter )
        {
            m_reporter->Report(
                    wxString::Format( _( "DipTrace import: parsed %d/%d components with "
                                         "count-guided decoding; falling back to "
                                         "boundary scan." ),
                                      parsedCount, m_componentCount ),
                    RPT_SEVERITY_WARNING );
        }

        m_components.clear();
        m_reader.SetOffset( compSectionStart );
    }

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
            size_t nameEnd = anchor - kBaseGap - extraInt3s * 3;

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
                uint8_t hi = data[strOff + 2 + i * 2];
                uint8_t lo = data[strOff + 2 + i * 2 + 1];

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
                                  n * 2 );
            size_t          afterStr = strOff + 2 + n * 2;

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

        pin->SetName( dchPin.name.IsEmpty() ? wxT( "~" ) : dchPin.name );
        pin->SetNumber( dchPin.number.IsEmpty() ? wxT( "1" ) : dchPin.number );
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


void SCH_PARSER::createSymbolInstance( const DCH_COMPONENT& aComp, SCH_SCREEN* aScreen )
{
    if( aComp.refdes.IsEmpty() && aComp.compName.IsEmpty() )
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

    int centerX = ( aComp.bboxX1 + aComp.bboxX2 ) / 2;
    int centerY = ( aComp.bboxY1 + aComp.bboxY2 ) / 2;

    VECTOR2I pos( toKiCadCoordX( centerX ), toKiCadCoordY( centerY ) );

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

    aScreen->Append( symbol );
}


void SCH_PARSER::createNetLabels()
{
    for( const DCH_NET_ENTRY& net : m_nets )
    {
        if( net.name.IsEmpty() )
            continue;

        SCH_SCREEN* screen = m_rootSheet->GetScreen();
        VECTOR2I    pos( toKiCadCoordX( net.coordX ), toKiCadCoordY( net.coordY ) );

        SCH_GLOBALLABEL* label = new SCH_GLOBALLABEL( pos, net.name );
        label->SetShape( LABEL_FLAG_SHAPE::L_BIDI );
        screen->Append( label );
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

    for( const DCH_COMPONENT& comp : m_components )
    {
        int sheetIdx = comp.sheetIndex;

        if( sheetIdx < 0 || sheetIdx >= m_numSheets )
            sheetIdx = 0;

        SCH_SCREEN* screen = getOrCreateSheet( sheetIdx );

        try
        {
            createSymbolInstance( comp, screen );
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

    if( !m_libSymbols.empty() )
    {
        wxFileName libFileName;
        libFileName.Assign( m_schematic->Project().GetProjectPath(), getLibName(),
                            FILEEXT::KiCadSymbolLibFileExtension );

        bool libSaved = false;

        // Step 1: Save the symbol library file. This is independent of table registration
        // and should succeed even in headless/CLI contexts.
        try
        {
            IO_RELEASER<SCH_IO> pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );

            if( pi )
            {
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
                        wxString libTableUri =
                                wxT( "${KIPRJMOD}/" ) + libFileName.GetFullName();

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
                                  m_components.size(), m_buses.size(), m_nets.size(), m_version,
                                  m_numSheets ),
                RPT_SEVERITY_INFO );
    }
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sch_io/orcad/sch_io_orcad.h>
#include <sch_io/orcad/orcad_cache.h>
#include <sch_io/orcad/orcad_converter.h>
#include <sch_io/orcad/orcad_ole.h>
#include <sch_io/orcad/orcad_page.h>

#include <schematic.h>
#include <connection_graph.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_sheet_pin.h>
#include <sch_sheet_path.h>
#include <sch_symbol.h>
#include <sch_label.h>
#include <sch_line.h>
#include <sch_bitmap.h>
#include <sch_shape.h>
#include <sch_pin.h>
#include <lib_symbol.h>
#include <reporter.h>
#include <settings/settings_manager.h>

#include <wx/ffile.h>
#include <wx/filefn.h>
#include <wx/filename.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>


namespace
{

static void appendLe32( std::vector<uint8_t>& aBytes, uint32_t aValue )
{
    for( int shift = 0; shift < 32; shift += 8 )
        aBytes.push_back( static_cast<uint8_t>( aValue >> shift ) );
}


static void appendLe16( std::vector<uint8_t>& aBytes, uint16_t aValue )
{
    aBytes.push_back( static_cast<uint8_t>( aValue ) );
    aBytes.push_back( static_cast<uint8_t>( aValue >> 8 ) );
}


static void appendLzt( std::vector<uint8_t>& aBytes, const std::string& aValue )
{
    appendLe16( aBytes, static_cast<uint16_t>( aValue.size() ) );
    aBytes.insert( aBytes.end(), aValue.begin(), aValue.end() );
    aBytes.push_back( 0 );
}


static void writeLe16( std::vector<uint8_t>& aBytes, size_t aOffset, uint16_t aValue )
{
    aBytes[aOffset] = static_cast<uint8_t>( aValue );
    aBytes[aOffset + 1] = static_cast<uint8_t>( aValue >> 8 );
}


static void writeLe32( std::vector<uint8_t>& aBytes, size_t aOffset, uint32_t aValue )
{
    for( int shift = 0; shift < 32; shift += 8 )
        aBytes[aOffset++] = static_cast<uint8_t>( aValue >> shift );
}


static std::vector<uint8_t> makeOlePreviewCfb( const std::vector<uint16_t>& aName, const std::vector<uint8_t>& aStream )
{
    constexpr uint32_t FREE_SECTOR = 0xFFFFFFFF;
    constexpr uint32_t END_OF_CHAIN = 0xFFFFFFFE;
    constexpr uint32_t FAT_SECTOR = 0xFFFFFFFD;
    constexpr size_t   SECTOR_SIZE = 512;
    constexpr size_t   STREAM_SECTORS = 8;

    std::vector<uint8_t> cfb( SECTOR_SIZE * ( 3 + STREAM_SECTORS ), 0 );
    const uint8_t        magic[] = { 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1 };
    std::copy( std::begin( magic ), std::end( magic ), cfb.begin() );
    writeLe16( cfb, 24, 0x003E );
    writeLe16( cfb, 26, 3 );
    writeLe16( cfb, 28, 0xFFFE );
    writeLe16( cfb, 30, 9 );
    writeLe16( cfb, 32, 6 );
    writeLe32( cfb, 44, 1 );
    writeLe32( cfb, 48, 1 );
    writeLe32( cfb, 56, 4096 );
    writeLe32( cfb, 60, END_OF_CHAIN );
    writeLe32( cfb, 68, END_OF_CHAIN );

    for( size_t i = 0; i < 109; ++i )
        writeLe32( cfb, 76 + 4 * i, i == 0 ? 0 : FREE_SECTOR );

    size_t fat = SECTOR_SIZE;
    writeLe32( cfb, fat, FAT_SECTOR );
    writeLe32( cfb, fat + 4, END_OF_CHAIN );

    for( size_t i = 0; i < STREAM_SECTORS; ++i )
        writeLe32( cfb, fat + 4 * ( 2 + i ), i + 1 == STREAM_SECTORS ? END_OF_CHAIN : 3 + i );

    for( size_t i = 2 + STREAM_SECTORS; i < SECTOR_SIZE / 4; ++i )
        writeLe32( cfb, fat + 4 * i, FREE_SECTOR );

    size_t root = 2 * SECTOR_SIZE;
    writeLe16( cfb, root, 'R' );
    writeLe16( cfb, root + 2, 0 );
    writeLe16( cfb, root + 64, 4 );
    cfb[root + 66] = 5;
    writeLe32( cfb, root + 68, FREE_SECTOR );
    writeLe32( cfb, root + 72, FREE_SECTOR );
    writeLe32( cfb, root + 76, 1 );
    writeLe32( cfb, root + 116, END_OF_CHAIN );

    size_t entry = root + 128;

    for( size_t i = 0; i < aName.size(); ++i )
        writeLe16( cfb, entry + 2 * i, aName[i] );

    writeLe16( cfb, entry + 64, static_cast<uint16_t>( 2 * aName.size() ) );
    cfb[entry + 66] = 2;
    writeLe32( cfb, entry + 68, FREE_SECTOR );
    writeLe32( cfb, entry + 72, FREE_SECTOR );
    writeLe32( cfb, entry + 76, FREE_SECTOR );
    writeLe32( cfb, entry + 116, 2 );
    writeLe32( cfb, entry + 120, STREAM_SECTORS * SECTOR_SIZE );

    std::copy_n( aStream.begin(), std::min( aStream.size(), STREAM_SECTORS * SECTOR_SIZE ),
                 cfb.begin() + 3 * SECTOR_SIZE );
    return cfb;
}

/// Throw-away temp file; impostor fixtures synthesized at runtime since user designs not redistributable.
struct TEMP_TEST_FILE
{
    TEMP_TEST_FILE( const wxString& aFileName, const wxString& aContents ) :
            m_path( wxFileName( wxFileName::GetTempDir(), aFileName ).GetFullPath() )
    {
        wxFFile file( m_path, wxS( "w" ) );

        if( file.IsOpened() )
            file.Write( aContents );
    }

    ~TEMP_TEST_FILE() { wxRemoveFile( m_path ); }

    wxString m_path;
};

} // namespace


struct ORCAD_SCH_IMPORT_FIXTURE
{
    ORCAD_SCH_IMPORT_FIXTURE() :
            m_schematic( new SCHEMATIC( nullptr ) )
    {
        m_manager.LoadProject( "" );
        m_schematic->SetProject( &m_manager.Prj() );
        m_schematic->CurrentSheet().clear();
        m_schematic->CurrentSheet().push_back( &m_schematic->Root() );
    }

    ~ORCAD_SCH_IMPORT_FIXTURE()
    {
        m_schematic.reset();
    }

    std::string dataPath( const std::string& aRelPath ) const
    {
        return KI_TEST::GetEeschemaTestDataDir() + "io/orcad/" + aRelPath;
    }

    SCH_SHEET* LoadOrcadSchematic( const std::string& aRelPath )
    {
        return m_plugin.LoadSchematicFile( dataPath( aRelPath ), m_schematic.get() );
    }

    SCH_IO_ORCAD               m_plugin;
    std::unique_ptr<SCHEMATIC> m_schematic;
    SETTINGS_MANAGER           m_manager;
};


BOOST_FIXTURE_TEST_SUITE( OrcadSchImport, ORCAD_SCH_IMPORT_FIXTURE )


BOOST_AUTO_TEST_CASE( PrimitiveLineWidth )
{
    std::vector<uint8_t> bytes = { ORCAD_PRIM_LINE, ORCAD_PRIM_LINE };
    appendLe32( bytes, 32 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 10 );
    appendLe32( bytes, 20 );
    appendLe32( bytes, 30 );
    appendLe32( bytes, 40 );
    appendLe32( bytes, 1 );
    appendLe32( bytes, 2 );

    ORCAD_STREAM                   stream( bytes.data(), bytes.size() );
    std::optional<ORCAD_PRIMITIVE> primitive = OrcadReadPrimitive( stream );

    BOOST_REQUIRE( primitive );
    BOOST_CHECK( primitive->kind == ORCAD_PRIM_KIND::LINE );
    BOOST_CHECK_EQUAL( primitive->lineWidth, 2 );
}


BOOST_AUTO_TEST_CASE( CaptureColorPalette )
{
    BOOST_CHECK( OrcadColor( 8 ) == KIGFX::COLOR4D( 1.0, 0.0, 0.0, 1.0 ) );
    BOOST_CHECK( OrcadColor( 18 ) == KIGFX::COLOR4D( 0.0, 1.0, 0.0, 1.0 ) );
    BOOST_CHECK( OrcadColor( 28 ) == KIGFX::COLOR4D( 0.0, 0.0, 1.0, 1.0 ) );
    BOOST_CHECK( OrcadColor( 40 ) == KIGFX::COLOR4D( 0.0, 0.0, 0.0, 1.0 ) );
    BOOST_CHECK( OrcadColor( 47 ) == KIGFX::COLOR4D( 1.0, 1.0, 1.0, 1.0 ) );
    BOOST_CHECK( OrcadColor( 48 ) == KIGFX::COLOR4D::UNSPECIFIED );
    BOOST_CHECK( OrcadColor( 0 ) == KIGFX::COLOR4D::UNSPECIFIED );
}


BOOST_AUTO_TEST_CASE( CaptureCompoundFileName )
{
    BOOST_CHECK_EQUAL( OrcadNormalizeCfbName( std::string( "Sch 2" ) + '\x03' + " PCI Connector" ),
                       "Sch 2: PCI Connector" );
}


BOOST_AUTO_TEST_CASE( CaptureStrokeAndFillSemantics )
{
    BOOST_CHECK_EQUAL( OrcadLineWidthIu( 0 ), schIUScale.MilsToIU( 5 ) );
    BOOST_CHECK_EQUAL( OrcadLineWidthIu( 1 ), schIUScale.MilsToIU( 10 ) );
    BOOST_CHECK_EQUAL( OrcadLineWidthIu( 2 ), schIUScale.MilsToIU( 15 ) );
    BOOST_CHECK_EQUAL( OrcadLineWidthIu( 3 ), 0 );

    BOOST_CHECK( OrcadLineStyle( 0 ) == LINE_STYLE::SOLID );
    BOOST_CHECK( OrcadLineStyle( 4 ) == LINE_STYLE::DASHDOTDOT );
    BOOST_CHECK( OrcadLineStyle( 5 ) == LINE_STYLE::DEFAULT );

    BOOST_CHECK( OrcadFillType( 0, 0 ) == FILL_T::FILLED_WITH_BG_BODYCOLOR );
    BOOST_CHECK( OrcadFillType( 1, 0 ) == FILL_T::NO_FILL );
    BOOST_CHECK( OrcadFillType( 2, 0 ) == FILL_T::HATCH );
    BOOST_CHECK( OrcadFillType( 2, 4 ) == FILL_T::REVERSE_HATCH );
    BOOST_CHECK( OrcadFillType( 2, 5 ) == FILL_T::CROSS_HATCH );
}


BOOST_AUTO_TEST_CASE( CapturePageOrder )
{
    wxString dashed = wxS( "03 - CAN" );
    wxString dotted = wxS( "02.uC" );
    wxString colon = wxS( "13:IMU" );
    wxString folder = wxS( "Sch 7: CAN Drivers" );
    wxString pager = wxS( "PAGER 8" );
    wxString plain = wxS( "Overview" );

    BOOST_CHECK_EQUAL( OrcadPageOrder( dashed ), 3 );
    BOOST_CHECK_EQUAL( dashed, wxS( "CAN" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( dotted ), 2 );
    BOOST_CHECK_EQUAL( dotted, wxS( "02.uC" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( colon ), 13 );
    BOOST_CHECK_EQUAL( colon, wxS( "13:IMU" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( folder ), 7 );
    BOOST_CHECK_EQUAL( folder, wxS( "Sch 7: CAN Drivers" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( pager ), -1 );
    BOOST_CHECK_EQUAL( pager, wxS( "PAGER 8" ) );
    BOOST_CHECK_EQUAL( OrcadPageOrder( plain ), -1 );
    BOOST_CHECK_EQUAL( plain, wxS( "Overview" ) );
}


BOOST_AUTO_TEST_CASE( PrimitiveStrokeAndFillStyles )
{
    std::vector<uint8_t> bytes = { ORCAD_PRIM_RECT, ORCAD_PRIM_RECT };
    appendLe32( bytes, 40 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 10 );
    appendLe32( bytes, 20 );
    appendLe32( bytes, 30 );
    appendLe32( bytes, 40 );
    appendLe32( bytes, 2 );
    appendLe32( bytes, 3 );
    appendLe32( bytes, 2 );
    appendLe32( bytes, 5 );

    ORCAD_STREAM                   stream( bytes.data(), bytes.size() );
    std::optional<ORCAD_PRIMITIVE> primitive = OrcadReadPrimitive( stream );

    BOOST_REQUIRE( primitive );
    BOOST_CHECK_EQUAL( primitive->lineStyle, 2 );
    BOOST_CHECK_EQUAL( primitive->lineWidth, 3 );
    BOOST_CHECK_EQUAL( primitive->fillStyle, 2 );
    BOOST_CHECK_EQUAL( primitive->hatchStyle, 5 );
}


BOOST_AUTO_TEST_CASE( PrimitivePolygonStylesBeforePoints )
{
    std::vector<uint8_t> bytes = { ORCAD_PRIM_POLYGON, ORCAD_PRIM_POLYGON };
    appendLe32( bytes, 34 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 1 );
    appendLe32( bytes, 2 );
    appendLe32( bytes, 2 );
    appendLe32( bytes, 4 );
    appendLe16( bytes, 2 );
    appendLe16( bytes, 20 );
    appendLe16( bytes, 10 );
    appendLe16( bytes, 40 );
    appendLe16( bytes, 30 );

    ORCAD_STREAM                   stream( bytes.data(), bytes.size() );
    std::optional<ORCAD_PRIMITIVE> primitive = OrcadReadPrimitive( stream );

    BOOST_REQUIRE( primitive );
    BOOST_CHECK_EQUAL( primitive->lineStyle, 1 );
    BOOST_CHECK_EQUAL( primitive->lineWidth, 2 );
    BOOST_CHECK_EQUAL( primitive->fillStyle, 2 );
    BOOST_CHECK_EQUAL( primitive->hatchStyle, 4 );
    BOOST_REQUIRE_EQUAL( primitive->points.size(), 2u );
    BOOST_CHECK( ( primitive->points[0] == ORCAD_POINT{ 10, 20 } ) );
    BOOST_CHECK( ( primitive->points[1] == ORCAD_POINT{ 30, 40 } ) );
}


BOOST_AUTO_TEST_CASE( PrimitiveSymbolVectorContents )
{
    std::vector<uint8_t> bytes = { ORCAD_PRIM_SYMBOL_VECTOR, ORCAD_PRIM_SYMBOL_VECTOR };
    appendLe32( bytes, 63 );
    appendLe32( bytes, 0 );
    bytes.push_back( ORCAD_PRIM_SYMBOL_VECTOR );
    appendLe16( bytes, 0 );
    bytes.insert( bytes.end(), std::begin( ORCAD_STREAM::PREAMBLE ), std::end( ORCAD_STREAM::PREAMBLE ) );
    appendLe32( bytes, 0 );
    appendLe16( bytes, 12 );
    appendLe16( bytes, 26 );
    appendLe16( bytes, 1 );
    bytes.insert( bytes.end(), { ORCAD_PRIM_POLYLINE, 0, ORCAD_PRIM_POLYLINE } );
    appendLe32( bytes, 30 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0 );
    appendLe32( bytes, 0 );
    appendLe16( bytes, 3 );
    appendLe16( bytes, 8 );
    appendLe16( bytes, 4 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 4 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 16 );
    appendLe16( bytes, 10 );
    bytes.insert( bytes.end(), { 'H', 'y', 's', 't', 'e', 'r', 'e', 's', 'i', 's', 0 } );

    ORCAD_STREAM                   stream( bytes.data(), bytes.size() );
    std::optional<ORCAD_PRIMITIVE> primitive = OrcadReadPrimitive( stream );

    BOOST_REQUIRE( primitive );
    BOOST_CHECK( primitive->kind == ORCAD_PRIM_KIND::GROUP );
    BOOST_CHECK_EQUAL( primitive->x1, 12 );
    BOOST_CHECK_EQUAL( primitive->y1, 26 );
    BOOST_REQUIRE_EQUAL( primitive->children.size(), 1u );
    BOOST_CHECK( primitive->children[0].kind == ORCAD_PRIM_KIND::POLYLINE );
    BOOST_REQUIRE_EQUAL( primitive->children[0].points.size(), 3u );
    BOOST_CHECK( ( primitive->children[0].points[2] == ORCAD_POINT{ 16, 0 } ) );
}


BOOST_AUTO_TEST_CASE( OleMetafilePreviewExtraction )
{
    std::vector<uint8_t> presentation( 4096, 0 );
    writeLe32( presentation, 4, 14 );
    presentation[40] = 1;
    presentation[41] = 0;
    presentation[42] = 9;
    presentation[43] = 0;

    std::vector<uint16_t> name = { 2, 'O', 'l', 'e', 'P', 'r', 'e', 's', '0', '0', '0', 0 };
    ORCAD_OLE_PREVIEW     preview = OrcadExtractOlePreview( makeOlePreviewCfb( name, presentation ) );

    BOOST_CHECK( preview.type == ORCAD_OLE_PREVIEW_TYPE::WMF );
    BOOST_REQUIRE_EQUAL( preview.data.size(), presentation.size() - 40 );
    BOOST_CHECK_EQUAL( preview.data[0], 1 );
    BOOST_CHECK_EQUAL( preview.data[2], 9 );
}


BOOST_AUTO_TEST_CASE( OleNativeBitmapExtraction )
{
    std::vector<uint8_t> native( 4096, 0 );
    writeLe32( native, 0, 58 );
    native[4] = 'B';
    native[5] = 'M';
    writeLe32( native, 6, 58 );

    std::vector<uint16_t> name = { 1, 'O', 'l', 'e', '1', '0', 'N', 'a', 't', 'i', 'v', 'e', 0 };
    ORCAD_OLE_PREVIEW     preview = OrcadExtractOlePreview( makeOlePreviewCfb( name, native ) );

    BOOST_CHECK( preview.type == ORCAD_OLE_PREVIEW_TYPE::BMP );
    BOOST_REQUIRE_EQUAL( preview.data.size(), native.size() - 4 );
    BOOST_CHECK_EQUAL( preview.data[0], 'B' );
    BOOST_CHECK_EQUAL( preview.data[1], 'M' );
}


BOOST_AUTO_TEST_CASE( LegacyPageNetGroups )
{
    std::vector<uint8_t> bytes = { ORCAD_ST_PAGE, 0, 0 };
    appendLzt( bytes, "PAGE" );
    appendLzt( bytes, "C" );
    bytes.resize( bytes.size() + 156 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 0 );
    appendLe16( bytes, 1 );
    appendLe32( bytes, 0x12345678 );
    appendLzt( bytes, "BUS[1:0]" );
    appendLe16( bytes, 2 );
    appendLe32( bytes, 0x11111111 );
    appendLe32( bytes, 0x22222222 );

    for( int i = 0; i < 10; ++i )
        appendLe16( bytes, 0 );

    std::vector<char> data( bytes.begin(), bytes.end() );
    ORCAD_RAW_PAGE    page = OrcadParsePageV2( data, {},
                                               []( const wxString& )
                                               {
                                            } );

    BOOST_REQUIRE_EQUAL( page.netGroups.size(), 1u );
    BOOST_CHECK_EQUAL( page.netGroups[0].id, 0x12345678u );
    BOOST_CHECK_EQUAL( page.netGroups[0].name, "BUS[1:0]" );
    BOOST_REQUIRE_EQUAL( page.netGroups[0].members.size(), 2u );
    BOOST_CHECK_EQUAL( page.netGroups[0].members[1], 0x22222222u );
}


// ============================================================================
// File discrimination tests
//
// .dsn shared with SPECCTRA PCB text files; OrCAD Capture is OLE2/CFB (magic
// D0 CF 11 E0...) with a "Library" stream and "Views"/"Schematics" storage.
// Anything failing those checks rejected.
// ============================================================================

BOOST_AUTO_TEST_CASE( RejectsSpecctraTextDsn )
{
    TEMP_TEST_FILE specctra( wxS( "qa_orcad_specctra_impostor.dsn" ),
                             wxS( "(pcb \"impostor.dsn\"\n  (parser\n    (string_quote \")\n  )\n)\n" ) );

    BOOST_REQUIRE( wxFileName::FileExists( specctra.m_path ) );
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( specctra.m_path ) );
}


BOOST_AUTO_TEST_CASE( RejectsNonexistentFile )
{
    wxFileName missing( wxFileName::GetTempDir(), wxS( "qa_orcad_does_not_exist.dsn" ) );

    BOOST_REQUIRE( !missing.FileExists() );
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( missing.GetFullPath() ) );
}


BOOST_AUTO_TEST_CASE( RejectsWrongExtension )
{
    TEMP_TEST_FILE textFile( wxS( "qa_orcad_impostor.txt" ),
                             wxS( "Just some text, not a schematic.\n" ) );

    BOOST_REQUIRE( wxFileName::FileExists( textFile.m_path ) );
    BOOST_CHECK( !m_plugin.CanReadSchematicFile( textFile.m_path ) );
}


// No positive-load test until a redistributable .dsn fixture exists under qa/data/eeschema/io/orcad/.


// ============================================================================
// Corpus validation (opt-in via KICAD_ORCAD_CORPUS)
//
// Sample designs not redistributable; skipped unless KICAD_ORCAD_CORPUS names a
// tree of .DSN files. Each design imported, its placed-component refdes set
// cross-checked against OrCAD-exported ground truth beside it: .NET (Cadstar
// RINF, ".ADD_COM <ref>" per part) and/or .BOM (tab-separated, comma-joined
// reference cell). Refdes coverage is the strongest available import check.
// ============================================================================

static std::string trimCell( std::string aText )
{
    auto notSpace = []( unsigned char c ) { return !std::isspace( c ); };
    aText.erase( aText.begin(), std::find_if( aText.begin(), aText.end(), notSpace ) );
    aText.erase( std::find_if( aText.rbegin(), aText.rend(), notSpace ).base(), aText.end() );

    if( aText.size() >= 2 && aText.front() == '"' && aText.back() == '"' )
        aText = aText.substr( 1, aText.size() - 2 );

    return aText;
}


static std::vector<std::string> splitRefs( const std::string& aCell )
{
    std::vector<std::string> refs;
    std::string              token;

    for( char c : aCell )
    {
        if( c == ',' )
        {
            std::string r = trimCell( token );

            if( !r.empty() )
                refs.push_back( r );

            token.clear();
        }
        else
        {
            token += c;
        }
    }

    std::string r = trimCell( token );

    if( !r.empty() )
        refs.push_back( r );

    return refs;
}


static std::set<std::string> parseBomRefs( const std::string& aPath )
{
    std::set<std::string> refs;
    std::ifstream         in( aPath );
    std::string           line;
    int                   refCol = -1;

    while( std::getline( in, line ) )
    {
        if( !line.empty() && line.back() == '\r' )
            line.pop_back();

        std::vector<std::string> cols;
        std::string              cell;

        for( char c : line )
        {
            if( c == '\t' ) { cols.push_back( cell ); cell.clear(); }
            else            { cell += c; }
        }

        cols.push_back( cell );

        // Header row names reference column; capture index once
        if( refCol < 0 )
        {
            for( size_t i = 0; i < cols.size(); ++i )
            {
                if( trimCell( cols[i] ) == "Reference" )
                {
                    refCol = static_cast<int>( i );
                    break;
                }
            }

            continue;
        }

        if( refCol < static_cast<int>( cols.size() ) )
        {
            for( const std::string& r : splitRefs( trimCell( cols[refCol] ) ) )
                refs.insert( r );
        }
    }

    return refs;
}


static std::set<std::string> parseNetComs( const std::string& aPath )
{
    std::set<std::string> refs;
    std::ifstream         in( aPath );
    std::string           line;

    while( std::getline( in, line ) )
    {
        if( line.rfind( ".ADD_COM", 0 ) != 0 )
            continue;

        // .ADD_COM <ref> "<footprint>"
        std::string rest = trimCell( line.substr( 8 ) );
        std::string ref;

        for( char c : rest )
        {
            if( std::isspace( static_cast<unsigned char>( c ) ) )
                break;

            ref += c;
        }

        if( !ref.empty() )
            refs.insert( ref );
    }

    return refs;
}


/// Unique refdes of real (non-power) parts. Annotated refs live in per-sheet-path instance data,
/// so walk sheet list and query GetRef() per path (same screen may recur on several paths).
static std::set<std::string> collectImportedRefs( SCHEMATIC& aSchematic )
{
    std::set<std::string> refs;
    SCH_SHEET_LIST        sheets = aSchematic.BuildSheetListSortedByPageNumbers();

    for( const SCH_SHEET_PATH& path : sheets )
    {
        SCH_SCREEN* screen = path.LastScreen();

        if( !screen )
            continue;

        for( SCH_ITEM* item : screen->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
            wxString    ref = symbol->GetRef( &path, false );

            // Leading '#' = power/hidden pseudo-part, not BOM; trailing '?' = unannotated.
            if( ref.IsEmpty() || ref.StartsWith( wxS( "#" ) ) || ref.EndsWith( wxS( "?" ) ) )
                continue;

            refs.insert( std::string( ref.ToUTF8() ) );
        }
    }

    return refs;
}


static std::string terminalToken( const std::string& aRef, const std::string& aPin )
{
    return trimCell( aRef ) + "." + trimCell( aPin );
}


/// Net terminal sets from .NET. Net starts at `.ADD_TER <ref> <pin>`, gathers following
/// `.TER`/indented `<ref> <pin>` continuations until next net.
static std::vector<std::set<std::string>> parseNetTerminals( const std::string& aPath )
{
    std::vector<std::set<std::string>> nets;
    std::set<std::string>              current;
    std::ifstream                      in( aPath );
    std::string                        line;

    auto flush = [&]()
    {
        if( current.size() >= 2 )
            nets.push_back( current );

        current.clear();
    };

    while( std::getline( in, line ) )
    {
        if( !line.empty() && line.back() == '\r' )
            line.pop_back();

        bool addTer = line.rfind( ".ADD_TER", 0 ) == 0;
        bool ter = line.rfind( ".TER", 0 ) == 0;
        bool cont = !line.empty() && std::isspace( static_cast<unsigned char>( line[0] ) );

        if( line.rfind( ".END", 0 ) == 0 )
            break;

        if( addTer )
            flush();

        if( addTer || ter || cont )
        {
            std::istringstream ss( addTer ? line.substr( 8 ) : ter ? line.substr( 4 ) : line );
            std::string        ref, pin;

            if( ss >> ref >> pin )
                current.insert( terminalToken( ref, pin ) );
        }
    }

    flush();
    return nets;
}


/// Count ground-truth nets whose resolvable terminals all land on one KiCad net after
/// connectivity rebuild. Returns {consistent, checkable}.
static std::pair<int, int> checkConnectivity( SCHEMATIC& aSchematic, const std::vector<std::set<std::string>>& aNets,
                                              std::vector<std::set<std::string>>* aInconsistent = nullptr )
{
    SCH_SHEET_LIST sheets = aSchematic.BuildSheetListSortedByPageNumbers();
    aSchematic.ConnectionGraph()->Recalculate( sheets, true );

    std::map<std::string, int> pinNet;
    int                        netId = 0;

    for( const auto& [key, subgraphs] : aSchematic.ConnectionGraph()->GetNetMap() )
    {
        for( CONNECTION_SUBGRAPH* subgraph : subgraphs )
        {
            for( SCH_ITEM* item : subgraph->GetItems() )
            {
                if( item->Type() != SCH_PIN_T )
                    continue;

                SCH_PIN*    pin = static_cast<SCH_PIN*>( item );
                SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( pin->GetParentSymbol() );

                if( !symbol )
                    continue;

                wxString ref = symbol->GetRef( &subgraph->GetSheet(), false );

                if( ref.IsEmpty() || ref.StartsWith( wxS( "#" ) ) || ref.EndsWith( wxS( "?" ) ) )
                    continue;

                pinNet[terminalToken( std::string( ref.ToUTF8() ),
                                      std::string( pin->GetNumber().ToUTF8() ) )] = netId;
            }
        }

        ++netId;
    }

    struct CHECKED_NET
    {
        const std::set<std::string>* terminals;
        std::set<int>                ids;
    };

    std::vector<CHECKED_NET> checkedNets;
    std::map<int, int>       sourceNetsPerImportedNet;

    for( const std::set<std::string>& net : aNets )
    {
        std::set<int> ids;
        int           resolved = 0;

        for( const std::string& term : net )
        {
            auto it = pinNet.find( term );

            if( it != pinNet.end() )
            {
                ids.insert( it->second );
                ++resolved;
            }
        }

        if( resolved >= 2 )
        {
            if( ids.size() == 1 )
                sourceNetsPerImportedNet[*ids.begin()]++;

            checkedNets.push_back( { &net, std::move( ids ) } );
        }
    }

    int consistent = 0;

    for( const CHECKED_NET& net : checkedNets )
    {
        bool exact = net.ids.size() == 1 && sourceNetsPerImportedNet[*net.ids.begin()] == 1;

        if( exact )
            ++consistent;
        else if( aInconsistent )
            aInconsistent->push_back( *net.terminals );
    }

    return { consistent, static_cast<int>( checkedNets.size() ) };
}


/// Ground-truth companion beside .DSN, .NET preferred over .BOM.
static std::set<std::string> expectedRefsFor( const std::filesystem::path& aDsn,
                                              std::string& aSource )
{
    for( const char* ext : { ".NET", ".net", ".BOM", ".bom" } )
    {
        std::filesystem::path candidate = aDsn;
        candidate.replace_extension( ext );

        if( std::filesystem::exists( candidate ) )
        {
            aSource = candidate.filename().string();

            bool isNet = std::string( ext ) == ".NET" || std::string( ext ) == ".net";
            return isNet ? parseNetComs( candidate.string() )
                         : parseBomRefs( candidate.string() );
        }
    }

    aSource.clear();
    return {};
}


BOOST_AUTO_TEST_CASE( CorpusValidation )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping OrCAD corpus validation." );
        return;
    }

    namespace fs = std::filesystem;
    fs::path root( corpusEnv );

    BOOST_REQUIRE_MESSAGE( fs::exists( root ), "KICAD_ORCAD_CORPUS path does not exist." );

    std::vector<fs::path> designs;

    for( auto it = fs::recursive_directory_iterator( root,
                                                     fs::directory_options::skip_permission_denied );
         it != fs::recursive_directory_iterator(); ++it )
    {
        if( !it->is_regular_file() )
            continue;

        std::string ext = it->path().extension().string();
        std::transform( ext.begin(), ext.end(), ext.begin(),
                        []( unsigned char c ) { return std::tolower( c ); } );

        if( ext == ".dsn" )
            designs.push_back( it->path() );
    }

    std::sort( designs.begin(), designs.end() );

    BOOST_TEST_MESSAGE( "OrCAD corpus: " << designs.size() << " .DSN files under " << root );

    int          imported = 0, crashed = 0, unsupported = 0, rejected = 0, checked = 0;
    unsigned int totalExpected = 0, totalMatched = 0, totalMissing = 0, totalExtra = 0;
    int          netConsistent = 0, netCheckable = 0, netTotal = 0;
    uint64_t     importedPages = 0, importedComponents = 0, importedPowerSymbols = 0;
    uint64_t     importedPins = 0, importedWires = 0, importedBuses = 0;
    uint64_t     importedLabels = 0, importedShapes = 0, importedTexts = 0, importedBitmaps = 0;

    const char* debugEnv = std::getenv( "KICAD_ORCAD_DEBUG" );
    std::string debugFilter = debugEnv ? debugEnv : "";
    const char* filterEnv = std::getenv( "KICAD_ORCAD_FILTER" );
    std::string designFilter = filterEnv ? filterEnv : "";

    for( const fs::path& dsn : designs )
    {
        std::string rel = fs::relative( dsn, root ).string();

        if( !designFilter.empty() && rel.find( designFilter ) == std::string::npos )
            continue;

        SCH_IO_ORCAD plugin;
        uint64_t     perDesignPages = 0, perDesignComponents = 0, perDesignPowerSymbols = 0;
        uint64_t     perDesignPins = 0, perDesignWires = 0, perDesignBuses = 0;
        uint64_t     perDesignLabels = 0, perDesignShapes = 0, perDesignTexts = 0;
        uint64_t     perDesignBitmaps = 0;
        uint64_t     perDesignRedWires = 0;
        uint64_t     perDesignVddmPowerSymbols = 0;

        bool debug = !debugFilter.empty() && rel.find( debugFilter ) != std::string::npos;

        if( !plugin.CanReadSchematicFile( dsn.string() ) )
        {
            ++rejected;
            continue;
        }

        std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
        SETTINGS_MANAGER           manager;
        manager.LoadProject( "" );
        schematic->SetProject( &manager.Prj() );
        schematic->CurrentSheet().clear();
        schematic->CurrentSheet().push_back( &schematic->Root() );

        WX_STRING_REPORTER reporter;
        plugin.SetReporter( &reporter );

        try
        {
            plugin.LoadSchematicFile( dsn.string(), schematic.get() );
            schematic->CurrentSheet().UpdateAllScreenReferences();
        }
        catch( const std::exception& e )
        {
            // Pre-2003 designs out of scope, rejected cleanly
            if( std::string( e.what() ).find( "pre-2003" ) != std::string::npos )
                ++unsupported;
            else
                ++crashed;

            BOOST_TEST_MESSAGE( "  THROW   " << rel << " : " << e.what() );
            continue;
        }

        ++imported;

        for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
        {
            SCH_SCREEN* screen = path.LastScreen();

            if( !screen )
                continue;

            ++importedPages;
            ++perDesignPages;

            for( SCH_ITEM* item : screen->Items() )
            {
                switch( item->Type() )
                {
                case SCH_SYMBOL_T:
                {
                    SCH_SYMBOL* symbol = static_cast<SCH_SYMBOL*>( item );
                    wxString    ref = symbol->GetRef( &path, false );

                    if( ref.StartsWith( wxS( "#" ) ) )
                    {
                        ++importedPowerSymbols;
                        ++perDesignPowerSymbols;

                        if( symbol->GetValue( false, &path, false ) == wxS( "VDDM" ) )
                            ++perDesignVddmPowerSymbols;
                    }
                    else
                    {
                        ++importedComponents;
                        ++perDesignComponents;
                    }

                    size_t pinCount = symbol->GetPins( &path ).size();
                    importedPins += pinCount;
                    perDesignPins += pinCount;
                    break;
                }

                case SCH_LINE_T:
                {
                    SCH_LINE* line = static_cast<SCH_LINE*>( item );

                    if( line->GetLayer() == LAYER_BUS )
                    {
                        ++importedBuses;
                        ++perDesignBuses;
                    }
                    else if( line->GetLayer() == LAYER_WIRE )
                    {
                        ++importedWires;
                        ++perDesignWires;

                        if( line->GetLineColor() == OrcadColor( 8 ) )
                            ++perDesignRedWires;
                    }

                    break;
                }

                case SCH_LABEL_T:
                case SCH_GLOBAL_LABEL_T:
                case SCH_HIER_LABEL_T:
                    ++importedLabels;
                    ++perDesignLabels;
                    break;

                case SCH_SHAPE_T:
                    ++importedShapes;
                    ++perDesignShapes;
                    break;

                case SCH_TEXT_T:
                    ++importedTexts;
                    ++perDesignTexts;
                    break;

                case SCH_BITMAP_T:
                    ++importedBitmaps;
                    ++perDesignBitmaps;
                    break;

                default: break;
                }
            }
        }

        BOOST_TEST_MESSAGE( "  AUDIT   " << rel << "|pages=" << perDesignPages << "|components=" << perDesignComponents
                                         << "|power=" << perDesignPowerSymbols << "|pins=" << perDesignPins
                                         << "|wires=" << perDesignWires << "|buses=" << perDesignBuses
                                         << "|labels=" << perDesignLabels << "|shapes=" << perDesignShapes
                                         << "|texts=" << perDesignTexts << "|bitmaps=" << perDesignBitmaps );

        if( rel == "allegro/beagleboard-xm/SCH/BeagleBoard-xM_ORCAD.DSN" )
        {
            BOOST_CHECK_EQUAL( perDesignBitmaps, 10u );
            BOOST_CHECK_EQUAL( perDesignShapes, 108u );
            BOOST_CHECK_EQUAL( perDesignTexts, 175u );
        }

        if( rel
            == "allegro/OpenCellular-LED/Rev-C/schematic/"
               "OpenCellular_Connect-1_LED_Life-3_Schematic.DSN" )
        {
            BOOST_CHECK_EQUAL( perDesignVddmPowerSymbols, 31u );
        }

        if( rel
            == "orcad/OpenCellular-GBC-Elgon_ARM/Rev-A/schematics/"
               "CN81XX_GBCV2_sch_0530.DSN" )
        {
            BOOST_CHECK_EQUAL( perDesignRedWires, 35u );
        }

        if( debug )
            BOOST_TEST_MESSAGE( "  DEBUG   " << rel << " warnings:\n"
                                             << std::string( reporter.GetMessages().ToUTF8() ) );

        std::set<std::string> got = collectImportedRefs( *schematic );
        std::string           source;
        std::set<std::string> expected = expectedRefsFor( dsn, source );

        if( debug )
            BOOST_TEST_MESSAGE( "  DEBUG   " << rel << " imported " << got.size() << " refs" );

        if( expected.empty() )
            continue;

        std::set<std::string> missing, extra;
        std::set_difference( expected.begin(), expected.end(), got.begin(), got.end(),
                             std::inserter( missing, missing.begin() ) );
        std::set_difference( got.begin(), got.end(), expected.begin(), expected.end(),
                             std::inserter( extra, extra.begin() ) );

        unsigned int matched = static_cast<unsigned int>( expected.size() - missing.size() );

        ++checked;
        totalExpected += expected.size();
        totalMatched += matched;
        totalMissing += missing.size();
        totalExtra += extra.size();

        BOOST_TEST_MESSAGE( "  CHECK   " << rel << " : " << matched << "/" << expected.size() << " refs ("
                                         << int( 100.0 * matched / expected.size() ) << "%), extra " << extra.size()
                                         << " [" << source << "]" );

        if( debug )
        {
            for( const std::string& ref : missing )
                BOOST_TEST_MESSAGE( "          missing ref: " << ref );

            for( const std::string& ref : extra )
                BOOST_TEST_MESSAGE( "          extra ref: " << ref );
        }

        // .NET ground truth carries terminal connectivity; verify pins group per net after rebuild.
        std::filesystem::path net = dsn;
        net.replace_extension( source.size() >= 4 && source.substr( source.size() - 4 ) == ".net"
                                       ? ".net"
                                       : ".NET" );

        if( std::filesystem::exists( net ) )
        {
            std::vector<std::set<std::string>> nets = parseNetTerminals( net.string() );

            if( !nets.empty() )
            {
                std::vector<std::set<std::string>> inconsistent;
                auto [consistent, checkableNets] = checkConnectivity( *schematic, nets, &inconsistent );
                netConsistent += consistent;
                netCheckable += checkableNets;
                netTotal += static_cast<int>( nets.size() );

                BOOST_TEST_MESSAGE( "          connectivity: " << consistent << "/" << checkableNets
                                                               << " nets consistent" );

                if( debug )
                {
                    for( const std::set<std::string>& terminals : inconsistent )
                    {
                        std::string joined;

                        for( const std::string& terminal : terminals )
                        {
                            if( !joined.empty() )
                                joined += ", ";

                            joined += terminal;
                        }

                        BOOST_TEST_MESSAGE( "          inconsistent net: " << joined );
                    }
                }
            }
        }
    }

    BOOST_TEST_MESSAGE( "==== OrCAD corpus summary ====" );
    BOOST_TEST_MESSAGE( "  designs: " << designs.size() << "  imported: " << imported << "  crashed: " << crashed
                                      << "  unsupported: " << unsupported << "  rejected: " << rejected );
    BOOST_TEST_MESSAGE( "  objects: pages "
                        << importedPages << "  components " << importedComponents << "  power " << importedPowerSymbols
                        << "  pins " << importedPins << "  wires " << importedWires << "  buses " << importedBuses
                        << "  labels " << importedLabels << "  shapes " << importedShapes << "  texts " << importedTexts
                        << "  bitmaps " << importedBitmaps );

    if( checked )
    {
        BOOST_TEST_MESSAGE( "  refdes coverage: " << totalMatched << "/" << totalExpected << " ("
                                                  << int( 100.0 * totalMatched / totalExpected )
                                                  << "%)  missing: " << totalMissing
                                                  << "  extra: " << totalExtra );
    }

    if( netCheckable )
    {
        BOOST_TEST_MESSAGE( "  net connectivity: " << netConsistent << "/" << netCheckable << " ("
                                                   << int( 100.0 * netConsistent / netCheckable )
                                                   << "%)" );
    }

    // Only pre-2003 format may throw; anything else is a crash
    BOOST_CHECK_MESSAGE( crashed == 0, crashed << " design(s) crashed during import." );

    const char* snapshotEnv = std::getenv( "KICAD_ORCAD_CORPUS_SNAPSHOT" );

    // Guard against vacuous pass when no companion files present.
    if( designFilter.empty() )
        BOOST_REQUIRE_MESSAGE( checked > 0, "No ground-truth .BOM/.NET companions were validated." );

    if( designFilter.empty() && snapshotEnv && *snapshotEnv )
    {
        BOOST_CHECK_EQUAL( imported, 92 );
        BOOST_CHECK_EQUAL( rejected, 1 );
        BOOST_CHECK_EQUAL( importedPages, 854u );
        BOOST_CHECK_EQUAL( importedBitmaps, 616u );
    }

    // Occurrence-annotation decode holds this above 95%; dropped Hierarchy-stream ref overlay collapses it.
    if( checked )
        BOOST_CHECK_GE( 100.0 * totalMatched / totalExpected, 95.0 );

    // Pin-placement/geometry regression breaking connectivity collapses this. Checkable floor
    // (>= 2 resolvable terminals per net) stops broad pin loss passing vacuously.
    if( netTotal )
    {
        BOOST_CHECK_GE( 100.0 * netCheckable / netTotal, 80.0 );
        BOOST_CHECK_EQUAL( netConsistent, netCheckable );
    }
}


static std::filesystem::path findCorpusDesign( const std::filesystem::path& aRoot,
                                               const std::string& aFileName )
{
    for( const std::filesystem::directory_entry& entry :
         std::filesystem::recursive_directory_iterator( aRoot ) )
    {
        if( entry.is_regular_file() && entry.path().filename() == aFileName )
            return entry.path();
    }

    return {};
}


BOOST_AUTO_TEST_CASE( Issue25005Hierarchy )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping issue 25005." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "CFW-002.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "CFW-002.DSN not present in corpus; skipping issue 25005." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::vector<SCH_SHEET*> topSheets = schematic->GetTopLevelSheets();
    BOOST_REQUIRE_EQUAL( topSheets.size(), 1u );

    SCH_SCREEN* rootScreen = topSheets.front()->GetScreen();
    size_t      sheets = 0;
    size_t      sheetPins = 0;

    for( SCH_ITEM* item : rootScreen->Items().OfType( SCH_SHEET_T ) )
    {
        SCH_SHEET* sheet = static_cast<SCH_SHEET*>( item );
        ++sheets;
        sheetPins += sheet->GetPins().size();
    }

    const std::vector<wxString> expectedNames = { wxS( "PAG_2" ), wxS( "PAG_3" ), wxS( "PAG_4" ),
                                                  wxS( "PAG_5" ), wxS( "PAG_6" ), wxS( "PAG_7" ),
                                                  wxS( "PAG_8" ), wxS( "PAG_9" ) };
    const std::vector<size_t> expectedPinCounts = { 31, 24, 39, 47, 40, 33, 26, 10 };
    SCH_SHEET_LIST            hierarchy = schematic->BuildSheetListSortedByPageNumbers();
    std::vector<wxString>     sheetNames;
    std::vector<size_t>       pinCounts;

    for( auto it = std::next( hierarchy.begin() ); it != hierarchy.end(); ++it )
    {
        SCH_SHEET*         sheet = it->Last();
        std::set<wxString> sheetPinNames;
        std::set<wxString> hierarchicalLabelNames;

        sheetNames.push_back( sheet->GetField( FIELD_T::SHEET_NAME )->GetText() );
        pinCounts.push_back( sheet->GetPins().size() );

        for( const SCH_SHEET_PIN* pin : sheet->GetPins() )
            sheetPinNames.insert( pin->GetText() );

        for( SCH_ITEM* item : sheet->GetScreen()->Items().OfType( SCH_HIER_LABEL_T ) )
            hierarchicalLabelNames.insert( static_cast<SCH_HIERLABEL*>( item )->GetText() );

        BOOST_CHECK_EQUAL_COLLECTIONS( sheetPinNames.begin(), sheetPinNames.end(),
                                       hierarchicalLabelNames.begin(), hierarchicalLabelNames.end() );
    }

    schematic->ConnectionGraph()->Recalculate( hierarchy, true );

    BOOST_CHECK_EQUAL( hierarchy.size(), 9u );
    BOOST_CHECK_EQUAL( sheets, 8u );
    BOOST_CHECK_EQUAL( sheetPins, 250u );
    BOOST_CHECK_EQUAL_COLLECTIONS( sheetNames.begin(), sheetNames.end(), expectedNames.begin(),
                                   expectedNames.end() );
    BOOST_CHECK_EQUAL_COLLECTIONS( pinCounts.begin(), pinCounts.end(), expectedPinCounts.begin(),
                                   expectedPinCounts.end() );
}


BOOST_AUTO_TEST_CASE( Issue25009PageOrderAndGraphics )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping issue 25009." );
        return;
    }

    std::filesystem::path dsn = findCorpusDesign( corpusEnv, "SE_NGFOC-L_01.DSN" );

    if( dsn.empty() )
    {
        BOOST_TEST_MESSAGE( "SE_NGFOC-L_01.DSN not present in corpus; skipping issue 25009." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    const std::vector<wxString> expectedNames = {
        wxS( "01.REV.HISTORY" ), wxS( "02.uC" ),          wxS( "03.CAN" ),
        wxS( "04. Ethercat" ),   wxS( "05.EtherSynch" ),  wxS( "06.RS-485" ),
        wxS( "11.GPIO" ),        wxS( "12.Analog" ),      wxS( "13:IMU" ),
        wxS( "14.Bridge" ),      wxS( "15.Encoder" ),     wxS( "29.uCPower" ),
        wxS( "30.PowerSupply" ), wxS( "31.Expansion" )
    };

    std::vector<SCH_SHEET*> sheets = schematic->GetTopLevelSheets();
    BOOST_REQUIRE_EQUAL( sheets.size(), expectedNames.size() );

    size_t wires = 0;
    size_t shapes = 0;
    size_t texts = 0;
    size_t tables = 0;

    for( size_t i = 0; i < sheets.size(); ++i )
    {
        BOOST_CHECK_EQUAL( sheets[i]->GetField( FIELD_T::SHEET_NAME )->GetText(), expectedNames[i] );

        for( SCH_ITEM* item : sheets[i]->GetScreen()->Items() )
        {
            if( item->Type() == SCH_LINE_T )
            {
                SCH_LINE* line = static_cast<SCH_LINE*>( item );
                BOOST_CHECK_EQUAL( line->GetLineWidth(), 0 );
                ++wires;
            }
            else if( item->Type() == SCH_SHAPE_T )
            {
                SCH_SHAPE* shape = static_cast<SCH_SHAPE*>( item );

                if( i == 0 )
                    BOOST_CHECK( shape->GetFillMode() == FILL_T::NO_FILL );

                ++shapes;
            }
            else if( item->Type() == SCH_TEXT_T )
            {
                ++texts;
            }
            else if( item->Type() == SCH_TABLE_T )
            {
                ++tables;
            }
        }
    }

    BOOST_CHECK_EQUAL( wires, 1921u );
    BOOST_CHECK_EQUAL( shapes, 168u );
    BOOST_CHECK_EQUAL( texts, 206u );
    BOOST_CHECK_EQUAL( tables, 0u );
}


// ============================================================================
// OLB symbol-library import (opt-in via KICAD_ORCAD_CORPUS)
//
// Each .OLB must enumerate symbols carrying pins or graphics; empty symbol means
// cache decode silently produced nothing.
// ============================================================================

BOOST_AUTO_TEST_CASE( OlbLibraryImport )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping OrCAD OLB library import." );
        return;
    }

    namespace fs = std::filesystem;
    std::vector<fs::path> libs;

    for( auto it = fs::recursive_directory_iterator( fs::path( corpusEnv ),
                                                     fs::directory_options::skip_permission_denied );
         it != fs::recursive_directory_iterator(); ++it )
    {
        if( !it->is_regular_file() )
            continue;

        std::string ext = it->path().extension().string();
        std::transform( ext.begin(), ext.end(), ext.begin(),
                        []( unsigned char c ) { return std::tolower( c ); } );

        if( ext == ".olb" )
            libs.push_back( it->path() );
    }

    std::sort( libs.begin(), libs.end() );
    BOOST_TEST_MESSAGE( "OrCAD OLB libraries: " << libs.size() );

    int totalSymbols = 0, emptySymbols = 0, checkedLibs = 0, crashedLibs = 0;

    for( const fs::path& lib : libs )
    {
        SCH_IO_ORCAD             plugin;
        std::vector<LIB_SYMBOL*> symbols;

        try
        {
            // Vector overload materializes all symbols O(n); per-name LoadSymbol rescans O(n^2).
            plugin.EnumerateSymbolLib( symbols, lib.string() );
        }
        catch( const std::exception& e )
        {
            ++crashedLibs;
            BOOST_TEST_MESSAGE( "  THROW  " << lib.filename().string() << " : " << e.what() );
            continue;
        }

        ++checkedLibs;
        int withGeometry = 0;

        for( LIB_SYMBOL* symbol : symbols )
        {
            BOOST_REQUIRE( symbol );
            ++totalSymbols;

            if( symbol->GetPinCount() > 0 || !symbol->GetDrawItems().empty() )
                ++withGeometry;
            else
                ++emptySymbols;
        }

        BOOST_TEST_MESSAGE( "  " << lib.filename().string() << " : " << symbols.size()
                                 << " symbols, " << withGeometry << " with pins/graphics" );
    }

    BOOST_TEST_MESSAGE( "OLB summary: " << checkedLibs << " libs, " << crashedLibs
                                        << " crashed, " << totalSymbols << " symbols, "
                                        << emptySymbols << " empty" );

    // Bad streams must degrade gracefully, not throw; wholesale empty result means decode broke.
    BOOST_CHECK_EQUAL( crashedLibs, 0 );
    BOOST_CHECK_GT( totalSymbols, 0 );

    if( totalSymbols )
        BOOST_CHECK_LT( emptySymbols, totalSymbols / 2 );
}


// CutiePi (3 pages) imports as three sibling top-level sheets; off-page connectors keep own
// names; reference/value fields honor source display positions.
BOOST_AUTO_TEST_CASE( MultiPageFlatImport )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping OrCAD multi-page import." );
        return;
    }

    namespace fs = std::filesystem;
    fs::path dsn = fs::path( corpusEnv ) / "cutiepi-board" / "CutiePi_V2.3-20210409.DSN";

    if( !fs::exists( dsn ) )
    {
        BOOST_TEST_MESSAGE( "CutiePi design not present in corpus; skipping." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    // Pages become flat ordered top-level sheets, not a stitching root w/ children; "N - " prefix orders them.
    std::vector<SCH_SHEET*> tops = schematic->GetTopLevelSheets();
    BOOST_REQUIRE_EQUAL( tops.size(), 3u );

    BOOST_CHECK_EQUAL( tops[0]->GetField( FIELD_T::SHEET_NAME )->GetText(), wxS( "CONTENTS" ) );
    BOOST_CHECK_EQUAL( tops[1]->GetField( FIELD_T::SHEET_NAME )->GetText(),
                       wxS( "CM4,USB HUB,AUDIO,MIC" ) );
    BOOST_CHECK_EQUAL( tops[2]->GetField( FIELD_T::SHEET_NAME )->GetText(),
                       wxS( "CSI, DSI, HDMI, MCU" ) );

    std::set<wxString> globalLabels;

    for( SCH_SHEET* top : tops )
    {
        for( SCH_ITEM* item : top->GetScreen()->Items().OfType( SCH_GLOBAL_LABEL_T ) )
            globalLabels.insert( static_cast<SCH_LABEL_BASE*>( item )->GetText() );
    }

    // Off-page connectors carry own name (CAM0_IO1), not the local wire net (GPIO19) they sit on.
    BOOST_CHECK( globalLabels.count( wxS( "CAM0_IO1" ) ) );
    BOOST_CHECK( globalLabels.count( wxS( "AMP_SHUTDOWN" ) ) );

    // R3197 reference honors OrCAD display position (left of body), not computed fallback (right).
    bool checkedField = false;

    for( const SCH_SHEET_PATH& path : schematic->BuildSheetListSortedByPageNumbers() )
    {
        for( SCH_ITEM* item : path.LastScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );

            if( sym->GetRef( &path, false ) == wxS( "R3197" ) )
            {
                BOOST_CHECK_LT( sym->GetField( FIELD_T::REFERENCE )->GetPosition().x,
                                sym->GetPosition().x );
                checkedField = true;
            }
        }
    }

    BOOST_CHECK( checkedField );
}


// CutiePi component fidelity: pin number/name visibility, off-page label orientation, hidden
// fields, no-connect markers.
BOOST_AUTO_TEST_CASE( ComponentDetailImport )
{
    const char* corpusEnv = std::getenv( "KICAD_ORCAD_CORPUS" );

    if( !corpusEnv || !*corpusEnv )
    {
        BOOST_TEST_MESSAGE( "KICAD_ORCAD_CORPUS not set; skipping OrCAD component detail." );
        return;
    }

    namespace fs = std::filesystem;
    fs::path dsn = fs::path( corpusEnv ) / "cutiepi-board" / "CutiePi_V2.3-20210409.DSN";

    if( !fs::exists( dsn ) )
    {
        BOOST_TEST_MESSAGE( "CutiePi design not present in corpus; skipping." );
        return;
    }

    std::unique_ptr<SCHEMATIC> schematic( new SCHEMATIC( nullptr ) );
    SETTINGS_MANAGER           manager;
    manager.LoadProject( "" );
    schematic->SetProject( &manager.Prj() );
    schematic->CurrentSheet().clear();
    schematic->CurrentSheet().push_back( &schematic->Root() );

    SCH_IO_ORCAD plugin;
    plugin.LoadSchematicFile( dsn.string(), schematic.get() );

    std::map<wxString, SCH_SYMBOL*> symbols;
    std::multimap<wxString, int>    labelSpins;
    int                             noConnects = 0;

    for( SCH_SHEET* top : schematic->GetTopLevelSheets() )
    {
        SCH_SHEET_PATH path;
        path.push_back( top );

        for( SCH_ITEM* item : top->GetScreen()->Items() )
        {
            if( item->Type() == SCH_SYMBOL_T )
            {
                SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
                symbols[sym->GetRef( &path, false )] = sym;
            }
            else if( item->Type() == SCH_GLOBAL_LABEL_T )
            {
                SCH_LABEL_BASE* lbl = static_cast<SCH_LABEL_BASE*>( item );
                labelSpins.emplace( lbl->GetText(), (int) lbl->GetSpinStyle() );
            }
            else if( item->Type() == SCH_NO_CONNECT_T )
            {
                ++noConnects;
            }
        }
    }

    // Pin numbers/names show on ICs (flags 0x3), hide on passives (0x6)
    BOOST_REQUIRE( symbols.count( wxS( "U3" ) ) );
    BOOST_CHECK( symbols[wxS( "U3" )]->GetShowPinNumbers() );
    BOOST_CHECK( symbols[wxS( "U3" )]->GetShowPinNames() );
    BOOST_REQUIRE( symbols.count( wxS( "R3174" ) ) );
    BOOST_CHECK( !symbols[wxS( "R3174" )]->GetShowPinNumbers() );
    BOOST_CHECK( !symbols[wxS( "R3174" )]->GetShowPinNames() );

    // Ferrite bead value hidden, reference visible
    BOOST_REQUIRE( symbols.count( wxS( "FB8" ) ) );
    BOOST_CHECK( !symbols[wxS( "FB8" )]->GetField( FIELD_T::VALUE )->IsVisible() );
    BOOST_CHECK( symbols[wxS( "FB8" )]->GetField( FIELD_T::REFERENCE )->IsVisible() );

    // Display-prop field positions are canvas-space (anchor + offset), not through body-orientation
    // transform. FB8 (90-deg ferrite) reference lands right of origin; rotation transform would flip left.
    SCH_FIELD* fb8Ref = symbols[wxS( "FB8" )]->GetField( FIELD_T::REFERENCE );
    BOOST_CHECK_GT( fb8Ref->GetPosition().x, symbols[wxS( "FB8" )]->GetPosition().x );
    BOOST_CHECK( fb8Ref->GetHorizJustify() == GR_TEXT_H_ALIGN_LEFT );

    // FB8 stored angle compensates for KiCad re-rotating fields on 90-deg symbol, so text stays horizontal.
    BOOST_CHECK( fb8Ref->GetDrawRotation() == ANGLE_HORIZONTAL );

    // References render horizontal even on rotated symbols (ferrites, vertical R/C).
    for( const wxString& ref : { wxS( "R3186" ), wxS( "C2517" ), wxS( "R3189" ),
                                 wxS( "FB13" ), wxS( "FB9" ) } )
    {
        BOOST_REQUIRE_MESSAGE( symbols.count( ref ), ref );
        BOOST_CHECK( symbols[ref]->GetField( FIELD_T::REFERENCE )->GetDrawRotation()
                     == ANGLE_HORIZONTAL );
    }

    // Value rotation is per-field from source: FB9 part number horizontal, C2517 "47pF" stays vertical.
    BOOST_CHECK( symbols[wxS( "FB9" )]->GetField( FIELD_T::VALUE )->GetDrawRotation()
                 == ANGLE_HORIZONTAL );
    BOOST_CHECK( symbols[wxS( "C2517" )]->GetField( FIELD_T::VALUE )->GetDrawRotation()
                 == ANGLE_VERTICAL );

    // Power net names read horizontal even on rotated power symbols (REG1V8/REG3V3).
    bool checkedPower = false;

    for( SCH_SHEET* top : schematic->GetTopLevelSheets() )
    {
        for( SCH_ITEM* item : top->GetScreen()->Items().OfType( SCH_SYMBOL_T ) )
        {
            SCH_SYMBOL* sym = static_cast<SCH_SYMBOL*>( item );
            wxString    val = sym->GetField( FIELD_T::VALUE )->GetText();

            if( val == wxS( "REG1V8" ) || val == wxS( "REG3V3" ) )
            {
                BOOST_CHECK( sym->GetField( FIELD_T::VALUE )->GetDrawRotation()
                             == ANGLE_HORIZONTAL );
                checkedPower = true;
            }
        }
    }

    BOOST_CHECK( checkedPower );

    // Unconnected IC pins get no-connect markers (U3 15 NC + U580 NC/ORG)
    BOOST_CHECK_GE( noConnects, 17 );

    // Off-page connectors on vertical wires point up/down, not left/right.
    auto hasSpin = [&]( const wxString& aText, SPIN_STYLE::SPIN aSpin )
    {
        auto range = labelSpins.equal_range( aText );

        for( auto it = range.first; it != range.second; ++it )
        {
            if( it->second == (int) aSpin )
                return true;
        }

        return false;
    };

    BOOST_CHECK( hasSpin( wxS( "VOLDN" ), SPIN_STYLE::UP ) );
    BOOST_CHECK( hasSpin( wxS( "MUTEP" ), SPIN_STYLE::BOTTOM ) );
}


BOOST_AUTO_TEST_SUITE_END()

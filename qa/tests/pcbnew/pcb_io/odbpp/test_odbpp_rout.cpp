/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software: you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>

#include <algorithm>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>

#include <board.h>
#include <board_design_settings.h>
#include <board_stackup_manager/board_stackup.h>
#include <geometry/shape_poly_set.h>
#include <reporter.h>
#include <pcbnew/pcb_io/odbpp/pcb_io_odbpp.h>

#include "odb_test_utils.h"

namespace fs = std::filesystem;

namespace
{
using FIELDS = std::map<std::string, std::string>;

std::vector<FIELDS> MatrixLayers( const fs::path& aRoot )
{
    std::vector<FIELDS> rows;
    FIELDS              fields;
    bool                inLayer = false;

    for( const std::string& line : ReadLines( aRoot / "matrix" / "matrix" ) )
    {
        if( line == "LAYER {" )
        {
            fields.clear();
            inLayer = true;
        }
        else if( inLayer && line == "}" )
        {
            rows.push_back( fields );
            inLayer = false;
        }
        else if( inLayer )
        {
            size_t equals = line.find( '=' );

            if( equals != std::string::npos )
            {
                size_t start = line.find_first_not_of( ' ' );
                fields[line.substr( start, equals - start )] = line.substr( equals + 1 );
            }
        }
    }

    return rows;
}


const FIELDS* FindRow( const std::vector<FIELDS>& aRows, const std::string& aName )
{
    auto it = std::find_if( aRows.begin(), aRows.end(), [&]( const FIELDS& aRow )
                            { return aRow.at( "NAME" ) == aName; } );

    return it == aRows.end() ? nullptr : &*it;
}


std::unique_ptr<BOARD> LoadBoard( const std::string& aPath )
{
    return KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir() + aPath );
}


struct ROUT_SPAN
{
    std::pair<std::string, std::string> m_start;
    std::pair<std::string, std::string> m_end;
};


std::map<int, std::vector<ROUT_SPAN>> RoutChains( const std::vector<std::string>& aLines )
{
    std::map<int, std::vector<ROUT_SPAN>> chains;
    std::string chainAttribute;

    for( const std::string& line : aLines )
    {
        if( line.rfind( "@", 0 ) == 0 && line.find( " .rout_chain" ) != std::string::npos )
            chainAttribute = line.substr( 1, line.find( ' ' ) - 1 ) + "=";
    }

    if( chainAttribute.empty() )
        return chains;

    for( const std::string& line : aLines )
    {
        if( line.rfind( "L ", 0 ) != 0 && line.rfind( "A ", 0 ) != 0 )
            continue;

        std::istringstream fields( line );
        std::string record;
        ROUT_SPAN span;
        fields >> record >> span.m_start.first >> span.m_start.second >> span.m_end.first >> span.m_end.second;

        size_t semicolon = line.find( ';' );

        if( semicolon == std::string::npos )
            continue;

        std::istringstream attributes( line.substr( semicolon + 1 ) );
        std::string attribute;

        while( std::getline( attributes, attribute, ',' ) )
        {
            if( attribute.rfind( chainAttribute, 0 ) == 0 )
                chains[std::stoi( attribute.substr( chainAttribute.size() ) )].push_back( span );
        }
    }

    return chains;
}
}


BOOST_AUTO_TEST_CASE( OdbCopperLayerTypesFollowBoardSetup )
{
    std::unique_ptr<BOARD> board = LoadBoard( "connect/connect.kicad_pcb" );
    BOOST_REQUIRE( board );
    BOOST_REQUIRE( board->GetLayerType( In1_Cu ) == LT_POWER );
    fs::path root = ExportOdb( *board, TempDir().Path() );
    std::vector<FIELDS> rows = MatrixLayers( root );
    const FIELDS* inner = FindRow( rows, "in1.cu" );
    BOOST_REQUIRE( inner );
    BOOST_CHECK_EQUAL( inner->at( "TYPE" ), "POWER_GROUND" );

    board = LoadBoard( "issue24089/issue24089.kicad_pcb" );
    BOOST_REQUIRE( board );
    BOOST_REQUIRE( board->GetLayerType( F_Cu ) == LT_MIXED );
    root = ExportOdb( *board, TempDir().Path() );
    rows = MatrixLayers( root );
    const FIELDS* mixed = FindRow( rows, "f.cu" );
    BOOST_REQUIRE( mixed );
    BOOST_CHECK_EQUAL( mixed->at( "TYPE" ), "MIXED" );
}


BOOST_AUTO_TEST_CASE( OdbRoutLayerCarriesEveryIsland )
{
    std::unique_ptr<BOARD> board = LoadBoard( "odbpp/two_board_islands.kicad_pcb" );
    BOOST_REQUIRE( board );
    SHAPE_POLY_SET outlines;
    BOOST_REQUIRE( board->GetBoardPolygonOutlines( outlines, true ) );
    BOOST_REQUIRE_EQUAL( outlines.OutlineCount(), 2 );
    fs::path root = ExportOdb( *board, TempDir().Path() );
    std::vector<FIELDS> rows = MatrixLayers( root );
    const FIELDS* rout = FindRow( rows, "rout" );
    BOOST_REQUIRE( rout );
    BOOST_CHECK( !FindRow( rows, "edge.cuts" ) );
    BOOST_CHECK_EQUAL( rout->at( "TYPE" ), "ROUT" );
    BOOST_CHECK_EQUAL( rout->at( "START_NAME" ), "f.cu" );
    BOOST_CHECK_EQUAL( rout->at( "END_NAME" ), "b.cu" );
    ODB_PRODUCT product( root );
    std::vector<std::string> lines = ReadLines( fs::path( product.LayerFile( "rout", "features" ).ToStdWstring() ) );
    size_t routes = std::count_if( lines.begin(), lines.end(), []( const std::string& aLine )
                                   { return aLine.rfind( "L ", 0 ) == 0 || aLine.rfind( "A ", 0 ) == 0; } );
    BOOST_CHECK_GT( routes, 7u );
    BOOST_CHECK( std::none_of( lines.begin(), lines.end(), []( const std::string& aLine )
                               { return aLine.rfind( "S ", 0 ) == 0; } ) );
    BOOST_CHECK( std::any_of( lines.begin(), lines.end(), []( const std::string& aLine )
                              { return aLine.rfind( "$0 r0", 0 ) == 0; } ) );
    OdbFeatureFile features( fs::path( product.LayerFile( "rout", "features" ).ToStdWstring() ) );
    BOOST_CHECK_GT( features.CountRecordsWithInteger( ".rout_chain", 1 ), 0u );
    BOOST_CHECK_GT( features.CountRecordsWithInteger( ".rout_chain", 2 ), 0u );

    std::map<int, std::vector<ROUT_SPAN>> chains = RoutChains( lines );
    BOOST_REQUIRE_EQUAL( chains.size(), 2u );

    for( const auto& [number, spans] : chains )
    {
        BOOST_REQUIRE_GT( spans.size(), 2u );

        for( size_t i = 0; i < spans.size(); ++i )
        {
            const ROUT_SPAN& current = spans[i];
            const ROUT_SPAN& next = spans[( i + 1 ) % spans.size()];
            BOOST_CHECK_MESSAGE( current.m_end == next.m_start, "open rout chain " << number << " at " << i );
        }
    }
}


BOOST_AUTO_TEST_CASE( OdbPlatedEdgesAreRoutPlated )
{
    std::unique_ptr<BOARD> board = LoadBoard( "issue24089/issue24089.kicad_pcb" );
    BOOST_REQUIRE( board );
    BOOST_REQUIRE( board->GetDesignSettings().GetStackupDescriptor().m_EdgePlating );
    fs::path root = ExportOdb( *board, TempDir().Path() );
    ODB_PRODUCT product( root );
    fs::path rout( product.LayerFile( "rout", "features" ).ToStdWstring() );
    BOOST_REQUIRE( fs::exists( rout ) );
    OdbFeatureFile features( rout );
    BOOST_CHECK_GT( features.CountRecordsWithFlag( ".rout_plated" ), 0u );
    std::vector<std::string> routLines = ReadLines( rout );
    BOOST_CHECK( std::any_of( routLines.begin(), routLines.end(), []( const std::string& aLine )
                              { return aLine.rfind( "A ", 0 ) == 0; } ) );

    board = LoadBoard( "issue19325/issue19325.kicad_pcb" );
    BOOST_REQUIRE( board );
    root = ExportOdb( *board, TempDir().Path() );
    ODB_PRODUCT castellated( root );
    fs::path castRout( castellated.LayerFile( "rout", "features" ).ToStdWstring() );
    BOOST_REQUIRE( fs::exists( castRout ) );
    OdbFeatureFile castFeatures( castRout );
    BOOST_CHECK_EQUAL( castFeatures.CountRecordsWithFlag( ".rout_plated" ), 0u );
}


BOOST_AUTO_TEST_CASE( OdbMaskPasteSilkReferenceCopper )
{
    std::unique_ptr<BOARD> board = LoadBoard( "issue24089/issue24089.kicad_pcb" );
    BOOST_REQUIRE( board );
    fs::path root = ExportOdb( *board, TempDir().Path() );
    std::vector<FIELDS> rows = MatrixLayers( root );

    for( const std::string& side : { "f", "b" } )
    {
        const FIELDS* copper = FindRow( rows, side + ".cu" );
        BOOST_REQUIRE( copper );
        BOOST_REQUIRE( copper->count( "ID" ) );

        for( const std::string& suffix : { "mask", "paste", "silkscreen" } )
        {
            const FIELDS* row = FindRow( rows, side + "." + suffix );

            if( row )
            {
                BOOST_REQUIRE( row->count( "REF" ) );
                BOOST_CHECK_EQUAL( row->at( "REF" ), copper->at( "ID" ) );
            }
        }
    }

    std::set<std::string> copperIds;

    for( const FIELDS& row : rows )
    {
        if( row.at( "TYPE" ) == "SIGNAL" || row.at( "TYPE" ) == "POWER_GROUND"
            || row.at( "TYPE" ) == "MIXED" )
        {
            copperIds.insert( row.at( "ID" ) );
        }
    }

    for( const FIELDS& row : rows )
    {
        if( row.at( "TYPE" ) != "DIELECTRIC" )
            continue;

        if( row.at( "DIELECTRIC_TYPE" ) == "CORE" )
        {
            BOOST_REQUIRE( row.count( "CU_TOP" ) );
            BOOST_REQUIRE( row.count( "CU_BOTTOM" ) );
            BOOST_REQUIRE( row.count( "DIELECTRIC_NAME" ) );
            BOOST_CHECK( copperIds.count( row.at( "CU_TOP" ) ) );
            BOOST_CHECK( copperIds.count( row.at( "CU_BOTTOM" ) ) );
            BOOST_CHECK( !row.at( "DIELECTRIC_NAME" ).empty() );
        }
        else
        {
            BOOST_CHECK( !row.count( "CU_TOP" ) );
            BOOST_CHECK( !row.count( "CU_BOTTOM" ) );
        }
    }
}


BOOST_AUTO_TEST_CASE( OdbLayerTypeOverrides )
{
    std::unique_ptr<BOARD> board = LoadBoard( "issue24089/issue24089.kicad_pcb" );
    BOOST_REQUIRE( board );
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "odb_layer_type_overrides" ) );
    PCB_IO_ODBPP             plugin;
    ODB_EXPORT_OPTIONS       options;
    options.m_layerOverrides = { { F_SilkS, true, wxEmptyString, wxS( "DOCUMENT" ) },
                                 { Dwgs_User, true, wxEmptyString, wxS( "SOLDER_MASK" ) },
                                 { In1_Cu, true, wxEmptyString, wxS( "SIGNAL" ) },
                                 { B_Cu, true, wxEmptyString, wxS( "SILK_SCREEN" ) } };
    BOOST_REQUIRE_NO_THROW( plugin.Export( wxString::FromUTF8( dir.Path().string() ), *board, options ) );
    std::vector<FIELDS> rows = MatrixLayers( dir.Path() );

    const FIELDS* silk = FindRow( rows, "f.silkscreen" );
    BOOST_REQUIRE( silk );
    BOOST_CHECK_EQUAL( silk->at( "TYPE" ), "DOCUMENT" );
    BOOST_CHECK( !silk->contains( "REF" ) );

    const FIELDS* drawings = FindRow( rows, "user.drawings" );
    BOOST_REQUIRE( drawings );
    BOOST_CHECK_EQUAL( drawings->at( "TYPE" ), "SOLDER_MASK" );
    BOOST_CHECK_EQUAL( drawings->at( "CONTEXT" ), "MISC" );
    BOOST_CHECK( !drawings->contains( "REF" ) );

    const FIELDS* inner = FindRow( rows, "in1.cu" );
    BOOST_REQUIRE( inner );
    BOOST_CHECK_EQUAL( inner->at( "TYPE" ), "SIGNAL" );

    // Copper cannot take a drawing layer type
    const FIELDS* bottom = FindRow( rows, "b.cu" );
    BOOST_REQUIRE( bottom );
    BOOST_CHECK_EQUAL( bottom->at( "TYPE" ), "MIXED" );
}


BOOST_AUTO_TEST_CASE( OdbMatrixIdsAreUnique )
{
    std::unique_ptr<BOARD> board = LoadBoard( "issue24089/issue24089.kicad_pcb" );
    BOOST_REQUIRE( board );
    fs::path root = ExportOdb( *board, TempDir().Path() );
    std::vector<FIELDS> rows = MatrixLayers( root );
    std::set<unsigned long> ids;
    unsigned long maxId = 0;

    for( const FIELDS& row : rows )
    {
        BOOST_REQUIRE( row.count( "ID" ) );
    }

    for( const std::string& line : ReadLines( root / "matrix" / "matrix" ) )
    {
        if( line.rfind( "    ID=", 0 ) != 0 )
            continue;

        unsigned long id = std::stoul( line.substr( 7 ) );

        BOOST_CHECK( ids.insert( id ).second );
        maxId = std::max( maxId, id );
    }

    bool foundMax = false;

    for( const std::string& line : ReadLines( root / "misc" / "info" ) )
    {
        if( line.rfind( "MAX_UID=", 0 ) == 0 )
        {
            foundMax = true;
            BOOST_CHECK_GE( std::stoul( line.substr( 8 ) ), maxId );
        }
    }

    BOOST_CHECK( foundMax );
}


BOOST_AUTO_TEST_CASE( OdbSlotsStayOnDrillAndGainRoutChains )
{
    std::unique_ptr<BOARD> board = LoadBoard( "odbpp/pad_roles.kicad_pcb" );
    BOOST_REQUIRE( board );
    fs::path root = ExportOdb( *board, TempDir().Path() );
    ODB_PRODUCT product( root );
    fs::path rout( product.LayerFile( "rout", "features" ).ToStdWstring() );
    BOOST_REQUIRE( fs::exists( rout ) );
    OdbFeatureFile routFeatures( rout );
    BOOST_CHECK_GT( routFeatures.CountRecordsWithFlag( ".rout_plated" ), 0u );

    bool drillHasSlotLine = false;

    for( const ODB_MATRIX_ROW& row : product.Matrix() )
    {
        if( row.m_type != wxS( "DRILL" ) )
            continue;

        for( const std::string& line : ReadLines( fs::path( product.LayerFile( row.m_name,
                                                                                wxS( "features" ) ).ToStdWstring() ) ) )
        {
            if( line.rfind( "L ", 0 ) == 0 )
                drillHasSlotLine = true;
        }
    }

    BOOST_CHECK( drillHasSlotLine );

    board = LoadBoard( "odbpp/npth_pressfit_slot.kicad_pcb" );
    BOOST_REQUIRE( board );
    root = ExportOdb( *board, TempDir().Path() );
    ODB_PRODUCT npthProduct( root );
    fs::path npthRout( npthProduct.LayerFile( "rout", "features" ).ToStdWstring() );
    BOOST_REQUIRE( fs::exists( npthRout ) );
    OdbFeatureFile npthFeatures( npthRout );
    BOOST_CHECK_EQUAL( npthFeatures.CountRecordsWithFlag( ".rout_plated" ), 0u );
}


BOOST_AUTO_TEST_CASE( OdbApiKitchenSinkMatrixReferencesResolve )
{
    std::unique_ptr<BOARD> board = LoadBoard( "api_kitchen_sink.kicad_pcb" );
    BOOST_REQUIRE( board );
    fs::path root = ExportOdb( *board, TempDir().Path() );
    ODB_PRODUCT        product( root );
    std::set<wxString> ids;

    for( const ODB_MATRIX_ROW& row : product.Matrix() )
        ids.insert( row.m_id );

    for( const ODB_MATRIX_ROW& row : product.Matrix() )
    {
        for( const wxString& reference : { row.m_ref, row.m_cuTop, row.m_cuBottom } )
            BOOST_CHECK_MESSAGE( reference.IsEmpty() || ids.count( reference ), row.m_name << " references " << reference );
    }
}


BOOST_AUTO_TEST_CASE( OdbRoutDoesNotInferMissingBoardOutline )
{
    std::unique_ptr<BOARD> board = LoadBoard( "component_classes.kicad_pcb" );
    BOOST_REQUIRE( board );
    SHAPE_POLY_SET outline;
    BOOST_CHECK( !board->GetBoardPolygonOutlines( outline, false ) );
    WX_STRING_REPORTER reporter;
    fs::path root = ExportOdb( *board, TempDir().Path(), "mm", "6", &reporter );
    ODB_PRODUCT product( root );
    std::vector<std::string> lines = ReadLines( fs::path( product.LayerFile( "rout", "features" ).ToStdWstring() ) );
    BOOST_CHECK( std::none_of( lines.begin(), lines.end(), []( const std::string& aLine )
                               { return aLine.rfind( "L ", 0 ) == 0 || aLine.rfind( "A ", 0 ) == 0; } ) );
    BOOST_CHECK( reporter.GetMessages().Contains( wxS( "ODB++ rout layer has no board contour" ) ) );
}


BOOST_AUTO_TEST_CASE( OdbCollidingLayerNamesKeepMatrixReferences )
{
    std::unique_ptr<BOARD> board = LoadBoard( "odbpp/layer_name_collisions.kicad_pcb" );
    BOOST_REQUIRE( board );
    BOOST_REQUIRE_EQUAL( board->GetLayerName( F_Cu ), wxS( "Signal" ) );
    BOOST_REQUIRE_EQUAL( board->GetLayerName( In1_Cu ), wxS( "SiGnAl" ) );
    BOOST_REQUIRE_EQUAL( board->GetLayerName( B_Cu ), wxS( "SIGNAL" ) );
    BOOST_REQUIRE_EQUAL( board->GetLayerName( User_1 ), wxS( "notes" ) );
    BOOST_REQUIRE_EQUAL( board->GetLayerName( User_2 ), wxS( "Notes" ) );
    BOOST_REQUIRE_EQUAL( board->GetLayerName( User_3 ), wxS( "notes_1" ) );
    BOOST_REQUIRE_EQUAL( board->GetLayerName( User_4 ), wxS( "COMP_+_BOT" ) );
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "odb_colliding_layers" ) );

    for( bool filtered : { false, true } )
    {
        fs::path root = dir.Path() / ( filtered ? "filtered" : "default" );

        if( filtered )
        {
            fs::create_directories( root );
            PCB_IO_ODBPP plugin;
            ODB_EXPORT_OPTIONS options;
            options.m_sections = FAB::LegacySections();
            BOOST_REQUIRE_NO_THROW( plugin.Export( wxString::FromUTF8( root.string() ), *board, options ) );
        }
        else
        {
            ExportOdb( *board, root );
        }

        std::vector<FIELDS> rows = MatrixLayers( root );
        BOOST_REQUIRE( !rows.empty() );
        std::set<std::string> names;

        for( const FIELDS& row : rows )
            BOOST_CHECK( names.insert( row.at( "NAME" ) ).second );

        for( const std::string& name : { "signal", "signal_2", "signal_3", "notes", "notes_2", "notes_1",
                                         "comp_+_bot_2" } )
            BOOST_CHECK_EQUAL( names.count( name ), 1u );

        const FIELDS* rout = FindRow( rows, "rout" );
        BOOST_REQUIRE( rout );
        BOOST_CHECK_EQUAL( rout->at( "START_NAME" ), "signal" );
        BOOST_CHECK_EQUAL( rout->at( "END_NAME" ), "signal_3" );

        bool throughDrill = false;
        bool backdrill = false;

        for( const FIELDS& row : rows )
        {
            if( row.at( "TYPE" ) != "DRILL" )
                continue;

            if( row.contains( "ADD_TYPE" ) && row.at( "ADD_TYPE" ) == "BACKDRILL" )
            {
                if( row.at( "START_NAME" ) == "in2.cu" && row.at( "END_NAME" ) == "signal_3" )
                {
                    backdrill = true;
                    fs::path attrs = root / "steps" / "pcb" / "layers" / row.at( "NAME" ) / "attrlist";
                    std::vector<std::string> lines = ReadLines( attrs );
                    BOOST_CHECK( std::find( lines.begin(), lines.end(),
                                            ".backdrill_penetrate_stop_layer=signal_2" ) != lines.end() );
                }
            }
            else if( row.at( "START_NAME" ) == "signal" && row.at( "END_NAME" ) == "signal_3" )
            {
                throughDrill = true;
            }
        }

        BOOST_CHECK( throughDrill );
        BOOST_CHECK( backdrill );

        const FIELDS* bottomComponents = FindRow( rows, "comp_+_bot_2" );
        BOOST_REQUIRE( bottomComponents );
        BOOST_CHECK_EQUAL( bottomComponents->at( "TYPE" ), "COMPONENT" );
        fs::path components = root / "steps" / "pcb" / "layers" / "comp_+_bot_2" / "components";
        BOOST_REQUIRE( fs::exists( components ) );
        std::vector<std::string> lines = ReadLines( components );
        BOOST_CHECK( std::any_of( lines.begin(), lines.end(), []( const std::string& aLine )
                                  { return aLine.rfind( "CMP ", 0 ) == 0; } ) );
    }
}

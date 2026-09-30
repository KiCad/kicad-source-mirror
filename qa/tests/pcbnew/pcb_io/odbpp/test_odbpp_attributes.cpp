/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
 *
 * This program is free software: you can redistribute it and/or
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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>

#include <algorithm>
#include <cmath>
#include <cctype>
#include <fstream>
#include <memory>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <board.h>
#include <base_units.h>
#include <board_design_settings.h>
#include <board_stackup_manager/board_stackup.h>
#include <footprint.h>
#include <netinfo.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcbnew/exporters/fab_model/fab_component_row.h>
#include <pcbnew/exporters/fab_model/fab_net_names.h>
#include <pcbnew/exporters/fab_model/fab_test_points.h>
#include <pcbnew/pcb_io/odbpp/odb_attribute.h>
#include <pcbnew/pcb_io/odbpp/odb_util.h>

#include "odb_test_utils.h"

namespace fs = std::filesystem;


BOOST_AUTO_TEST_CASE( FabComponentRowUsesSavedVariantFields )
{
    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir()
                                                                      + "variant_test/variant_test.kicad_pcb" );
    BOOST_REQUIRE( board );
    const FOOTPRINT* resistor = nullptr;

    for( const FOOTPRINT* footprint : board->Footprints() )
    {
        if( footprint->GetReference() == wxS( "R1" ) )
            resistor = footprint;
    }

    BOOST_REQUIRE( resistor );
    FAB_COMPONENT_ROW normal = MakeFabComponentRow( *resistor, wxString() );
    FAB_COMPONENT_ROW variant = MakeFabComponentRow( *resistor, wxS( "Variant A" ) );
    BOOST_CHECK( normal.Field( wxS( "Datasheet" ) ).IsEmpty() );
    BOOST_CHECK_EQUAL( variant.Field( wxS( "Datasheet" ) ), wxString( wxS( "test" ) ) );
    BOOST_CHECK( variant.m_mount == FAB_MOUNT::SMT );
}


BOOST_AUTO_TEST_CASE( FabTestPointsKeepSavedBoardViaOrder )
{
    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir()
                                                                      + "connect/connect.kicad_pcb" );
    BOOST_REQUIRE( board );
    BOOST_REQUIRE( board->BuildConnectivity() );
    std::vector<FAB_TEST_POINT> points = FabTestPoints( *board );
    BOOST_REQUIRE( !points.empty() );
    bool sawPad = false;
    size_t vias = 0;

    for( const FAB_TEST_POINT& point : points )
    {
        if( point.m_pad )
        {
            sawPad = true;
            BOOST_CHECK( !point.m_via );
        }
        else
        {
            BOOST_CHECK( !sawPad );
            BOOST_REQUIRE( point.m_via );
            BOOST_CHECK( point.m_position == point.m_via->GetPosition() );
            ++vias;
        }
    }

    BOOST_CHECK_GT( vias, 0u );
    BOOST_CHECK( sawPad );
}


BOOST_AUTO_TEST_CASE( FabAnonymousNetNamesFollowSavedNetCodes )
{
    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir()
                                                                      + "odbpp/net_name_collision.kicad_pcb" );
    BOOST_REQUIRE( board );
    std::map<int, wxString> names = FabAnonymousNetNames( *board );
    BOOST_REQUIRE_GT( names.size(), 1u );
    size_t ordinal = 0;

    for( const auto& [code, name] : names )
    {
        BOOST_CHECK_GT( code, 0 );
        BOOST_CHECK_EQUAL( name, wxString::Format( wxS( "NET_%zu" ), ++ordinal ) );
    }

    const NETINFO_ITEM* supply = board->FindNet( wxS( "+12V" ) );
    const NETINFO_ITEM* q2 = board->FindNet( wxS( "Net-(Q2-C)" ) );
    const NETINFO_ITEM* q1 = board->FindNet( wxS( "Net-(Q1-C)" ) );
    BOOST_REQUIRE( supply );
    BOOST_REQUIRE( q2 );
    BOOST_REQUIRE( q1 );
    BOOST_REQUIRE_LT( q2->GetNetCode(), q1->GetNetCode() );
    BOOST_CHECK_EQUAL( names.at( supply->GetNetCode() ), wxString( wxS( "NET_1" ) ) );
    BOOST_CHECK_EQUAL( names.at( q2->GetNetCode() ), wxString( wxS( "NET_2" ) ) );
    BOOST_CHECK_EQUAL( names.at( q1->GetNetCode() ), wxString( wxS( "NET_3" ) ) );
}


BOOST_AUTO_TEST_CASE( OdbPadUsageOptionOrderMatchesSpec )
{
    BOOST_CHECK_EQUAL( static_cast<int>( ODB_ATTR::PAD_USAGE::TOEPRINT ), 0 );
    BOOST_CHECK_EQUAL( static_cast<int>( ODB_ATTR::PAD_USAGE::VIA ), 1 );
    BOOST_CHECK_EQUAL( static_cast<int>( ODB_ATTR::PAD_USAGE::G_FIDUCIAL ), 2 );
    BOOST_CHECK_EQUAL( static_cast<int>( ODB_ATTR::PAD_USAGE::L_FIDUCIAL ), 3 );
    BOOST_CHECK_EQUAL( static_cast<int>( ODB_ATTR::PAD_USAGE::TOOLING_HOLE ), 4 );
    BOOST_CHECK_EQUAL( static_cast<int>( ODB_ATTR::PAD_USAGE::BOND_FINGER ), 5 );
}


BOOST_AUTO_TEST_CASE( OdbFiducialPadUsage )
{
    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir()
                                                                      + "odbpp/pad_roles.kicad_pcb" );
    BOOST_REQUIRE( board );

    size_t globals = CountPads( *board, PAD_PROP::FIDUCIAL_GLBL, F_Cu );
    size_t locals = CountPads( *board, PAD_PROP::FIDUCIAL_LOCAL, F_Cu );
    BOOST_REQUIRE_GT( globals, 0 );
    BOOST_REQUIRE_EQUAL( locals, 1 );

    ODB_PRODUCT product( ExportOdb( *board, TempDir().Path() ) );
    OdbFeatureFile top( fs::path( product.LayerFile( ODB::GenLegalEntityName( board->GetLayerName( F_Cu ) ),
                                                       wxS( "features" ) ).ToStdWstring() ) );

    BOOST_CHECK_EQUAL( top.CountRecordsWithOption( ".pad_usage", "g_fiducial" ), globals );
    BOOST_CHECK_EQUAL( top.CountRecordsWithOption( ".pad_usage", "l_fiducial" ), locals );
}


BOOST_AUTO_TEST_CASE( OdbTestPointFeatureAndCadnetFlag )
{
    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir()
                                                                      + "odbpp/pad_roles.kicad_pcb" );
    BOOST_REQUIRE( board );
    BOOST_REQUIRE_EQUAL( CountPads( *board, PAD_PROP::TESTPOINT, F_Cu ), 1 );

    const PAD* testPoint = nullptr;

    for( const FOOTPRINT* footprint : board->Footprints() )
    {
        for( const PAD* pad : footprint->Pads() )
        {
            if( pad->GetProperty() == PAD_PROP::TESTPOINT )
                testPoint = pad;
        }
    }

    BOOST_REQUIRE( testPoint );

    fs::path root = ExportOdb( *board, TempDir().Path() );
    ODB_PRODUCT product( root );
    OdbFeatureFile top( fs::path( product.LayerFile( ODB::GenLegalEntityName( board->GetLayerName( F_Cu ) ),
                                                       wxS( "features" ) ).ToStdWstring() ) );
    BOOST_CHECK_EQUAL( top.CountRecordsWithFlag( ".test_point" ), 1 );

    size_t taggedPoints = 0;

    for( const std::string& line : ReadLines( root / "steps" / "pcb" / "netlists" / "cadnet" / "netlist" ) )
    {
        std::istringstream stream( line );
        std::vector<std::string> fields;
        std::string token;

        while( stream >> token )
            fields.push_back( token );

        if( fields.size() < 9 || fields[0].empty() || !std::isdigit( fields[0][0] ) )
            continue;

        double x = std::stod( fields[2] );
        double y = std::stod( fields[3] );
        double padX = pcbIUScale.IUTomm( testPoint->GetPosition().x );
        double padY = -pcbIUScale.IUTomm( testPoint->GetPosition().y );

        if( std::abs( x - padX ) < 0.00001 && std::abs( y - padY ) < 0.00001 )
        {
            BOOST_CHECK_EQUAL( fields[7], "e" );
            BOOST_CHECK_EQUAL( fields.back(), "t" );
            ++taggedPoints;
        }
    }

    BOOST_CHECK_EQUAL( taggedPoints, 1 );
}


BOOST_AUTO_TEST_CASE( OdbConnectedViaIsCadnetMidpoint )
{
    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir()
                                                                      + "connect/connect.kicad_pcb" );
    BOOST_REQUIRE( board );
    BOOST_REQUIRE( board->BuildConnectivity() );
    const FAB_TEST_POINT* midpoint = nullptr;
    std::vector<FAB_TEST_POINT> points = FabTestPoints( *board );

    for( const FAB_TEST_POINT& point : points )
    {
        if( point.m_via && point.m_netCode > 0 && ( point.m_front || point.m_back ) && !point.m_netEnd )
        {
            midpoint = &point;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( midpoint, "connect board has no exposed midpoint via" );
    fs::path root = ExportOdb( *board, TempDir().Path() );
    bool found = false;

    for( const std::string& line : ReadLines( root / "steps" / "pcb" / "netlists" / "cadnet" / "netlist" ) )
    {
        std::istringstream stream( line );
        std::vector<std::string> fields;
        std::string token;

        while( stream >> token )
            fields.push_back( token );

        if( fields.size() < 8 || fields.back() != "v" || fields[0] != std::to_string( midpoint->m_netCode ) )
            continue;

        double x = pcbIUScale.IUTomm( midpoint->m_position.x );
        double y = -pcbIUScale.IUTomm( midpoint->m_position.y );

        if( std::abs( std::stod( fields[2] ) - x ) < 0.00001
            && std::abs( std::stod( fields[3] ) - y ) < 0.00001 )
        {
            BOOST_CHECK_EQUAL( fields[5], "m" );
            found = true;
        }
    }

    BOOST_CHECK( found );
}


BOOST_AUTO_TEST_CASE( OdbPressFitUsesDrillToolComponentAndPinRoles )
{
    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir()
                                                                      + "odbpp/pad_roles.kicad_pcb" );
    BOOST_REQUIRE( board );
    BOOST_REQUIRE_EQUAL( CountPads( *board, PAD_PROP::PRESSFIT, F_Cu ), 1 );

    fs::path root = ExportOdb( *board, TempDir().Path() );
    ODB_PRODUCT product( root );
    wxString drillLayer;

    for( const ODB_MATRIX_ROW& row : product.Matrix() )
    {
        if( row.m_name.StartsWith( wxS( "drill_plated_" ) ) )
            drillLayer = row.m_name;
    }

    BOOST_REQUIRE( !drillLayer.IsEmpty() );
    OdbFeatureFile drill( fs::path( product.LayerFile( drillLayer, wxS( "features" ) ).ToStdWstring() ) );
    BOOST_CHECK_EQUAL( drill.CountRecordsWithOption( ".plated_type", "press_fit" ), 1 );

    size_t pressFitTools = 0;

    for( const std::string& line : ReadLines( fs::path( product.LayerFile( drillLayer,
                                                                             wxS( "tools" ) ).ToStdWstring() ) ) )
    {
        if( line.find( "TYPE2=PRESS_FIT" ) != std::string::npos )
            ++pressFitTools;
    }

    BOOST_CHECK_GT( pressFitTools, 0 );

    size_t pressFitPins = 0;

    for( const std::string& line : ReadLines( root / "steps" / "pcb" / "eda" / "data" ) )
    {
        if( line.rfind( "PIN ", 0 ) == 0 && line.size() >= 2
            && line.compare( line.size() - 2, 2, " P" ) == 0 )
            ++pressFitPins;
    }

    BOOST_CHECK_EQUAL( pressFitPins, 1 );

    size_t pressFitComponents = 0;

    for( const ODB_MATRIX_ROW& row : product.Matrix() )
    {
        if( row.m_type != wxS( "COMPONENT" ) )
            continue;

        OdbFeatureFile components( fs::path( product.LayerFile( row.m_name, wxS( "components" ) ).ToStdWstring() ) );
        pressFitComponents += components.CountRecordsWithOption( ".comp_mount_type", "pressfit" );
    }

    BOOST_CHECK_EQUAL( pressFitComponents, 1 );
}


BOOST_AUTO_TEST_CASE( OdbUnnumberedNpthUsesToolingHoleRole )
{
    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir()
                                                                      + "odbpp/pad_roles.kicad_pcb" );
    BOOST_REQUIRE( board );

    ODB_PRODUCT product( ExportOdb( *board, TempDir().Path() ) );
    wxString drillLayer;

    for( const ODB_MATRIX_ROW& row : product.Matrix() )
    {
        if( row.m_name.StartsWith( wxS( "drill_non-plated_" ) ) )
            drillLayer = row.m_name;
    }

    BOOST_REQUIRE( !drillLayer.IsEmpty() );
    OdbFeatureFile drill( fs::path( product.LayerFile( drillLayer, wxS( "features" ) ).ToStdWstring() ) );
    size_t nonPlated = drill.CountRecordsWithOption( ".drill", "non_plated" );
    BOOST_REQUIRE_GT( nonPlated, 0 );
    BOOST_CHECK_EQUAL( drill.CountRecordsWithOption( ".pad_usage", "tooling_hole" ), nonPlated );
}


BOOST_AUTO_TEST_CASE( OdbMicroviasUseLaserAttributesAndTools )
{
    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir()
                                                                      + "padstacks.kicad_pcb" );
    BOOST_REQUIRE( board );
    size_t microvias = 0;

    for( PCB_TRACK* track : board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T && static_cast<PCB_VIA*>( track )->GetViaType() == VIATYPE::MICROVIA )
            ++microvias;
    }

    BOOST_REQUIRE_GT( microvias, 0 );
    ODB_PRODUCT product( ExportOdb( *board, TempDir().Path() ) );
    size_t laserFeatures = 0;
    size_t laserTools = 0;

    for( const ODB_MATRIX_ROW& row : product.Matrix() )
    {
        if( row.m_type != wxS( "DRILL" ) )
            continue;

        OdbFeatureFile features( fs::path( product.LayerFile( row.m_name, wxS( "features" ) ).ToStdWstring() ) );
        laserFeatures += features.CountRecordsWithOption( ".via_type", "laser" );

        for( const std::string& line : ReadLines( fs::path( product.LayerFile( row.m_name,
                                                                                 wxS( "tools" ) ).ToStdWstring() ) ) )
        {
            if( line.find( "TYPE2=LASER" ) != std::string::npos )
                ++laserTools;
        }
    }

    BOOST_CHECK_EQUAL( laserFeatures, microvias );
    BOOST_CHECK_GT( laserTools, 0 );
}


BOOST_AUTO_TEST_CASE( OdbNpthPressFitNeverUsesPressFitTool )
{
    const std::vector<std::pair<std::string, bool>> fixtures = {
        { "odbpp/npth_pressfit.kicad_pcb", false }, { "odbpp/npth_pressfit_slot.kicad_pcb", true }
    };

    for( const auto& [name, isSlot] : fixtures )
    {
        std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir() + name );
        BOOST_REQUIRE( board );
        size_t invalidPads = 0;

        for( const FOOTPRINT* footprint : board->Footprints() )
        {
            for( const PAD* pad : footprint->Pads() )
            {
                if( pad->GetAttribute() == PAD_ATTRIB::NPTH && pad->GetProperty() == PAD_PROP::PRESSFIT )
                {
                    ++invalidPads;
                    BOOST_CHECK_EQUAL( pad->GetDrillSizeX() != pad->GetDrillSizeY(), isSlot );
                }
            }
        }

        BOOST_REQUIRE_EQUAL( invalidPads, 1 );
        ODB_PRODUCT product( ExportOdb( *board, TempDir().Path() ) );
        size_t nonPlatedTools = 0;
        size_t pressFitTools = 0;

        for( const ODB_MATRIX_ROW& row : product.Matrix() )
        {
            if( row.m_type != wxS( "DRILL" ) )
                continue;

            fs::path toolsPath( product.LayerFile( row.m_name, wxS( "tools" ) ).ToStdWstring() );

            for( const std::string& line : ReadLines( toolsPath ) )
            {
                if( line.find( "TYPE=NON_PLATED" ) != std::string::npos )
                    ++nonPlatedTools;

                if( line.find( "TYPE2=PRESS_FIT" ) != std::string::npos )
                    ++pressFitTools;
            }
        }

        BOOST_CHECK_GT( nonPlatedTools, 0 );
        BOOST_CHECK_EQUAL( pressFitTools, 0 );
    }
}


BOOST_AUTO_TEST_CASE( OdbStepBoardThicknessMatchesDefaultStackup )
{
    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir()
                                                                      + "custom_pads.kicad_pcb" );
    BOOST_REQUIRE( board );
    BOOST_REQUIRE( !board->GetDesignSettings().m_HasStackup );

    fs::path root = ExportOdb( *board, TempDir().Path() );
    std::vector<std::string> lines = ReadLines( root / "steps" / "pcb" / "attrlist" );
    BOOST_REQUIRE( !lines.empty() );
    BOOST_CHECK_EQUAL( lines.front(), "UNITS=MM" );
    BOOST_CHECK( std::find( lines.begin(), lines.end(), ".board_thickness=1600.0" ) != lines.end() );

    fs::path inchRoot = ExportOdb( *board, TempDir().Path(), "inch" );
    std::vector<std::string> inchLines = ReadLines( inchRoot / "steps" / "pcb" / "attrlist" );
    BOOST_REQUIRE( inchLines.size() >= 2 );
    BOOST_CHECK_EQUAL( inchLines.front(), "UNITS=INCH" );
    BOOST_CHECK_EQUAL( inchLines[1].substr( 0, 17 ), ".board_thickness=" );
    BOOST_CHECK_CLOSE( std::stod( inchLines[1].substr( 17 ) ), 62.992126, 0.01 );
}


BOOST_AUTO_TEST_CASE( OdbLayerMaterialHasUserAttributeDeclaration )
{
    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir()
                                                                      + "ipc2581/dielectric-sublayer.kicad_pcb" );
    BOOST_REQUIRE( board );

    fs::path root = ExportOdb( *board, TempDir().Path() );
    ODB_PRODUCT product( root );
    size_t materials = 0;

    for( const ODB_MATRIX_ROW& row : product.Matrix() )
    {
        if( row.m_type != wxS( "DIELECTRIC" ) )
            continue;

        fs::path attrlistPath( product.LayerFile( row.m_name, wxS( "attrlist" ) ).ToStdWstring() );
        std::vector<std::string> attrlist = ReadLines( attrlistPath );
        BOOST_REQUIRE( !attrlist.empty() );
        BOOST_CHECK_EQUAL( attrlist.front(), "UNITS=MM" );

        for( const std::string& line : attrlist )
        {
            if( line.rfind( "material=", 0 ) == 0 )
                ++materials;

            BOOST_CHECK( line.rfind( ".material=", 0 ) != 0 );
        }
    }

    BOOST_CHECK_GT( materials, 0 );
    std::vector<std::string> declaration = ReadLines( root / "misc" / "userattr" );

    for( const std::string& field : { "UNITS=MM", "TEXT {", "NAME=material", "ENTITY=LAYER", "MAX_LEN=64" } )
        BOOST_CHECK( std::find( declaration.begin(), declaration.end(), field ) != declaration.end() );
}


BOOST_AUTO_TEST_CASE( OdbBackdrillStopLayerFollowsMustCutCopper )
{
    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( KI_TEST::GetPcbnewTestDataDir()
                                                                      + "issue25021/backdrill.kicad_pcb" );
    BOOST_REQUIRE( board );

    ODB_PRODUCT product( ExportOdb( *board, TempDir().Path() ) );
    const std::map<std::pair<wxString, wxString>, wxString> stopLayers = {
        { { wxS( "f.cu" ), wxS( "in3.cu" ) }, wxS( "in4.cu" ) },
        { { wxS( "in6.cu" ), wxS( "b.cu" ) }, wxS( "in5.cu" ) },
        { { wxS( "in3.cu" ), wxS( "b.cu" ) }, wxS( "in2.cu" ) }
    };
    size_t checked = 0;

    for( const ODB_MATRIX_ROW& row : product.Matrix() )
    {
        if( row.m_addType != wxS( "BACKDRILL" ) )
            continue;

        auto it = stopLayers.find( { row.m_startName, row.m_endName } );
        BOOST_REQUIRE( it != stopLayers.end() );
        ++checked;
        std::string expected = ".backdrill_penetrate_stop_layer=" + it->second.ToStdString();
        fs::path attrlistPath( product.LayerFile( row.m_name, wxS( "attrlist" ) ).ToStdWstring() );
        std::vector<std::string> lines = ReadLines( attrlistPath );
        BOOST_CHECK( std::find( lines.begin(), lines.end(), expected ) != lines.end() );
    }

    BOOST_CHECK_EQUAL( checked, stopLayers.size() );
}



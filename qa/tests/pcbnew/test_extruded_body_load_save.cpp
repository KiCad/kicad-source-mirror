#include <boost/test/unit_test.hpp>
#include <footprint.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <settings/settings_manager.h>
#include <filesystem>

BOOST_AUTO_TEST_SUITE( ExtrudedBodyLoadSave )

BOOST_AUTO_TEST_CASE( FullRoundTrip )
{
    SETTINGS_MANAGER settingsManager;

    FOOTPRINT         footprint( nullptr );
    EXTRUDED_3D_BODY& body = footprint.EnsureExtrudedBody();
    body.m_height = pcbIUScale.mmToIU( 2.5 );
    body.m_standoff = pcbIUScale.mmToIU( 0.1 );
    body.m_layer = F_CrtYd;
    body.m_material = EXTRUSION_MATERIAL::METAL;
    body.m_color = KIGFX::COLOR4D( 0.8, 0.4, 0.0, 0.9 );
    body.m_offset = VECTOR3D( 1.0, -0.5, 0.2 );
    body.m_scale = VECTOR3D( 1.5, 0.8, 2.0 );
    body.m_rotation = VECTOR3D( 10.0, 20.0, 45.0 );
    body.m_show = false;

    KI_TEST::TEMPORARY_DIRECTORY tempLib( "kicad_qa_extruded_body_roundtrip", ".pretty" );
    const auto savePath = tempLib.GetPath() / "extruded_body_roundtrip.kicad_mod";
    KI_TEST::DumpFootprintToFile( footprint, savePath.string() );

    std::unique_ptr<FOOTPRINT> loaded = KI_TEST::ReadFootprintFromFileOrStream( savePath.string() );
    BOOST_REQUIRE( loaded != nullptr );

    BOOST_REQUIRE( loaded->HasExtrudedBody() );
    const EXTRUDED_3D_BODY* lb = loaded->GetExtrudedBody();

    BOOST_CHECK_EQUAL( lb->m_height, body.m_height );
    BOOST_CHECK_EQUAL( lb->m_standoff, body.m_standoff );
    BOOST_CHECK_EQUAL( lb->m_layer, body.m_layer );
    BOOST_CHECK_EQUAL( static_cast<int>( lb->m_material ), static_cast<int>( body.m_material ) );
    BOOST_CHECK_CLOSE( lb->m_color.r, body.m_color.r, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_color.g, body.m_color.g, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_color.b, body.m_color.b, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_color.a, body.m_color.a, 0.01 );

    BOOST_CHECK_CLOSE( lb->m_offset.x, body.m_offset.x, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_offset.y, body.m_offset.y, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_offset.z, body.m_offset.z, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_scale.x, body.m_scale.x, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_scale.y, body.m_scale.y, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_scale.z, body.m_scale.z, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_rotation.x, body.m_rotation.x, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_rotation.y, body.m_rotation.y, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_rotation.z, body.m_rotation.z, 0.01 );

    BOOST_CHECK_EQUAL( lb->m_show, body.m_show );
}

BOOST_AUTO_TEST_CASE( DefaultsRoundTrip )
{
    SETTINGS_MANAGER settingsManager;

    FOOTPRINT         footprint( nullptr );
    EXTRUDED_3D_BODY& body = footprint.EnsureExtrudedBody();
    body.m_height = pcbIUScale.mmToIU( 5.0 );
    // Leave everything else at defaults

    KI_TEST::TEMPORARY_DIRECTORY tempLib( "kicad_qa_extruded_body_defaults_roundtrip", ".pretty" );
    const auto savePath = tempLib.GetPath() / "extruded_body_defaults_roundtrip.kicad_mod";
    KI_TEST::DumpFootprintToFile( footprint, savePath.string() );

    std::unique_ptr<FOOTPRINT> loaded = KI_TEST::ReadFootprintFromFileOrStream( savePath.string() );
    BOOST_REQUIRE( loaded != nullptr );
    BOOST_REQUIRE( loaded->HasExtrudedBody() );

    const EXTRUDED_3D_BODY* lb = loaded->GetExtrudedBody();
    BOOST_CHECK_EQUAL( lb->m_height, body.m_height );
    BOOST_CHECK_EQUAL( lb->m_standoff, 0 );
    BOOST_CHECK_EQUAL( lb->m_layer, UNDEFINED_LAYER );
    BOOST_CHECK_EQUAL( static_cast<int>( lb->m_material ), static_cast<int>( EXTRUSION_MATERIAL::PLASTIC ) );
    BOOST_CHECK( lb->m_color == KIGFX::COLOR4D::UNSPECIFIED );

    BOOST_CHECK_CLOSE( lb->m_offset.x, 0.0, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_offset.y, 0.0, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_offset.z, 0.0, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_scale.x, 1.0, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_scale.y, 1.0, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_scale.z, 1.0, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_rotation.x, 0.0, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_rotation.y, 0.0, 0.01 );
    BOOST_CHECK_CLOSE( lb->m_rotation.z, 0.0, 0.01 );

    BOOST_CHECK_EQUAL( lb->m_show, true );
}

BOOST_AUTO_TEST_CASE( NoExtrudedBodyRoundTrip )
{
    SETTINGS_MANAGER settingsManager;

    FOOTPRINT footprint( nullptr );

    KI_TEST::TEMPORARY_DIRECTORY tempLib( "kicad_qa_extruded_body_none_roundtrip", ".pretty" );
    const auto savePath = tempLib.GetPath() / "extruded_body_none_roundtrip.kicad_mod";
    KI_TEST::DumpFootprintToFile( footprint, savePath.string() );

    std::unique_ptr<FOOTPRINT> loaded = KI_TEST::ReadFootprintFromFileOrStream( savePath.string() );
    BOOST_REQUIRE( loaded != nullptr );
    BOOST_CHECK( !loaded->HasExtrudedBody() );
}

BOOST_AUTO_TEST_SUITE_END()

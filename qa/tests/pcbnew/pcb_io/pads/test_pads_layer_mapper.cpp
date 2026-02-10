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

/**
 * @file test_pads_layer_mapper.cpp
 * Test suite for PADS_LAYER_MAPPER
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <pcb_io/pads/pads_layer_mapper.h>


struct PADS_LAYER_MAPPER_FIXTURE
{
    PADS_LAYER_MAPPER_FIXTURE() {}
};


BOOST_FIXTURE_TEST_SUITE( PadsLayerMapper, PADS_LAYER_MAPPER_FIXTURE )


BOOST_AUTO_TEST_CASE( CopperLayerType_TwoLayer )
{
    PADS_LAYER_MAPPER mapper;
    mapper.SetCopperLayerCount( 2 );

    // Layer 1 = Top
    BOOST_CHECK( mapper.GetLayerType( 1 ) == PADS_LAYER_TYPE::COPPER_TOP );

    // Layer 2 = Bottom (for 2-layer board)
    BOOST_CHECK( mapper.GetLayerType( 2 ) == PADS_LAYER_TYPE::COPPER_BOTTOM );
}


BOOST_AUTO_TEST_CASE( CopperLayerType_FourLayer )
{
    PADS_LAYER_MAPPER mapper;
    mapper.SetCopperLayerCount( 4 );

    // Layer 1 = Top
    BOOST_CHECK( mapper.GetLayerType( 1 ) == PADS_LAYER_TYPE::COPPER_TOP );

    // Layer 2, 3 = Inner
    BOOST_CHECK( mapper.GetLayerType( 2 ) == PADS_LAYER_TYPE::COPPER_INNER );
    BOOST_CHECK( mapper.GetLayerType( 3 ) == PADS_LAYER_TYPE::COPPER_INNER );

    // Layer 4 = Bottom
    BOOST_CHECK( mapper.GetLayerType( 4 ) == PADS_LAYER_TYPE::COPPER_BOTTOM );
}


BOOST_AUTO_TEST_CASE( CopperLayerType_SixLayer )
{
    PADS_LAYER_MAPPER mapper;
    mapper.SetCopperLayerCount( 6 );

    BOOST_CHECK( mapper.GetLayerType( 1 ) == PADS_LAYER_TYPE::COPPER_TOP );
    BOOST_CHECK( mapper.GetLayerType( 2 ) == PADS_LAYER_TYPE::COPPER_INNER );
    BOOST_CHECK( mapper.GetLayerType( 3 ) == PADS_LAYER_TYPE::COPPER_INNER );
    BOOST_CHECK( mapper.GetLayerType( 4 ) == PADS_LAYER_TYPE::COPPER_INNER );
    BOOST_CHECK( mapper.GetLayerType( 5 ) == PADS_LAYER_TYPE::COPPER_INNER );
    BOOST_CHECK( mapper.GetLayerType( 6 ) == PADS_LAYER_TYPE::COPPER_BOTTOM );
}


BOOST_AUTO_TEST_CASE( PadStackLayerTypes )
{
    PADS_LAYER_MAPPER mapper;
    mapper.SetCopperLayerCount( 4 );

    // Pad stack special values
    BOOST_CHECK( mapper.GetLayerType( PADS_LAYER_MAPPER::LAYER_PAD_STACK_TOP ) == PADS_LAYER_TYPE::COPPER_TOP );
    BOOST_CHECK( mapper.GetLayerType( PADS_LAYER_MAPPER::LAYER_PAD_STACK_BOTTOM ) == PADS_LAYER_TYPE::COPPER_BOTTOM );
    BOOST_CHECK( mapper.GetLayerType( PADS_LAYER_MAPPER::LAYER_PAD_STACK_INNER ) == PADS_LAYER_TYPE::COPPER_INNER );
}


BOOST_AUTO_TEST_CASE( NonCopperLayerTypes )
{
    PADS_LAYER_MAPPER mapper;

    BOOST_CHECK( mapper.GetLayerType( PADS_LAYER_MAPPER::LAYER_SILKSCREEN_TOP ) == PADS_LAYER_TYPE::SILKSCREEN_TOP );
    BOOST_CHECK( mapper.GetLayerType( PADS_LAYER_MAPPER::LAYER_SILKSCREEN_BOTTOM ) == PADS_LAYER_TYPE::SILKSCREEN_BOTTOM );
    BOOST_CHECK( mapper.GetLayerType( PADS_LAYER_MAPPER::LAYER_SOLDERMASK_TOP ) == PADS_LAYER_TYPE::SOLDERMASK_TOP );
    BOOST_CHECK( mapper.GetLayerType( PADS_LAYER_MAPPER::LAYER_SOLDERMASK_BOTTOM ) == PADS_LAYER_TYPE::SOLDERMASK_BOTTOM );
    BOOST_CHECK( mapper.GetLayerType( PADS_LAYER_MAPPER::LAYER_PASTE_TOP ) == PADS_LAYER_TYPE::PASTE_TOP );
    BOOST_CHECK( mapper.GetLayerType( PADS_LAYER_MAPPER::LAYER_PASTE_BOTTOM ) == PADS_LAYER_TYPE::PASTE_BOTTOM );
    BOOST_CHECK( mapper.GetLayerType( PADS_LAYER_MAPPER::LAYER_ASSEMBLY_TOP ) == PADS_LAYER_TYPE::ASSEMBLY_TOP );
    BOOST_CHECK( mapper.GetLayerType( PADS_LAYER_MAPPER::LAYER_ASSEMBLY_BOTTOM ) == PADS_LAYER_TYPE::ASSEMBLY_BOTTOM );
}


BOOST_AUTO_TEST_CASE( UnknownLayerType )
{
    PADS_LAYER_MAPPER mapper;
    mapper.SetCopperLayerCount( 2 );

    // Unknown layer numbers should return UNKNOWN
    BOOST_CHECK( mapper.GetLayerType( 100 ) == PADS_LAYER_TYPE::UNKNOWN );
    BOOST_CHECK( mapper.GetLayerType( 999 ) == PADS_LAYER_TYPE::UNKNOWN );
}


BOOST_AUTO_TEST_CASE( AutoMapCopper )
{
    PADS_LAYER_MAPPER mapper;
    mapper.SetCopperLayerCount( 4 );

    BOOST_CHECK_EQUAL( mapper.GetAutoMapLayer( 1 ), F_Cu );
    BOOST_CHECK_EQUAL( mapper.GetAutoMapLayer( 4 ), B_Cu );
    BOOST_CHECK_EQUAL( mapper.GetAutoMapLayer( 2 ), In1_Cu );
    BOOST_CHECK_EQUAL( mapper.GetAutoMapLayer( 3 ), In2_Cu );
}


BOOST_AUTO_TEST_CASE( AutoMapNonCopper )
{
    PADS_LAYER_MAPPER mapper;

    BOOST_CHECK_EQUAL( mapper.GetAutoMapLayer( PADS_LAYER_MAPPER::LAYER_SILKSCREEN_TOP ), F_SilkS );
    BOOST_CHECK_EQUAL( mapper.GetAutoMapLayer( PADS_LAYER_MAPPER::LAYER_SILKSCREEN_BOTTOM ), B_SilkS );
    BOOST_CHECK_EQUAL( mapper.GetAutoMapLayer( PADS_LAYER_MAPPER::LAYER_SOLDERMASK_TOP ), F_Mask );
    BOOST_CHECK_EQUAL( mapper.GetAutoMapLayer( PADS_LAYER_MAPPER::LAYER_SOLDERMASK_BOTTOM ), B_Mask );
    BOOST_CHECK_EQUAL( mapper.GetAutoMapLayer( PADS_LAYER_MAPPER::LAYER_PASTE_TOP ), F_Paste );
    BOOST_CHECK_EQUAL( mapper.GetAutoMapLayer( PADS_LAYER_MAPPER::LAYER_PASTE_BOTTOM ), B_Paste );
    BOOST_CHECK_EQUAL( mapper.GetAutoMapLayer( PADS_LAYER_MAPPER::LAYER_ASSEMBLY_TOP ), F_Fab );
    BOOST_CHECK_EQUAL( mapper.GetAutoMapLayer( PADS_LAYER_MAPPER::LAYER_ASSEMBLY_BOTTOM ), B_Fab );
}


BOOST_AUTO_TEST_CASE( ParseLayerName_Silkscreen )
{
    PADS_LAYER_MAPPER mapper;

    BOOST_CHECK( mapper.ParseLayerName( "Silkscreen Top" ) == PADS_LAYER_TYPE::SILKSCREEN_TOP );
    BOOST_CHECK( mapper.ParseLayerName( "SILKSCREEN TOP" ) == PADS_LAYER_TYPE::SILKSCREEN_TOP );
    BOOST_CHECK( mapper.ParseLayerName( "silkscreen top" ) == PADS_LAYER_TYPE::SILKSCREEN_TOP );
    BOOST_CHECK( mapper.ParseLayerName( "SST" ) == PADS_LAYER_TYPE::SILKSCREEN_TOP );
    BOOST_CHECK( mapper.ParseLayerName( "Top Silk" ) == PADS_LAYER_TYPE::SILKSCREEN_TOP );

    BOOST_CHECK( mapper.ParseLayerName( "Silkscreen Bottom" ) == PADS_LAYER_TYPE::SILKSCREEN_BOTTOM );
    BOOST_CHECK( mapper.ParseLayerName( "SSB" ) == PADS_LAYER_TYPE::SILKSCREEN_BOTTOM );
}


BOOST_AUTO_TEST_CASE( ParseLayerName_SolderMask )
{
    PADS_LAYER_MAPPER mapper;

    BOOST_CHECK( mapper.ParseLayerName( "Solder Mask Top" ) == PADS_LAYER_TYPE::SOLDERMASK_TOP );
    BOOST_CHECK( mapper.ParseLayerName( "SOLDERMASK TOP" ) == PADS_LAYER_TYPE::SOLDERMASK_TOP );
    BOOST_CHECK( mapper.ParseLayerName( "SMT" ) == PADS_LAYER_TYPE::SOLDERMASK_TOP );
    BOOST_CHECK( mapper.ParseLayerName( "Top Mask" ) == PADS_LAYER_TYPE::SOLDERMASK_TOP );

    BOOST_CHECK( mapper.ParseLayerName( "Solder Mask Bottom" ) == PADS_LAYER_TYPE::SOLDERMASK_BOTTOM );
    BOOST_CHECK( mapper.ParseLayerName( "SMB" ) == PADS_LAYER_TYPE::SOLDERMASK_BOTTOM );
}


BOOST_AUTO_TEST_CASE( ParseLayerName_Paste )
{
    PADS_LAYER_MAPPER mapper;

    BOOST_CHECK( mapper.ParseLayerName( "Paste Top" ) == PADS_LAYER_TYPE::PASTE_TOP );
    BOOST_CHECK( mapper.ParseLayerName( "Solder Paste Top" ) == PADS_LAYER_TYPE::PASTE_TOP );

    BOOST_CHECK( mapper.ParseLayerName( "Paste Bottom" ) == PADS_LAYER_TYPE::PASTE_BOTTOM );
}


BOOST_AUTO_TEST_CASE( ParseLayerName_Assembly )
{
    PADS_LAYER_MAPPER mapper;

    BOOST_CHECK( mapper.ParseLayerName( "Assembly Top" ) == PADS_LAYER_TYPE::ASSEMBLY_TOP );
    BOOST_CHECK( mapper.ParseLayerName( "Top Assembly" ) == PADS_LAYER_TYPE::ASSEMBLY_TOP );
    BOOST_CHECK( mapper.ParseLayerName( "ASSY TOP" ) == PADS_LAYER_TYPE::ASSEMBLY_TOP );

    BOOST_CHECK( mapper.ParseLayerName( "Assembly Bottom" ) == PADS_LAYER_TYPE::ASSEMBLY_BOTTOM );
}


BOOST_AUTO_TEST_CASE( ParseLayerName_BoardOutline )
{
    PADS_LAYER_MAPPER mapper;

    BOOST_CHECK( mapper.ParseLayerName( "Board Outline" ) == PADS_LAYER_TYPE::BOARD_OUTLINE );
    BOOST_CHECK( mapper.ParseLayerName( "Board" ) == PADS_LAYER_TYPE::BOARD_OUTLINE );
    BOOST_CHECK( mapper.ParseLayerName( "Outline" ) == PADS_LAYER_TYPE::BOARD_OUTLINE );
}


BOOST_AUTO_TEST_CASE( ParseLayerName_Copper )
{
    PADS_LAYER_MAPPER mapper;

    // Pattern matching for copper layers
    BOOST_CHECK( mapper.ParseLayerName( "Top" ) == PADS_LAYER_TYPE::COPPER_TOP );
    BOOST_CHECK( mapper.ParseLayerName( "Layer 1" ) == PADS_LAYER_TYPE::COPPER_TOP );

    BOOST_CHECK( mapper.ParseLayerName( "Bottom" ) == PADS_LAYER_TYPE::COPPER_BOTTOM );
    BOOST_CHECK( mapper.ParseLayerName( "Bot" ) == PADS_LAYER_TYPE::COPPER_BOTTOM );

    BOOST_CHECK( mapper.ParseLayerName( "Inner 1" ) == PADS_LAYER_TYPE::COPPER_INNER );
    BOOST_CHECK( mapper.ParseLayerName( "Internal" ) == PADS_LAYER_TYPE::COPPER_INNER );
    BOOST_CHECK( mapper.ParseLayerName( "Mid Layer" ) == PADS_LAYER_TYPE::COPPER_INNER );
}


BOOST_AUTO_TEST_CASE( ParseLayerName_Unknown )
{
    PADS_LAYER_MAPPER mapper;

    BOOST_CHECK( mapper.ParseLayerName( "Random Name" ) == PADS_LAYER_TYPE::UNKNOWN );
    BOOST_CHECK( mapper.ParseLayerName( "XYZ123" ) == PADS_LAYER_TYPE::UNKNOWN );
}


BOOST_AUTO_TEST_CASE( CustomLayerNameMapping )
{
    PADS_LAYER_MAPPER mapper;

    // Add custom mapping
    mapper.AddLayerNameMapping( "Custom Layer", PADS_LAYER_TYPE::DOCUMENTATION );

    BOOST_CHECK( mapper.ParseLayerName( "Custom Layer" ) == PADS_LAYER_TYPE::DOCUMENTATION );
    BOOST_CHECK( mapper.ParseLayerName( "CUSTOM LAYER" ) == PADS_LAYER_TYPE::DOCUMENTATION );
}


BOOST_AUTO_TEST_CASE( PermittedLayers )
{
    PADS_LAYER_MAPPER mapper;

    // Copper layers should permit all copper
    LSET copper_permitted = mapper.GetPermittedLayers( PADS_LAYER_TYPE::COPPER_TOP );
    BOOST_CHECK( copper_permitted.Contains( F_Cu ) );
    BOOST_CHECK( copper_permitted.Contains( B_Cu ) );
    BOOST_CHECK( copper_permitted.Contains( In1_Cu ) );

    // Silkscreen should permit silkscreen layers
    LSET silk_permitted = mapper.GetPermittedLayers( PADS_LAYER_TYPE::SILKSCREEN_TOP );
    BOOST_CHECK( silk_permitted.Contains( F_SilkS ) );
    BOOST_CHECK( silk_permitted.Contains( B_SilkS ) );
    BOOST_CHECK( !silk_permitted.Contains( F_Cu ) );

    // Board outline should only permit Edge_Cuts
    LSET outline_permitted = mapper.GetPermittedLayers( PADS_LAYER_TYPE::BOARD_OUTLINE );
    BOOST_CHECK( outline_permitted.Contains( Edge_Cuts ) );
    BOOST_CHECK( !outline_permitted.Contains( F_Cu ) );
    BOOST_CHECK( !outline_permitted.Contains( F_SilkS ) );
}


BOOST_AUTO_TEST_CASE( BuildInputLayerDescriptions )
{
    PADS_LAYER_MAPPER mapper;
    mapper.SetCopperLayerCount( 4 );

    std::vector<PADS_LAYER_INFO> infos = {
        { 1, "Top Copper", PADS_LAYER_TYPE::COPPER_TOP, true },
        { 4, "Bottom Copper", PADS_LAYER_TYPE::COPPER_BOTTOM, true },
        { 26, "Silkscreen Top", PADS_LAYER_TYPE::SILKSCREEN_TOP, false }
    };

    std::vector<INPUT_LAYER_DESC> descs = mapper.BuildInputLayerDescriptions( infos );

    BOOST_CHECK_EQUAL( descs.size(), 3 );

    BOOST_CHECK_EQUAL( descs[0].Name, wxT( "Top Copper" ) );
    BOOST_CHECK_EQUAL( descs[0].AutoMapLayer, F_Cu );
    BOOST_CHECK( descs[0].Required );

    BOOST_CHECK_EQUAL( descs[1].Name, wxT( "Bottom Copper" ) );
    BOOST_CHECK_EQUAL( descs[1].AutoMapLayer, B_Cu );
    BOOST_CHECK( descs[1].Required );

    BOOST_CHECK_EQUAL( descs[2].Name, wxT( "Silkscreen Top" ) );
    BOOST_CHECK_EQUAL( descs[2].AutoMapLayer, F_SilkS );
    BOOST_CHECK( !descs[2].Required );
}


BOOST_AUTO_TEST_CASE( LayerTypeToString )
{
    BOOST_CHECK_EQUAL( PADS_LAYER_MAPPER::LayerTypeToString( PADS_LAYER_TYPE::COPPER_TOP ), "Copper Top" );
    BOOST_CHECK_EQUAL( PADS_LAYER_MAPPER::LayerTypeToString( PADS_LAYER_TYPE::SILKSCREEN_TOP ), "Silkscreen Top" );
    BOOST_CHECK_EQUAL( PADS_LAYER_MAPPER::LayerTypeToString( PADS_LAYER_TYPE::UNKNOWN ), "Unknown" );
}


BOOST_AUTO_TEST_SUITE_END()

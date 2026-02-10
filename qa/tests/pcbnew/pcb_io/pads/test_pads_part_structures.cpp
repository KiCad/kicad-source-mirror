/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2025 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <boost/test/unit_test.hpp>
#include <sstream>
#include <pcb_io/pads/pads_parser.h>
#include <pcbnew_utils/board_file_utils.h>

using namespace PADS_IO;


BOOST_AUTO_TEST_SUITE( PadsPartStructures )


BOOST_AUTO_TEST_CASE( Part_DefaultConstruction )
{
    PART part;
    BOOST_CHECK( part.name.empty() );
    BOOST_CHECK( part.decal.empty() );
    BOOST_CHECK( part.alternate_decals.empty() );
    BOOST_CHECK( part.value.empty() );
    BOOST_CHECK_EQUAL( part.rotation, 0.0 );
    BOOST_CHECK_EQUAL( part.bottom_layer, false );
    BOOST_CHECK_EQUAL( part.glued, false );
}


BOOST_AUTO_TEST_CASE( Part_SingleDecal )
{
    PART part;
    part.decal = "DIP8";

    BOOST_CHECK_EQUAL( part.decal, "DIP8" );
    BOOST_CHECK( part.alternate_decals.empty() );
}


BOOST_AUTO_TEST_CASE( Part_AlternateDecals )
{
    PART part;
    part.decal = "DIP8";
    part.alternate_decals.push_back( "SOIC8" );
    part.alternate_decals.push_back( "QFN8" );

    BOOST_CHECK_EQUAL( part.decal, "DIP8" );
    BOOST_REQUIRE_EQUAL( part.alternate_decals.size(), 2 );
    BOOST_CHECK_EQUAL( part.alternate_decals[0], "SOIC8" );
    BOOST_CHECK_EQUAL( part.alternate_decals[1], "QFN8" );
}


BOOST_AUTO_TEST_CASE( DecalStringSplit_SingleDecal )
{
    // Simulate what the parser does with a single decal
    std::string decal_string = "RESISTOR_0603";

    std::string primary;
    std::vector<std::string> alternates;

    size_t pos = 0;
    size_t colon_pos = 0;
    bool first = true;

    while( ( colon_pos = decal_string.find( ':', pos ) ) != std::string::npos )
    {
        std::string decal_name = decal_string.substr( pos, colon_pos - pos );

        if( first )
        {
            primary = decal_name;
            first = false;
        }
        else
        {
            alternates.push_back( decal_name );
        }

        pos = colon_pos + 1;
    }

    std::string last_decal = decal_string.substr( pos );

    if( first )
        primary = last_decal;
    else
        alternates.push_back( last_decal );

    BOOST_CHECK_EQUAL( primary, "RESISTOR_0603" );
    BOOST_CHECK( alternates.empty() );
}


BOOST_AUTO_TEST_CASE( DecalStringSplit_TwoDecals )
{
    std::string decal_string = "DIP8:SOIC8";

    std::string primary;
    std::vector<std::string> alternates;

    size_t pos = 0;
    size_t colon_pos = 0;
    bool first = true;

    while( ( colon_pos = decal_string.find( ':', pos ) ) != std::string::npos )
    {
        std::string decal_name = decal_string.substr( pos, colon_pos - pos );

        if( first )
        {
            primary = decal_name;
            first = false;
        }
        else
        {
            alternates.push_back( decal_name );
        }

        pos = colon_pos + 1;
    }

    std::string last_decal = decal_string.substr( pos );

    if( first )
        primary = last_decal;
    else
        alternates.push_back( last_decal );

    BOOST_CHECK_EQUAL( primary, "DIP8" );
    BOOST_REQUIRE_EQUAL( alternates.size(), 1 );
    BOOST_CHECK_EQUAL( alternates[0], "SOIC8" );
}


BOOST_AUTO_TEST_CASE( DecalStringSplit_ThreeDecals )
{
    std::string decal_string = "DIP8:SOIC8:QFN8";

    std::string primary;
    std::vector<std::string> alternates;

    size_t pos = 0;
    size_t colon_pos = 0;
    bool first = true;

    while( ( colon_pos = decal_string.find( ':', pos ) ) != std::string::npos )
    {
        std::string decal_name = decal_string.substr( pos, colon_pos - pos );

        if( first )
        {
            primary = decal_name;
            first = false;
        }
        else
        {
            alternates.push_back( decal_name );
        }

        pos = colon_pos + 1;
    }

    std::string last_decal = decal_string.substr( pos );

    if( first )
        primary = last_decal;
    else
        alternates.push_back( last_decal );

    BOOST_CHECK_EQUAL( primary, "DIP8" );
    BOOST_REQUIRE_EQUAL( alternates.size(), 2 );
    BOOST_CHECK_EQUAL( alternates[0], "SOIC8" );
    BOOST_CHECK_EQUAL( alternates[1], "QFN8" );
}


BOOST_AUTO_TEST_CASE( DecalStringSplit_ManyDecals )
{
    std::string decal_string = "PKG1:PKG2:PKG3:PKG4:PKG5";

    std::string primary;
    std::vector<std::string> alternates;

    size_t pos = 0;
    size_t colon_pos = 0;
    bool first = true;

    while( ( colon_pos = decal_string.find( ':', pos ) ) != std::string::npos )
    {
        std::string decal_name = decal_string.substr( pos, colon_pos - pos );

        if( first )
        {
            primary = decal_name;
            first = false;
        }
        else
        {
            alternates.push_back( decal_name );
        }

        pos = colon_pos + 1;
    }

    std::string last_decal = decal_string.substr( pos );

    if( first )
        primary = last_decal;
    else
        alternates.push_back( last_decal );

    BOOST_CHECK_EQUAL( primary, "PKG1" );
    BOOST_REQUIRE_EQUAL( alternates.size(), 4 );
    BOOST_CHECK_EQUAL( alternates[0], "PKG2" );
    BOOST_CHECK_EQUAL( alternates[1], "PKG3" );
    BOOST_CHECK_EQUAL( alternates[2], "PKG4" );
    BOOST_CHECK_EQUAL( alternates[3], "PKG5" );
}


BOOST_AUTO_TEST_CASE( Parameters_DefaultThermalSettings )
{
    PARAMETERS params;
    BOOST_CHECK_CLOSE( params.thermal_line_width, 30.0, 0.001 );
    BOOST_CHECK_CLOSE( params.thermal_smd_width, 20.0, 0.001 );
    BOOST_CHECK_EQUAL( params.thermal_flags, 0 );
    BOOST_CHECK_CLOSE( params.thermal_min_clearance, 5.0, 0.001 );
    BOOST_CHECK_EQUAL( params.thermal_min_spokes, 4 );
}


BOOST_AUTO_TEST_CASE( Parameters_CustomThermalSettings )
{
    PARAMETERS params;
    params.thermal_line_width = 40.0;
    params.thermal_smd_width = 25.0;
    params.thermal_flags = 1024;
    params.thermal_min_clearance = 10.0;
    params.thermal_min_spokes = 6;

    BOOST_CHECK_CLOSE( params.thermal_line_width, 40.0, 0.001 );
    BOOST_CHECK_CLOSE( params.thermal_smd_width, 25.0, 0.001 );
    BOOST_CHECK_EQUAL( params.thermal_flags, 1024 );
    BOOST_CHECK_CLOSE( params.thermal_min_clearance, 10.0, 0.001 );
    BOOST_CHECK_EQUAL( params.thermal_min_spokes, 6 );
}


BOOST_AUTO_TEST_CASE( Pour_DefaultConstruction )
{
    POUR pour;
    BOOST_CHECK( pour.net_name.empty() );
    BOOST_CHECK_EQUAL( pour.layer, 0 );
    BOOST_CHECK_EQUAL( pour.priority, 0 );
    BOOST_CHECK_EQUAL( pour.width, 0.0 );
    BOOST_CHECK( pour.points.empty() );
    BOOST_CHECK_EQUAL( pour.is_cutout, false );
    BOOST_CHECK( pour.owner_pour.empty() );
}


BOOST_AUTO_TEST_CASE( Pour_CutoutFlag )
{
    POUR outline;
    outline.net_name = "GND";
    outline.layer = 1;
    outline.is_cutout = false;
    outline.owner_pour = "POUR1";

    POUR cutout;
    cutout.net_name = "GND";
    cutout.layer = 1;
    cutout.is_cutout = true;
    cutout.owner_pour = "POUR1";

    BOOST_CHECK_EQUAL( outline.is_cutout, false );
    BOOST_CHECK_EQUAL( cutout.is_cutout, true );
    BOOST_CHECK_EQUAL( outline.owner_pour, cutout.owner_pour );
}


BOOST_AUTO_TEST_CASE( ReuseInstance_DefaultConstruction )
{
    REUSE_INSTANCE instance;
    BOOST_CHECK( instance.instance_name.empty() );
    BOOST_CHECK( instance.part_naming.empty() );
    BOOST_CHECK( instance.net_naming.empty() );
    BOOST_CHECK_EQUAL( instance.location.x, 0.0 );
    BOOST_CHECK_EQUAL( instance.location.y, 0.0 );
    BOOST_CHECK_EQUAL( instance.rotation, 0.0 );
    BOOST_CHECK_EQUAL( instance.glued, false );
}


BOOST_AUTO_TEST_CASE( ReuseInstance_WithValues )
{
    REUSE_INSTANCE instance;
    instance.instance_name = "BLOCK1_INST1";
    instance.part_naming = "PREFIX";
    instance.net_naming = "APPEND";
    instance.location.x = 1000.0;
    instance.location.y = 2000.0;
    instance.rotation = 90.0;
    instance.glued = true;

    BOOST_CHECK_EQUAL( instance.instance_name, "BLOCK1_INST1" );
    BOOST_CHECK_EQUAL( instance.part_naming, "PREFIX" );
    BOOST_CHECK_EQUAL( instance.net_naming, "APPEND" );
    BOOST_CHECK_CLOSE( instance.location.x, 1000.0, 0.001 );
    BOOST_CHECK_CLOSE( instance.location.y, 2000.0, 0.001 );
    BOOST_CHECK_CLOSE( instance.rotation, 90.0, 0.001 );
    BOOST_CHECK_EQUAL( instance.glued, true );
}


BOOST_AUTO_TEST_CASE( ReuseBlock_DefaultConstruction )
{
    REUSE_BLOCK block;
    BOOST_CHECK( block.name.empty() );
    BOOST_CHECK_EQUAL( block.timestamp, 0 );
    BOOST_CHECK( block.part_naming.empty() );
    BOOST_CHECK( block.net_naming.empty() );
    BOOST_CHECK( block.part_names.empty() );
    BOOST_CHECK( block.nets.empty() );
    BOOST_CHECK( block.instances.empty() );
}


BOOST_AUTO_TEST_CASE( ReuseBlock_WithContents )
{
    REUSE_BLOCK block;
    block.name = "MEMORY_BLOCK";
    block.timestamp = 1234567890;
    block.part_naming = "PREFIX";
    block.net_naming = "APPEND";
    block.part_names.push_back( "U1" );
    block.part_names.push_back( "U2" );
    block.part_names.push_back( "C1" );

    REUSE_NET net1;
    net1.merge = true;
    net1.name = "DATA0";
    block.nets.push_back( net1 );

    REUSE_NET net2;
    net2.merge = false;
    net2.name = "DATA1";
    block.nets.push_back( net2 );

    REUSE_INSTANCE inst;
    inst.instance_name = "MEM1";
    inst.location.x = 5000.0;
    inst.location.y = 3000.0;
    block.instances.push_back( inst );

    BOOST_CHECK_EQUAL( block.name, "MEMORY_BLOCK" );
    BOOST_CHECK_EQUAL( block.timestamp, 1234567890 );
    BOOST_CHECK_EQUAL( block.part_naming, "PREFIX" );
    BOOST_CHECK_EQUAL( block.net_naming, "APPEND" );
    BOOST_REQUIRE_EQUAL( block.part_names.size(), 3 );
    BOOST_CHECK_EQUAL( block.part_names[0], "U1" );
    BOOST_CHECK_EQUAL( block.part_names[1], "U2" );
    BOOST_CHECK_EQUAL( block.part_names[2], "C1" );
    BOOST_REQUIRE_EQUAL( block.nets.size(), 2 );
    BOOST_CHECK_EQUAL( block.nets[0].name, "DATA0" );
    BOOST_CHECK_EQUAL( block.nets[0].merge, true );
    BOOST_CHECK_EQUAL( block.nets[1].name, "DATA1" );
    BOOST_CHECK_EQUAL( block.nets[1].merge, false );
    BOOST_REQUIRE_EQUAL( block.instances.size(), 1 );
    BOOST_CHECK_EQUAL( block.instances[0].instance_name, "MEM1" );
}


BOOST_AUTO_TEST_CASE( ReuseBlock_PartMembershipMapping )
{
    // Test building the part-to-block membership map used during expansion
    std::map<std::string, REUSE_BLOCK> blocks;

    REUSE_BLOCK memBlock;
    memBlock.name = "MEMORY";
    memBlock.part_names.push_back( "U1" );
    memBlock.part_names.push_back( "U2" );
    blocks["MEMORY"] = memBlock;

    REUSE_BLOCK pwrBlock;
    pwrBlock.name = "POWER";
    pwrBlock.part_names.push_back( "C1" );
    pwrBlock.part_names.push_back( "C2" );
    pwrBlock.part_names.push_back( "L1" );
    blocks["POWER"] = pwrBlock;

    // Build membership map as done in pcb_io_pads.cpp
    std::map<std::string, std::string> partToBlockMap;

    for( const auto& [blockName, block] : blocks )
    {
        for( const std::string& partName : block.part_names )
        {
            partToBlockMap[partName] = blockName;
        }
    }

    BOOST_REQUIRE_EQUAL( partToBlockMap.size(), 5 );
    BOOST_CHECK_EQUAL( partToBlockMap["U1"], "MEMORY" );
    BOOST_CHECK_EQUAL( partToBlockMap["U2"], "MEMORY" );
    BOOST_CHECK_EQUAL( partToBlockMap["C1"], "POWER" );
    BOOST_CHECK_EQUAL( partToBlockMap["C2"], "POWER" );
    BOOST_CHECK_EQUAL( partToBlockMap["L1"], "POWER" );
}


BOOST_AUTO_TEST_CASE( ReuseBlock_EmptyBlocksMap )
{
    // Test that empty reuse blocks result in empty membership map
    std::map<std::string, REUSE_BLOCK> blocks;
    std::map<std::string, std::string> partToBlockMap;

    for( const auto& [blockName, block] : blocks )
    {
        for( const std::string& partName : block.part_names )
        {
            partToBlockMap[partName] = blockName;
        }
    }

    BOOST_CHECK( partToBlockMap.empty() );
}


BOOST_AUTO_TEST_CASE( ReuseBlock_GroupCreationCondition )
{
    // Test conditions for when groups should be created
    REUSE_BLOCK emptyBlock;
    emptyBlock.name = "EMPTY";

    REUSE_BLOCK blockWithParts;
    blockWithParts.name = "WITH_PARTS";
    blockWithParts.part_names.push_back( "U1" );

    REUSE_BLOCK blockWithInstances;
    blockWithInstances.name = "WITH_INSTANCES";
    REUSE_INSTANCE inst;
    inst.instance_name = "INST1";
    blockWithInstances.instances.push_back( inst );

    // Groups should be created when block has parts OR instances
    bool createEmptyGroup = !emptyBlock.instances.empty() || !emptyBlock.part_names.empty();
    bool createPartsGroup = !blockWithParts.instances.empty() || !blockWithParts.part_names.empty();
    bool createInstanceGroup = !blockWithInstances.instances.empty() || !blockWithInstances.part_names.empty();

    BOOST_CHECK_EQUAL( createEmptyGroup, false );
    BOOST_CHECK_EQUAL( createPartsGroup, true );
    BOOST_CHECK_EQUAL( createInstanceGroup, true );
}


BOOST_AUTO_TEST_CASE( Cluster_DefaultConstruction )
{
    CLUSTER cluster;
    BOOST_CHECK( cluster.name.empty() );
    BOOST_CHECK_EQUAL( cluster.id, 0 );
    BOOST_CHECK( cluster.net_names.empty() );
    BOOST_CHECK( cluster.segment_refs.empty() );
}


BOOST_AUTO_TEST_CASE( Cluster_WithContents )
{
    CLUSTER cluster;
    cluster.name = "MEMORY_BUS";
    cluster.id = 42;
    cluster.net_names.push_back( "DATA0" );
    cluster.net_names.push_back( "DATA1" );
    cluster.net_names.push_back( "ADDR0" );
    cluster.segment_refs.push_back( "DATA0.1" );
    cluster.segment_refs.push_back( "DATA0.2" );
    cluster.segment_refs.push_back( "DATA1.1" );

    BOOST_CHECK_EQUAL( cluster.name, "MEMORY_BUS" );
    BOOST_CHECK_EQUAL( cluster.id, 42 );
    BOOST_REQUIRE_EQUAL( cluster.net_names.size(), 3 );
    BOOST_CHECK_EQUAL( cluster.net_names[0], "DATA0" );
    BOOST_CHECK_EQUAL( cluster.net_names[1], "DATA1" );
    BOOST_CHECK_EQUAL( cluster.net_names[2], "ADDR0" );
    BOOST_REQUIRE_EQUAL( cluster.segment_refs.size(), 3 );
    BOOST_CHECK_EQUAL( cluster.segment_refs[0], "DATA0.1" );
    BOOST_CHECK_EQUAL( cluster.segment_refs[1], "DATA0.2" );
    BOOST_CHECK_EQUAL( cluster.segment_refs[2], "DATA1.1" );
}


BOOST_AUTO_TEST_CASE( Cluster_SegmentRefDetection )
{
    // Test the segment reference detection logic used in parser
    std::string netName = "DATA0";
    std::string segRef = "DATA0.1";
    std::string segRefComplex = "NET_123.45";

    bool netNameHasDot = netName.find( '.' ) != std::string::npos;
    bool segRefHasDot = segRef.find( '.' ) != std::string::npos;
    bool segRefComplexHasDot = segRefComplex.find( '.' ) != std::string::npos;

    BOOST_CHECK_EQUAL( netNameHasDot, false );
    BOOST_CHECK_EQUAL( segRefHasDot, true );
    BOOST_CHECK_EQUAL( segRefComplexHasDot, true );
}


BOOST_AUTO_TEST_CASE( TestPoint_DefaultConstruction )
{
    TEST_POINT tp;
    BOOST_CHECK( tp.type.empty() );
    BOOST_CHECK_EQUAL( tp.x, 0.0 );
    BOOST_CHECK_EQUAL( tp.y, 0.0 );
    BOOST_CHECK_EQUAL( tp.side, 0 );
    BOOST_CHECK( tp.net_name.empty() );
    BOOST_CHECK( tp.symbol_name.empty() );
}


BOOST_AUTO_TEST_CASE( TestPoint_ViaType )
{
    TEST_POINT tp;
    tp.type = "VIA";
    tp.x = 7000.0;
    tp.y = 3450.0;
    tp.side = 0;  // Through
    tp.net_name = "+5V";
    tp.symbol_name = "TESTVIATHRU";

    BOOST_CHECK_EQUAL( tp.type, "VIA" );
    BOOST_CHECK_CLOSE( tp.x, 7000.0, 0.001 );
    BOOST_CHECK_CLOSE( tp.y, 3450.0, 0.001 );
    BOOST_CHECK_EQUAL( tp.side, 0 );
    BOOST_CHECK_EQUAL( tp.net_name, "+5V" );
    BOOST_CHECK_EQUAL( tp.symbol_name, "TESTVIATHRU" );
}


BOOST_AUTO_TEST_CASE( TestPoint_PinType )
{
    TEST_POINT tp;
    tp.type = "PIN";
    tp.x = 1000.0;
    tp.y = 2000.0;
    tp.side = 1;  // Top
    tp.net_name = "GND";
    tp.symbol_name = "U1.3";

    BOOST_CHECK_EQUAL( tp.type, "PIN" );
    BOOST_CHECK_EQUAL( tp.side, 1 );
    BOOST_CHECK_EQUAL( tp.net_name, "GND" );
    BOOST_CHECK_EQUAL( tp.symbol_name, "U1.3" );
}


BOOST_AUTO_TEST_CASE( TestPoint_SideValues )
{
    // Test side value meaning: 0=through, 1=top, 2=bottom
    TEST_POINT tpThrough, tpTop, tpBottom;
    tpThrough.side = 0;
    tpTop.side = 1;
    tpBottom.side = 2;

    BOOST_CHECK_EQUAL( tpThrough.side, 0 );
    BOOST_CHECK_EQUAL( tpTop.side, 1 );
    BOOST_CHECK_EQUAL( tpBottom.side, 2 );
}


BOOST_AUTO_TEST_CASE( Dimension_DefaultConstruction )
{
    DIMENSION dim;
    BOOST_CHECK( dim.name.empty() );
    BOOST_CHECK_EQUAL( dim.x, 0.0 );
    BOOST_CHECK_EQUAL( dim.y, 0.0 );
    BOOST_CHECK_EQUAL( dim.layer, 0 );
    BOOST_CHECK( dim.points.empty() );
    BOOST_CHECK( dim.text.empty() );
    BOOST_CHECK_EQUAL( dim.text_height, 0.0 );
    BOOST_CHECK_EQUAL( dim.text_width, 0.0 );
    BOOST_CHECK_EQUAL( dim.rotation, 0.0 );
}


BOOST_AUTO_TEST_CASE( Dimension_WithValues )
{
    DIMENSION dim;
    dim.name = "DIM001";
    dim.x = 1000.0;
    dim.y = 2000.0;
    dim.layer = 26;  // Typical doc layer
    dim.text = "10.5mm";
    dim.text_height = 50.0;
    dim.text_width = 8.0;
    dim.rotation = 90.0;

    dim.points.push_back( { 0.0, 0.0 } );
    dim.points.push_back( { 1050.0, 0.0 } );
    dim.points.push_back( { 1050.0, 100.0 } );

    BOOST_CHECK_EQUAL( dim.name, "DIM001" );
    BOOST_CHECK_CLOSE( dim.x, 1000.0, 0.001 );
    BOOST_CHECK_CLOSE( dim.y, 2000.0, 0.001 );
    BOOST_CHECK_EQUAL( dim.layer, 26 );
    BOOST_CHECK_EQUAL( dim.text, "10.5mm" );
    BOOST_CHECK_CLOSE( dim.text_height, 50.0, 0.001 );
    BOOST_CHECK_CLOSE( dim.text_width, 8.0, 0.001 );
    BOOST_CHECK_CLOSE( dim.rotation, 90.0, 0.001 );
    BOOST_CHECK_EQUAL( dim.points.size(), 3 );
}


BOOST_AUTO_TEST_CASE( Dimension_NamePrefix )
{
    // Test the dimension detection pattern (names starting with "DIM")
    std::string dimName1 = "DIM001";
    std::string dimName2 = "DIM_BOARD_WIDTH";
    std::string nonDimName = "OUTLINE";

    BOOST_CHECK( dimName1.rfind( "DIM", 0 ) == 0 );
    BOOST_CHECK( dimName2.rfind( "DIM", 0 ) == 0 );
    BOOST_CHECK( nonDimName.rfind( "DIM", 0 ) != 0 );
}


BOOST_AUTO_TEST_CASE( DesignRules_DefaultConstruction )
{
    DESIGN_RULES rules;

    BOOST_CHECK_CLOSE( rules.min_clearance, 8.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.default_clearance, 10.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.min_track_width, 6.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.default_track_width, 10.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.min_via_size, 20.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.default_via_size, 40.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.min_via_drill, 10.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.default_via_drill, 20.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.hole_to_hole, 10.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.silk_clearance, 5.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.mask_clearance, 3.0, 0.001 );
}


BOOST_AUTO_TEST_CASE( DesignRules_WithValues )
{
    DESIGN_RULES rules;
    rules.min_clearance = 12.0;
    rules.default_clearance = 15.0;
    rules.min_track_width = 8.0;
    rules.default_track_width = 12.0;
    rules.min_via_size = 25.0;
    rules.default_via_size = 50.0;
    rules.min_via_drill = 15.0;
    rules.default_via_drill = 25.0;
    rules.hole_to_hole = 12.0;
    rules.silk_clearance = 6.0;
    rules.mask_clearance = 4.0;

    BOOST_CHECK_CLOSE( rules.min_clearance, 12.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.default_clearance, 15.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.min_track_width, 8.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.default_track_width, 12.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.min_via_size, 25.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.default_via_size, 50.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.min_via_drill, 15.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.default_via_drill, 25.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.hole_to_hole, 12.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.silk_clearance, 6.0, 0.001 );
    BOOST_CHECK_CLOSE( rules.mask_clearance, 4.0, 0.001 );
}


BOOST_AUTO_TEST_CASE( NetClassDef_DefaultConstruction )
{
    NET_CLASS_DEF nc;
    BOOST_CHECK( nc.name.empty() );
    BOOST_CHECK_EQUAL( nc.clearance, 0.0 );
    BOOST_CHECK_EQUAL( nc.track_width, 0.0 );
    BOOST_CHECK_EQUAL( nc.via_size, 0.0 );
    BOOST_CHECK_EQUAL( nc.via_drill, 0.0 );
    BOOST_CHECK_EQUAL( nc.diff_pair_gap, 0.0 );
    BOOST_CHECK_EQUAL( nc.diff_pair_width, 0.0 );
    BOOST_CHECK( nc.net_names.empty() );
}


BOOST_AUTO_TEST_CASE( NetClassDef_WithValues )
{
    NET_CLASS_DEF nc;
    nc.name = "HighSpeed";
    nc.clearance = 12.0;
    nc.track_width = 8.0;
    nc.via_size = 30.0;
    nc.via_drill = 15.0;
    nc.diff_pair_gap = 6.0;
    nc.diff_pair_width = 8.0;
    nc.net_names.push_back( "CLK" );
    nc.net_names.push_back( "DATA0" );
    nc.net_names.push_back( "DATA1" );

    BOOST_CHECK_EQUAL( nc.name, "HighSpeed" );
    BOOST_CHECK_CLOSE( nc.clearance, 12.0, 0.001 );
    BOOST_CHECK_CLOSE( nc.track_width, 8.0, 0.001 );
    BOOST_CHECK_CLOSE( nc.via_size, 30.0, 0.001 );
    BOOST_CHECK_CLOSE( nc.via_drill, 15.0, 0.001 );
    BOOST_CHECK_CLOSE( nc.diff_pair_gap, 6.0, 0.001 );
    BOOST_CHECK_CLOSE( nc.diff_pair_width, 8.0, 0.001 );
    BOOST_CHECK_EQUAL( nc.net_names.size(), 3 );
    BOOST_CHECK_EQUAL( nc.net_names[0], "CLK" );
    BOOST_CHECK_EQUAL( nc.net_names[1], "DATA0" );
    BOOST_CHECK_EQUAL( nc.net_names[2], "DATA1" );
}


BOOST_AUTO_TEST_CASE( DiffPairDef_DefaultConstruction )
{
    DIFF_PAIR_DEF dp;
    BOOST_CHECK( dp.name.empty() );
    BOOST_CHECK( dp.positive_net.empty() );
    BOOST_CHECK( dp.negative_net.empty() );
    BOOST_CHECK_EQUAL( dp.gap, 0.0 );
    BOOST_CHECK_EQUAL( dp.width, 0.0 );
}


BOOST_AUTO_TEST_CASE( DiffPairDef_WithValues )
{
    DIFF_PAIR_DEF dp;
    dp.name = "USB_DATA";
    dp.positive_net = "USB_D+";
    dp.negative_net = "USB_D-";
    dp.gap = 8.0;
    dp.width = 10.0;

    BOOST_CHECK_EQUAL( dp.name, "USB_DATA" );
    BOOST_CHECK_EQUAL( dp.positive_net, "USB_D+" );
    BOOST_CHECK_EQUAL( dp.negative_net, "USB_D-" );
    BOOST_CHECK_CLOSE( dp.gap, 8.0, 0.001 );
    BOOST_CHECK_CLOSE( dp.width, 10.0, 0.001 );
}


BOOST_AUTO_TEST_SUITE_END()

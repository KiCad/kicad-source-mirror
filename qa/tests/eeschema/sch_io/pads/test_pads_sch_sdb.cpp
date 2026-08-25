/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <sch_io/pads/pads_sch_sdb.h>
#include <sch_io/ole_image.h>

#include <wx/image.h>

#include <json_common.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

using PADS_SCH_BINARY::PADS_SCH_SDB;
using PADS_SCH_BINARY::SCH_SDB_BLOCK_KIND;

namespace
{

static std::vector<uint8_t> loadBinary( const std::string& aPath )
{
    std::ifstream file( aPath, std::ios::binary );

    if( !file )
        throw std::runtime_error( "Cannot open test fixture: " + aPath );

    return { std::istreambuf_iterator<char>( file ), std::istreambuf_iterator<char>() };
}


static std::vector<uint8_t> loadPublicFixture()
{
    return loadBinary( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/minimal_v13.sch" );
}


static void putU32( std::vector<uint8_t>& aBytes, size_t aOffset, uint32_t aValue )
{
    aBytes.at( aOffset ) = static_cast<uint8_t>( aValue );
    aBytes.at( aOffset + 1 ) = static_cast<uint8_t>( aValue >> 8 );
    aBytes.at( aOffset + 2 ) = static_cast<uint8_t>( aValue >> 16 );
    aBytes.at( aOffset + 3 ) = static_cast<uint8_t>( aValue >> 24 );
}


static bool errorContains( const IO_ERROR& aError, const std::string& aNeedle1, const std::string& aNeedle2 )
{
    std::string message = aError.What().ToStdString();
    return message.find( aNeedle1 ) != std::string::npos && message.find( aNeedle2 ) != std::string::npos;
}


static void checkPhysicalPartition( const std::string& aPath )
{
    PADS_SCH_SDB sdb;
    sdb.Load( loadBinary( aPath ) );

    const auto& blocks = sdb.Blocks();
    BOOST_REQUIRE_MESSAGE( !blocks.empty(), aPath );
    BOOST_CHECK_EQUAL( blocks.front().offset, sdb.PayloadOffset() );

    for( size_t i = 1; i < blocks.size(); ++i )
        BOOST_CHECK_EQUAL( blocks[i - 1].End(), blocks[i].offset );

    BOOST_CHECK_EQUAL( blocks.back().End(), sdb.FooterOffset() );
}

} // namespace


BOOST_AUTO_TEST_SUITE( PadsSchBinarySdb )


BOOST_AUTO_TEST_CASE( PublicV13Container )
{
    std::vector<uint8_t> bytes = loadPublicFixture();
    PADS_SCH_SDB         sdb;
    sdb.Load( bytes );

    BOOST_CHECK( PADS_SCH_SDB::HasFamilyMagic( bytes ) );
    BOOST_CHECK( PADS_SCH_SDB::IsSupportedVersion( 0x000C ) );
    BOOST_CHECK( PADS_SCH_SDB::IsSupportedVersion( 0x000D ) );
    BOOST_CHECK( !PADS_SCH_SDB::IsSupportedVersion( 0x1234 ) );
    BOOST_CHECK_EQUAL( sdb.Version(), 0x000D );
    BOOST_CHECK_EQUAL( sdb.Pools().size(), 20 );
    BOOST_CHECK_EQUAL( sdb.PayloadOffset(), 0x250 );
    BOOST_CHECK_LT( sdb.FooterOffset(), sdb.Bytes().size() );
    BOOST_CHECK_EQUAL( sdb.Cursor().Size(), sdb.Bytes().size() );

    BOOST_CHECK_EQUAL( sdb.Pools()[0].allocatedBytes, 588 );
    BOOST_CHECK_EQUAL( sdb.Pools()[0].count, 21 );
    BOOST_CHECK_EQUAL( sdb.Pools()[0].usedBytes, 588 );
    BOOST_CHECK_EQUAL( sdb.Pools()[0].handle, 140233272 );
    BOOST_CHECK_EQUAL( sdb.Pools()[3].allocatedBytes, 49152 );
    BOOST_CHECK_EQUAL( sdb.Pools()[3].count, 1 );
    BOOST_CHECK_EQUAL( sdb.Pools()[3].usedBytes, 48 );
    BOOST_CHECK_EQUAL( sdb.Pools()[3].handle, 140233872 );
    BOOST_CHECK_EQUAL( sdb.Pools()[9].allocatedBytes, 1280 );
    BOOST_CHECK_EQUAL( sdb.Pools()[9].count, 0 );
    BOOST_CHECK_EQUAL( sdb.Pools()[9].usedBytes, 0 );
    BOOST_CHECK_EQUAL( sdb.Pools()[9].handle, 179219184 );
}


BOOST_AUTO_TEST_CASE( PhysicalStreamPartitionsCorpus )
{
    const std::string publicRoot = KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/";
    std::ifstream     publicFile( publicRoot + "manifest.json" );
    nlohmann::json    publicManifest;
    publicFile >> publicManifest;

    for( const nlohmann::json& fixture : publicManifest.at( "fixtures" ) )
        checkPhysicalPartition( publicRoot + fixture.at( "binary" ).get<std::string>() );

}


BOOST_AUTO_TEST_CASE( MinimalV13PhysicalLedger )
{
    PADS_SCH_SDB sdb;
    sdb.Load( loadPublicFixture() );
    const auto& blocks = sdb.Blocks();

    BOOST_REQUIRE_EQUAL( blocks.size(), 3 );
    BOOST_CHECK( blocks[0].kind == SCH_SDB_BLOCK_KIND::FIXED_CONTROLLER );
    BOOST_CHECK_EQUAL( blocks[0].controller, 0x24 );
    BOOST_CHECK_EQUAL( blocks[0].offset, 0x250 );
    BOOST_CHECK_EQUAL( blocks[0].End(), 0x5439 );
    BOOST_CHECK_EQUAL( blocks[0].count, 19 );
    BOOST_CHECK_EQUAL( blocks[0].stride, 0 );
    BOOST_CHECK( blocks[1].kind == SCH_SDB_BLOCK_KIND::SHEET );
    BOOST_CHECK_EQUAL( blocks[1].offset, 0x5439 );
    BOOST_CHECK_EQUAL( blocks[1].End(), 0x9938 );
    BOOST_CHECK( blocks[2].kind == SCH_SDB_BLOCK_KIND::FOOTER_AUX );
    BOOST_CHECK_EQUAL( blocks[2].offset, 0x9938 );
    BOOST_CHECK_EQUAL( blocks[2].bytes, 4 );
}


BOOST_AUTO_TEST_CASE( EmbeddedOleObjects )
{
    PADS_SCH_SDB sdb;
    sdb.Load( loadBinary( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/ole_images.sch" ) );

    const auto isContainerItem = []( const PADS_SCH_BINARY::SCH_SDB_BLOCK& aBlock )
    {
        return aBlock.kind == SCH_SDB_BLOCK_KIND::CFB_CONTAINER_ITEM;
    };

    const auto&                                        blocks = sdb.Blocks();
    std::vector<const PADS_SCH_BINARY::SCH_SDB_BLOCK*> items;

    for( const auto& block : blocks )
    {
        if( isContainerItem( block ) )
            items.push_back( &block );
    }

    BOOST_REQUIRE_EQUAL( items.size(), 2 );
    BOOST_REQUIRE_EQUAL( sdb.OleItems().size(), 2 );
    BOOST_CHECK( ( sdb.OleItems()[0].extent == std::array<int32_t, 4>{ 30, 30, 350, 210 } ) );
    BOOST_CHECK_EQUAL( sdb.OleItems()[0].left, -15766 );
    BOOST_CHECK_EQUAL( sdb.OleItems()[0].bottom, -10366 );
    BOOST_CHECK_EQUAL( sdb.OleItems()[0].right, -13296 );
    BOOST_CHECK_EQUAL( sdb.OleItems()[0].top, -11755 );
    BOOST_CHECK_EQUAL( sdb.OleItems()[0].sheetPlane, 0 );
    BOOST_CHECK_EQUAL( sdb.OleItems()[0].flags, 1 );
    BOOST_CHECK( ( sdb.OleItems()[1].extent == std::array<int32_t, 4>{ 30, 30, 170, 84 } ) );
    BOOST_CHECK_EQUAL( sdb.OleItems()[1].left, -15766 );
    BOOST_CHECK_EQUAL( sdb.OleItems()[1].bottom, -10366 );
    BOOST_CHECK_EQUAL( sdb.OleItems()[1].right, -14686 );
    BOOST_CHECK_EQUAL( sdb.OleItems()[1].top, -10782 );
    BOOST_CHECK_EQUAL( sdb.OleItems()[1].sheetPlane, 0 );
    BOOST_CHECK_EQUAL( sdb.OleItems()[1].flags, 1 );

    OLE_IMAGE_PAYLOAD bitmap = ExtractOleImage( sdb.Bytes().data() + items[0]->offset, items[0]->bytes );
    BOOST_CHECK( bitmap.type == OLE_IMAGE_TYPE::BMP );
    BOOST_CHECK_EQUAL( bitmap.streamName, "\\x01Ole10Native" );
    BOOST_REQUIRE_GE( bitmap.data.size(), 2 );
    BOOST_CHECK_EQUAL( bitmap.data[0], 'B' );
    BOOST_CHECK_EQUAL( bitmap.data[1], 'M' );

    OLE_IMAGE_PAYLOAD metafile = ExtractOleImage( sdb.Bytes().data() + items[1]->offset, items[1]->bytes );
    BOOST_CHECK( metafile.type == OLE_IMAGE_TYPE::WMF );
    BOOST_CHECK_EQUAL( metafile.streamName, "\\x01Ole10Native" );
    BOOST_REQUIRE_GE( metafile.data.size(), 4 );
    BOOST_CHECK_EQUAL( metafile.data[0], 0xD7 );
    BOOST_CHECK_EQUAL( metafile.data[1], 0xCD );
    BOOST_CHECK_EQUAL( metafile.data[2], 0xC6 );
    BOOST_CHECK_EQUAL( metafile.data[3], 0x9A );
    wxImage rendered;
    BOOST_REQUIRE( OleRenderWmf( metafile.data, 1024, 1024, rendered ) );
    BOOST_CHECK( rendered.IsOk() );
    BOOST_CHECK_GT( rendered.GetWidth(), 0 );
    BOOST_CHECK_GT( rendered.GetHeight(), 0 );
}


BOOST_AUTO_TEST_CASE( RejectsEmbeddedOleWrongSheet )
{
    std::vector<uint8_t> bytes =
            loadBinary( KI_TEST::GetEeschemaTestDataDir() + "/plugins/pads/binary/ole_images.sch" );
    PADS_SCH_SDB parsed;
    parsed.Load( bytes );
    BOOST_REQUIRE_EQUAL( parsed.OleItems().size(), 2 );
    size_t sheetOffset = parsed.OleItems()[0].trailerOffset + 38 + 16 + 16;
    putU32( bytes, sheetOffset, 1 );

    PADS_SCH_SDB sdb;
    auto         hasOffset = [sheetOffset]( const IO_ERROR& e )
    {
        return errorContains( e, "v0x000D", wxString::Format( "0x%zX", sheetOffset ).ToStdString() );
    };
    BOOST_CHECK_EXCEPTION( sdb.Load( std::move( bytes ) ), IO_ERROR, hasOffset );
}


BOOST_AUTO_TEST_CASE( RejectsShortHeader )
{
    std::vector<uint8_t> bytes = loadPublicFixture();
    bytes.resize( 31 );

    PADS_SCH_SDB sdb;
    BOOST_CHECK_EXCEPTION( sdb.Load( std::move( bytes ) ), IO_ERROR,
                           []( const IO_ERROR& e )
                           {
                               return errorContains( e, "v0x000D", "0x1F" );
                           } );
}


BOOST_AUTO_TEST_CASE( RejectsBadMagic )
{
    std::vector<uint8_t> bytes = loadPublicFixture();
    bytes[1] ^= 1;

    PADS_SCH_SDB sdb;
    BOOST_CHECK_EXCEPTION( sdb.Load( std::move( bytes ) ), IO_ERROR,
                           []( const IO_ERROR& e )
                           {
                               return errorContains( e, "v0x000D", "0x1" );
                           } );
}


BOOST_AUTO_TEST_CASE( RejectsUnsupportedVersion )
{
    std::vector<uint8_t> bytes = loadPublicFixture();
    bytes[2] = 0x34;
    bytes[3] = 0x12;

    PADS_SCH_SDB sdb;
    BOOST_CHECK_EXCEPTION( sdb.Load( std::move( bytes ) ), IO_ERROR,
                           []( const IO_ERROR& e )
                           {
                               return errorContains( e, "v0x1234", "0x2" );
                           } );
}


BOOST_AUTO_TEST_CASE( RejectsBadV13FormatFlags )
{
    std::vector<uint8_t> bytes = loadPublicFixture();
    bytes[4] = 1;

    PADS_SCH_SDB sdb;
    BOOST_CHECK_EXCEPTION( sdb.Load( std::move( bytes ) ), IO_ERROR,
                           []( const IO_ERROR& e )
                           {
                               return errorContains( e, "v0x000D", "0x4" );
                           } );
}


BOOST_AUTO_TEST_CASE( RejectsBadV12FormatFlags )
{
    std::vector<uint8_t> bytes = loadPublicFixture();
    bytes[2] = 0x0C;
    bytes[4] = 0;

    PADS_SCH_SDB sdb;
    BOOST_CHECK_EXCEPTION( sdb.Load( std::move( bytes ) ), IO_ERROR,
                           []( const IO_ERROR& e )
                           {
                               return errorContains( e, "v0x000C", "0x4" );
                           } );
}


BOOST_AUTO_TEST_CASE( RejectsBadFooterGuid )
{
    std::vector<uint8_t> bytes = loadPublicFixture();
    size_t               footer = bytes.size() - 42;
    bytes[footer] ^= 1;

    PADS_SCH_SDB sdb;
    BOOST_CHECK_EXCEPTION( sdb.Load( std::move( bytes ) ), IO_ERROR,
                           [footer]( const IO_ERROR& e )
                           {
                               return errorContains( e, "v0x000D", wxString::Format( "0x%zX", footer ).ToStdString() );
                           } );
}


BOOST_AUTO_TEST_CASE( RejectsBadFooterBackPointer )
{
    std::vector<uint8_t> bytes = loadPublicFixture();
    size_t               pointerOffset = bytes.size() - 4;
    putU32( bytes, pointerOffset, static_cast<uint32_t>( bytes.size() + 1 ) );

    PADS_SCH_SDB sdb;
    BOOST_CHECK_EXCEPTION( sdb.Load( std::move( bytes ) ), IO_ERROR,
                           [pointerOffset]( const IO_ERROR& e )
                           {
                               return errorContains( e, "v0x000D",
                                                     wxString::Format( "0x%zX", pointerOffset ).ToStdString() );
                           } );
}


BOOST_AUTO_TEST_CASE( RejectsUsedBytesAboveAllocation )
{
    std::vector<uint8_t> bytes = loadPublicFixture();
    constexpr size_t     usedBytesOffset = 0x20 + 12;
    putU32( bytes, usedBytesOffset, 589 );

    PADS_SCH_SDB sdb;
    BOOST_CHECK_EXCEPTION( sdb.Load( std::move( bytes ) ), IO_ERROR,
                           []( const IO_ERROR& e )
                           {
                               return errorContains( e, "v0x000D", "0x2C" );
                           } );
}


BOOST_AUTO_TEST_CASE( RejectsUsedBytesForEmptyPool )
{
    std::vector<uint8_t> bytes = loadPublicFixture();
    constexpr size_t     descriptor = 0x20 + 9 * 28;
    putU32( bytes, descriptor + 12, 1 );

    PADS_SCH_SDB sdb;
    BOOST_CHECK_EXCEPTION( sdb.Load( std::move( bytes ) ), IO_ERROR,
                           []( const IO_ERROR& e )
                           {
                               return errorContains( e, "v0x000D", "0x128" );
                           } );
}


BOOST_AUTO_TEST_CASE( RejectsOuterControllerPastFooter )
{
    std::vector<uint8_t> bytes = loadPublicFixture();
    constexpr size_t     descriptor = 0x20 + 1 * 28;
    putU32( bytes, descriptor + 4, UINT32_MAX );
    putU32( bytes, descriptor + 12, UINT32_MAX );

    PADS_SCH_SDB sdb;
    BOOST_CHECK_EXCEPTION( sdb.Load( std::move( bytes ) ), IO_ERROR,
                           []( const IO_ERROR& e )
                           {
                               return errorContains( e, "v0x000D", "0x48" );
                           } );
}


BOOST_AUTO_TEST_CASE( RejectsSheetControllerPastFooter )
{
    std::vector<uint8_t> bytes = loadPublicFixture();
    constexpr size_t     descriptor = 0x5439 + 20;
    putU32( bytes, descriptor + 8, UINT32_MAX );
    putU32( bytes, descriptor + 16, UINT32_MAX );

    PADS_SCH_SDB sdb;
    BOOST_CHECK_EXCEPTION( sdb.Load( std::move( bytes ) ), IO_ERROR,
                           []( const IO_ERROR& e )
                           {
                               return errorContains( e, "v0x000D", "0x545D" );
                           } );
}


BOOST_AUTO_TEST_CASE( RejectsDerivedSheetCountPastFooter )
{
    std::vector<uint8_t> bytes = loadPublicFixture();
    constexpr size_t     countOffset = 0x20 + 3 * 28 + 8;
    putU32( bytes, countOffset, UINT32_MAX );

    PADS_SCH_SDB sdb;
    BOOST_CHECK_EXCEPTION( sdb.Load( std::move( bytes ) ), IO_ERROR,
                           []( const IO_ERROR& e )
                           {
                               return errorContains( e, "v0x000D", "0x7C" );
                           } );
}


BOOST_AUTO_TEST_CASE( RejectsHugePreviewCountExtent )
{
    std::vector<uint8_t> bytes = loadPublicFixture();
    constexpr size_t     itemCountOffset = 0x9938;
    putU32( bytes, itemCountOffset, UINT32_MAX );

    PADS_SCH_SDB sdb;
    BOOST_CHECK_EXCEPTION( sdb.Load( std::move( bytes ) ), IO_ERROR,
                           []( const IO_ERROR& e )
                           {
                               return errorContains( e, "v0x000D", "0x9938" );
                           } );
}


BOOST_AUTO_TEST_CASE( RejectsFooterAuxBaseBeforePayload )
{
    std::vector<uint8_t> bytes = loadPublicFixture();
    size_t               pointerOffset = bytes.size() - 4;
    putU32( bytes, pointerOffset, 0x24F );

    PADS_SCH_SDB sdb;
    BOOST_CHECK_EXCEPTION( sdb.Load( std::move( bytes ) ), IO_ERROR,
                           [pointerOffset]( const IO_ERROR& e )
                           {
                               return errorContains( e, "v0x000D",
                                                     wxString::Format( "0x%zX", pointerOffset ).ToStdString() );
                           } );
}


BOOST_AUTO_TEST_CASE( RejectsFooterAuxOverlapWithDerivedBlocks )
{
    std::vector<uint8_t> bytes = loadPublicFixture();
    size_t               pointerOffset = bytes.size() - 4;
    putU32( bytes, pointerOffset, 0x250 );

    PADS_SCH_SDB sdb;
    BOOST_CHECK_EXCEPTION( sdb.Load( std::move( bytes ) ), IO_ERROR,
                           [pointerOffset]( const IO_ERROR& e )
                           {
                               return errorContains( e, "v0x000D",
                                                     wxString::Format( "0x%zX", pointerOffset ).ToStdString() );
                           } );
}


BOOST_AUTO_TEST_SUITE_END()

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

#include <sch_io/orcad/orcad_ole.h>
#include <sch_io/ole_image.h>

#include <algorithm>
#include <boost/endian/conversion.hpp>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <optional>
#include <string_view>
#include <utility>

#include <wx/image.h>

#include <compoundfilereader.h>
#include <paths.h>

#include <libwmf/api.h>
#include <libwmf/gd.h>


namespace
{

constexpr std::array<uint8_t, 8> CFB_MAGIC = { 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1 };


uint32_t readU32( const uint8_t* aData )
{
    return boost::endian::load_little_u32( aData );
}


uint16_t readU16( const uint8_t* aData )
{
    return boost::endian::load_little_u16( aData );
}


bool entryNameIs( const CFB::COMPOUND_FILE_ENTRY* aEntry, uint16_t aPrefix, std::string_view aName )
{
    size_t length = aEntry->nameLen >= 2 ? aEntry->nameLen / 2 - 1 : 0;

    if( length != aName.size() + 1 || aEntry->name[0] != aPrefix )
        return false;

    for( size_t i = 0; i < aName.size(); ++i )
    {
        if( aEntry->name[i + 1] != static_cast<uint8_t>( aName[i] ) )
            return false;
    }

    return true;
}


std::vector<uint8_t> readStream( const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    uint64_t size = aReader.GetStreamSize( aEntry );

    if( size > aReader.GetBufferLen() || size > std::numeric_limits<size_t>::max() )
        return {};

    std::vector<uint8_t> data( static_cast<size_t>( size ) );

    if( !data.empty() )
        aReader.ReadFile( aEntry, 0, reinterpret_cast<char*>( data.data() ), data.size() );

    return data;
}

// The 26-byte prologue stores the compound length at offset 22 and length plus 22 at offset 0.
std::optional<std::pair<size_t, size_t>> embeddedCompoundFile( const std::vector<uint8_t>& aPayload )
{
    constexpr size_t PROLOGUE = 26;

    if( aPayload.size() < PROLOGUE + CFB_MAGIC.size() )
        return std::nullopt;

    uint64_t stated = readU32( aPayload.data() );
    uint64_t length = readU32( aPayload.data() + 22 );

    // The two length fields must agree. Capture truncates the unused tail of the final sector,
    // so the length is not always a multiple of 512; never round it up.
    if( stated != length + 22 || length < CFB_MAGIC.size() )
        return std::nullopt;

    if( !std::equal( CFB_MAGIC.begin(), CFB_MAGIC.end(), aPayload.begin() + PROLOGUE ) )
        return std::nullopt;

    // A container cut short of its stated length still reads; the compound file reader pads the
    // final sector. Clamping keeps that working without letting the length address absent bytes.
    size_t extent = std::min<size_t>( length, aPayload.size() - PROLOGUE );

    return std::make_pair( PROLOGUE, extent );
}

} // namespace


std::vector<uint8_t> OrcadExtractEmbeddedEmf( const std::vector<uint8_t>& aWmf )
{
    constexpr uint32_t WMFC_IDENTIFIER = 0x43464D57;
    constexpr uint16_t META_ESCAPE = 0x0626;
    constexpr uint16_t ENHANCED_METAFILE = 0x000F;
    constexpr size_t   COMMENT_HEADER_SIZE = 34;

    size_t headerOffset = 0;

    if( aWmf.size() >= 4 && readU32( aWmf.data() ) == 0x9AC6CDD7 )
        headerOffset = 22;

    if( aWmf.size() < headerOffset + 18 || readU16( aWmf.data() + headerOffset + 2 ) != 9 )
        return {};

    size_t               offset = headerOffset + 18;
    uint32_t             expectedRecordCount = 0;
    uint32_t             expectedEmfSize = 0;
    uint32_t             chunkCount = 0;
    std::vector<uint8_t> emf;

    while( offset + 6 <= aWmf.size() )
    {
        uint32_t sizeWords = readU32( aWmf.data() + offset );

        if( sizeWords < 3 || sizeWords > std::numeric_limits<size_t>::max() / 2 )
            return {};

        size_t recordSize = static_cast<size_t>( sizeWords ) * 2;

        if( recordSize > aWmf.size() - offset )
            return {};

        uint16_t function = readU16( aWmf.data() + offset + 4 );

        if( function == META_ESCAPE && recordSize >= 10 + COMMENT_HEADER_SIZE
            && readU16( aWmf.data() + offset + 6 ) == ENHANCED_METAFILE )
        {
            uint16_t byteCount = readU16( aWmf.data() + offset + 8 );

            if( byteCount < COMMENT_HEADER_SIZE || static_cast<size_t>( byteCount ) + 10 > recordSize )
                return {};

            const uint8_t* header = aWmf.data() + offset + 10;

            if( readU32( header ) != WMFC_IDENTIFIER || readU32( header + 4 ) != 1 )
                return {};

            uint32_t recordCount = readU32( header + 18 );
            uint32_t chunkSize = readU32( header + 22 );
            uint32_t remaining = readU32( header + 26 );
            uint32_t emfSize = readU32( header + 30 );

            if( chunkSize > byteCount - COMMENT_HEADER_SIZE || emfSize < remaining )
                return {};

            if( chunkCount == 0 )
            {
                expectedRecordCount = recordCount;
                expectedEmfSize = emfSize;

                if( emfSize > aWmf.size() )
                    return {};

                emf.reserve( emfSize );
            }
            else if( recordCount != expectedRecordCount || emfSize != expectedEmfSize )
            {
                return {};
            }

            if( chunkSize > expectedEmfSize - emf.size()
                || remaining != expectedEmfSize - emf.size() - chunkSize )
                return {};

            emf.insert( emf.end(), header + COMMENT_HEADER_SIZE, header + COMMENT_HEADER_SIZE + chunkSize );
            ++chunkCount;
        }

        offset += recordSize;

        if( function == 0 )
            break;
    }

    if( chunkCount == 0 || chunkCount != expectedRecordCount || emf.size() != expectedEmfSize || emf.size() < 52
        || readU32( emf.data() ) != 1 || readU32( emf.data() + 40 ) != 0x464D4520
        || readU32( emf.data() + 48 ) != emf.size() )
    {
        return {};
    }

    return emf;
}


std::vector<uint8_t> OrcadExtractCiImage( const std::vector<uint8_t>& aPayload )
{
    constexpr std::string_view ciMarker = "~~CI_IMAGE~~";
    constexpr size_t           DIB_HEADER = 40;

    // The CI marker follows the preview DIB, including its palette and padded rows.
    if( aPayload.size() < DIB_HEADER )
        return {};

    uint32_t headerSize = readU32( aPayload.data() );
    int32_t  width = static_cast<int32_t>( readU32( aPayload.data() + 4 ) );
    int32_t  height = static_cast<int32_t>( readU32( aPayload.data() + 8 ) );
    uint16_t planes = readU16( aPayload.data() + 12 );
    uint16_t depth = readU16( aPayload.data() + 14 );
    uint32_t paletteEntries = readU32( aPayload.data() + 32 );

    if( headerSize != DIB_HEADER || planes != 1 || width <= 0 || height == 0 )
        return {};

    if( depth != 1 && depth != 4 && depth != 8 && depth != 16 && depth != 24 && depth != 32 )
        return {};

    if( !paletteEntries && depth <= 8 )
        paletteEntries = uint32_t( 1 ) << depth;

    uint64_t rows = height < 0 ? -static_cast<int64_t>( height ) : height;
    uint64_t stride = ( ( static_cast<uint64_t>( width ) * depth + 31 ) / 32 ) * 4;
    uint64_t headerBytes = DIB_HEADER + static_cast<uint64_t>( paletteEntries ) * 4;

    if( headerBytes > aPayload.size() || rows > ( aPayload.size() - headerBytes ) / stride )
        return {};

    size_t previewSize = static_cast<size_t>( headerBytes + stride * rows );

    if( aPayload.size() - previewSize < ciMarker.size() )
        return {};

    auto marker = aPayload.begin() + previewSize;

    if( !std::equal( ciMarker.begin(), ciMarker.end(), marker ) )
        return {};

    if( embeddedCompoundFile( aPayload ) )
        return {};

    // After the marker: NUL, digit count, decimal byte length, then the raster bytes.
    auto header = marker + ciMarker.size();

    if( aPayload.end() - header < 2 || *header != 0 )
        return {};

    size_t digits = header[1];

    if( digits < 1 || digits > 10 || static_cast<size_t>( aPayload.end() - header ) < 2 + digits )
        return {};

    size_t length = 0;

    for( size_t i = 0; i < digits; ++i )
    {
        uint8_t byte = header[2 + i];

        if( byte < '0' || byte > '9' )
            return {};

        length = length * 10 + static_cast<size_t>( byte - '0' );
    }

    auto image = header + 2 + digits;

    if( static_cast<size_t>( aPayload.end() - image ) < length )
        return {};

    return std::vector<uint8_t>( image, image + length );
}


ORCAD_OLE_PREVIEW OrcadExtractOlePreview( const std::vector<uint8_t>& aPayload )
{
    std::optional<std::pair<size_t, size_t>> located = embeddedCompoundFile( aPayload );

    if( !located )
        return {};

    try
    {
        auto                 begin = aPayload.begin() + located->first;
        std::vector<uint8_t> compound( begin, begin + located->second );
        compound.resize( ( compound.size() + 511 ) & ~size_t( 511 ) );
        CFB::CompoundFileReader         reader( compound.data(), compound.size() );
        const CFB::COMPOUND_FILE_ENTRY* presentation = nullptr;
        const CFB::COMPOUND_FILE_ENTRY* native = nullptr;

        reader.EnumFiles( reader.GetRootEntry(), -1,
                          [&]( const CFB::COMPOUND_FILE_ENTRY* aEntry, const CFB::utf16string&, int )
                          {
                              if( !reader.IsStream( aEntry ) )
                                  return 0;

                              if( entryNameIs( aEntry, 2, "OlePres000" ) )
                                  presentation = aEntry;
                              else if( entryNameIs( aEntry, 1, "Ole10Native" ) )
                                  native = aEntry;

                              return 0;
                          } );

        if( native && !presentation )
        {
            // Ole10Native contains a u32 length and server-specific bytes. Accept only a recognized raster.
            std::vector<uint8_t> data = readStream( reader, native );

            if( data.size() >= 4 )
            {
                size_t length = readU32( data.data() );

                if( length <= data.size() - 4 )
                {
                    const uint8_t* body = data.data() + 4;
                    bool           raster =
                            ( length >= 2 && body[0] == 'B' && body[1] == 'M' )
                            || ( length >= 4 && body[0] == 0x89 && body[1] == 'P' && body[2] == 'N'
                                 && body[3] == 'G' )
                            || ( length >= 3 && body[0] == 0xFF && body[1] == 0xD8 && body[2] == 0xFF );

                    if( raster )
                        return { ORCAD_OLE_PREVIEW_TYPE::BMP, { body, body + length } };
                }
            }
        }

        if( presentation )
        {
            std::vector<uint8_t> data = readStream( reader, presentation );

            if( data.size() >= 40 )
            {
                uint32_t clipboardFormat = readU32( data.data() + 4 );

                if( clipboardFormat == 3 || clipboardFormat == 14 )
                {
                    std::vector<uint8_t> wmf( data.begin() + 40, data.end() );

                    if( wmf.size() >= 18 && wmf[0] == 1 && wmf[1] == 0 && wmf[2] == 9 && wmf[3] == 0 )
                    {
                        size_t declaredSize = static_cast<size_t>( readU32( wmf.data() + 6 ) ) * 2;

                        if( declaredSize >= 18 && declaredSize <= wmf.size() )
                            wmf.resize( declaredSize );
                    }

                    return { ORCAD_OLE_PREVIEW_TYPE::WMF, std::move( wmf ) };
                }

                if( clipboardFormat == 8 )
                    return { ORCAD_OLE_PREVIEW_TYPE::DIB, { data.begin() + 40, data.end() } };


            }
        }
    }
    catch( const std::exception& )
    {
    }

    return {};
}


bool OrcadRenderWmf( const std::vector<uint8_t>& aWmf, int aMaxWidth, int aMaxHeight, wxImage& aImage,
                     double aTargetAspect )
{
    if( aWmf.empty() || aWmf.size() > static_cast<size_t>( std::numeric_limits<long>::max() ) )
        return false;

    wmfAPI*        api = nullptr;
    wmfAPI_Options options{};
    wxCharBuffer   fontDir = ( PATHS::GetStockDataPath() + wxS( "/libwmf/fonts" ) ).utf8_str();
    char*          fontDirs[] = { fontDir.data(), nullptr };
    options.function = wmf_gd_function;
    options.fontdirs = fontDirs;

    constexpr unsigned long flags =
            WMF_OPT_FUNCTION | WMF_OPT_FONTDIRS | WMF_OPT_IGNORE_NONFATAL | WMF_OPT_NO_DEBUG | WMF_OPT_NO_ERROR;

    if( wmf_api_create( &api, flags, &options ) != wmf_E_None )
        return false;

    auto destroyApi = [&]
    {
        wmf_api_destroy( api );
        api = nullptr;
    };

    wmf_gd_t* gd = WMF_GD_GetData( api );
    gd->type = wmf_gd_image;

    if( wmf_mem_open( api, const_cast<uint8_t*>( aWmf.data() ), static_cast<long>( aWmf.size() ) ) != wmf_E_None )
    {
        destroyApi();
        return false;
    }

    wmfD_Rect bbox;

    if( wmf_scan( api, 0, &bbox ) != wmf_E_None )
    {
        destroyApi();
        return false;
    }

    unsigned int naturalWidth = 0;
    unsigned int naturalHeight = 0;

    if( wmf_display_size( api, &naturalWidth, &naturalHeight, 144.0, 144.0 ) != wmf_E_None || naturalWidth == 0
        || naturalHeight == 0 )
    {
        destroyApi();
        return false;
    }

    VECTOR2I renderSize = OleWmfRenderSize( naturalWidth, naturalHeight, std::max( 1, aMaxWidth ),
                                              std::max( 1, aMaxHeight ), aTargetAspect );
    unsigned int width = static_cast<unsigned int>( renderSize.x );
    unsigned int height = static_cast<unsigned int>( renderSize.y );

    gd->bbox = bbox;
    gd->width = width;
    gd->height = height;

    if( wmf_play( api, 0, &bbox ) != wmf_E_None )
    {
        destroyApi();
        return false;
    }

    int* pixels = wmf_gd_get_image_pixels( api );

    if( !pixels || !aImage.Create( width, height, false ) )
    {
        destroyApi();
        return false;
    }

    unsigned char* rgb = aImage.GetData();

    for( size_t i = 0; i < static_cast<size_t>( width ) * height; ++i )
    {
        rgb[3 * i] = static_cast<unsigned char>( ( pixels[i] >> 16 ) & 0xFF );
        rgb[3 * i + 1] = static_cast<unsigned char>( ( pixels[i] >> 8 ) & 0xFF );
        rgb[3 * i + 2] = static_cast<unsigned char>( pixels[i] & 0xFF );
    }

    destroyApi();
    return true;
}


bool OrcadRenderMetafilePreview( const std::vector<uint8_t>& aWmf, int aMaxWidth, int aMaxHeight,
                                 wxImage& aImage, double aTargetAspect, bool* aUsedEmbeddedEmf )
{
    if( aUsedEmbeddedEmf )
        *aUsedEmbeddedEmf = false;

    std::vector<uint8_t> emf = OrcadExtractEmbeddedEmf( aWmf );

    if( !emf.empty() && OleRenderEmf( emf, aMaxWidth, aMaxHeight, aImage, aTargetAspect ) )
    {
        if( aUsedEmbeddedEmf )
            *aUsedEmbeddedEmf = true;

        return true;
    }

    return OrcadRenderWmf( aWmf, aMaxWidth, aMaxHeight, aImage, aTargetAspect );
}


wxString OrcadDescribeImagePayload( const std::vector<uint8_t>& aPayload )
{
    if( aPayload.empty() )
        return wxS( "empty" );

    auto starts = [&]( std::initializer_list<uint8_t> aSig, size_t aOffset = 0 )
    {
        if( aPayload.size() < aOffset + aSig.size() )
            return false;

        return std::equal( aSig.begin(), aSig.end(), aPayload.begin() + aOffset );
    };

    if( starts( { 0x89, 'P', 'N', 'G' } ) )
        return wxS( "PNG" );
    if( starts( { 0xFF, 0xD8, 0xFF } ) )
        return wxS( "JPEG" );
    if( starts( { 'G', 'I', 'F', '8' } ) )
        return wxS( "GIF" );
    if( starts( { 'B', 'M' } ) )
        return wxS( "BMP" );
    if( starts( { 'I', 'I', 0x2A, 0x00 } ) )
        return wxS( "TIFF" );
    if( starts( { 'M', 'M', 0x00, 0x2A } ) )
        return wxS( "TIFF" );
    if( starts( { 0xD7, 0xCD, 0xC6, 0x9A } ) )
        return wxS( "placeable WMF" );
    if( starts( { 0x01, 0x00, 0x09, 0x00 } ) )
        return wxS( "WMF" );
    if( starts( { 'E', 'M', 'F', 0x20 }, 40 ) )
        return wxS( "EMF" );
    if( starts( { 0xD0, 0xCF, 0x11, 0xE0 } ) )
        return wxS( "OLE compound document" );

    wxString head;

    for( size_t i = 0; i < std::min<size_t>( 8, aPayload.size() ); ++i )
        head += wxString::Format( wxS( "%02X" ), aPayload[i] );

    return wxString::Format( wxS( "unrecognized, %zu bytes starting %s" ), aPayload.size(), head );
}

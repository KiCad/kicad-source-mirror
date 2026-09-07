/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your option)
 * any later version.
 */

#include <sch_io/ole_image.h>

#include <algorithm>
#include <boost/endian/conversion.hpp>
#include <memory>
#include <utility>
#include <optional>
#include <array>
#include <cmath>
#include <limits>
#include <string_view>

#include <wx/buffer.h>
#include <wx/filename.h>
#include <wx/image.h>

#include <math/vector2d.h>
#include <wx/log.h>

#include <compoundfilereader.h>
#include <paths.h>
#include <trace_helpers.h>

#include <libwmf/api.h>
#include <libwmf/gd.h>


namespace
{

constexpr std::array<uint8_t, 8> CFB_MAGIC = { 0xD0, 0xCF, 0x11, 0xE0, 0xA1, 0xB1, 0x1A, 0xE1 };

constexpr size_t MAX_CFB_BYTES = 256 * 1024 * 1024;
constexpr size_t MAX_STREAM_BYTES = 64 * 1024 * 1024;


uint16_t readU16( const uint8_t* aData )
{
    return boost::endian::load_little_u16( aData );
}


uint32_t readU32( const uint8_t* aData )
{
    return boost::endian::load_little_u32( aData );
}


bool entryNameIs( const CFB::COMPOUND_FILE_ENTRY* aEntry, std::u16string_view aName )
{
    // nameLen counts bytes including the terminator and comes from the file, so require the exact
    // encoded length rather than letting an odd value divide down onto a real name
    if( aEntry->nameLen != 2 * ( aName.size() + 1 ) || aEntry->name[aName.size()] != 0 )
        return false;

    for( size_t i = 0; i < aName.size(); ++i )
    {
        if( aEntry->name[i] != static_cast<uint16_t>( aName[i] ) )
            return false;
    }

    return true;
}


std::vector<uint8_t> readStream( const CFB::CompoundFileReader& aReader, const CFB::COMPOUND_FILE_ENTRY* aEntry )
{
    uint64_t size = aReader.GetStreamSize( aEntry );

    if( size > MAX_STREAM_BYTES || size > aReader.GetBufferLen() || size > std::numeric_limits<size_t>::max() )
        return {};

    std::vector<uint8_t> data( static_cast<size_t>( size ) );

    if( !data.empty() )
        aReader.ReadFile( aEntry, 0, reinterpret_cast<char*>( data.data() ), data.size() );

    return data;
}


bool isWmf( const uint8_t* aData, size_t aSize )
{
    if( aSize >= 4 && readU32( aData ) == 0x9AC6CDD7 )
        return true;

    return aSize >= 4 && ( readU16( aData ) == 1 || readU16( aData ) == 2 ) && readU16( aData + 2 ) == 9;
}


wxString wmfFontDirectory()
{
    wxFileName fontDir;
    fontDir.AssignDir( PATHS::GetStockDataPath() );
    fontDir.AppendDir( wxS( "libwmf" ) );
    fontDir.AppendDir( wxS( "fonts" ) );

    if( fontDir.DirExists() )
        return fontDir.GetPath();

    wxFileName buildDir;
    buildDir.AssignDir( PATHS::GetExecutablePath() );

    for( int depth = 0; depth < 4; ++depth )
    {
        wxFileName candidate = buildDir;
        candidate.AppendDir( wxS( "libwmf" ) );
        candidate.AppendDir( wxS( "fonts" ) );

        if( candidate.DirExists() )
            return candidate.GetPath();

        buildDir.RemoveLastDir();
    }

    wxLogTrace( traceSchPlugin, wxS( "libwmf fonts missing (looked for %s); WMF rendering will fail" ),
                fontDir.GetPath() );

    return fontDir.GetPath();
}


OLE_IMAGE_PAYLOAD classifyContents( std::vector<uint8_t> aData, std::string aName )
{
    if( ( aData.size() >= 2 && aData[0] == 'B' && aData[1] == 'M' )
        || ( aData.size() >= 4 && aData[0] == 0x89 && aData[1] == 'P' && aData[2] == 'N' && aData[3] == 'G' )
        || ( aData.size() >= 3 && aData[0] == 0xFF && aData[1] == 0xD8 && aData[2] == 0xFF ) )
        return { OLE_IMAGE_TYPE::BMP, std::move( aData ), std::move( aName ) };

    if( aData.size() >= 40 )
    {
        // Validate the DIB fields before selecting CONTENTS and hiding a valid presentation.
        uint32_t headerSize = readU32( aData.data() );
        int32_t  width = static_cast<int32_t>( readU32( aData.data() + 4 ) );
        int32_t  height = static_cast<int32_t>( readU32( aData.data() + 8 ) );
        uint16_t planes = readU16( aData.data() + 12 );
        uint16_t bitCount = readU16( aData.data() + 14 );

        bool depthIsValid = bitCount == 1 || bitCount == 4 || bitCount == 8 || bitCount == 16
                            || bitCount == 24 || bitCount == 32;

        if( headerSize >= 40 && headerSize <= 200 && planes == 1 && depthIsValid && width != 0
            && height != 0 )
        {
            return { OLE_IMAGE_TYPE::DIB, std::move( aData ), std::move( aName ) };
        }
    }

    if( isWmf( aData.data(), aData.size() ) )
        return { OLE_IMAGE_TYPE::WMF, std::move( aData ), std::move( aName ) };

    return {};
}


OLE_IMAGE_PAYLOAD classifyPresentation( std::vector<uint8_t> aData )
{
    if( aData.size() < 40 )
        return {};

    uint32_t       clipboardFormat = readU32( aData.data() + 4 );
    OLE_IMAGE_TYPE type = OLE_IMAGE_TYPE::NONE;

    if( clipboardFormat == 3 || clipboardFormat == 14 )
        type = OLE_IMAGE_TYPE::WMF;
    else if( clipboardFormat == 8 )
        type = OLE_IMAGE_TYPE::DIB;

    if( type == OLE_IMAGE_TYPE::NONE )
        return {};

    return { type, { aData.begin() + 40, aData.end() }, "\\x02OlePres000" };
}


OLE_IMAGE_PAYLOAD classifyNative( const std::vector<uint8_t>& aData )
{
    // Ole10Native starts with a length. A 0x0002 flag selects the Packager header.
    if( aData.size() < 6 )
        return {};

    size_t stated = readU32( aData.data() );

    if( stated > aData.size() - 4 )
        return {};

    size_t offset = 4;
    size_t length = stated;

    if( readU16( aData.data() + 4 ) == 0x0002 )
    {
        offset += 2;

        // The label and the originating path are NUL terminated; the temporary path that
        // follows them is counted instead.
        for( int i = 0; i < 2; ++i )
        {
            while( offset < aData.size() && aData[offset] != 0 )
                ++offset;

            if( offset >= aData.size() )
                return {};

            ++offset;
        }

        if( aData.size() - offset < 8 )
            return {};

        offset += 4;

        size_t pathLength = readU32( aData.data() + offset );
        offset += 4;

        if( pathLength > aData.size() - offset )
            return {};

        offset += pathLength;

        if( aData.size() - offset < 4 )
            return {};

        length = readU32( aData.data() + offset );
        offset += 4;

        if( length > aData.size() - offset )
            return {};
    }

    std::vector<uint8_t> body( aData.begin() + offset, aData.begin() + offset + length );

    return classifyContents( std::move( body ), "\\x01Ole10Native" );
}


} // namespace


std::optional<std::pair<size_t, size_t>> OleEmbeddedCompoundFile( const std::vector<uint8_t>& aPayload )
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


OLE_IMAGE_PAYLOAD ExtractOleImage( const uint8_t* aCfb, size_t aSize )
{
    if( !aCfb || aSize < 512 || aSize > MAX_CFB_BYTES )
        return {};

    try
    {
        CFB::CompoundFileReader         reader( aCfb, aSize );
        const CFB::COMPOUND_FILE_ENTRY* contents = nullptr;
        const CFB::COMPOUND_FILE_ENTRY* presentation = nullptr;
        const CFB::COMPOUND_FILE_ENTRY* native = nullptr;

        reader.EnumFiles( reader.GetRootEntry(), -1,
                          [&]( const CFB::COMPOUND_FILE_ENTRY* aEntry, const CFB::utf16string&, int )
                          {
                              if( !reader.IsStream( aEntry ) )
                                  return 0;

                              if( entryNameIs( aEntry, u"CONTENTS" ) )
                                  contents = aEntry;
                              else if( entryNameIs( aEntry, u"\x02OlePres000" ) )
                                  presentation = aEntry;
                              else if( entryNameIs( aEntry, u"\x01Ole10Native" ) )
                                  native = aEntry;

                              return 0;
                          } );

        if( contents )
        {
            OLE_IMAGE_PAYLOAD result = classifyContents( readStream( reader, contents ), "CONTENTS" );

            if( result.type != OLE_IMAGE_TYPE::NONE )
                return result;
        }

        if( presentation )
        {
            OLE_IMAGE_PAYLOAD result = classifyPresentation( readStream( reader, presentation ) );

            if( result.type != OLE_IMAGE_TYPE::NONE )
                return result;
        }

        if( native )
            return classifyNative( readStream( reader, native ) );
    }
    catch( const std::exception& )
    {
    }

    return {};
}


OLE_IMAGE_PAYLOAD ExtractOleImageFromPayload( const std::vector<uint8_t>& aPayload )
{
    std::optional<std::pair<size_t, size_t>> located = OleEmbeddedCompoundFile( aPayload );

    if( !located )
        return {};

    // The reader wants whole sectors, and a container cut short of its stated length is still
    // readable once the final one is padded.
    std::vector<uint8_t> compound( aPayload.begin() + located->first,
                                   aPayload.begin() + located->first + located->second );
    compound.resize( ( compound.size() + 511 ) & ~size_t( 511 ) );

    return ExtractOleImage( compound.data(), compound.size() );
}


bool OleMakeBmpFromDib( const std::vector<uint8_t>& aDib, wxMemoryBuffer& aOut )
{
    if( aDib.size() < 40 || aDib.size() > std::numeric_limits<uint32_t>::max() - 14 )
        return false;

    uint32_t biSize = readU32( aDib.data() );

    if( biSize < 40 || biSize > 200 || biSize > aDib.size() )
        return false;

    uint32_t bitCount = readU16( aDib.data() + 14 );
    uint32_t compression = readU32( aDib.data() + 16 );
    uint32_t clrUsed = readU32( aDib.data() + 32 );
    uint32_t paletteEntries = clrUsed ? clrUsed : ( bitCount <= 8 ? ( 1u << bitCount ) : 0 );
    uint64_t pixelOffset = 14ULL + biSize + uint64_t( paletteEntries ) * 4 + ( compression == 3 ? 12 : 0 );
    uint32_t fileSize = 14 + static_cast<uint32_t>( aDib.size() );

    if( pixelOffset > fileSize )
        return false;

    std::array<uint8_t, 14> header{};
    header[0] = 'B';
    header[1] = 'M';

    for( int shift = 0; shift < 32; shift += 8 )
    {
        header[2 + shift / 8] = static_cast<uint8_t>( fileSize >> shift );
        header[10 + shift / 8] = static_cast<uint8_t>( pixelOffset >> shift );
    }

    aOut.AppendData( header.data(), header.size() );
    aOut.AppendData( aDib.data(), aDib.size() );
    return true;
}


std::vector<uint8_t> OleExtractEmbeddedEmf( const std::vector<uint8_t>& aWmf )
{
    constexpr uint32_t c_WMFC_IDENTIFIER = 0x43464D57;
    constexpr uint16_t c_META_ESCAPE = 0x0626;
    constexpr uint16_t c_ENHANCED_METAFILE = 0x000F;
    constexpr size_t   c_COMMENT_HEADER_SIZE = 34;

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

        if( function == c_META_ESCAPE && recordSize >= 10 + c_COMMENT_HEADER_SIZE
            && readU16( aWmf.data() + offset + 6 ) == c_ENHANCED_METAFILE )
        {
            uint16_t byteCount = readU16( aWmf.data() + offset + 8 );

            if( byteCount < c_COMMENT_HEADER_SIZE || static_cast<size_t>( byteCount ) + 10 > recordSize )
                return {};

            const uint8_t* header = aWmf.data() + offset + 10;

            if( readU32( header ) != c_WMFC_IDENTIFIER || readU32( header + 4 ) != 1 )
                return {};

            uint32_t recordCount = readU32( header + 18 );
            uint32_t chunkSize = readU32( header + 22 );
            uint32_t remaining = readU32( header + 26 );
            uint32_t emfSize = readU32( header + 30 );

            if( chunkSize > byteCount - c_COMMENT_HEADER_SIZE || emfSize < remaining )
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

            emf.insert( emf.end(), header + c_COMMENT_HEADER_SIZE, header + c_COMMENT_HEADER_SIZE + chunkSize );
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


std::vector<uint8_t> OleExtractCiImage( const std::vector<uint8_t>& aPayload )
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

    if( OleEmbeddedCompoundFile( aPayload ) )
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


VECTOR2I OleWmfRenderSize( int aNaturalWidth, int aNaturalHeight, int aMaxWidth, int aMaxHeight,
                             double aTargetAspect )
{
    if( aNaturalWidth <= 0 || aNaturalHeight <= 0 || aMaxWidth <= 0 || aMaxHeight <= 0 )
        return VECTOR2I( 0, 0 );

    if( !std::isfinite( aTargetAspect ) )
        return VECTOR2I( 0, 0 );

    if( aTargetAspect > 0.0 )
    {
        double width = aMaxWidth;
        double height = width / aTargetAspect;

        if( height > aMaxHeight )
        {
            height = aMaxHeight;
            width = height * aTargetAspect;
        }

        return VECTOR2I( std::max( 1, KiROUND( width ) ), std::max( 1, KiROUND( height ) ) );
    }

    double scale = std::min( static_cast<double>( aMaxWidth ) / aNaturalWidth,
                             static_cast<double>( aMaxHeight ) / aNaturalHeight );
    scale = std::min( scale, 1.0 );
    return VECTOR2I( std::max( 1, KiROUND( aNaturalWidth * scale ) ),
                     std::max( 1, KiROUND( aNaturalHeight * scale ) ) );
}

bool OleRenderWmf( const std::vector<uint8_t>& aWmf, int aMaxWidth, int aMaxHeight, wxImage& aImage,
                     double aTargetAspect )
{
    if( aWmf.empty() || aWmf.size() > MAX_STREAM_BYTES
        || aWmf.size() > static_cast<size_t>( std::numeric_limits<long>::max() ) )
    {
        return false;
    }

    // Copy the buffer for libwmf. Recompute the standard size when a placeable header is present.
    std::vector<uint8_t> normalized = aWmf;

    if( normalized.size() >= 40 && readU32( normalized.data() ) == 0x9AC6CDD7 )
    {
        uint32_t standardWords = static_cast<uint32_t>( ( normalized.size() - 22 ) / 2 );

        for( int shift = 0; shift < 32; shift += 8 )
            normalized[28 + shift / 8] = static_cast<uint8_t>( standardWords >> shift );
    }

    wmfAPI*        api = nullptr;
    wmfAPI_Options options{};
    wxCharBuffer   fontDir = wmfFontDirectory().utf8_str();
    char*          fontDirs[] = { fontDir.data(), nullptr };
    options.function = wmf_gd_function;
    options.fontdirs = fontDirs;

    constexpr unsigned long flags = WMF_OPT_FUNCTION | WMF_OPT_FONTDIRS | WMF_OPT_SYS_FONTS
                                    | WMF_OPT_IGNORE_NONFATAL | WMF_OPT_NO_DEBUG | WMF_OPT_NO_ERROR;

    if( wmf_api_create( &api, flags, &options ) != wmf_E_None )
        return false;

    std::unique_ptr<wmfAPI, decltype( &wmf_api_destroy )> apiOwner( api, wmf_api_destroy );

    wmf_gd_t* gd = WMF_GD_GetData( api );
    gd->type = wmf_gd_image;

    if( wmf_mem_open( api, normalized.data(), static_cast<long>( normalized.size() ) ) != wmf_E_None )
    {
        return false;
    }

    wmfD_Rect bbox;

    if( wmf_scan( api, 0, &bbox ) != wmf_E_None )
    {
        return false;
    }

    unsigned int naturalWidth = 0;
    unsigned int naturalHeight = 0;

    if( wmf_display_size( api, &naturalWidth, &naturalHeight, 144.0, 144.0 ) != wmf_E_None || naturalWidth == 0
        || naturalHeight == 0 )
    {
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
        return false;
    }

    int* pixels = wmf_gd_get_image_pixels( api );

    if( !pixels || !aImage.Create( width, height, false ) )
    {
        return false;
    }

    unsigned char* rgb = aImage.GetData();

    for( size_t i = 0; i < static_cast<size_t>( width ) * height; ++i )
    {
        rgb[3 * i] = static_cast<unsigned char>( ( pixels[i] >> 16 ) & 0xFF );
        rgb[3 * i + 1] = static_cast<unsigned char>( ( pixels[i] >> 8 ) & 0xFF );
        rgb[3 * i + 2] = static_cast<unsigned char>( pixels[i] & 0xFF );
    }

    return true;
}


bool OleRenderMetafilePreview( const std::vector<uint8_t>& aWmf, int aMaxWidth, int aMaxHeight,
                                 wxImage& aImage, double aTargetAspect, bool* aUsedEmbeddedEmf )
{
    if( aUsedEmbeddedEmf )
        *aUsedEmbeddedEmf = false;

    std::vector<uint8_t> emf = OleExtractEmbeddedEmf( aWmf );

    if( !emf.empty() && OleRenderEmf( emf, aMaxWidth, aMaxHeight, aImage, aTargetAspect ) )
    {
        if( aUsedEmbeddedEmf )
            *aUsedEmbeddedEmf = true;

        return true;
    }

    return OleRenderWmf( aWmf, aMaxWidth, aMaxHeight, aImage, aTargetAspect );
}


wxString OleDescribeImagePayload( const std::vector<uint8_t>& aPayload )
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

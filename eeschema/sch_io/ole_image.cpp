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
#include <array>
#include <cmath>
#include <limits>
#include <string_view>

#include <wx/buffer.h>
#include <wx/filename.h>
#include <wx/image.h>

#include <compoundfilereader.h>
#include <paths.h>

#include <libwmf/api.h>
#include <libwmf/gd.h>


namespace
{

constexpr size_t MAX_CFB_BYTES = 256 * 1024 * 1024;
constexpr size_t MAX_STREAM_BYTES = 64 * 1024 * 1024;


uint16_t readU16( const uint8_t* aData )
{
    return static_cast<uint16_t>( aData[0] ) | ( static_cast<uint16_t>( aData[1] ) << 8 );
}


uint32_t readU32( const uint8_t* aData )
{
    return static_cast<uint32_t>( aData[0] ) | ( static_cast<uint32_t>( aData[1] ) << 8 )
           | ( static_cast<uint32_t>( aData[2] ) << 16 ) | ( static_cast<uint32_t>( aData[3] ) << 24 );
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


size_t wmfPayloadSize( const uint8_t* aData, size_t aSize )
{
    size_t headerOffset = readU32( aData ) == 0x9AC6CDD7 ? 22 : 0;

    if( aSize < headerOffset + 18 || readU16( aData + headerOffset + 2 ) != 9 )
        return 0;

    size_t cursor = headerOffset + 18;

    while( cursor + 6 <= aSize )
    {
        uint32_t recordWords = readU32( aData + cursor );
        uint16_t function = readU16( aData + cursor + 4 );
        uint64_t recordBytes = uint64_t( recordWords ) * 2;

        if( recordWords < 3 || recordBytes > aSize - cursor )
            return 0;

        cursor += static_cast<size_t>( recordBytes );

        if( function == 0 )
            return cursor;
    }

    return 0;
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

    return fontDir.GetPath();
}


OLE_IMAGE_PAYLOAD classifyContents( std::vector<uint8_t> aData, std::string aName )
{
    if( aData.size() >= 2 && aData[0] == 'B' && aData[1] == 'M' )
        return { OLE_IMAGE_TYPE::BMP, std::move( aData ), std::move( aName ) };

    if( aData.size() >= 40 )
    {
        // A bare size word is a weak signature, and claiming the stream here permanently hides the
        // OlePres000 preview because the caller only falls through on an unrecognized type, not on
        // a failed decode. Check the rest of the BITMAPINFOHEADER before taking it.
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
    for( size_t offset = 0; offset < aData.size(); ++offset )
    {
        if( offset + 2 <= aData.size() && aData[offset] == 'B' && aData[offset + 1] == 'M' )
        {
            return { OLE_IMAGE_TYPE::BMP, { aData.begin() + offset, aData.end() }, "\\x01Ole10Native" };
        }

        if( isWmf( aData.data() + offset, aData.size() - offset ) )
        {
            size_t payloadBytes = wmfPayloadSize( aData.data() + offset, aData.size() - offset );

            if( payloadBytes )
                return { OLE_IMAGE_TYPE::WMF,
                         { aData.begin() + offset, aData.begin() + offset + payloadBytes },
                         "\\x01Ole10Native" };

            return { OLE_IMAGE_TYPE::WMF, { aData.begin() + offset, aData.end() }, "\\x01Ole10Native" };
        }
    }

    return {};
}

} // namespace


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


bool OleRenderWmf( const std::vector<uint8_t>& aWmf, int aMaxWidth, int aMaxHeight, wxImage& aImage )
{
    if( aWmf.empty() || aWmf.size() > MAX_STREAM_BYTES
        || aWmf.size() > static_cast<size_t>( std::numeric_limits<long>::max() ) )
    {
        return false;
    }

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

    constexpr unsigned long flags = WMF_OPT_FUNCTION | WMF_OPT_FONTDIRS | WMF_OPT_SYS_FONTS | WMF_OPT_IGNORE_NONFATAL
                                    | WMF_OPT_NO_DEBUG | WMF_OPT_NO_ERROR;

    if( wmf_api_create( &api, flags, &options ) != wmf_E_None )
    {
        return false;
    }

    auto destroyApi = [&]
    {
        wmf_api_destroy( api );
        api = nullptr;
    };

    wmf_gd_t* gd = WMF_GD_GetData( api );
    gd->type = wmf_gd_image;

    if( wmf_mem_open( api, normalized.data(), static_cast<long>( normalized.size() ) ) != wmf_E_None )
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

    double scale = std::min( static_cast<double>( std::max( 1, aMaxWidth ) ) / naturalWidth,
                             static_cast<double>( std::max( 1, aMaxHeight ) ) / naturalHeight );
    scale = std::min( scale, 1.0 );
    unsigned int width = static_cast<unsigned int>( std::max<long>( 1, std::lround( naturalWidth * scale ) ) );
    unsigned int height = static_cast<unsigned int>( std::max<long>( 1, std::lround( naturalHeight * scale ) ) );

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

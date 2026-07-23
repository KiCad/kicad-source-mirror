/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 */

#include <sch_io/orcad/orcad_ole.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <limits>
#include <string_view>

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
    return static_cast<uint32_t>( aData[0] ) | ( static_cast<uint32_t>( aData[1] ) << 8 )
           | ( static_cast<uint32_t>( aData[2] ) << 16 ) | ( static_cast<uint32_t>( aData[3] ) << 24 );
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

} // namespace


ORCAD_OLE_PREVIEW OrcadExtractOlePreview( const std::vector<uint8_t>& aPayload )
{
    auto cfb = std::search( aPayload.begin(), aPayload.end(), CFB_MAGIC.begin(), CFB_MAGIC.end() );

    if( cfb == aPayload.end() )
        return {};

    try
    {
        const uint8_t*                  base = &*cfb;
        size_t                          size = static_cast<size_t>( aPayload.end() - cfb );
        CFB::CompoundFileReader         reader( base, size );
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

        if( presentation )
        {
            std::vector<uint8_t> data = readStream( reader, presentation );

            if( data.size() >= 40 )
            {
                uint32_t clipboardFormat = readU32( data.data() + 4 );

                if( clipboardFormat == 3 )
                    return { ORCAD_OLE_PREVIEW_TYPE::WMF, { data.begin() + 40, data.end() } };

                if( clipboardFormat == 8 )
                    return { ORCAD_OLE_PREVIEW_TYPE::DIB, { data.begin() + 40, data.end() } };

                if( clipboardFormat == 14 )
                    return { ORCAD_OLE_PREVIEW_TYPE::WMF, { data.begin() + 40, data.end() } };
            }
        }

        if( native )
        {
            std::vector<uint8_t> data = readStream( reader, native );
            size_t               limit = std::min<size_t>( data.size(), 64 );

            for( size_t offset = 0; offset + 14 <= limit; ++offset )
            {
                if( data[offset] == 'B' && data[offset + 1] == 'M' )
                    return { ORCAD_OLE_PREVIEW_TYPE::BMP, { data.begin() + offset, data.end() } };
            }
        }
    }
    catch( const std::exception& )
    {
    }

    return {};
}


bool OrcadRenderWmf( const std::vector<uint8_t>& aWmf, int aMaxWidth, int aMaxHeight, wxImage& aImage )
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

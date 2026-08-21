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

#include <sch_io/ole_image.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string_view>

#include <boost/endian/conversion.hpp>
#include <emf2svg.h>
#include <fontconfig/fontconfig.h>
#include <nanosvg.h>
#include <nanosvgrast.h>
#include <paths.h>
#include <wx/filename.h>
#include <wx/image.h>
#include <wx/log.h>

namespace
{

constexpr size_t MAX_EMF_BYTES = 64 * 1024 * 1024;
constexpr size_t MAX_RENDER_PIXELS = 16 * 1024 * 1024;

using FONT_CONFIG = std::unique_ptr<FcConfig, decltype( &FcConfigDestroy )>;

FONT_CONFIG emfFontConfig()
{
    FONT_CONFIG config( FcConfigCreate(), FcConfigDestroy );

    if( !config )
        return config;

    wxString directory = PATHS::GetStockDataPath() + wxS( "/libwmf/fonts" );

    const char* aliases = R"(<fontconfig>
<cachedir prefix="xdg">fontconfig</cachedir>
<alias><family>Calibri</family><prefer><family>Carlito</family></prefer></alias>
<alias><family>Arial</family><prefer><family>Nimbus Sans</family></prefer></alias>
<alias><family>sans-serif</family><prefer><family>Source Sans 3</family></prefer></alias>
</fontconfig>)";

    if( !FcConfigParseAndLoadFromMemory( config.get(), reinterpret_cast<const FcChar8*>( aliases ), FcTrue ) )
        return FONT_CONFIG( nullptr, FcConfigDestroy );

    // Glyph indices belong to these font files, not other installed revisions of the same family.
    const char* files[] = { "SourceSansPro-Regular.ttf", "SourceSansPro-Semibold.ttf",
                            "SourceSans3-Regular.otf", "SourceSans3-Semibold.otf",
                            "Carlito-Regular.ttf", "Carlito-Bold.ttf",
                            "NimbusSans-Regular.t1", "NimbusSans-Bold.t1",
                            "NimbusSans-Italic.t1", "NimbusSans-BoldItalic.t1" };

    bool added = false;

    for( const char* file : files )
    {
        wxCharBuffer path = ( directory + wxS( "/" ) + wxString::FromUTF8( file ) ).utf8_str();

        if( FcConfigAppFontAddFile( config.get(), reinterpret_cast<const FcChar8*>( path.data() ) ) )
            added = true;
        else
            wxLogTrace( wxS( "OLE" ), wxS( "Cannot load EMF font %s" ), wxString::FromUTF8( path.data() ) );
    }

    if( !added )
        config.reset();

    return config;
}


bool containsRaster( std::string_view aSvg )
{
    size_t offset = 0;

    while( ( offset = aSvg.find( '<', offset ) ) != std::string_view::npos )
    {
        if( aSvg.substr( offset, 9 ) == "<![CDATA[" )
        {
            offset = aSvg.find( "]]>", offset + 9 );

            if( offset == std::string_view::npos )
                return false;

            offset += 3;
        }
        else if( aSvg.substr( offset, 7 ) == "<image " )
        {
            return true;
        }
        else
        {
            ++offset;
        }
    }

    return false;
}

} // namespace


bool OleRenderEmf( const std::vector<uint8_t>& aEmf, int aMaxWidth, int aMaxHeight, wxImage& aImage,
                   double aTargetAspect )
{
    using boost::endian::load_little_u32;
    using boost::endian::load_little_s32;

    if( aEmf.size() < 108 || aEmf.size() > MAX_EMF_BYTES || load_little_u32( aEmf.data() ) != 1
        || load_little_u32( aEmf.data() + 4 ) < 88
        || load_little_u32( aEmf.data() + 40 ) != 0x464D4520
        || load_little_u32( aEmf.data() + 48 ) != aEmf.size() || !std::isfinite( aTargetAspect ) )
    {
        return false;
    }

    int64_t width = int64_t( load_little_s32( aEmf.data() + 16 ) ) - load_little_s32( aEmf.data() + 8 );
    int64_t height = int64_t( load_little_s32( aEmf.data() + 20 ) ) - load_little_s32( aEmf.data() + 12 );

    if( width <= 0 || height <= 0 || width > std::numeric_limits<int>::max()
        || height > std::numeric_limits<int>::max() )
    {
        return false;
    }

    // Reject truncated records before the library reads their headers.
    size_t offset = 0;

    while( offset < aEmf.size() )
    {
        if( aEmf.size() - offset < 8 )
            return false;

        uint32_t length = load_little_u32( aEmf.data() + offset + 4 );

        if( length < 8 || length % 4 != 0 || length > aEmf.size() - offset )
            return false;

        offset += length;
    }

    VECTOR2I size = OleWmfRenderSize( static_cast<int>( width ), static_cast<int>( height ),
                                     aMaxWidth, aMaxHeight, aTargetAspect );

    if( size.x <= 0 || size.y <= 0 || size_t( size.x ) * size.y > MAX_RENDER_PIXELS )
        return false;

    const FONT_CONFIG fonts = emfFontConfig();

    if( !fonts )
        return false;

    generatorOptions options{};
    options.svgDelimiter = true;
    options.imgWidth = size.x;
    options.imgHeight = size.y;
    options.fontConfig = fonts.get();

    std::vector<char> input( aEmf.begin(), aEmf.end() );
    char* svg = nullptr;
    size_t svgLength = 0;
    int converted = emf2svg( input.data(), input.size(), &svg, &svgLength, &options );
    std::unique_ptr<char, decltype( &free )> svgOwner( svg, free );

    if( !converted || !svg || svgLength == 0 || svgLength > MAX_EMF_BYTES )
        return false;

    // NanoSVG cannot draw embedded rasters; let the caller try its WMF preview.
    if( containsRaster( std::string_view( svg, svgLength ) ) )
        return false;

    std::unique_ptr<NSVGimage, decltype( &nsvgDelete )> drawing(
            nsvgParseWithFontConfig( svg, "px", 96, fonts.get() ), nsvgDelete );

    if( !drawing || !drawing->shapes || !std::isfinite( drawing->width ) || !std::isfinite( drawing->height )
        || drawing->width <= 0 || drawing->height <= 0 || drawing->width > aMaxWidth + 1.0
        || drawing->height > aMaxHeight + 1.0 )
    {
        return false;
    }

    int renderWidth = std::max( 1, KiROUND( drawing->width ) );
    int renderHeight = std::max( 1, KiROUND( drawing->height ) );

    if( size_t( renderWidth ) * renderHeight > MAX_RENDER_PIXELS )
        return false;

    std::unique_ptr<NSVGrasterizer, decltype( &nsvgDeleteRasterizer )> rasterizer(
            nsvgCreateRasterizer(), nsvgDeleteRasterizer );

    if( !rasterizer )
        return false;

    std::vector<unsigned char> pixels( size_t( renderWidth ) * renderHeight * 4 );
    nsvgRasterize( rasterizer.get(), drawing.get(), 0, 0, 1, pixels.data(), renderWidth, renderHeight,
                   renderWidth * 4 );
    wxImage image( renderWidth, renderHeight );

    if( !image.IsOk() )
        return false;

    unsigned char* rgb = image.GetData();

    for( size_t i = 0; i < pixels.size() / 4; ++i )
    {
        unsigned int alpha = pixels[i * 4 + 3];

        for( size_t channel = 0; channel < 3; ++channel )
            rgb[i * 3 + channel] = ( pixels[i * 4 + channel] * alpha + 255 * ( 255 - alpha ) + 127 ) / 255;
    }

    if( renderWidth != size.x || renderHeight != size.y )
        image.Rescale( size.x, size.y, wxIMAGE_QUALITY_HIGH );

    aImage = std::move( image );
    return aImage.IsOk();
}

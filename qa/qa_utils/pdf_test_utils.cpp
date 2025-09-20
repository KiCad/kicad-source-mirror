/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <qa_utils/pdf_test_utils.h>
#include <gal/color4d.h>

#include <wx/filename.h>
#include <wx/filefn.h>
#include <wx/ffile.h>
#include <wx/utils.h>
#include <wx/image.h>
#include <wx/imagpng.h>

#include <zlib.h>

wxString MakeTempPdfPath( const wxString& aPrefix )
{
    wxFileName fn = wxFileName::CreateTempFileName( aPrefix );
    fn.SetExt( "pdf" );
    return fn.GetFullPath();
}

SIMPLE_RENDER_SETTINGS::SIMPLE_RENDER_SETTINGS()
{
    m_background = KIGFX::COLOR4D( 1.0, 1.0, 1.0, 1.0 );
    m_grid = KIGFX::COLOR4D( 0.8, 0.8, 0.8, 1.0 );
    m_cursor = KIGFX::COLOR4D( 0.0, 0.0, 0.0, 1.0 );
}

KIGFX::COLOR4D SIMPLE_RENDER_SETTINGS::GetColor( const KIGFX::VIEW_ITEM*, int ) const
{
    return KIGFX::COLOR4D( 0.0, 0.0, 0.0, 1.0 );
}

TEXT_ATTRIBUTES BuildTextAttributes( int aSizeIu, int aStrokeWidth, bool aBold, bool aItalic )
{
    TEXT_ATTRIBUTES attrs;
    attrs.m_Size = VECTOR2I( aSizeIu, aSizeIu );
    attrs.m_StrokeWidth = aStrokeWidth;
    attrs.m_Multiline = false;
    attrs.m_Italic = aItalic;
    attrs.m_Bold = aBold;
    attrs.m_Halign = GR_TEXT_H_ALIGN_LEFT;
    attrs.m_Valign = GR_TEXT_V_ALIGN_BOTTOM;
    attrs.m_Angle = ANGLE_0;
    attrs.m_Mirrored = false;
    return attrs;
}

std::unique_ptr<KIFONT::STROKE_FONT> LoadStrokeFontUnique()
{
    return std::unique_ptr<KIFONT::STROKE_FONT>( KIFONT::STROKE_FONT::LoadFont( wxEmptyString ) );
}

static void append_decompressed_streams( std::string& aBuffer )
{
    std::string aggregate = aBuffer;
    size_t pos = 0;

    while( true )
    {
        size_t streamPos = aBuffer.find( "stream\n", pos );

        if( streamPos == std::string::npos )
            break;

        size_t endPos = aBuffer.find( "endstream", streamPos );

        if( endPos == std::string::npos )
            break;

        size_t dataStart = streamPos + 7; // skip "stream\n"
        size_t dataLen = ( endPos > dataStart ) ? ( endPos - dataStart ) : 0;

        if( dataLen > 0 )
        {
            const unsigned char* data = reinterpret_cast<const unsigned char*>( aBuffer.data() + dataStart );
            z_stream             zs{};
            zs.next_in = const_cast<Bytef*>( data );
            zs.avail_in = static_cast<uInt>( dataLen );

            if( inflateInit( &zs ) == Z_OK )
            {
                std::string out;
                out.resize( dataLen * 4 + 64 );
                zs.next_out = reinterpret_cast<Bytef*>( out.data() );
                zs.avail_out = static_cast<uInt>( out.size() );
                int ret = inflate( &zs, Z_FINISH );

                if( ret == Z_STREAM_END )
                {
                    out.resize( zs.total_out );
                    aggregate += out;
                }

                inflateEnd( &zs );
            }
        }

        pos = endPos + 9; // skip past "endstream"
    }

    aBuffer.swap( aggregate );
}

bool ReadPdfWithDecompressedStreams( const wxString& aPdfPath, std::string& aOutBuffer )
{
    wxFFile file( aPdfPath, "rb" );

    if( !file.IsOpened() )
        return false;

    wxFileOffset len = file.Length();
    if( len <= 0 )
        return false;

    aOutBuffer.resize( static_cast<size_t>( len ) );
    file.Read( aOutBuffer.data(), len );

    append_decompressed_streams( aOutBuffer );
    return true;
}

int CountOccurrences( const std::string& aHaystack, const std::string& aNeedle )
{
    if( aNeedle.empty() )
        return 0;

    int count = 0;
    size_t pos = 0;

    while( true )
    {
        size_t p = aHaystack.find( aNeedle, pos );
        if( p == std::string::npos )
            break;
        ++count;
        pos = p + 1; // allow overlaps
    }

    return count;
}

bool RasterizePdfCountDark( const wxString& aPdfPath, int aDpi, int aNearWhiteThresh,
                            long& aOutDarkPixels )
{
    aOutDarkPixels = 0;

    wxString rasterBase = wxFileName::CreateTempFileName( wxT( "kicad_pdf_raster" ) );
    wxString cmd = wxString::Format( wxT( "pdftoppm -r %d -singlefile -png \"%s\" \"%s\"" ),
                                     aDpi, aPdfPath, rasterBase );
    int ret = wxExecute( cmd, wxEXEC_SYNC );

    if( ret != 0 )
        return false;

    wxString pngPath = rasterBase + wxT( ".png" );

    if( !wxFileExists( pngPath ) )
        return false;

    if( !wxImage::FindHandler( wxBITMAP_TYPE_PNG ) )
        wxImage::AddHandler( new wxPNGHandler );

    wxImage img( pngPath );
    if( !img.IsOk() )
        return false;

    int w = img.GetWidth();
    int h = img.GetHeight();

    long dark = 0;
    for( int y = 0; y < h; ++y )
    {
        for( int x = 0; x < w; ++x )
        {
            unsigned char r = img.GetRed( x, y );
            unsigned char g = img.GetGreen( x, y );
            unsigned char b = img.GetBlue( x, y );

            if( r < aNearWhiteThresh || g < aNearWhiteThresh || b < aNearWhiteThresh )
                ++dark;
        }
    }

    aOutDarkPixels = dark;

    // cleanup the rasterized file
    wxRemoveFile( pngPath );
    return true;
}

void MaybeRemoveFile( const wxString& aPath, const wxString& aEnvVar )
{
    wxString keepEnv;
    if( !wxGetEnv( aEnvVar, &keepEnv ) || keepEnv.IsEmpty() )
        wxRemoveFile( aPath );
}

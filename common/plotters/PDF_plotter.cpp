/**
 * @file PDF_plotter.cpp
 * @brief KiCad: specialized plotter for PDF files format
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 Lorenzo Marcantonio, l.marcantonio@logossrl.com
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

#include <algorithm>
#include <cstdio> // snprintf
#include <stack>

#include <wx/filename.h>
#include <wx/mstream.h>
#include <wx/zstream.h>
#include <wx/wfstream.h>
#include <wx/datstrm.h>
#include <wx/tokenzr.h>

#include <advanced_config.h>
#include <common.h>               // ResolveUriByEnvVars
#include <eda_text.h>             // for IsGotoPageHref
#include <font/font.h>
#include <core/ignore.h>
#include <macros.h>
#include <trigo.h>
#include <string_utils.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>

#include <plotters/plotters_pslike.h>


std::string PDF_PLOTTER::encodeStringForPlotter( const wxString& aText )
{
    // returns a string compatible with PDF string convention from a unicode string.
    // if the initial text is only ASCII7, return the text between ( and ) for a good readability
    // if the initial text is no ASCII7, return the text between < and >
    // and encoded using 16 bits hexa (4 digits) by wide char (unicode 16)
    std::string result;

    // Is aText only ASCII7 ?
    bool is_ascii7 = true;

    for( size_t ii = 0; ii < aText.Len(); ii++ )
    {
        if( aText[ii] >= 0x7F )
        {
            is_ascii7 = false;
            break;
        }
    }

    if( is_ascii7 )
    {
        result = '(';

        for( unsigned ii = 0; ii < aText.Len(); ii++ )
        {
            unsigned int code = aText[ii];

            // These characters must be escaped
            switch( code )
            {
            case '(':
            case ')':
            case '\\':
                result += '\\';
                KI_FALLTHROUGH;

            default:
                result += code;
                break;
            }
        }

        result += ')';
    }
    else
    {
        result = "<FEFF";

        for( size_t ii = 0; ii < aText.Len(); ii++ )
        {
            unsigned int code = aText[ii];
            result += fmt::format("{:04X}", code);
        }

        result += '>';
    }

    return result;
}


bool PDF_PLOTTER::OpenFile( const wxString& aFullFilename )
{
    m_filename = aFullFilename;

    wxASSERT( !m_outputFile );

    // Open the PDF file in binary mode
    m_outputFile = wxFopen( m_filename, wxT( "wb" ) );

    if( m_outputFile == nullptr )
        return false ;

    return true;
}


void PDF_PLOTTER::SetViewport( const VECTOR2I& aOffset, double aIusPerDecimil,
                               double aScale, bool aMirror )
{
    m_plotMirror    = aMirror;
    m_plotOffset    = aOffset;
    m_plotScale     = aScale;
    m_IUsPerDecimil = aIusPerDecimil;

    // The CTM is set to 1 user unit per decimal
    m_iuPerDeviceUnit = 1.0 / aIusPerDecimil;

    /* The paper size in this engine is handled page by page
       Look in the StartPage function */
}


void PDF_PLOTTER::SetCurrentLineWidth( int aWidth, void* aData )
{
    wxASSERT( m_workFile );

    if( aWidth == DO_NOT_SET_LINE_WIDTH )
        return;
    else if( aWidth == USE_DEFAULT_LINE_WIDTH )
        aWidth = m_renderSettings->GetDefaultPenWidth();

    if( aWidth == 0 )
        aWidth = 1;

    wxASSERT_MSG( aWidth > 0, "Plotter called to set negative pen width" );

    if( aWidth != m_currentPenWidth )
        fmt::println( m_workFile, "{:g} w", userToDeviceSize( aWidth ) );

    m_currentPenWidth = aWidth;
}


void PDF_PLOTTER::emitSetRGBColor( double r, double g, double b, double a )
{
    wxASSERT( m_workFile );

    // PDF treats all colors as opaque, so the best we can do with alpha is generate an
    // appropriate blended color assuming white paper.
    if( a < 1.0 )
    {
        r = ( r * a ) + ( 1 - a );
        g = ( g * a ) + ( 1 - a );
        b = ( b * a ) + ( 1 - a );
    }

    fmt::println( m_workFile, "{:g} {:g} {:g} rg {:g} {:g} {:g} RG", r, g, b, r, g, b );
}


void PDF_PLOTTER::SetDash( int aLineWidth, LINE_STYLE aLineStyle )
{
    wxASSERT( m_workFile );

    switch( aLineStyle )
    {
    case LINE_STYLE::DASH:
        fmt::println( m_workFile, "[{} {}] 0 d",
                      (int) GetDashMarkLenIU( aLineWidth ),
                      (int) GetDashGapLenIU( aLineWidth ) );
        break;

    case LINE_STYLE::DOT:
        fmt::println( m_workFile, "[{} {}] 0 d",
                      (int) GetDotMarkLenIU( aLineWidth ),
                      (int) GetDashGapLenIU( aLineWidth ) );
        break;

    case LINE_STYLE::DASHDOT:
        fmt::println( m_workFile, "[{} {} {} {}] 0 d",
                      (int) GetDashMarkLenIU( aLineWidth ),
                      (int) GetDashGapLenIU( aLineWidth ),
                      (int) GetDotMarkLenIU( aLineWidth ),
                      (int) GetDashGapLenIU( aLineWidth ) );
        break;

    case LINE_STYLE::DASHDOTDOT:
        fmt::println( m_workFile, "[{} {} {} {} {} {}] 0 d",
                      (int) GetDashMarkLenIU( aLineWidth ),
                      (int) GetDashGapLenIU( aLineWidth ),
                      (int) GetDotMarkLenIU( aLineWidth ),
                      (int) GetDashGapLenIU( aLineWidth ),
                      (int) GetDotMarkLenIU( aLineWidth ),
                      (int) GetDashGapLenIU( aLineWidth ) );
        break;

    default:
        fmt::println( m_workFile, "[] 0 d\n" );
    }
}


void PDF_PLOTTER::Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T fill, int width )
{
    wxASSERT( m_workFile );

    if( fill == FILL_T::NO_FILL && width == 0 )
        return;

    SetCurrentLineWidth( width );

    VECTOR2I size = p2 - p1;

    if( size.x == 0 && size.y == 0 )
    {
        // Can't draw zero-sized rectangles
        MoveTo( VECTOR2I( p1.x, p1.y ) );
        FinishTo( VECTOR2I( p1.x, p1.y ) );

        return;
    }

    if( std::min( std::abs( size.x ), std::abs( size.y ) ) < width )
    {
        // Too thick stroked rectangles are buggy, draw as polygon
        std::vector<VECTOR2I> cornerList;

        cornerList.emplace_back( p1.x, p1.y );
        cornerList.emplace_back( p2.x, p1.y );
        cornerList.emplace_back( p2.x, p2.y );
        cornerList.emplace_back( p1.x, p2.y );
        cornerList.emplace_back( p1.x, p1.y );

        PlotPoly( cornerList, fill, width, nullptr );

        return;
    }

    VECTOR2D p1_dev = userToDeviceCoordinates( p1 );
    VECTOR2D p2_dev = userToDeviceCoordinates( p2 );

    char paintOp;

    if( fill == FILL_T::NO_FILL )
        paintOp = 'S';
    else
        paintOp = width > 0 ? 'B' : 'f';

    fmt::println( m_workFile, "{:g} {:g} {:g} {:g} re {}",
                  p1_dev.x,
                  p1_dev.y,
                  p2_dev.x - p1_dev.x,
                  p2_dev.y - p1_dev.y,
                  paintOp );
}


void PDF_PLOTTER::Circle( const VECTOR2I& pos, int diametre, FILL_T aFill, int width )
{
    wxASSERT( m_workFile );

    if( aFill == FILL_T::NO_FILL && width == 0 )
        return;

    SetCurrentLineWidth( width );

    VECTOR2D pos_dev = userToDeviceCoordinates( pos );
    double   radius = userToDeviceSize( diametre / 2.0 );

    // If diameter is less than width, switch to filled mode
    if( aFill == FILL_T::NO_FILL && diametre < GetCurrentLineWidth() )
    {
        aFill = FILL_T::FILLED_SHAPE;
        radius = userToDeviceSize( ( diametre / 2.0 ) + ( width / 2.0 ) );
    }

    /* OK. Here's a trick. PDF doesn't support circles or circular angles, that's
       a fact. You'll have to do with cubic beziers. These *can't* represent
       circular arcs (NURBS can, beziers don't). But there is a widely known
       approximation which is really good
    */

    double magic = radius * 0.551784; // You don't want to know where this come from

    // This is the convex hull for the bezier approximated circle
    fmt::println( m_workFile,
                  "{:g} {:g} m "
                  "{:g} {:g} {:g} {:g} {:g} {:g} c "
                  "{:g} {:g} {:g} {:g} {:g} {:g} c "
                  "{:g} {:g} {:g} {:g} {:g} {:g} c "
                  "{:g} {:g} {:g} {:g} {:g} {:g} c {}",
                  pos_dev.x - radius, pos_dev.y,

                  pos_dev.x - radius, pos_dev.y + magic,
                  pos_dev.x - magic, pos_dev.y + radius,
                  pos_dev.x, pos_dev.y + radius,

                  pos_dev.x + magic, pos_dev.y + radius,
                  pos_dev.x + radius, pos_dev.y + magic,
                  pos_dev.x + radius, pos_dev.y,

                  pos_dev.x + radius, pos_dev.y - magic,
                  pos_dev.x + magic, pos_dev.y - radius,
                  pos_dev.x, pos_dev.y - radius,

                  pos_dev.x - magic, pos_dev.y - radius,
                  pos_dev.x - radius, pos_dev.y - magic,
                  pos_dev.x - radius, pos_dev.y,

                  aFill == FILL_T::NO_FILL ? 's' : 'b' );
}


void PDF_PLOTTER::Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                       const EDA_ANGLE& aAngle, double aRadius, FILL_T aFill, int aWidth )
{
    wxASSERT( m_workFile );

    SetCurrentLineWidth( aWidth );

    if( aRadius <= 0 )
    {
        Circle( aCenter, GetCurrentLineWidth(), FILL_T::FILLED_SHAPE, 0 );
        return;
    }

    /*
     * Arcs are not so easily approximated by beziers (in the general case), so we approximate
     * them in the old way
     */
    EDA_ANGLE       startAngle = -aStartAngle;
    EDA_ANGLE       endAngle = startAngle - aAngle;
    VECTOR2I        start;
    VECTOR2I        end;
    const EDA_ANGLE delta( 5, DEGREES_T );   // increment to draw circles

    if( startAngle > endAngle )
        std::swap( startAngle, endAngle );

    // Usual trig arc plotting routine...
    start.x = KiROUND( aCenter.x + aRadius * ( -startAngle ).Cos() );
    start.y = KiROUND( aCenter.y + aRadius * ( -startAngle ).Sin() );
    VECTOR2D pos_dev = userToDeviceCoordinates( start );
    fmt::print( m_workFile, "{:g} {:g} m ", pos_dev.x, pos_dev.y );

    for( EDA_ANGLE ii = startAngle + delta; ii < endAngle; ii += delta )
    {
        end.x = KiROUND( aCenter.x + aRadius * ( -ii ).Cos() );
        end.y = KiROUND( aCenter.y + aRadius * ( -ii ).Sin() );
        pos_dev = userToDeviceCoordinates( end );
        fmt::print( m_workFile, "{:g} {:g} l ", pos_dev.x, pos_dev.y );
    }

    end.x = KiROUND( aCenter.x + aRadius * ( -endAngle ).Cos() );
    end.y = KiROUND( aCenter.y + aRadius * ( -endAngle ).Sin() );
    pos_dev = userToDeviceCoordinates( end );
    fmt::print( m_workFile, "{:g} {:g} l ", pos_dev.x, pos_dev.y );

    // The arc is drawn... if not filled we stroke it, otherwise we finish
    // closing the pie at the center
    if( aFill == FILL_T::NO_FILL )
    {
        fmt::println( m_workFile, "S" );
    }
    else
    {
        pos_dev = userToDeviceCoordinates( aCenter );
        fmt::println( m_workFile, "{:g} {:g} l b", pos_dev.x, pos_dev.y );
    }
}


void PDF_PLOTTER::PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill, int aWidth,
                            void* aData )
{
    wxASSERT( m_workFile );

    if( aCornerList.size() <= 1 )
        return;

    if( aFill == FILL_T::NO_FILL && aWidth == 0 )
        return;

    SetCurrentLineWidth( aWidth );

    VECTOR2D pos = userToDeviceCoordinates( aCornerList[0] );
    fmt::println( m_workFile, "{:f} {:f} m", pos.x, pos.y );

    for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
    {
        pos = userToDeviceCoordinates( aCornerList[ii] );
        fmt::println( m_workFile, "{:f} {:f} l", pos.x, pos.y );
    }

    // Close path and stroke and/or fill
    if( aFill == FILL_T::NO_FILL )
        fmt::println( m_workFile, "S" );
    else if( aWidth == 0 )
        fmt::println( m_workFile, "f" );
    else
        fmt::println( m_workFile, "b" );
}


void PDF_PLOTTER::PenTo( const VECTOR2I& pos, char plume )
{
    wxASSERT( m_workFile );

    if( plume == 'Z' )
    {
        if( m_penState != 'Z' )
        {
            fmt::println( m_workFile, "S" );
            m_penState     = 'Z';
            m_penLastpos.x = -1;
            m_penLastpos.y = -1;
        }

        return;
    }

    if( m_penState != plume || pos != m_penLastpos )
    {
        VECTOR2D pos_dev = userToDeviceCoordinates( pos );
        fmt::println( m_workFile, "{:f} {:f} {}",
                      pos_dev.x,
                      pos_dev.y,
                      plume == 'D' ? 'l' : 'm' );
    }

    m_penState   = plume;
    m_penLastpos = pos;
}


void PDF_PLOTTER::PlotImage( const wxImage& aImage, const VECTOR2I& aPos, double aScaleFactor )
{
    wxASSERT( m_workFile );
    VECTOR2I pix_size( aImage.GetWidth(), aImage.GetHeight() );

    // Requested size (in IUs)
    VECTOR2D drawsize( aScaleFactor * pix_size.x, aScaleFactor * pix_size.y );

    // calculate the bitmap start position
    VECTOR2I start( aPos.x - drawsize.x / 2, aPos.y + drawsize.y / 2 );
    VECTOR2D dev_start = userToDeviceCoordinates( start );

    // Deduplicate images
    auto findHandleForImage = [&]( const wxImage& aCurrImage ) -> int
    {
        for( const auto& [imgHandle, image] : m_imageHandles )
        {
            if( image.IsSameAs( aCurrImage ) )
                return imgHandle;

            if( image.GetWidth() != aCurrImage.GetWidth() )
                continue;

            if( image.GetHeight() != aCurrImage.GetHeight() )
                continue;

            if( image.GetType() != aCurrImage.GetType() )
                continue;

            if( image.HasAlpha() != aCurrImage.HasAlpha() )
                continue;

            if( image.HasMask() != aCurrImage.HasMask()
              || image.GetMaskRed() != aCurrImage.GetMaskRed()
              || image.GetMaskGreen() != aCurrImage.GetMaskGreen()
              || image.GetMaskBlue() != aCurrImage.GetMaskBlue() )
                continue;

            int pixCount = image.GetWidth() * image.GetHeight();

            if( memcmp( image.GetData(), aCurrImage.GetData(), pixCount * 3 ) != 0 )
                continue;

            if( image.HasAlpha() && memcmp( image.GetAlpha(), aCurrImage.GetAlpha(), pixCount ) != 0 )
                continue;

            return imgHandle;
        }

        return -1;
    };

    int imgHandle = findHandleForImage( aImage );

    if( imgHandle == -1 )
    {
        imgHandle = allocPdfObject();
        m_imageHandles.emplace( imgHandle, aImage );
    }

    /* PDF has an uhm... simplified coordinate system handling. There is
       *one* operator to do everything (the PS concat equivalent). At least
       they kept the matrix stack to save restore environments. Also images
       are always emitted at the origin with a size of 1x1 user units.
       What we need to do is:
       1) save the CTM end establish the new one
       2) plot the image
       3) restore the CTM
       4) profit
     */
    fmt::println( m_workFile, "q {:g} 0 0 {:g} {:g} {:g} cm", // Step 1
                  userToDeviceSize( drawsize.x ),
                  userToDeviceSize( drawsize.y ),
                  dev_start.x,
                  dev_start.y );

    fmt::println( m_workFile, "/Im{} Do", imgHandle );
    fmt::println( m_workFile, "Q" );
}


int PDF_PLOTTER::allocPdfObject()
{
    m_xrefTable.push_back( 0 );
    return m_xrefTable.size() - 1;
}


int PDF_PLOTTER::startPdfObject( int aHandle )
{
    wxASSERT( m_outputFile );
    wxASSERT( !m_workFile );

    if( aHandle < 0 )
        aHandle = allocPdfObject();

    m_xrefTable[aHandle] = ftell( m_outputFile );
    fmt::println( m_outputFile, "{} 0 obj", aHandle );
    return aHandle;
}


void PDF_PLOTTER::closePdfObject()
{
    wxASSERT( m_outputFile );
    wxASSERT( !m_workFile );
    fmt::println( m_outputFile, "endobj" );
}


int PDF_PLOTTER::startPdfStream( int aHandle )
{
    wxASSERT( m_outputFile );
    wxASSERT( !m_workFile );
    int handle = startPdfObject( aHandle );

    // This is guaranteed to be handle+1 but needs to be allocated since
    // you could allocate more object during stream preparation
    m_streamLengthHandle = allocPdfObject();

    if( ADVANCED_CFG::GetCfg().m_DebugPDFWriter )
    {
        fmt::println( m_outputFile,
                      "<< /Length {} 0 R >>\n"
                      "stream",
                      handle + 1 );
    }
    else
    {
        fmt::println( m_outputFile,
                      "<< /Length {} 0 R /Filter /FlateDecode >>\n"
                      "stream",
                      handle + 1 );
    }

    // Open a temporary file to accumulate the stream
    m_workFilename = wxFileName::CreateTempFileName( "" );
    m_workFile = wxFopen( m_workFilename, wxT( "w+b" ) );
    wxASSERT( m_workFile );
    return handle;
}


void PDF_PLOTTER::closePdfStream()
{
    wxASSERT( m_workFile );

    long stream_len = ftell( m_workFile );

    if( stream_len < 0 )
    {
        wxASSERT( false );
        return;
    }

    // Rewind the file, read in the page stream and DEFLATE it
    fseek( m_workFile, 0, SEEK_SET );
    unsigned char *inbuf = new unsigned char[stream_len];

    int rc = fread( inbuf, 1, stream_len, m_workFile );
    wxASSERT( rc == stream_len );
    ignore_unused( rc );

    // We are done with the temporary file, junk it
    fclose( m_workFile );
    m_workFile = nullptr;
    ::wxRemoveFile( m_workFilename );

    unsigned out_count;

    if( ADVANCED_CFG::GetCfg().m_DebugPDFWriter )
    {
        out_count = stream_len;
        fwrite( inbuf, out_count, 1, m_outputFile );
    }
    else
    {
        // NULL means memos owns the memory, but provide a hint on optimum size needed.
        wxMemoryOutputStream    memos( nullptr, std::max( 2000l, stream_len ) ) ;

        {
            /* Somewhat standard parameters to compress in DEFLATE. The PDF spec is
             * misleading, it says it wants a DEFLATE stream but it really want a ZLIB
             * stream! (a DEFLATE stream would be generated with -15 instead of 15)
             * rc = deflateInit2( &zstrm, Z_BEST_COMPRESSION, Z_DEFLATED, 15,
             *                    8, Z_DEFAULT_STRATEGY );
             */

            wxZlibOutputStream      zos( memos, wxZ_BEST_COMPRESSION, wxZLIB_ZLIB );

            zos.Write( inbuf, stream_len );
        }   // flush the zip stream using zos destructor

        wxStreamBuffer* sb = memos.GetOutputStreamBuffer();

        out_count = sb->Tell();
        fwrite( sb->GetBufferStart(), 1, out_count, m_outputFile );
    }

    delete[] inbuf;
    fmt::print( m_outputFile, "\nendstream\n" );
    closePdfObject();

    // Writing the deferred length as an indirect object
    startPdfObject( m_streamLengthHandle );
    fmt::println( m_outputFile, "{}", out_count );
    closePdfObject();
}


void PDF_PLOTTER::StartPage( const wxString& aPageNumber, const wxString& aPageName,
                             const wxString& aParentPageNumber, const wxString& aParentPageName )
{
    wxASSERT( m_outputFile );
    wxASSERT( !m_workFile );

    m_pageNumbers.push_back( aPageNumber );
    m_pageName = aPageName.IsEmpty() ? wxString::Format( _( "Page %s" ),
                                                         aPageNumber )
                                     : wxString::Format( _( "%s (Page %s)" ),
                                                         aPageName,
                                                         aPageNumber );
    m_parentPageName = aParentPageName.IsEmpty() ? wxString::Format( _( "Page %s" ),
                                                                     aParentPageNumber )
                                                 : wxString::Format( _( "%s (Page %s)" ),
                                                                     aParentPageName,
                                                                     aParentPageNumber );

    // Compute the paper size in IUs
    m_paperSize = m_pageInfo.GetSizeMils();
    m_paperSize.x *= 10.0 / m_iuPerDeviceUnit;
    m_paperSize.y *= 10.0 / m_iuPerDeviceUnit;

    // Set m_currentPenWidth to a unused value to ensure the pen width
    // will be initialized to a the right value in pdf file by the first item to plot
    m_currentPenWidth = 0;

    if( !m_3dExportMode )
    {
        // Open the content stream; the page object will go later
        m_pageStreamHandle = startPdfStream();

        /* Now, until ClosePage *everything* must be wrote in workFile, to be
           compressed later in closePdfStream */

        // Default graphic settings (coordinate system, default color and line style)
        fmt::println( m_workFile,
                      "{:g} 0 0 {:g} 0 0 cm 1 J 1 j 0 0 0 rg 0 0 0 RG {:g} w",
                      0.0072 * plotScaleAdjX, 0.0072 * plotScaleAdjY,
                      userToDeviceSize( m_renderSettings->GetDefaultPenWidth() ) );
    }
}


void WriteImageStream( const wxImage& aImage, wxDataOutputStream& aOut, wxColor bg, bool colorMode )
{
    int w = aImage.GetWidth();
    int h = aImage.GetHeight();

    for( int y = 0; y < h; y++ )
    {
        for( int x = 0; x < w; x++ )
        {
            unsigned char r = aImage.GetRed( x, y ) & 0xFF;
            unsigned char g = aImage.GetGreen( x, y ) & 0xFF;
            unsigned char b = aImage.GetBlue( x, y ) & 0xFF;

            if( aImage.HasMask() )
            {
                if( r == aImage.GetMaskRed() && g == aImage.GetMaskGreen()
                    && b == aImage.GetMaskBlue() )
                {
                    r = bg.Red();
                    g = bg.Green();
                    b = bg.Blue();
                }
            }

            if( colorMode )
            {
                aOut.Write8( r );
                aOut.Write8( g );
                aOut.Write8( b );
            }
            else
            {
                // Greyscale conversion (CIE 1931)
                unsigned char grey = KiROUND( r * 0.2126 + g * 0.7152 + b * 0.0722 );

                aOut.Write8( grey );
            }
        }
    }
}


void WriteImageSMaskStream( const wxImage& aImage, wxDataOutputStream& aOut )
{
    int w = aImage.GetWidth();
    int h = aImage.GetHeight();

    if( aImage.HasMask() )
    {
        for( int y = 0; y < h; y++ )
        {
            for( int x = 0; x < w; x++ )
            {
                unsigned char a = 255;
                unsigned char r = aImage.GetRed( x, y );
                unsigned char g = aImage.GetGreen( x, y );
                unsigned char b = aImage.GetBlue( x, y );

                if( r == aImage.GetMaskRed() && g == aImage.GetMaskGreen()
                    && b == aImage.GetMaskBlue() )
                {
                    a = 0;
                }

                aOut.Write8( a );
            }
        }
    }
    else if( aImage.HasAlpha() )
    {
        int size = w * h;
        aOut.Write8( aImage.GetAlpha(), size );
    }
}


void PDF_PLOTTER::ClosePage()
{
    // non 3d exports need this
    if( m_pageStreamHandle != -1 )
    {
        wxASSERT( m_workFile );

        // Close the page stream (and compress it)
        closePdfStream();
    }

    // Page size is in 1/72 of inch (default user space units).  Works like the bbox in postscript
    // but there is no need for swapping the sizes, since PDF doesn't require a portrait page.
    // We use the MediaBox but PDF has lots of other less-used boxes that could be used.
    const double PTsPERMIL = 0.072;
    VECTOR2D     psPaperSize = VECTOR2D( m_pageInfo.GetSizeMils() ) * PTsPERMIL;

    auto iuToPdfUserSpace =
            [&]( const VECTOR2I& aCoord ) -> VECTOR2D
            {
                VECTOR2D pos = VECTOR2D( aCoord ) * PTsPERMIL / ( m_IUsPerDecimil * 10 );

                // PDF y=0 is at bottom of page, invert coordinate
                VECTOR2D retval( pos.x, psPaperSize.y - pos.y );

                // The pdf plot can be mirrored (from left to right). So mirror the
                // x coordinate if m_plotMirror is set
                if( m_plotMirror )
                {
                    if( m_mirrorIsHorizontal )
                        retval.x = ( psPaperSize.x - pos.x );
                    else
                        retval.y = pos.y;
                }

                return retval;
            };

    // Handle annotations (at the moment only "link" type objects)
    std::vector<int> annotHandles;

    // Allocate all hyperlink objects for the page and calculate their position in user space
    // coordinates
    for( const std::pair<BOX2I, wxString>& linkPair : m_hyperlinksInPage )
    {
        const BOX2I&    box = linkPair.first;
        const wxString& url = linkPair.second;

        VECTOR2D bottomLeft = iuToPdfUserSpace( box.GetPosition() );
        VECTOR2D topRight = iuToPdfUserSpace( box.GetEnd() );

        BOX2D userSpaceBox;
        userSpaceBox.SetOrigin( bottomLeft );
        userSpaceBox.SetEnd( topRight );

        annotHandles.push_back( allocPdfObject() );

        m_hyperlinkHandles.insert( { annotHandles.back(), { userSpaceBox, url } } );
    }

    for( const std::pair<BOX2I, std::vector<wxString>>& menuPair : m_hyperlinkMenusInPage )
    {
        const BOX2I&                 box = menuPair.first;
        const std::vector<wxString>& urls = menuPair.second;

        VECTOR2D bottomLeft = iuToPdfUserSpace( box.GetPosition() );
        VECTOR2D topRight = iuToPdfUserSpace( box.GetEnd() );

        BOX2D userSpaceBox;
        userSpaceBox.SetOrigin( bottomLeft );
        userSpaceBox.SetEnd( topRight );

        annotHandles.push_back( allocPdfObject() );

        m_hyperlinkMenuHandles.insert( { annotHandles.back(), { userSpaceBox, urls } } );
    }

    int annot3DHandle = -1;

    if( m_3dExportMode )
    {
        annot3DHandle = allocPdfObject();
        annotHandles.push_back( annot3DHandle );
    }


    int hyperLinkArrayHandle = -1;

    // If we have added any annotation links, create an array containing all the objects
    if( annotHandles.size() > 0 )
    {
        hyperLinkArrayHandle = startPdfObject();
        bool isFirst = true;

        fmt::print( m_outputFile, "[" );

        for( int handle : annotHandles )
        {
            if( isFirst )
                isFirst = false;
            else
                fmt::print( m_outputFile, " " );

            fmt::print( m_outputFile, "{} 0 R", handle );
        }

        fmt::println( m_outputFile, "]" );
        closePdfObject();
    }

    // Emit the page object and put it in the page list for later
    int pageHandle = startPdfObject();
    m_pageHandles.push_back( pageHandle );

    fmt::print( m_outputFile,
                "<<\n"
                "/Type /Page\n"
                "/Parent {} 0 R\n"
                "/Resources <<\n"
                "    /ProcSet [/PDF /Text /ImageC /ImageB]\n"
                "    /Font {} 0 R\n"
                "    /XObject {} 0 R >>\n"
                "/MediaBox [0 0 {:g} {:g}]\n"
                "/Contents {} 0 R\n",
                m_pageTreeHandle, m_fontResDictHandle, m_imgResDictHandle, psPaperSize.x, psPaperSize.y,
                m_pageStreamHandle );

    if( annotHandles.size() > 0 )
        fmt::print( m_outputFile, "/Annots {} 0 R", hyperLinkArrayHandle );

    fmt::print( m_outputFile, ">>\n" );

    closePdfObject();

    if( m_3dExportMode )
    {
        startPdfObject( annot3DHandle );
        fmt::print( m_outputFile,
                    "<<\n"
                    "/Type /Annot\n"
                    "/Subtype /3D\n"
                    "/Rect [0 0 {:g} {:g}]\n"
                    "/NM (3D Annotation)\n"
                    "/3DD {} 0 R\n"
                    "/3DV 0\n"
                    "/3DA<</A/PO/D/PC/TB true/NP true>>\n"
                    "/3DI true\n"
                    "/P {} 0 R\n"
                    ">>\n",
                    psPaperSize.x, psPaperSize.y, m_3dModelHandle, pageHandle );

        closePdfObject();
    }

    // Mark the page stream as idle
    m_pageStreamHandle = -1;

    int                        actionHandle = emitGoToAction( pageHandle );
    PDF_PLOTTER::OUTLINE_NODE* parent_node = m_outlineRoot.get();

    if( !m_parentPageName.IsEmpty() )
    {
        // Search for the parent node iteratively through the entire tree
        std::stack<OUTLINE_NODE*> nodes;
        nodes.push( m_outlineRoot.get() );

        while( !nodes.empty() )
        {
            OUTLINE_NODE* node = nodes.top();
            nodes.pop();

            // Check if this node matches
            if( node->title == m_parentPageName )
            {
                parent_node = node;
                break;
            }

            // Add all children to the stack
            for( OUTLINE_NODE* child : node->children )
                nodes.push( child );
        }
    }

    OUTLINE_NODE* pageOutlineNode = addOutlineNode( parent_node, actionHandle, m_pageName );

    // let's reorg the symbol bookmarks under a page handle
    // let's reorg the symbol bookmarks under a page handle
    for( const auto& [groupName, groupVector] : m_bookmarksInPage )
    {
        OUTLINE_NODE* groupOutlineNode = addOutlineNode( pageOutlineNode, actionHandle, groupName );

        for( const std::pair<BOX2I, wxString>& bookmarkPair : groupVector )
        {
            const BOX2I&    box = bookmarkPair.first;
            const wxString& ref = bookmarkPair.second;

            VECTOR2I bottomLeft = iuToPdfUserSpace( box.GetPosition() );
            VECTOR2I topRight = iuToPdfUserSpace( box.GetEnd() );

            actionHandle = emitGoToAction( pageHandle, bottomLeft, topRight );

            addOutlineNode( groupOutlineNode, actionHandle, ref );
        }

        std::sort( groupOutlineNode->children.begin(), groupOutlineNode->children.end(),
                   []( const OUTLINE_NODE* a, const OUTLINE_NODE* b ) -> bool
                   {
                       return a->title < b->title;
                   } );
    }

    // Clean up
    m_hyperlinksInPage.clear();
    m_hyperlinkMenusInPage.clear();
    m_bookmarksInPage.clear();
}


bool PDF_PLOTTER::StartPlot( const wxString& aPageNumber )
{
    return StartPlot( aPageNumber, wxEmptyString );
}


bool PDF_PLOTTER::StartPlot( const wxString& aPageNumber, const wxString& aPageName )
{
    wxASSERT( m_outputFile );

    // First things first: the customary null object
    m_xrefTable.clear();
    m_xrefTable.push_back( 0 );
    m_hyperlinksInPage.clear();
    m_hyperlinkMenusInPage.clear();
    m_hyperlinkHandles.clear();
    m_hyperlinkMenuHandles.clear();
    m_bookmarksInPage.clear();
    m_totalOutlineNodes = 0;

    m_outlineRoot = std::make_unique<OUTLINE_NODE>();

    /* The header (that's easy!). The second line is binary junk required
       to make the file binary from the beginning (the important thing is
       that they must have the bit 7 set) */
    fmt::print( m_outputFile, "%PDF-1.5\n%\200\201\202\203\n" );

    /* Allocate an entry for the page tree root, it will go in every page parent entry */
    m_pageTreeHandle = allocPdfObject();

    /* In the same way, the font resource dictionary is used by every page
       (it *could* be inherited via the Pages tree */
    m_fontResDictHandle = allocPdfObject();

    m_imgResDictHandle = allocPdfObject();

    m_jsNamesHandle = allocPdfObject();

    /* Now, the PDF is read from the end, (more or less)... so we start
       with the page stream for page 1. Other more important stuff is written
       at the end */
    StartPage( aPageNumber, aPageName );
    return true;
}


int PDF_PLOTTER::emitGoToAction( int aPageHandle, const VECTOR2I& aBottomLeft,
                                 const VECTOR2I& aTopRight )
{
    int actionHandle = allocPdfObject();
    startPdfObject( actionHandle );

    fmt::print( m_outputFile,
                "<</S /GoTo /D [{} 0 R /FitR {} {} {} {}]\n"
                ">>\n",
                aPageHandle,
                aBottomLeft.x,
                aBottomLeft.y,
                aTopRight.x,
                aTopRight.y );

    closePdfObject();

    return actionHandle;
}


int PDF_PLOTTER::emitGoToAction( int aPageHandle )
{
    int actionHandle = allocPdfObject();
    startPdfObject( actionHandle );

    fmt::println( m_outputFile,
                  "<</S /GoTo /D [{} 0 R /Fit]\n"
                  ">>",
                  aPageHandle );

    closePdfObject();

    return actionHandle;
}


void PDF_PLOTTER::emitOutlineNode( OUTLINE_NODE* node, int parentHandle, int nextNode,
                                   int prevNode )
{
    int nodeHandle = node->entryHandle;
    int prevHandle = -1;
    int nextHandle = -1;

    for( std::vector<OUTLINE_NODE*>::iterator it = node->children.begin();
         it != node->children.end(); it++ )
    {
        if( it >= node->children.end() - 1 )
        {
            nextHandle = -1;
        }
        else
        {
            nextHandle = ( *( it + 1 ) )->entryHandle;
        }

        emitOutlineNode( *it, nodeHandle, nextHandle, prevHandle );

        prevHandle = ( *it )->entryHandle;
    }

    // -1 for parentHandle is the outline root itself which is handed elsewhere.
    if( parentHandle != -1 )
    {
        startPdfObject( nodeHandle );

        fmt::print( m_outputFile,
                    "<<\n"
                    "/Title {}\n"
                    "/Parent {} 0 R\n",
                    encodeStringForPlotter(node->title ),
                    parentHandle);

        if( nextNode > 0 )
        {
            fmt::println( m_outputFile, "/Next {} 0 R", nextNode );
        }

        if( prevNode > 0 )
        {
            fmt::println( m_outputFile, "/Prev {} 0 R", prevNode );
        }

        if( node->children.size() > 0 )
        {
            int32_t count = -1 * static_cast<int32_t>( node->children.size() );
            fmt::println( m_outputFile, "/Count {}", count );
            fmt::println( m_outputFile, "/First {} 0 R", node->children.front()->entryHandle );
            fmt::println( m_outputFile, "/Last {} 0 R", node->children.back()->entryHandle );
        }

        if( node->actionHandle != -1 )
        {
            fmt::println( m_outputFile, "/A {} 0 R", node->actionHandle );
        }

        fmt::println( m_outputFile, ">>" );
        closePdfObject();
    }
}


PDF_PLOTTER::OUTLINE_NODE* PDF_PLOTTER::addOutlineNode( OUTLINE_NODE* aParent, int aActionHandle,
                                                        const wxString& aTitle )
{
    OUTLINE_NODE *node = aParent->AddChild( aActionHandle, aTitle, allocPdfObject() );
    m_totalOutlineNodes++;

    return node;
}


int PDF_PLOTTER::emitOutline()
{
    if( m_outlineRoot->children.size() > 0 )
    {
        // declare the outline object
        m_outlineRoot->entryHandle = allocPdfObject();

        emitOutlineNode( m_outlineRoot.get(), -1, -1, -1 );

        startPdfObject( m_outlineRoot->entryHandle );

        fmt::print( m_outputFile,
                    "<< /Type /Outlines\n"
                    "   /Count {}\n"
                    "   /First {} 0 R\n"
                    "   /Last {} 0 R\n"
                    ">>\n",
                    m_totalOutlineNodes,
                    m_outlineRoot->children.front()->entryHandle,
                    m_outlineRoot->children.back()->entryHandle
            );

        closePdfObject();

        return m_outlineRoot->entryHandle;
    }

    return -1;
}

void PDF_PLOTTER::endPlotEmitResources()
{
    /* We need to declare the resources we're using (fonts in particular)
       The useful standard one is the Helvetica family. Adding external fonts
       is *very* involved! */
    struct {
        const char *psname;
        const char *rsname;
        int font_handle;
    } fontdefs[4] = {
        { "/Helvetica",             "/KicadFont",   0 },
        { "/Helvetica-Oblique",     "/KicadFontI",  0 },
        { "/Helvetica-Bold",        "/KicadFontB",  0 },
        { "/Helvetica-BoldOblique", "/KicadFontBI", 0 }
    };

    /* Declare the font resources. Since they're builtin fonts, no descriptors (yay!)
       We'll need metrics anyway to do any alignment (these are in the shared with
       the postscript engine) */
    for( int i = 0; i < 4; i++ )
    {
        fontdefs[i].font_handle = startPdfObject();
        fmt::println( m_outputFile,
                      "<< /BaseFont {}\n"
                      "   /Type /Font\n"
                      "   /Subtype /Type1\n"
                      "   /Encoding /WinAnsiEncoding\n"
                      ">>",
                      fontdefs[i].psname );
        closePdfObject();
    }

    // Named font dictionary (was allocated, now we emit it)
    startPdfObject( m_fontResDictHandle );
    fmt::println( m_outputFile, "<<" );

    for( int i = 0; i < 4; i++ )
    {
        fmt::println( m_outputFile, "    {} {} 0 R",
                 fontdefs[i].rsname, fontdefs[i].font_handle );
    }

    fmt::println( m_outputFile, ">>" );
    closePdfObject();

    // Named image dictionary (was allocated, now we emit it)
    startPdfObject( m_imgResDictHandle );
    fmt::println( m_outputFile, "<<\n" );

    for( const auto& [imgHandle, image] : m_imageHandles )
    {
        fmt::print( m_outputFile, "    /Im{} {} 0 R\n", imgHandle, imgHandle );
    }

    fmt::println( m_outputFile, ">>" );
    closePdfObject();

    // Emit images with optional SMask for transparency
    for( const auto& [imgHandle, image] : m_imageHandles )
    {
        // Init wxFFile so wxFFileOutputStream won't close file in dtor.
        wxFFile outputFFile( m_outputFile );

        // Image
        startPdfObject( imgHandle );
        int imgLenHandle = allocPdfObject();
        int smaskHandle = ( image.HasAlpha() || image.HasMask() ) ? allocPdfObject() : -1;

        fmt::print( m_outputFile,
                    "<<\n"
                    "/Type /XObject\n"
                    "/Subtype /Image\n"
                    "/BitsPerComponent 8\n"
                    "/ColorSpace {}\n"
                    "/Width {}\n"
                    "/Height {}\n"
                    "/Filter /FlateDecode\n"
                    "/Length {} 0 R\n", // Length is deferred
                    m_colorMode ? "/DeviceRGB" : "/DeviceGray",
                    image.GetWidth(),
                    image.GetHeight(),
                    imgLenHandle );

        if( smaskHandle != -1 )
            fmt::println( m_outputFile, "/SMask {} 0 R", smaskHandle );

        fmt::println( m_outputFile, ">>" );
        fmt::println( m_outputFile, "stream" );

        long imgStreamStart = ftell( m_outputFile );

        {
            wxFFileOutputStream ffos( outputFFile );
            wxZlibOutputStream  zos( ffos, wxZ_BEST_COMPRESSION, wxZLIB_ZLIB );
            wxDataOutputStream  dos( zos );

            WriteImageStream( image, dos, m_renderSettings->GetBackgroundColor().ToColour(),
                              m_colorMode );
        }

        long imgStreamSize = ftell( m_outputFile ) - imgStreamStart;

        fmt::print( m_outputFile, "\nendstream\n" );
        closePdfObject();

        startPdfObject( imgLenHandle );
        fmt::println( m_outputFile, "{}", imgStreamSize );
        closePdfObject();

        if( smaskHandle != -1 )
        {
            // SMask
            startPdfObject( smaskHandle );
            int smaskLenHandle = allocPdfObject();

            fmt::print( m_outputFile,
                        "<<\n"
                        "/Type /XObject\n"
                        "/Subtype /Image\n"
                        "/BitsPerComponent 8\n"
                        "/ColorSpace /DeviceGray\n"
                        "/Width {}\n"
                        "/Height {}\n"
                        "/Length {} 0 R\n"
                        "/Filter /FlateDecode\n"
                        ">>\n", // Length is deferred
                        image.GetWidth(),
                        image.GetHeight(),
                        smaskLenHandle );

            fmt::println( m_outputFile, "stream" );

            long smaskStreamStart = ftell( m_outputFile );

            {
                wxFFileOutputStream ffos( outputFFile );
                wxZlibOutputStream  zos( ffos, wxZ_BEST_COMPRESSION, wxZLIB_ZLIB );
                wxDataOutputStream  dos( zos );

                WriteImageSMaskStream( image, dos );
            }

            long smaskStreamSize = ftell( m_outputFile ) - smaskStreamStart;

            fmt::print( m_outputFile, "\nendstream\n" );
            closePdfObject();

            startPdfObject( smaskLenHandle );
            fmt::println( m_outputFile, "{}", (unsigned) smaskStreamSize );
            closePdfObject();
        }

        outputFFile.Detach(); // Don't close it
    }

    for( const auto& [ linkHandle, linkPair ] : m_hyperlinkHandles )
    {
        BOX2D    box = linkPair.first;
        wxString url = linkPair.second;

        startPdfObject( linkHandle );

        fmt::print( m_outputFile,
                    "<<\n"
                    "/Type /Annot\n"
                    "/Subtype /Link\n"
                    "/Rect [{:g} {:g} {:g} {:g}]\n"
                    "/Border [16 16 0]\n",
                    box.GetLeft(),
                    box.GetBottom(),
                    box.GetRight(),
                    box.GetTop() );

        wxString pageNumber;
        bool     pageFound = false;

        if( EDA_TEXT::IsGotoPageHref( url, &pageNumber ) )
        {
            for( size_t ii = 0; ii < m_pageNumbers.size(); ++ii )
            {
                if( m_pageNumbers[ii] == pageNumber )
                {
                    fmt::print( m_outputFile,
                                "/Dest [{} 0 R /FitB]\n"
                                ">>\n",
                                m_pageHandles[ii] );

                    pageFound = true;
                    break;
                }
            }

            if( !pageFound )
            {
                // destination page is not being plotted, assign the NOP action to the link
                fmt::print( m_outputFile, "/A << /Type /Action /S /NOP >>\n"
                                          ">>\n" );
            }
        }
        else
        {
            if( m_project )
                url = ResolveUriByEnvVars( url, m_project );

            fmt::print( m_outputFile,
                        "/A << /Type /Action /S /URI /URI {} >>\n"
                        ">>\n",
                        encodeStringForPlotter( url ) );
        }

        closePdfObject();
    }

    for( const auto& [ menuHandle, menuPair ] : m_hyperlinkMenuHandles )
    {
        const BOX2D&                 box = menuPair.first;
        const std::vector<wxString>& urls = menuPair.second;
        wxString                     js = wxT( "ShM([\n" );

        for( const wxString& url : urls )
        {
            if( url.StartsWith( "!" ) )
            {
                wxString property = url.AfterFirst( '!' );

                if( property.Find( "http:" ) >= 0 )
                {
                    wxString href = property.substr( property.Find( "http:" ) );

                    if( m_project )
                        href = ResolveUriByEnvVars( href, m_project );

                    js += wxString::Format( wxT( "[\"%s\", \"%s\"],\n" ),
                                            EscapeString( href, CTX_JS_STR ),
                                            EscapeString( href, CTX_JS_STR ) );
                }
                else if( property.Find( "https:" ) >= 0 )
                {
                    wxString href = property.substr( property.Find( "https:" ) );

                    if( m_project )
                        href = ResolveUriByEnvVars( href, m_project );

                    js += wxString::Format( wxT( "[\"%s\", \"%s\"],\n" ),
                                            EscapeString( href, CTX_JS_STR ),
                                            EscapeString( href, CTX_JS_STR ) );
                }
                else if( property.Find( "file:" ) >= 0 )
                {
                    wxString href = property.substr( property.Find( "file:" ) );

                    if( m_project )
                        href = ResolveUriByEnvVars( href, m_project );

                    href = NormalizeFileUri( href );

                    js += wxString::Format( wxT( "[\"%s\", \"%s\"],\n" ),
                                            EscapeString( href, CTX_JS_STR ),
                                            EscapeString( href, CTX_JS_STR ) );
                }
                else
                {
                    js += wxString::Format( wxT( "[\"%s\"],\n" ),
                                            EscapeString( property, CTX_JS_STR ) );
                }
            }
            else if( url.StartsWith( "#" ) )
            {
                wxString pageNumber = url.AfterFirst( '#' );

                for( size_t ii = 0; ii < m_pageNumbers.size(); ++ii )
                {
                    if( m_pageNumbers[ii] == pageNumber )
                    {
                        wxString menuText = wxString::Format( _( "Show Page %s" ), pageNumber );

                        js += wxString::Format( wxT( "[\"%s\", \"#%d\"],\n" ),
                                                EscapeString( menuText, CTX_JS_STR ),
                                                static_cast<int>( ii ) );
                        break;
                    }
                }
            }
            else if( url.StartsWith( "http:" ) || url.StartsWith( "https:" )
                   || url.StartsWith( "file:" ) )
            {
                wxString href = url;

                if( m_project )
                    href = ResolveUriByEnvVars( url, m_project );

                if( url.StartsWith( "file:" ) )
                    href = NormalizeFileUri( href );

                wxString menuText = wxString::Format( _( "Open %s" ), href );

                js += wxString::Format( wxT( "[\"%s\", \"%s\"],\n" ),
                                        EscapeString( href, CTX_JS_STR ),
                                        EscapeString( href, CTX_JS_STR ) );
            }
        }

        js += wxT( "]);" );

        startPdfObject( menuHandle );

        fmt::print( m_outputFile,
                    "<<\n"
                    "/Type /Annot\n"
                    "/Subtype /Link\n"
                    "/Rect [{:g} {:g} {:g} {:g}]\n"
                    "/Border [16 16 0]\n",
                    box.GetLeft(), box.GetBottom(), box.GetRight(), box.GetTop() );

        fmt::print( m_outputFile,
                 "/A << /Type /Action /S /JavaScript /JS {} >>\n"
                 ">>\n",
                 encodeStringForPlotter( js ) );

        closePdfObject();
    }

    {
        startPdfObject( m_jsNamesHandle );

        wxString js = R"JS(
function ShM(aEntries) {
    var aParams = [];
    for (var i in aEntries) {
        aParams.push({
            cName: aEntries[i][0],
            cReturn: aEntries[i][1]
        })
    }

    var cChoice = app.popUpMenuEx.apply(app, aParams);
    if (cChoice != null && cChoice.substring(0, 1) == '#') this.pageNum = parseInt(cChoice.slice(1));
    else if (cChoice != null && cChoice.substring(0, 4) == 'http') app.launchURL(cChoice);
    else if (cChoice != null && cChoice.substring(0, 4) == 'file') app.openDoc(cChoice.substring(7));
}
)JS";

        fmt::print( m_outputFile,
                    "<< /JavaScript\n"
                    " << /Names\n"
                    "    [ (JSInit) << /Type /Action /S /JavaScript /JS {} >> ]\n"
                    " >>\n"
                    ">>\n",
                    encodeStringForPlotter( js ) );

        closePdfObject();
    }
}

bool PDF_PLOTTER::EndPlot()
{
    // We can end up here if there was nothing to plot
    if( !m_outputFile )
        return false;

    // Close the current page (often the only one)
    ClosePage();

    if( !m_3dExportMode )
    {
        endPlotEmitResources();
    }

    /* The page tree: it's a B-tree but luckily we only have few pages!
       So we use just an array... The handle was allocated at the beginning,
       now we instantiate the corresponding object */
    startPdfObject( m_pageTreeHandle );
    fmt::print( m_outputFile,
                "<<\n"
                "/Type /Pages\n"
                "/Kids [\n" );

    for( unsigned i = 0; i < m_pageHandles.size(); i++ )
        fmt::println( m_outputFile, "{} 0 R", m_pageHandles[i] );

    fmt::print( m_outputFile,
                "]\n"
                "/Count {}\n"
                ">>\n", m_pageHandles.size() );
    closePdfObject();

    int         infoDictHandle = startPdfObject();

    std::string dt = fmt::format( "D:{:%Y:%m:%d:%H:%M:%S}", fmt::localtime( std::time( nullptr ) ) );

    if( m_title.IsEmpty() )
    {
        // Windows uses '\' and other platforms use '/' as separator
        m_title = m_filename.AfterLast( '\\' );
        m_title = m_title.AfterLast( '/' );
    }

    fmt::print( m_outputFile,
                "<<\n"
                "/Producer (KiCad PDF)\n"
                "/CreationDate ({})\n"
                "/Creator {}\n"
                "/Title {}\n"
                "/Author {}\n"
                "/Subject {}\n",
                dt,
                encodeStringForPlotter( m_creator ),
                encodeStringForPlotter( m_title ),
                encodeStringForPlotter( m_author ),
                encodeStringForPlotter( m_subject ) );

    fmt::println( m_outputFile, ">>" );
    closePdfObject();

    // Let's dump in the outline
    int outlineHandle = -1;
    if( !m_3dExportMode )
    {
        outlineHandle = emitOutline();
    }

    // The catalog, at last
    int catalogHandle = startPdfObject();

    if( outlineHandle > 0 )
    {
        fmt::println( m_outputFile,
                      "<<\n"
                      "/Type /Catalog\n"
                      "/Pages {} 0 R\n"
                      "/Version /1.5\n"
                      "/PageMode /UseOutlines\n"
                      "/Outlines {} 0 R\n"
                      "/Names {} 0 R\n"
                      "/PageLayout /SinglePage\n"
                      ">>",
                      m_pageTreeHandle,
                      outlineHandle,
                      m_jsNamesHandle );
    }
    else
    {
        fmt::println( m_outputFile,
                      "<<\n"
                      "/Type /Catalog\n"
                      "/Pages {} 0 R\n"
                      "/Version /1.5\n"
                      "/PageMode /UseNone\n"
                      "/PageLayout /SinglePage\n"
                      ">>",
                      m_pageTreeHandle );
    }

    closePdfObject();

    /* Emit the xref table (format is crucial to the byte, each entry must
       be 20 bytes long, and object zero must be done in that way). Also
       the offset must be kept along for the trailer */
    long xref_start = ftell( m_outputFile );
    fmt::print( m_outputFile,
                "xref\n"
                "0 {}\n"
                "0000000000 65535 f \n",
                m_xrefTable.size() );

    for( unsigned i = 1; i < m_xrefTable.size(); i++ )
    {
        fmt::print( m_outputFile, "{:010d} 00000 n \n", m_xrefTable[i] );
    }

    // Done the xref, go for the trailer
    fmt::print( m_outputFile,
                "trailer\n"
                "<< /Size {} /Root {} 0 R /Info {} 0 R >>\n"
                "startxref\n"
                "{}\n" // The offset we saved before
                "%%EOF\n",
                m_xrefTable.size(),
                catalogHandle,
                infoDictHandle,
                xref_start );

    fclose( m_outputFile );
    m_outputFile = nullptr;

    return true;
}


void PDF_PLOTTER::Text( const VECTOR2I&        aPos,
                        const COLOR4D&         aColor,
                        const wxString&        aText,
                        const EDA_ANGLE&       aOrient,
                        const VECTOR2I&        aSize,
                        enum GR_TEXT_H_ALIGN_T aH_justify,
                        enum GR_TEXT_V_ALIGN_T aV_justify,
                        int                    aWidth,
                        bool                   aItalic,
                        bool                   aBold,
                        bool                   aMultilineAllowed,
                        KIFONT::FONT*          aFont,
                        const KIFONT::METRICS& aFontMetrics,
                        void*                  aData )
{
    // PDF files do not like 0 sized texts which create broken files.
    if( aSize.x == 0 || aSize.y == 0 )
        return;

    // Render phantom text (which will be searchable) behind the stroke font.  This won't
    // be pixel-accurate, but it doesn't matter for searching.
    int render_mode = 3;    // invisible

    VECTOR2I pos( aPos );
    const char *fontname = aItalic ? ( aBold ? "/KicadFontBI" : "/KicadFontI" )
                                   : ( aBold ? "/KicadFontB"  : "/KicadFont"  );

    // Compute the copious transformation parameters of the Current Transform Matrix
    double ctm_a, ctm_b, ctm_c, ctm_d, ctm_e, ctm_f;
    double wideningFactor, heightFactor;

    VECTOR2I t_size( std::abs( aSize.x ), std::abs( aSize.y ) );
    bool     textMirrored = aSize.x < 0;

    computeTextParameters( aPos, aText, aOrient, t_size, textMirrored, aH_justify, aV_justify,
                           aWidth, aItalic, aBold, &wideningFactor, &ctm_a, &ctm_b, &ctm_c, &ctm_d,
                           &ctm_e, &ctm_f, &heightFactor );

    SetColor( aColor );
    SetCurrentLineWidth( aWidth, aData );

    wxStringTokenizer str_tok( aText, " ", wxTOKEN_RET_DELIMS );

    if( !aFont )
        aFont = KIFONT::FONT::GetFont( m_renderSettings->GetDefaultFont() );

    VECTOR2I full_box( aFont->StringBoundaryLimits( aText, t_size, aWidth, aBold, aItalic,
                                                    aFontMetrics ) );

    if( textMirrored )
        full_box.x *= -1;

    VECTOR2I box_x( full_box.x, 0 );
    VECTOR2I box_y( 0, full_box.y );

    RotatePoint( box_x, aOrient );
    RotatePoint( box_y, aOrient );

    if( aH_justify == GR_TEXT_H_ALIGN_CENTER )
        pos -= box_x / 2;
    else if( aH_justify == GR_TEXT_H_ALIGN_RIGHT )
        pos -= box_x;

    if( aV_justify == GR_TEXT_V_ALIGN_CENTER )
        pos += box_y / 2;
    else if( aV_justify == GR_TEXT_V_ALIGN_TOP )
        pos += box_y;

    while( str_tok.HasMoreTokens() )
    {
        wxString word = str_tok.GetNextToken();

        computeTextParameters( pos, word, aOrient, t_size, textMirrored, GR_TEXT_H_ALIGN_LEFT,
                               GR_TEXT_V_ALIGN_BOTTOM, aWidth, aItalic, aBold, &wideningFactor,
                               &ctm_a, &ctm_b, &ctm_c, &ctm_d, &ctm_e, &ctm_f, &heightFactor );

        // Extract the changed width and rotate by the orientation to get the offset for the
        // next word
        VECTOR2I bbox( aFont->StringBoundaryLimits( word, t_size, aWidth,
                                                    aBold, aItalic, aFontMetrics ).x, 0 );

        if( textMirrored )
            bbox.x *= -1;

        RotatePoint( bbox, aOrient );
        pos += bbox;

        // Don't try to output a blank string
        if( word.Trim( false ).Trim( true ).empty() )
            continue;

        /* We use the full CTM instead of the text matrix because the same
           coordinate system will be used for the overlining. Also the %f
           for the trig part of the matrix to avoid %g going in exponential
           format (which is not supported) */
        fmt::print( m_workFile, "q {:f} {:f} {:f} {:f} {:f} {:f} cm BT {} {:g} Tf {} Tr {:g} Tz ",
                    ctm_a, ctm_b, ctm_c, ctm_d, ctm_e, ctm_f,
                    fontname,
                    heightFactor,
                    render_mode,
                    wideningFactor * 100 );

        std::string txt_pdf = encodeStringForPlotter( word );
        fmt::println( m_workFile, "{} Tj ET", txt_pdf );
        // Restore the CTM
        fmt::println( m_workFile, "Q" );
    }

    // Plot the stroked text (if requested)
    PLOTTER::Text( aPos, aColor, aText, aOrient, aSize, aH_justify, aV_justify, aWidth, aItalic,
                   aBold, aMultilineAllowed, aFont, aFontMetrics );
}


void PDF_PLOTTER::PlotText( const VECTOR2I&        aPos,
                            const COLOR4D&         aColor,
                            const wxString&        aText,
                            const TEXT_ATTRIBUTES& aAttributes,
                            KIFONT::FONT*          aFont,
                            const KIFONT::METRICS& aFontMetrics,
                            void*                  aData )
{
    VECTOR2I size = aAttributes.m_Size;

    // PDF files do not like 0 sized texts which create broken files.
    if( size.x == 0 || size.y == 0 )
        return;

    if( aAttributes.m_Mirrored )
        size.x = -size.x;

    PDF_PLOTTER::Text( aPos, aColor, aText, aAttributes.m_Angle, size, aAttributes.m_Halign,
                       aAttributes.m_Valign, aAttributes.m_StrokeWidth, aAttributes.m_Italic,
                       aAttributes.m_Bold, aAttributes.m_Multiline, aFont, aFontMetrics, aData );
}


void PDF_PLOTTER::HyperlinkBox( const BOX2I& aBox, const wxString& aDestinationURL )
{
    m_hyperlinksInPage.push_back( std::make_pair( aBox, aDestinationURL ) );
}


void PDF_PLOTTER::HyperlinkMenu( const BOX2I& aBox, const std::vector<wxString>& aDestURLs )
{
    m_hyperlinkMenusInPage.push_back( std::make_pair( aBox, aDestURLs ) );
}


void PDF_PLOTTER::Bookmark( const BOX2I& aLocation, const wxString& aSymbolReference,
                            const wxString &aGroupName )
{

    m_bookmarksInPage[aGroupName].push_back( std::make_pair( aLocation, aSymbolReference ) );
}


void PDF_PLOTTER::Plot3DModel( const wxString&                 aSourcePath,
                               const std::vector<PDF_3D_VIEW>& a3DViews )
{
    std::map<float, int> m_fovMap;
    std::vector<int>     m_viewHandles;

    for( const PDF_3D_VIEW& view : a3DViews )
    {
        int fovHandle = -1;
        if( !m_fovMap.contains( view.m_fov ) )
        {
            fovHandle = allocPdfObject();
            m_fovMap[view.m_fov] = fovHandle;

            startPdfObject( fovHandle );
            fmt::print( m_outputFile,
                        "<<\n"
                        "/FOV {}\n"
                        "/PS /Min\n"
                        "/Subtype /P\n"
                        ">>\n",
                        view.m_fov );
            closePdfObject();
        }
        else
        {
            fovHandle = m_fovMap[view.m_fov];
        }

        int viewHandle = allocPdfObject();
        startPdfObject( viewHandle );

        fmt::print( m_outputFile,
                    "<<\n"
                    "/Type /3DView\n"
                    "/XN ({})\n"
                    "/IN ({})\n"
                    "/MS /M\n"
                    "/C2W [{:f} {:f} {:f} {:f} {:f} {:f} {:f} {:f} {:f} {:f} {:f} {:f}]\n"
                    "/CO {:f}\n"
                    "/NR false\n"
                    "/BG<<\n"
                    "/Type /3DBG\n"
                    "/Subtype /SC\n"
                    "/CS /DeviceRGB\n"
                    "/C [1.000000 1.000000 1.000000]>>\n"
                    "/P {} 0 R\n"
                    "/LS<<\n"
                    "/Type /3DLightingScheme\n"
                    "/Subtype /CAD>>\n"
                    ">>\n",
                    view.m_name, view.m_name, view.m_cameraMatrix[0],
                    view.m_cameraMatrix[1],
                    view.m_cameraMatrix[2], view.m_cameraMatrix[3], view.m_cameraMatrix[4],
                    view.m_cameraMatrix[5], view.m_cameraMatrix[6], view.m_cameraMatrix[7],
                    view.m_cameraMatrix[8], view.m_cameraMatrix[9], view.m_cameraMatrix[10],
                    view.m_cameraMatrix[11],
                    view.m_cameraCenter,
                    fovHandle );

        closePdfObject();

        m_viewHandles.push_back( viewHandle );
    }

    m_3dModelHandle = startPdfObject();

    // so we can get remotely stuff the length afterwards
    int modelLenHandle = allocPdfObject();

    fmt::print( m_outputFile,
                "<<\n"
                "/Type /3D\n"
                "/Subtype /U3D\n"
                "/DV 0\n" );

    fmt::print( m_outputFile, "/VA [" );
    for( int viewHandle : m_viewHandles )
    {
        fmt::print( m_outputFile, "{} 0 R ", viewHandle );
    }
    fmt::print( m_outputFile, "]\n" );

    fmt::print( m_outputFile,
                "/Length {} 0 R\n"
                "/Filter /FlateDecode\n"
                ">>\n", // Length is deferred
                modelLenHandle );

    fmt::println( m_outputFile, "stream" );

    wxFFile outputFFile( m_outputFile );

    fflush( m_outputFile );
    long imgStreamStart = ftell( m_outputFile );

    size_t model_stored_size = 0;
    {
        wxFFileOutputStream ffos( outputFFile );
        wxZlibOutputStream  zos( ffos, wxZ_BEST_COMPRESSION, wxZLIB_ZLIB );

        wxFFileInputStream fileStream( aSourcePath );
        if( !fileStream.IsOk() )
        {
            wxLogError( _( "Failed to open 3D model file: %s" ), aSourcePath );
        }

        zos.Write( fileStream );
    }
    fflush( m_outputFile );
    model_stored_size = ftell( m_outputFile );
    model_stored_size -= imgStreamStart; // Get the size of the compressed stream

    fmt::print( m_outputFile, "\nendstream\n" );
    closePdfObject();

    startPdfObject( modelLenHandle );
    fmt::println( m_outputFile, "{}", (unsigned) model_stored_size );
    closePdfObject();

    outputFFile.Detach(); // Don't close it
}
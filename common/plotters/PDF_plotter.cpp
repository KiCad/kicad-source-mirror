/**
 * @file PDF_plotter.cpp
 * @brief KiCad: specialized plotter for PDF files format
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 Lorenzo Marcantonio, l.marcantonio@logossrl.com
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <wx/filename.h>
#include <wx/mstream.h>
#include <wx/zstream.h>
#include <wx/wfstream.h>
#include <wx/datstrm.h>

#include <advanced_config.h>
#include <eda_text.h> // for IsGotoPageHref
#include <font/font.h>
#include <core/ignore.h>
#include <macros.h>
#include <trigo.h>
#include <string_utils.h>

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
            char buffer[16];
            std::snprintf( buffer, sizeof( buffer ), "%4.4X", code );
            result += buffer;

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
        fprintf( m_workFile, "%g w\n", userToDeviceSize( aWidth ) );

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

    fprintf( m_workFile, "%g %g %g rg %g %g %g RG\n", r, g, b, r, g, b );
}


void PDF_PLOTTER::SetDash( int aLineWidth, PLOT_DASH_TYPE aLineStyle )
{
    wxASSERT( m_workFile );

    switch( aLineStyle )
    {
    case PLOT_DASH_TYPE::DASH:
        fprintf( m_workFile, "[%d %d] 0 d\n",
                (int) GetDashMarkLenIU( aLineWidth ), (int) GetDashGapLenIU( aLineWidth ) );
        break;

    case PLOT_DASH_TYPE::DOT:
        fprintf( m_workFile, "[%d %d] 0 d\n",
                (int) GetDotMarkLenIU( aLineWidth ), (int) GetDashGapLenIU( aLineWidth ) );
        break;

    case PLOT_DASH_TYPE::DASHDOT:
        fprintf( m_workFile, "[%d %d %d %d] 0 d\n",
                (int) GetDashMarkLenIU( aLineWidth ), (int) GetDashGapLenIU( aLineWidth ),
                (int) GetDotMarkLenIU( aLineWidth ), (int) GetDashGapLenIU( aLineWidth ) );
        break;

    case PLOT_DASH_TYPE::DASHDOTDOT:
        fprintf( m_workFile, "[%d %d %d %d %d %d] 0 d\n",
                (int) GetDashMarkLenIU( aLineWidth ), (int) GetDashGapLenIU( aLineWidth ),
                (int) GetDotMarkLenIU( aLineWidth ), (int) GetDashGapLenIU( aLineWidth ),
                (int) GetDotMarkLenIU( aLineWidth ), (int) GetDashGapLenIU( aLineWidth ) );
        break;

    default:
        fputs( "[] 0 d\n", m_workFile );
    }
}


void PDF_PLOTTER::Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T fill, int width )
{
    wxASSERT( m_workFile );
    VECTOR2D p1_dev = userToDeviceCoordinates( p1 );
    VECTOR2D p2_dev = userToDeviceCoordinates( p2 );

    SetCurrentLineWidth( width );
    fprintf( m_workFile, "%g %g %g %g re %c\n", p1_dev.x, p1_dev.y,
             p2_dev.x - p1_dev.x, p2_dev.y - p1_dev.y, fill == FILL_T::NO_FILL ? 'S' : 'B' );
}


void PDF_PLOTTER::Circle( const VECTOR2I& pos, int diametre, FILL_T aFill, int width )
{
    wxASSERT( m_workFile );
    VECTOR2D pos_dev = userToDeviceCoordinates( pos );
    double   radius = userToDeviceSize( diametre / 2.0 );

    /* OK. Here's a trick. PDF doesn't support circles or circular angles, that's
       a fact. You'll have to do with cubic beziers. These *can't* represent
       circular arcs (NURBS can, beziers don't). But there is a widely known
       approximation which is really good
    */

    SetCurrentLineWidth( width );

    // If diameter is less than width, switch to filled mode
    if( aFill == FILL_T::NO_FILL && diametre < width )
    {
        aFill = FILL_T::FILLED_SHAPE;
        SetCurrentLineWidth( 0 );

        radius = userToDeviceSize( ( diametre / 2.0 ) + ( width / 2.0 ) );
    }

    double magic = radius * 0.551784; // You don't want to know where this come from

    // This is the convex hull for the bezier approximated circle
    fprintf( m_workFile,
             "%g %g m "
             "%g %g %g %g %g %g c "
             "%g %g %g %g %g %g c "
             "%g %g %g %g %g %g c "
             "%g %g %g %g %g %g c %c\n",
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


void PDF_PLOTTER::Arc( const VECTOR2I& aCenter, const VECTOR2I& aStart, const VECTOR2I& aEnd,
                       FILL_T aFill, int aWidth, int aMaxError )
{
    wxASSERT( m_workFile );

    /*
     * Arcs are not so easily approximated by beziers (in the general case), so we approximate
     * them in the old way
     */
    EDA_ANGLE startAngle( aStart - aCenter );
    EDA_ANGLE endAngle( aEnd - aCenter );
    int       radius = ( aStart - aCenter ).EuclideanNorm();
    int       numSegs = GetArcToSegmentCount( radius, aMaxError, FULL_CIRCLE );
    EDA_ANGLE delta = ANGLE_360 / std::max( 8, numSegs );
    VECTOR2I  start( aStart );
    VECTOR2I  end( aEnd );
    VECTOR2I  pt;

    if( startAngle > endAngle )
    {
        if( endAngle < ANGLE_0 )
            endAngle.Normalize();
        else
            startAngle = startAngle.Normalize() - ANGLE_360;
    }

    SetCurrentLineWidth( aWidth );
    VECTOR2D pos_dev = userToDeviceCoordinates( start );
    fprintf( m_workFile, "%g %g m ", pos_dev.x, pos_dev.y );

    for( EDA_ANGLE ii = delta; startAngle + ii < endAngle; ii += delta )
    {
        pt = start;
        RotatePoint( pt, aCenter, -ii );

        pos_dev = userToDeviceCoordinates( pt );
        fprintf( m_workFile, "%g %g l ", pos_dev.x, pos_dev.y );
    }

    pos_dev = userToDeviceCoordinates( end );
    fprintf( m_workFile, "%g %g l ", pos_dev.x, pos_dev.y );

    // The arc is drawn... if not filled we stroke it, otherwise we finish
    // closing the pie at the center
    if( aFill == FILL_T::NO_FILL )
    {
        fputs( "S\n", m_workFile );
    }
    else
    {
        pos_dev = userToDeviceCoordinates( aCenter );
        fprintf( m_workFile, "%g %g l b\n", pos_dev.x, pos_dev.y );
    }
}


void PDF_PLOTTER::Arc( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                       const EDA_ANGLE& aEndAngle, double aRadius, FILL_T aFill, int aWidth )
{
    wxASSERT( m_workFile );

    if( aRadius <= 0 )
    {
        Circle( aCenter, aWidth, FILL_T::FILLED_SHAPE, 0 );
        return;
    }

    /*
     * Arcs are not so easily approximated by beziers (in the general case), so we approximate
     * them in the old way
     */
    EDA_ANGLE       startAngle( aStartAngle );
    EDA_ANGLE       endAngle( aEndAngle );
    VECTOR2I        start;
    VECTOR2I        end;
    const EDA_ANGLE delta( 5, DEGREES_T );   // increment to draw circles

    if( startAngle > endAngle )
    {
        std::swap( startAngle, endAngle );
        std::swap( start, end );
    }

    SetCurrentLineWidth( aWidth );

    // Usual trig arc plotting routine...
    start.x = aCenter.x + KiROUND( aRadius * (-startAngle).Cos() );
    start.y = aCenter.y + KiROUND( aRadius * (-startAngle).Sin() );
    VECTOR2D pos_dev = userToDeviceCoordinates( start );
    fprintf( m_workFile, "%g %g m ", pos_dev.x, pos_dev.y );

    for( EDA_ANGLE ii = startAngle + delta; ii < endAngle; ii += delta )
    {
        end.x = aCenter.x + KiROUND( aRadius * (-ii).Cos() );
        end.y = aCenter.y + KiROUND( aRadius * (-ii).Sin() );
        pos_dev = userToDeviceCoordinates( end );
        fprintf( m_workFile, "%g %g l ", pos_dev.x, pos_dev.y );
    }

    end.x = aCenter.x + KiROUND( aRadius * (-endAngle).Cos() );
    end.y = aCenter.y + KiROUND( aRadius * (-endAngle).Sin() );
    pos_dev = userToDeviceCoordinates( end );
    fprintf( m_workFile, "%g %g l ", pos_dev.x, pos_dev.y );

    // The arc is drawn... if not filled we stroke it, otherwise we finish
    // closing the pie at the center
    if( aFill == FILL_T::NO_FILL )
    {
        fputs( "S\n", m_workFile );
    }
    else
    {
        pos_dev = userToDeviceCoordinates( aCenter );
        fprintf( m_workFile, "%g %g l b\n", pos_dev.x, pos_dev.y );
    }
}


void PDF_PLOTTER::PlotPoly( const std::vector<VECTOR2I>& aCornerList, FILL_T aFill, int aWidth,
                            void* aData )
{
    wxASSERT( m_workFile );

    if( aCornerList.size() <= 1 )
        return;

    SetCurrentLineWidth( aWidth );

    VECTOR2D pos = userToDeviceCoordinates( aCornerList[0] );
    fprintf( m_workFile, "%g %g m\n", pos.x, pos.y );

    for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
    {
        pos = userToDeviceCoordinates( aCornerList[ii] );
        fprintf( m_workFile, "%g %g l\n", pos.x, pos.y );
    }

    // Close path and stroke and/or fill
    if( aFill == FILL_T::NO_FILL )
        fputs( "S\n", m_workFile );
    else if( aWidth == 0 )
        fputs( "f\n", m_workFile );
    else
        fputs( "b\n", m_workFile );
}


void PDF_PLOTTER::PenTo( const VECTOR2I& pos, char plume )
{
    wxASSERT( m_workFile );

    if( plume == 'Z' )
    {
        if( m_penState != 'Z' )
        {
            fputs( "S\n", m_workFile );
            m_penState     = 'Z';
            m_penLastpos.x = -1;
            m_penLastpos.y = -1;
        }

        return;
    }

    if( m_penState != plume || pos != m_penLastpos )
    {
        VECTOR2D pos_dev = userToDeviceCoordinates( pos );
        fprintf( m_workFile, "%g %g %c\n",
                 pos_dev.x, pos_dev.y,
                 ( plume=='D' ) ? 'l' : 'm' );
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

            if( image.HasMask() != aCurrImage.HasMask() || image.GetMaskRed() != aCurrImage.GetMaskRed()
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
    fprintf( m_workFile, "q %g 0 0 %g %g %g cm\n", // Step 1
             userToDeviceSize( drawsize.x ),
             userToDeviceSize( drawsize.y ),
             dev_start.x, dev_start.y );

    fprintf( m_workFile, "/Im%d Do\n", imgHandle );
    fputs( "Q\n", m_workFile );
}


int PDF_PLOTTER::allocPdfObject()
{
    m_xrefTable.push_back( 0 );
    return m_xrefTable.size() - 1;
}


int PDF_PLOTTER::startPdfObject(int handle)
{
    wxASSERT( m_outputFile );
    wxASSERT( !m_workFile );

    if( handle < 0)
        handle = allocPdfObject();

    m_xrefTable[handle] = ftell( m_outputFile );
    fprintf( m_outputFile, "%d 0 obj\n", handle );
    return handle;
}


void PDF_PLOTTER::closePdfObject()
{
    wxASSERT( m_outputFile );
    wxASSERT( !m_workFile );
    fputs( "endobj\n", m_outputFile );
}


int PDF_PLOTTER::startPdfStream( int handle )
{
    wxASSERT( m_outputFile );
    wxASSERT( !m_workFile );
    handle = startPdfObject( handle );

    // This is guaranteed to be handle+1 but needs to be allocated since
    // you could allocate more object during stream preparation
    m_streamLengthHandle = allocPdfObject();

    if( ADVANCED_CFG::GetCfg().m_DebugPDFWriter )
    {
        fprintf( m_outputFile,
                 "<< /Length %d 0 R >>\n" // Length is deferred
                 "stream\n", handle + 1 );
    }
    else
    {
        fprintf( m_outputFile,
                 "<< /Length %d 0 R /Filter /FlateDecode >>\n" // Length is deferred
                 "stream\n", handle + 1 );
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
    fputs( "\nendstream\n", m_outputFile );
    closePdfObject();

    // Writing the deferred length as an indirect object
    startPdfObject( m_streamLengthHandle );
    fprintf( m_outputFile, "%u\n", out_count );
    closePdfObject();
}


void PDF_PLOTTER::StartPage( const wxString& aPageNumber, const wxString& aPageName )
{
    wxASSERT( m_outputFile );
    wxASSERT( !m_workFile );

    m_pageNumbers.push_back( aPageNumber );
    m_pageName = aPageName;

    // Compute the paper size in IUs
    m_paperSize = m_pageInfo.GetSizeMils();
    m_paperSize.x *= 10.0 / m_iuPerDeviceUnit;
    m_paperSize.y *= 10.0 / m_iuPerDeviceUnit;

    // Open the content stream; the page object will go later
    m_pageStreamHandle = startPdfStream();

    /* Now, until ClosePage *everything* must be wrote in workFile, to be
       compressed later in closePdfStream */

    // Default graphic settings (coordinate system, default color and line style)
    fprintf( m_workFile,
             "%g 0 0 %g 0 0 cm 1 J 1 j 0 0 0 rg 0 0 0 RG %g w\n",
             0.0072 * plotScaleAdjX, 0.0072 * plotScaleAdjY,
             userToDeviceSize( m_renderSettings->GetDefaultPenWidth() ) );
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
    wxASSERT( m_workFile );

    // Close the page stream (and compress it)
    closePdfStream();

    // Page size is in 1/72 of inch (default user space units).  Works like the bbox in postscript
    // but there is no need for swapping the sizes, since PDF doesn't require a portrait page.
    // We use the MediaBox but PDF has lots of other less-used boxes that could be used.
    const double PTsPERMIL = 0.072;
    VECTOR2D     psPaperSize = VECTOR2D( m_pageInfo.GetSizeMils() ) * PTsPERMIL;

    auto iuToPdfUserSpace =
            [&]( const VECTOR2I& aCoord ) -> VECTOR2D
            {
                VECTOR2D retval = VECTOR2D( aCoord ) * PTsPERMIL / ( m_IUsPerDecimil * 10 );
                // PDF y=0 is at bottom of page, invert coordinate
                retval.y = psPaperSize.y - retval.y;
                return retval;
            };

    // Handle annotations (at the moment only "link" type objects)
    std::vector<int> hyperlinkHandles;

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

        hyperlinkHandles.push_back( allocPdfObject() );

        m_hyperlinkHandles.insert( { hyperlinkHandles.back(), { userSpaceBox, url } } );
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

        hyperlinkHandles.push_back( allocPdfObject() );

        m_hyperlinkMenuHandles.insert( { hyperlinkHandles.back(), { userSpaceBox, urls } } );
    }

    int hyperLinkArrayHandle = -1;

    // If we have added any annotation links, create an array containing all the objects
    if( hyperlinkHandles.size() > 0 )
    {
        hyperLinkArrayHandle = startPdfObject();
        bool isFirst = true;

        fputs( "[", m_outputFile );

        for( int handle : hyperlinkHandles )
        {
            if( isFirst )
                isFirst = false;
            else
                fprintf( m_outputFile, " " );

            fprintf( m_outputFile, "%d 0 R", handle );
        }

        fputs( "]\n", m_outputFile );
        closePdfObject();
    }

    // Emit the page object and put it in the page list for later
    int pageHandle = startPdfObject();
    m_pageHandles.push_back( pageHandle );

    fprintf( m_outputFile,
             "<<\n"
             "/Type /Page\n"
             "/Parent %d 0 R\n"
             "/Resources <<\n"
             "    /ProcSet [/PDF /Text /ImageC /ImageB]\n"
             "    /Font %d 0 R\n"
             "    /XObject %d 0 R >>\n"
             "/MediaBox [0 0 %g %g]\n"
             "/Contents %d 0 R\n",
             m_pageTreeHandle,
             m_fontResDictHandle,
             m_imgResDictHandle,
             psPaperSize.x,
             psPaperSize.y,
             m_pageStreamHandle );

    if( hyperlinkHandles.size() > 0 )
        fprintf( m_outputFile, "/Annots %d 0 R", hyperLinkArrayHandle );

    fputs( ">>\n", m_outputFile );

    closePdfObject();

    // Mark the page stream as idle
    m_pageStreamHandle = 0;

    wxString pageOutlineName = wxEmptyString;

    if( m_pageName.IsEmpty() )
    {
        pageOutlineName = wxString::Format( _( "Page %s" ), m_pageNumbers.back() );
    }
    else
    {
        pageOutlineName = wxString::Format( _( "%s (Page %s)" ), m_pageName, m_pageNumbers.back() );
    }


    int           actionHandle = emitGoToAction( pageHandle );
    OUTLINE_NODE* pageOutlineNode =
            addOutlineNode( m_outlineRoot.get(), actionHandle, pageOutlineName );

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
    fputs("%PDF-1.5\n%\200\201\202\203\n", m_outputFile);

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

    fprintf( m_outputFile,
             "<</S /GoTo /D [%d 0 R /FitR %d %d %d %d]\n"
             ">>\n",
             aPageHandle, aBottomLeft.x, aBottomLeft.y, aTopRight.x, aTopRight.y );

    closePdfObject();

    return actionHandle;
}


int PDF_PLOTTER::emitGoToAction( int aPageHandle )
{
    int actionHandle = allocPdfObject();
    startPdfObject( actionHandle );

    fprintf( m_outputFile,
             "<</S /GoTo /D [%d 0 R /Fit]\n"
             ">>\n",
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

        fprintf( m_outputFile,
                 "<<\n"
                 "/Title %s\n"
                 "/Parent %d 0 R\n",
                 encodeStringForPlotter(node->title ).c_str(),
                 parentHandle);

        if( nextNode > 0 )
        {
            fprintf( m_outputFile, "/Next %d 0 R\n", nextNode );
        }

        if( prevNode > 0 )
        {
            fprintf( m_outputFile, "/Prev %d 0 R\n", prevNode );
        }

        if( node->children.size() > 0 )
        {
            fprintf( m_outputFile, "/Count %zd\n", -1 * node->children.size() );
            fprintf( m_outputFile, "/First %d 0 R\n", node->children.front()->entryHandle );
            fprintf( m_outputFile, "/Last %d 0 R\n", node->children.back()->entryHandle );
        }

        if( node->actionHandle != -1 )
        {
            fprintf( m_outputFile, "/A %d 0 R\n", node->actionHandle );
        }

        fputs( ">>\n", m_outputFile );
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

        fprintf( m_outputFile,
                 "<< /Type /Outlines\n"
                 "   /Count %d\n"
                 "   /First %d 0 R\n"
                 "   /Last %d 0 R\n"
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


bool PDF_PLOTTER::EndPlot()
{
    wxASSERT( m_outputFile );

    // Close the current page (often the only one)
    ClosePage();

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
        fprintf( m_outputFile,
                 "<< /BaseFont %s\n"
                 "   /Type /Font\n"
                 "   /Subtype /Type1\n"
                 /* Adobe is so Mac-based that the nearest thing to Latin1 is
                    the Windows ANSI encoding! */
                 "   /Encoding /WinAnsiEncoding\n"
                 ">>\n",
                 fontdefs[i].psname );
        closePdfObject();
    }

    // Named font dictionary (was allocated, now we emit it)
    startPdfObject( m_fontResDictHandle );
    fputs( "<<\n", m_outputFile );

    for( int i = 0; i < 4; i++ )
    {
        fprintf( m_outputFile, "    %s %d 0 R\n",
                 fontdefs[i].rsname, fontdefs[i].font_handle );
    }

    fputs( ">>\n", m_outputFile );
    closePdfObject();

    // Named image dictionary (was allocated, now we emit it)
    startPdfObject( m_imgResDictHandle );
    fputs( "<<\n", m_outputFile );

    for( const auto& [imgHandle, image] : m_imageHandles )
    {
        fprintf( m_outputFile, "    /Im%d %d 0 R\n", imgHandle, imgHandle );
    }

    fputs( ">>\n", m_outputFile );
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

        fprintf( m_outputFile,
                 "<<\n"
                 "/Type /XObject\n"
                 "/Subtype /Image\n"
                 "/BitsPerComponent 8\n"
                 "/ColorSpace %s\n"
                 "/Width %d\n"
                 "/Height %d\n"
                 "/Filter /FlateDecode\n"
                 "/Length %d 0 R\n", // Length is deferred
                 m_colorMode ? "/DeviceRGB" : "/DeviceGray", image.GetWidth(), image.GetHeight(),
                 imgLenHandle );

        if( smaskHandle != -1 )
            fprintf( m_outputFile, "/SMask %d 0 R\n", smaskHandle );

        fputs( ">>\n", m_outputFile );
        fputs( "stream\n", m_outputFile );

        long imgStreamStart = ftell( m_outputFile );

        {
            wxFFileOutputStream ffos( outputFFile );
            wxZlibOutputStream  zos( ffos, wxZ_BEST_COMPRESSION, wxZLIB_ZLIB );
            wxDataOutputStream  dos( zos );

            WriteImageStream( image, dos, m_renderSettings->GetBackgroundColor().ToColour(),
                              m_colorMode );
        }

        long imgStreamSize = ftell( m_outputFile ) - imgStreamStart;

        fputs( "\nendstream\n", m_outputFile );
        closePdfObject();

        startPdfObject( imgLenHandle );
        fprintf( m_outputFile, "%ld\n", imgStreamSize );
        closePdfObject();

        if( smaskHandle != -1 )
        {
            // SMask
            startPdfObject( smaskHandle );
            int smaskLenHandle = allocPdfObject();

            fprintf( m_outputFile,
                     "<<\n"
                     "/Type /XObject\n"
                     "/Subtype /Image\n"
                     "/BitsPerComponent 8\n"
                     "/ColorSpace /DeviceGray\n"
                     "/Width %d\n"
                     "/Height %d\n"
                     "/Length %d 0 R\n"
                     "/Filter /FlateDecode\n"
                     ">>\n", // Length is deferred
                     image.GetWidth(), image.GetHeight(), smaskLenHandle );

            fputs( "stream\n", m_outputFile );

            long smaskStreamStart = ftell( m_outputFile );

            {
                wxFFileOutputStream ffos( outputFFile );
                wxZlibOutputStream  zos( ffos, wxZ_BEST_COMPRESSION, wxZLIB_ZLIB );
                wxDataOutputStream  dos( zos );

                WriteImageSMaskStream( image, dos );
            }

            long smaskStreamSize = ftell( m_outputFile ) - smaskStreamStart;

            fputs( "\nendstream\n", m_outputFile );
            closePdfObject();

            startPdfObject( smaskLenHandle );
            fprintf( m_outputFile, "%u\n", (unsigned) smaskStreamSize );
            closePdfObject();
        }

        outputFFile.Detach(); // Don't close it
    }

    for( const auto& [ linkHandle, linkPair ] : m_hyperlinkHandles )
    {
        const BOX2D&    box = linkPair.first;
        const wxString& url = linkPair.second;

        startPdfObject( linkHandle );

        fprintf( m_outputFile,
                 "<<\n"
                 "/Type /Annot\n"
                 "/Subtype /Link\n"
                 "/Rect [%g %g %g %g]\n"
                 "/Border [16 16 0]\n",
                 box.GetLeft(), box.GetBottom(), box.GetRight(), box.GetTop() );

        wxString pageNumber;
        bool     pageFound = false;

        if( EDA_TEXT::IsGotoPageHref( url, &pageNumber ) )
        {
            for( size_t ii = 0; ii < m_pageNumbers.size(); ++ii )
            {
                if( m_pageNumbers[ii] == pageNumber )
                {
                    fprintf( m_outputFile,
                             "/Dest [%d 0 R /FitB]\n"
                             ">>\n",
                             m_pageHandles[ii] );

                    pageFound = true;
                    break;
                }
            }

            if( !pageFound )
            {
                // destination page is not being plotted, assign the NOP action to the link
                fprintf( m_outputFile, "/A << /Type /Action /S /NOP >>\n"
                                       ">>\n" );
            }
        }
        else
        {
            fprintf( m_outputFile,
                     "/A << /Type /Action /S /URI /URI %s >>\n"
                     ">>\n",
                     encodeStringForPlotter( url ).c_str() );
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

                    js += wxString::Format( wxT( "[\"%s\", \"%s\"],\n" ),
                                            EscapeString( property, CTX_JS_STR ),
                                            EscapeString( href, CTX_JS_STR ) );
                }
                else if( property.Find( "https:" ) >= 0 )
                {
                    wxString href = property.substr( property.Find( "https:" ) );

                    js += wxString::Format( wxT( "[\"%s\", \"%s\"],\n" ),
                                            EscapeString( property, CTX_JS_STR ),
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
            else if( url.StartsWith( "http:" ) || url.StartsWith( "https:" ) )
            {
                wxString menuText = wxString::Format( _( "Open %s" ), url );

                js += wxString::Format( wxT( "[\"%s\", \"%s\"],\n" ),
                                        EscapeString( menuText, CTX_JS_STR ),
                                        EscapeString( url, CTX_JS_STR ) );
            }
        }

        js += wxT( "]);" );

        startPdfObject( menuHandle );

        fprintf( m_outputFile,
                 "<<\n"
                 "/Type /Annot\n"
                 "/Subtype /Link\n"
                 "/Rect [%g %g %g %g]\n"
                 "/Border [16 16 0]\n",
                 box.GetLeft(), box.GetBottom(), box.GetRight(), box.GetTop() );

        fprintf( m_outputFile,
                 "/A << /Type /Action /S /JavaScript /JS %s >>\n"
                 ">>\n",
                 encodeStringForPlotter( js ).c_str() );

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
}
)JS";

        fprintf( m_outputFile,
                 "<< /JavaScript\n"
                 " << /Names\n"
                 "    [ (JSInit) << /Type /Action /S /JavaScript /JS %s >> ]\n"
                 " >>\n"
                 ">>\n",
                 encodeStringForPlotter( js ).c_str() );

        closePdfObject();
    }

    /* The page tree: it's a B-tree but luckily we only have few pages!
       So we use just an array... The handle was allocated at the beginning,
       now we instantiate the corresponding object */
    startPdfObject( m_pageTreeHandle );
    fputs( "<<\n"
           "/Type /Pages\n"
           "/Kids [\n", m_outputFile );

    for( unsigned i = 0; i < m_pageHandles.size(); i++ )
        fprintf( m_outputFile, "%d 0 R\n", m_pageHandles[i] );

    fprintf( m_outputFile,
            "]\n"
            "/Count %ld\n"
            ">>\n", (long) m_pageHandles.size() );
    closePdfObject();


    // The info dictionary
    int infoDictHandle = startPdfObject();
    char date_buf[250];
    time_t ltime = time( nullptr );
    strftime( date_buf, 250, "D:%Y%m%d%H%M%S", localtime( &ltime ) );

    if( m_title.IsEmpty() )
    {
        // Windows uses '\' and other platforms use '/' as separator
        m_title = m_filename.AfterLast( '\\' );
        m_title = m_title.AfterLast( '/' );
    }

    fprintf( m_outputFile,
             "<<\n"
             "/Producer (KiCad PDF)\n"
             "/CreationDate (%s)\n"
             "/Creator %s\n"
             "/Title %s\n",
             date_buf,
             encodeStringForPlotter( m_creator ).c_str(),
             encodeStringForPlotter( m_title ).c_str() );

    fputs( ">>\n", m_outputFile );
    closePdfObject();

    // Let's dump in the outline
    int outlineHandle = emitOutline();

    // The catalog, at last
    int catalogHandle = startPdfObject();

    if( outlineHandle > 0 )
    {
        fprintf( m_outputFile,
                 "<<\n"
                 "/Type /Catalog\n"
                 "/Pages %d 0 R\n"
                 "/Version /1.5\n"
                 "/PageMode /UseOutlines\n"
                 "/Outlines %d 0 R\n"
                 "/Names %d 0 R\n"
                 "/PageLayout /SinglePage\n"
                 ">>\n",
                 m_pageTreeHandle,
                 outlineHandle,
                 m_jsNamesHandle );
    }
    else
    {
        fprintf( m_outputFile,
                 "<<\n"
                 "/Type /Catalog\n"
                 "/Pages %d 0 R\n"
                 "/Version /1.5\n"
                 "/PageMode /UseNone\n"
                 "/PageLayout /SinglePage\n"
                 ">>\n",
                 m_pageTreeHandle );
    }

    closePdfObject();

    /* Emit the xref table (format is crucial to the byte, each entry must
       be 20 bytes long, and object zero must be done in that way). Also
       the offset must be kept along for the trailer */
    long xref_start = ftell( m_outputFile );
    fprintf( m_outputFile,
             "xref\n"
             "0 %ld\n"
             "0000000000 65535 f \n", (long) m_xrefTable.size() );

    for( unsigned i = 1; i < m_xrefTable.size(); i++ )
    {
        fprintf( m_outputFile, "%010ld 00000 n \n", m_xrefTable[i] );
    }

    // Done the xref, go for the trailer
    fprintf( m_outputFile,
             "trailer\n"
             "<< /Size %lu /Root %d 0 R /Info %d 0 R >>\n"
             "startxref\n"
             "%ld\n" // The offset we saved before
             "%%%%EOF\n",
             (unsigned long) m_xrefTable.size(), catalogHandle, infoDictHandle, xref_start );

    fclose( m_outputFile );
    m_outputFile = nullptr;

    return true;
}


void PDF_PLOTTER::Text( const VECTOR2I&             aPos,
                        const COLOR4D&              aColor,
                        const wxString&             aText,
                        const EDA_ANGLE&            aOrient,
                        const VECTOR2I&             aSize,
                        enum GR_TEXT_H_ALIGN_T      aH_justify,
                        enum GR_TEXT_V_ALIGN_T      aV_justify,
                        int                         aWidth,
                        bool                        aItalic,
                        bool                        aBold,
                        bool                        aMultilineAllowed,
                        KIFONT::FONT*               aFont,
                        void*                       aData )
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

    computeTextParameters( aPos, aText, aOrient, t_size, m_plotMirror, aH_justify,
                           aV_justify, aWidth, aItalic, aBold, &wideningFactor, &ctm_a,
                           &ctm_b, &ctm_c, &ctm_d, &ctm_e, &ctm_f, &heightFactor );

    SetColor( aColor );
    SetCurrentLineWidth( aWidth, aData );

    wxStringTokenizer str_tok( aText, " ", wxTOKEN_RET_DELIMS );

    // If aFont is not specilied (== nullptr), use the default kicad stroke font
    if( !aFont )
        aFont = KIFONT::FONT::GetFont();

    VECTOR2I full_box( aFont->StringBoundaryLimits( aText, t_size, aWidth, aBold, aItalic ) );
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

        computeTextParameters( pos, word, aOrient, t_size, m_plotMirror, GR_TEXT_H_ALIGN_LEFT,
                               GR_TEXT_V_ALIGN_BOTTOM, aWidth, aItalic, aBold, &wideningFactor, &ctm_a,
                               &ctm_b, &ctm_c, &ctm_d, &ctm_e, &ctm_f, &heightFactor );

        // Extract the changed width and rotate by the orientation to get the offset for the
        // next word
        VECTOR2I bbox( aFont->StringBoundaryLimits( word, t_size, aWidth, aBold, aItalic ).x, 0 );
        RotatePoint( bbox, aOrient );
        pos += bbox;

        // Don't try to output a blank string
        if( word.Trim( false ).Trim( true ).empty() )
            continue;

        /* We use the full CTM instead of the text matrix because the same
           coordinate system will be used for the overlining. Also the %f
           for the trig part of the matrix to avoid %g going in exponential
           format (which is not supported) */
        fprintf( m_workFile, "q %f %f %f %f %g %g cm BT %s %g Tf %d Tr %g Tz ",
                 ctm_a, ctm_b, ctm_c, ctm_d, ctm_e, ctm_f,
                 fontname, heightFactor, render_mode, wideningFactor * 100 );

        std::string txt_pdf = encodeStringForPlotter( word );
        fprintf( m_workFile, "%s Tj ET\n", txt_pdf.c_str() );
        // Restore the CTM
        fputs( "Q\n", m_workFile );
    }

    // Plot the stroked text (if requested)
    PLOTTER::Text( aPos, aColor, aText, aOrient, aSize, aH_justify, aV_justify, aWidth, aItalic,
                   aBold, aMultilineAllowed, aFont );
}


void PDF_PLOTTER::PlotText( const VECTOR2I& aPos, const COLOR4D& aColor,
                    const wxString& aText,
                    const TEXT_ATTRIBUTES& aAttributes,
                    KIFONT::FONT* aFont,
                    void* aData )
{
    VECTOR2I size = aAttributes.m_Size;

    // PDF files do not like 0 sized texts which create broken files.
    if( size.x == 0 || size.y == 0 )
        return;

    if( aAttributes.m_Mirrored )
        size.x = -size.x;

    PDF_PLOTTER::Text( aPos, aColor, aText, aAttributes.m_Angle, size,
                       aAttributes.m_Halign, aAttributes.m_Valign,
                       aAttributes.m_StrokeWidth,
                       aAttributes.m_Italic, aAttributes.m_Bold,
                       aAttributes.m_Multiline,
                       aFont, aData );
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

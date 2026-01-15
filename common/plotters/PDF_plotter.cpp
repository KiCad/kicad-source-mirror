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
#include <iterator>
#include <cstdio> // snprintf
#include <stack>
#include <ranges>

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
#include <trace_helpers.h>
#include <trigo.h>
#include <string_utils.h>
#include <core/utf8.h>
#include <markup_parser.h>
#include <fmt/format.h>
#include <fmt/chrono.h>
#include <fmt/ranges.h>

#include <plotters/pdf_stroke_font.h>
#include <plotters/pdf_outline_font.h>
#include <plotters/plotters_pslike.h>
#include <geometry/shape_rect.h>
#include <text_eval/text_eval_wrapper.h>


PDF_PLOTTER::~PDF_PLOTTER() = default;

#define GLM_ENABLE_EXPERIMENTAL //for older glm to enable euler angles
#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>

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


std::string PDF_PLOTTER::encodeByteString( const std::string& aBytes )
{
    std::string result;
    result.reserve( aBytes.size() * 4 + 2 );
    result.push_back( '(' );

    for( unsigned char byte : aBytes )
    {
        if( byte == '(' || byte == ')' || byte == '\\' )
        {
            result.push_back( '\\' );
            result.push_back( static_cast<char>( byte ) );
        }
        else if( byte < 32 || byte > 126 )
        {
            fmt::format_to( std::back_inserter( result ), "\\{:03o}", byte );
        }
        else
        {
            result.push_back( static_cast<char>( byte ) );
        }
    }

    result.push_back( ')' );
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


void PDF_PLOTTER::Rect( const VECTOR2I& p1, const VECTOR2I& p2, FILL_T fill, int width,
                        int aCornerRadius )
{
    wxASSERT( m_workFile );

    if( fill == FILL_T::NO_FILL && width == 0 )
        return;

    SetCurrentLineWidth( width );

    if( aCornerRadius > 0 )
    {
        BOX2I box( p1, VECTOR2I( p2.x - p1.x, p2.y - p1.y ) );
        box.Normalize();
        SHAPE_RECT rect( box );
        rect.SetRadius( aCornerRadius );
        PlotPoly( rect.Outline(), fill, width, nullptr );
        return;
    }

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


std::vector<VECTOR2D> PDF_PLOTTER::arcPath( const VECTOR2D& aCenter, const EDA_ANGLE& aStartAngle,
                                            const EDA_ANGLE& aAngle, double aRadius )
{
    std::vector<VECTOR2D> path;

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
    path.emplace_back( userToDeviceCoordinates( start ) );

    for( EDA_ANGLE ii = startAngle + delta; ii < endAngle; ii += delta )
    {
        end.x = KiROUND( aCenter.x + aRadius * ( -ii ).Cos() );
        end.y = KiROUND( aCenter.y + aRadius * ( -ii ).Sin() );
        path.emplace_back( userToDeviceCoordinates( end ) );
    }

    end.x = KiROUND( aCenter.x + aRadius * ( -endAngle ).Cos() );
    end.y = KiROUND( aCenter.y + aRadius * ( -endAngle ).Sin() );
    path.emplace_back( userToDeviceCoordinates( end ) );

    return path;
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

    std::vector<VECTOR2D> path = arcPath( aCenter, aStartAngle, aAngle, aRadius );

    if( path.size() >= 2 )
    {
        fmt::print( m_workFile, "{:g} {:g} m ", path[0].x, path[0].y );

        for( int ii = 1; ii < (int) path.size(); ++ii )
            fmt::print( m_workFile, "{:g} {:g} l ", path[ii].x, path[ii].y );
    }

    // The arc is drawn... if not filled we stroke it, otherwise we finish
    // closing the pie at the center
    if( aFill == FILL_T::NO_FILL )
    {
        fmt::println( m_workFile, "S" );
    }
    else
    {
        VECTOR2D pos_dev = userToDeviceCoordinates( aCenter );
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
    fmt::print( m_workFile, "{:f} {:f} m ", pos.x, pos.y );

    for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
    {
        pos = userToDeviceCoordinates( aCornerList[ii] );
        fmt::print( m_workFile, "{:f} {:f} l ", pos.x, pos.y );
    }

    // Close path and stroke and/or fill
    if( aFill == FILL_T::NO_FILL )
        fmt::println( m_workFile, "S" );
    else if( aWidth == 0 )
        fmt::println( m_workFile, "h f" );
    else
        fmt::println( m_workFile, "b" );
}


void PDF_PLOTTER::PlotPoly( const SHAPE_LINE_CHAIN& aLineChain, FILL_T aFill, int aWidth, void* aData )
{
    SetCurrentLineWidth( aWidth );

    std::set<size_t>      handledArcs;
    std::vector<VECTOR2D> path;

    for( int ii = 0; ii < aLineChain.SegmentCount(); ++ii )
    {
        if( aLineChain.IsArcSegment( ii ) )
        {
            size_t arcIndex = aLineChain.ArcIndex( ii );

            if( !handledArcs.contains( arcIndex ) )
            {
                handledArcs.insert( arcIndex );
                const SHAPE_ARC& arc( aLineChain.Arc( arcIndex ) );
                std::vector<VECTOR2D> arc_path = arcPath( arc.GetCenter(), arc.GetStartAngle(),
                                                          arc.GetCentralAngle(), arc.GetRadius() );

                for( const VECTOR2D& pt : std::ranges::reverse_view( arc_path ) )
                    path.emplace_back( pt );
            }
        }
        else
        {
            const SEG& seg( aLineChain.Segment( ii ) );
            path.emplace_back( userToDeviceCoordinates( seg.A ) );
            path.emplace_back( userToDeviceCoordinates( seg.B ) );
        }
    }

    if( path.size() <= 1 )
        return;

    fmt::print( m_workFile, "{:g} {:g} m ", path[0].x, path[0].y );

    for( int ii = 1; ii < (int) path.size(); ++ii )
        fmt::print( m_workFile, "{:g} {:g} l ", path[ii].x, path[ii].y );

    // Close path and stroke and/or fill
    if( aFill == FILL_T::NO_FILL )
        fmt::println( m_workFile, "S" );
    else if( aWidth == 0 )
        fmt::println( m_workFile, "h f" );
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
    auto findHandleForImage =
            [&]( const wxImage& aCurrImage ) -> int
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
        fmt::print( m_outputFile,
                     "<< /Length {} 0 R >>\nstream\n",
                     m_streamLengthHandle );
    }
    else
    {
        fmt::print( m_outputFile,
                     "<< /Length {} 0 R /Filter /FlateDecode >>\nstream\n",
                     m_streamLengthHandle );
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


    int annotArrayHandle = -1;

    // If we have added any annotation links, create an array containing all the objects
    if( annotHandles.size() > 0 )
    {
        annotArrayHandle = startPdfObject();
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
                "/MediaBox [0 0 {:g} {:g}]\n",
                m_pageTreeHandle,
                m_fontResDictHandle,
                m_imgResDictHandle,
                psPaperSize.x,
                psPaperSize.y );

    if( m_pageStreamHandle != -1 )
        fmt::print( m_outputFile, "/Contents {} 0 R\n", m_pageStreamHandle );

    if( annotHandles.size() > 0 )
        fmt::print( m_outputFile, "/Annots {} 0 R", annotArrayHandle );

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

    if( !m_strokeFontManager )
        m_strokeFontManager = std::make_unique<PDF_STROKE_FONT_MANAGER>();
    else
        m_strokeFontManager->Reset();

    if( !m_outlineFontManager )
        m_outlineFontManager = std::make_unique<PDF_OUTLINE_FONT_MANAGER>();
    else
        m_outlineFontManager->Reset();

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

void PDF_PLOTTER::emitStrokeFonts()
{
    if( !m_strokeFontManager )
        return;

    for( PDF_STROKE_FONT_SUBSET* subsetPtr : m_strokeFontManager->AllSubsets() )
    {
        PDF_STROKE_FONT_SUBSET& subset = *subsetPtr;

        if( subset.GlyphCount() <= 1 )
        {
            subset.SetCharProcsHandle( -1 );
            subset.SetFontHandle( -1 );
            subset.SetToUnicodeHandle( -1 );
            continue;
        }

        for( PDF_STROKE_FONT_SUBSET::GLYPH& glyph : subset.Glyphs() )
        {
            int charProcHandle = startPdfStream();

            if( !glyph.m_stream.empty() )
                fmt::print( m_workFile, "{}\n", glyph.m_stream );

            closePdfStream();
            glyph.m_charProcHandle = charProcHandle;
        }

        int charProcDictHandle = startPdfObject();
        fmt::println( m_outputFile, "<<" );

        for( const PDF_STROKE_FONT_SUBSET::GLYPH& glyph : subset.Glyphs() )
            fmt::println( m_outputFile, "    /{} {} 0 R", glyph.m_name, glyph.m_charProcHandle );

        fmt::println( m_outputFile, ">>" );
        closePdfObject();
        subset.SetCharProcsHandle( charProcDictHandle );

        int toUnicodeHandle = startPdfStream();
        std::string cmap = subset.BuildToUnicodeCMap();

        if( !cmap.empty() )
            fmt::print( m_workFile, "{}", cmap );

        closePdfStream();
        subset.SetToUnicodeHandle( toUnicodeHandle );

        double fontMatrixScale = 1.0 / subset.UnitsPerEm();
        double minX = subset.FontBBoxMinX();
        double minY = subset.FontBBoxMinY();
        double maxX = subset.FontBBoxMaxX();
        double maxY = subset.FontBBoxMaxY();

        int fontHandle = startPdfObject();
        fmt::print( m_outputFile,
                    "<<\n/Type /Font\n/Subtype /Type3\n/Name {}\n/FontBBox [ {:g} {:g} {:g} {:g} ]\n",
                    subset.ResourceName(),
                    minX,
                    minY,
                    maxX,
                    maxY );
        fmt::print( m_outputFile,
                    "/FontMatrix [ {:g} 0 0 {:g} 0 0 ]\n/CharProcs {} 0 R\n",
                    fontMatrixScale,
                    fontMatrixScale,
                    subset.CharProcsHandle() );
        fmt::print( m_outputFile,
                    "/Encoding << /Type /Encoding /Differences {} >>\n",
                    subset.BuildDifferencesArray() );
        fmt::print( m_outputFile,
                    "/FirstChar {}\n/LastChar {}\n/Widths {}\n",
                    subset.FirstChar(),
                    subset.LastChar(),
                    subset.BuildWidthsArray() );
        fmt::print( m_outputFile,
                    "/ToUnicode {} 0 R\n/Resources << /ProcSet [/PDF /Text] >>\n>>\n",
                    subset.ToUnicodeHandle() );
        closePdfObject();
        subset.SetFontHandle( fontHandle );
    }
}


void PDF_PLOTTER::emitOutlineFonts()
{
    if( !m_outlineFontManager )
        return;

    for( PDF_OUTLINE_FONT_SUBSET* subsetPtr : m_outlineFontManager->AllSubsets() )
    {
        if( !subsetPtr || !subsetPtr->HasGlyphs() )
            continue;

        const std::vector<uint8_t>& fontData = subsetPtr->FontFileData();

        if( fontData.empty() )
            continue;

        int fontFileHandle = startPdfStream();
        subsetPtr->SetFontFileHandle( fontFileHandle );

        if( !fontData.empty() )
            fwrite( fontData.data(), fontData.size(), 1, m_workFile );

        closePdfStream();

        std::string cidMap = subsetPtr->BuildCIDToGIDStream();
        int cidMapHandle = startPdfStream();
        subsetPtr->SetCIDMapHandle( cidMapHandle );

        if( !cidMap.empty() )
            fwrite( cidMap.data(), cidMap.size(), 1, m_workFile );

        closePdfStream();

        std::string toUnicode = subsetPtr->BuildToUnicodeCMap();
        int toUnicodeHandle = startPdfStream();
        subsetPtr->SetToUnicodeHandle( toUnicodeHandle );

        if( !toUnicode.empty() )
            fmt::print( m_workFile, "{}", toUnicode );

        closePdfStream();

        int descriptorHandle = startPdfObject();
        subsetPtr->SetFontDescriptorHandle( descriptorHandle );

        fmt::print( m_outputFile,
                    "<<\n/Type /FontDescriptor\n/FontName /{}\n/Flags {}\n/ItalicAngle {:g}\n/Ascent {:g}\n/Descent {:g}\n"
                    "/CapHeight {:g}\n/StemV {:g}\n/FontBBox [ {:g} {:g} {:g} {:g} ]\n/FontFile2 {} 0 R\n>>\n",
                    subsetPtr->BaseFontName(),
                    subsetPtr->Flags(),
                    subsetPtr->ItalicAngle(),
                    subsetPtr->Ascent(),
                    subsetPtr->Descent(),
                    subsetPtr->CapHeight(),
                    subsetPtr->StemV(),
                    subsetPtr->BBoxMinX(),
                    subsetPtr->BBoxMinY(),
                    subsetPtr->BBoxMaxX(),
                    subsetPtr->BBoxMaxY(),
                    subsetPtr->FontFileHandle() );
        closePdfObject();

        int cidFontHandle = startPdfObject();
        subsetPtr->SetCIDFontHandle( cidFontHandle );

        std::string widths = subsetPtr->BuildWidthsArray();

        fmt::print( m_outputFile,
                    "<<\n/Type /Font\n/Subtype /CIDFontType2\n/BaseFont /{}\n"
                    "/CIDSystemInfo << /Registry (Adobe) /Ordering (Identity) /Supplement 0 >>\n"
                    "/FontDescriptor {} 0 R\n/W {}\n/CIDToGIDMap {} 0 R\n>>\n",
                    subsetPtr->BaseFontName(),
                    subsetPtr->FontDescriptorHandle(),
                    widths,
                    subsetPtr->CIDMapHandle() );
        closePdfObject();

        int fontHandle = startPdfObject();
        subsetPtr->SetFontHandle( fontHandle );

        fmt::print( m_outputFile,
                    "<<\n/Type /Font\n/Subtype /Type0\n/BaseFont /{}\n/Encoding /Identity-H\n"
                    "/DescendantFonts [ {} 0 R ]\n/ToUnicode {} 0 R\n>>\n",
                    subsetPtr->BaseFontName(),
                    subsetPtr->CIDFontHandle(),
                    subsetPtr->ToUnicodeHandle() );
        closePdfObject();
    }
}


void PDF_PLOTTER::endPlotEmitResources()
{
    emitOutlineFonts();
    emitStrokeFonts();

    startPdfObject( m_fontResDictHandle );
    fmt::println( m_outputFile, "<<" );

    if( m_outlineFontManager )
    {
        for( PDF_OUTLINE_FONT_SUBSET* subsetPtr : m_outlineFontManager->AllSubsets() )
        {
            if( subsetPtr && subsetPtr->FontHandle() >= 0 )
                fmt::println( m_outputFile, "    {} {} 0 R", subsetPtr->ResourceName(), subsetPtr->FontHandle() );
        }
    }

    if( m_strokeFontManager )
    {
        for( PDF_STROKE_FONT_SUBSET* subsetPtr : m_strokeFontManager->AllSubsets() )
        {
            const PDF_STROKE_FONT_SUBSET& subset = *subsetPtr;
            if( subset.FontHandle() >= 0 )
                fmt::println( m_outputFile, "    {} {} 0 R", subset.ResourceName(), subset.FontHandle() );
        }
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

    std::time_t time = std::time( nullptr );
    std::tm tm{};
    #if defined( _WIN32 ) || defined( _MSC_VER )
    localtime_s( &tm, &time );
    #else
    localtime_r( &time, &tm );
    #endif
    std::string dt = fmt::format( "D:{:%Y:%m:%d:%H:%M:%S}", tm );

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

    wxString text( aText );

    if( text.Contains( wxS( "@{" ) ) )
    {
        EXPRESSION_EVALUATOR evaluator;
        text = evaluator.Evaluate( text );
    }

    SetColor( aColor );
    SetCurrentLineWidth( aWidth, aData );

    if( !aFont )
        aFont = KIFONT::FONT::GetFont( m_renderSettings->GetDefaultFont() );

    VECTOR2I t_size( std::abs( aSize.x ), std::abs( aSize.y ) );
    bool     textMirrored = aSize.x < 0;

    // Parse the text for markup
    // IMPORTANT: Use explicit UTF-8 encoding. wxString::ToStdString() is locale-dependent
    // and under C/POSIX locale can drop or mangle non-ASCII, leading to missing CMaps.
    // The markup parser expects UTF-8 bytes.
    UTF8 utf8Text( text );
    MARKUP::MARKUP_PARSER markupParser( utf8Text.substr() );
    std::unique_ptr<MARKUP::NODE> markupTree( markupParser.Parse() );

    if( !markupTree )
    {
        wxLogTrace( tracePdfPlotter, "PDF_PLOTTER::Text: Markup parsing failed, falling back to plain text." );
        // Fallback to simple text rendering if parsing fails
        wxStringTokenizer str_tok( text, " ", wxTOKEN_RET_DELIMS );
        VECTOR2I pos = aPos;

        while( str_tok.HasMoreTokens() )
        {
            wxString word = str_tok.GetNextToken();
            pos = renderWord( word, pos, t_size, aOrient, textMirrored, aWidth, aBold, aItalic, aFont,
                              aFontMetrics, aV_justify, 0 );
        }
        return;
    }

    // Calculate the full text bounding box for alignment
    VECTOR2I full_box( aFont->StringBoundaryLimits( text, t_size, aWidth, aBold, aItalic, aFontMetrics ) );

    if( textMirrored )
        full_box.x *= -1;

    VECTOR2I box_x( full_box.x, 0 );
    VECTOR2I box_y( 0, full_box.y );

    RotatePoint( box_x, aOrient );
    RotatePoint( box_y, aOrient );

    VECTOR2I pos( aPos );
    if( aH_justify == GR_TEXT_H_ALIGN_CENTER )
        pos -= box_x / 2;
    else if( aH_justify == GR_TEXT_H_ALIGN_RIGHT )
        pos -= box_x;

    if( aV_justify == GR_TEXT_V_ALIGN_CENTER )
        pos += box_y / 2;
    else if( aV_justify == GR_TEXT_V_ALIGN_TOP )
        pos += box_y;

    // Render markup tree
    std::vector<OVERBAR_INFO> overbars;
    renderMarkupNode( markupTree.get(), pos, t_size, aOrient, textMirrored, aWidth,
                      aBold, aItalic, aFont, aFontMetrics, aV_justify, 0, overbars );

    // Draw any overbars that were accumulated
    drawOverbars( overbars, aOrient, aFontMetrics );
}


VECTOR2I PDF_PLOTTER::renderWord( const wxString& aWord, const VECTOR2I& aPosition,
                                  const VECTOR2I& aSize, const EDA_ANGLE& aOrient,
                                  bool aTextMirrored, int aWidth, bool aBold, bool aItalic,
                                  KIFONT::FONT* aFont, const KIFONT::METRICS& aFontMetrics,
                                  enum GR_TEXT_V_ALIGN_T aV_justify, TEXT_STYLE_FLAGS aTextStyle )
{
    if( wxGetEnv( "KICAD_DEBUG_SYN_STYLE", nullptr ) )
    {
        int styleFlags = 0;
        if( aFont->IsOutline() )
            styleFlags = static_cast<KIFONT::OUTLINE_FONT*>( aFont )->GetFace() ? static_cast<KIFONT::OUTLINE_FONT*>( aFont )->GetFace()->style_flags : 0;
        wxLogTrace( tracePdfPlotter, "renderWord enter word='%s' bold=%d italic=%d textStyle=%u styleFlags=%d", TO_UTF8( aWord ), (int) aBold, (int) aItalic, (unsigned) aTextStyle, styleFlags );
    }

    // Don't try to output a blank string, but handle space characters for word separation
    if( aWord.empty() )
        return aPosition;

    // If the word is just a space character, advance position by space width and continue
    if( aWord == wxT( " " ) )
    {
        // Calculate space width and advance position
        VECTOR2I spaceBox( aFont->StringBoundaryLimits( "n", aSize, aWidth, aBold, aItalic,
                                                        aFontMetrics ).x / 2, 0 );

        if( aTextMirrored )
            spaceBox.x *= -1;

        VECTOR2I rotatedSpaceBox = spaceBox;
        RotatePoint( rotatedSpaceBox, aOrient );
        return aPosition + rotatedSpaceBox;
    }

    // If the word contains tab characters, we need to handle them specially.
    // Split by tabs and render each segment, advancing to the next tab stop for each tab.
    if( aWord.Contains( wxT( '\t' ) ) )
    {
        constexpr double TAB_WIDTH = 4 * 0.6;

        VECTOR2I pos = aPosition;
        wxString segment;

        for( wxUniChar c : aWord )
        {
            if( c == '\t' )
            {
                if( !segment.IsEmpty() )
                {
                    pos = renderWord( segment, pos, aSize, aOrient, aTextMirrored, aWidth, aBold,
                                      aItalic, aFont, aFontMetrics, aV_justify, aTextStyle );
                    segment.clear();
                }

                int tabWidth = KiROUND( aSize.x * TAB_WIDTH );
                int currentIntrusion = ( pos.x - aPosition.x ) % tabWidth;
                VECTOR2I tabAdvance( tabWidth - currentIntrusion, 0 );

                if( aTextMirrored )
                    tabAdvance.x *= -1;

                RotatePoint( tabAdvance, aOrient );
                pos += tabAdvance;
            }
            else
            {
                segment += c;
            }
        }

        if( !segment.IsEmpty() )
        {
            pos = renderWord( segment, pos, aSize, aOrient, aTextMirrored, aWidth, aBold, aItalic,
                              aFont, aFontMetrics, aV_justify, aTextStyle );
        }

        return pos;
    }

    // Compute transformation parameters for this word
    double ctm_a, ctm_b, ctm_c, ctm_d, ctm_e, ctm_f;
    double wideningFactor, heightFactor;

    computeTextParameters( aPosition, aWord, aOrient, aSize, aTextMirrored, GR_TEXT_H_ALIGN_LEFT,
                           GR_TEXT_V_ALIGN_BOTTOM, aWidth, aItalic, aBold, &wideningFactor,
                           &ctm_a, &ctm_b, &ctm_c, &ctm_d, &ctm_e, &ctm_f, &heightFactor );

    // Calculate next position for word spacing
    VECTOR2I bbox( aFont->StringBoundaryLimits( aWord, aSize, aWidth, aBold, aItalic, aFontMetrics ).x, 0 );

    if( aTextMirrored )
        bbox.x *= -1;

    RotatePoint( bbox, aOrient );
    VECTOR2I nextPos = aPosition + bbox;

    // Apply vertical offset for subscript/superscript
    // Stroke font positioning (baseline) already correct per user feedback.
    // Outline fonts need: superscript +1 full font height higher; subscript +1 full font height higher
    if( aTextStyle & TEXT_STYLE::SUPERSCRIPT )
    {
        double factor = aFont->IsOutline() ? 0.050 : 0.030; // stroke original ~0.40, outline needs +1.0
        VECTOR2I offset( 0, static_cast<int>( std::lround( aSize.y * factor ) ) );
        RotatePoint( offset, aOrient );
        ctm_e -= offset.x;
        ctm_f += offset.y; // Note: PDF Y increases upward
    }
    else if( aTextStyle & TEXT_STYLE::SUBSCRIPT )
    {
        // For outline fonts raise by one font height versus stroke (which shifts downward slightly)
        VECTOR2I offset( 0, 0 );

        if( aFont->IsStroke() )
            offset.y = static_cast<int>( std::lround( aSize.y * 0.01 ) );

        RotatePoint( offset, aOrient );
        ctm_e += offset.x;
        ctm_f -= offset.y; // Note: PDF Y increases upward
    }

    // Render the word using existing outline font logic
    if( aFont->IsOutline() )
    {
        std::vector<PDF_OUTLINE_FONT_RUN> outlineRuns;

        if( m_outlineFontManager )
        {
            m_outlineFontManager->EncodeString( aWord, static_cast<KIFONT::OUTLINE_FONT*>( aFont ),
                                                ( aItalic || ( aTextStyle & TEXT_STYLE::ITALIC ) ),
                                                ( aBold || ( aTextStyle & TEXT_STYLE::BOLD ) ),
                                                &outlineRuns );
        }

        if( !outlineRuns.empty() )
        {
            // Apply baseline adjustment (keeping existing logic)
            double baseline_factor = 0.17;
            double alignment_multiplier = 1.0;

            if( aV_justify == GR_TEXT_V_ALIGN_CENTER )
                alignment_multiplier = 2.0;
            else if( aV_justify == GR_TEXT_V_ALIGN_TOP )
                alignment_multiplier = 4.0;

            VECTOR2D font_size_dev = userToDeviceSize( aSize );
            double baseline_adjustment = font_size_dev.y * baseline_factor * alignment_multiplier;

            double adjusted_ctm_e = ctm_e;
            double adjusted_ctm_f = ctm_f;

            double angle_rad = aOrient.AsRadians();
            double cos_angle = cos( angle_rad );
            double sin_angle = sin( angle_rad );

            adjusted_ctm_e = ctm_e - baseline_adjustment * sin_angle;
            adjusted_ctm_f = ctm_f + baseline_adjustment * cos_angle;

            double adj_c = ctm_c;
            double adj_d = ctm_d;

            // Synthetic italic (shear) for outline font if requested but font not intrinsically italic
            bool syntheticItalicApplied = false;
            double appliedTilt = 0.0;
            double syn_c = adj_c;
            double syn_d = adj_d;
            double syn_a = ctm_a;
            double syn_b = ctm_b;
            bool wantItalic = ( aItalic || ( aTextStyle & TEXT_STYLE::ITALIC ) );

            if( std::getenv( "KICAD_FORCE_SYN_ITALIC" ) )
                wantItalic = true; // debug: ensure path triggers when forcing synthetic italic

            bool wantBold   = ( aBold || ( aTextStyle & TEXT_STYLE::BOLD ) );
            bool fontIsItalic = aFont->IsItalic();
            bool fontIsBold   = aFont->IsBold();
            bool fontIsFakeItalic = static_cast<KIFONT::OUTLINE_FONT*>( aFont )->IsFakeItalic();
            bool fontIsFakeBold   = static_cast<KIFONT::OUTLINE_FONT*>( aFont )->IsFakeBold();

            // Environment overrides for testing synthetic italics:
            //   KICAD_FORCE_SYN_ITALIC=1 forces synthetic shear even if font has italic face
            //   KICAD_SYN_ITALIC_TILT=<float degrees or tangent?>: if value contains 'deg' treat as degrees,
            //   otherwise treat as raw tilt factor (x += tilt*y)
            bool forceSynItalic = false;
            double overrideTilt = 0.0;

            if( const char* envForce = std::getenv( "KICAD_FORCE_SYN_ITALIC" ) )
            {
                if( *envForce != '\0' && *envForce != '0' )
                    forceSynItalic = true;
            }

            if( const char* envTilt = std::getenv( "KICAD_SYN_ITALIC_TILT" ) )
            {
                std::string tiltStr( envTilt );

                try
                {
                    if( tiltStr.find( "deg" ) != std::string::npos )
                    {
                        double deg = std::stod( tiltStr );
                        overrideTilt = tan( deg * M_PI / 180.0 );
                    }
                    else
                    {
                        overrideTilt = std::stod( tiltStr );
                    }
                }
                catch( ... )
                {
                    overrideTilt = 0.0; // ignore malformed
                }
            }

            // Trace after we know style flags
            wxLogTrace( tracePdfPlotter,
                        "Outline path word='%s' runs=%zu wantItalic=%d fontIsItalic=%d fontIsFakeItalic=%d wantBold=%d fontIsBold=%d fontIsFakeBold=%d forceSyn=%d",
                        TO_UTF8( aWord ), outlineRuns.size(), (int) wantItalic, (int) fontIsItalic,
                        (int) fontIsFakeItalic, (int) wantBold, (int) fontIsBold, (int) fontIsFakeBold,
                        (int) forceSynItalic );

            // Apply synthetic italic if:
            //  - Italic requested AND outline font
            //  - And either forceSynItalic env var set OR there is no REAL italic face.
            //    (A fake italic flag from fontconfig substitution should NOT block synthetic shear.)
            bool realItalicFace = fontIsItalic && !fontIsFakeItalic;

            if( wantItalic && ( forceSynItalic || !realItalicFace ) )
            {
                // We want to apply a horizontal shear so that x' = x + tilt * y in the glyph's
                // local coordinate system BEFORE rotation.  The existing text matrix columns are:
                //   first column  = (a, b)^T  -> x-axis direction & scale
                //   second column = (c, d)^T  -> y-axis direction & scale
                // Prepending a shear matrix S = [[1 tilt][0 1]] (i.e. T' = T * S is WRONG here).
                // We need to LEFT-multiply: T' = R * S where R is the original rotation/scale.
                // Left multiplication keeps first column unchanged and adds (tilt * firstColumn)
                // to the second column: (c', d') = (c + tilt * a, d + tilt * b).
                // This produces a right-leaning italic for positive tilt.
                double tilt = ( overrideTilt != 0.0 ) ? overrideTilt : ITALIC_TILT;

                if( wideningFactor < 0 )       // mirrored text should mirror the shear
                    tilt = -tilt;

                syn_c = adj_c + tilt * syn_a;
                syn_d = adj_d + tilt * syn_b;
                appliedTilt = tilt;
                syntheticItalicApplied = true;

                wxLogTrace( tracePdfPlotter, "Synthetic italic shear applied: tilt=%f a=%f b=%f c->%f d->%f",
                            tilt, syn_a, syn_b, syn_c, syn_d );
            }

            if( wantBold && !fontIsBold )
            {
                // Slight horizontal widening to simulate bold (~3%)
                syn_a *= 1.03;
                syn_b *= 1.03;
            }

            if( syntheticItalicApplied )
            {
                // PDF comment to allow manual inspection in the output stream
                fmt::print( m_workFile, "% syn-italic tilt={} a={} b={} c={} d={}\n",
                            appliedTilt, syn_a, syn_b, syn_c, syn_d );
            }

            fmt::print( m_workFile, "q {:f} {:f} {:f} {:f} {:f} {:f} cm BT {} Tr {:g} Tz ",
                        syn_a, syn_b, syn_c, syn_d, adjusted_ctm_e, adjusted_ctm_f,
                        0, // render_mode
                        wideningFactor * 100 );

            for( const PDF_OUTLINE_FONT_RUN& run : outlineRuns )
            {
                fmt::print( m_workFile, "{} {:g} Tf <", run.m_subset->ResourceName(), heightFactor );

                for( const PDF_OUTLINE_FONT_GLYPH& glyph : run.m_glyphs )
                {
                    fmt::print( m_workFile, "{:02X}{:02X}",
                                static_cast<unsigned char>( ( glyph.cid >> 8 ) & 0xFF ),
                                static_cast<unsigned char>( glyph.cid & 0xFF ) );
                }

                fmt::print( m_workFile, "> Tj " );
            }

            fmt::println( m_workFile, "ET" );
            fmt::println( m_workFile, "Q" );
        }
    }
    else
    {
        // Handle stroke fonts
        if( !m_strokeFontManager )
            return nextPos;

        wxLogTrace( tracePdfPlotter, "Stroke path word='%s' wantItalic=%d aItalic=%d aBold=%d",
                    TO_UTF8( aWord ), (int) ( aItalic || ( aTextStyle & TEXT_STYLE::ITALIC ) ), (int) aItalic, (int) aBold );

        std::vector<PDF_STROKE_FONT_RUN> runs;
        m_strokeFontManager->EncodeString( aWord, &runs, aBold, aItalic );

        if( !runs.empty() )
        {
            VECTOR2D dev_size = userToDeviceSize( aSize );
            double fontSize = dev_size.y;

            double adj_c = ctm_c;
            double adj_d = ctm_d;

            if( aItalic )
            {
                double tilt = -ITALIC_TILT;

                if( wideningFactor < 0 )
                    tilt = -tilt;

                adj_c -= ctm_a * tilt;
                adj_d -= ctm_b * tilt;
            }

            fmt::print( m_workFile, "q {:f} {:f} {:f} {:f} {:f} {:f} cm BT {} Tr {:g} Tz ",
                        ctm_a, ctm_b, adj_c, adj_d, ctm_e, ctm_f,
                        0, // render_mode
                        wideningFactor * 100 );

            for( const PDF_STROKE_FONT_RUN& run : runs )
            {
                fmt::print( m_workFile, "{} {:g} Tf {} Tj ",
                            run.m_subset->ResourceName(),
                            fontSize,
                            encodeByteString( run.m_bytes ) );
            }

            fmt::println( m_workFile, "ET" );
            fmt::println( m_workFile, "Q" );
        }
    }

    return nextPos;
}


VECTOR2I PDF_PLOTTER::renderMarkupNode( const MARKUP::NODE* aNode, const VECTOR2I& aPosition,
                                         const VECTOR2I& aBaseSize, const EDA_ANGLE& aOrient,
                                         bool aTextMirrored, int aWidth, bool aBaseBold, bool aBaseItalic,
                                         KIFONT::FONT* aFont, const KIFONT::METRICS& aFontMetrics,
                                         enum GR_TEXT_V_ALIGN_T aV_justify, TEXT_STYLE_FLAGS aTextStyle,
                                         std::vector<OVERBAR_INFO>& aOverbars )
{
    VECTOR2I nextPosition = aPosition;

    if( !aNode )
        return nextPosition;

    TEXT_STYLE_FLAGS currentStyle = aTextStyle;
    VECTOR2I currentSize = aBaseSize;
    bool drawOverbar = false;

    // Handle markup node types
    if( !aNode->is_root() )
    {
        if( aNode->isSubscript() )
        {
            currentStyle |= TEXT_STYLE::SUBSCRIPT;
            // Subscript: smaller size and lower position
            currentSize = VECTOR2I( aBaseSize.x * 0.5, aBaseSize.y * 0.6 );
        }
        else if( aNode->isSuperscript() )
        {
            currentStyle |= TEXT_STYLE::SUPERSCRIPT;
            // Superscript: smaller size and higher position
            currentSize = VECTOR2I( aBaseSize.x * 0.5, aBaseSize.y * 0.6 );
        }

        if( aNode->isOverbar() )
        {
            drawOverbar = true;
            // Overbar doesn't change font size, just adds decoration
        }

        // Render content of this node if it has text
        if( aNode->has_content() )
        {
            wxString nodeText = aNode->asWxString();

            // Process text content (simplified version of the main text processing)
            wxStringTokenizer str_tok( nodeText, " ", wxTOKEN_RET_DELIMS );

            while( str_tok.HasMoreTokens() )
            {
                wxString word = str_tok.GetNextToken();
                nextPosition = renderWord( word, nextPosition, currentSize, aOrient, aTextMirrored,
                                           aWidth, aBaseBold || (currentStyle & TEXT_STYLE::BOLD),
                                           aBaseItalic || (currentStyle & TEXT_STYLE::ITALIC),
                                           aFont, aFontMetrics, aV_justify, currentStyle );
            }
        }
    }

    // Process child nodes recursively
    for( const std::unique_ptr<MARKUP::NODE>& child : aNode->children )
    {
        VECTOR2I startPos = nextPosition;

        nextPosition = renderMarkupNode( child.get(), nextPosition, currentSize, aOrient,
                                         aTextMirrored, aWidth, aBaseBold, aBaseItalic,
                                         aFont, aFontMetrics, aV_justify, currentStyle, aOverbars );

        // Store overbar info for later rendering
        if( drawOverbar )
        {
            VECTOR2I endPos = nextPosition;
            aOverbars.push_back( { startPos, endPos, currentSize, aFont->IsOutline(), aV_justify } );
        }
    }

    return nextPosition;
}


void PDF_PLOTTER::drawOverbars( const std::vector<OVERBAR_INFO>& aOverbars, const EDA_ANGLE& aOrient,
                                const KIFONT::METRICS& aFontMetrics )
{
    for( const OVERBAR_INFO& overbar : aOverbars )
    {
        // Baseline direction (vector from start to end). If zero length, derive from orientation.
        VECTOR2D dir( overbar.endPos.x - overbar.startPos.x, overbar.endPos.y - overbar.startPos.y );

        double len = hypot( dir.x, dir.y );
        if( len <= 1e-6 )
        {
            // Fallback: derive direction from orientation angle
            double ang = aOrient.AsRadians();
            dir.x = cos( ang );
            dir.y = sin( ang );
            len = 1.0;
        }

        dir.x /= len;
        dir.y /= len;

        // Perpendicular (rotate dir 90° CCW). Upward in text space so overbar sits above baseline.
        VECTOR2D nrm( -dir.y, dir.x );

        // Base vertical offset distance in device units (baseline -> default overbar position)
        double barOffset = aFontMetrics.GetOverbarVerticalPosition( overbar.fontSize.y );

        // Adjust further to match screen drawing.  This is somewhat disturbing, but I can't figure
        // out why it's needed.
        if( overbar.isOutline )
            barOffset += overbar.fontSize.y * 0.16;
        else
            barOffset += overbar.fontSize.y * 0.32;

        // Mirror the text vertical alignment adjustments used for baseline shifting.
        // Earlier logic scales baseline adjustment: CENTER ~2x, TOP ~4x. We apply proportional
        // extra raise so that overbars track visually with perceived baseline shift.
        double alignMult = 1.0;

        switch( overbar.vAlign )
        {
        case GR_TEXT_V_ALIGN_CENTER: alignMult = overbar.isOutline ? 2.0 : 1.0; break;
        case GR_TEXT_V_ALIGN_TOP:    alignMult = overbar.isOutline ? 4.0 : 1.0; break;
        default:                     alignMult = 1.0; break; // bottom
        }

        if( alignMult > 1.0 )
        {
            // Scale only the baseline component (approx 17% of height, matching earlier baseline_factor)
            double baseline_factor = 0.17;
            barOffset += ( alignMult - 1.0 ) * ( baseline_factor * overbar.fontSize.y );
        }

        // Trim to avoid rounded cap extension (assumes stroke caps); proportion of font width.
        double barTrim = overbar.fontSize.x * 0.1;

        // Apply trim along baseline direction and offset along normal
        VECTOR2D startPt( overbar.startPos.x, overbar.startPos.y );
        VECTOR2D endPt( overbar.endPos.x, overbar.endPos.y );

        // Both endpoints should share identical vertical (normal) offset above baseline.
        // Use a single offset vector offVec = -barOffset * nrm (negative because nrm points 'up').
        VECTOR2D offVec( -barOffset * nrm.x, -barOffset * nrm.y );

        startPt.x += dir.x * barTrim + offVec.x;
        startPt.y += dir.y * barTrim + offVec.y;
        endPt.x -= dir.x * barTrim - offVec.x; // subtract trim, then apply same vertical offset
        endPt.y -= dir.y * barTrim - offVec.y;

        VECTOR2I iStart = KiROUND( startPt.x, startPt.y );
        VECTOR2I iEnd = KiROUND( endPt.x, endPt.y );

        MoveTo( iStart );
        LineTo( iEnd );
        PenFinish();
    }
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
        // this is a strict need
        wxASSERT( view.m_cameraMatrix.size() == 12 );

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


std::vector<float> PDF_PLOTTER::CreateC2WMatrixFromAngles( const VECTOR3D& aTargetPosition,
                                                            float aCameraDistance,
                                                            float aYawDegrees,
                                                            float aPitchDegrees,
                                                            float aRollDegrees )
{
    float yRadians = glm::radians( aYawDegrees );
    float xRadians = glm::radians( aPitchDegrees );
    float zRadians = glm::radians( aRollDegrees );

    // Create rotation matrix from Euler angles
    glm::mat4 rotationMatrix = glm::eulerAngleYXZ( yRadians, xRadians, zRadians );

    // Calculate camera position based on target, distance, and rotation
    // Start with a vector pointing backward along the z-axis
    glm::vec4 cameraOffset = glm::vec4( 0.0f, 0.0f, aCameraDistance, 1.0f );

    // Apply rotation to this offset
    cameraOffset = rotationMatrix * cameraOffset;

    // Camera position is target position minus the rotated offset
    glm::vec3 cameraPosition = glm::vec3(aTargetPosition.x, aTargetPosition.y, aTargetPosition.z)
        - glm::vec3( cameraOffset );

    std::vector<float> result( 12 );

    // Handle rotation part in column-major order (first 9 elements)
    int index = 0;
    for( int col = 0; col < 3; ++col )
    {
        for( int row = 0; row < 3; ++row )
        {
            result[index++] = static_cast<float>( rotationMatrix[col][row] );
        }
    }

    // Handle translation part (last 3 elements)
    result[9] = static_cast<float>( cameraPosition.x );
    result[10] = static_cast<float>( cameraPosition.y );
    result[11] = static_cast<float>( cameraPosition.z );

    return result;
}
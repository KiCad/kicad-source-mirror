/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

/**
 * @file common_plotPS_functions.cpp
 * @brief Kicad: Common plot Postscript Routines
 */

#include <fctsys.h>
#include <trigo.h>
#include <wxstruct.h>
#include <base_struct.h>
#include <common.h>
#include <plot_common.h>
#include <macros.h>
#include <kicad_string.h>

/* Forward declaration of the font width metrics
   (yes extern! this is the way to forward declare variables */
extern const double hv_widths[256];
extern const double hvb_widths[256];
extern const double hvo_widths[256];
extern const double hvbo_widths[256];

const double PSLIKE_PLOTTER::postscriptTextAscent = 0.718;


// Common routines for Postscript-like plotting engines

void PSLIKE_PLOTTER::SetDefaultLineWidth( int width )
{
    defaultPenWidth = width;
    currentPenWidth = -1;
}


void PSLIKE_PLOTTER::SetColor( EDA_COLOR_T color )
{
    // Return at invalid color index
    if( color < 0 )
        return;

    if( colorMode )
    {
        double r = g_ColorRefs[color].m_Red / 255.0;
        double g = g_ColorRefs[color].m_Green / 255.0;
        double b = g_ColorRefs[color].m_Blue / 255.0;
        if( negativeMode )
            emitSetRGBColor( 1 - r, 1 - g, 1 - b );
        else
            emitSetRGBColor( r, g, b );
    }
    else
    {
        /* B/W Mode - Use BLACK or WHITE for all items
         * note the 2 colors are used in B&W mode, mainly by Pcbnew to draw
         * holes in white on pads in black
         */
        double k = 1; // White
        if( color != WHITE )
            k = 0;
        if( negativeMode )
            emitSetRGBColor( 1 - k, 1 - k, 1 - k );
        else
            emitSetRGBColor( k, k, k );
    }
}


void PSLIKE_PLOTTER::FlashPadOval( const wxPoint& pos, const wxSize& aSize, double orient,
                                   EDA_DRAW_MODE_T modetrace )
{
    wxASSERT( outputFile );
    int x0, y0, x1, y1, delta;
    wxSize size( aSize );

    // The pad is reduced to an oval by dy > dx
    if( size.x > size.y )
    {
        EXCHG( size.x, size.y );
        orient = AddAngles( orient, 900 );
    }

    delta = size.y - size.x;
    x0    = 0;
    y0    = -delta / 2;
    x1    = 0;
    y1    = delta / 2;
    RotatePoint( &x0, &y0, orient );
    RotatePoint( &x1, &y1, orient );

    if( modetrace == FILLED )
        ThickSegment( wxPoint( pos.x + x0, pos.y + y0 ),
                      wxPoint( pos.x + x1, pos.y + y1 ), size.x, modetrace );
    else
        sketchOval( pos, size, orient, -1 );
}


void PSLIKE_PLOTTER::FlashPadCircle( const wxPoint& pos, int diametre,
                                     EDA_DRAW_MODE_T modetrace )
{
    int current_line_width;
    wxASSERT( outputFile );

    SetCurrentLineWidth( -1 );
    current_line_width = GetCurrentLineWidth();
    if( current_line_width > diametre )
        current_line_width = diametre;

    if( modetrace == FILLED )
        Circle( pos, diametre - currentPenWidth, FILLED_SHAPE, current_line_width );
    else
        Circle( pos, diametre - currentPenWidth, NO_FILL, current_line_width );

    SetCurrentLineWidth( -1 );
}


void PSLIKE_PLOTTER::FlashPadRect( const wxPoint& pos, const wxSize& aSize,
                                   double orient, EDA_DRAW_MODE_T trace_mode )
{
    static std::vector< wxPoint > cornerList;
    wxSize size( aSize );
    cornerList.clear();

    SetCurrentLineWidth( -1 );
    int w = currentPenWidth;
    size.x -= w;
    if( size.x < 1 )
        size.x = 1;
    size.y -= w;
    if( size.y < 1 )
        size.y = 1;

    int dx = size.x / 2;
    int dy = size.y / 2;

    wxPoint corner;
    corner.x = pos.x - dx;
    corner.y = pos.y + dy;
    cornerList.push_back( corner );
    corner.x = pos.x - dx;
    corner.y = pos.y - dy;
    cornerList.push_back( corner );
    corner.x = pos.x + dx;
    corner.y = pos.y - dy;
    cornerList.push_back( corner );
    corner.x = pos.x + dx;
    corner.y = pos.y + dy,
    cornerList.push_back( corner );

    for( unsigned ii = 0; ii < cornerList.size(); ii++ )
    {
        RotatePoint( &cornerList[ii], pos, orient );
    }

    cornerList.push_back( cornerList[0] );

    PlotPoly( cornerList, ( trace_mode == FILLED ) ? FILLED_SHAPE : NO_FILL );
}


void PSLIKE_PLOTTER::FlashPadTrapez( const wxPoint& aPadPos, const wxPoint *aCorners,
                                     double aPadOrient, EDA_DRAW_MODE_T aTrace_Mode )
{
    static std::vector< wxPoint > cornerList;
    cornerList.clear();

    for( int ii = 0; ii < 4; ii++ )
        cornerList.push_back( aCorners[ii] );

    if( aTrace_Mode == FILLED )
    {
        SetCurrentLineWidth( 0 );
    }
    else
    {
        SetCurrentLineWidth( -1 );
        int w = currentPenWidth;
        // offset polygon by w
        // coord[0] is assumed the lower left
        // coord[1] is assumed the upper left
        // coord[2] is assumed the upper right
        // coord[3] is assumed the lower right

        /* Trace the outline. */
        cornerList[0].x += w;
        cornerList[0].y -= w;
        cornerList[1].x += w;
        cornerList[1].y += w;
        cornerList[2].x -= w;
        cornerList[2].y += w;
        cornerList[3].x -= w;
        cornerList[3].y -= w;
    }

    for( int ii = 0; ii < 4; ii++ )
    {
        RotatePoint( &cornerList[ii], aPadOrient );
        cornerList[ii] += aPadPos;
    }

    cornerList.push_back( cornerList[0] );
    PlotPoly( cornerList, ( aTrace_Mode == FILLED ) ? FILLED_SHAPE : NO_FILL );
}


/**
 * Write on a stream a string escaped for postscript/PDF
 */
void PSLIKE_PLOTTER::fputsPostscriptString(FILE *fout, const wxString& txt)
{
    putc( '(', fout );
    for( unsigned i = 0; i < txt.length(); i++ )
    {
        // Lazyness made me use stdio buffering yet another time...
        wchar_t ch = txt[i];

        if( ch < 256 )
        {
            switch (ch)
            {
                // The ~ shouldn't reach the outside
            case '~':
                break;
                // These characters must be escaped
            case '(':
            case ')':
            case '\\':
                putc( '\\', fout );

                // FALLTHRU
            default:
                putc( ch, fout );
                break;
            }
        }
    }

    putc( ')', fout );
}


/**
 * Sister function for the GraphicTextWidth in drawtxt.cpp
 * Does the same processing (i.e. calculates a text string width) but
 * using postscript metrics for the Helvetica font (optionally used for
 * PS and PDF plotting
 */
int PSLIKE_PLOTTER::returnPostscriptTextWidth( const wxString& aText, int aXSize,
                                               bool aItalic, bool aBold )
{
    const double *width_table = aBold ? ( aItalic ? hvbo_widths : hvb_widths )
                                      : ( aItalic ? hvo_widths : hv_widths );
    double tally = 0;

    for( unsigned i = 0; i < aText.length(); i++ )
    {
        wchar_t AsciiCode = aText[i];
        // Skip the negation marks and untabled points
        if( AsciiCode != '~' && AsciiCode < 256 )
        {
            tally += width_table[AsciiCode];
        }
    }

    // Widths are proportional to height, but height is enlarged by a
    // scaling factor
    return KiROUND( aXSize * tally / postscriptTextAscent );
}


/**
 * Computes the x coordinates for the overlining in a string of text.
 * Fills the passed vector with couples of (start, stop) values to be
 * used in the text coordinate system (use computeTextParameters to
 * obtain the parameters to estabilish such a system)
 */
void PSLIKE_PLOTTER::postscriptOverlinePositions( const wxString& aText, int aXSize,
                                                  bool aItalic, bool aBold,
                                                  std::vector<int> *pos_pairs )
{
    /* XXX This function is *too* similar to returnPostscriptTextWidth.
       Consider merging them... */
    const double *width_table = aBold ? ( aItalic ? hvbo_widths : hvb_widths )
                                      : ( aItalic ? hvo_widths : hv_widths );
    double tally = 0;

    for( unsigned i = 0; i < aText.length(); i++ )
    {
        wchar_t AsciiCode = aText[i];
        // Skip the negation marks and untabled points
        if( AsciiCode != '~' && AsciiCode < 256 )
        {
            tally += width_table[AsciiCode];
        }
        else
        {
            if( AsciiCode == '~' )
                pos_pairs->push_back( KiROUND( aXSize * tally / postscriptTextAscent ) );
        }
    }

    // Special rule: we have to complete the last bar if the ~ aren't matched
    if( pos_pairs->size() % 2 == 1 )
        pos_pairs->push_back( KiROUND( aXSize * tally / postscriptTextAscent ) );
}

void PS_PLOTTER::SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                  double aScale, bool aMirror )
{
    wxASSERT( !outputFile );
    m_plotMirror = aMirror;
    plotOffset = aOffset;
    plotScale = aScale;
    m_IUsPerDecimil = aIusPerDecimil;
    iuPerDeviceUnit = 1.0 / aIusPerDecimil;
    /* Compute the paper size in IUs */
    paperSize = pageInfo.GetSizeMils();
    paperSize.x *= 10.0 * aIusPerDecimil;
    paperSize.y *= 10.0 * aIusPerDecimil;
    SetDefaultLineWidth( 100 * aIusPerDecimil );  // arbitrary default
}


/** This is the core for postscript/PDF text alignment
 * It computes the transformation matrix to generate a user space
 * system aligned with the text. Even the PS uses the concat
 * operator to simplify PDF generation (concat is everything PDF
 * has to modify the CTM. Lots of parameters, both in and out.
 */
void PSLIKE_PLOTTER::computeTextParameters( const wxPoint&           aPos,
                                            const wxString&          aText,
                                            int                      aOrient,
                                            const wxSize&            aSize,
                                            enum EDA_TEXT_HJUSTIFY_T aH_justify,
                                            enum EDA_TEXT_VJUSTIFY_T aV_justify,
                                            int                      aWidth,
                                            bool                     aItalic,
                                            bool                     aBold,
                                            double                   *wideningFactor,
                                            double                   *ctm_a,
                                            double                   *ctm_b,
                                            double                   *ctm_c,
                                            double                   *ctm_d,
                                            double                   *ctm_e,
                                            double                   *ctm_f,
                                            double                   *heightFactor )
{
    // Compute the starting position (compensated for alignment)
    wxPoint start_pos = aPos;

    // This is an approximation of the text bounds (in IUs)
    int tw = returnPostscriptTextWidth( aText, aSize.x, aItalic, aWidth );
    int th = aSize.y;
    int dx, dy;

    switch( aH_justify )
    {
    case GR_TEXT_HJUSTIFY_CENTER:
        dx = -tw / 2;
        break;

    case GR_TEXT_HJUSTIFY_RIGHT:
        dx = -tw;
        break;

    case GR_TEXT_HJUSTIFY_LEFT:
        dx = 0;
        break;
    }

    switch( aV_justify )
    {
    case GR_TEXT_VJUSTIFY_CENTER:
        dy = th / 2;
        break;

    case GR_TEXT_VJUSTIFY_TOP:
        dy = th;
        break;

    case GR_TEXT_VJUSTIFY_BOTTOM:
        dy = 0;
        break;
    }

    RotatePoint( &dx, &dy, aOrient );
    RotatePoint( &tw, &th, aOrient );
    start_pos.x += dx;
    start_pos.y += dy;
    DPOINT pos_dev = userToDeviceCoordinates( start_pos );
    DPOINT sz_dev = userToDeviceSize( aSize );

    // Now returns the final values... the widening factor
    *wideningFactor = sz_dev.y / sz_dev.x;

    // The CTM transformation matrix
    double alpha = DECIDEG2RAD( aOrient );
    double sinalpha = sin( alpha );
    double cosalpha = cos( alpha );

    *ctm_a = cosalpha;
    *ctm_b = sinalpha;
    *ctm_c = -sinalpha;
    *ctm_d = cosalpha;
    *ctm_e = pos_dev.x;
    *ctm_f = pos_dev.y;

    // This is because the letters are less than 1 unit high
    *heightFactor = sz_dev.y / postscriptTextAscent;
}


/* Set the current line width (in IUs) for the next plot
 */
void PS_PLOTTER::SetCurrentLineWidth( int width )
{
    wxASSERT( outputFile );
    int pen_width;

    if( width >= 0 )
        pen_width = width;
    else
        pen_width = defaultPenWidth;

    if( pen_width != currentPenWidth )
        fprintf( outputFile, "%g setlinewidth\n", userToDeviceSize( pen_width ) );

    currentPenWidth = pen_width;
}


void PS_PLOTTER::emitSetRGBColor( double r, double g, double b )
{
    wxASSERT( outputFile );

    // XXX why %.3g ? shouldn't %g suffice? who cares...
    fprintf( outputFile, "%.3g %.3g %.3g setrgbcolor\n", r, g, b );
}


/**
 * Postscript supports dashed lines
 */
void PS_PLOTTER::SetDash( bool dashed )
{
    wxASSERT( outputFile );
    if( dashed )
        fputs( "dashedline\n", outputFile );
    else
        fputs( "solidline\n", outputFile );
}


void PS_PLOTTER::Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill, int width )
{
    DPOINT p1_dev = userToDeviceCoordinates( p1 );
    DPOINT p2_dev = userToDeviceCoordinates( p2 );

    SetCurrentLineWidth( width );
    fprintf( outputFile, "%g %g %g %g rect%d\n", p1_dev.x, p1_dev.y,
             p2_dev.x - p1_dev.x, p2_dev.y - p1_dev.y, fill );
}


void PS_PLOTTER::Circle( const wxPoint& pos, int diametre, FILL_T fill, int width )
{
    wxASSERT( outputFile );
    DPOINT pos_dev = userToDeviceCoordinates( pos );
    double radius = userToDeviceSize( diametre / 2.0 );

    SetCurrentLineWidth( width );
    fprintf( outputFile, "%g %g %g cir%d\n", pos_dev.x, pos_dev.y, radius, fill );
}


void PS_PLOTTER::Arc( const wxPoint& centre, double StAngle, double EndAngle,
                      int radius, FILL_T fill, int width )
{
    wxASSERT( outputFile );
    if( radius <= 0 )
        return;

    if( StAngle > EndAngle )
        EXCHG( StAngle, EndAngle );

    SetCurrentLineWidth( width );

    // Calculate start point.
    DPOINT centre_dev = userToDeviceCoordinates( centre );
    double radius_dev = userToDeviceSize( radius );

    if( m_plotMirror )
    {
        if( m_mirrorIsHorizontal )
        {
            StAngle = 1800.0 -StAngle;
            EndAngle = 1800.0 -EndAngle;
            EXCHG( StAngle, EndAngle );
        }
        else
        {
            StAngle = -StAngle;
            EndAngle = -EndAngle;
        }
    }

    fprintf( outputFile, "%g %g %g %g %g arc%d\n", centre_dev.x, centre_dev.y,
             radius_dev, StAngle / 10.0, EndAngle / 10.0, fill );
}


void PS_PLOTTER::PlotPoly( const std::vector< wxPoint >& aCornerList,
                           FILL_T aFill, int aWidth )
{
    if( aCornerList.size() <= 1 )
        return;

    SetCurrentLineWidth( aWidth );

    DPOINT pos = userToDeviceCoordinates( aCornerList[0] );
    fprintf( outputFile, "newpath\n%g %g moveto\n", pos.x, pos.y );

    for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
    {
        pos = userToDeviceCoordinates( aCornerList[ii] );
        fprintf( outputFile, "%g %g lineto\n", pos.x, pos.y );
    }

    // Close/(fill) the path
    fprintf( outputFile, "poly%d\n", aFill );
}


/**
 * Postscript-likes at the moment are the only plot engines supporting bitmaps...
 */
void PS_PLOTTER::PlotImage( const wxImage & aImage, const wxPoint& aPos,
                            double aScaleFactor )
{
    wxSize pix_size;                // size of the bitmap in pixels
    pix_size.x = aImage.GetWidth();
    pix_size.y = aImage.GetHeight();
    DPOINT drawsize( aScaleFactor * pix_size.x,
                     aScaleFactor * pix_size.y ); // requested size of image

    // calculate the bottom left corner position of bitmap
    wxPoint start = aPos;
    start.x -= drawsize.x / 2;    // left
    start.y += drawsize.y / 2;    // bottom (Y axis reversed)

    // calculate the top right corner position of bitmap
    wxPoint end;
    end.x = start.x + drawsize.x;
    end.y = start.y - drawsize.y;

    fprintf( outputFile, "/origstate save def\n" );
    fprintf( outputFile, "/pix %d string def\n", pix_size.x );

    // Locate lower-left corner of image
    DPOINT start_dev = userToDeviceCoordinates( start );
    fprintf( outputFile, "%g %g translate\n", start_dev.x, start_dev.y );
    // Map image size to device
    DPOINT end_dev = userToDeviceCoordinates( end );
    fprintf( outputFile, "%g %g scale\n",
             std::abs(end_dev.x - start_dev.x), std::abs(end_dev.y - start_dev.y));

    // Dimensions of source image (in pixels
    fprintf( outputFile, "%d %d 8", pix_size.x, pix_size.y );
    //  Map unit square to source
    fprintf( outputFile, " [%d 0 0 %d 0 %d]\n", pix_size.x, -pix_size.y , pix_size.y);
    // include image data in ps file
    fprintf( outputFile, "{currentfile pix readhexstring pop}\n" );

    if( colorMode )
        fputs( "false 3 colorimage\n", outputFile );
    else
        fputs( "image\n", outputFile );
    // Single data source, 3 colors, Output RGB data (hexadecimal)
    // (or the same downscaled to gray)
    int jj = 0;

    for( int yy = 0; yy < pix_size.y; yy ++ )
    {
        for( int xx = 0; xx < pix_size.x; xx++, jj++ )
        {
            if( jj >= 16 )
            {
                jj = 0;
                fprintf( outputFile, "\n");
            }

            int red, green, blue;
            red = aImage.GetRed( xx, yy) & 0xFF;
            green = aImage.GetGreen( xx, yy) & 0xFF;
            blue = aImage.GetBlue( xx, yy) & 0xFF;

            if( colorMode )
                fprintf( outputFile, "%2.2X%2.2X%2.2X", red, green, blue );
            else
                fprintf( outputFile, "%2.2X", (red + green + blue) / 3 );
        }
    }

    fprintf( outputFile, "\n");
    fprintf( outputFile, "origstate restore\n" );
}


void PS_PLOTTER::PenTo( const wxPoint& pos, char plume )
{
    wxASSERT( outputFile );

    if( plume == 'Z' )
    {
        if( penState != 'Z' )
        {
            fputs( "stroke\n", outputFile );
            penState     = 'Z';
            penLastpos.x = -1;
            penLastpos.y = -1;
        }

        return;
    }

    if( penState == 'Z' )
    {
        fputs( "newpath\n", outputFile );
    }

    if( penState != plume || pos != penLastpos )
    {
        DPOINT pos_dev = userToDeviceCoordinates( pos );
        fprintf( outputFile, "%g %g %sto\n",
                 pos_dev.x, pos_dev.y,
                 ( plume=='D' ) ? "line" : "move" );
    }

    penState   = plume;
    penLastpos = pos;
}


/**
 * The code within this function (and the CloseFilePS function)
 * creates postscript files whose contents comply with Adobe's
 * Document Structuring Convention, as documented by assorted
 * details described within the following URLs:
 *
 * http://en.wikipedia.org/wiki/Document_Structuring_Conventions
 * http://partners.adobe.com/public/developer/en/ps/5001.DSC_Spec.pdf
 *
 *
 * BBox is the boundary box (position and size of the "client rectangle"
 * for drawings (page - margins) in mils (0.001 inch)
 */
bool PS_PLOTTER::StartPlot()
{
    wxASSERT( outputFile );
    wxString           msg;

    static const char* PSMacro[] =
    {
    "%%BeginProlog\n"
    "/line { newpath moveto lineto stroke } bind def\n",
    "/cir0 { newpath 0 360 arc stroke } bind def\n",
    "/cir1 { newpath 0 360 arc gsave fill grestore stroke } bind def\n",
    "/cir2 { newpath 0 360 arc gsave fill grestore stroke } bind def\n",
    "/arc0 { newpath arc stroke } bind def\n",
    "/arc1 { newpath 4 index 4 index moveto arc closepath gsave fill\n",
    "    grestore stroke } bind def\n",
    "/arc2 { newpath 4 index 4 index moveto arc closepath gsave fill\n",
    "    grestore stroke } bind def\n",
    "/poly0 { stroke } bind def\n",
    "/poly1 { closepath gsave fill grestore stroke } bind def\n",
    "/poly2 { closepath gsave fill grestore stroke } bind def\n",
    "/rect0 { rectstroke } bind def\n",
    "/rect1 { rectfill } bind def\n",
    "/rect2 { rectfill } bind def\n",
    "/linemode0 { 0 setlinecap 0 setlinejoin 0 setlinewidth } bind def\n",
    "/linemode1 { 1 setlinecap 1 setlinejoin } bind def\n",
    "/dashedline { [200] 100 setdash } bind def\n",
    "/solidline { [] 0 setdash } bind def\n",

    // This is for 'hidden' text (search anchors for PDF)
        "/phantomshow { moveto\n",
        "    /KicadFont findfont 0.000001 scalefont setfont\n",
    "    show } bind def\n",

        // This is for regular postscript text
        "/textshow { gsave\n",
        "    findfont exch scalefont setfont concat 1 scale 0 0 moveto show\n",
        "    } bind def\n",

    // Utility for getting Latin1 encoded fonts
    "/reencodefont {\n",
        "  findfont dup length dict begin\n",
        "  { 1 index /FID ne\n",
        "    { def }\n",
        "    { pop pop } ifelse\n",
        "  } forall\n",
        "  /Encoding ISOLatin1Encoding def\n",
        "  currentdict\n",
        "  end } bind def\n"

    // Remap AdobeStandard fonts to Latin1
    "/KicadFont /Helvetica reencodefont definefont pop\n",
    "/KicadFont-Bold /Helvetica-Bold reencodefont definefont pop\n",
    "/KicadFont-Oblique /Helvetica-Oblique reencodefont definefont pop\n",
    "/KicadFont-BoldOblique /Helvetica-BoldOblique reencodefont definefont pop\n",
    "%%EndProlog\n",
    NULL
    };

    time_t time1970 = time( NULL );

    fputs( "%!PS-Adobe-3.0\n", outputFile );    // Print header

    fprintf( outputFile, "%%%%Creator: %s\n", TO_UTF8( creator ) );

    /* A "newline" character ("\n") is not included in the following string,
       because it is provided by the ctime() function. */
    fprintf( outputFile, "%%%%CreationDate: %s", ctime( &time1970 ) );
    fprintf( outputFile, "%%%%Title: %s\n", TO_UTF8( filename ) );
    fprintf( outputFile, "%%%%Pages: 1\n" );
    fprintf( outputFile, "%%%%PageOrder: Ascend\n" );

    // Print boundary box in 1/72 pixels per inch, box is in mils
    const double BIGPTsPERMIL = 0.072;

    /* The coordinates of the lower left corner of the boundary
       box need to be "rounded down", but the coordinates of its
       upper right corner need to be "rounded up" instead. */
    wxSize psPaperSize = pageInfo.GetSizeMils();

    if( !pageInfo.IsPortrait() )
        psPaperSize.Set( pageInfo.GetHeightMils(), pageInfo.GetWidthMils() );

    fprintf( outputFile, "%%%%BoundingBox: 0 0 %d %d\n",
        (int) ceil( psPaperSize.x * BIGPTsPERMIL ),
        (int) ceil( psPaperSize.y * BIGPTsPERMIL ) );

    // Specify the size of the sheet and the name associated with that size.
    // (If the "User size" option has been selected for the sheet size,
    // identify the sheet size as "Custom" (rather than as "User"), but
    // otherwise use the name assigned by KiCad for each sheet size.)
    //
    // (The Document Structuring Convention also supports sheet weight,
    // sheet color, and sheet type properties being specified within a
    // %%DocumentMedia comment, but they are not being specified here;
    // a zero and two null strings are subsequently provided instead.)
    //
    // (NOTE: m_Size.y is *supposed* to be listed before m_Size.x;
    // the order in which they are specified is not wrong!)
    // Also note pageSize is given in mils, not in internal units and must be
    // converted to internal units.

    if( pageInfo.IsCustom() )
        fprintf( outputFile, "%%%%DocumentMedia: Custom %d %d 0 () ()\n",
                 KiROUND( psPaperSize.x * BIGPTsPERMIL ),
                 KiROUND( psPaperSize.y * BIGPTsPERMIL ) );

    else  // a standard paper size
        fprintf( outputFile, "%%%%DocumentMedia: %s %d %d 0 () ()\n",
                 TO_UTF8( pageInfo.GetType() ),
                 KiROUND( psPaperSize.x * BIGPTsPERMIL ),
                 KiROUND( psPaperSize.y * BIGPTsPERMIL ) );

    if( pageInfo.IsPortrait() )
        fprintf( outputFile, "%%%%Orientation: Portrait\n" );
    else
        fprintf( outputFile, "%%%%Orientation: Landscape\n" );

    fprintf( outputFile, "%%%%EndComments\n" );

    // Now specify various other details.

    for( int ii = 0; PSMacro[ii] != NULL; ii++ )
    {
        fputs( PSMacro[ii], outputFile );
    }

    // The following string has been specified here (rather than within
    // PSMacro[]) to highlight that it has been provided to ensure that the
    // contents of the postscript file comply with the details specified
    // within the Document Structuring Convention.
    fputs( "%%Page: 1 1\n"
           "%%BeginPageSetup\n"
       "gsave\n"
       "0.0072 0.0072 scale\n"    // Configure postscript for decimils coordinates
       "linemode1\n", outputFile );


    // Rototranslate the coordinate to achieve the landscape layout
    if( !pageInfo.IsPortrait() )
        fprintf( outputFile, "%d 0 translate 90 rotate\n", 10 * psPaperSize.x );

    // Apply the user fine scale adjustments
    if( plotScaleAdjX != 1.0 || plotScaleAdjY != 1.0 )
        fprintf( outputFile, "%g %g scale\n",
                 plotScaleAdjX, plotScaleAdjY );

    // Set default line width
    fprintf( outputFile, "%g setlinewidth\n", userToDeviceSize( defaultPenWidth ) );
    fputs( "%%EndPageSetup\n", outputFile );

    return true;
}


bool PS_PLOTTER::EndPlot()
{
    wxASSERT( outputFile );
    fputs( "showpage\n"
           "grestore\n"
           "%%EOF\n", outputFile );
    fclose( outputFile );
    outputFile = NULL;

    return true;
}



void PS_PLOTTER::Text( const wxPoint&       aPos,
                enum EDA_COLOR_T            aColor,
                const wxString&             aText,
                double                      aOrient,
                const wxSize&               aSize,
                enum EDA_TEXT_HJUSTIFY_T    aH_justify,
                enum EDA_TEXT_VJUSTIFY_T    aV_justify,
                int                         aWidth,
                bool                        aItalic,
                bool                        aBold,
                bool                        aMultilineAllowed )
{
    SetCurrentLineWidth( aWidth );
    SetColor( aColor );

    // Fix me: see how to use PS text mode for multiline texts
    if( aMultilineAllowed && !aText.Contains( wxT( "\n" ) ) )
        aMultilineAllowed = false;  // the text has only one line.

    // Draw the native postscript text (if requested)
    if( m_textMode == PLOTTEXTMODE_NATIVE && !aMultilineAllowed )
    {
        const char *fontname = aItalic ? (aBold ? "/KicadFont-BoldOblique"
                : "/KicadFont-Oblique")
                : (aBold ? "/KicadFont-Bold"
                : "/KicadFont");

        // Compute the copious tranformation parameters
        double ctm_a, ctm_b, ctm_c, ctm_d, ctm_e, ctm_f;
        double wideningFactor, heightFactor;
        computeTextParameters( aPos, aText, aOrient, aSize, aH_justify,
                aV_justify, aWidth, aItalic, aBold,
                &wideningFactor, &ctm_a, &ctm_b, &ctm_c,
                &ctm_d, &ctm_e, &ctm_f, &heightFactor );


        // The text must be escaped correctly, the others are the various
        // parameters. The CTM is formatted with %f since sin/cos tends
        // to make %g use exponential notation (which is not supported)
        fputsPostscriptString( outputFile, aText );
        fprintf( outputFile, " %g [%f %f %f %f %f %f] %g %s textshow\n",
                wideningFactor, ctm_a, ctm_b, ctm_c, ctm_d, ctm_e, ctm_f,
                heightFactor, fontname );

        /* The textshow operator retained the coordinate system, we use it
         * to plot the overbars. See the PDF sister function for more
         * details */

        std::vector<int> pos_pairs;
        postscriptOverlinePositions( aText, aSize.x, aItalic, aBold, &pos_pairs );
        int overbar_y = KiROUND( aSize.y * 1.1 );

        for( unsigned i = 0; i < pos_pairs.size(); i += 2)
        {
            DPOINT dev_from = userToDeviceSize( wxSize( pos_pairs[i], overbar_y ) );
            DPOINT dev_to = userToDeviceSize( wxSize( pos_pairs[i + 1], overbar_y ) );
            fprintf( outputFile, "%g %g %g %g line ",
                     dev_from.x, dev_from.y, dev_to.x, dev_to.y );
        }

        // Restore the CTM
        fputs( "grestore\n", outputFile );
    }

    // Draw the hidden postscript text (if requested)
    if( m_textMode == PLOTTEXTMODE_PHANTOM )
    {
        fputsPostscriptString( outputFile, aText );
        DPOINT pos_dev = userToDeviceCoordinates( aPos );
        fprintf( outputFile, " %g %g phantomshow\n", pos_dev.x, pos_dev.y );
    }

    // Draw the stroked text (if requested)
    if( m_textMode != PLOTTEXTMODE_NATIVE || aMultilineAllowed )
    {
        PLOTTER::Text( aPos, aColor, aText, aOrient, aSize, aH_justify, aV_justify,
                       aWidth, aItalic, aBold, aMultilineAllowed );
    }
}


/**
 * Character widths for Helvetica
 */
const double hv_widths[256] = {
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.355, 0.556, 0.556, 0.889, 0.667, 0.191,
    0.333, 0.333, 0.389, 0.584, 0.278, 0.333, 0.278, 0.278,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556,
    0.556, 0.556, 0.278, 0.278, 0.584, 0.584, 0.584, 0.556,
    1.015, 0.667, 0.667, 0.722, 0.722, 0.667, 0.611, 0.778,
    0.722, 0.278, 0.500, 0.667, 0.556, 0.833, 0.722, 0.778,
    0.667, 0.778, 0.722, 0.667, 0.611, 0.722, 0.667, 0.944,
    0.667, 0.667, 0.611, 0.278, 0.278, 0.278, 0.469, 0.556,
    0.333, 0.556, 0.556, 0.500, 0.556, 0.556, 0.278, 0.556,
    0.556, 0.222, 0.222, 0.500, 0.222, 0.833, 0.556, 0.556,
    0.556, 0.556, 0.333, 0.500, 0.278, 0.556, 0.500, 0.722,
    0.500, 0.500, 0.500, 0.334, 0.260, 0.334, 0.584, 0.278,
    0.278, 0.278, 0.222, 0.556, 0.333, 1.000, 0.556, 0.556,
    0.333, 1.000, 0.667, 0.333, 1.000, 0.278, 0.278, 0.278,
    0.278, 0.222, 0.222, 0.333, 0.333, 0.350, 0.556, 1.000,
    0.333, 1.000, 0.500, 0.333, 0.944, 0.278, 0.278, 0.667,
    0.278, 0.333, 0.556, 0.556, 0.556, 0.556, 0.260, 0.556,
    0.333, 0.737, 0.370, 0.556, 0.584, 0.333, 0.737, 0.333,
    0.400, 0.584, 0.333, 0.333, 0.333, 0.556, 0.537, 0.278,
    0.333, 0.333, 0.365, 0.556, 0.834, 0.834, 0.834, 0.611,
    0.667, 0.667, 0.667, 0.667, 0.667, 0.667, 1.000, 0.722,
    0.667, 0.667, 0.667, 0.667, 0.278, 0.278, 0.278, 0.278,
    0.722, 0.722, 0.778, 0.778, 0.778, 0.778, 0.778, 0.584,
    0.778, 0.722, 0.722, 0.722, 0.722, 0.667, 0.667, 0.611,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.889, 0.500,
    0.556, 0.556, 0.556, 0.556, 0.278, 0.278, 0.278, 0.278,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.584,
    0.611, 0.556, 0.556, 0.556, 0.556, 0.500, 0.556, 0.500
};

/**
 * Character widths for Helvetica-Bold
 */
const double hvb_widths[256] = {
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.333, 0.474, 0.556, 0.556, 0.889, 0.722, 0.238,
    0.333, 0.333, 0.389, 0.584, 0.278, 0.333, 0.278, 0.278,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556,
    0.556, 0.556, 0.333, 0.333, 0.584, 0.584, 0.584, 0.611,
    0.975, 0.722, 0.722, 0.722, 0.722, 0.667, 0.611, 0.778,
    0.722, 0.278, 0.556, 0.722, 0.611, 0.833, 0.722, 0.778,
    0.667, 0.778, 0.722, 0.667, 0.611, 0.722, 0.667, 0.944,
    0.667, 0.667, 0.611, 0.333, 0.278, 0.333, 0.584, 0.556,
    0.333, 0.556, 0.611, 0.556, 0.611, 0.556, 0.333, 0.611,
    0.611, 0.278, 0.278, 0.556, 0.278, 0.889, 0.611, 0.611,
    0.611, 0.611, 0.389, 0.556, 0.333, 0.611, 0.556, 0.778,
    0.556, 0.556, 0.500, 0.389, 0.280, 0.389, 0.584, 0.278,
    0.278, 0.278, 0.278, 0.556, 0.500, 1.000, 0.556, 0.556,
    0.333, 1.000, 0.667, 0.333, 1.000, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.500, 0.500, 0.350, 0.556, 1.000,
    0.333, 1.000, 0.556, 0.333, 0.944, 0.278, 0.278, 0.667,
    0.278, 0.333, 0.556, 0.556, 0.556, 0.556, 0.280, 0.556,
    0.333, 0.737, 0.370, 0.556, 0.584, 0.333, 0.737, 0.333,
    0.400, 0.584, 0.333, 0.333, 0.333, 0.611, 0.556, 0.278,
    0.333, 0.333, 0.365, 0.556, 0.834, 0.834, 0.834, 0.611,
    0.722, 0.722, 0.722, 0.722, 0.722, 0.722, 1.000, 0.722,
    0.667, 0.667, 0.667, 0.667, 0.278, 0.278, 0.278, 0.278,
    0.722, 0.722, 0.778, 0.778, 0.778, 0.778, 0.778, 0.584,
    0.778, 0.722, 0.722, 0.722, 0.722, 0.667, 0.667, 0.611,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.889, 0.556,
    0.556, 0.556, 0.556, 0.556, 0.278, 0.278, 0.278, 0.278,
    0.611, 0.611, 0.611, 0.611, 0.611, 0.611, 0.611, 0.584,
    0.611, 0.611, 0.611, 0.611, 0.611, 0.556, 0.611, 0.556
};

/**
 * Character widths for Helvetica-Oblique
 */
const double hvo_widths[256] = {
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.355, 0.556, 0.556, 0.889, 0.667, 0.191,
    0.333, 0.333, 0.389, 0.584, 0.278, 0.333, 0.278, 0.278,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556,
    0.556, 0.556, 0.278, 0.278, 0.584, 0.584, 0.584, 0.556,
    1.015, 0.667, 0.667, 0.722, 0.722, 0.667, 0.611, 0.778,
    0.722, 0.278, 0.500, 0.667, 0.556, 0.833, 0.722, 0.778,
    0.667, 0.778, 0.722, 0.667, 0.611, 0.722, 0.667, 0.944,
    0.667, 0.667, 0.611, 0.278, 0.278, 0.278, 0.469, 0.556,
    0.333, 0.556, 0.556, 0.500, 0.556, 0.556, 0.278, 0.556,
    0.556, 0.222, 0.222, 0.500, 0.222, 0.833, 0.556, 0.556,
    0.556, 0.556, 0.333, 0.500, 0.278, 0.556, 0.500, 0.722,
    0.500, 0.500, 0.500, 0.334, 0.260, 0.334, 0.584, 0.278,
    0.278, 0.278, 0.222, 0.556, 0.333, 1.000, 0.556, 0.556,
    0.333, 1.000, 0.667, 0.333, 1.000, 0.278, 0.278, 0.278,
    0.278, 0.222, 0.222, 0.333, 0.333, 0.350, 0.556, 1.000,
    0.333, 1.000, 0.500, 0.333, 0.944, 0.278, 0.278, 0.667,
    0.278, 0.333, 0.556, 0.556, 0.556, 0.556, 0.260, 0.556,
    0.333, 0.737, 0.370, 0.556, 0.584, 0.333, 0.737, 0.333,
    0.400, 0.584, 0.333, 0.333, 0.333, 0.556, 0.537, 0.278,
    0.333, 0.333, 0.365, 0.556, 0.834, 0.834, 0.834, 0.611,
    0.667, 0.667, 0.667, 0.667, 0.667, 0.667, 1.000, 0.722,
    0.667, 0.667, 0.667, 0.667, 0.278, 0.278, 0.278, 0.278,
    0.722, 0.722, 0.778, 0.778, 0.778, 0.778, 0.778, 0.584,
    0.778, 0.722, 0.722, 0.722, 0.722, 0.667, 0.667, 0.611,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.889, 0.500,
    0.556, 0.556, 0.556, 0.556, 0.278, 0.278, 0.278, 0.278,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.584,
    0.611, 0.556, 0.556, 0.556, 0.556, 0.500, 0.556, 0.500
};

/**
 * Character widths for Helvetica-BoldOblique
 */
const double hvbo_widths[256] = {
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278, 0.278,
    0.278, 0.333, 0.474, 0.556, 0.556, 0.889, 0.722, 0.238,
    0.333, 0.333, 0.389, 0.584, 0.278, 0.333, 0.278, 0.278,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.556,
    0.556, 0.556, 0.333, 0.333, 0.584, 0.584, 0.584, 0.611,
    0.975, 0.722, 0.722, 0.722, 0.722, 0.667, 0.611, 0.778,
    0.722, 0.278, 0.556, 0.722, 0.611, 0.833, 0.722, 0.778,
    0.667, 0.778, 0.722, 0.667, 0.611, 0.722, 0.667, 0.944,
    0.667, 0.667, 0.611, 0.333, 0.278, 0.333, 0.584, 0.556,
    0.333, 0.556, 0.611, 0.556, 0.611, 0.556, 0.333, 0.611,
    0.611, 0.278, 0.278, 0.556, 0.278, 0.889, 0.611, 0.611,
    0.611, 0.611, 0.389, 0.556, 0.333, 0.611, 0.556, 0.778,
    0.556, 0.556, 0.500, 0.389, 0.280, 0.389, 0.584, 0.278,
    0.278, 0.278, 0.278, 0.556, 0.500, 1.000, 0.556, 0.556,
    0.333, 1.000, 0.667, 0.333, 1.000, 0.278, 0.278, 0.278,
    0.278, 0.278, 0.278, 0.500, 0.500, 0.350, 0.556, 1.000,
    0.333, 1.000, 0.556, 0.333, 0.944, 0.278, 0.278, 0.667,
    0.278, 0.333, 0.556, 0.556, 0.556, 0.556, 0.280, 0.556,
    0.333, 0.737, 0.370, 0.556, 0.584, 0.333, 0.737, 0.333,
    0.400, 0.584, 0.333, 0.333, 0.333, 0.611, 0.556, 0.278,
    0.333, 0.333, 0.365, 0.556, 0.834, 0.834, 0.834, 0.611,
    0.722, 0.722, 0.722, 0.722, 0.722, 0.722, 1.000, 0.722,
    0.667, 0.667, 0.667, 0.667, 0.278, 0.278, 0.278, 0.278,
    0.722, 0.722, 0.778, 0.778, 0.778, 0.778, 0.778, 0.584,
    0.778, 0.722, 0.722, 0.722, 0.722, 0.667, 0.667, 0.611,
    0.556, 0.556, 0.556, 0.556, 0.556, 0.556, 0.889, 0.556,
    0.556, 0.556, 0.556, 0.556, 0.278, 0.278, 0.278, 0.278,
    0.611, 0.611, 0.611, 0.611, 0.611, 0.611, 0.611, 0.584,
    0.611, 0.611, 0.611, 0.611, 0.611, 0.556, 0.611, 0.556
};

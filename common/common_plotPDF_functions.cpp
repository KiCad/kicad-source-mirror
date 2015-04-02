/**
 * @file common_plotPDF_functions.cpp
 * @brief Kicad: Common plot PDF Routines
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2012 Lorenzo Marcantonio, l.marcantonio@logossrl.com
 * Copyright (C) 1992-2012 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <pgm_base.h>
#include <trigo.h>
#include <wxstruct.h>
#include <base_struct.h>
#include <common.h>
#include <plot_common.h>
#include <macros.h>
#include <kicad_string.h>
#include <wx/zstream.h>
#include <wx/mstream.h>


/*
 * Open or create the plot file aFullFilename
 * return true if success, false if the file cannot be created/opened
 *
 * Opens the PDF file in binary mode
 */
bool PDF_PLOTTER::OpenFile( const wxString& aFullFilename )
{
    filename = aFullFilename;

    wxASSERT( !outputFile );

    // Open the PDF file in binary mode
    outputFile = wxFopen( filename, wxT( "wb" ) );

    if( outputFile == NULL )
        return false ;

    return true;
}

void PDF_PLOTTER::SetPageSettings( const PAGE_INFO& aPageSettings )
{
    wxASSERT( !workFile );
    pageInfo = aPageSettings;
}

void PDF_PLOTTER::SetViewport( const wxPoint& aOffset, double aIusPerDecimil,
                              double aScale, bool aMirror )
{
    wxASSERT( !workFile );
    m_plotMirror = aMirror;
    plotOffset = aOffset;
    plotScale = aScale;
    m_IUsPerDecimil = aIusPerDecimil;

    // The CTM is set to 1 user unit per decimil
    iuPerDeviceUnit = 1.0 / aIusPerDecimil;

    SetDefaultLineWidth( 100 / iuPerDeviceUnit );  // arbitrary default

    /* The paper size in this engined is handled page by page
       Look in the StartPage function */
}


/**
 * Pen width setting for PDF. Since the specs *explicitly* says that a 0
 * width is a bad thing to use (since it results in 1 pixel traces), we
 * convert such requests to the default width (like -1)
 */
void PDF_PLOTTER::SetCurrentLineWidth( int width )
{
    wxASSERT( workFile );
    int pen_width;

    if( width > 0 )
        pen_width = width;
    else
        pen_width = defaultPenWidth;

    if( pen_width != currentPenWidth )
        fprintf( workFile, "%g w\n",
                 userToDeviceSize( pen_width ) );

    currentPenWidth = pen_width;
}


/**
 * PDF supports colors fully. It actually has distinct fill and pen colors,
 * but we set both at the same time.
 *
 * XXX Keeping them divided could result in a minor optimization in
 * eeschema filled shapes, but would propagate to all the other plot
 * engines. Also arcs are filled as pies but only the arc is stroked so
 * it would be difficult to handle anyway.
 */
void PDF_PLOTTER::emitSetRGBColor( double r, double g, double b )
{
    wxASSERT( workFile );
    fprintf( workFile, "%g %g %g rg %g %g %g RG\n",
             r, g, b, r, g, b );
}

/**
 * PDF supports dashed lines
 */
void PDF_PLOTTER::SetDash( bool dashed )
{
    wxASSERT( workFile );
    if( dashed )
        fprintf( workFile, "[%d %d] 0 d\n",
                 (int) GetDashMarkLenIU(), (int) GetDashGapLenIU() );
    else
        fputs( "[] 0 d\n", workFile );
}


/**
 * Rectangles in PDF. Supported by the native operator
 */
void PDF_PLOTTER::Rect( const wxPoint& p1, const wxPoint& p2, FILL_T fill, int width )
{
    wxASSERT( workFile );
    DPOINT p1_dev = userToDeviceCoordinates( p1 );
    DPOINT p2_dev = userToDeviceCoordinates( p2 );

    SetCurrentLineWidth( width );
    fprintf( workFile, "%g %g %g %g re %c\n", p1_dev.x, p1_dev.y,
             p2_dev.x - p1_dev.x, p2_dev.y - p1_dev.y,
             fill == NO_FILL ? 'S' : 'B' );
}


/**
 * Circle drawing for PDF. They're approximated by curves, but fill is supported
 */
void PDF_PLOTTER::Circle( const wxPoint& pos, int diametre, FILL_T aFill, int width )
{
    wxASSERT( workFile );
    DPOINT pos_dev = userToDeviceCoordinates( pos );
    double radius = userToDeviceSize( diametre / 2.0 );

    /* OK. Here's a trick. PDF doesn't support circles or circular angles, that's
       a fact. You'll have to do with cubic beziers. These *can't* represent
       circular arcs (NURBS can, beziers don't). But there is a widely known
       approximation which is really good
    */

    SetCurrentLineWidth( width );
    double magic = radius * 0.551784; // You don't want to know where this come from

    // This is the convex hull for the bezier approximated circle
    fprintf( workFile, "%g %g m "
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

             aFill == NO_FILL ? 's' : 'b' );
}


/**
 * The PDF engine can't directly plot arcs, it uses the base emulation.
 * So no filled arcs (not a great loss... )
 */
void PDF_PLOTTER::Arc( const wxPoint& centre, double StAngle, double EndAngle, int radius,
                      FILL_T fill, int width )
{
    wxASSERT( workFile );
    if( radius <= 0 )
        return;

    /* Arcs are not so easily approximated by beziers (in the general case),
       so we approximate them in the old way */
    wxPoint   start, end;
    const int delta = 50;   // increment (in 0.1 degrees) to draw circles

    if( StAngle > EndAngle )
        EXCHG( StAngle, EndAngle );

    SetCurrentLineWidth( width );

    // Usual trig arc plotting routine...
    start.x = centre.x + KiROUND( cosdecideg( radius, -StAngle ) );
    start.y = centre.y + KiROUND( sindecideg( radius, -StAngle ) );
    DPOINT pos_dev = userToDeviceCoordinates( start );
    fprintf( workFile, "%g %g m ", pos_dev.x, pos_dev.y );
    for( int ii = StAngle + delta; ii < EndAngle; ii += delta )
    {
        end.x = centre.x + KiROUND( cosdecideg( radius, -ii ) );
        end.y = centre.y + KiROUND( sindecideg( radius, -ii ) );
        pos_dev = userToDeviceCoordinates( end );
        fprintf( workFile, "%g %g l ", pos_dev.x, pos_dev.y );
    }

    end.x = centre.x + KiROUND( cosdecideg( radius, -EndAngle ) );
    end.y = centre.y + KiROUND( sindecideg( radius, -EndAngle ) );
    pos_dev = userToDeviceCoordinates( end );
    fprintf( workFile, "%g %g l ", pos_dev.x, pos_dev.y );

    // The arc is drawn... if not filled we stroke it, otherwise we finish
    // closing the pie at the center
    if( fill == NO_FILL )
    {
        fputs( "S\n", workFile );
    }
    else
    {
        pos_dev = userToDeviceCoordinates( centre );
        fprintf( workFile, "%g %g l b\n", pos_dev.x, pos_dev.y );
    }
}


/**
 * Polygon plotting for PDF. Everything is supported
 */
void PDF_PLOTTER::PlotPoly( const std::vector< wxPoint >& aCornerList,
                           FILL_T aFill, int aWidth )
{
    wxASSERT( workFile );
    if( aCornerList.size() <= 1 )
        return;

    SetCurrentLineWidth( aWidth );

    DPOINT pos = userToDeviceCoordinates( aCornerList[0] );
    fprintf( workFile, "%g %g m\n", pos.x, pos.y );

    for( unsigned ii = 1; ii < aCornerList.size(); ii++ )
    {
        pos = userToDeviceCoordinates( aCornerList[ii] );
        fprintf( workFile, "%g %g l\n", pos.x, pos.y );
    }

    // Close path and stroke(/fill)
    fprintf( workFile, "%c\n", aFill == NO_FILL ? 'S' : 'b' );
}


void PDF_PLOTTER::PenTo( const wxPoint& pos, char plume )
{
    wxASSERT( workFile );
    if( plume == 'Z' )
    {
        if( penState != 'Z' )
        {
            fputs( "S\n", workFile );
            penState     = 'Z';
            penLastpos.x = -1;
            penLastpos.y = -1;
        }
        return;
    }

    if( penState != plume || pos != penLastpos )
    {
        DPOINT pos_dev = userToDeviceCoordinates( pos );
        fprintf( workFile, "%g %g %c\n",
                 pos_dev.x, pos_dev.y,
                 ( plume=='D' ) ? 'l' : 'm' );
    }
    penState   = plume;
    penLastpos = pos;
}

/**
 * PDF images are handles as inline, not XObject streams...
 */
void PDF_PLOTTER::PlotImage( const wxImage & aImage, const wxPoint& aPos,
                            double aScaleFactor )
{
    wxASSERT( workFile );
    wxSize pix_size( aImage.GetWidth(), aImage.GetHeight() );

    // Requested size (in IUs)
    DPOINT drawsize( aScaleFactor * pix_size.x,
                     aScaleFactor * pix_size.y );

    // calculate the bitmap start position
    wxPoint start( aPos.x - drawsize.x / 2,
                   aPos.y + drawsize.y / 2);

    DPOINT dev_start = userToDeviceCoordinates( start );

    /* PDF has an uhm... simplified coordinate system handling. There is
       *one* operator to do everything (the PS concat equivalent). At least
       they kept the matrix stack to save restore environments. Also images
       are always emitted at the origin with a size of 1x1 user units.
       What we need to do is:
       1) save the CTM end estabilish the new one
       2) plot the image
       3) restore the CTM
       4) profit
     */
    fprintf( workFile, "q %g 0 0 %g %g %g cm\n", // Step 1
            userToDeviceSize( drawsize.x ),
            userToDeviceSize( drawsize.y ),
            dev_start.x, dev_start.y );

    /* An inline image is a cross between a dictionary and a stream.
       A real ugly construct (compared with the elegance of the PDF
       format). Also it accepts some 'abbreviations', which is stupid
       since the content stream is usually compressed anyway... */
    fprintf( workFile,
             "BI\n"
             "  /BPC 8\n"
             "  /CS %s\n"
             "  /W %d\n"
             "  /H %d\n"
             "ID\n", colorMode ? "/RGB" : "/G", pix_size.x, pix_size.y );

    /* Here comes the stream (in binary!). I *could* have hex or ascii84
       encoded it, but who cares? I'll go through zlib anyway */
    for( int y = 0; y < pix_size.y; y++ )
    {
        for( int x = 0; x < pix_size.x; x++ )
        {
            unsigned char r = aImage.GetRed( x, y ) & 0xFF;
            unsigned char g = aImage.GetGreen( x, y ) & 0xFF;
            unsigned char b = aImage.GetBlue( x, y ) & 0xFF;
            // As usual these days, stdio buffering has to suffeeeeerrrr
            if( colorMode )
            {
            putc( r, workFile );
            putc( g, workFile );
            putc( b, workFile );
            }
            else
            {
                // Grayscale conversion
                putc( (r + g + b) / 3, workFile );
            }
        }
    }

    fputs( "EI Q\n", workFile ); // Finish step 2 and do step 3
}


/**
 * Allocate a new handle in the table of the PDF object. The
 * handle must be completed using startPdfObject. It's an in-RAM operation
 * only, no output is done.
 */
int PDF_PLOTTER::allocPdfObject()
{
    xrefTable.push_back( 0 );
    return xrefTable.size() - 1;
}


/**
 * Open a new PDF object and returns the handle if the parameter is -1.
 * Otherwise fill in the xref entry for the passed object
 */
int PDF_PLOTTER::startPdfObject(int handle)
{
    wxASSERT( outputFile );
    wxASSERT( !workFile );
    if( handle < 0)
        handle = allocPdfObject();

    xrefTable[handle] = ftell( outputFile );
    fprintf( outputFile, "%d 0 obj\n", handle );
    return handle;
}


/**
 * Close the current PDF object
 */
void PDF_PLOTTER::closePdfObject()
{
    wxASSERT( outputFile );
    wxASSERT( !workFile );
    fputs( "endobj\n", outputFile );
}


/**
 * Starts a PDF stream (for the page). Returns the object handle opened
 * Pass -1 (default) for a fresh object. Especially from PDF 1.5 streams
 * can contain a lot of things, but for the moment we only handle page
 * content.
 */
int PDF_PLOTTER::startPdfStream(int handle)
{
    wxASSERT( outputFile );
    wxASSERT( !workFile );
    handle = startPdfObject( handle );

    // This is guaranteed to be handle+1 but needs to be allocated since
    // you could allocate more object during stream preparation
    streamLengthHandle = allocPdfObject();
    fprintf( outputFile,
             "<< /Length %d 0 R /Filter /FlateDecode >>\n" // Length is deferred
             "stream\n", handle + 1 );

    // Open a temporary file to accumulate the stream
    workFilename = filename + wxT(".tmp");
    workFile = wxFopen( workFilename, wxT( "w+b" ));
    wxASSERT( workFile );
    return handle;
}


/**
 * Finish the current PDF stream (writes the deferred length, too)
 */
void PDF_PLOTTER::closePdfStream()
{
    wxASSERT( workFile );

    long stream_len = ftell( workFile );

    if( stream_len < 0 )
    {
        wxASSERT( false );
        return;
    }

    // Rewind the file, read in the page stream and DEFLATE it
    fseek( workFile, 0, SEEK_SET );
    unsigned char *inbuf = new unsigned char[stream_len];

    int rc = fread( inbuf, 1, stream_len, workFile );
    wxASSERT( rc == stream_len );
    (void) rc;

    // We are done with the temporary file, junk it
    fclose( workFile );
    workFile = 0;
    ::wxRemoveFile( workFilename );

    // NULL means memos owns the memory, but provide a hint on optimum size needed.
    wxMemoryOutputStream    memos( NULL, std::max( 2000l, stream_len ) ) ;

    {
        /* Somewhat standard parameters to compress in DEFLATE. The PDF spec is
         * misleading, it says it wants a DEFLATE stream but it really want a ZLIB
         * stream! (a DEFLATE stream would be generated with -15 instead of 15)
         * rc = deflateInit2( &zstrm, Z_BEST_COMPRESSION, Z_DEFLATED, 15,
         *                    8, Z_DEFAULT_STRATEGY );
         */

        wxZlibOutputStream      zos( memos, wxZ_BEST_COMPRESSION, wxZLIB_ZLIB );

        zos.Write( inbuf, stream_len );

        delete[] inbuf;

    }   // flush the zip stream using zos destructor

    wxStreamBuffer* sb = memos.GetOutputStreamBuffer();

    unsigned out_count = sb->Tell();

    fwrite( sb->GetBufferStart(), 1, out_count, outputFile );

    fputs( "endstream\n", outputFile );
    closePdfObject();

    // Writing the deferred length as an indirect object
    startPdfObject( streamLengthHandle );
    fprintf( outputFile, "%u\n", out_count );
    closePdfObject();
}

/**
 * Starts a new page in the PDF document
 */
void PDF_PLOTTER::StartPage()
{
    wxASSERT( outputFile );
    wxASSERT( !workFile );

    // Compute the paper size in IUs
    paperSize = pageInfo.GetSizeMils();
    paperSize.x *= 10.0 / iuPerDeviceUnit;
    paperSize.y *= 10.0 / iuPerDeviceUnit;

    // Open the content stream; the page object will go later
    pageStreamHandle = startPdfStream();

    /* Now, until ClosePage *everything* must be wrote in workFile, to be
       compressed later in closePdfStream */

    // Default graphic settings (coordinate system, default color and line style)
    fprintf( workFile,
             "%g 0 0 %g 0 0 cm 1 J 1 j 0 0 0 rg 0 0 0 RG %g w\n",
             0.0072 * plotScaleAdjX, 0.0072 * plotScaleAdjY,
             userToDeviceSize( defaultPenWidth ) );
}

/**
 * Close the current page in the PDF document (and emit its compressed stream)
 */
void PDF_PLOTTER::ClosePage()
{
    wxASSERT( workFile );

    // Close the page stream (and compress it)
    closePdfStream();

    // Emit the page object and put it in the page list for later
    pageHandles.push_back( startPdfObject() );

    /* Page size is in 1/72 of inch (default user space units)
       Works like the bbox in postscript but there is no need for
       swapping the sizes, since PDF doesn't require a portrait page.
       We use the MediaBox but PDF has lots of other less used boxes
       to use */

    const double BIGPTsPERMIL = 0.072;
    wxSize psPaperSize = pageInfo.GetSizeMils();

    fprintf( outputFile,
             "<<\n"
             "/Type /Page\n"
             "/Parent %d 0 R\n"
             "/Resources <<\n"
             "    /ProcSet [/PDF /Text /ImageC /ImageB]\n"
             "    /Font %d 0 R >>\n"
             "/MediaBox [0 0 %d %d]\n"
             "/Contents %d 0 R\n"
             ">>\n",
             pageTreeHandle,
             fontResDictHandle,
             int( ceil( psPaperSize.x * BIGPTsPERMIL ) ),
             int( ceil( psPaperSize.y * BIGPTsPERMIL ) ),
             pageStreamHandle );
    closePdfObject();

    // Mark the page stream as idle
    pageStreamHandle = 0;
}

/**
 * The PDF engine supports multiple pages; the first one is opened
 * 'for free' the following are to be closed and reopened. Between
 * each page parameters can be set
 */
bool PDF_PLOTTER::StartPlot()
{
    wxASSERT( outputFile );

    // First things first: the customary null object
    xrefTable.clear();
    xrefTable.push_back( 0 );

    /* The header (that's easy!). The second line is binary junk required
       to make the file binary from the beginning (the important thing is
       that they must have the bit 7 set) */
    fputs( "%PDF-1.5\n%\200\201\202\203\n", outputFile );

    /* Allocate an entry for the page tree root, it will go in every page
       parent entry */
    pageTreeHandle = allocPdfObject();

    /* In the same way, the font resource dictionary is used by every page
       (it *could* be inherited via the Pages tree */
    fontResDictHandle = allocPdfObject();

    /* Now, the PDF is read from the end, (more or less)... so we start
       with the page stream for page 1. Other more important stuff is written
       at the end */
    StartPage();
    return true;
}


bool PDF_PLOTTER::EndPlot()
{
    wxASSERT( outputFile );

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
       We'll need metrics anyway to do any aligment (these are in the shared with
       the postscript engine) */
    for( int i = 0; i < 4; i++ )
    {
        fontdefs[i].font_handle = startPdfObject();
        fprintf( outputFile,
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
    startPdfObject( fontResDictHandle );
    fputs( "<<\n", outputFile );
    for( int i = 0; i < 4; i++ )
    {
        fprintf( outputFile, "    %s %d 0 R\n",
                fontdefs[i].rsname, fontdefs[i].font_handle );
    }
    fputs( ">>\n", outputFile );
    closePdfObject();

    /* The page tree: it's a B-tree but luckily we only have few pages!
       So we use just an array... The handle was allocated at the beginning,
       now we instantiate the corresponding object */
    startPdfObject( pageTreeHandle );
    fputs( "<<\n"
           "/Type /Pages\n"
           "/Kids [\n", outputFile );

    for( unsigned i = 0; i < pageHandles.size(); i++ )
        fprintf( outputFile, "%d 0 R\n", pageHandles[i] );

    fprintf( outputFile,
            "]\n"
            "/Count %ld\n"
             ">>\n", (long) pageHandles.size() );
    closePdfObject();


    // The info dictionary
    int infoDictHandle = startPdfObject();
    char date_buf[250];
    time_t ltime = time( NULL );
    strftime( date_buf, 250, "D:%Y%m%d%H%M%S",
              localtime( &ltime ) );
    fprintf( outputFile,
             "<<\n"
             "/Producer (KiCAD PDF)\n"
             "/CreationDate (%s)\n"
             "/Creator (%s)\n"
             "/Title (%s)\n"
             "/Trapped false\n",
             date_buf,
             TO_UTF8( creator ),
             TO_UTF8( filename ) );

    fputs( ">>\n", outputFile );
    closePdfObject();

    // The catalog, at last
    int catalogHandle = startPdfObject();
    fprintf( outputFile,
             "<<\n"
             "/Type /Catalog\n"
             "/Pages %d 0 R\n"
             "/Version /1.5\n"
             "/PageMode /UseNone\n"
             "/PageLayout /SinglePage\n"
             ">>\n", pageTreeHandle );
    closePdfObject();

    /* Emit the xref table (format is crucial to the byte, each entry must
       be 20 bytes long, and object zero must be done in that way). Also
       the offset must be kept along for the trailer */
    long xref_start = ftell( outputFile );
    fprintf( outputFile,
             "xref\n"
             "0 %ld\n"
             "0000000000 65535 f \n", (long) xrefTable.size() );
    for( unsigned i = 1; i < xrefTable.size(); i++ )
    {
        fprintf( outputFile, "%010ld 00000 n \n", xrefTable[i] );
    }

    // Done the xref, go for the trailer
    fprintf( outputFile,
             "trailer\n"
             "<< /Size %lu /Root %d 0 R /Info %d 0 R >>\n"
             "startxref\n"
             "%ld\n" // The offset we saved before
             "%%%%EOF\n",
             (unsigned long) xrefTable.size(), catalogHandle, infoDictHandle, xref_start );

    fclose( outputFile );
    outputFile = NULL;

    return true;
}

void PDF_PLOTTER::Text( const wxPoint&              aPos,
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
    // PDF files do not like 0 sized texts which create broken files.
    if( aSize.x == 0 || aSize.y == 0 )
        return;

    // Fix me: see how to use PDF text mode for multiline texts
    if( aMultilineAllowed && !aText.Contains( wxT( "\n" ) ) )
        aMultilineAllowed = false;  // the text has only one line.

    // Emit native PDF text (if requested)
    if( m_textMode != PLOTTEXTMODE_STROKE && !aMultilineAllowed )
    {
        const char *fontname = aItalic ? (aBold ? "/KicadFontBI" : "/KicadFontI")
            : (aBold ? "/KicadFontB" : "/KicadFont");

        // Compute the copious tranformation parameters
        double ctm_a, ctm_b, ctm_c, ctm_d, ctm_e, ctm_f;
        double wideningFactor, heightFactor;
        computeTextParameters( aPos, aText, aOrient, aSize, aH_justify,
                aV_justify, aWidth, aItalic, aBold,
                &wideningFactor, &ctm_a, &ctm_b, &ctm_c,
                &ctm_d, &ctm_e, &ctm_f, &heightFactor );

        SetColor( aColor );
        SetCurrentLineWidth( aWidth );

        /* We use the full CTM instead of the text matrix because the same
           coordinate system will be used for the overlining. Also the %f
           for the trig part of the matrix to avoid %g going in exponential
           format (which is not supported)
           Rendermode 0 shows the text, rendermode 3 is invisible */
        fprintf( workFile, "q %f %f %f %f %g %g cm BT %s %g Tf %d Tr %g Tz ",
                ctm_a, ctm_b, ctm_c, ctm_d, ctm_e, ctm_f,
                fontname, heightFactor,
                (m_textMode == PLOTTEXTMODE_NATIVE) ? 0 : 3,
                wideningFactor * 100 );

        // The text must be escaped correctly
        fputsPostscriptString( workFile, aText );
        fputs( " Tj ET\n", workFile );

        /* We are still in text coordinates, plot the overbars (if we're
         * not doing phantom text) */
        if( m_textMode == PLOTTEXTMODE_NATIVE )
        {
            std::vector<int> pos_pairs;
            postscriptOverlinePositions( aText, aSize.x, aItalic, aBold, &pos_pairs );
            int overbar_y = KiROUND( aSize.y * 1.1 );
            for( unsigned i = 0; i < pos_pairs.size(); i += 2)
            {
                /* This is a nontrivial situation: we are *not* in the user
                   coordinate system, so the userToDeviceCoordinates function
                   can't be used! Strange as it may seem, the userToDeviceSize
                   is the right function to use here... */
                DPOINT dev_from = userToDeviceSize( wxSize( pos_pairs[i], overbar_y ) );
                DPOINT dev_to = userToDeviceSize( wxSize( pos_pairs[i + 1], overbar_y ) );
                fprintf( workFile, "%g %g m %g %g l ",
                        dev_from.x, dev_from.y, dev_to.x, dev_to.y );
            }
        }

        // Stroke and restore the CTM
        fputs( "S Q\n", workFile );
    }

    // Plot the stroked text (if requested)
    if( m_textMode != PLOTTEXTMODE_NATIVE || aMultilineAllowed )
    {
        PLOTTER::Text( aPos, aColor, aText, aOrient, aSize, aH_justify, aV_justify,
                aWidth, aItalic, aBold, aMultilineAllowed );
    }
}


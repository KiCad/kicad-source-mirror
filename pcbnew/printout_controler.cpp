/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 1992-2011 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file printout_controler.cpp
 * @brief Board print handler implementation file.
 */


// Set this to 1 if you want to test PostScript printing under MSW.
#define wxTEST_POSTSCRIPT_IN_MSW 1

#include <fctsys.h>
#include <pgm_base.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <base_units.h>
#ifdef PCBNEW
    #include <wxBasePcbFrame.h>
    #include <class_board.h>
    #include <pcbnew.h>
#else
    #include <wxstruct.h>
    #include <class_base_screen.h>
    #include <layers_id_colors_and_visibility.h>
    #include <gerbview_frame.h>
#endif
#include <printout_controler.h>


/**
 * Definition for enabling and disabling print controller trace output.  See the
 * wxWidgets documentation on using the WXTRACE environment variable.
 */
static const wxString tracePrinting( wxT( "KicadPrinting" ) );


PRINT_PARAMETERS::PRINT_PARAMETERS()
{
    m_PenDefaultSize        = Millimeter2iu( 0.2 ); // A reasonable default value to draw items
                                      // which do not have a specified line width
    m_PrintScale            = 1.0;
    m_XScaleAdjust          = 1.0;
    m_YScaleAdjust          = 1.0;
    m_Print_Sheet_Ref       = false;
    m_PrintMaskLayer.set();
    m_PrintMirror           = false;
    m_Print_Black_and_White = true;
    m_OptionPrintPage       = 1;
    m_PageCount             = 1;
    m_ForceCentered         = false;
    m_Flags                 = 0;
    m_DrillShapeOpt         = PRINT_PARAMETERS::SMALL_DRILL_SHAPE;
    m_PageSetupData         = NULL;
}


BOARD_PRINTOUT_CONTROLLER::BOARD_PRINTOUT_CONTROLLER( const PRINT_PARAMETERS& aParams,
                                                      EDA_DRAW_FRAME*         aParent,
                                                      const wxString&         aTitle ) :
    wxPrintout( aTitle )
{
    m_PrintParams = aParams;   // Make a local copy of the print parameters.
    m_Parent = aParent;
}


bool BOARD_PRINTOUT_CONTROLLER::OnPrintPage( int aPage )
{
#ifdef PCBNEW
    LSET lset = m_PrintParams.m_PrintMaskLayer;

    // compute layer mask from page number if we want one page per layer
    if( m_PrintParams.m_OptionPrintPage == 0 )  // One page per layer
    {
        // This sequence is TBD, call a different
        // sequencer if needed, such as Seq().  Could not find documentation on
        // page order.
        LSEQ seq = lset.UIOrder();

        if( unsigned( aPage ) < seq.size() )
            m_PrintParams.m_PrintMaskLayer = LSET( seq[aPage] );
    }

    if( !m_PrintParams.m_PrintMaskLayer.any() )
        return false;

    // In Pcbnew we can want the layer EDGE always printed
    if( m_PrintParams.m_Flags == 1 )
        m_PrintParams.m_PrintMaskLayer.set( Edge_Cuts );

    DrawPage();

    m_PrintParams.m_PrintMaskLayer = lset;
#else   // GERBVIEW
    // in gerbview, draw layers are printed on separate pages
    m_PrintParams.m_Flags = aPage-1;    // = gerber draw layer id
    DrawPage();
#endif

    return true;
}


void BOARD_PRINTOUT_CONTROLLER::GetPageInfo( int* minPage, int* maxPage,
                                             int* selPageFrom, int* selPageTo )
{
    *minPage     = 1;
    *selPageFrom = 1;

    int icnt = 1;

    if( m_PrintParams.m_OptionPrintPage == 0 )
        icnt = m_PrintParams.m_PageCount;

    *maxPage   = icnt;
    *selPageTo = icnt;
}


void BOARD_PRINTOUT_CONTROLLER::DrawPage()
{
    wxPoint       offset;
    double        userscale;
    EDA_RECT      boardBoundingBox;
    EDA_RECT      drawRect;
    wxDC*         dc = GetDC();
    BASE_SCREEN*  screen = m_Parent->GetScreen();
    bool          printMirror = m_PrintParams.m_PrintMirror;
    wxSize        pageSizeIU = m_Parent->GetPageSizeIU();

    wxBusyCursor  dummy;

#if defined (PCBNEW)
    BOARD * brd = ((PCB_BASE_FRAME*) m_Parent)->GetBoard();
    boardBoundingBox = brd->ComputeBoundingBox();
    wxString titleblockFilename = brd->GetFileName();
#elif defined (GERBVIEW)
    boardBoundingBox = ((GERBVIEW_FRAME*) m_Parent)->GetGerberLayoutBoundingBox();
    wxString titleblockFilename;    // TODO see if we uses the gerber file name
#else
    #error BOARD_PRINTOUT_CONTROLLER::DrawPage() works only for PCBNEW or GERBVIEW
#endif

    // Use the page size as the drawing area when the board is shown or the user scale
    // is less than 1.
    if( m_PrintParams.PrintBorderAndTitleBlock() )
        boardBoundingBox = EDA_RECT( wxPoint( 0, 0 ), pageSizeIU );

    wxLogTrace( tracePrinting, wxT( "Drawing bounding box:                 x=%d, y=%d, w=%d, h=%d" ),
                boardBoundingBox.GetX(), boardBoundingBox.GetY(),
                boardBoundingBox.GetWidth(), boardBoundingBox.GetHeight() );

    // Compute the PCB size in internal units
    userscale = m_PrintParams.m_PrintScale;

    if( m_PrintParams.m_PrintScale == 0 )   //  fit in page option
    {
        if(boardBoundingBox.GetWidth() && boardBoundingBox.GetHeight())
        {
            int margin = Millimeter2iu( 10.0 ); // add a margin around the drawings
            double scaleX = (double)(pageSizeIU.x - (2 * margin)) /
                            boardBoundingBox.GetWidth();
            double scaleY = (double)(pageSizeIU.y - (2 * margin)) /
                            boardBoundingBox.GetHeight();
            userscale = (scaleX < scaleY) ? scaleX : scaleY;
        }
        else
            userscale = 1.0;
    }

    wxSize scaledPageSize = pageSizeIU;
    drawRect.SetSize( scaledPageSize );
    scaledPageSize.x = wxRound( scaledPageSize.x / userscale );
    scaledPageSize.y = wxRound( scaledPageSize.y / userscale );


    if( m_PrintParams.m_PageSetupData )
    {
        wxLogTrace( tracePrinting, wxT( "Fit size to page margins:         x=%d, y=%d" ),
                    scaledPageSize.x, scaledPageSize.y );

        // Always scale to the size of the paper.
        FitThisSizeToPageMargins( scaledPageSize, *m_PrintParams.m_PageSetupData );
    }

    // Compute Accurate scale 1
    if( m_PrintParams.m_PrintScale == 1.0 )
    {
        // We want a 1:1 scale, regardless the page setup
        // like page size, margin ...
        MapScreenSizeToPaper(); // set best scale and offset (scale is not used)
        int w, h;
        GetPPIPrinter( &w, &h );
        double accurate_Xscale = (double) w / (IU_PER_MILS*1000);
        double accurate_Yscale = (double) h / (IU_PER_MILS*1000);

        if( IsPreview() )  // Scale must take in account the DC size in Preview
        {
            // Get the size of the DC in pixels
            wxSize       PlotAreaSize;
            dc->GetSize( &PlotAreaSize.x, &PlotAreaSize.y );
            GetPageSizePixels( &w, &h );
            accurate_Xscale *= (double)PlotAreaSize.x / w;
            accurate_Yscale *= (double)PlotAreaSize.y / h;
        }
        // Fine scale adjust
        accurate_Xscale *= m_PrintParams.m_XScaleAdjust;
        accurate_Yscale *= m_PrintParams.m_YScaleAdjust;

        // Set print scale for 1:1 exact scale
        dc->SetUserScale( accurate_Xscale, accurate_Yscale );
    }

    // Get the final size of the DC in pixels
    wxSize       PlotAreaSizeInPixels;
    dc->GetSize( &PlotAreaSizeInPixels.x, &PlotAreaSizeInPixels.y );
    wxLogTrace( tracePrinting, wxT( "Plot area in pixels:              x=%d, y=%d" ),
                PlotAreaSizeInPixels.x, PlotAreaSizeInPixels.y );
    double scalex, scaley;
    dc->GetUserScale( &scalex, &scaley );
    wxLogTrace( tracePrinting, wxT( "DC user scale:                    x=%g, y=%g" ),
                scalex, scaley );

    wxSize PlotAreaSizeInUserUnits;
    PlotAreaSizeInUserUnits.x = KiROUND( PlotAreaSizeInPixels.x / scalex );
    PlotAreaSizeInUserUnits.y = KiROUND( PlotAreaSizeInPixels.y / scaley );
    wxLogTrace( tracePrinting, wxT( "Scaled plot area in user units:   x=%d, y=%d" ),
                PlotAreaSizeInUserUnits.x, PlotAreaSizeInUserUnits.y );

    // In module editor, the module is located at 0,0 but for printing
    // it is moved to pageSizeIU.x/2, pageSizeIU.y/2.
    // So the equivalent board must be moved to the center of the page:
    if( m_Parent->IsType( FRAME_PCB_MODULE_EDITOR ) )
    {
        boardBoundingBox.Move( wxPoint( pageSizeIU.x/2, pageSizeIU.y/2 ) );
    }

    // In some cases the plot origin is the centre of the board outline rather than the center
    // of the selected paper size.
    if( m_PrintParams.CenterOnBoardOutline() )
    {
        // Here we are only drawing the board and it's contents.
        drawRect = boardBoundingBox;
        offset.x += wxRound( (double) -scaledPageSize.x / 2.0 );
        offset.y += wxRound( (double) -scaledPageSize.y / 2.0 );

        wxPoint center = boardBoundingBox.Centre();

        if( printMirror )
        {
            // Calculate the mirrored center of the board.
            center.x = m_Parent->GetPageSizeIU().x - boardBoundingBox.Centre().x;
        }

        offset += center;
    }

    GRResetPenAndBrush( dc );

    EDA_DRAW_PANEL* panel = m_Parent->GetCanvas();
    EDA_RECT        tmp   = *panel->GetClipBox();

    // Set clip box to the max size
    #define MAX_VALUE (INT_MAX/2)   // MAX_VALUE is the max we can use in an integer
                                    // and that allows calculations without overflow
    panel->SetClipBox( EDA_RECT( wxPoint( 0, 0 ), wxSize( MAX_VALUE, MAX_VALUE ) ) );

    screen->m_IsPrinting = true;
    EDA_COLOR_T bg_color = g_DrawBgColor;

    // Print frame reference, if reqquested, before
    if( m_PrintParams.m_Print_Black_and_White )
        GRForceBlackPen( true );

    if( m_PrintParams.PrintBorderAndTitleBlock() )
        m_Parent->DrawWorkSheet( dc, screen, m_PrintParams.m_PenDefaultSize,
                                  IU_PER_MILS, titleblockFilename );

    if( printMirror )
    {
        // To plot mirror, we reverse the x axis, and modify the plot x origin
        dc->SetAxisOrientation( false, false);

        /* Plot offset x is moved by the x plot area size in order to have
         * the old draw area in the new draw area, because the draw origin has not moved
         * (this is the upper left corner) but the X axis is reversed, therefore the plotting area
         * is the x coordinate values from  - PlotAreaSize.x to 0 */
        int x_dc_offset = PlotAreaSizeInPixels.x;
        x_dc_offset = KiROUND( x_dc_offset  * userscale );
        dc->SetDeviceOrigin( x_dc_offset, 0 );

        wxLogTrace( tracePrinting, wxT( "Device origin:                    x=%d, y=%d" ),
                    x_dc_offset, 0 );

        panel->SetClipBox( EDA_RECT( wxPoint( -MAX_VALUE/2, -MAX_VALUE/2 ),
                                     panel->GetClipBox()->GetSize() ) );
    }

    // screen->m_DrawOrg = offset;
    dc->SetLogicalOrigin( offset.x, offset.y );

    wxLogTrace( tracePrinting, wxT( "Logical origin:                   x=%d, y=%d" ),
                offset.x, offset.y );

#if defined(wxUSE_LOG_TRACE) && defined( DEBUG )
    wxRect paperRect = GetPaperRectPixels();
    wxLogTrace( tracePrinting, wxT( "Paper rectangle:                  left=%d, top=%d, "
                                    "right=%d, bottom=%d" ),
                paperRect.GetLeft(), paperRect.GetTop(), paperRect.GetRight(),
                paperRect.GetBottom() );

    int devLeft = dc->LogicalToDeviceX( drawRect.GetX() );
    int devTop = dc->LogicalToDeviceY( drawRect.GetY() );
    int devRight = dc->LogicalToDeviceX( drawRect.GetRight() );
    int devBottom = dc->LogicalToDeviceY( drawRect.GetBottom() );
    wxLogTrace( tracePrinting, wxT( "Final device rectangle:           left=%d, top=%d, "
                                    "right=%d, bottom=%d\n" ),
                devLeft, devTop, devRight, devBottom );
#endif

    g_DrawBgColor = WHITE;

    /* when printing in color mode, we use the graphic OR mode that gives the same look as
     * the screen but because the background is white when printing, we must use a trick:
     * In order to plot on a white background in OR mode we must:
     * 1 - Plot all items in black, this creates a local black background
     * 2 - Plot in OR mode on black "local" background
     */
    if( !m_PrintParams.m_Print_Black_and_White )
    {
        // Creates a "local" black background
        GRForceBlackPen( true );
        m_Parent->PrintPage( dc, m_PrintParams.m_PrintMaskLayer,
                             printMirror, &m_PrintParams );
        GRForceBlackPen( false );
    }
    else
        GRForceBlackPen( true );


#if defined (GERBVIEW)
    // In B&W mode, do not force black pen for Gerbview
    // because negative objects need a white pen, not a black pen
    // B&W mode is handled in print page
    GRForceBlackPen( false );
#endif
    m_Parent->PrintPage( dc, m_PrintParams.m_PrintMaskLayer, printMirror,
                         &m_PrintParams );

    g_DrawBgColor = bg_color;
    screen->m_IsPrinting = false;
    panel->SetClipBox( tmp );
    GRForceBlackPen( false );
}

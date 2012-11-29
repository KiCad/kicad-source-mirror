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
#include <appl_wxstruct.h>
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
    m_PenDefaultSize        = 50;     // A reasonable minimal value to draw items
                                      // mainly that do not have a specified line width
    m_PrintScale            = 1.0;
    m_XScaleAdjust          = 1.0;
    m_YScaleAdjust          = 1.0;
    m_Print_Sheet_Ref       = false;
    m_PrintMaskLayer        = 0xFFFFFFFF;
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
    int layers_count = NB_LAYERS;
#else
    int layers_count = LAYER_COUNT;
#endif

    int mask_layer = m_PrintParams.m_PrintMaskLayer;

    // compute layer mask from page number if we want one page per layer
    if( m_PrintParams.m_OptionPrintPage == 0 )  // One page per layer
    {
        int ii, jj, mask = 1;

        for( ii = 0, jj = 0; ii < layers_count; ii++ )
        {
            if( mask_layer & mask )
                jj++;

            if( jj == aPage )
            {
                m_PrintParams.m_PrintMaskLayer = mask;
                break;
            }

            mask <<= 1;
        }
    }

    if( m_PrintParams.m_PrintMaskLayer == 0 )
        return false;

#ifdef PCBNEW
    // In Pcbnew we can want the layer EDGE always printed
    if( m_PrintParams.m_Flags == 1 )
        m_PrintParams.m_PrintMaskLayer |= EDGE_LAYER;
#endif

    DrawPage();

    m_PrintParams.m_PrintMaskLayer = mask_layer;

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

    wxBusyCursor  dummy;

#ifdef PCBNEW
    if( m_PrintParams.PrintBorderAndTitleBlock() )
        boardBoundingBox =((PCB_BASE_FRAME*) m_Parent)->GetBoard()->ComputeBoundingBox();
    else
        boardBoundingBox =((PCB_BASE_FRAME*) m_Parent)->GetBoard()->ComputeBoundingBox( true );
#else
    boardBoundingBox = ((GERBVIEW_FRAME*) m_Parent)->GetLayoutBoundingBox();
#endif

    // Use the page size as the drawing area when the board is shown or the user scale
    // is less than 1.
    if( m_PrintParams.PrintBorderAndTitleBlock() )
        boardBoundingBox = EDA_RECT( wxPoint( 0, 0 ), m_Parent->GetPageSizeIU() );

    // In module editor, the module is located at 0,0 but for printing
    // it is moved to pageSizeIU.x/2, pageSizeIU.y/2.
    // So the equivalent board must be moved:
    if( m_Parent->IsType( MODULE_EDITOR_FRAME_TYPE ) )
    {
        boardBoundingBox.Move( wxPoint( boardBoundingBox.GetWidth()/2,
                                        boardBoundingBox.GetHeight()/2 ) );
    }

    wxLogTrace( tracePrinting, wxT( "Drawing bounding box:             x=%d, y=%d, w=%d, h=%d" ),
                boardBoundingBox.GetX(), boardBoundingBox.GetY(),
                boardBoundingBox.GetWidth(), boardBoundingBox.GetHeight() );

    // Compute the PCB size in internal units
    userscale = m_PrintParams.m_PrintScale;

    if( m_PrintParams.m_PrintScale == 0 )                //  fit in page
    {
        userscale = 1.0;
    }

    wxSize scaledPageSize = m_Parent->GetPageSizeIU();
    drawRect.SetSize( scaledPageSize );
    scaledPageSize.x = wxRound( (double) scaledPageSize.x / userscale );
    scaledPageSize.y = wxRound( (double) scaledPageSize.y / userscale );


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
        // We want a 1:1 scale and margins for printing
        MapScreenSizeToPaper();
        int w, h;
        GetPPIPrinter( &w, &h );
        double accurate_Xscale = ( (double) (  w ) ) / (IU_PER_MILS*1000);
        double accurate_Yscale = ( (double) (  h ) ) / (IU_PER_MILS*1000);

        if( IsPreview() )  // Scale must take in account the DC size in Preview
        {
            // Get the size of the DC in pixels
            wxSize       PlotAreaSize;
            dc->GetSize( &PlotAreaSize.x, &PlotAreaSize.y );
            GetPageSizePixels( &w, &h );
            accurate_Xscale *= PlotAreaSize.x;
            accurate_Xscale /= (double) w;
            accurate_Yscale *= PlotAreaSize.y;
            accurate_Yscale /= (double) h;
        }

        accurate_Xscale *= m_PrintParams.m_XScaleAdjust;
        accurate_Yscale *= m_PrintParams.m_YScaleAdjust;

        // Fine scale adjust
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
    PlotAreaSizeInUserUnits.x = (int) (PlotAreaSizeInPixels.x/scalex);
    PlotAreaSizeInUserUnits.y = (int) (PlotAreaSizeInPixels.y/scaley);
    wxLogTrace( tracePrinting, wxT( "Scaled plot area in user units:   x=%d, y=%d" ),
                PlotAreaSizeInUserUnits.x, PlotAreaSizeInUserUnits.y );

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
            center.y = m_Parent->GetPageSizeIU().y - boardBoundingBox.Centre().y;
        }

        offset += center;
    }

    GRResetPenAndBrush( dc );

    if( m_PrintParams.m_Print_Black_and_White )
        GRForceBlackPen( true );

    EDA_DRAW_PANEL* panel = m_Parent->GetCanvas();
    EDA_RECT        tmp   = *panel->GetClipBox();

    // Set clip box to the max size
    #define MAX_VALUE (INT_MAX/2)   // MAX_VALUE is the max we can use in an integer
                                    // and that allows calculations without overflow
    panel->SetClipBox( EDA_RECT( wxPoint( 0, 0 ), wxSize( MAX_VALUE, MAX_VALUE ) ) );

    screen->m_IsPrinting = true;
    EDA_COLOR_T bg_color = g_DrawBgColor;

    if( printMirror )
    {
        // To plot mirror, we reverse the y axis, and modify the plot y origin
        dc->SetAxisOrientation( true, true );

        /* Plot offset y is moved by the y plot area size in order to have
         * the old draw area in the new draw area, because the draw origin has not moved
         * (this is the upper left corner) but the Y axis is reversed, therefore the plotting area
         * is the y coordinate values from  - PlotAreaSize.y to 0 */
        int y_dc_offset = PlotAreaSizeInPixels.y;
        y_dc_offset = (int) ( ( double ) y_dc_offset * userscale );
        dc->SetDeviceOrigin( 0, y_dc_offset );

        wxLogTrace( tracePrinting, wxT( "Device origin:                    x=%d, y=%d" ),
                    0, y_dc_offset );

        panel->SetClipBox( EDA_RECT( wxPoint( -MAX_VALUE/2, -MAX_VALUE/2 ),
                                     panel->GetClipBox()->GetSize() ) );
    }

    // screen->m_DrawOrg = offset;
    dc->SetLogicalOrigin( offset.x, offset.y );

    wxLogTrace( tracePrinting, wxT( "Logical origin:                   x=%d, y=%d" ),
                offset.x, offset.y );

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
        m_Parent->PrintPage( dc, m_PrintParams.m_PrintMaskLayer, printMirror, &m_PrintParams );
        GRForceBlackPen( false );
    }

    if( m_PrintParams.PrintBorderAndTitleBlock() )
        m_Parent->TraceWorkSheet( dc, screen, m_PrintParams.m_PenDefaultSize,
                                  IU_PER_MILS, m_Parent->GetScreenDesc() );

    m_Parent->PrintPage( dc, m_PrintParams.m_PrintMaskLayer, printMirror, &m_PrintParams );

    g_DrawBgColor = bg_color;
    screen->m_IsPrinting = false;
    panel->SetClipBox( tmp );
    GRForceBlackPen( false );
}

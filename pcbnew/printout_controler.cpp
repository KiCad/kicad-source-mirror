/**************************/
/* printout_controler.cpp */
/**************************/

// Set this to 1 if you want to test PostScript printing under MSW.
#define wxTEST_POSTSCRIPT_IN_MSW 1

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "common.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "pcbnew.h"
#include "protos.h"
#include "pcbplot.h"

#include "printout_controler.h"

// This class is an helper to pass print parameters to print functions
PRINT_PARAMETERS::PRINT_PARAMETERS()
{
    m_PenDefaultSize  = 50;     // A reasonnable minimal value to draw items
                                // mainly that do not have a specifed line width
    m_PrintScale      = 1.0;
    m_XScaleAdjust    = m_YScaleAdjust = 1.0;
    m_Print_Sheet_Ref = false;
    m_PrintMaskLayer  = 0xFFFFFFFF;
    m_PrintMirror     = false;
    m_Print_Black_and_White = true;
    m_OptionPrintPage = 1;
    m_PageCount     = 1;
    m_ForceCentered = false;
    m_Flags = 0;
    m_DrillShapeOpt = PRINT_PARAMETERS::SMALL_DRILL_SHAPE;
}


BOARD_PRINTOUT_CONTROLER::BOARD_PRINTOUT_CONTROLER( const PRINT_PARAMETERS& print_params,
                                                    WinEDA_DrawFrame* parent,
                                                    const wxString&   title ) :
    wxPrintout( title )
{
    m_PrintParams = print_params;   // Make a local copy of parameters.
                                    // So they can change in printout controler
    m_Parent = parent;
}


/*****************************************************/
bool BOARD_PRINTOUT_CONTROLER::OnPrintPage( int page )
/*****************************************************/
{
    int layers_count = NB_LAYERS;

    if( m_Parent->m_Ident == GERBER_FRAME )
        layers_count = 32;

    int mask_layer = m_PrintParams.m_PrintMaskLayer;

    // compute layer mask from page number if we want one page per layer
    if( m_PrintParams.m_OptionPrintPage == 0 )
    {
        int ii, jj, mask = 1;
        for( ii = 0, jj = 0; ii < layers_count; ii++ )
        {
            if( mask_layer & mask )
                jj++;
            if( jj == page )
            {
                m_PrintParams.m_PrintMaskLayer = mask;
                break;
            }
            mask <<= 1;
        }
    }

    if( m_PrintParams.m_PrintMaskLayer == 0 )
        return false;

    // In pcbnew we can want the layer EDGE always printed
    if( m_PrintParams.m_Flags == 1 )
        m_PrintParams.m_PrintMaskLayer |= EDGE_LAYER;

    DrawPage();

    m_PrintParams.m_PrintMaskLayer = mask_layer;

    return true;
}


/*********************************************************/
void BOARD_PRINTOUT_CONTROLER::GetPageInfo( int* minPage, int* maxPage,
                                            int* selPageFrom, int* selPageTo )
/*********************************************************/
{
    *minPage     = 1;
    *selPageFrom = 1;

    int icnt = 1;
    if( m_PrintParams.m_OptionPrintPage == 0 )
        icnt = m_PrintParams.m_PageCount;

    *maxPage   = icnt;
    *selPageTo = icnt;
}


/****************************************/
void BOARD_PRINTOUT_CONTROLER::DrawPage()
/****************************************/

/*
 * This is the real print function: print the active screen
 */
{
    int          tmpzoom;
    wxPoint      tmp_startvisu;
    wxSize       PageSize_in_mm;
    wxSize       SheetSize;     // Page size in internal units
    wxSize       PlotAreaSize;  // plot area size in pixels
    double       scaleX, scaleY, scale;
    wxPoint      old_org;
    wxPoint      DrawOffset; // Offset de trace
    double       userscale;
    double       DrawZoom = 1;
    wxDC*        dc = GetDC();

    bool         printMirror = m_PrintParams.m_PrintMirror;

    wxBusyCursor dummy;

    GetPageSizeMM( &PageSize_in_mm.x, &PageSize_in_mm.y );

    /* Save old draw scale and draw offset */
    tmp_startvisu = ActiveScreen->m_StartVisu;
    tmpzoom = ActiveScreen->GetZoom();
    old_org = ActiveScreen->m_DrawOrg;
    /* Change draw scale and offset to draw the whole page */
    ActiveScreen->SetScalingFactor( DrawZoom );
    ActiveScreen->m_DrawOrg.x   = ActiveScreen->m_DrawOrg.y = 0;
    ActiveScreen->m_StartVisu.x = ActiveScreen->m_StartVisu.y = 0;

    // Gerbview uses a very large sheet (called "World" in gerber language)
    // to print a sheet, uses A4 is better
    SheetSize = ActiveScreen->m_CurrentSheetDesc->m_Size;       // size in 1/1000 inch
    if( m_Parent->m_Ident == GERBER_FRAME )
    {
        SheetSize = g_Sheet_A4.m_Size;    // size in 1/1000 inch
    }
    SheetSize.x *= m_Parent->m_InternalUnits / 1000;
    SheetSize.y *= m_Parent->m_InternalUnits / 1000;            // size in pixels

    // Get the size of the DC in pixels
    dc->GetSize( &PlotAreaSize.x, &PlotAreaSize.y );

    WinEDA_BasePcbFrame* pcbframe = (WinEDA_BasePcbFrame*) m_Parent;
    pcbframe->GetBoard()->ComputeBoundaryBox();
    /* Compute the PCB size in internal units*/
    userscale = m_PrintParams.m_PrintScale;
    if( userscale == 0 )            //  fit in page
    {
        int extra_margin = 0;    // Margin = 8000/2 units pcb = 0,4 inch
        SheetSize.x = pcbframe->GetBoard()->m_BoundaryBox.GetWidth() + extra_margin;
        SheetSize.y = pcbframe->GetBoard()->m_BoundaryBox.GetHeight() + extra_margin;
        userscale   = 0.99;
    }

    if( (m_PrintParams.m_PrintScale > 1.0)          //  scale > 1 -> Recadrage
       || (m_PrintParams.m_PrintScale == 0) )       //  fit in page
    {
        DrawOffset.x += pcbframe->GetBoard()->m_BoundaryBox.Centre().x;
        DrawOffset.y += pcbframe->GetBoard()->m_BoundaryBox.Centre().y;
    }

    // Calculate a suitable scaling factor
    scaleX = (double) SheetSize.x / (double) PlotAreaSize.x;
    scaleY = (double) SheetSize.y / (double) PlotAreaSize.y;
    scale  = wxMax( scaleX, scaleY ) / userscale; // Use x or y scaling factor, whichever fits on the DC

    // ajust the real draw scale
    double accurate_Xscale, accurate_Yscale;
    dc->SetUserScale( DrawZoom / scale * m_PrintParams.m_XScaleAdjust,
                      DrawZoom / scale * m_PrintParams.m_YScaleAdjust );

    // Compute Accurate scale 1
    {
        int w, h;
        GetPPIPrinter( &w, &h );
        accurate_Xscale = ( (double) ( DrawZoom * w ) ) / (double) PCB_INTERNAL_UNIT;
        accurate_Yscale = ( (double) ( DrawZoom * h ) ) / (double) PCB_INTERNAL_UNIT;

        if( IsPreview() )  // Scale must take in account the DC size in Preview
        {
            // Get the size of the DC in pixels
            dc->GetSize( &PlotAreaSize.x, &PlotAreaSize.y );
            GetPageSizePixels( &w, &h );
            accurate_Xscale *= PlotAreaSize.x;
            accurate_Xscale /= (double) w;
            accurate_Yscale *= PlotAreaSize.y;
            accurate_Yscale /= (double) h;
        }
        accurate_Xscale *= m_PrintParams.m_XScaleAdjust;
        accurate_Yscale *= m_PrintParams.m_YScaleAdjust;
    }

    /* In some cases the plot origin is the centre of the page
     *  when:
     *  - Asked
     *  - scale > 1
     *  - fit in page
     */
    if( m_PrintParams.m_ForceCentered
       || (m_PrintParams.m_PrintScale > 1.0)        //  scale > 1
       || (m_PrintParams.m_PrintScale == 0) )       //  fit in page
    {
        DrawOffset.x -= wxRound( ( (double) PlotAreaSize.x / 2.0 ) * scale );
        DrawOffset.y -= wxRound( ( (double) PlotAreaSize.y / 2.0 ) * scale );
    }
    DrawOffset.x += wxRound( ( (double) SheetSize.x / 2.0 ) *
                             ( m_PrintParams.m_XScaleAdjust - 1.0 ) );
    DrawOffset.y += wxRound( ( (double) SheetSize.y / 2.0 ) *
                             ( m_PrintParams.m_YScaleAdjust - 1.0 ) );

    ActiveScreen->m_DrawOrg = DrawOffset;

    GRResetPenAndBrush( dc );
    if( m_PrintParams.m_Print_Black_and_White )
        GRForceBlackPen( true );


    WinEDA_DrawPanel* panel = m_Parent->DrawPanel;
    EDA_Rect          tmp   = panel->m_ClipBox;

    panel->m_ClipBox.SetOrigin( wxPoint( 0, 0 ) );
    panel->m_ClipBox.SetSize( wxSize( 0x7FFFFF0, 0x7FFFFF0 ) );

    m_Parent->GetBaseScreen()->m_IsPrinting = true;
    int bg_color = g_DrawBgColor;

    if( userscale == 1.0 )
    {
        dc->SetUserScale( accurate_Xscale, accurate_Yscale );
    }

    if( m_PrintParams.m_Print_Sheet_Ref )
        m_Parent->TraceWorkSheet( dc, ActiveScreen, m_PrintParams.m_PenDefaultSize );

    if( printMirror )
    {
        // To plot mirror, we reverse the y axis, and modify the plot y origin
        double sx, sy;

        dc->GetUserScale( &sx, &sy );
        dc->SetAxisOrientation( true, true );
        if( userscale < 1.0 )
            sy /= userscale;

        /* Plot offset y is moved by the y plot area size in order to have
         * the old draw area in the new draw area, because the draw origin has not moved
         * (this is the upper left corner) but the Y axis is reversed, therefore the plotting area
         * is the y coordinate values from  - PlotAreaSize.y to 0 */
        int ysize = (int) ( PlotAreaSize.y / sy );
        DrawOffset.y += ysize;

        /* in order to keep the board position in the sheet
         * (when user scale <= 1) the y offset in moved by the distance between
         * the middle of the page and the middle of the board
         * This is equivalent to put the mirror axis to the board centre
         * for scales > 1, the DrawOffset was already computed to have the board centre
         * to the middle of the page.
         */
        wxPoint pcb_centre = pcbframe->GetBoard()->m_BoundaryBox.Centre();
        if( userscale <= 1.0 )
            DrawOffset.y += pcb_centre.y - (ysize / 2);
        ActiveScreen->m_DrawOrg = DrawOffset;
        panel->m_ClipBox.SetOrigin( wxPoint( -0x7FFFFF, -0x7FFFFF ) );
    }

    g_DrawBgColor = WHITE;

    /* when printing in color mode, we use the graphic OR mode that gives the same look as the screen
     * But because the backgroud is white when printing, we must use a trick:
     * In order to plot on a white background in OR mode we must:
     * 1 - Plot all items in black, this creates a local black backgroud
     * 2 - Plot in OR mode on black "local" background
     */
    if( !m_PrintParams.m_Print_Black_and_White )
    {   // Creates a "local" black background
        GRForceBlackPen( true );
        panel->PrintPage( dc, 0, m_PrintParams.m_PrintMaskLayer, printMirror, &m_PrintParams );
        GRForceBlackPen( false );
    }

    panel->PrintPage( dc, 0, m_PrintParams.m_PrintMaskLayer, printMirror, &m_PrintParams );

    g_DrawBgColor = bg_color;
    m_Parent->GetBaseScreen()->m_IsPrinting = false;
    panel->m_ClipBox = tmp;

    GRForceBlackPen( false );

    ActiveScreen->m_StartVisu = tmp_startvisu;
    ActiveScreen->m_DrawOrg   = old_org;
    ActiveScreen->SetZoom( tmpzoom );
}

/****************/
/* tracepcb.cpp */
/****************/

/*
 *  Redraw the screen.
 */

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "class_drawpanel.h"
#include "drawtxt.h"

#include "gerbview.h"
#include "class_board_design_settings.h"
#include "colors_selection.h"
#include "class_gerber_draw_item.h"
#include "class_GERBER.h"

#ifdef __WINDOWS__
// Blit function seems have problems when scale != 1 and/or offsets
#define AVOID_BLIT_SCALE_BUG    true
#else
#define AVOID_BLIT_SCALE_BUG    false   // not needed on Linux
#endif



/**
 * Function PrintPage (virtual)
 * Used to print the board (on printer, or when creating SVF files).
 * Print the board, but only layers allowed by aPrintMaskLayer
 * @param aDC = the print device context
 * @param aPrintMasklayer = a 32 bits mask: bit n = 1 -> layer n is printed
 * @param aPrintMirrorMode = true to plot mirrored
 * @param aData = a pointer to an optional data (not used here: can be NULL)
 */
void WinEDA_GerberFrame::PrintPage( wxDC* aDC, int aPrintMasklayer,
                                    bool aPrintMirrorMode, void* aData )
{
    // Save current draw options, because print mode has specfic options:
    int             DisplayPolygonsModeImg = g_DisplayPolygonsModeSketch;
    int             visiblemask = GetBoard()->GetVisibleLayers();
    DISPLAY_OPTIONS save_opt    = DisplayOpt;

    // Set draw options for printing:
    GetBoard()->SetVisibleLayers( aPrintMasklayer );
    DisplayOpt.DisplayPcbTrackFill = FILLED;
    DisplayOpt.DisplayDrawItems    = FILLED;
    DisplayOpt.DisplayZonesMode    = 0;
    g_DisplayPolygonsModeSketch    = 0;

    DrawPanel->m_PrintIsMirrored = aPrintMirrorMode;

    GetBoard()->Draw( DrawPanel, aDC, -1, wxPoint( 0, 0 ) );

    DrawPanel->m_PrintIsMirrored = false;

    // Restore draw options:
    GetBoard()->SetVisibleLayers( visiblemask );
    DisplayOpt = save_opt;
    g_DisplayPolygonsModeSketch = DisplayPolygonsModeImg;
}


/*******************************************************************/
void WinEDA_GerberFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
/*******************************************************************/

/* Redraws the full screen, including axis and grid
 */
{
    PCB_SCREEN* screen = (PCB_SCREEN*) GetScreen();

    if( !GetBoard() )
        return;

    wxBusyCursor dummy;

    ActiveScreen = screen;

    GRSetDrawMode( DC, GR_COPY );

    int drawMode = -1;
    switch ( GetDisplayMode() )
    {
        default:
        case 0:
            break;

        case 1:
            drawMode = GR_COPY;
            break;

        case 2:
            drawMode = GR_OR;
            break;
    }
    GetBoard()->Draw( DrawPanel, DC,
                drawMode,                  // this needs to be GR_COPY or GR_OR, set from a toggle button.
                wxPoint( 0, 0 ) );

    // Draw the "background" now, i.e. grid and axis after gerber layers
    // because most of time the actual background is erased by succesive drawings of each gerber layer
    // mainly in COPY mode
    DrawPanel->DrawBackGround( DC );

    if( IsElementVisible( DCODES_VISIBLE ) )
        DrawItemsDCodeID( DC, GR_COPY );

    TraceWorkSheet( DC, screen, 0 );

    if( DrawPanel->ManageCurseur )
        DrawPanel->ManageCurseur( DrawPanel, DC, FALSE );

    DrawPanel->DrawCursor( DC );

    // Display the filename and the layer name (found in the gerber files, if any)
    // relative to the active layer
    UpdateTitleAndInfo();
}


/*
 * Redraw All gerbview layers, using a buffered mode or not
 */
void BOARD::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, int aDrawMode, const wxPoint& aOffset )
{
    // Because Images can be negative (i.e with background filled in color) items are drawn
    // graphic layer per graphic layer, after the background is filled

    wxColour    bgColor = MakeColour( g_DrawBgColor );
    wxBrush     bgBrush( bgColor, wxSOLID );

    int         bitmapWidth, bitmapHeight;
    wxDC*       plotDC = aDC;

#if AVOID_BLIT_SCALE_BUG
    // Blit function used below seems to work OK only with scale = 1 and no offsets
    // at least under Windows
    // Store device context scale and origins:
    double      dc_scalex, dc_scaley;
    wxPoint     dev_org;
    wxPoint     logical_org;

    aDC->GetDeviceOrigin( &dev_org.x, &dev_org.y );
    aDC->GetLogicalOrigin( &logical_org.x, &logical_org.y );
    aDC->GetUserScale( &dc_scalex, &dc_scaley );

    if( aDrawMode != -1 )
    {
        aDC->SetUserScale( 1.0, 1.0 );
        aDC->SetDeviceOrigin( 0, 0 );
        aDC->SetLogicalOrigin( 0, 0 );
    }
#endif

    aPanel->GetClientSize( &bitmapWidth, &bitmapHeight );

    wxBitmap*   layerBitmap = NULL;
    wxBitmap*   screenBitmap = NULL;

    wxMemoryDC  layerDC;        // used sequentially for each gerber layer
    wxMemoryDC  screenDC;

    if( aDrawMode != -1 )
    {
        layerBitmap = new wxBitmap( bitmapWidth, bitmapHeight );
        screenBitmap = new wxBitmap( bitmapWidth, bitmapHeight );
        layerDC.SelectObject( *layerBitmap );
        layerDC.SetBackground( bgBrush );
        layerDC.Clear();
        screenDC.SelectObject( *screenBitmap );
        screenDC.SetBackground( bgBrush );
        screenDC.Clear();

        aPanel->DoPrepareDC(layerDC);
        aPanel->DrawBackGround( &layerDC );
        plotDC = &layerDC;
    }

    bool doBlit = false; // this flag requests an image transfert to actual screen when true.
    for( int layer = 0; layer < 32; layer++ )
    {
        if( !GetBoard()->IsLayerVisible( layer ) )
            continue;

        GERBER_IMAGE* gerber = g_GERBER_List[layer];
        if( gerber == NULL )    // Graphic layer not yet used
            continue;

        if( aDrawMode != -1 )
        {
            // Draw each layer into a bitmap first. Negative Gerber
            // layers are drawn in background color.
            if( gerber->HasNegativeItems() )
            {
                if( doBlit )
                {
#if AVOID_BLIT_SCALE_BUG
                    layerDC.SetUserScale( 1.0, 1.0 );
                    layerDC.SetDeviceOrigin( 0, 0 );
                    layerDC.SetLogicalOrigin( 0, 0 );
#endif
                    if( aDrawMode == GR_COPY )
                    {
                        // Use the layer bitmap itself as a mask when blitting.
                        // The bitmap cannot be referenced by a device context
                        // when setting the mask.
                        layerDC.SelectObject( wxNullBitmap );
                        layerBitmap->SetMask( new wxMask( *layerBitmap, bgColor ) );
                        layerDC.SelectObject( *layerBitmap );
                        screenDC.Blit( 0, 0, bitmapWidth, bitmapHeight, &layerDC, 0, 0, wxCOPY, true );
                    }

                    else if( aDrawMode == GR_OR )
                    {
                        // On Linux with a large screen, this version is much faster and without flicker,
                        // but gives a PCBNEW look where layer colors blend together.  Plus it works
                        // only because the background color is black.  But it may be more useable for some.
                        // The difference is due in part because of the cpu cycles needed to create the
                        // monochromatic bitmap above, and the extra time needed to do bit indexing
                        // into the monochromatic bitmap on the blit above.
                        screenDC.Blit( 0, 0, bitmapWidth, bitmapHeight, &layerDC, 0, 0, wxOR );
                    }
                }
                doBlit = false;
                layerDC.Clear();
            }
        }

#if AVOID_BLIT_SCALE_BUG
        layerDC.SetUserScale( dc_scalex, dc_scaley );
        layerDC.SetDeviceOrigin( dev_org.x, dev_org.y );
        layerDC.SetLogicalOrigin( logical_org.x, logical_org.y );
#endif
        if( gerber->m_ImageNegative )
        {
            // Draw background negative (i.e. in graphic layer color) for negative images.
            int color = GetBoard()->GetLayerColor( layer );

            GRSetDrawMode( &layerDC, GR_COPY );

            EDA_Rect* cbox = &aPanel->m_ClipBox;

            GRSFilledRect( cbox, plotDC, cbox->GetX(), cbox->GetY(),
                           cbox->GetRight(), cbox->GetBottom(),
                           0, color, color );

            GRSetDrawMode( plotDC, GR_COPY );
            doBlit = true;
        }

        int dcode_highlight = 0;
        if( layer == m_PcbFrame->GetScreen()->m_Active_Layer )
            dcode_highlight = gerber->m_Selected_Tool;

        int layerdrawMode = GR_COPY;
        if( aDrawMode == GR_OR && !gerber->HasNegativeItems() )
            layerdrawMode = GR_OR;
        for( BOARD_ITEM* item = GetBoard()->m_Drawings; item; item = item->Next() )
        {
            GERBER_DRAW_ITEM* gerb_item = (GERBER_DRAW_ITEM*) item;
            if( gerb_item->GetLayer() != layer )
                continue;

            int drawMode = layerdrawMode;
            if( dcode_highlight && dcode_highlight == gerb_item->m_DCode )
                drawMode |= GR_SURBRILL;

            gerb_item->Draw( aPanel, plotDC, drawMode );
            doBlit = true;
        }

    }

    if( doBlit && aDrawMode != -1 )     // Blit is used only if aDrawMode >= 0
    {
    // this is the last transfert to screenDC
    // If there are no negative items, this is the only one
    #if AVOID_BLIT_SCALE_BUG
        if( aDrawMode != -1 )
        {
            layerDC.SetUserScale( 1.0, 1.0 );
            layerDC.SetDeviceOrigin( 0, 0 );
            layerDC.SetLogicalOrigin( 0, 0 );
        }
#endif
        if( aDrawMode == GR_COPY )
        {
            layerDC.SelectObject( wxNullBitmap );
            layerBitmap->SetMask( new wxMask( *layerBitmap, bgColor ) );

            layerDC.SelectObject( *layerBitmap );

            screenDC.Blit( 0, 0, bitmapWidth, bitmapHeight, &layerDC, 0, 0, wxCOPY, true );
        }

        else if( aDrawMode == GR_OR )
            screenDC.Blit( 0, 0, bitmapWidth, bitmapHeight, &layerDC, 0, 0, wxOR, false );
    }

    if( aDrawMode != -1 )
    {
        aDC->Blit( 0, 0, bitmapWidth, bitmapHeight, &screenDC, 0, 0, wxCOPY );
#if AVOID_BLIT_SCALE_BUG
        // Restore scale and offsets values:
        aDC->SetUserScale( dc_scalex, dc_scaley );
        aDC->SetDeviceOrigin( dev_org.x, dev_org.y );
        aDC->SetLogicalOrigin( logical_org.x, logical_org.y );
#endif
        delete layerBitmap;
        delete screenBitmap;
    }

    m_PcbFrame->GetScreen()->ClrRefreshReq();
}

/* Function DrawItemsDCodeID
 * Draw the DCode value (if exists) corresponding to gerber item
 * Polygons do not have a DCode
 */
void WinEDA_GerberFrame::DrawItemsDCodeID( wxDC* aDC, int aDrawMode )
{
    wxPoint     pos;
    int         width, orient;
    wxString    Line;

    GRSetDrawMode( aDC, aDrawMode );
    BOARD_ITEM* item = GetBoard()->m_Drawings;
    for( ; item != NULL; item = item->Next() )
    {
        GERBER_DRAW_ITEM* gerb_item = (GERBER_DRAW_ITEM*) item;
        if( GetBoard()->IsLayerVisible( gerb_item->GetLayer() ) == false )
            continue;
        if( gerb_item->m_DCode <= 0 )
            continue;

        if( gerb_item->m_Flashed || gerb_item->m_Shape == GBR_ARC )
            pos = gerb_item->m_Start;
        else
        {
            pos.x = (gerb_item->m_Start.x + gerb_item->m_End.x) / 2;
            pos.y = (gerb_item->m_Start.y + gerb_item->m_End.y) / 2;
        }

        pos = gerb_item->GetABPosition( pos );

        Line.Printf( wxT( "D%d" ), gerb_item->m_DCode );

        if( gerb_item->GetDcodeDescr() )
            width  = gerb_item->GetDcodeDescr()->GetShapeDim( gerb_item );
        else
            width  = MIN( gerb_item->m_Size.x, gerb_item->m_Size.y );

        orient = TEXT_ORIENT_HORIZ;
        if( gerb_item->m_Flashed )
        {
            // A reasonnable size for text is width/3 because most of time this text has 3 chars.
            width /= 3;
        }
        else        // this item is a line
        {
            wxPoint delta = gerb_item->m_Start - gerb_item->m_End;
            if( abs( delta.x ) < abs( delta.y ) )
                orient = TEXT_ORIENT_VERT;
            // A reasonnable size for text is width/2 because text needs margin below and above it.
            // a margin = width/4 seems good
            width /= 2;
        }

        int color = g_ColorsSettings.GetItemColor( DCODES_VISIBLE );

        DrawGraphicText( DrawPanel, aDC,
                         pos, (EDA_Colors) color, Line,
                         orient, wxSize( width, width ),
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                         0, false, false );
    }
}


/* Virtual fonction needed by the PCB_SCREEN class derived from BASE_SCREEN
 * this is a virtual pure function in BASE_SCREEN
 * do nothing in gerbview
 * could be removed later
 */
void PCB_SCREEN::ClearUndoORRedoList( UNDO_REDO_CONTAINER&, int )
{
}

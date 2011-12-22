/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file draw_gerber_screen.cpp
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


void GERBVIEW_FRAME::PrintPage( wxDC* aDC, int aPrintMasklayer,
                                bool aPrintMirrorMode, void* aData )
{
    // Save current draw options, because print mode has specific options:
    int             DisplayPolygonsModeImg = g_DisplayPolygonsModeSketch;
    int             visiblemask = GetBoard()->GetVisibleLayers();
    DISPLAY_OPTIONS save_opt    = DisplayOpt;

    // Set draw options for printing:
    GetBoard()->SetVisibleLayers( aPrintMasklayer );
    DisplayOpt.DisplayPcbTrackFill = FILLED;
    DisplayOpt.DisplayDrawItems    = FILLED;
    DisplayOpt.DisplayZonesMode    = 0;
    g_DisplayPolygonsModeSketch    = 0;

    m_canvas->m_PrintIsMirrored = aPrintMirrorMode;

    GetBoard()->Draw( m_canvas, aDC, -1, wxPoint( 0, 0 ) );

    m_canvas->m_PrintIsMirrored = false;

    // Restore draw options:
    GetBoard()->SetVisibleLayers( visiblemask );
    DisplayOpt = save_opt;
    g_DisplayPolygonsModeSketch = DisplayPolygonsModeImg;
}


void GERBVIEW_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    PCB_SCREEN* screen = (PCB_SCREEN*) GetScreen();

    if( !GetBoard() )
        return;

    wxBusyCursor dummy;

    int          drawMode = -1;

    switch( GetDisplayMode() )
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

    // Draw according to the current setting.  This needs to be GR_COPY or GR_OR.
    GetBoard()->Draw( m_canvas, DC, drawMode, wxPoint( 0, 0 ) );

    // Draw the "background" now, i.e. grid and axis after gerber layers
    // because most of time the actual background is erased by successive drawings of each gerber
    // layer mainly in COPY mode
    m_canvas->DrawBackGround( DC );

    if( IsElementVisible( DCODES_VISIBLE ) )
        DrawItemsDCodeID( DC, GR_COPY );

    TraceWorkSheet( DC, screen, 0 );

    if( m_canvas->IsMouseCaptured() )
        m_canvas->m_mouseCaptureCallback( m_canvas, DC, wxDefaultPosition, false );

    m_canvas->DrawCrossHair( DC );

    // Display the filename and the layer name (found in the gerber files, if any)
    // relative to the active layer
    UpdateTitleAndInfo();
}


/*
 * Redraw All GerbView layers, using a buffered mode or not
 */
void BOARD::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, int aDrawMode, const wxPoint& aOffset )
{
    // Because Images can be negative (i.e with background filled in color) items are drawn
    // graphic layer per graphic layer, after the background is filled
    // to a temporary bitmap
    // at least when aDrawMode = GR_COPY or aDrawMode = GR_OR
    // If aDrawMode = -1, items are drawn to the main screen, and therefore
    // artifacts can happen with negative items or negative images

    wxColour bgColor = MakeColour( g_DrawBgColor );
    wxBrush  bgBrush( bgColor, wxSOLID );

    GERBVIEW_FRAME* gerbFrame = (GERBVIEW_FRAME*) aPanel->GetParent();

    int      bitmapWidth, bitmapHeight;
    wxDC*    plotDC = aDC;

    aPanel->GetClientSize( &bitmapWidth, &bitmapHeight );

    wxBitmap*  layerBitmap  = NULL;
    wxBitmap*  screenBitmap = NULL;
    wxMemoryDC layerDC;         // used sequentially for each gerber layer
    wxMemoryDC screenDC;

    // When each image must be drawn using GR_OR (transparency mode)
    // or GR_COPY (stacked mode) we must use a temporary bitmap
    // to draw gerber images.
    // this is due to negative objects (drawn using background color) that create artifacts
    // on other images when drawn on screen
    bool useBufferBitmap = false;

    if( (aDrawMode == GR_COPY) || ( aDrawMode == GR_OR ) )
        useBufferBitmap = true;

    // these parameters are saved here, because they are modified
    // and restored later
    EDA_RECT   drawBox = aPanel->m_ClipBox;
    double scale;
    aDC->GetUserScale(&scale, &scale);
    wxPoint dev_org = aDC->GetDeviceOrigin();
    wxPoint logical_org = aDC->GetLogicalOrigin( );


    if( useBufferBitmap )
    {
        layerBitmap  = new wxBitmap( bitmapWidth, bitmapHeight );
        screenBitmap = new wxBitmap( bitmapWidth, bitmapHeight );
        layerDC.SelectObject( *layerBitmap );
        aPanel->DoPrepareDC( layerDC );
        aPanel->m_ClipBox = drawBox;
        layerDC.SetBackground( bgBrush );
        layerDC.SetBackgroundMode( wxSOLID );
        layerDC.Clear();

        screenDC.SelectObject( *screenBitmap );
        screenDC.SetBackground( bgBrush );
        screenDC.SetBackgroundMode( wxSOLID );
        screenDC.Clear();

        plotDC = &layerDC;
    }

    bool doBlit = false; // this flag requests an image transfer to actual screen when true.

    bool end = false;

    for( int layer = 0; !end; layer++ )
    {
        int active_layer = gerbFrame->getActiveLayer();

        if( layer == active_layer ) // active layer will be drawn after other layers
            continue;

        if( layer == 32 )   // last loop: draw active layer
        {
            end   = true;
            layer = active_layer;
        }

        if( !GetBoard()->IsLayerVisible( layer ) )
            continue;

        GERBER_IMAGE* gerber = g_GERBER_List[layer];

        if( gerber == NULL )    // Graphic layer not yet used
            continue;

        if( useBufferBitmap )
        {
            // Draw each layer into a bitmap first. Negative Gerber
            // layers are drawn in background color.
            if( gerber->HasNegativeItems() &&  doBlit )
            {
                // Set Device origin, logical origin and scale to default values
                // This is needed by Blit function when using a mask.
                // Beside, for Blit call, both layerDC and screenDc must have the same settings
                layerDC.SetDeviceOrigin(0,0);
                layerDC.SetLogicalOrigin( 0, 0 );
                layerDC.SetUserScale( 1, 1 );

                if( aDrawMode == GR_COPY )
                {
                    // Use the layer bitmap itself as a mask when blitting.  The bitmap
                    // cannot be referenced by a device context when setting the mask.
                    layerDC.SelectObject( wxNullBitmap );
                    layerBitmap->SetMask( new wxMask( *layerBitmap, bgColor ) );
                    layerDC.SelectObject( *layerBitmap );
                    screenDC.Blit( 0, 0, bitmapWidth, bitmapHeight, &layerDC, 0, 0, wxCOPY, true );
                }
                else if( aDrawMode == GR_OR )
                {
                    // On Linux with a large screen, this version is much faster and without
                    // flicker, but gives a Pcbnew look where layer colors blend together.
                    // Plus it works only because the background color is black.  But it may
                    // be more usable for some.  The difference is due in part because of
                    // the cpu cycles needed to create the monochromatic bitmap above, and
                    // the extra time needed to do bit indexing into the monochromatic bitmap
                    // on the blit above.
                    screenDC.Blit( 0, 0, bitmapWidth, bitmapHeight, &layerDC, 0, 0, wxOR );
                }
                // Restore actual values and clear bitmap for next drawing
                layerDC.SetDeviceOrigin( dev_org.x, dev_org.y );
                layerDC.SetLogicalOrigin( logical_org.x, logical_org.y );
                layerDC.SetUserScale( scale, scale );
                layerDC.SetBackground( bgBrush );
                layerDC.SetBackgroundMode( wxSOLID );
                layerDC.Clear();

                doBlit = false;
            }

        }

        if( gerber->m_ImageNegative )
        {
            // Draw background negative (i.e. in graphic layer color) for negative images.
            int color = GetBoard()->GetLayerColor( layer );

            GRSetDrawMode( &layerDC, GR_COPY );
            GRFilledRect( &drawBox, plotDC, drawBox.GetX(), drawBox.GetY(),
                          drawBox.GetRight(), drawBox.GetBottom(),
                          0, color, color );

            GRSetDrawMode( plotDC, GR_COPY );
            doBlit = true;
        }

        int dcode_highlight = 0;

        if( layer == gerbFrame->getActiveLayer() )
            dcode_highlight = gerber->m_Selected_Tool;

        int layerdrawMode = GR_COPY;

        if( aDrawMode == GR_OR && !gerber->HasNegativeItems() )
            layerdrawMode = GR_OR;

        // Now we can draw the current layer to the bitmap buffer
        // When needed, the previous bitmap is already copied to the screen buffer.
        for( BOARD_ITEM* item = GetBoard()->m_Drawings; item; item = item->Next() )
        {
            GERBER_DRAW_ITEM* gerb_item = (GERBER_DRAW_ITEM*) item;

            if( gerb_item->GetLayer() != layer )
                continue;

            int drawMode = layerdrawMode;

            if( dcode_highlight && dcode_highlight == gerb_item->m_DCode )
                drawMode |= GR_HIGHLIGHT;

            gerb_item->Draw( aPanel, plotDC, drawMode );
            doBlit = true;
        }
    }

    if( doBlit && useBufferBitmap )     // Blit is used only if aDrawMode >= 0
    {
        // For this Blit call, layerDC and screenDC must have the same settings
        // So we set device origin, logical origin and scale to default values
        // in layerDC
        layerDC.SetDeviceOrigin(0,0);
        layerDC.SetLogicalOrigin( 0, 0 );
        layerDC.SetUserScale( 1, 1 );

        // this is the last transfer to screenDC.  If there are no negative items, this is
        // the only one
        if( aDrawMode == GR_COPY )
        {
            layerDC.SelectObject( wxNullBitmap );
            layerBitmap->SetMask( new wxMask( *layerBitmap, bgColor ) );
            layerDC.SelectObject( *layerBitmap );
            screenDC.Blit( 0, 0, bitmapWidth, bitmapHeight, &layerDC, 0, 0, wxCOPY, true );

        }
        else if( aDrawMode == GR_OR )
        {
            screenDC.Blit( 0, 0, bitmapWidth, bitmapHeight, &layerDC, 0, 0, wxOR );
        }
    }

    if( useBufferBitmap )
    {
        // For this Blit call, aDC and screenDC must have the same settings
        // So we set device origin, logical origin and scale to default values
        // in aDC
        aDC->SetDeviceOrigin( 0, 0);
        aDC->SetLogicalOrigin( 0, 0 );
        aDC->SetUserScale( 1, 1 );

        aDC->Blit( 0, 0, bitmapWidth, bitmapHeight, &screenDC, 0, 0, wxCOPY );

        // Restore aDC values
        aDC->SetDeviceOrigin(dev_org.x, dev_org.y);
        aDC->SetLogicalOrigin( logical_org.x, logical_org.y );
        aDC->SetUserScale( scale, scale );

        layerDC.SelectObject( wxNullBitmap );
        screenDC.SelectObject( wxNullBitmap );
        delete layerBitmap;
        delete screenBitmap;
    }
}


void GERBVIEW_FRAME::DrawItemsDCodeID( wxDC* aDC, int aDrawMode )
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
        {
            pos = gerb_item->m_Start;
        }
        else
        {
            pos.x = (gerb_item->m_Start.x + gerb_item->m_End.x) / 2;
            pos.y = (gerb_item->m_Start.y + gerb_item->m_End.y) / 2;
        }

        pos = gerb_item->GetABPosition( pos );

        Line.Printf( wxT( "D%d" ), gerb_item->m_DCode );

        if( gerb_item->GetDcodeDescr() )
            width = gerb_item->GetDcodeDescr()->GetShapeDim( gerb_item );
        else
            width = MIN( gerb_item->m_Size.x, gerb_item->m_Size.y );

        orient = TEXT_ORIENT_HORIZ;

        if( gerb_item->m_Flashed )
        {
            // A reasonable size for text is width/3 because most of time this text has 3 chars.
            width /= 3;
        }
        else        // this item is a line
        {
            wxPoint delta = gerb_item->m_Start - gerb_item->m_End;

            if( abs( delta.x ) < abs( delta.y ) )
                orient = TEXT_ORIENT_VERT;

            // A reasonable size for text is width/2 because text needs margin below and above it.
            // a margin = width/4 seems good
            width /= 2;
        }

        int color = g_ColorsSettings.GetItemColor( DCODES_VISIBLE );

        DrawGraphicText( m_canvas, aDC, pos, (EDA_Colors) color, Line,
                         orient, wxSize( width, width ),
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                         0, false, false );
    }
}


/* Virtual function needed by the PCB_SCREEN class derived from BASE_SCREEN
 * this is a virtual pure function in BASE_SCREEN
 * do nothing in GerbView
 * could be removed later
 */
void PCB_SCREEN::ClearUndoORRedoList( UNDO_REDO_CONTAINER&, int )
{
}


/* dummy_functions
 *
 *  These functions are used in some classes.
 *  they are useful in Pcbnew, but have no meaning or are never used
 *  in CvPcb or GerbView.
 *  but they must exist because they appear in some classes, and here, no nothing.
 */

TRACK* MarkTrace( BOARD* aPcb,
                  TRACK* aStartSegm,
                  int*   aSegmCount,
                  int*   aTrackLen,
                  int*   aLenDie,
                  bool   aReorder )
{
    return NULL;
}

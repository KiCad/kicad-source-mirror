/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jpierre.charras at wanadoo
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2013 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <fctsys.h>
#include <gr_basic.h>
#include <common.h>
#include <class_drawpanel.h>
#include <drawtxt.h>
#include <base_units.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <colors_selection.h>
#include <class_gerber_draw_item.h>
#include <class_GERBER.h>
#include <printout_controler.h>


void GERBVIEW_FRAME::PrintPage( wxDC* aDC, LSET aPrintMasklayer,
                                bool aPrintMirrorMode, void* aData )
{
    // Save current draw options, because print mode has specific options:
    GBR_DISPLAY_OPTIONS imgDisplayOptions = m_DisplayOptions;
    std::bitset <GERBER_DRAWLAYERS_COUNT> printLayersMask = GetGerberLayout()->GetPrintableLayers();

    // Set draw options for printing:
    m_DisplayOptions.m_DisplayFlashedItemsFill = true;
    m_DisplayOptions.m_DisplayLinesFill = true;
    m_DisplayOptions.m_DisplayPolygonsFill = true;
    m_DisplayOptions.m_DisplayDCodes = false;
    m_DisplayOptions.m_IsPrinting = true;

    PRINT_PARAMETERS* printParameters = (PRINT_PARAMETERS*)aData;
    std::bitset <GERBER_DRAWLAYERS_COUNT> printCurrLayerMask;
    printCurrLayerMask.reset();
    printCurrLayerMask.set(printParameters->m_Flags);   // m_Flags contains the draw layer number
    GetGerberLayout()->SetPrintableLayers( printCurrLayerMask );
    m_canvas->SetPrintMirrored( aPrintMirrorMode );
    bool printBlackAndWhite = printParameters && printParameters->m_Print_Black_and_White;

    GetGerberLayout()->Draw( m_canvas, aDC, UNSPECIFIED_DRAWMODE,
                             wxPoint( 0, 0 ), printBlackAndWhite );

    m_canvas->SetPrintMirrored( false );

    // Restore draw options:
    GetGerberLayout()->SetPrintableLayers( printLayersMask );
    m_DisplayOptions = imgDisplayOptions;
}


void GERBVIEW_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    GBR_SCREEN* screen = (GBR_SCREEN*) GetScreen();

    if( !GetGerberLayout() )
        return;

    wxBusyCursor dummy;

    GR_DRAWMODE drawMode = UNSPECIFIED_DRAWMODE;

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
    GetGerberLayout()->Draw( m_canvas, DC, drawMode, wxPoint( 0, 0 ) );

    // Draw the "background" now, i.e. grid and axis after gerber layers
    // because most of time the actual background is erased by successive drawings of each gerber
    // layer mainly in COPY mode
    m_canvas->DrawBackGround( DC );

    if( IsElementVisible( DCODES_VISIBLE ) )
        DrawItemsDCodeID( DC, GR_COPY );

    DrawWorkSheet( DC, screen, 0, IU_PER_MILS, wxEmptyString );

    if( m_canvas->IsMouseCaptured() )
        m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );

    m_canvas->DrawCrossHair( DC );

    // Display the filename and the layer name (found in the gerber files, if any)
    // relative to the active layer
    UpdateTitleAndInfo();
}


/*
 * Redraw All GerbView layers, using a buffered mode or not
 */
void GBR_LAYOUT::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode,
                       const wxPoint& aOffset, bool aPrintBlackAndWhite )
{
    // Because Images can be negative (i.e with background filled in color) items are drawn
    // graphic layer per graphic layer, after the background is filled
    // to a temporary bitmap
    // at least when aDrawMode = GR_COPY or aDrawMode = GR_OR
    // If aDrawMode = UNSPECIFIED_DRAWMODE, items are drawn to the main screen, and therefore
    // artifacts can happen with negative items or negative images

    wxColour bgColor = MakeColour( g_DrawBgColor );

#if wxCHECK_VERSION( 3, 0, 0 )
    wxBrush  bgBrush( bgColor, wxBRUSHSTYLE_SOLID );
#else
    wxBrush  bgBrush( bgColor, wxSOLID );
#endif

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

#ifndef __WXMAC__
    // Can't work with MAC
    // Don't try this with retina display
    if( (aDrawMode == GR_COPY) || ( aDrawMode == GR_OR ) )
        useBufferBitmap = true;
#endif

    // these parameters are saved here, because they are modified
    // and restored later
    EDA_RECT drawBox = *aPanel->GetClipBox();
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
        aPanel->SetClipBox( drawBox );
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

    for( int layer = 0; !end; ++layer )
    {
        int active_layer = gerbFrame->getActiveLayer();

        if( layer == active_layer ) // active layer will be drawn after other layers
            continue;

        if( layer == GERBER_DRAWLAYERS_COUNT )   // last loop: draw active layer
        {
            end   = true;
            layer = active_layer;
        }

        if( !gerbFrame->IsLayerVisible( layer ) )
            continue;

        GERBER_IMAGE* gerber = g_GERBER_List[layer];

        if( gerber == NULL )    // Graphic layer not yet used
            continue;

        EDA_COLOR_T color = gerbFrame->GetLayerColor( layer );

        // Force black and white draw mode on request:
        if( aPrintBlackAndWhite )
            gerbFrame->SetLayerColor( layer, g_DrawBgColor == BLACK ? WHITE : BLACK );

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
            EDA_COLOR_T color = gerbFrame->GetLayerColor( layer );

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

        GR_DRAWMODE layerdrawMode = GR_COPY;

        if( aDrawMode == GR_OR && !gerber->HasNegativeItems() )
            layerdrawMode = GR_OR;

        // Now we can draw the current layer to the bitmap buffer
        // When needed, the previous bitmap is already copied to the screen buffer.
        for( GERBER_DRAW_ITEM* item = gerbFrame->GetItemsList(); item; item = item->Next() )
        {
            if( item->GetLayer() != layer )
                continue;

            GR_DRAWMODE drawMode = layerdrawMode;

            if( dcode_highlight && dcode_highlight == item->m_DCode )
                DrawModeAddHighlight( &drawMode);

            item->Draw( aPanel, plotDC, drawMode, wxPoint(0,0) );
            doBlit = true;
        }

        if( aPrintBlackAndWhite )
            gerbFrame->SetLayerColor( layer, color );
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


void GERBVIEW_FRAME::DrawItemsDCodeID( wxDC* aDC, GR_DRAWMODE aDrawMode )
{
    wxPoint     pos;
    int         width;
    double      orient;
    wxString    Line;

    GRSetDrawMode( aDC, aDrawMode );

    for( GERBER_DRAW_ITEM* item = GetItemsList(); item != NULL; item = item->Next() )
    {
        if( IsLayerVisible( item->GetLayer() ) == false )
            continue;

        if( item->m_DCode <= 0 )
            continue;

        if( item->m_Flashed || item->m_Shape == GBR_ARC )
        {
            pos = item->m_Start;
        }
        else
        {
            pos.x = (item->m_Start.x + item->m_End.x) / 2;
            pos.y = (item->m_Start.y + item->m_End.y) / 2;
        }

        pos = item->GetABPosition( pos );

        Line.Printf( wxT( "D%d" ), item->m_DCode );

        if( item->GetDcodeDescr() )
            width = item->GetDcodeDescr()->GetShapeDim( item );
        else
            width = std::min( item->m_Size.x, item->m_Size.y );

        orient = TEXT_ORIENT_HORIZ;

        if( item->m_Flashed )
        {
            // A reasonable size for text is width/3 because most of time this text has 3 chars.
            width /= 3;
        }
        else        // this item is a line
        {
            wxPoint delta = item->m_Start - item->m_End;

            if( abs( delta.x ) < abs( delta.y ) )
                orient = TEXT_ORIENT_VERT;

            // A reasonable size for text is width/2 because text needs margin below and above it.
            // a margin = width/4 seems good
            width /= 2;
        }

        int color = GetVisibleElementColor( DCODES_VISIBLE );

        DrawGraphicText( m_canvas->GetClipBox(), aDC, pos, (EDA_COLOR_T) color, Line,
                         orient, wxSize( width, width ),
                         GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                         0, false, false );
    }
}

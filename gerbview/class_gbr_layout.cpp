/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012-2014 Jean-Pierre Charras  jp.charras at wanadoo.fr
 * Copyright (C) 1992-2014 KiCad Developers, see change_log.txt for contributors.
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
 * @file class_gbr_layout.cpp
 * @brief  GBR_LAYOUT class functions.
 */

#include <fctsys.h>
#include <gr_basic.h>
#include <drawtxt.h>
#include <gerbview_frame.h>
#include <class_drawpanel.h>
#include <class_gbr_layout.h>
#include <class_gerber_file_image.h>
#include <class_gerber_file_image_list.h>

GBR_LAYOUT::GBR_LAYOUT()
{
}


GBR_LAYOUT::~GBR_LAYOUT()
{
}

// Accessor to the list of gerber files (and drill files) images
GERBER_FILE_IMAGE_LIST* GBR_LAYOUT::GetImagesList()
{
    return &GERBER_FILE_IMAGE_LIST::GetImagesList();
}


bool GBR_LAYOUT::IsLayerPrintable( int aLayer ) const
{
    for( unsigned ii = 0; ii < m_printLayersList.size(); ++ii )
    {
        if( m_printLayersList[ii] == aLayer )
            return true;
    }

    return false;
}


EDA_RECT GBR_LAYOUT::ComputeBoundingBox()
{
    EDA_RECT bbox;
    bool first_item = true;

    for( unsigned layer = 0; layer < GetImagesList()->ImagesMaxCount(); ++layer )
    {
        GERBER_FILE_IMAGE* gerber = GetImagesList()->GetGbrImage( layer );

        if( gerber == NULL )    // Graphic layer not yet used
            continue;

        for( GERBER_DRAW_ITEM* item = gerber->GetItemsList(); item; item = item->Next() )
        {
            if( first_item )
            {
                bbox = item->GetBoundingBox();
                first_item = false;
            }
            else
                bbox.Merge( item->GetBoundingBox() );
        }
    }

    SetBoundingBox( bbox );
    return bbox;
}


// Redraw All GerbView layers, using a buffered mode or not
void GBR_LAYOUT::Draw( EDA_DRAW_PANEL* aPanel, wxDC* aDC, GR_DRAWMODE aDrawMode,
                       const wxPoint& aOffset, GBR_DISPLAY_OPTIONS* aDisplayOptions )
{
    GERBVIEW_FRAME* gerbFrame = (GERBVIEW_FRAME*) aPanel->GetParent();

    // Collect the highlight selections
    wxString cmpHighlight;

    if( gerbFrame->m_SelComponentBox->GetSelection() > 0 )
        cmpHighlight = gerbFrame->m_SelComponentBox->GetStringSelection();

    wxString netHighlight;

    if( gerbFrame->m_SelNetnameBox->GetSelection() > 0 )
        netHighlight = gerbFrame->m_SelNetnameBox->GetStringSelection();

    wxString aperAttrHighlight = gerbFrame->m_SelAperAttributesBox->GetStringSelection();

    if( gerbFrame->m_SelAperAttributesBox->GetSelection() > 0 )
        aperAttrHighlight = gerbFrame->m_SelAperAttributesBox->GetStringSelection();


    // Because Images can be negative (i.e with background filled in color) items are drawn
    // graphic layer per graphic layer, after the background is filled
    // to a temporary bitmap
    // at least when aDrawMode = GR_COPY or aDrawMode = GR_OR
    // If aDrawMode = UNSPECIFIED_DRAWMODE, items are drawn to the main screen, and therefore
    // artifacts can happen with negative items or negative images

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

    COLOR4D bgColor = aDisplayOptions->m_BgDrawColor;
    wxBrush  bgBrush( bgColor.ToColour(), wxBRUSHSTYLE_SOLID );

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

    // Draw graphic layers from bottom to top, and the active layer is on the top of others.
    // In non transparent modes, the last layer drawn masks others layers
    for( int layer = GERBER_DRAWLAYERS_COUNT-1; !end; --layer )
    {
        int active_layer = gerbFrame->getActiveLayer();

        if( layer == active_layer ) // active layer will be drawn after other layers
            continue;

        if( layer < 0 )   // last loop: draw active layer
        {
            end   = true;
            layer = active_layer;
        }

        GERBER_FILE_IMAGE* gerber = GetImagesList()->GetGbrImage( layer );

        if( gerber == NULL )    // Graphic layer not yet used
            continue;

        if( aDisplayOptions->m_IsPrinting )
            gerber->m_IsVisible = IsLayerPrintable( layer );
        else
            gerber->m_IsVisible = gerbFrame->IsLayerVisible( layer );

        if( !gerber->m_IsVisible )
            continue;

        gerber->m_PositiveDrawColor = gerbFrame->GetLayerColor( layer );

       // Force black and white draw mode on request:
        if( aDisplayOptions->m_ForceBlackAndWhite )
            gerber->m_PositiveDrawColor = ( aDisplayOptions->m_BgDrawColor == BLACK ) ? WHITE : BLACK;

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
                    layerBitmap->SetMask( new wxMask( *layerBitmap, bgColor.ToColour() ) );
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
            COLOR4D neg_color = gerber->GetPositiveDrawColor();

            GRSetDrawMode( &layerDC, GR_COPY );
            GRFilledRect( &drawBox, plotDC, drawBox.GetX(), drawBox.GetY(),
                          drawBox.GetRight(), drawBox.GetBottom(),
                          0, neg_color, neg_color );

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
        for( GERBER_DRAW_ITEM* item = gerber->GetItemsList(); item; item = item->Next() )
        {
            if( item->GetLayer() != layer )
                continue;

            GR_DRAWMODE drawMode = layerdrawMode;

            if( dcode_highlight && dcode_highlight == item->m_DCode )
                DrawModeAddHighlight( &drawMode);

            if( !aperAttrHighlight.IsEmpty() && item->GetDcodeDescr() &&
                item->GetDcodeDescr()->m_AperFunction == aperAttrHighlight )
                DrawModeAddHighlight( &drawMode);

            if( !cmpHighlight.IsEmpty() &&
                cmpHighlight == item->GetNetAttributes().m_Cmpref )
                DrawModeAddHighlight( &drawMode);

            if( !netHighlight.IsEmpty() &&
                netHighlight == item->GetNetAttributes().m_Netname )
                DrawModeAddHighlight( &drawMode);

            item->Draw( aPanel, plotDC, drawMode, wxPoint(0,0), aDisplayOptions );
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
            layerBitmap->SetMask( new wxMask( *layerBitmap, bgColor.ToColour() ) );
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


void GBR_LAYOUT::DrawItemsDCodeID( EDA_DRAW_PANEL* aPanel, wxDC* aDC,
                                   GR_DRAWMODE aDrawMode, COLOR4D aDrawColor )
{
    wxPoint     pos;
    int         width;
    wxString    Line;

    GRSetDrawMode( aDC, aDrawMode );

    for( unsigned layer = 0; layer < GetImagesList()->ImagesMaxCount(); ++layer )
    {
        GERBER_FILE_IMAGE* gerber = GetImagesList()->GetGbrImage( layer );

        if( gerber == NULL )    // Graphic layer not yet used
            continue;

        if( ! gerber->m_IsVisible )
            continue;

        for( GERBER_DRAW_ITEM* item = gerber->GetItemsList(); item != NULL; item = item->Next() )
        {

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

            double orient = TEXT_ANGLE_HORIZ;

            if( item->m_Flashed )
            {
                // A reasonable size for text is width/3 because most of time this text has 3 chars.
                width /= 3;
            }
            else        // this item is a line
            {
                wxPoint delta = item->m_Start - item->m_End;

                if( abs( delta.x ) < abs( delta.y ) )
                    orient = TEXT_ANGLE_VERT;

                // A reasonable size for text is width/2 because text needs margin below and above it.
                // a margin = width/4 seems good
                width /= 2;
            }

            DrawGraphicText( aPanel->GetClipBox(), aDC, pos, aDrawColor, Line,
                             orient, wxSize( width, width ),
                             GR_TEXT_HJUSTIFY_CENTER, GR_TEXT_VJUSTIFY_CENTER,
                             0, false, false );
        }
    }
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jpierre.charras at wanadoo
 * Copyright (C) 2013-2015 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <class_gerber_file_image.h>
#include <class_gerber_file_image_list.h>
#include <printout_controler.h>


void GERBVIEW_FRAME::PrintPage( wxDC* aDC, LSET aPrintMasklayer,
                                bool aPrintMirrorMode, void* aData )
{
    wxCHECK_RET( aData != NULL, wxT( "aData cannot be NULL." ) );

    // Save current draw options, because print mode has specific options:
    GBR_DISPLAY_OPTIONS imgDisplayOptions = m_DisplayOptions;
    std::bitset <GERBER_DRAWLAYERS_COUNT> printLayersMask = GetGerberLayout()->GetPrintableLayers();

    // Set draw options for printing:
    m_DisplayOptions.m_DisplayFlashedItemsFill = true;
    m_DisplayOptions.m_DisplayLinesFill = true;
    m_DisplayOptions.m_DisplayPolygonsFill = true;
    m_DisplayOptions.m_DisplayDCodes = false;
    m_DisplayOptions.m_IsPrinting = true;

    PRINT_PARAMETERS* printParameters = (PRINT_PARAMETERS*) aData;

    // Find the layer to be printed
    int page = printParameters->m_Flags;    // contains the page number (not necessarily graphic layer number)
    int layer = 0;

    // Find the layer number for the printed page (search through the mask and count bits)
    while( page > 0 )
    {
        if( printLayersMask[layer++] )
            --page;
    }
    --layer;

    std::bitset <GERBER_DRAWLAYERS_COUNT> printCurrLayerMask;
    printCurrLayerMask.reset();
    printCurrLayerMask.set( layer );
    GetGerberLayout()->SetPrintableLayers( printCurrLayerMask );
    m_canvas->SetPrintMirrored( aPrintMirrorMode );
    bool printBlackAndWhite = printParameters->m_Print_Black_and_White;

    GetGerberLayout()->Draw( m_canvas, aDC, (GR_DRAWMODE) 0,
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

#ifdef USE_WX_OVERLAY
    if( IsShown() )
    {
        m_overlay.Reset();
        wxDCOverlay overlaydc( m_overlay, (wxWindowDC*)DC );
        overlaydc.Clear();
    }
#endif

    if( m_canvas->IsMouseCaptured() )
        m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );

    m_canvas->DrawCrossHair( DC );

    // Display the filename and the layer name (found in the gerber files, if any)
    // relative to the active layer
    UpdateTitleAndInfo();
}


void GERBVIEW_FRAME::DrawItemsDCodeID( wxDC* aDC, GR_DRAWMODE aDrawMode )
{
    wxPoint     pos;
    int         width;
    double      orient;
    wxString    Line;

    GRSetDrawMode( aDC, aDrawMode );

    for( int layer = 0; layer < GERBER_DRAWLAYERS_COUNT; ++layer )
    {
        GERBER_FILE_IMAGE* gerber = g_GERBER_List.GetGbrImage( layer );

        if( gerber == NULL )    // Graphic layer not yet used
            continue;

        if( IsLayerVisible( layer ) == false )
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
}

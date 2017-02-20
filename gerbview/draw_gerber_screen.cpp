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

    PRINT_PARAMETERS* printParameters = (PRINT_PARAMETERS*) aData;

    // Build a suitable draw options for printing:
    GBR_DISPLAY_OPTIONS displayOptions;
    displayOptions.m_DisplayFlashedItemsFill = true;
    displayOptions.m_DisplayLinesFill = true;
    displayOptions.m_DisplayPolygonsFill = true;
    displayOptions.m_DisplayDCodes = false;
    displayOptions.m_IsPrinting = true;
    displayOptions.m_ForceBlackAndWhite = printParameters->m_Print_Black_and_White;
    displayOptions.m_NegativeDrawColor = GetDrawBgColor();
    displayOptions.m_BgDrawColor = GetDrawBgColor();

    // Find the graphic layer to be printed
    int page_number = printParameters->m_Flags;    // contains the page number (not necessarily graphic layer number)

    // Find the graphic layer number for the printed page (search through the mask and count bits)
    std::vector<int> printList = GetGerberLayout()->GetPrintableLayers();

    if( printList.size() < 1 )
        return;

    int graphiclayer = printList[page_number-1];

    // In Gerbview, only one graphic layer is printed by page.
    // So we temporary set the graphic layer list to print with only one layer id
    GetGerberLayout()->ClearPrintableLayers();
    GetGerberLayout()->AddLayerToPrintableList( graphiclayer );
    m_canvas->SetPrintMirrored( aPrintMirrorMode );

    GetGerberLayout()->Draw( m_canvas, aDC, (GR_DRAWMODE) 0,
                             wxPoint( 0, 0 ), &displayOptions );

    m_canvas->SetPrintMirrored( false );

    // Restore the list of printable graphic layers list:
    GetGerberLayout()->SetPrintableLayers( printList );
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
    m_DisplayOptions.m_NegativeDrawColor = GetNegativeItemsColor();
    m_DisplayOptions.m_BgDrawColor = GetDrawBgColor();
    GetGerberLayout()->Draw( m_canvas, DC, drawMode, wxPoint( 0, 0 ), &m_DisplayOptions );

    if( m_DisplayOptions.m_DisplayDCodes )
    {
        COLOR4D dcode_color = GetVisibleElementColor( DCODES_VISIBLE );
        GetGerberLayout()->DrawItemsDCodeID( m_canvas, DC, GR_COPY, dcode_color );
    }

    // Draw the "background" now, i.e. grid and axis after gerber layers
    // because most of time the actual background is erased by successive drawings of each gerber
    // layer mainly in COPY mode
    m_canvas->DrawBackGround( DC );

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
}

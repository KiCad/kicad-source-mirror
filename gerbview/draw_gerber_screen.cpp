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
#include <draw_graphic_text.h>
#include <base_units.h>

#include <gerbview.h>
#include <gerbview_frame.h>
#include <gerber_file_image.h>
#include <gerber_file_image_list.h>
#include "gerbview_printout.h"


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
        COLOR4D dcode_color = GetVisibleElementColor( LAYER_DCODES );
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

        // On macOS, the call to create overlaydc fails for some reason due to
        // the DC having zero size initially.
        wxCoord w = 0, h = 0;
        static_cast<wxWindowDC*>( DC )->GetSize( &w, &h );

        if( w == 0 || h == 0)
        {
            w = h = 1;
        }

        wxDCOverlay overlaydc( m_overlay, (wxWindowDC*)DC, 0, 0, 1, 1 );
        overlaydc.Clear();
    }
#endif

    if( m_canvas->IsMouseCaptured() )
        m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );

    m_canvas->DrawCrossHair( DC );
}

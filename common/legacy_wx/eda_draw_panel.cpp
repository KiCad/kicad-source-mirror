/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2007 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiface_i.h>
#include <gr_basic.h>
#include <common.h>
#include <macros.h>
#include <id.h>
#include <class_drawpanel.h>
#include <base_screen.h>
#include <trace_helpers.h>
#include <kicad_device_context.h>

#define CLIP_BOX_PADDING 2


#ifdef __WXMAC__
const int drawPanelStyle = wxHSCROLL | wxVSCROLL | wxALWAYS_SHOW_SB;
#else
const int drawPanelStyle = wxHSCROLL | wxVSCROLL;
#endif

EDA_DRAW_PANEL::EDA_DRAW_PANEL( EDA_DRAW_FRAME* parent, int id, const wxPoint& pos,
                                const wxSize& size ) :
    wxScrolledWindow( parent, id, pos, size, drawPanelStyle )
{
    wxASSERT( parent );

    ShowScrollbars( wxSHOW_SB_ALWAYS, wxSHOW_SB_ALWAYS );
    DisableKeyboardScrolling();

    m_scrollIncrementX = std::min( size.x / 8, 10 );
    m_scrollIncrementY = std::min( size.y / 8, 10 );

    SetLayoutDirection( wxLayout_LeftToRight );

    SetBackgroundColour( parent->GetDrawBgColor().ToColour() );

#if KICAD_USE_BUFFERED_DC || KICAD_USE_BUFFERED_PAINTDC
    SetBackgroundStyle( wxBG_STYLE_CUSTOM );
#endif

    m_ClipBox.SetSize( size );
    m_ClipBox.SetX( 0 );
    m_ClipBox.SetY( 0 );

#ifdef __WXMAC__
    m_defaultCursor = m_currentCursor = wxCURSOR_CROSS;
    m_showCrossHair = false;
#else
    m_defaultCursor = m_currentCursor = wxCURSOR_ARROW;
    m_showCrossHair = true;
#endif

    m_cursorLevel = 0;
    m_PrintIsMirrored = false;
}


EDA_DRAW_PANEL::~EDA_DRAW_PANEL()
{
}


EDA_DRAW_FRAME* EDA_DRAW_PANEL::GetParent() const
{
    wxWindow* mom = wxScrolledWindow::GetParent();
    return (EDA_DRAW_FRAME*) mom;
}


void* EDA_DRAW_PANEL::GetDisplayOptions()
{
    return GetParent()->GetDisplayOptions();
}


BASE_SCREEN* EDA_DRAW_PANEL::GetScreen()
{
    EDA_DRAW_FRAME* parentFrame = GetParent();

    return parentFrame->GetScreen();
}


void EDA_DRAW_PANEL::Refresh( bool eraseBackground, const wxRect* rect )
{
    GetParent()->GetGalCanvas()->Refresh();
}


wxPoint EDA_DRAW_PANEL::GetScreenCenterLogicalPosition()
{
    wxSize size = GetClientSize() / 2;
    INSTALL_UNBUFFERED_DC( dc, this );

    return wxPoint( dc.DeviceToLogicalX( size.x ), dc.DeviceToLogicalY( size.y ) );
}


void EDA_DRAW_PANEL::SetClipBox( wxDC& aDC, const wxRect* aRect )
{
    wxRect clipBox;

    // Use the entire visible device area if no clip area was defined.
    if( aRect == NULL )
    {
        BASE_SCREEN* Screen = GetScreen();

        if( !Screen )
            return;

        Screen->m_StartVisu = CalcUnscrolledPosition( wxPoint( 0, 0 ) );
        clipBox.SetSize( GetClientSize() );

        int scrollX, scrollY;

        double scalar = Screen->GetScalingFactor();
        scrollX = KiROUND( Screen->GetGridSize().x * scalar );
        scrollY = KiROUND( Screen->GetGridSize().y * scalar );

        m_scrollIncrementX = std::max( GetClientSize().x / 8, scrollX );
        m_scrollIncrementY = std::max( GetClientSize().y / 8, scrollY );
        Screen->m_ScrollbarPos.x = GetScrollPos( wxHORIZONTAL );
        Screen->m_ScrollbarPos.y = GetScrollPos( wxVERTICAL );
    }
    else
    {
        clipBox = *aRect;
    }

    // Pad clip box in device units.
    clipBox.Inflate( CLIP_BOX_PADDING );

    // Convert from device units to drawing units.
    m_ClipBox.SetOrigin( wxPoint( aDC.DeviceToLogicalX( clipBox.x ),
                                  aDC.DeviceToLogicalY( clipBox.y ) ) );
    m_ClipBox.SetSize( wxSize( aDC.DeviceToLogicalXRel( clipBox.width ),
                               aDC.DeviceToLogicalYRel( clipBox.height ) ) );

    wxLogTrace( kicadTraceCoords,
                wxT( "Device clip box=(%d, %d, %d, %d), Logical clip box=(%d, %d, %d, %d)" ),
                clipBox.x, clipBox.y, clipBox.width, clipBox.height,
                m_ClipBox.GetX(), m_ClipBox.GetY(), m_ClipBox.GetWidth(), m_ClipBox.GetHeight() );
}


void EDA_DRAW_PANEL::DoPrepareDC( wxDC& dc )
{
    wxScrolledWindow::DoPrepareDC( dc );

    if( GetScreen() != NULL )
    {
        double scale = GetScreen()->GetScalingFactor();
        dc.SetUserScale( scale, scale );

        wxPoint pt = GetScreen()->m_DrawOrg;
        dc.SetLogicalOrigin( pt.x, pt.y );
    }

    SetClipBox( dc );                         // Reset the clip box to the entire screen.
    GRResetPenAndBrush( &dc );
    dc.SetBackgroundMode( wxTRANSPARENT );
}


void EDA_DRAW_PANEL::OnCharHook( wxKeyEvent& event )
{
    wxLogTrace( kicadTraceKeyEvent, "EDA_DRAW_PANEL::OnCharHook %s", dump( event ) );
    event.Skip();
}

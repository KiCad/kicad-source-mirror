/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
 * Transient mouse following popup window implementation.
 */

#include <wx/settings.h>
#include <math/vector2wx.h>
#include <status_popup.h>
#include <eda_draw_frame.h>
#include <bitmaps.h>

STATUS_POPUP::STATUS_POPUP( wxWindow* aParent ) :
        wxPopupWindow( aParent ),
        m_expireTimer( this )
{
    SetDoubleBuffered( true );

    m_panel = new wxPanel( this, wxID_ANY );
    m_topSizer = new wxBoxSizer( wxHORIZONTAL );
    m_panel->SetSizer( m_topSizer );
    m_panel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    Connect( m_expireTimer.GetId(), wxEVT_TIMER, wxTimerEventHandler( STATUS_POPUP::onExpire ), nullptr, this );

#ifdef __WXOSX_MAC__
    // Key events from popups don't get put through the wxWidgets event system on OSX,
    // so we have to fall back to the CHAR_HOOK to forward hotkeys from the popup to
    // the canvas / frame.
    Connect( wxEVT_CHAR_HOOK, wxKeyEventHandler( STATUS_POPUP::onCharHook ), nullptr, this );
#endif
}


void STATUS_POPUP::onCharHook( wxKeyEvent& aEvent )
{
    // Key events from the status popup don't get put through the wxWidgets event system on
    // OSX, so we have to fall back to the CHAR_HOOK to forward hotkeys from the popup to
    // the canvas / frame.
    aEvent.SetEventType( wxEVT_CHAR );

    EDA_DRAW_FRAME* frame = dynamic_cast<EDA_DRAW_FRAME*>( GetParent() );

    if( frame )
        frame->GetCanvas()->OnEvent( aEvent );
    else
        GetParent()->GetEventHandler()->ProcessEvent( aEvent );
}


void STATUS_POPUP::Popup( wxWindow* )
{
    Show( true );
    Raise();
}


void STATUS_POPUP::PopupFor( int aMsecs )
{
    Popup();
    Expire( aMsecs );
}


void STATUS_POPUP::Move( const VECTOR2I& aWhere )
{
    SetPosition( ToWxPoint( aWhere ) );
}


void STATUS_POPUP::Move( const wxPoint& aWhere )
{
    SetPosition( aWhere );
}


void STATUS_POPUP::Expire( int aMsecs )
{
    m_expireTimer.StartOnce( aMsecs );
}


void STATUS_POPUP::updateSize()
{
    m_topSizer->Fit( m_panel );
    SetClientSize( m_panel->GetSize() );
}


void STATUS_POPUP::onExpire( wxTimerEvent& aEvent )
{
    Hide();
}


STATUS_TEXT_POPUP::STATUS_TEXT_POPUP( wxWindow* aParent ) :
    STATUS_POPUP( aParent )
{
    SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
    m_panel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNFACE ) );
    m_panel->SetForegroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_BTNTEXT ) );

    m_statusLine = new wxStaticText( m_panel, wxID_ANY, wxEmptyString ) ;
    m_topSizer->Add( m_statusLine, 1, wxALL | wxEXPAND, 5 );
}


void STATUS_TEXT_POPUP::SetText( const wxString& aText )
{
    m_statusLine->SetLabel( aText );
    updateSize();
}


void STATUS_TEXT_POPUP::SetTextColor( const wxColour& aColor )
{
    m_statusLine->SetForegroundColour( aColor );
}



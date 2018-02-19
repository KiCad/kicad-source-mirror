/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 CERN
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

#include <status_popup.h>
#include <draw_frame.h>

STATUS_POPUP::STATUS_POPUP( EDA_DRAW_FRAME* aParent ) :
    wxPopupWindow( aParent ), m_expireTimer( this )
{
    m_panel = new wxPanel( this, wxID_ANY );
    m_panel->SetBackgroundColour( *wxLIGHT_GREY );

    m_topSizer = new wxBoxSizer( wxVERTICAL );
    m_panel->SetSizer( m_topSizer );
    m_panel->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW ) );

    Connect( wxEVT_TIMER, wxTimerEventHandler( STATUS_POPUP::onExpire ), NULL, this );
}


void STATUS_POPUP::Popup( wxWindow* )
{
    Show( true );
    Raise();
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


STATUS_TEXT_POPUP::STATUS_TEXT_POPUP( EDA_DRAW_FRAME* aParent ) :
    STATUS_POPUP( aParent )
{
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

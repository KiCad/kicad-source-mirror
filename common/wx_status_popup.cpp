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

#include <wx_status_popup.h>
#include <wxPcbStruct.h>

WX_STATUS_POPUP::WX_STATUS_POPUP( PCB_EDIT_FRAME* aParent ) :
    wxPopupWindow( aParent )
{
    m_panel = new wxPanel( this, wxID_ANY );
    m_panel->SetBackgroundColour( *wxLIGHT_GREY );

    m_topSizer = new wxBoxSizer( wxVERTICAL );
    m_panel->SetSizer( m_topSizer );
}


void WX_STATUS_POPUP::updateSize()
{
    m_topSizer->Fit( m_panel );
    SetClientSize( m_panel->GetSize() );
}


WX_STATUS_POPUP::~WX_STATUS_POPUP()
{
}


void WX_STATUS_POPUP::Popup( wxWindow* )
{
    Show( true );
    Raise();
}


void WX_STATUS_POPUP::Move( const wxPoint& aWhere )
{
    SetPosition ( aWhere );
}

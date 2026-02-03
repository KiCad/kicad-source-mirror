/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <kiway_mail.h>
#include <wx/window.h>

wxDEFINE_EVENT( EDA_KIWAY_MAIL_RECEIVED, KIWAY_MAIL_EVENT );


KIWAY_MAIL_EVENT::KIWAY_MAIL_EVENT( const KIWAY_MAIL_EVENT& anOther ) :
    wxEvent( anOther ),
    m_destination( anOther.m_destination ),
    m_payload( anOther.m_payload )
{
}


KIWAY_MAIL_EVENT::KIWAY_MAIL_EVENT( FRAME_T aDestination, MAIL_T aCommand, std::string& aPayload,
                              wxWindow* aSource ) :
        wxEvent( aCommand, EDA_KIWAY_MAIL_RECEIVED ),
    m_destination( aDestination ),
    m_payload( aPayload )
{
    SetEventObject( aSource );
}


/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include <kiway_express.h>

//IMPLEMENT_DYNAMIC_CLASS( KIWAY_EXPRESS, wxEvent )


const wxEventType KIWAY_EXPRESS::wxEVENT_ID = wxNewEventType();


KIWAY_EXPRESS::KIWAY_EXPRESS( const KIWAY_EXPRESS& anOther ) :
    wxEvent( anOther )
{
    m_destination = anOther.m_destination;
    m_payload     = anOther.m_payload;
}


KIWAY_EXPRESS::KIWAY_EXPRESS( FRAME_T aDestination, int aCommand,
        const std::string& aPayload, wxWindow* aSource ) :
    wxEvent( aCommand, wxEVENT_ID ),
    m_destination( aDestination ),
    m_payload( aPayload )
{
    SetEventObject( aSource );
}


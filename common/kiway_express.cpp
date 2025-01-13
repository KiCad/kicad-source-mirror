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

#include <kiway_express.h>

#include <wx/window.h>

//IMPLEMENT_DYNAMIC_CLASS( KIWAY_EXPRESS, wxEvent )


#if 0   // requires that this code reside in only a single link image, rather than
        // in each of kicad.exe, _pcbnew.kiface, and _eeschema.kiface as now.
        // In the current case wxEVENT_ID will get a different value in each link
        // image.  We need to put this into a shared library for common utilization,
        // I think that library should be libki.so.  I am reluctant to do that now
        // because the cost will be finding libki.so at runtime, and we need infrastructure
        // to set our LIB_ENV_VAR to the proper place so libki.so can be reliably found.
        // All things in due course.
const wxEventType KIWAY_EXPRESS::wxEVENT_ID = wxNewEventType();
#else
const wxEventType KIWAY_EXPRESS::wxEVENT_ID = 30000;    // common across all link images,
                                                        // hopefully unique.
#endif


KIWAY_EXPRESS::KIWAY_EXPRESS( const KIWAY_EXPRESS& anOther ) :
    wxEvent( anOther ),
    m_destination( anOther.m_destination ),
    m_payload( anOther.m_payload )
{
}


KIWAY_EXPRESS::KIWAY_EXPRESS( FRAME_T aDestination, MAIL_T aCommand, std::string& aPayload,
                              wxWindow* aSource ) :
    wxEvent( aCommand, wxEVENT_ID ),
    m_destination( aDestination ),
    m_payload( aPayload )
{
    SetEventObject( aSource );
}


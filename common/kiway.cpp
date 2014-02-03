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

#include <kiway.h>
#include <wx/debug.h>
#include <string.h>

// one for each FACE_T
wxDynamicLibrary KIWAY::s_sch_dso;
wxDynamicLibrary KIWAY::s_pcb_dso;


KIWAY::KIWAY()
{
    memset( &m_dso_players, 0, sizeof( m_dso_players ) );
}


const wxString KIWAY::dso_name( FACE_T aFaceId )
{
    switch( aFaceId )
    {
    case FACE_SCH:  return wxT( "_eeschema." ) DSO_EXT;
    case FACE_PCB:  return wxT( "_pcbnew."   ) DSO_EXT;

    default:
        wxASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFaceId" ) );
        return wxEmptyString;
    }
}


PROJECT& KIWAY::Project()
{
    return m_project;
}


KIFACE*  KIWAY::KiFACE( FACE_T aFaceId, bool doLoad )
{
    switch( aFaceId )
    {
    case FACE_SCH:
    case FACE_PCB:
    //case FACE_LIB:
    //case FACE_MOD:
        if( m_dso_players[aFaceId] )
            return m_dso_players[aFaceId];

    default:
        wxASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFaceId" ) );
        return NULL;
    }

    // DSO with KIFACE has not been loaded yet, does user want to load it?
    if( doLoad  )
    {
        switch( aFaceId )
        {
        case FACE_SCH:
            break;

        case FACE_PCB:
            break;

        //case FACE_LIB:
        //case FACE_MOD:
        default:
            ;
        }
    }

    return NULL;
}

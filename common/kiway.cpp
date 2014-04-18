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

#include <string.h>

#include <kiway.h>
#include <config.h>
#include <wx/debug.h>
#include <wx/stdpaths.h>


KIWAY::KIWAY()
{
    memset( &m_kiface, 0, sizeof( m_kiface ) );
}


const wxString KIWAY::dso_full_path( FACE_T aFaceId )
{
    const wxChar*   name = wxT("");

    switch( aFaceId )
    {
    case FACE_SCH:  name = KIFACE_PREFIX wxT( "eeschema" );     break;
    case FACE_PCB:  name = KIFACE_PREFIX wxT( "pcbnew"   );     break;

    default:
        wxASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFaceId" ) );
        return wxEmptyString;
    }

    wxFileName fn = wxStandardPaths::Get().GetExecutablePath();

    fn.SetName( name );

    // Here a "suffix" == an extension with a preceding '.',
    // so skip the preceding '.' to get an extension
    fn.SetExt( KIFACE_SUFFIX + 1 );         // + 1 => &KIFACE_SUFFIX[1]

    return fn.GetFullPath();
}


PROJECT& KIWAY::Prj() const
{
    return *(PROJECT*) &m_project;      // strip const-ness, function really is const.
}


KIFACE*  KIWAY::KiFACE( PGM_BASE* aProgram, FACE_T aFaceId, bool doLoad )
{
    switch( aFaceId )
    {
    // case FACE_SCH:
    case FACE_PCB:
        if( m_kiface[aFaceId] )
            return m_kiface[aFaceId];
        break;

    default:
        wxASSERT_MSG( 0, wxT( "caller has a bug, passed a bad aFaceId" ) );
        return NULL;
    }

    // DSO with KIFACE has not been loaded yet, does user want to load it?
    if( doLoad  )
    {
        wxString dname = dso_full_path( aFaceId );

        wxDynamicLibrary dso;

        void*   addr = NULL;

        if( !dso.Load( dname, wxDL_VERBATIM | wxDL_NOW ) )
        {
            // Failure: error reporting UI was done via wxLogSysError().
            // No further reporting required here.
        }

        else if( ( addr = dso.GetSymbol( wxT( KIFACE_INSTANCE_NAME_AND_VERSION ) ) ) == NULL )
        {
            // Failure: error reporting UI was done via wxLogSysError().
            // No further reporting required here.
        }

        else
        {
            KIFACE_GETTER_FUNC* getter = (KIFACE_GETTER_FUNC*) addr;

            KIFACE* kiface = getter( &m_kiface_version[aFaceId], KIFACE_VERSION, aProgram );

            // KIFACE_GETTER_FUNC function comment (API) says the non-NULL is unconditional.
            wxASSERT_MSG( kiface,
                wxT( "attempted DSO has a bug, failed to return a KIFACE*" ) );

            // Give the DSO a single chance to do its "process level" initialization.
            // "Process level" specifically means stay away from any projects in there.
            if( kiface->OnKifaceStart( aProgram, KFCTL_PROJECT_SUITE ) )
            {
                // Tell dso's wxDynamicLibrary destructor not to Unload() the program image.
                (void) dso.Detach();

                return m_kiface[aFaceId] = kiface;
            }
        }

        // In any of the failure cases above, dso.Unload() should be called here
        // by dso destructor.
    }

    return NULL;
}

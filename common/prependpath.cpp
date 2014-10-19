/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * Copyright (C) 2014 KiCad Developers, see CHANGELOG.TXT for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <macros.h>
#include <fctsys.h>
#include <wx/filename.h>



#if !wxCHECK_VERSION( 2, 9, 0 )

// implement missing wx2.8 function until >= wx3.0 pervades.
static wxString wxJoin(const wxArrayString& arr, const wxChar sep,
                const wxChar escape = '\\')
{
    size_t count = arr.size();
    if ( count == 0 )
        return wxEmptyString;

    wxString str;

    // pre-allocate memory using the estimation of the average length of the
    // strings in the given array: this is very imprecise, of course, but
    // better than nothing
    str.reserve(count*(arr[0].length() + arr[count-1].length()) / 2);

    if ( escape == wxT('\0') )
    {
        // escaping is disabled:
        for ( size_t i = 0; i < count; i++ )
        {
            if ( i )
                str += sep;
            str += arr[i];
        }
    }
    else // use escape character
    {
        for ( size_t n = 0; n < count; n++ )
        {
            if ( n )
                str += sep;

            for ( wxString::const_iterator i = arr[n].begin(),
                                         end = arr[n].end();
                  i != end;
                  ++i )
            {
                const wxChar ch = *i;
                if ( ch == sep )
                    str += escape;      // escape this separator
                str += ch;
            }
        }
    }

    str.Shrink(); // release extra memory if we allocated too much
    return str;
}
#endif


/// Put aPriorityPath in front of all paths in the value of aEnvVar.
const wxString PrePendPath( const wxString& aEnvVar, const wxString& aPriorityPath )
{
    wxPathList  paths;

    paths.AddEnvList( aEnvVar );
    paths.Insert( aPriorityPath, 0 );

    return wxJoin( paths, wxPATH_SEP[0] );
}

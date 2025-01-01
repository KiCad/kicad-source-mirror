/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <common.h>
#include <pgm_base.h>
#include <paths.h>
#include <systemdirsappend.h>
#include <trace_helpers.h>

#include <wx/arrstr.h>
#include <wx/log.h>


wxString SearchHelpFileFullPath( const wxString& aBaseName )
{
    SEARCH_STACK basePaths;
    wxString     helpFile;

    // help files are most likely located in the documentation install path
    basePaths.Add( PATHS::GetDocumentationPath() );

#ifndef __WIN32__
    // just in case, add all known system directories to the search stack
    SystemDirsAppend( &basePaths );
#endif

#if defined( DEBUG )
    basePaths.Show( wxString( __func__ ) + wxS( ": basePaths" ) );
#endif

    // By default, the documentation from kicad-doc is installed to a folder called "help" with
    // subdirectories for all supported languages. Although this can be changed at build-time by
    // overwriting ${KICAD_DOC_PATH}, the best guess KiCad can make is that help files are always
    // located in a folder named "help". If no translation matching the current locale settings is
    // available, the English version will be returned instead.

    wxLocale*     currentLocale = Pgm().GetLocale();
    wxArrayString localeNameDirs;

    // canonical form of the current locale (e.g., "fr_FR")
    localeNameDirs.Add( currentLocale->GetCanonicalName() );

    // short form of the current locale (e.g., "fr")
    // wxLocale::GetName() does not always return the short form
    localeNameDirs.Add( currentLocale->GetName().BeforeLast( wxS( '_' ) ) );

    // plain English (in case a localised version of the help file cannot be found)
    localeNameDirs.Add( wxS( "en" ) );

    for( wxString& locale : localeNameDirs )
    {
        SEARCH_STACK docPaths;

        for( wxString& base : basePaths )
        {
            wxFileName path( base, wxEmptyString );

            // add <base>/help/<locale>/
            path.AppendDir( wxS( "help" ) );
            path.AppendDir( locale );
            docPaths.AddPaths( path.GetPath() );

            // add <base>/doc/help/<locale>/
            path.InsertDir( path.GetDirCount() - 2, wxS( "doc" ) );
            docPaths.AddPaths( path.GetPath() );

            // add <base>/doc/kicad/help/<locale>/
            path.InsertDir( path.GetDirCount() - 2, wxS( "kicad" ) );
            docPaths.AddPaths( path.GetPath() );
        }

#if defined( DEBUG )
        docPaths.Show( wxString( __func__ ) + wxS( ": docPaths (" ) + locale + wxS( ")" ) );
#endif

        // search HTML first, as it is the preferred format for help files
        wxLogTrace( tracePathsAndFiles, wxS( "Checking SEARCH_STACK for file %s.html" ),
                    aBaseName );
        helpFile = docPaths.FindValidPath( aBaseName + wxS( ".html" ) );

        if( !helpFile.IsEmpty() )
        {
            // prepend URI protocol to open the file in a browser
            helpFile = wxS( "file://" ) + helpFile;
            break;
        }

        // search PDF only when no corresponding HTML file was found
        wxLogTrace( tracePathsAndFiles, wxS( "Checking SEARCH_STACK for file %s.pdf" ), aBaseName );
        helpFile = docPaths.FindValidPath( aBaseName + wxS( ".pdf" ) );

        if( !helpFile.IsEmpty() )
            break;
    }

    return helpFile;
}

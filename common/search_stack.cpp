/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
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

#include <search_stack.h>
#include <string_utils.h>
#include <trace_helpers.h>
#include <wx/tokenzr.h>
#include <wx/log.h>


#if defined(_WIN32)
 #define PATH_SEPS          wxT( ";\r\n" )
#else
 #define PATH_SEPS          wxT( ":;\r\n" )       // unix == linux | mac
#endif


int SEARCH_STACK::Split( wxArrayString* aResult, const wxString& aPathString )
{
    wxStringTokenizer   tokenizer( aPathString, PATH_SEPS, wxTOKEN_STRTOK );

    while( tokenizer.HasMoreTokens() )
    {
        wxString path = tokenizer.GetNextToken();

        aResult->Add( path );
    }

    return aResult->GetCount();
}


// Convert aRelativePath to an absolute path based on aBaseDir
static wxString base_dir( const wxString& aRelativePath, const wxString& aBaseDir )
{
    wxFileName fn = aRelativePath;

    if( !fn.IsAbsolute() && !!aBaseDir )
    {
        wxASSERT_MSG( wxFileName( aBaseDir ).IsAbsolute(),
                      wxT( "Must pass absolute path in aBaseDir" ) );
        fn.MakeRelativeTo( aBaseDir );
    }

    return fn.GetFullPath();
}


wxString SEARCH_STACK::FilenameWithRelativePathInSearchList(
        const wxString& aFullFilename, const wxString& aBaseDir )
{
    wxFileName fn = aFullFilename;
    wxString   filename = aFullFilename;

    unsigned   pathlen  = fn.GetPath().Len();   // path len, used to find the better (shortest)
                                                // subpath within defaults paths

    for( unsigned kk = 0; kk < GetCount(); kk++ )
    {
        fn = aFullFilename;

        // Search for the shortest subpath within 'this':
        if( fn.MakeRelativeTo( base_dir( (*this)[kk], aBaseDir ) ) )
        {
            if( fn.GetPathWithSep().StartsWith( wxT("..") ) )  // Path outside KiCad libs paths
                continue;

            if( pathlen > fn.GetPath().Len() )    // A better (shortest) subpath is found
            {
                filename = fn.GetPathWithSep() + fn.GetFullName();
                pathlen  = fn.GetPath().Len();
            }
        }
    }

    return filename;
}


void SEARCH_STACK::RemovePaths( const wxString& aPaths )
{
    bool            isCS = wxFileName::IsCaseSensitive();
    wxArrayString   paths;

    Split( &paths, aPaths );

    for( unsigned i=0; i<paths.GetCount();  ++i )
    {
        wxString path = paths[i];

        if( Index( path, isCS ) != wxNOT_FOUND )
        {
            Remove( path );
        }
    }
}


void SEARCH_STACK::AddPaths( const wxString& aPaths, int aIndex )
{
    bool            isCS = wxFileName::IsCaseSensitive();
    wxArrayString   paths;

    Split( &paths, aPaths );

    // appending all of them, on large or negative aIndex
    if( unsigned( aIndex ) >= GetCount() )
    {
        for( unsigned i=0; i<paths.GetCount();  ++i )
        {
            wxString path = paths[i];

            if( wxFileName::IsDirReadable( path )
                && Index( path, isCS ) == wxNOT_FOUND )
            {
                Add( path );
            }
        }
    }

    // inserting all of them:
    else
    {
        for( unsigned i=0; i<paths.GetCount();  ++i )
        {
            wxString path = paths[i];

            if( wxFileName::IsDirReadable( path )
                && Index( path, isCS ) == wxNOT_FOUND )
            {
                Insert( path, aIndex );
                aIndex++;
            }
        }
    }
}


#if 1       // this function is too convoluted for words.

const wxString SEARCH_STACK::LastVisitedPath( const wxString& aSubPathToSearch )
{
    wxString path;

    // Initialize default path to the main default lib path
    // this is the second path in list (the first is the project path).
    unsigned pcount = GetCount();

    if( pcount )
    {
        unsigned ipath = 0;

        if( (*this)[0] == wxGetCwd() )
            ipath = 1;

        // First choice of path:
        if( ipath < pcount )
            path = (*this)[ipath];

        // Search a sub path matching this SEARCH_PATH
        if( !IsEmpty() )
        {
            for( ; ipath < pcount; ipath++ )
            {
                if( (*this)[ipath].Contains( aSubPathToSearch ) )
                {
                    path = (*this)[ipath];
                    break;
                }
            }
        }
    }

    if( path.IsEmpty() )
        path = wxGetCwd();

    return path;
}
#endif


#if defined( DEBUG )
void SEARCH_STACK::Show( const wxString& aPrefix ) const
{
    wxLogTrace( tracePathsAndFiles, "%s SEARCH_STACK:", aPrefix );

    for( unsigned i = 0; i < GetCount(); ++i )
    {
        wxLogTrace( tracePathsAndFiles, "  [%2u]:%s", i, TO_UTF8( ( *this )[i] ) );
    }
}
#endif

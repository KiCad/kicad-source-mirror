/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2015 KiCad Developers, see CHANGELOG.TXT for contributors.
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


/*
 *  While creating a wizard to edit the fp lib tables, and mainly the web viewer
 *  which can read the list of pretty library on a github repos, I was told there is
 *  this URL to retrieve info from any particular repo:
 *
 *  https://api.github.com/orgs/KiCad/repos
 *  or
 *  https://api.github.com/users/KiCad/repos
 *
 *  This gets just information on the repo in JSON format.
 *
 *  I used avhttp, already used in the pcbnew Github plugin to download
 *  the json file.
 *
 *  JP Charras.
 */


#if 0
/*
 *  FIX ME
 *  I do not include avhttp.hpp here, because it is already included in
 *  github_plugin.cpp
 *  and if it is also included in this file, the link fails (double definiton of modules)
 *  therefore, the GITHUB_GETLIBLIST method which uses avhttp to download dats from gitub
 *  is in github_plugin.cpp
 */

#ifndef WIN32_LEAN_AND_MEAN
// when WIN32_LEAN_AND_MEAN is defined, some useless includes in <window.h>
// are skipped, and this avoid some compil issues
#define WIN32_LEAN_AND_MEAN
#endif

#ifdef WIN32
// defines needed by avhttp
// Minimal Windows version is XP: Google for _WIN32_WINNT
 #define _WIN32_WINNT   0x0501
 #define WINVER         0x0501
#endif

#include <wx/wx.h>
#include <avhttp.hpp>

#endif

#include <wx/uri.h>

#include <github_getliblist.h>
#include <macros.h>
#include <common.h>
#include <html_link_parser.h>


GITHUB_GETLIBLIST::GITHUB_GETLIBLIST( const wxString& aRepoURL )
{
    m_repoURL = aRepoURL;
    m_libs_ext =  wxT( ".pretty" );
    strcpy( m_option_string, "application/json" );
}


bool GITHUB_GETLIBLIST::Get3DshapesLibsList( wxArrayString* aList,
                        bool (*aFilter)( const wxString& aData ) )
{
    std::string fullURLCommand;
    strcpy( m_option_string, "text/html" );

    wxString repoURL = m_repoURL;

    wxString errorMsg;

    fullURLCommand = repoURL.utf8_str();
    bool success = remote_get_json( &fullURLCommand, &errorMsg );

    if( !success )
    {
        wxMessageBox( errorMsg );
        return false;
    }

    if( aFilter && aList )
    {
        //Convert m_image (std::string) to a wxString for HTML_LINK_PARSER
        wxString buffer( GetBuffer() );

        HTML_LINK_PARSER html_parser( buffer, *aList );
        html_parser.ParseLinks( aFilter );
    }

    return true;
}


bool GITHUB_GETLIBLIST::GetFootprintLibraryList( wxArrayString& aList )
{
    std::string fullURLCommand;
    int page = 1;
    int itemCountMax = 99;              // Do not use a valu > 100, it does not work
    strcpy( m_option_string, "application/json" );

    // Github max items returned is 100 per page

    if( !repoURL2listURL( m_repoURL, &fullURLCommand, itemCountMax, page ) )
    {
        wxString msg = wxString::Format( _( "malformed URL:\n'%s'" ), GetChars( m_repoURL ) );
        wxMessageBox( msg );
        return false;
    }

    // The URL lib names are relative to the server name.
    // so add the server name to them.
    wxURI repo( m_repoURL );
    wxString urlPrefix = repo.GetScheme() + wxT( "://" ) + repo.GetServer() + wxT( "/" );

    wxString errorMsg;
    const char  sep = ',';      // Separator fields, in json returned file
    wxString    tmp;
    int items_count_per_page = 0;
    std::string& json_image = GetBuffer();

    while( 1 )
    {
        bool success = remote_get_json( &fullURLCommand, &errorMsg );

        if( !success )
        {
            wxMessageBox( errorMsg );
            return false;
        }


        for( unsigned ii = 0; ii < json_image.size(); ii++ )
        {
            if( json_image[ii] == sep || ii == json_image.size() - 1 )
            {
                if( tmp.StartsWith( wxT( "\"full_name\"" ) ) )
                {
                    #define QUOTE '\"'
                    // Remove useless quotes:
                    if( tmp[tmp.Length() - 1] == QUOTE )
                        tmp.RemoveLast();

                    if( tmp.EndsWith( m_libs_ext ) )
                    {
                        aList.Add( tmp.AfterLast( ':' ) );
                        int idx = aList.GetCount() - 1;

                        if( aList[idx][0] == QUOTE )
                            aList[idx].Remove( 0, 1 );

                        aList[idx].Prepend( urlPrefix );
                    }

                    items_count_per_page++;
                }

                tmp.Clear();
            }
            else
                tmp << json_image[ii];
        }

        if( items_count_per_page >= itemCountMax )
        {
            page++;
            repoURL2listURL( m_repoURL, &fullURLCommand, itemCountMax, page );
            items_count_per_page = 0;
            ClearBuffer();
        }
        else
            break;
    }

    aList.Sort();
    return true;
}


bool GITHUB_GETLIBLIST::repoURL2listURL( const wxString& aRepoURL,
        std::string* aFullURLCommand,
        int aItemCountMax, int aPage  )
{
    // aListURL is e.g. "https://api.github.com/orgs/KiCad/repos"
    // or "https://api.github.com/users/KiCad/repos"
    // aRepoURL is e.g. "https://github.com/KiCad"
    // Github has a default pagination set to 30 items.
    // but allows up to 100 items max if we add the "?per_page=100" option

    wxURI repo( aRepoURL );

    if( repo.HasServer() && repo.HasPath() )
    {
        // goal: "https://api.github.com/orgs/KiCad"
        wxString target_url( wxT( "https://api.github.com/orgs" ) );
        target_url  += repo.GetPath();
        target_url  += wxT( "/repos" );

        // Github has a default pagination set to 30 items.
        // but allows up to 100 items max. Use this limit
        target_url += wxString::Format( "?per_page=%d&page=%d", aItemCountMax, aPage );

        *aFullURLCommand = target_url.utf8_str();
        return true;
    }

    return false;
}

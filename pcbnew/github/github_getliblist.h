/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras jp.charras at wanadoo.fr
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

#ifndef GITHUB_GETLIBLIST_H_
#define GITHUB_GETLIBLIST_H_


/**
 *   Class GITHUB_GETLIBLIST
 *   implements a portion of pcbnew's PLUGIN interface to provide read only access
 *   to a github repo to extract pretty footprints library list, in json format.
 *
 *   this plugin simply reads in a zip file of the repo and unzips it from RAM as
 *   needed. Therefore this "Github" plugin is <b>read only for accessing remote
 *   at https://api.github.com/orgs/KiCad/repos</b>
 */
class GITHUB_GETLIBLIST
{
public:
    // -----<API>----------------------------------------------------------
    bool GetLibraryList( wxArrayString& aList );

    // -----</API>---------------------------------------------------------

    GITHUB_GETLIBLIST( const wxString& aRepoURL );
    ~GITHUB_GETLIBLIST() {}

protected:

    /**
     * Function repoURL2listURL
     * translates a repo URL to the URL name which gives the state of repos URL
     * as commonly seen on github.com
     *
     * @param  aRepoURL points to the base of the repo.
     * @param  aFullURLCommand is URL the full URL command (URL+options).
     * @param  aItemCountMax is the max item count in apage,
     *   and is 100 for github repo.
     * @param  aPage is the page number, if there are more than one page in repo.
     * @return bool - true if @a aRepoULR was parseable, else false
     */
    bool repoURL2listURL( const wxString& aRepoURL, std::string* aFullURLCommand,
            int aItemCountMax, int aPage = 1 );

    /**
     * Function remote_get_json
     * Download a json text from a github repo.  The text image
     * is received into the m_input_stream.
     * @param aFullURLCommand the full command, i.e. the url with options like
     * "https://api.github.com/users/KiCad/repos?per_page=100?page=1"
     * @param aMsgError a pointer to a wxString which can store an error message
     * @return true if OK, false if error (which an error message in *aMsgError
     */
    bool remote_get_json( std::string* aFullURLCommand, wxString* aMsgError );

    wxString m_github_path;     ///< Something like https://api.github.com/orgs/KiCad
    std::string m_json_image;   ///< image of the text file in its entirety.
    wxString m_repoURL;         // the URL of the Github repo
};


#endif    // GITHUB_GETLIBLIST_H_

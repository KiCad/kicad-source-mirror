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
 * Class GITHUB_GETLIBLIST
 * implements a portion of pcbnew's PLUGIN interface to provide read only access
 * to a github repo to extract pretty footprints library list, in json format,
 * or extract 3D shapes libraries list (.3dshapes folders) and download the 3d shapes files
 *
 * To extract pretty footprints library list, this plugin simply reads in
 * a zip file of the repo and unzips it from RAM as needed.
 * Therefore this "Github" plugin is <b>read only for accessing remote
 * at https://api.github.com/orgs/KiCad/repos</b>
 *
 * To extract 3D shapes libraries list (.3dshapes folders) we cannot use api.github.com
 * to read this list, becuse it is in a subdirectory of https://github.com/KiCad.
 * The corresponding html page of the server is read, and URLs of all .3dshapes folders
 * are extracted.
 * files are then read from https://raw.githubusercontent.com/<lib path>, but not zipped
 * because they are not accessible in zipped form.
 */
class GITHUB_GETLIBLIST
{
public:
    // -----<API>----------------------------------------------------------

    /**
     * Fills aList by the name of footprint libraries found on the github repo
     */
    bool GetFootprintLibraryList( wxArrayString& aList );

    /**
     * Fills aList by the URL of libraries found on the github repo
     * @param aList = a reference to a wxArrayString to fill with names
     * @param aFilter( const wxString& aData ) = a callback funtion to
     * to filter URLs to put in aList.
     * If NULL, no URL will be stored in aList
     */
    bool Get3DshapesLibsList( wxArrayString* aList,
                        bool (*aFilter)( const wxString& aData ) );

    /**
     * @return the buffer which stores all the downloaded raw data
     */
    std::string& GetBuffer() { return m_image; }

    /**
     * Clear the buffer which stores all the downloaded raw data
     */
    void ClearBuffer() { m_image.clear(); }

    /**
     * The library names are expecting ending by .pretty
     * SetLibraryExt set the extension to aExt
     */
    void SetLibraryExt( const wxString& aExt ) { m_libs_ext = aExt; }

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
     * @param  aItemCountMax is the max item count in a page,
     *   and is 100 for github repo.
     * @param  aPage is the page number, if there are more than one page in repo.
     * @return bool - true if @a aRepoULR was parseable, else false
     */
    bool repoURL2listURL( const wxString& aRepoURL, std::string* aFullURLCommand,
            int aItemCountMax, int aPage = 1 );

    /**
     * Function remoteGetJSON
     * Download a json text from a github repo.  The text image
     * is received into the m_input_stream.
     * @param aFullURLCommand the full command, i.e. the url with options like
     * "https://api.github.com/users/KiCad/repos?per_page=100?page=1"
     * @param aMsgError a pointer to a wxString which can store an error message
     * @return true if OK, false if error (which an error message in *aMsgError
     */
    bool remoteGetJSON( const std::string& aFullURLCommand, wxString* aMsgError );

    wxString m_github_path;     ///< Something like https://api.github.com/orgs/KiCad
    std::string m_image;        ///< image of the downloaded data in its entirety.
    wxString m_repoURL;         ///< the URL of the Github repo
    wxString m_libs_ext;        ///< the extension of the name of libraries (default = .pretty)
    char m_option_string[64];   ///< option for transfert type, like "application/json"
};


#endif    // GITHUB_GETLIBLIST_H_

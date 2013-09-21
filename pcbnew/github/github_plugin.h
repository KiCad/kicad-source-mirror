/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#ifndef GITHUB_PLUGIN_H_
#define GITHUB_PLUGIN_H_


struct FPL_CACHE;


/**
 * Class GITHUB_PLUGIN
 * implements a portion of pcbnew PLUGIN to provide read only access to a github
 * repo consisting of pretty footprints
 *
 * @author Dick Hollenbeck
 * @date Original date: 10-Sep-2013
 */
class GITHUB_PLUGIN : public PLUGIN
{
public:
    //-----<PLUGIN API>----------------------------------------------------------
    // ("read-only" subset)

    const wxString& PluginName() const;

    const wxString& GetFileExtension() const;

    wxArrayString FootprintEnumerate( const wxString& aLibraryPath, PROPERTIES* aProperties );

    MODULE* FootprintLoad( const wxString& aLibraryPath,
            const wxString& aFootprintName, PROPERTIES* aProperties );

    bool IsFootprintLibWritable( const wxString& aLibraryPath );

    //-----</PLUGIN API>---------------------------------------------------------

    GITHUB_PLUGIN();        // constructor, if any, must be zero arg
    ~GITHUB_PLUGIN();

private:

    void cacheLib( const wxString& aLibraryPath ) throw( IO_ERROR );

    /**
     * Function repoURL_zipURL
     * translates a repo URL to a zipfile URL name as commonly seen on github.com
     *
     * @param  aRepoURL points to the base of the repo.
     * @param  aZipURL is where to put the zip file URL.
     * @return bool - true if @a aRepoULR was parseable, else false
     */
    static bool repoURL_zipURL( const wxString& aRepoURL, std::string* aZipURL );

    /**
     * Function remote_get_zip
     * fetches a zip file image from a github repo synchronously.  The byte image
     * is received into the m_input_stream.
     */
    void remote_get_zip( const wxString& aRepoURL ) throw( IO_ERROR );

    wxString    m_lib_path;     ///< from aLibraryPath, something like https://github.com/liftoff-sr/pretty_footprints
    std::string m_zip_image;    ///< byte image of the zip file in its entirety.
    FPL_CACHE*  m_cache;
};

#endif // GITHUB_PLUGIN_H_

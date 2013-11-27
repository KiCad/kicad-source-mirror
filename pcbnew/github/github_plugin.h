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

#include <kicad_plugin.h>

struct GH_CACHE;


/**
   Class GITHUB_PLUGIN
   implements a portion of pcbnew PLUGIN to provide read only access to a github
   repo consisting of pretty footprints.  It could have used version 3 of the
   github.com API documented here:

   <pre>
       http://developer.github.com
       https://help.github.com/articles/creating-an-access-token-for-command-line-use
   </pre>

   but it does not. Rather it simply reads in a zip file of the repo and unzips
   it from RAM as needed. <b>Therefore the PLUGIN is read only for accessing
   remote pretty libraries at https://github.com.</b> The "Library Path" in the
   fp-lib-table row for a Github library should be set to the full https:// URL.
   For example:

   <pre>
        https://github.com/liftoff-sr/pretty_footprints
   </pre>

   This is typically

   <pre>
        https://github.com/user_name/repo_name
   </pre>
   <p>

   This PLUGIN also supports "Copy On Write", a.k.a. "COW". Thus a Github
   library defined in either the fp-lib-table (project or global) will take an
   optional option called <b>allow_pretty_writing_to_this_dir</b>. This option
   is essentially the "Library Path" for a local Kicad (pretty) library which is
   combined to make up the Github library found in the same fp-lib-table row. If
   the option is missing, then the Github library is read only as always. If the
   option is present for a Github library, then any writes to this library will
   go to the local *.pretty directory. Note that the github.com resident portion
   of this hybrid COW library is always read only, meaning you cannot delete
   anything or modify any footprint at github directly.

   <p>

   Any footprint loads will always give precedence to the local footprints found
   in the pretty dir given by option <b>allow_pretty_writing_to_this_dir</b>. So
   once you have written to the COW library's local directory by doing a
   footprint save, no github updates will be seen when loading a footprint by
   the same name as one for which you've written locally.

   <p>

   Always keep a separate local *.pretty directory for each Github library,
   never combine them by referring to the same directory more than once. Also,
   do not also use the same COW (*.pretty) directory in a "Kicad" fp-lib-table
   entry. This would likely create a mess. The COW directory should be manually
   created in advance, and the directory name must end with ".pretty". The value
   of the option <b>allow_pretty_writing_to_this_dir</b> will be path
   substituted with any environment variable strings embedded, just like the
   "Library Path" is.

   <p>

   What's the point of COW? It is to turbo-charge the sharing of footprints. If
   you periodically email your COW pretty footprint modifications to the Github
   repo maintainer, you can help update the Github copy. Simply email the
   individual *.kicad_mod file you find in your COW directories. After you've
   received confirmation that your changes have been committed up at github.com,
   you can safely delete your COW file(s) and those from github.com will flow
   down. Your goal should be to keep the COW file set as small as possible by
   contributing frequently to the shared master copies at https://github.com.

   <p>

   Note that if you use the module editor to delete a footprint and it is
   present in the COW local dir, it will get deleted from there. However, it may
   not be deleted from the library as a whole if the footprint of the same name
   also exists in the github repo. In this case deleting the local copy will
   simply unmask the one at the github repo. Remember, it is masked out if there
   is a local COW copy, since the local copy always takes precedence. And
   remember you cannot modify the github copy except by emailing a COW
   modification to the repo maintainer.

   @author Dick Hollenbeck
   @date Original date: 10-Sep-2013

 */
class GITHUB_PLUGIN : public PCB_IO
{
public:
    //-----<PLUGIN API>----------------------------------------------------------
    const wxString PluginName() const;

    const wxString GetFileExtension() const;

    wxArrayString FootprintEnumerate( const wxString& aLibraryPath,
            const PROPERTIES* aProperties = NULL );

    MODULE* FootprintLoad( const wxString& aLibraryPath,
            const wxString& aFootprintName, const PROPERTIES* aProperties );

    void FootprintSave( const wxString& aLibraryPath, const MODULE* aFootprint,
            const PROPERTIES* aProperties = NULL );

    void FootprintDelete( const wxString& aLibraryPath, const wxString& aFootprintName,
            const PROPERTIES* aProperties = NULL );

    bool IsFootprintLibWritable( const wxString& aLibraryPath );

    void FootprintLibOptions( PROPERTIES* aListToAppendTo ) const;

    // Since I derive from PCB_IO, I have to implement this, else I'd inherit his, which is bad since
    // my lib_path is not his.  Note: it is impossible to create a Github library, but can the C.O.W. portion.
    void FootprintLibCreate( const wxString& aLibraryPath, const PROPERTIES* aProperties );

    // Since I derive from PCB_IO, I have to implement this, else I'd inherit his, which is bad since
    // my lib_path is not his.  Note: it is impossible to delete a Github library, but can the C.O.W portion.
    bool FootprintLibDelete( const wxString& aLibraryPath, const PROPERTIES* aProperties );

    //-----</PLUGIN API>---------------------------------------------------------

    GITHUB_PLUGIN();        // constructor, if any, must be zero arg
    ~GITHUB_PLUGIN();

protected:

    void init( const PROPERTIES* aProperties );

    void cacheLib( const wxString& aLibraryPath, const PROPERTIES* aProperties ) throw( IO_ERROR );

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
    GH_CACHE*   m_gh_cache;
    wxString    m_pretty_dir;
};


#endif // GITHUB_PLUGIN_H_

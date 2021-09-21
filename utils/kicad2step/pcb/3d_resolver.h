/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file 3d_resolver.h
 * provides an extensible class to resolve 3D model paths.
 * Derived from 3d_filename_resolver.h,cpp and modified for
 * use in stand-alone utilities.
 */

#ifndef RESOLVER_3D_H
#define RESOLVER_3D_H

#include <list>
#include <map>
#include <vector>
#include <wx/string.h>

namespace S3D
{
struct rsort_wxString
{
    bool operator() (const wxString& strA, const wxString& strB ) const
    {
        // sort a wxString using the reverse character order; for 3d model
        // filenames this will typically be a much faster operation than
        // a normal alphabetic sort
        wxString::const_reverse_iterator sA = strA.rbegin();
        wxString::const_reverse_iterator eA = strA.rend();

        wxString::const_reverse_iterator sB = strB.rbegin();
        wxString::const_reverse_iterator eB = strB.rend();

        if( strA.empty() )
        {
            if( strB.empty() )
                return false;

            // note: this rule implies that a null string is first in the sort order
            return true;
        }

        if( strB.empty() )
            return false;

        while( sA != eA && sB != eB )
        {
            if( (*sA) == (*sB) )
            {
                ++sA;
                ++sB;
                continue;
            }

            if( (*sA) < (*sB) )
                return true;
            else
                return false;
        }

        if( sB == eB )
            return false;

        return true;
    }
};

};  // end NAMESPACE


class KICADPCB;

struct SEARCH_PATH
{
    wxString m_Alias;           // alias to the base path
    wxString m_Pathvar;         // base path as stored in the config file
    wxString m_Pathexp;         // expanded base path
    wxString m_Description;     // description of the aliased path
};


class S3D_RESOLVER
{
public:
    S3D_RESOLVER();

    /**
     * Set the user's configuration directory for 3D models.
     *
     * @param aConfigDir
     * @return true if the call succeeds (directory exists).
     */
    bool Set3DConfigDir( const wxString& aConfigDir );

    /**
     * Set the current KiCad project directory as the first entry in the model path list.
     *
     * @param aProjDir is the current project directory.
     * @param flgChanged, if specified, is set to true if the directory actually changed.
     * @return true if the call succeeds.
     */
    bool SetProjectDir( const wxString& aProjDir, bool* flgChanged = NULL );
    wxString GetProjectDir( void );

    /**
     * Determine the full path of the given file name.
     *
     * In the future remote files may be supported, in which case it is best to require a full
     * URI in which case #ResolvePath should check that the URI conforms to RFC-2396 and related
     * documents and copies \a aFileName into the resolved name if the URI is valid.
     */
    wxString ResolvePath( const wxString& aFileName );

    /**
     * Produce a relative path based on the existing search directories or returns the same path
     * if the path is not a superset of an existing search path.
     *
     * @param aFullPathName is an absolute path to shorten.
     * @return the shortened path or aFullPathName.
     */
    wxString ShortenPath( const wxString& aFullPathName );

    /**
     * Return a pointer to the internal path list.
     *
     * The list can be used to set up the list of search paths available to a 3D file browser.
     *
     * @return the search path list.
     */
    const std::list< SEARCH_PATH >* GetPaths( void );

    /**
     * Return true if the given name contains an alias and populates the string with the alias
     * and the relative path.
     */
    bool SplitAlias( const wxString& aFileName, wxString& anAlias, wxString& aRelPath );

    /**
     * If the path contains an alias then \a hasAlias is set true.
     *
     * @return true if the given path is a valid aliased relative path.
     */
    bool ValidateFileName( const wxString& aFileName, bool& hasAlias );

private:
    /**
     * Build the path list using available information such as KICAD6_3DMODEL_DIR and the
     * 3d_path_list configuration file.
     *
     * Invalid paths are silently discarded and removed from the configuration file.
     *
     * @return true if at least one valid path was found
     */
    bool createPathList( void );

    /**
     * Check that \a aPath is valid and adds it to the search list.
     *
     * @param aPath is the alias set to be checked and added.
     * @return true if \a aPath is valid.
     */
    bool addPath( const SEARCH_PATH& aPath );

    /**
     * Read a list of path names from a configuration file.
     *
     * @return true if a file was found and contained at least one valid path.
     */
    bool readPathList( void );

    /**
     * Check the ${ENV_VAR} component of a path and adds it to the resolver's path list if it
     * is not yet in the list.
     */
    void checkEnvVarPath( const wxString& aPath );

    wxString expandVars( const wxString& aPath );

    std::list< SEARCH_PATH > m_Paths; ///< List of base search paths.

    ///< Mapping of (short) file names to resolved names.
    std::map< wxString, wxString, S3D::rsort_wxString > m_NameMap;
    int m_errflags;
    wxString m_curProjDir;

    ///< Environment variables.
    std::map< wxString, wxString > m_EnvVars;
};

#endif  // RESOLVER_3D_H

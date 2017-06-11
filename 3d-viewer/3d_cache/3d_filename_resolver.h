/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file 3d_filename_resolver.h
 * provides an extensible class to resolve 3D model paths. Initially
 * the legacy behavior will be implemented and an incomplete path
 * would be checked against the project directory or the KISYS3DMOD
 * environment variable. In the future a configurable set of search
 * paths may be specified.
 */

#ifndef FILENAME_RESOLVER_3D_H
#define FILENAME_RESOLVER_3D_H

#include <list>
#include <map>
#include <vector>
#include <wx/string.h>
#include "str_rsort.h"

class PGM_BASE;

struct S3D_ALIAS
{
    wxString m_alias;           // alias to the base path
    wxString m_pathvar;         // base path as stored in the config file
    wxString m_pathexp;         // expanded base path
    wxString m_description;     // description of the aliased path
};

class S3D_FILENAME_RESOLVER
{
private:
    wxString m_ConfigDir;           // 3D configuration directory
    std::list< S3D_ALIAS > m_Paths; // list of base paths to search from
    int m_errflags;
    PGM_BASE* m_pgm;
    wxString m_curProjDir;

    /**
     * Function createPathList
     * builds the path list using available information such as
     * KISYS3DMOD and the 3d_path_list configuration file. Invalid
     * paths are silently discarded and removed from the configuration
     * file.
     *
     * @return true if at least one valid path was found
     */
    bool createPathList( void );

    /**
     * Function addPath
     * checks that a path is valid and adds it to the search list
     *
     * @param aPath is the alias set to be checked and added
     * @return true if aPath is valid
     */
    bool addPath( const S3D_ALIAS& aPath );

    /**
     * Function readPathList
     * reads a list of path names from a configuration file
     *
     * @return true if a file was found and contained at least
     * one valid path
     */
    bool readPathList( void );

    /**
     * Function writePathList
     * writes the current path list to a configuration file
     *
     * @return true if the path list was not empty and was
     * successfully written to the configuration file
     */
    bool writePathList( void );

    /**
     * Function checkEnvVarPath
     * checks the ${ENV_VAR} component of a path and adds
     * it to the resolver's path list if it is not yet in
     * the list
     */
    void checkEnvVarPath( const wxString& aPath );

public:
    S3D_FILENAME_RESOLVER();

    /**
     * Function Set3DConfigDir
     * sets the user's configuration directory
     * for 3D models.
     *
     * @param aConfigDir
     * @return true if the call succeeds (directory exists)
     */
    bool Set3DConfigDir( const wxString& aConfigDir );

    /**
     * Function SetProjectDir
     * sets the current KiCad project directory as the first
     * entry in the model path list
     *
     * @param[in]   aProjDir    current project directory
     * @param[out]  flgChanged  optional, set to true if directory was changed
     * @retval      true        success
     * @retval      false       failure
     */
    bool SetProjectDir( const wxString& aProjDir, bool* flgChanged = NULL );
    wxString GetProjectDir( void );

    /**
     * Function SetProgramBase
     * sets a pointer to the application's PGM_BASE instance;
     * the pointer is used to extract the local env vars.
     */
    void SetProgramBase( PGM_BASE* aBase );


    /**
     * Function UpdatePathList
     * clears the current path list and substitutes the given path
     * list, updating the path configuration file on success.
     */
    bool UpdatePathList( std::vector< S3D_ALIAS >& aPathList );

    /**
     * Function ResolvePath
     * determines the full path of the given file name. In the future
     * remote files may be supported, in which case it is best to
     * require a full URI in which case ResolvePath should check that
     * the URI conforms to RFC-2396 and related documents and copies
     * aFileName into aResolvedName if the URI is valid.
     */
    wxString ResolvePath( const wxString& aFileName );

    /**
     * Function ShortenPath
     * produces a relative path based on the existing
     * search directories or returns the same path if
     * the path is not a superset of an existing search path.
     *
     * @param aFullPathName is an absolute path to shorten
     * @return the shortened path or aFullPathName
     */
    wxString ShortenPath( const wxString& aFullPathName );

    /**
     * Function GetPaths
     * returns a pointer to the internal path list; the items in:load
     *
     * the list can be used to set up the list of search paths
     * available to a 3D file browser.
     *
     * @return pointer to the internal path list
     */
    const std::list< S3D_ALIAS >* GetPaths( void );

    /**
     * Function SplitAlias
     * returns true if the given name contains an alias and
     * populates the string anAlias with the alias and aRelPath
     * with the relative path.
     */
    bool SplitAlias( const wxString& aFileName, wxString& anAlias, wxString& aRelPath );

    /**
     * Function ValidateName
     * returns true if the given path is a valid aliased relative path.
     * If the path contains an alias then hasAlias is set true.
     */
    bool ValidateFileName( const wxString& aFileName, bool& hasAlias );

    /**
     * Function GetKicadPaths
     * returns a list of path environment variables local to Kicad;
     * this list always includes KISYS3DMOD even if it is not
     * defined locally.
     */
    bool GetKicadPaths( std::list< wxString >& paths );
};

#endif  // FILENAME_RESOLVER_3D_H

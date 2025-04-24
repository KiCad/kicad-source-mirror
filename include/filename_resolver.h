/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file filename_resolver.h
 */

#ifndef FILENAME_RESOLVER_H
#define FILENAME_RESOLVER_H

#include <list>
#include <map>
#include <vector>
#include <wx/string.h>

class PROJECT;
class PGM_BASE;
class EMBEDDED_FILES;

struct SEARCH_PATH
{
    wxString m_Alias;           ///< Alias to the base path.
    wxString m_Pathvar;         ///< Base path as stored in the configuration file.
    wxString m_Pathexp;         ///< Expanded base path.
    wxString m_Description;     ///< Description of the aliased path.
};


/**
 * Provide an extensible class to resolve 3D model paths.
 *
 * Initially the legacy behavior will be implemented and an incomplete path would be checked
 * against the project directory or the KICAD7_3DMODEL_DIR environment variable. In the future a
 * configurable set of search paths may be specified.
 */
class FILENAME_RESOLVER
{
public:
    FILENAME_RESOLVER();

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
     * @param[in]   aProjDir    current project directory.
     * @param[out]  flgChanged  optional, set to true if directory was changed.
     * @retval      true        success.
     * @retval      false       failure.
     */
    bool SetProject( const PROJECT* aProject, bool* flgChanged = nullptr );

    wxString GetProjectDir() const;

    /**
     * Set a pointer to the application's #PGM_BASE instance used to extract the local env vars.
     */
    void SetProgramBase( PGM_BASE* aBase );


    /**
     * Clear the current path list and substitutes the given path list and update the path
     * configuration file on success.
     */
    bool UpdatePathList( const std::vector<SEARCH_PATH>& aPathList );

    /**
     * Determine the full path of the given file name.
     *
     * In the future remote files may be supported, in which case it is best to require a full
     * URI in which case ResolvePath should check that the URI conforms to RFC-2396 and related
     * documents and copies \a aFileName into aResolvedName if the URI is valid.
     *
     * @param aFileName The configured file path to resolve.
     * @param aWorkingPath The current working path for relative path resolutions.
     * @param aEmbeddedFilesStack is a list of pointers to the embedded files list.  They will
     *                            be searched from the front of the list.
     */
    wxString ResolvePath( const wxString& aFileName, const wxString& aWorkingPath,
                          std::vector<const EMBEDDED_FILES*> aEmbeddedFilesStack );

    /**
     * Produce a relative path based on the existing search directories or returns the same path
     * if the path is not a superset of an existing search path.
     *
     * @param aFullPathName is an absolute path to shorten.
     * @return the shortened path or \a aFullPathName.
     */
    wxString ShortenPath( const wxString& aFullPathName );

    /**
     * Return a pointer to the internal path list; the items in:load.
     *
     * The list can be used to set up the list of search paths available to a 3D file browser.
     *
     * @return pointer to the internal path list.
     */
    const std::list<SEARCH_PATH>* GetPaths() const;

    /**
     * Return true if the given name contains an alias and populates the string \a anAlias
     * with the alias and \a aRelPath with the relative path.
     */
    bool SplitAlias( const wxString& aFileName, wxString& anAlias, wxString& aRelPath ) const;

    /**
     * Return true if the given path is a valid aliased relative path.
     *
     * If the path contains an alias then hasAlias is set true.
     */
    bool ValidateFileName( const wxString& aFileName, bool& hasAlias ) const;

    /**
     * Return a list of path environment variables local to KiCad.
     *
     * This list always includes KICAD7_3DMODEL_DIR even if it is not defined locally.
     */
    bool GetKicadPaths( std::list< wxString >& paths ) const;

private:
    /**
     * Build the path list using available information such as KICAD7_3DMODEL_DIR and the
     * 3d_path_list configuration file.
     *
     * @warning Invalid paths are silently discarded and removed from the configuration file.
     *
     * @return true if at least one valid path was found.
     */
    bool createPathList( void );

    /**
     * Check that a path is valid and adds it to the search list.
     *
     * @param aPath is the alias set to be checked and added.
     * @return true if aPath is valid.
     */
    bool addPath( const SEARCH_PATH& aPath );

    /**
     * Check the ${ENV_VAR} component of a path and adds it to the resolver's path list if
     * it is not yet in the list.
     */
    void checkEnvVarPath( const wxString& aPath );

    wxString               m_configDir;     ///< 3D configuration directory.
    std::list<SEARCH_PATH> m_paths;         ///< List of base paths to search from.
    int                    m_errflags;
    PGM_BASE*              m_pgm;
    const PROJECT*         m_project;
    wxString               m_curProjDir;
};

#endif  // FILENAME_RESOLVER_H

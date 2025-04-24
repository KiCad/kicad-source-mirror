/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file 3d_cache.h
 */

#ifndef CACHE_3D_H
#define CACHE_3D_H

#include "3d_info.h"
#include <core/typeinfo.h>
#include "string_utils.h"
#include <hash_128.h>
#include <list>
#include <map>
#include "plugins/3dapi/c3dmodel.h"
#include <project.h>
#include <wx/string.h>

class  EMBEDDED_FILES;
class  PGM_BASE;
class  S3D_CACHE_ENTRY;
class  SCENEGRAPH;
class  FILENAME_RESOLVER;
class  S3D_PLUGIN_MANAGER;


/**
 * Cache for storing the 3D shapes. This cache is able to be stored as a project
 * element (since it inherits from PROJECT::_ELEM).
 */
class S3D_CACHE : public PROJECT::_ELEM
{
public:
    S3D_CACHE();
    virtual ~S3D_CACHE();

    PROJECT::ELEM ProjectElementType() noexcept override
    {
        return PROJECT::ELEM::S3DCACHE;
    }

    /**
     * Set the configuration directory to be used by the model manager for storing 3D
     * model manager configuration data and the model cache.
     *
     * The config directory may only be set once in the lifetime of the object.
     *
     * @param aConfigDir is the configuration directory to use for 3D model manager data
     * @return true on success
     */
    bool Set3DConfigDir( const wxString& aConfigDir );

    /**
     * Set the current project's working directory; this affects the model search path.
     */
    bool SetProject( PROJECT* aProject );

    /**
     * Set the filename resolver's pointer to the application's PGM_BASE instance.
     *
     * The pointer is used to extract the local environment variables.
     */
    void SetProgramBase( PGM_BASE* aBase );

    /**
     * Attempt to load the scene data for a model.
     *
     * It will consult the internal cache list and load from cache if possible before invoking
     * the load() function of the available plugins.  The model may fail to load if, for example,
     * the plugin does not support rendering of the 3D model.

     *
     * @param aModelFile is the partial or full path to the model to be loaded.
     * @param aBasePath is the path to search for any relative files
     * @param aEmbeddedFilesStack is a list of pointers to the embedded files list.  They will
     *                            be searched from the front of the list.
     * @return true if the model was successfully loaded, otherwise false.
     */
    SCENEGRAPH* Load( const wxString& aModelFile, const wxString& aBasePath,
                      std::vector<const EMBEDDED_FILES*> aEmbeddedFilesStack );

    FILENAME_RESOLVER* GetResolver() noexcept;

    /**
     * Return the list of file filters retrieved from the plugins.
     *
     * This will contain at least the default "All Files (*.*)|*.*"
     *
     * @return a pointer to the filter list.
     */
    std::list< wxString > const* GetFileFilters() const;

    /**
     * Free all data in the cache and by default closes all plugins.
     */
    void FlushCache( bool closePlugins = true );

    /**
     * Unload plugins to free memory.
     */
    void ClosePlugins();

    /**
     * Attempt to load the scene data for a model and to translate it into an S3D_MODEL
     * structure for display by a renderer.
     *
     * @param aModelFileName is the full path to the model to be loaded.
     * @param aBasePath is the path to search for any relative files.
     * @param aEmbeddedFilesStack is a stack of pointers to the embedded files lists.  They will
     *                            be searched from the bottom of the stack.
     * @return is a pointer to the render data or NULL if not available.
     */
    S3DMODEL* GetModel( const wxString& aModelFileName, const wxString& aBasePath,
                        std::vector<const EMBEDDED_FILES*> aEmbeddedFilesStack );

    /**
     * Delete up old cache files in cache directory.
     *
     * Deletes ".3dc" files in the cache directory that are older than \a aNumDaysOld.
     *
     * @param aNumDaysOld is age threshold to delete ".3dc" cache files.
     */
    void CleanCacheDir( int aNumDaysOld );

private:
    /**
     * Find or create cache entry for file name
     *
     * Searches the cache list for the given filename and retrieves the cache data; a cache
     * entry is created if one does not already exist.
     *
     * @param aFileName  is the file name (full or partial path).
     * @param aCachePtr is an optional return address for cache entry pointer.
     * @return SCENEGRAPH object associated with file name or NULL on error.
     */
    SCENEGRAPH* checkCache( const wxString& aFileName, S3D_CACHE_ENTRY** aCachePtr = nullptr );

    /**
     * Calculate the SHA1 hash of the given file.
     *
     * @param aFileName file name (full path).
     * @param aHash a 128 bit hash to hold the hash.
     * @return true on  success, otherwise false.
     */
    bool getHash( const wxString& aFileName, HASH_128& aHash );

    // load scene data from a cache file
    bool loadCacheData( S3D_CACHE_ENTRY* aCacheItem );

    // save scene data to a cache file
    bool saveCacheData( S3D_CACHE_ENTRY* aCacheItem );

    // the real load function (can supply a cache entry pointer to member functions)
    SCENEGRAPH* load( const wxString& aModelFile, const wxString& aBasePath,
                      S3D_CACHE_ENTRY** aCachePtr = nullptr,
                      std::vector<const EMBEDDED_FILES*> aEmbeddedFilesStack = {} );

    /// Cache entries.
    std::list< S3D_CACHE_ENTRY* > m_CacheList;

    /// Mapping of file names to cache names and data.
    std::map< wxString, S3D_CACHE_ENTRY*, rsort_wxString > m_CacheMap;

    FILENAME_RESOLVER*  m_FNResolver;

    S3D_PLUGIN_MANAGER* m_Plugins;

    PROJECT*            m_project;
    wxString            m_CacheDir;
    wxString            m_ConfigDir;       ///< base configuration path for 3D items.
};

#endif  // CACHE_3D_H

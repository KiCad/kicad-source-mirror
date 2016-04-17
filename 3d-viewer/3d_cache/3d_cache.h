/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * defines the display data cache manager for 3D models
 */

#ifndef CACHE_3D_H
#define CACHE_3D_H

#include <list>
#include <map>
#include <wx/string.h>
#include "str_rsort.h"
#include "3d_filename_resolver.h"
#include "3d_info.h"
#include "plugins/3dapi/c3dmodel.h"


class  PGM_BASE;
class  S3D_CACHE;
class  S3D_CACHE_ENTRY;
class  SCENEGRAPH;
class  S3D_FILENAME_RESOLVER;
class  S3D_PLUGIN_MANAGER;
struct S3D_INFO;


class S3D_CACHE
{
private:
    /// cache entries
    std::list< S3D_CACHE_ENTRY* > m_CacheList;

    /// mapping of file names to cache names and data
    std::map< wxString, S3D_CACHE_ENTRY*, S3D::rsort_wxString > m_CacheMap;

    /// object to resolve file names
    S3D_FILENAME_RESOLVER* m_FNResolver;

    /// plugin manager
    S3D_PLUGIN_MANAGER* m_Plugins;

    /// set true if the cache needs to be updated
    bool m_DirtyCache;

    /// 3D cache directory
    wxString m_CacheDir;

    /// base configuration path for 3D items
    wxString m_ConfigDir;

    /// current KiCad project dir
    wxString m_ProjDir;

    /**
     * Function checkCache
     * searches the cache list for the given filename and retrieves
     * the cache data; a cache entry is created if one does not
     * already exist
     *
     * @param aFileName [in] is a partial or full file path
     * @param [out] if not NULL will hold a pointer to the cache entry for the model
     * @return on success a pointer to a SCENEGRAPH, otherwise NULL
     */
    SCENEGRAPH* checkCache( const wxString& aFileName, S3D_CACHE_ENTRY** aCachePtr = NULL );

    /**
     * Function getSHA1
     * calculates the SHA1 hash of the given file
     *
     * @param aFileName [in] is a fully qualified path to the model file
     * @param aSHA1Sum [out] is a 20-byte character array to hold the SHA1 hash
     * @return true if the sha1 hash was calculated; otherwise false
     */
    bool getSHA1( const wxString& aFileName, unsigned char* aSHA1Sum );

    // load scene data from a cache file
    bool loadCacheData( S3D_CACHE_ENTRY* aCacheItem );

    // save scene data to a cache file
    bool saveCacheData( S3D_CACHE_ENTRY* aCacheItem );

    // the real load function (can supply a cache entry pointer to member functions)
    SCENEGRAPH* load( const wxString& aModelFile, S3D_CACHE_ENTRY** aCachePtr = NULL );

public:
    S3D_CACHE();
    virtual ~S3D_CACHE();

    /**
     * Function Set3DConfigDir
     * Sets the configuration directory to be used by the
     * model manager for storing 3D model manager configuration
     * data and the model cache. The config directory may only be
     * set once in the lifetime of the object.
     *
     * @param aConfigDir is the configuration directory to use
     * for 3D model manager data
     * @return true on success
     */
    bool Set3DConfigDir( const wxString& aConfigDir );

    /**
     * Function Get3DConfigDir
     * returns the current 3D configuration directory on
     * success, otherwise it returns wxEmptyString. If the
     * directory was not previously set via Set3DConfigDir()
     * then a default is used which is based on kicad's
     * configuration directory code as of September 2015.
     */
    wxString Get3DConfigDir( bool createDefault = false );

    /**
     * Function SetProjectDir
     * sets the current project's working directory; this
     * affects the model search path
     */
    bool SetProjectDir( const wxString& aProjDir );

    /**
     * Function SetProgramBase
     * sets the filename resolver's pointer to the application's
     * PGM_BASE instance; the pointer is used to extract the
     * local env vars.
     */
    void SetProgramBase( PGM_BASE* aBase );

    /**
     * Function GetProjectDir
     * returns the current project's working directory
     */
    wxString GetProjectDir( void );

    /**
     * Function Load
     * attempts to load the scene data for a model; it will consult the
     * internal cache list and load from cache if possible before invoking
     * the load() function of the available plugins.
     *
     * @param aModelFile [in] is the partial or full path to the model to be loaded
     * @return true if the model was successfully loaded, otherwise false.
     * The model may fail to load if, for example, the plugin does not
     * support rendering of the 3D model.
     */
    SCENEGRAPH* Load( const wxString& aModelFile );

    S3D_FILENAME_RESOLVER* GetResolver( void );

    /**
     * Function GetFileFilters
     * returns the list of file filters retrieved from the plugins;
     * this will contain at least the default "All Files (*.*)|*.*"
     *
     * @return a pointer to the filter list
     */
    std::list< wxString > const* GetFileFilters( void ) const;

    /**
     * Function FlushCache
     * frees all data in the cache and by default closes all plugins
     */
    void FlushCache( bool closePlugins = true );

    /**
     * Function ClosePlugins
     * unloads plugins to free memory
     */
    void ClosePlugins( void );

    /**
     * Function GetModel
     * attempts to load the scene data for a model and to translate it
     * into an S3D_MODEL structure for display by a renderer
     *
     * @param aModelFileName is the full path to the model to be loaded
     * @return is a pointer to the render data or NULL if not available
     */
    S3DMODEL* GetModel( const wxString& aModelFileName );

    wxString GetModelHash( const wxString& aModelFileName );
};

#endif  // CACHE_3D_H

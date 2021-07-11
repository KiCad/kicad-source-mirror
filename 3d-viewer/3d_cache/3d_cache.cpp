/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright (C) 2018-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#define GLM_FORCE_RADIANS

#include <mutex>
#include <utility>

#include <wx/datetime.h>
#include <wx/dir.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/stdpaths.h>

#include <boost/version.hpp>

#if BOOST_VERSION >= 106800
#include <boost/uuid/detail/sha1.hpp>
#else
#include <boost/uuid/sha1.hpp>
#endif

#include "3d_cache.h"
#include "3d_info.h"
#include "3d_plugin_manager.h"
#include "sg/scenegraph.h"
#include "plugins/3dapi/ifsg_api.h"

#include <advanced_config.h>
#include <common.h>     // For ExpandEnvVarSubstitutions
#include <filename_resolver.h>
#include <paths.h>
#include <pgm_base.h>
#include <project.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>


#define MASK_3D_CACHE "3D_CACHE"

static std::mutex mutex3D_cache;
static std::mutex mutex3D_cacheManager;


static bool isSHA1Same( const unsigned char* shaA, const unsigned char* shaB ) noexcept
{
    for( int i = 0; i < 20; ++i )
    {
        if( shaA[i] != shaB[i] )
            return false;
    }

    return true;
}


static bool checkTag( const char* aTag, void* aPluginMgrPtr )
{
    if( nullptr == aTag || nullptr == aPluginMgrPtr )
        return false;

    S3D_PLUGIN_MANAGER *pp = (S3D_PLUGIN_MANAGER*) aPluginMgrPtr;

    return pp->CheckTag( aTag );
}


static const wxString sha1ToWXString( const unsigned char* aSHA1Sum )
{
    unsigned char uc;
    unsigned char tmp;
    char          sha1[41];
    int           j = 0;

    for( int i = 0; i < 20; ++i )
    {
        uc = aSHA1Sum[i];
        tmp = uc / 16;

        if( tmp > 9 )
            tmp += 87;
        else
            tmp += 48;

        sha1[j++] = tmp;
        tmp = uc % 16;

        if( tmp > 9 )
            tmp += 87;
        else
            tmp += 48;

        sha1[j++] = tmp;
    }

    sha1[j] = 0;

    return wxString::FromUTF8Unchecked( sha1 );
}


class S3D_CACHE_ENTRY
{
public:
    S3D_CACHE_ENTRY();
    ~S3D_CACHE_ENTRY();

    void SetSHA1( const unsigned char* aSHA1Sum );
    const wxString GetCacheBaseName();

    wxDateTime    modTime;      // file modification time
    unsigned char sha1sum[20];
    std::string   pluginInfo;   // PluginName:Version string
    SCENEGRAPH*   sceneData;
    S3DMODEL*     renderData;

private:
    // prohibit assignment and default copy constructor
    S3D_CACHE_ENTRY( const S3D_CACHE_ENTRY& source );
    S3D_CACHE_ENTRY& operator=( const S3D_CACHE_ENTRY& source );

    wxString m_CacheBaseName;  // base name of cache file (a SHA1 digest)
};


S3D_CACHE_ENTRY::S3D_CACHE_ENTRY()
{
    sceneData = nullptr;
    renderData = nullptr;
    memset( sha1sum, 0, 20 );
}


S3D_CACHE_ENTRY::~S3D_CACHE_ENTRY()
{
    delete sceneData;

    if( nullptr != renderData )
        S3D::Destroy3DModel( &renderData );
}


void S3D_CACHE_ENTRY::SetSHA1( const unsigned char* aSHA1Sum )
{
    if( nullptr == aSHA1Sum )
    {
        wxLogTrace( MASK_3D_CACHE, "%s:%s:%d\n * [BUG] NULL passed for aSHA1Sum",
                    __FILE__, __FUNCTION__, __LINE__ );

        return;
    }

    memcpy( sha1sum, aSHA1Sum, 20 );
}


const wxString S3D_CACHE_ENTRY::GetCacheBaseName()
{
    if( m_CacheBaseName.empty() )
        m_CacheBaseName = sha1ToWXString( sha1sum );

    return m_CacheBaseName;
}


S3D_CACHE::S3D_CACHE()
{
    m_FNResolver = new FILENAME_RESOLVER;
    m_project = nullptr;
    m_Plugins = new S3D_PLUGIN_MANAGER;
}


S3D_CACHE::~S3D_CACHE()
{
    COMMON_SETTINGS* commonSettings = Pgm().GetCommonSettings();

    FlushCache();

    // We'll delete ".3dc" cache files older than this many days
    int clearCacheInterval = commonSettings->m_System.clear_3d_cache_interval;

    // An interval of zero means the user doesn't want to ever clear the cache

    if( clearCacheInterval > 0 )
        CleanCacheDir( clearCacheInterval );

    delete m_FNResolver;
    delete m_Plugins;
}


SCENEGRAPH* S3D_CACHE::load( const wxString& aModelFile, S3D_CACHE_ENTRY** aCachePtr )
{
    if( aCachePtr )
        *aCachePtr = nullptr;

    wxString full3Dpath = m_FNResolver->ResolvePath( aModelFile );

    if( full3Dpath.empty() )
    {
        // the model cannot be found; we cannot proceed
        wxLogTrace( MASK_3D_CACHE, "%s:%s:%d\n * [3D model] could not find model '%s'\n",
                    __FILE__, __FUNCTION__, __LINE__, aModelFile );
        return nullptr;
    }

    // check cache if file is already loaded
    std::lock_guard<std::mutex> lock( mutex3D_cache );

    std::map< wxString, S3D_CACHE_ENTRY*, rsort_wxString >::iterator mi;
    mi = m_CacheMap.find( full3Dpath );

    if( mi != m_CacheMap.end() )
    {
        wxFileName fname( full3Dpath );

        if( fname.FileExists() )    // Only check if file exists. If not, it will
        {                           // use the same model in cache.
            bool reload = false;
            wxDateTime fmdate = fname.GetModificationTime();

            if( fmdate != mi->second->modTime )
            {
                unsigned char hashSum[20];
                getSHA1( full3Dpath, hashSum );
                mi->second->modTime = fmdate;

                if( !isSHA1Same( hashSum, mi->second->sha1sum ) )
                {
                    mi->second->SetSHA1( hashSum );
                    reload = true;
                }
            }

            if( reload )
            {
                if( nullptr != mi->second->sceneData )
                {
                    S3D::DestroyNode( mi->second->sceneData );
                    mi->second->sceneData = nullptr;
                }

                if( nullptr != mi->second->renderData )
                    S3D::Destroy3DModel( &mi->second->renderData );

                mi->second->sceneData = m_Plugins->Load3DModel( full3Dpath,
                                                                mi->second->pluginInfo );
            }
        }

        if( nullptr != aCachePtr )
            *aCachePtr = mi->second;

        return mi->second->sceneData;
    }

    // a cache item does not exist; search the Filename->Cachename map
    return checkCache( full3Dpath, aCachePtr );
}


SCENEGRAPH* S3D_CACHE::Load( const wxString& aModelFile )
{
    return load( aModelFile );
}


SCENEGRAPH* S3D_CACHE::checkCache( const wxString& aFileName, S3D_CACHE_ENTRY** aCachePtr )
{
    if( aCachePtr )
        *aCachePtr = nullptr;

    unsigned char sha1sum[20];

    if( !getSHA1( aFileName, sha1sum ) || m_CacheDir.empty() )
    {
        // just in case we can't get a hash digest (for example, on access issues)
        // or we do not have a configured cache file directory, we create an
        // entry to prevent further attempts at loading the file
        S3D_CACHE_ENTRY* ep = new S3D_CACHE_ENTRY;
        m_CacheList.push_back( ep );
        wxFileName fname( aFileName );
        ep->modTime = fname.GetModificationTime();

        if( m_CacheMap.insert( std::pair< wxString, S3D_CACHE_ENTRY* >
            ( aFileName, ep ) ).second == false )
        {
            wxLogTrace( MASK_3D_CACHE, "%s:%s:%d\n * [BUG] duplicate entry in map file; key = '%s'",
                        __FILE__, __FUNCTION__, __LINE__, aFileName );

            m_CacheList.pop_back();
            delete ep;
        }
        else
        {
            if( aCachePtr )
                *aCachePtr = ep;
        }

        return nullptr;
    }

    S3D_CACHE_ENTRY* ep = new S3D_CACHE_ENTRY;
    m_CacheList.push_back( ep );
    wxFileName fname( aFileName );
    ep->modTime = fname.GetModificationTime();

    if( m_CacheMap.insert( std::pair< wxString, S3D_CACHE_ENTRY* >
                               ( aFileName, ep ) ).second == false )
    {
        wxLogTrace( MASK_3D_CACHE, "%s:%s:%d\n * [BUG] duplicate entry in map file; key = '%s'",
                    __FILE__, __FUNCTION__, __LINE__, aFileName );

        m_CacheList.pop_back();
        delete ep;
        return nullptr;
    }

    if( aCachePtr )
        *aCachePtr = ep;

    ep->SetSHA1( sha1sum );

    wxString bname = ep->GetCacheBaseName();
    wxString cachename = m_CacheDir + bname + wxT( ".3dc" );

    if( !ADVANCED_CFG::GetCfg().m_Skip3DFileCache && wxFileName::FileExists( cachename )
        && loadCacheData( ep ) )
        return ep->sceneData;

    ep->sceneData = m_Plugins->Load3DModel( aFileName, ep->pluginInfo );

    if( !ADVANCED_CFG::GetCfg().m_Skip3DFileCache && nullptr != ep->sceneData )
        saveCacheData( ep );

    return ep->sceneData;
}


bool S3D_CACHE::getSHA1( const wxString& aFileName, unsigned char* aSHA1Sum )
{
    if( aFileName.empty() )
    {
        wxLogTrace( MASK_3D_CACHE, "%s:%s:%d\n * [BUG] empty filename",
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    if( nullptr == aSHA1Sum )
    {
        wxLogTrace( MASK_3D_CACHE, "%s\n * [BUG] NULL pointer passed for aMD5Sum",
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

#ifdef _WIN32
    FILE* fp = _wfopen( aFileName.wc_str(), L"rb" );
#else
    FILE* fp = fopen( aFileName.ToUTF8(), "rb" );
#endif

    if( nullptr == fp )
        return false;

    boost::uuids::detail::sha1 dblock;
    unsigned char block[4096];
    size_t bsize = 0;

    while( ( bsize = fread( &block, 1, 4096, fp ) ) > 0 )
        dblock.process_bytes( block, bsize );

    fclose( fp );
    unsigned int digest[5];
    dblock.get_digest( digest );

    // ensure MSB order
    for( int i = 0; i < 5; ++i )
    {
        int idx = i << 2;
        unsigned int tmp = digest[i];
        aSHA1Sum[idx+3] = tmp & 0xff;
        tmp >>= 8;
        aSHA1Sum[idx+2] = tmp & 0xff;
        tmp >>= 8;
        aSHA1Sum[idx+1] = tmp & 0xff;
        tmp >>= 8;
        aSHA1Sum[idx] = tmp & 0xff;
    }

    return true;
}


bool S3D_CACHE::loadCacheData( S3D_CACHE_ENTRY* aCacheItem )
{
    wxString bname = aCacheItem->GetCacheBaseName();

    if( bname.empty() )
    {
        wxLogTrace( MASK_3D_CACHE,
                    " * [3D model] cannot load cached model; no file hash available" );

        return false;
    }

    if( m_CacheDir.empty() )
    {
        wxLogTrace( MASK_3D_CACHE,
                    " * [3D model] cannot load cached model; config directory unknown" );

        return false;
    }

    wxString fname = m_CacheDir + bname + wxT( ".3dc" );

    if( !wxFileName::FileExists( fname ) )
    {
        wxString errmsg = "cannot open file";
        wxLogTrace( MASK_3D_CACHE, " * [3D model] %s '%s'", errmsg.GetData(), fname.GetData() );
        return false;
    }

    if( nullptr != aCacheItem->sceneData )
        S3D::DestroyNode( (SGNODE*) aCacheItem->sceneData );

    aCacheItem->sceneData = (SCENEGRAPH*)S3D::ReadCache( fname.ToUTF8(), m_Plugins, checkTag );

    if( nullptr == aCacheItem->sceneData )
        return false;

    return true;
}


bool S3D_CACHE::saveCacheData( S3D_CACHE_ENTRY* aCacheItem )
{
    if( nullptr == aCacheItem )
    {
        wxLogTrace( MASK_3D_CACHE, "%s:%s:%d\n * NULL passed for aCacheItem",
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    if( nullptr == aCacheItem->sceneData )
    {
        wxLogTrace( MASK_3D_CACHE, "%s:%s:%d\n * aCacheItem has no valid scene data",
                    __FILE__, __FUNCTION__, __LINE__ );

        return false;
    }

    wxString bname = aCacheItem->GetCacheBaseName();

    if( bname.empty() )
    {
        wxLogTrace( MASK_3D_CACHE,
                    " * [3D model] cannot load cached model; no file hash available" );

        return false;
    }

    if( m_CacheDir.empty() )
    {
        wxLogTrace( MASK_3D_CACHE,
                    " * [3D model] cannot load cached model; config directory unknown" );

        return false;
    }

    wxString fname = m_CacheDir + bname + wxT( ".3dc" );

    if( wxFileName::Exists( fname ) )
    {
        if( !wxFileName::FileExists( fname ) )
        {
            wxLogTrace( MASK_3D_CACHE, " * [3D model] path exists but is not a regular file '%s'",
                        fname );

            return false;
        }
    }

    return S3D::WriteCache( fname.ToUTF8(), true, (SGNODE*)aCacheItem->sceneData,
                            aCacheItem->pluginInfo.c_str() );
}


bool S3D_CACHE::Set3DConfigDir( const wxString& aConfigDir )
{
    if( !m_ConfigDir.empty() )
        return false;

    wxFileName cfgdir( ExpandEnvVarSubstitutions( aConfigDir, m_project ), "" );

    cfgdir.Normalize();

    if( !cfgdir.DirExists() )
    {
        cfgdir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

        if( !cfgdir.DirExists() )
        {
            wxLogTrace( MASK_3D_CACHE,
                        "%s:%s:%d\n * failed to create 3D configuration directory '%s'",
                        __FILE__, __FUNCTION__, __LINE__, cfgdir.GetPath() );

            return false;
        }
    }

    m_ConfigDir = cfgdir.GetPath();

    // inform the file resolver of the config directory
    if( !m_FNResolver->Set3DConfigDir( m_ConfigDir ) )
    {
        wxLogTrace( MASK_3D_CACHE,
                    "%s:%s:%d\n * could not set 3D Config Directory on filename resolver\n"
                    " * config directory: '%s'",
                    __FILE__, __FUNCTION__, __LINE__, m_ConfigDir );
    }

    // 3D cache data must go to a user's cache directory;
    // unfortunately wxWidgets doesn't seem to provide
    // functions to retrieve such a directory.
    //
    // 1. OSX: ~/Library/Caches/kicad/3d/
    // 2. Linux: ${XDG_CACHE_HOME}/kicad/3d ~/.cache/kicad/3d/
    // 3. MSWin: AppData\Local\kicad\3d
    wxFileName cacheDir;
    cacheDir.AssignDir( PATHS::GetUserCachePath() );
    cacheDir.AppendDir( "3d" );

    if( !cacheDir.DirExists() )
    {
        cacheDir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

        if( !cacheDir.DirExists() )
        {
            wxLogTrace( MASK_3D_CACHE, "%s:%s:%d\n * failed to create 3D cache directory '%s'",
                        __FILE__, __FUNCTION__, __LINE__, cacheDir.GetPath() );

            return false;
        }
    }

    m_CacheDir = cacheDir.GetPathWithSep();
    return true;
}


bool S3D_CACHE::SetProject( PROJECT* aProject )
{
    m_project = aProject;

    bool hasChanged = false;

    if( m_FNResolver->SetProject( aProject, &hasChanged ) && hasChanged )
    {
        m_CacheMap.clear();

        std::list< S3D_CACHE_ENTRY* >::iterator sL = m_CacheList.begin();
        std::list< S3D_CACHE_ENTRY* >::iterator eL = m_CacheList.end();

        while( sL != eL )
        {
            delete *sL;
            ++sL;
        }

        m_CacheList.clear();

        return true;
    }

    return false;
}


void S3D_CACHE::SetProgramBase( PGM_BASE* aBase )
{
    m_FNResolver->SetProgramBase( aBase );
}


FILENAME_RESOLVER* S3D_CACHE::GetResolver() noexcept
{
    return m_FNResolver;
}


std::list< wxString > const* S3D_CACHE::GetFileFilters() const
{
    return m_Plugins->GetFileFilters();
}


void S3D_CACHE::FlushCache( bool closePlugins )
{
    std::list< S3D_CACHE_ENTRY* >::iterator sCL = m_CacheList.begin();
    std::list< S3D_CACHE_ENTRY* >::iterator eCL = m_CacheList.end();

    while( sCL != eCL )
    {
        delete *sCL;
        ++sCL;
    }

    m_CacheList.clear();
    m_CacheMap.clear();

    if( closePlugins )
        ClosePlugins();
}


void S3D_CACHE::ClosePlugins()
{
    if( m_Plugins )
        m_Plugins->ClosePlugins();
}


S3DMODEL* S3D_CACHE::GetModel( const wxString& aModelFileName )
{
    S3D_CACHE_ENTRY* cp = nullptr;
    SCENEGRAPH* sp = load( aModelFileName, &cp );

    if( !sp )
        return nullptr;

    if( !cp )
    {
        wxLogTrace( MASK_3D_CACHE,
                    "%s:%s:%d\n  * [BUG] model loaded with no associated S3D_CACHE_ENTRY",
                    __FILE__, __FUNCTION__, __LINE__ );

        return nullptr;
    }

    if( cp->renderData )
        return cp->renderData;

    S3DMODEL* mp = S3D::GetModel( sp );
    cp->renderData = mp;

    return mp;
}

void S3D_CACHE::CleanCacheDir( int aNumDaysOld )
{
    wxDir         dir;
    wxString      fileSpec = wxT( "*.3dc" );
    wxArrayString fileList; // Holds list of ".3dc" files found in cache directory
    size_t        numFilesFound = 0;

    wxFileName thisFile;
    wxDateTime lastAccess, thresholdDate;
    wxDateSpan durationInDays;

    // Calc the threshold date above which we delete cache files
    durationInDays.SetDays( aNumDaysOld );
    thresholdDate = wxDateTime::Now() - durationInDays;

    // If the cache directory can be found and opened, then we'll try and clean it up
    if( dir.Open( m_CacheDir ) )
    {
        thisFile.SetPath( m_CacheDir ); // Set the base path to the cache folder

        // Get a list of all the ".3dc" files in the cache directory
        numFilesFound = dir.GetAllFiles( m_CacheDir, &fileList, fileSpec );

        for( unsigned int i = 0; i < numFilesFound; i++ )
        {
            // Completes path to specific file so we can get its "last access" date
            thisFile.SetFullName( fileList[i] );

            // Only get "last access" time to compare against. Don't need the other 2 timestamps.
            if( thisFile.GetTimes( &lastAccess, nullptr, nullptr ) )
            {
                if( lastAccess.IsEarlierThan( thresholdDate ) )
                {
                    // This file is older than the threshold so delete it
                    wxRemoveFile( thisFile.GetFullPath() );
                }
            }
        }
    }
}


S3D_CACHE* PROJECT::Get3DCacheManager( bool aUpdateProjDir )
{
    std::lock_guard<std::mutex> lock( mutex3D_cacheManager );

    // Get the existing cache from the project
    S3D_CACHE* cache = dynamic_cast<S3D_CACHE*>( GetElem( ELEM_3DCACHE ) );

    if( !cache )
    {
        // Create a cache if there is not one already
        cache = new S3D_CACHE();

        wxFileName cfgpath;
        cfgpath.AssignDir( SETTINGS_MANAGER::GetUserSettingsPath() );
        cfgpath.AppendDir( wxT( "3d" ) );

        cache->SetProgramBase( &Pgm() );
        cache->Set3DConfigDir( cfgpath.GetFullPath() );

        SetElem( ELEM_3DCACHE, cache );
        aUpdateProjDir = true;
    }

    if( aUpdateProjDir )
        cache->SetProject( this );

    return cache;
}


FILENAME_RESOLVER* PROJECT::Get3DFilenameResolver()
{
    return Get3DCacheManager()->GetResolver();
}

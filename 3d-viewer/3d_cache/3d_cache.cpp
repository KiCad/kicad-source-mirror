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

#define GLM_FORCE_RADIANS

#include <iostream>
#include <sstream>
#include <fstream>
#include <utility>
#include <iterator>

#include <wx/datetime.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/stdpaths.h>

#include <boost/uuid/sha1.hpp>

#include <glm/glm.hpp>
#include <glm/ext.hpp>

#include "common.h"
#include "3d_cache.h"
#include "3d_info.h"
#include "sg/scenegraph.h"
#include "3d_filename_resolver.h"
#include "3d_plugin_manager.h"
#include "plugins/3dapi/ifsg_api.h"


#define MASK_3D_CACHE "3D_CACHE"

static wxCriticalSection lock3D_cache;

static bool isSHA1Same( const unsigned char* shaA, const unsigned char* shaB )
{
    for( int i = 0; i < 20; ++i )
        if( shaA[i] != shaB[i] )
            return false;

    return true;
}

static bool checkTag( const char* aTag, void* aPluginMgrPtr )
{
    if( NULL == aTag || NULL == aPluginMgrPtr )
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
private:
    // prohibit assignment and default copy constructor
    S3D_CACHE_ENTRY( const S3D_CACHE_ENTRY& source );
    S3D_CACHE_ENTRY& operator=( const S3D_CACHE_ENTRY& source );

    wxString m_CacheBaseName;  // base name of cache file (a SHA1 digest)

public:
    S3D_CACHE_ENTRY();
    ~S3D_CACHE_ENTRY();

    void SetSHA1( const unsigned char* aSHA1Sum );
    const wxString GetCacheBaseName( void );

    wxDateTime    modTime;      // file modification time
    unsigned char sha1sum[20];
    std::string   pluginInfo;   // PluginName:Version string
    SCENEGRAPH*   sceneData;
    S3DMODEL*     renderData;
};


S3D_CACHE_ENTRY::S3D_CACHE_ENTRY()
{
    sceneData = NULL;
    renderData = NULL;
    memset( sha1sum, 0, 20 );
}


S3D_CACHE_ENTRY::~S3D_CACHE_ENTRY()
{
    if( NULL != sceneData )
        delete sceneData;

    if( NULL != renderData )
        S3D::Destroy3DModel( &renderData );
}


void S3D_CACHE_ENTRY::SetSHA1( const unsigned char* aSHA1Sum )
{
    if( NULL == aSHA1Sum )
    {
        #ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] NULL passed for aSHA1Sum";
            wxLogTrace( MASK_3D_CACHE, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return;
    }

    memcpy( sha1sum, aSHA1Sum, 20 );
    return;
}


const wxString S3D_CACHE_ENTRY::GetCacheBaseName( void )
{
    if( m_CacheBaseName.empty() )
        m_CacheBaseName = sha1ToWXString( sha1sum );

    return m_CacheBaseName;
}


S3D_CACHE::S3D_CACHE()
{
    m_DirtyCache = false;
    m_FNResolver = new S3D_FILENAME_RESOLVER;
    m_Plugins = new S3D_PLUGIN_MANAGER;

    return;
}

S3D_CACHE::~S3D_CACHE()
{
    FlushCache();

    if( m_FNResolver )
        delete m_FNResolver;

    if( m_Plugins )
        delete m_Plugins;

    return;
}


SCENEGRAPH* S3D_CACHE::load( const wxString& aModelFile, S3D_CACHE_ENTRY** aCachePtr )
{
    if( aCachePtr )
        *aCachePtr = NULL;

    wxString full3Dpath = m_FNResolver->ResolvePath( aModelFile );

    if( full3Dpath.empty() )
    {
        // the model cannot be found; we cannot proceed
        wxLogTrace( MASK_3D_CACHE, " * [3D model] could not find model '%s'\n",
            aModelFile.GetData() );
        return NULL;
    }

    // check cache if file is already loaded
    wxCriticalSectionLocker lock( lock3D_cache );
    std::map< wxString, S3D_CACHE_ENTRY*, S3D::rsort_wxString >::iterator mi;
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
                if( NULL != mi->second->sceneData )
                {
                    S3D::DestroyNode( mi->second->sceneData );
                    mi->second->sceneData = NULL;
                }

                if( NULL != mi->second->renderData )
                    S3D::Destroy3DModel( &mi->second->renderData );

                mi->second->sceneData = m_Plugins->Load3DModel( full3Dpath, mi->second->pluginInfo );
            }
        }

        if( NULL != aCachePtr )
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
        *aCachePtr = NULL;

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
            #ifdef DEBUG
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [BUG] duplicate entry in map file; key = '";
                ostr << aFileName.ToUTF8() << "'";
                wxLogTrace( MASK_3D_CACHE, "%s\n", ostr.str().c_str() );
            } while( 0 );
            #endif

            m_CacheList.pop_back();
            delete ep;
        }
        else
        {
            if( aCachePtr )
                *aCachePtr = ep;

        }

        return NULL;
    }

    S3D_CACHE_ENTRY* ep = new S3D_CACHE_ENTRY;
    m_CacheList.push_back( ep );
    wxFileName fname( aFileName );
    ep->modTime = fname.GetModificationTime();

    if( m_CacheMap.insert( std::pair< wxString, S3D_CACHE_ENTRY* >
                               ( aFileName, ep ) ).second == false )
    {
        #ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] duplicate entry in map file; key = '";
            ostr << aFileName.ToUTF8() << "'";
            wxLogTrace( MASK_3D_CACHE, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        m_CacheList.pop_back();
        delete ep;
        return NULL;
    }

    if( aCachePtr )
        *aCachePtr = ep;

    ep->SetSHA1( sha1sum );

    wxString bname = ep->GetCacheBaseName();
    wxString cachename = m_CacheDir + bname + wxT( ".3dc" );

    if( wxFileName::FileExists( cachename ) && loadCacheData( ep ) )
        return ep->sceneData;

    ep->sceneData = m_Plugins->Load3DModel( aFileName, ep->pluginInfo );

    if( NULL != ep->sceneData )
        saveCacheData( ep );

    return ep->sceneData;
}


bool S3D_CACHE::getSHA1( const wxString& aFileName, unsigned char* aSHA1Sum )
{
    if( aFileName.empty() )
    {
        #ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] empty filename";
            wxLogTrace( MASK_3D_CACHE, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    if( NULL == aSHA1Sum )
    {
        #ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] NULL pointer passed for aMD5Sum";
            wxLogTrace( MASK_3D_CACHE, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    #ifdef WIN32
    FILE* fp = _wfopen( aFileName.wc_str(), L"rb" );
    #else
    FILE* fp = fopen( aFileName.ToUTF8(), "rb" );
    #endif

    if( NULL == fp )
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
        #ifdef DEBUG
        wxLogTrace( MASK_3D_CACHE, " * [3D model] cannot load cached model; no file hash available\n" );
        #endif

        return false;
    }

    if( m_CacheDir.empty() )
    {
        wxString errmsg = "cannot load cached model; config directory unknown";
        wxLogTrace( MASK_3D_CACHE, " * [3D model] %s\n", errmsg.GetData() );

        return false;
    }

    wxString fname = m_CacheDir + bname + wxT( ".3dc" );

    if( !wxFileName::FileExists( fname ) )
    {
        wxString errmsg = "cannot open file";
        wxLogTrace( MASK_3D_CACHE, " * [3D model] %s '%s'\n",
            errmsg.GetData(), fname.GetData() );
        return false;
    }

    if( NULL != aCacheItem->sceneData )
        S3D::DestroyNode( (SGNODE*) aCacheItem->sceneData );

    aCacheItem->sceneData = (SCENEGRAPH*)S3D::ReadCache( fname.ToUTF8(), m_Plugins, checkTag );

    if( NULL == aCacheItem->sceneData )
        return false;

    return true;
}


bool S3D_CACHE::saveCacheData( S3D_CACHE_ENTRY* aCacheItem )
{
    if( NULL == aCacheItem )
    {
        #ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * NULL passed for aCacheItem";
            wxLogTrace( MASK_3D_CACHE, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    if( NULL == aCacheItem->sceneData )
    {
        #ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * aCacheItem has no valid scene data";
            wxLogTrace( MASK_3D_CACHE, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return false;
    }

    wxString bname = aCacheItem->GetCacheBaseName();

    if( bname.empty() )
    {
        #ifdef DEBUG
        wxLogTrace( MASK_3D_CACHE, " * [3D model] cannot load cached model; no file hash available\n" );
        #endif

        return false;
    }

    if( m_CacheDir.empty() )
    {
        wxString errmsg = "cannot load cached model; config directory unknown";
        wxLogTrace( MASK_3D_CACHE, " * [3D model] %s\n", errmsg.GetData() );

        return false;
    }

    wxString fname = m_CacheDir + bname + wxT( ".3dc" );

    if( wxFileName::Exists( fname ) )
    {
        if( !wxFileName::FileExists( fname ) )
        {
            wxString errmsg = _( "path exists but is not a regular file" );
            wxLogTrace( MASK_3D_CACHE, " * [3D model] %s '%s'\n", errmsg.GetData(),
                fname.ToUTF8() );

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

    wxFileName cfgdir;

    if( aConfigDir.StartsWith( "${" ) || aConfigDir.StartsWith( "$(" ) )
        cfgdir.Assign( ExpandEnvVarSubstitutions( aConfigDir ), "" );
    else
        cfgdir.Assign( aConfigDir, "" );

    cfgdir.Normalize();

    if( !cfgdir.DirExists() )
    {
        cfgdir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

        if( !cfgdir.DirExists() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            wxString errmsg = _( "failed to create 3D configuration directory" );
            ostr << " * " << errmsg.ToUTF8() << "\n";
            errmsg = _( "config directory" );
            ostr << " * " << errmsg.ToUTF8() << " '";
            ostr << cfgdir.GetPath().ToUTF8() << "'";
            wxLogTrace( MASK_3D_CACHE, "%s\n", ostr.str().c_str() );

            return false;
        }
    }

    m_ConfigDir = cfgdir.GetPath();

    // inform the file resolver of the config directory
    if( !m_FNResolver->Set3DConfigDir( m_ConfigDir ) )
    {
        #ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * could not set 3D Config Directory on filename resolver\n";
            ostr << " * config directory: '" << m_ConfigDir.ToUTF8() << "'";
            wxLogTrace( MASK_3D_CACHE, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif
    }

    // 3D cache data must go to a user's cache directory;
    // unfortunately wxWidgets doesn't seem to provide
    // functions to retrieve such a directory.
    //
    // 1. OSX: ~/Library/Caches/kicad/3d/
    // 2. Linux: ${XDG_CACHE_HOME}/kicad/3d ~/.cache/kicad/3d/
    // 3. MSWin: AppData\Local\kicad\3d
    wxString cacheDir;

    #if defined(_WIN32)
    wxStandardPaths::Get().UseAppInfo( wxStandardPaths::AppInfo_None );
    cacheDir = wxStandardPaths::Get().GetUserLocalDataDir();
    cacheDir.append( "\\kicad\\3d" );
    #elif defined(__APPLE)
    cacheDir = "${HOME}/Library/Caches/kicad/3d";
    #else   // assume Linux
    cacheDir = ExpandEnvVarSubstitutions( "${XDG_CACHE_HOME}" );

    if( cacheDir.empty() || cacheDir == "${XDG_CACHE_HOME}" )
        cacheDir = "${HOME}/.cache";

    cacheDir.append( "/kicad/3d" );
    #endif

    cacheDir = ExpandEnvVarSubstitutions( cacheDir );
    cfgdir.Assign( cacheDir, "" );

    if( !cfgdir.DirExists() )
    {
        cfgdir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

        if( !cfgdir.DirExists() )
        {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            wxString errmsg = "failed to create 3D cache directory";
            ostr << " * " << errmsg.ToUTF8() << "\n";
            errmsg = "cache directory";
            ostr << " * " << errmsg.ToUTF8() << " '";
            ostr << cfgdir.GetPath().ToUTF8() << "'";
            wxLogTrace( MASK_3D_CACHE, "%s\n", ostr.str().c_str() );

            return false;
        }
    }

    m_CacheDir = cfgdir.GetPathWithSep();
    return true;
}


wxString S3D_CACHE::Get3DConfigDir( bool createDefault )
{
    if( !m_ConfigDir.empty() || !createDefault )
        return m_ConfigDir;

    // note: duplicated from common/common.cpp GetKicadConfigPath() to avoid
    // code coupling; ideally the instantiating code should call
    // Set3DConfigDir() to set the directory rather than relying on this
    // directory remaining the same in future KiCad releases.
    wxFileName cfgpath;

    // From the wxWidgets wxStandardPaths::GetUserConfigDir() help:
    //      Unix: ~ (the home directory)
    //      Windows: "C:\Documents and Settings\username\Application Data"
    //      Mac: ~/Library/Preferences
    cfgpath.AssignDir( wxStandardPaths::Get().GetUserConfigDir() );

#if !defined( __WINDOWS__ ) && !defined( __WXMAC__ )
    wxString envstr = ExpandEnvVarSubstitutions( "${XDG_CONFIG_HOME}" );

    if( envstr.IsEmpty() || envstr == "${XDG_CONFIG_HOME}" )
    {
        // XDG_CONFIG_HOME is not set, so use the fallback
        cfgpath.AppendDir( wxT( ".config" ) );
    }
    else
    {
        // Override the assignment above with XDG_CONFIG_HOME
        cfgpath.AssignDir( envstr );
    }
#endif

    cfgpath.AppendDir( wxT( "kicad" ) );
    cfgpath.AppendDir( wxT( "3d" ) );

    if( !cfgpath.DirExists() )
    {
        cfgpath.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );
    }

    if( !cfgpath.DirExists() )
    {
        std::ostringstream ostr;
        wxString errmsg = "failed to create 3D configuration directory";
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * " << errmsg.ToUTF8();
        wxLogTrace( MASK_3D_CACHE, "%s\n", ostr.str().c_str() );

        return wxT( "" );
    }

    if( Set3DConfigDir( cfgpath.GetPath() ) )
        return m_ConfigDir;

    return wxEmptyString;
}


bool S3D_CACHE::SetProjectDir( const wxString& aProjDir )
{
    bool hasChanged = false;

    if( m_FNResolver->SetProjectDir( aProjDir, &hasChanged ) && hasChanged )
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
    return;
}


wxString S3D_CACHE::GetProjectDir( void )
{
    return m_FNResolver->GetProjectDir();
}


S3D_FILENAME_RESOLVER* S3D_CACHE::GetResolver( void )
{
    return m_FNResolver;
}


std::list< wxString > const* S3D_CACHE::GetFileFilters( void ) const
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

    return;
}


void S3D_CACHE::ClosePlugins( void )
{
    if( NULL != m_Plugins )
        m_Plugins->ClosePlugins();

    return;
}


S3DMODEL* S3D_CACHE::GetModel( const wxString& aModelFileName )
{
    S3D_CACHE_ENTRY* cp = NULL;
    SCENEGRAPH* sp = load( aModelFileName, &cp );

    if( !sp )
        return NULL;

    if( !cp )
    {
        #ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            ostr << " * [BUG] model loaded with no associated S3D_CACHE_ENTRY";
            wxLogTrace( MASK_3D_CACHE, "%s\n", ostr.str().c_str() );
        } while( 0 );
        #endif

        return NULL;
    }

    if( cp->renderData )
        return cp->renderData;

    S3DMODEL* mp = S3D::GetModel( sp );
    cp->renderData = mp;

    return mp;
}


wxString S3D_CACHE::GetModelHash( const wxString& aModelFileName )
{
    wxString full3Dpath = m_FNResolver->ResolvePath( aModelFileName );

    if( full3Dpath.empty() || !wxFileName::FileExists( full3Dpath ) )
        return wxEmptyString;

    // check cache if file is already loaded
    std::map< wxString, S3D_CACHE_ENTRY*, S3D::rsort_wxString >::iterator mi;
    mi = m_CacheMap.find( full3Dpath );

    if( mi != m_CacheMap.end() )
        return mi->second->GetCacheBaseName();

    // a cache item does not exist; search the Filename->Cachename map
    S3D_CACHE_ENTRY* cp = NULL;
    checkCache( full3Dpath, &cp );

    if( NULL != cp )
        return cp->GetCacheBaseName();

    return wxEmptyString;
}

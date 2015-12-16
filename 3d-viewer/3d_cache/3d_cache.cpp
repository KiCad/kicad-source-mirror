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

#define GLM_FORCE_RADIANS

#include <iostream>
#include <fstream>
#include <cstdio>
#include <string>
#include <utility>

#include <wx/filename.h>
#include <wx/utils.h>
#include <wx/stdpaths.h>

#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "md5.h"
#include "3d_cache.h"
#include "3d_info.h"
#include "sg/scenegraph.h"
#include "3d_filename_resolver.h"
#include "3d_plugin_manager.h"
#include "plugins/3dapi/ifsg_api.h"


#define CACHE_CONFIG_NAME wxT( "cache.cfg" )



static const wxString md5ToWXString( const unsigned char* aMD5Sum )
{
    unsigned char uc;
    unsigned char tmp;
    char          md5[33];
    int           j = 0;

    for( int i = 0; i < 16; ++i )
    {
        uc = aMD5Sum[i];
        tmp = uc / 16;

        if( tmp > 9 )
            tmp += 87;
        else
            tmp += 48;

        md5[j++] = tmp;
        tmp = uc % 16;

        if( tmp > 9 )
            tmp += 87;
        else
            tmp += 48;

        md5[j++] = tmp;
    }

    md5[j] = 0;

    return wxString::FromUTF8Unchecked( md5 );
}


static bool md5matches( const unsigned char* md5a, const unsigned char* md5b )
{
    for( int i = 0; i < 16; ++i )
    {
        if( md5a[i] != md5b[i] )
            return false;
    }

    return true;
}


static bool isMD5null( const unsigned char* aMD5Sum )
{
    if( NULL == aMD5Sum )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL passed for aMD5Sum\n";
        return false;
    }

    for( int i = 0; i < 16; ++i )
    {
        if( 0 != aMD5Sum[i] )
            return false;
    }

    return true;
}


class S3D_CACHE_ENTRY
{
private:
    // prohibit assignment and default copy constructor
    S3D_CACHE_ENTRY( const S3D_CACHE_ENTRY& source );
    S3D_CACHE_ENTRY& operator=( const S3D_CACHE_ENTRY& source );

    wxString m_CacheBaseName;  // base name of cache file (an MD5 sum)

public:
    S3D_CACHE_ENTRY();
    ~S3D_CACHE_ENTRY();

    void SetMD5( const unsigned char* aMD5Sum );
    const wxString GetCacheBaseName( void );

    unsigned char md5sum[16];
    SCENEGRAPH* sceneData;
    S3DMODEL*   renderData;
};


S3D_CACHE_ENTRY::S3D_CACHE_ENTRY()
{
    sceneData = NULL;
    renderData = NULL;
    memset( md5sum, 0, 16 );
}


S3D_CACHE_ENTRY::~S3D_CACHE_ENTRY()
{
    if( NULL != sceneData )
        delete sceneData;

    if( NULL != renderData )
        S3D::Destroy3DModel( &renderData );
}


void S3D_CACHE_ENTRY::SetMD5( const unsigned char* aMD5Sum )
{
    if( NULL == aMD5Sum )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL passed for aMD5Sum\n";
        return;
    }

    memcpy( md5sum, aMD5Sum, 16 );
    return;
}


const wxString S3D_CACHE_ENTRY::GetCacheBaseName( void )
{
    if( m_CacheBaseName.empty() )
        m_CacheBaseName = md5ToWXString( md5sum );

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
        std::cout << " * [3D model] could not find model '";
        std::cout << aModelFile.ToUTF8() << "'\n";
        return NULL;
    }

    // check cache if file is already loaded
    std::map< wxString, S3D_CACHE_ENTRY*, S3D::rsort_wxString >::iterator mi;
    mi = m_CacheMap.find( full3Dpath );

    if( mi != m_CacheMap.end() )
    {
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

    unsigned char md5sum[16];

    if( !getMD5( aFileName, md5sum ) || m_CacheDir.empty() )
    {
        // just in case we can't get an MD5 sum (for example, on access issues)
        // or we do not have a configured cache file directory, we create an
        // entry to prevent further attempts at loading the file
        S3D_CACHE_ENTRY* ep = new S3D_CACHE_ENTRY;
        m_CacheList.push_back( ep );

        if( m_CacheMap.insert( std::pair< wxString, S3D_CACHE_ENTRY* >
            ( aFileName, ep ) ).second == false )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [BUG] duplicate entry in map file; key = ";
            std::cerr << aFileName.ToUTF8() << "\n";
            m_CacheList.pop_back();
            delete ep;
        }
        else
        {
            std::cerr << " * [3D Model] [0] added cached name '" << aFileName.ToUTF8() << "'\n";

            if( aCachePtr )
                *aCachePtr = ep;

        }

        return NULL;
    }

    S3D_CACHE_ENTRY* ep = new S3D_CACHE_ENTRY;
    m_CacheList.push_back( ep );

    if( m_CacheMap.insert( std::pair< wxString, S3D_CACHE_ENTRY* >
                               ( aFileName, ep ) ).second == false )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] duplicate entry in map file; key = ";
        std::cerr << aFileName.ToUTF8() << "\n";
        m_CacheList.pop_back();
        delete ep;
        return NULL;
    }
    else
    {
        std::cerr << " * [3D Model] [1] added cached name '" << aFileName.ToUTF8() << "'\n";
    }

    if( aCachePtr )
        *aCachePtr = ep;

    ep->SetMD5( md5sum );

    wxString bname = ep->GetCacheBaseName();
    wxString cachename = m_CacheDir + bname + wxT( ".3dc" );

    if( wxFileName::FileExists( cachename ) )
    {
        loadCacheData( ep );
        return ep->sceneData;
    }

    ep->sceneData = m_Plugins->Load3DModel( aFileName );

    if( NULL != ep->sceneData )
        saveCacheData( ep );

    return ep->sceneData;
}


bool S3D_CACHE::getMD5( const wxString& aFileName, unsigned char* aMD5Sum )
{
    if( aFileName.empty() )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] empty filename\n";
        return false;
    }

    if( NULL == aMD5Sum )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] NULL pointer passed for aMD5Sum\n";
        return false;
    }

    FILE* fp = fopen( aFileName.ToUTF8(), "rb" );

    if( !fp )
    {
        std::cerr << " * [3dmodel] could not open file '" << aFileName.ToUTF8() << "'\n";
        return false;
    }

    struct md5_ctx msum;
    md5_init_ctx( &msum );
    int res = md5_stream( fp, aMD5Sum );
    fclose( fp );

    if( 0 != res )
    {
        std::cerr << " * [3dmodel] md5 calculation failed on file '" << aFileName.ToUTF8() << "'\n";
        return false;
    }

    return true;
}


bool S3D_CACHE::loadCacheData( S3D_CACHE_ENTRY* aCacheItem )
{
    wxString bname = aCacheItem->GetCacheBaseName();

    if( bname.empty() )
    {
        std::cerr << " * [3D model] cannot load cached model; no md5 hash available\n";
        return false;
    }

    if( m_CacheDir.empty() )
    {
        std::cerr << " * [3D model] cannot load cached model; config directory unknown\n";
        return false;
    }

    wxString fname = m_CacheDir + bname + wxT( ".3dc" );

    if( !wxFileName::FileExists( fname ) )
    {
        std::cerr << " * [3D model] cannot open file '";
        std::cerr << fname.ToUTF8() << "'\n";
        return false;
    }

    if( NULL != aCacheItem->sceneData )
        S3D::DestroyNode( (SGNODE*) aCacheItem->sceneData );

    aCacheItem->sceneData = (SCENEGRAPH*)S3D::ReadCache( fname );

    if( NULL == aCacheItem->sceneData )
        return false;

    return true;
}


bool S3D_CACHE::saveCacheData( S3D_CACHE_ENTRY* aCacheItem )
{
    if( NULL == aCacheItem )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * NULL passed for aCacheItem\n";
        return false;
    }

    if( NULL == aCacheItem->sceneData )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * aCacheItem has no valid scene data\n";
        return false;
    }

    wxString bname = aCacheItem->GetCacheBaseName();

    if( bname.empty() )
    {
        std::cerr << " * [3D model] cannot load cached model; no md5 hash available\n";
        return false;
    }

    if( m_CacheDir.empty() )
    {
        std::cerr << " * [3D model] cannot load cached model; config directory unknown\n";
        return false;
    }

    wxString fname = m_CacheDir + bname + wxT( ".3dc" );

    if( wxFileName::Exists( fname ) )
    {
        // determine if the file is a regular file
        struct stat info;

        if( stat( fname.ToUTF8(), &info ) )
            return false;

        if( !S_ISREG( info.st_mode ) )
        {
            std::cerr << " * [3D model] path exists but is not a regular file: '";
            std::cerr << fname.ToUTF8() << "'\n";
            return false;
        }

        // the file already exists on disk; just exit
        return true;
    }

    return S3D::WriteCache( fname, true, (SGNODE*)aCacheItem->sceneData );
}


bool S3D_CACHE::Set3DConfigDir( const wxString& aConfigDir )
{
    if( !m_ConfigDir.empty() )
        return false;

    wxFileName cfgdir( aConfigDir, wxT( "" ) );
    cfgdir.Normalize();

    if( !cfgdir.DirExists() )
    {
        cfgdir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

        if( !cfgdir.DirExists() )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * failed to create 3D configuration directory\n";
            std::cerr << " * config directory: '";
            std::cerr << cfgdir.GetPath().ToUTF8() << "'\n";
            return false;
        }
    }

    m_ConfigDir = cfgdir.GetPath();

    // inform the file resolver of the config directory
    if( !m_FNResolver->Set3DConfigDir( m_ConfigDir ) )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * could not set 3D Config Directory on filename resolver\n";
        std::cerr << " * config directory: '" << m_ConfigDir.ToUTF8() << "'\n";
    }

    cfgdir.AppendDir( wxT( "cache" ) );

    if( !cfgdir.DirExists() )
    {
        cfgdir.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL );

        if( !cfgdir.DirExists() )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * failed to create 3D cache directory\n";
            std::cerr << " * cache directory: '";
            std::cerr << cfgdir.GetPath().ToUTF8() << "'\n";
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
    wxString envstr;

    if( !wxGetEnv( wxT( "XDG_CONFIG_HOME" ), &envstr ) || envstr.IsEmpty() )
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
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * failed to create 3D configuration directory\n";
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


void S3D_CACHE::FlushCache( void )
{
    std::list< S3D_CACHE_ENTRY* >::iterator sCL = m_CacheList.begin();
    std::list< S3D_CACHE_ENTRY* >::iterator eCL = m_CacheList.end();

    while( sCL != eCL )
    {
        delete *sCL;
        ++sCL;
    }

    m_CacheList.clear();
    ClosePlugins();

    return;
}


void S3D_CACHE::ClosePlugins( void )
{
    if( NULL != m_Plugins )
        m_Plugins->ClosePlugins();

    return;
}


S3DMODEL* S3D_CACHE::Prepare( const wxString& aModelFileName )
{
    S3D_CACHE_ENTRY* cp = NULL;
    SCENEGRAPH* sp = load( aModelFileName, &cp );

    if( !sp )
        return NULL;

    if( !cp )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * [BUG] model loaded with no associated S3D_CACHE_ENTRY\n";
        return NULL;
    }

    if( cp->renderData )
        return cp->renderData;

    S3DMODEL* mp = S3D::Prepare( sp );
    cp->renderData = mp;

    return mp;
}

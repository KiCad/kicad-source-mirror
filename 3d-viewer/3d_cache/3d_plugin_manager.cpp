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


#include <utility>
#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <string>

#include <wx/string.h>
#include <wx/dir.h>
#include <wx/config.h>
#include <wx/stdpaths.h>
#include <wx/filename.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <dlfcn.h>
    #include <pwd.h>
#endif

#include <3d_plugin_manager.h>
#include <plugins/3d/3d_plugin.h>
#include <3d_cache/sg/scenegraph.h>

class S3D_PLUGIN_ITEM
{
private:
#ifdef _WIN32
    HMODULE m_dlHandle;
#else
    void* m_dlHandle;       // handle to the opened plugin
#endif

    S3D_PLUGIN* m_plugin;   // pointer to an instance
    wxString m_pluginName;  // plugin name

public:
    S3D_PLUGIN_ITEM( const wxString& aPluginPath );
    ~S3D_PLUGIN_ITEM();
    bool Open( void );
    void Close( void );
    S3D_PLUGIN* GetPlugin( void );
    const wxString GetPluginName( void );
};


S3D_PLUGIN_ITEM::S3D_PLUGIN_ITEM( const wxString& aPluginPath )
{
    m_pluginName = aPluginPath;
    m_dlHandle = NULL;
    m_plugin = NULL;

    return;
}

S3D_PLUGIN_ITEM::~S3D_PLUGIN_ITEM()
{
    Close();
}

bool S3D_PLUGIN_ITEM::Open( void )
{
    if( NULL != m_dlHandle )
        return true;

    if( m_pluginName.IsEmpty() )
        return false;

    m_plugin = NULL;

#ifdef _WIN32
    // NOTE: MSWin uses UTF-16 encoding
    #if defined( UNICODE ) || defined( _UNICODE )
        m_dlHandle = LoadLibrary( m_pluginName.wc_str() );
    #else
        m_dlHandle = LoadLibrary( m_pluginName.ToUTF8() );
    #endif
#else
    m_dlHandle = dlopen( m_pluginName.ToUTF8(), RTLD_LAZY | RTLD_LOCAL );
#endif

    if( NULL == m_dlHandle )
    {
        std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        std::cerr << " * could not open file: '" << m_pluginName.ToUTF8() << "'\n";
        return false;
    }
    else
    {

#ifdef _WIN32
        typedef S3D_PLUGIN* (*pPLUGIN)( void );
        pPLUGIN Get3DPlugin = (pPLUGIN) GetProcAddress( m_dlHandle, "Get3DPlugin" );
#else
        S3D_PLUGIN* (*Get3DPlugin)( void );
        *(void **) (&Get3DPlugin) = dlsym( m_dlHandle, "Get3DPlugin" );
#endif

        if( NULL == Get3DPlugin )
        {
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";

#ifdef _WIN32
            std::cerr << " * [INFO] could not find symbol\n";
#else
            char* err = dlerror();
            std::cerr << " * [INFO] could not find symbol: '" << err << "'\n";
#endif

        }
        else
        {
            // set the 3D Model Plugin object
            m_plugin = (*Get3DPlugin)();
            return true;
        }
    }

#ifdef _WIN32
    FreeLibrary( m_dlHandle );
#else
    dlclose( m_dlHandle );
#endif

    m_dlHandle = NULL;
    return false;
}

void S3D_PLUGIN_ITEM::Close( void )
{
    m_plugin = NULL;

    if( m_dlHandle )
    {

#ifdef _WIN32
        FreeLibrary( m_dlHandle );
#else
        dlclose( m_dlHandle );
#endif

        m_dlHandle = NULL;
    }

    return;
}


S3D_PLUGIN* S3D_PLUGIN_ITEM::GetPlugin( void )
{
    if( NULL == m_plugin && !Open() )
        return NULL;

    return m_plugin;
}


const wxString S3D_PLUGIN_ITEM::GetPluginName( void )
{
    return m_pluginName;
}


S3D_PLUGIN_MANAGER::S3D_PLUGIN_MANAGER()
{
    // create the initial file filter list entry
    m_FileFilters.push_back( _( "All Files (*.*)|*.*" ) );

    // discover and load plugins
    loadPlugins();

#ifdef DEBUG
    if( !m_ExtMap.empty() )
    {
        std::multimap< const wxString, S3D_PLUGIN_ITEM* >::const_iterator sM = m_ExtMap.begin();
        std::multimap< const wxString, S3D_PLUGIN_ITEM* >::const_iterator eM = m_ExtMap.end();
        std::cout << "* Extension [plugin name]:\n";

        while( sM != eM )
        {
            std::cout << "  + '" << sM->first.ToUTF8() << "' [";
            std::cout << sM->second->GetPluginName().ToUTF8() << "]\n";
            ++sM;
        }

    }
    else
    {
        std::cout << "* No plugins available\n";
    }


    if( !m_FileFilters.empty() )
    {
        /// list of file filters
        std::list< wxString >::const_iterator sFF = m_FileFilters.begin();
        std::list< wxString >::const_iterator eFF = m_FileFilters.end();
        std::cout << "* File filters:\n";
        int i = 0;

        while( sFF != eFF )
        {
            std::cout << " + '" << *sFF << "'\n";
            ++sFF;
        }
    }
    else
    {
        std::cout << "* No file filters available\n";
    }
#endif  // DEBUG

    return;
}


S3D_PLUGIN_MANAGER::~S3D_PLUGIN_MANAGER()
{
    std::list< S3D_PLUGIN_ITEM* >::iterator sP = m_Plugins.begin();
    std::list< S3D_PLUGIN_ITEM* >::iterator eP = m_Plugins.end();

    while( sP != eP )
    {
        (*sP)->Close();
        delete *sP;
        ++sP;
    }

    m_Plugins.clear();
    return;
}


void S3D_PLUGIN_MANAGER::loadPlugins( void )
{
    std::list< std::string > pathlist;
    std::list< wxString > searchpaths;
    std::list< wxString > pluginlist;
    wxFileName fn;

#ifdef DEBUG
    // set up to work from the build directory
    fn.Assign( wxStandardPaths::Get().GetExecutablePath() );
    fn.AppendDir( wxT("..") );
    fn.AppendDir( wxT("plugins") );
    fn.AppendDir( wxT("3d") );

    std::string testpath = std::string( fn.GetPathWithSep().ToUTF8() );
    checkPluginPath( testpath, searchpaths );
#endif
    fn.Assign( wxStandardPaths::Get().GetPluginsDir() );
    fn.AppendDir( wxT( "kicad" ) );
    fn.AppendDir( wxT( "plugins" ) );
    fn.AppendDir( wxT( "3d" ) );
    checkPluginPath( std::string( fn.GetPathWithSep().ToUTF8() ), searchpaths );

    checkPluginPath( wxT( "/usr/lib/kicad/plugins/3d" ), searchpaths );
    checkPluginPath( wxT( "/usr/local/lib/kicad/plugins/3d" ), searchpaths );
    checkPluginPath( wxT( "/opt/kicad/lib/kicad/plugins/3d" ), searchpaths );

#ifdef __APPLE__
    // XXX - we want to use GetOSX... so add support for that somehow in order to avoid hard coding
    // "/Library/Application Support/kicad/plugins/3d"
    checkPluginPath( wxT( "/Library/Application Support/kicad/plugins/3d" ), searchpaths );

    // /Library/Application Support/kicad/plugins
    //fn.Assign( GetOSXKicadMachineDataDir() );
    //fn.AppendDir( wxT( "plugins" ) );
    //fn.AppendDir( wxT( "3d" ) );
    //checkPluginPath( fn.GetPathWithSep(), searchpaths );
#endif

    // note: GetUserDataDir() gives '.pcbnew' rather than '.kicad' since it uses the exe name;
    fn.Assign( wxStandardPaths::Get().GetUserDataDir() );
    fn.AppendDir( wxT( ".kicad" ) );
    fn.AppendDir( wxT( "plugins" ) );
    fn.AppendDir( wxT( "3d" ) );
    checkPluginPath( fn.GetPathWithSep(), searchpaths );

    std::list< wxString >::iterator sPL = searchpaths.begin();
    std::list< wxString >::iterator ePL = searchpaths.end();

    while( sPL != ePL )
    {
#ifdef DEBUG
        std::cout << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ":\n";
        std::cout << "* [DEBUG] searching path: '" << (*sPL).ToUTF8() << "'\n";
#endif
        listPlugins( *sPL, pluginlist );
        ++sPL;
    }

    if( pluginlist.empty() )
        return;

    sPL = pluginlist.begin();
    ePL = pluginlist.end();

    while( sPL != ePL )
    {
        S3D_PLUGIN_ITEM* pp = new S3D_PLUGIN_ITEM( *sPL );

        if( pp )
        {
            if( pp->Open() )
            {
#ifdef DEBUG
                std::cout << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ":\n";
                std::cout << "* [DEBUG] adding plugin\n";
#endif
                m_Plugins.push_back( pp );

                S3D_PLUGIN* lpp = pp->GetPlugin();

                if( lpp )
                {
                    int nf = lpp->GetNFilters();

                    for( int i = 0; i < nf; ++i )
                        addFilterString( lpp->GetFileFilter( i ) );

                }

                addExtensionMap( pp );

                // close the loaded library
                pp->Close();
            }
            else
            {
#ifdef DEBUG
                std::cout << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ":\n";
                std::cout << "* [DEBUG] deleting plugin\n";
#endif
                delete pp;
            }
        }

        ++sPL;
    }

#ifdef DEBUG
    std::cout << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ":\n";
    std::cout << "* [DEBUG] plugins loaded\n";
#endif

    return;
}


void S3D_PLUGIN_MANAGER::listPlugins( const wxString& aPath,
    std::list< wxString >& aPluginList )
{
    // list potential plugins given a search paths
    // note on typical plugin names:
    // Linux: *.so, *.so.* (note: *.so.* will not be supported)
    // MSWin: *.dll
    // OSX: *.dylib, *.bundle

    std::list< wxString > nameFilter;   // filter to apply to files
    wxString lName;                     // stores name of enumerated files
    wxString fName;                     // full name of file

#ifdef __linux

    nameFilter.push_back( wxString::FromUTF8Unchecked( "*.so" ) );

#elif defined _WIN32

    nameFilter.push_back( wxString::FromUTF8Unchecked( "*.dll" ) );

#elif defined __APPLE__

    nameFilter.push_back( wxString::FromUTF8Unchecked( "*.dylib" ) );
    nameFilter.push_back( wxString::FromUTF8Unchecked( "*.bundle" ) );

#else

    // note: we need to positively identify a supported OS here
    // and add suffixes which may be used for 3D model plugins
    // on the specific OS
    #warning NOT IMPLEMENTED

#endif

    wxDir wd;
    wd.Open( aPath );

    if( !wd.IsOpened() )
        return;

    wxString lp = wd.GetNameWithSep();
    std::list< wxString >::iterator sExt = nameFilter.begin();
    std::list< wxString >::iterator eExt = nameFilter.end();

    while( sExt != eExt )
    {
        if( wd.GetFirst( &lName, *sExt, wxDIR_FILES ) )
        {
            fName = lp + lName;
            checkPluginName( fName, aPluginList );

            while( wd.GetNext( &lName ) )
            {
                fName = lp + lName;
                checkPluginName( fName, aPluginList );
            }
        }

        ++sExt;
    }

    wd.Close();

    return;
}


void S3D_PLUGIN_MANAGER::checkPluginName( const wxString& aPath,
    std::list< wxString >& aPluginList )
{
    // check the existence of a plugin name and add it to the list

    if( aPath.empty() || !wxFileName::FileExists( aPath ) )
        return;

    wxFileName path( aPath );
    path.Normalize();

    // determine if the path is already in the list
    wxString wxpath = path.GetFullPath();
    std::list< wxString >::iterator bl = aPluginList.begin();
    std::list< wxString >::iterator el = aPluginList.end();

    while( bl != el )
    {
        if( 0 == (*bl).Cmp( wxpath ) )
            return;

        ++bl;
    }

    aPluginList.push_back( wxpath );

    return;
}


void S3D_PLUGIN_MANAGER::checkPluginPath( const wxString& aPath,
    std::list< wxString >& aSearchList )
{
    // check the existence of a path and add it to the path search list
    if( aPath.empty() )
        return;

    wxFileName path( wxFileName::DirName( aPath ) );
    path.Normalize();

    if( !wxFileName::DirExists( path.GetFullPath() ) )
        return;

    // determine if the directory is already in the list
    wxString wxpath = path.GetFullPath();
    std::list< wxString >::iterator bl = aSearchList.begin();
    std::list< wxString >::iterator el = aSearchList.end();

    while( bl != el )
    {
        if( 0 == (*bl).Cmp( wxpath ) )
            return;

        ++bl;
    }

    aSearchList.push_back( wxpath );

    return;
}


void S3D_PLUGIN_MANAGER::addFilterString( const wxString& aFilterString )
{
    // add an entry to the file filter list
    if( aFilterString.empty() )
        return;

    std::list< wxString >::iterator sFF = m_FileFilters.begin();
    std::list< wxString >::iterator eFF = m_FileFilters.end();

    while( sFF != eFF )
    {
        if( 0 == (*sFF).Cmp( aFilterString ) )
            return;

        ++sFF;
    }

    m_FileFilters.push_back( aFilterString );
    return;
}


void S3D_PLUGIN_MANAGER::addExtensionMap( S3D_PLUGIN_ITEM* aPlugin )
{
    // add entries to the extension map
    if( NULL == aPlugin )
        return;

    S3D_PLUGIN* pp = aPlugin->GetPlugin();

    if( NULL == pp )
        return;

    int nExt = pp->GetNExtensions();

    for( int i = 0; i < nExt; ++i )
    {
        wxString ws = pp->GetModelExtension( i );

        if( !ws.empty() )
        {
            m_ExtMap.insert( std::pair< const wxString, S3D_PLUGIN_ITEM* >( ws, aPlugin ) );
        }

    }

    return;
}


std::list< wxString > const* S3D_PLUGIN_MANAGER::GetFileFilters( void ) const
{
    return &m_FileFilters;
}


SCENEGRAPH* S3D_PLUGIN_MANAGER::Load3DModel( const wxString& aFileName )
{
    wxFileName raw( aFileName );
    wxString ext = raw.GetExt();

    std::pair < std::multimap< const wxString, S3D_PLUGIN_ITEM* >::iterator,
        std::multimap< const wxString, S3D_PLUGIN_ITEM* >::iterator > items;

    items = m_ExtMap.equal_range( ext );
    std::multimap< const wxString, S3D_PLUGIN_ITEM* >::iterator sL = items.first;

    while( sL != items.second )
    {
        S3D_PLUGIN* pplug = sL->second->GetPlugin();

        if( NULL != pplug && pplug->CanRender() )
        {
            SCENEGRAPH* sp = pplug->Load( aFileName );

            if( NULL != sp )
                return sp;
        }

        ++sL;
    }

    return NULL;
}


void S3D_PLUGIN_MANAGER::ClosePlugins( void )
{
    std::list< S3D_PLUGIN_ITEM* >::iterator sP = m_Plugins.begin();
    std::list< S3D_PLUGIN_ITEM* >::iterator eP = m_Plugins.end();

    while( sP != eP )
    {
        (*sP)->Close();
        ++sP;
    }

    return;
}

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

#include "3d_plugin_manager.h"
#include "plugins/3d/3d_plugin.h"
#include "3d_cache/sg/scenegraph.h"
#include "plugins/ldr/3d/pluginldr3D.h"

S3D_PLUGIN_MANAGER::S3D_PLUGIN_MANAGER()
{
    // create the initial file filter list entry
    m_FileFilters.push_back( _( "All Files (*.*)|*.*" ) );

    // discover and load plugins
    loadPlugins();

#ifdef DEBUG
    if( !m_ExtMap.empty() )
    {
        std::multimap< const wxString, KICAD_PLUGIN_LDR_3D* >::const_iterator sM = m_ExtMap.begin();
        std::multimap< const wxString, KICAD_PLUGIN_LDR_3D* >::const_iterator eM = m_ExtMap.end();
        std::cout << "* Extension [plugin name]:\n";

        while( sM != eM )
        {
            std::cout << "  + '" << sM->first.ToUTF8() << "' [";
            std::cout << sM->second->GetKicadPluginName() << "]\n";
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
    std::list< KICAD_PLUGIN_LDR_3D* >::iterator sP = m_Plugins.begin();
    std::list< KICAD_PLUGIN_LDR_3D* >::iterator eP = m_Plugins.end();

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

    #ifndef _WIN32  // suppress 'kicad' subdir since it is redundant on MSWin
        fn.Assign( wxStandardPaths::Get().GetPluginsDir() );
        fn.AppendDir( wxT( "kicad" ) );
    #else
        fn.Assign( wxStandardPaths::Get().GetExecutablePath() );
    #endif

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
        KICAD_PLUGIN_LDR_3D* pp = new KICAD_PLUGIN_LDR_3D;

        if( pp->Open( sPL->ToUTF8() ) )
        {
#ifdef DEBUG
            std::cout << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ":\n";
            std::cout << "* [DEBUG] adding plugin\n";
#endif
            m_Plugins.push_back( pp );
            int nf = pp->GetNFilters();

            #ifdef DEBUG
            std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
            std::cerr << " * [INFO] adding " << nf << " filters\n";
            #endif

            for( int i = 0; i < nf; ++i )
            {
                char const* cp = pp->GetFileFilter( i );

                if( cp )
                    addFilterString( wxString::FromUTF8Unchecked( cp ) );
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

    #ifdef DEBUG
    std::cerr << " * [INFO] found 3D plugin '" << wxpath.ToUTF8() << "'\n";
    #endif

    return;
}


void S3D_PLUGIN_MANAGER::checkPluginPath( const wxString& aPath,
    std::list< wxString >& aSearchList )
{
    // check the existence of a path and add it to the path search list
    if( aPath.empty() )
        return;

    #ifdef DEBUG
    std::cerr << " * [INFO] checking for 3D plugins in '" << aPath << "'\n";
    #endif

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


void S3D_PLUGIN_MANAGER::addExtensionMap( KICAD_PLUGIN_LDR_3D* aPlugin )
{
    // add entries to the extension map
    if( NULL == aPlugin )
        return;

    int nExt = aPlugin->GetNExtensions();

    #ifdef DEBUG
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [INFO] adding " << nExt << " extensions\n";
    #endif

    for( int i = 0; i < nExt; ++i )
    {
        char const* cp = aPlugin->GetModelExtension( i );
        wxString ws;

        if( cp )
            ws = wxString::FromUTF8Unchecked( cp );

        if( !ws.empty() )
        {
            m_ExtMap.insert( std::pair< const wxString, KICAD_PLUGIN_LDR_3D* >( ws, aPlugin ) );
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

    std::pair < std::multimap< const wxString, KICAD_PLUGIN_LDR_3D* >::iterator,
        std::multimap< const wxString, KICAD_PLUGIN_LDR_3D* >::iterator > items;

    items = m_ExtMap.equal_range( ext );
    std::multimap< const wxString, KICAD_PLUGIN_LDR_3D* >::iterator sL = items.first;

    while( sL != items.second )
    {
        if( sL->second->CanRender() )
        {
            SCENEGRAPH* sp = sL->second->Load( aFileName );

            if( NULL != sp )
                return sp;
        }

        ++sL;
    }

    return NULL;
}


void S3D_PLUGIN_MANAGER::ClosePlugins( void )
{
    std::list< KICAD_PLUGIN_LDR_3D* >::iterator sP = m_Plugins.begin();
    std::list< KICAD_PLUGIN_LDR_3D* >::iterator eP = m_Plugins.end();

    #ifdef DEBUG
    std::cerr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
    std::cerr << " * [INFO] closing " << m_Plugins.size() << " plugins\n";
    #endif

    while( sP != eP )
    {
        (*sP)->Close();
        ++sP;
    }

    return;
}

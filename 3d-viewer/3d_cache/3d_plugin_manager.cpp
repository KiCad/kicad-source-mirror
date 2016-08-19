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


#include <utility>
#include <iostream>
#include <sstream>

#include <wx/config.h>
#include <wx/dir.h>
#include <wx/dynlib.h>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/stdpaths.h>
#include <wx/string.h>

#include "common.h"
#include "pgm_base.h"
#include "3d_plugin_manager.h"
#include "plugins/3d/3d_plugin.h"
#include "3d_cache/sg/scenegraph.h"
#include "plugins/ldr/3d/pluginldr3D.h"

#define MASK_3D_PLUGINMGR "3D_PLUGIN_MANAGER"


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
        wxLogTrace( MASK_3D_PLUGINMGR, " * Extension [plugin name]:\n" );

        while( sM != eM )
        {
            wxLogTrace( MASK_3D_PLUGINMGR, "   + '%s' [%s]\n", sM->first.GetData(),
                  sM->second->GetKicadPluginName() );
            ++sM;
        }

    }
    else
    {
        wxLogTrace( MASK_3D_PLUGINMGR, " * No plugins available\n" );
    }


    if( !m_FileFilters.empty() )
    {
        /// list of file filters
        std::list< wxString >::const_iterator sFF = m_FileFilters.begin();
        std::list< wxString >::const_iterator eFF = m_FileFilters.end();
        wxLogTrace( MASK_3D_PLUGINMGR, " * File filters:\n" );

        while( sFF != eFF )
        {
            wxLogTrace( MASK_3D_PLUGINMGR, " + '%s'\n", (*sFF).GetData() );
            ++sFF;
        }
    }
    else
    {
        wxLogTrace( MASK_3D_PLUGINMGR, " * No file filters available\n" );
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
    std::list< wxString > searchpaths;
    std::list< wxString > pluginlist;
    wxFileName fn;

#ifndef __WXMAC__

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
    fn.Assign( wxStandardPaths::Get().GetPluginsDir(), "" );
    fn.RemoveLastDir();
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

    // note: GetUserDataDir() gives '.pcbnew' rather than '.kicad' since it uses the exe name;
    fn.Assign( wxStandardPaths::Get().GetUserDataDir(), "" );
    fn.RemoveLastDir();
    #ifdef _WIN32
    fn.AppendDir( wxT( "kicad" ) );
    #else
    fn.AppendDir( wxT( ".kicad" ) );
    #endif
    fn.AppendDir( wxT( "plugins" ) );
    fn.AppendDir( wxT( "3d" ) );
    checkPluginPath( fn.GetPathWithSep(), searchpaths );

#else

   // Search path on OS X is
   // (1) User     ~/Library/Application Support/kicad/PlugIns/3d
   checkPluginPath( GetOSXKicadUserDataDir() + wxT( "/PlugIns/3d" ), searchpaths );
   // (2) Machine  /Library/Application Support/kicad/PlugIns/3d
   checkPluginPath( GetOSXKicadMachineDataDir() + wxT( "/PlugIns/3d" ), searchpaths );
   // (3) Bundle   kicad.app/Contents/PlugIns/3d
   fn.Assign( Pgm().GetExecutablePath() );
   fn.AppendDir( wxT( "Contents" ) );
   fn.AppendDir( wxT( "PlugIns" ) );
   fn.AppendDir( wxT( "3d" ) );
   checkPluginPath( fn.GetPathWithSep(), searchpaths );

#endif

    std::list< wxString >::iterator sPL = searchpaths.begin();
    std::list< wxString >::iterator ePL = searchpaths.end();

    while( sPL != ePL )
    {
#ifdef DEBUG
        do {
            std::ostringstream ostr;
            ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ":\n";
            ostr << " * [DEBUG] searching path: '" << (*sPL).ToUTF8() << "'";
            wxLogTrace( MASK_3D_PLUGINMGR, "%s\n", ostr.str().c_str() );
        } while( 0 );
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
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ":\n";
                ostr << "* [DEBUG] adding plugin";
                wxLogTrace( MASK_3D_PLUGINMGR, "%s\n", ostr.str().c_str() );
            } while( 0 );
#endif
            m_Plugins.push_back( pp );
            int nf = pp->GetNFilters();

            #ifdef DEBUG
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
                ostr << " * [INFO] adding " << nf << " filters";
                wxLogTrace( MASK_3D_PLUGINMGR, "%s\n", ostr.str().c_str() );
            } while( 0 );
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
            do {
                std::ostringstream ostr;
                ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ":\n";
                ostr << "* [DEBUG] deleting plugin";
                wxLogTrace( MASK_3D_PLUGINMGR, "%s\n", ostr.str().c_str() );
            } while( 0 );
#endif
            delete pp;
        }

        ++sPL;
    }

#ifdef DEBUG
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ":" << __FUNCTION__ << ":" << __LINE__ << ":\n";
        ostr << "* [DEBUG] plugins loaded";
        wxLogTrace( MASK_3D_PLUGINMGR, "%s\n", ostr.str().c_str() );
    } while( 0 );
#endif

    return;
}


void S3D_PLUGIN_MANAGER::listPlugins( const wxString& aPath,
    std::list< wxString >& aPluginList )
{
    // list potential plugins given a search path

    wxString nameFilter;                // filter for user-loadable libraries (aka modules)
    wxString lName;                     // stores name of enumerated files
    wxString fName;                     // full name of file
    wxDir wd;
    wd.Open( aPath );

    if( !wd.IsOpened() )
        return;

    nameFilter = wxT( "*" );

#ifndef __WXMAC__
    nameFilter.Append( wxDynamicLibrary::GetDllExt( wxDL_MODULE ) );
#else
    // wxDynamicLibrary::GetDllExt( wxDL_MODULE ) will return ".bundle" on OS X.
    // This might be correct, but cmake builds a ".so" for a library MODULE.
    // Per definition a loadable "xxx.bundle" is similar to an "xxx.app" app
    // bundle being a folder with some special content in it. We obviously don't
    // want to have that here for our loadable module, so just use ".so".
    nameFilter.Append( ".so" );
#endif

    wxString lp = wd.GetNameWithSep();

    if( wd.GetFirst( &lName, nameFilter, wxDIR_FILES ) )
    {
        fName = lp + lName;
        checkPluginName( fName, aPluginList );

        while( wd.GetNext( &lName ) )
        {
            fName = lp + lName;
            checkPluginName( fName, aPluginList );
        }
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

    wxFileName path;

    if( aPath.StartsWith( "${" ) || aPath.StartsWith( "$(" ) )
        path.Assign( ExpandEnvVarSubstitutions( aPath ) );
    else
        path.Assign( aPath );

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
    wxLogTrace( MASK_3D_PLUGINMGR, " * [INFO] found 3D plugin '%s'\n",
        wxpath.GetData() );
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
    wxLogTrace( MASK_3D_PLUGINMGR, " * [INFO] checking for 3D plugins in '%s'\n",
        aPath.GetData() );
    #endif

    wxFileName path;

    if( aPath.StartsWith( "${" ) || aPath.StartsWith( "$(" ) )
        path.Assign( ExpandEnvVarSubstitutions( aPath ), "" );
    else
        path.Assign( aPath, "" );

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
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [INFO] adding " << nExt << " extensions";
        wxLogTrace( MASK_3D_PLUGINMGR, "%s\n", ostr.str().c_str() );
    } while( 0 );
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


SCENEGRAPH* S3D_PLUGIN_MANAGER::Load3DModel( const wxString& aFileName, std::string& aPluginInfo )
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
            SCENEGRAPH* sp = sL->second->Load( aFileName.ToUTF8() );

            if( NULL != sp )
            {
                sL->second->GetPluginInfo( aPluginInfo );
                return sp;
            }
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
    do {
        std::ostringstream ostr;
        ostr << __FILE__ << ": " << __FUNCTION__ << ": " << __LINE__ << "\n";
        ostr << " * [INFO] closing " << m_Plugins.size() << " plugins";
        wxLogTrace( MASK_3D_PLUGINMGR, "%s\n", ostr.str().c_str() );
    } while( 0 );
    #endif

    while( sP != eP )
    {
        (*sP)->Close();
        ++sP;
    }

    return;
}


bool S3D_PLUGIN_MANAGER::CheckTag( const char* aTag )
{
    if( NULL == aTag || aTag[0] == 0 || m_Plugins.empty() )
        return false;

    std::string tname = aTag;
    std::string pname;      // plugin name

    size_t cpos = tname.find( ':' );

    // if there is no colon or plugin name then the tag is bad
    if( cpos == std::string::npos || cpos == 0 )
        return false;

    pname = tname.substr( 0, cpos );
    std::string ptag;   // tag from the plugin

    std::list< KICAD_PLUGIN_LDR_3D* >::iterator pS = m_Plugins.begin();
    std::list< KICAD_PLUGIN_LDR_3D* >::iterator pE = m_Plugins.end();

    while( pS != pE )
    {
        ptag.clear();
        (*pS)->GetPluginInfo( ptag );

        // if the plugin name matches then the version
        // must also match
        if( !ptag.compare( 0, pname.size(), pname ) )
        {
            if( ptag.compare( tname ) )
                return false;

            return true;
        }

        ++pS;
    }

    return true;
}

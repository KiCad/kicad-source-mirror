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

#include <iostream>
#include <sstream>
#include <wx/dynload.h>
#include <wx/log.h>
#include <wx/string.h>
#include <wx/translation.h>

#include "pluginldr.h"


/**
 * Flag to enable plugin loader trace output.
 *
 * @ingroup trace_env_vars
 */
const wxChar* const tracePluginLoader = wxT( "KICAD_PLUGIN_LOADER" );


KICAD_PLUGIN_LDR::KICAD_PLUGIN_LDR()
{
    ok = false;
    m_getPluginClass = nullptr;
    m_getClassVersion = nullptr;
    m_checkClassVersion = nullptr;
    m_getPluginName = nullptr;
    m_getVersion = nullptr;

    return;
}


KICAD_PLUGIN_LDR::~KICAD_PLUGIN_LDR()
{
    close();
    return;
}


bool KICAD_PLUGIN_LDR::open( const wxString& aFullFileName, const char* aPluginClass )
{
    m_error.clear();

    if( ok )
        Close();

    if( aFullFileName.empty() )
        return false;

    m_fileName.clear();

    m_PluginLoader.Load( aFullFileName, wxDL_LAZY );

    if( !m_PluginLoader.IsLoaded() )
    {
        wxLogTrace( tracePluginLoader, wxT( " * could not open file: '%s'" ),
                    aFullFileName.ToUTF8() );

        return false;
    }

    LINK_ITEM( m_getPluginClass, GET_PLUGIN_CLASS, "GetKicadPluginClass" );
    LINK_ITEM( m_getClassVersion, GET_CLASS_VERSION, "GetClassVersion" );
    LINK_ITEM( m_checkClassVersion, CHECK_CLASS_VERSION , "CheckClassVersion" );
    LINK_ITEM( m_getPluginName, GET_PLUGIN_NAME, "GetKicadPluginName" );
    LINK_ITEM( m_getVersion, GET_VERSION, "GetPluginVersion" );

#ifdef DEBUG
    bool fail = false;

    if( !m_getPluginClass )
    {
        wxLogTrace( tracePluginLoader,
                    wxT( "%s:%s:%d\n"
                         "incompatible plugin (missing function 'GetKicadPluginClass')" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        fail = true;
    }

    if( !m_getClassVersion )
    {
        if( !fail )
        {
            wxLogTrace( tracePluginLoader,
                        wxT( "%s:%s:%d\n"
                             "incompatible plugin (missing function 'GetClassVersion')" ),
                        __FILE__, __FUNCTION__, __LINE__ );
            fail = true;
        }
        else
        {
            wxLogTrace( tracePluginLoader, wxT( "missing function 'GetClassVersion'" ) );
        }
    }

    if( !m_checkClassVersion )
    {
        if( !fail )
        {
            wxLogTrace( tracePluginLoader,
                        wxT( "%s:%s:%d\n"
                             "incompatible plugin (missing function 'CheckClassVersion')" ),
                        __FILE__, __FUNCTION__, __LINE__ );

            fail = true;
        }
        else
        {
            wxLogTrace( tracePluginLoader, wxT( "missing function 'CheckClassVersion'" ) );
        }
    }

    if( !m_getPluginName )
    {
        if( !fail )
        {
            wxLogTrace( tracePluginLoader,
                        wxT( "%s:%s:%d\n"
                             "incompatible plugin (missing function 'GetKicadPluginName')" ),
                        __FILE__, __FUNCTION__, __LINE__ );

            fail = true;
        }
        else
        {
            wxLogTrace( tracePluginLoader, wxT( "missing function 'GetKicadPluginName'" ) );
        }
    }

    if( !m_getVersion )
    {
        if( !fail )
        {
            wxLogTrace( tracePluginLoader,
                        wxT( "%s:%s:%d\n"
                             "incompatible plugin (missing function 'GetVersion')" ),
                        __FILE__, __FUNCTION__, __LINE__ );
        }
        else
        {
            wxLogTrace( tracePluginLoader, wxT( "missing function 'GetVersion'" ) );
        }
    }

#endif

    if( !m_getPluginClass || !m_getClassVersion || !m_checkClassVersion
        || !m_getPluginName || !m_getVersion )
    {
        m_error = "incompatible plugin interface (missing functions)";
        close();
        return false;
    }

    // note: since 'ok' is not yet set at this point we must use the function
    // pointers directly rather than invoking the functions exposed by this class

    // check that the Plugin Class matches
    char const* pclassName = m_getPluginClass();

    if( !pclassName || strcmp( aPluginClass, pclassName ) )
    {
        m_error = "Loader type (";
        m_error.append( aPluginClass );
        m_error.append( ") does not match Plugin type (" );

        if( pclassName )
            m_error.append( pclassName );
        else
            m_error.append( "nullptr" );

        m_error.append( ")" );

        close();
        return false;
    }

    // perform a universally enforced version check (major number must match)
    unsigned char lMajor;
    unsigned char lMinor;
    unsigned char lPatch;
    unsigned char lRevno;
    unsigned char pMajor;
    unsigned char pMinor;
    unsigned char pPatch;
    unsigned char pRevno;

    m_getClassVersion( &pMajor, &pMinor, &pPatch, &pRevno );
    GetLoaderVersion( &lMajor, &lMinor, &lPatch, &lRevno );

    // major version changes by definition are incompatible and that is enforced here.
    if( pMajor != lMajor )
    {
        std::ostringstream ostr;
        ostr << "Loader Major version (" << lMajor;
        ostr << ") does not match Plugin Major version (" << pMajor << ")";

        m_error = ostr.str();
        close();
        return false;
    }

    if( !m_checkClassVersion( lMajor, lMinor, lPatch, lRevno ) )
    {
        std::ostringstream ostr;
        ostr << "Plugin Version (" << pMajor << "." << pMinor << "." << pPatch << "." << pRevno;
        ostr << ") does not support Loader Version (" << pMajor << "." << pMinor;
        ostr << "." << pPatch << "." << pRevno << ")";

        m_error = ostr.str();
        close();
        return false;
    }

    m_fileName = aFullFileName;

    wxLogTrace( tracePluginLoader,
                wxT( "%s:%s:%d\n"
                     " * [INFO] opened plugin '%s'" ),
                __FILE__, __FUNCTION__, __LINE__, m_fileName );

    ok = true;

    // set the plugin info string
    m_pluginInfo = GetKicadPluginName();
    std::ostringstream ostr;
    unsigned char r0, r1, r2, r3;
    GetVersion( &r0, &r1, &r2, &r3 );
    ostr << ":" << (unsigned int)r0 << "." << (unsigned int)r1;
    ostr << "." << (unsigned int)r2 << "." << (unsigned int)r3;
    m_pluginInfo.append( ostr.str() );

    return true;
}


void KICAD_PLUGIN_LDR::close( void )
{
    ok = false;
    m_getPluginClass = nullptr;
    m_getClassVersion = nullptr;
    m_checkClassVersion = nullptr;
    m_getPluginName = nullptr;
    m_getVersion = nullptr;
    m_PluginLoader.Unload();

    return;
}


bool KICAD_PLUGIN_LDR::reopen( void )
{
    m_error.clear();

    if( m_fileName.empty() )
        return false;

    wxString fname = m_fileName;

    return Open( fname );
}


std::string KICAD_PLUGIN_LDR::GetLastError( void ) const
{
    return m_error;
}


char const* KICAD_PLUGIN_LDR::GetKicadPluginClass( void )
{
    m_error.clear();

    if( !ok && !reopen() )
    {
        if( m_error.empty() )
            m_error = "[INFO] no open plugin / plugin could not be opened";

        return nullptr;
    }

    if( nullptr == m_getPluginClass )
    {
        m_error = "[BUG] GetPluginClass is not linked";

        wxLogTrace( tracePluginLoader, wxT( "%s:%s:%d\n"
                                            "%s" ),
                    __FILE__, __FUNCTION__, __LINE__, m_error );;

        return nullptr;
    }

    return m_getPluginClass();
}


bool KICAD_PLUGIN_LDR::GetClassVersion( unsigned char* Major, unsigned char* Minor,
                                        unsigned char* Patch, unsigned char* Revision )
{
    m_error.clear();

    if( Major )
        *Major = 0;

    if( Minor )
        *Minor = 0;

    if( Patch )
        *Patch = 0;

    if( Revision )
        *Revision = 0;

    unsigned char major;
    unsigned char minor;
    unsigned char patch;
    unsigned char revno;

    if( !ok && !reopen() )
    {
        if( m_error.empty() )
            m_error = "[INFO] no open plugin / plugin could not be opened";

        return false;
    }

    if( nullptr == m_checkClassVersion )
    {
        m_error = "[BUG] CheckClassVersion is not linked";

        wxLogTrace( tracePluginLoader, wxT( "%s:%s:%d\n"
                                            "%s" ),
                    __FILE__, __FUNCTION__, __LINE__, m_error );;

        return false;
    }

    m_getClassVersion( &major, &minor, &patch, &revno );

    if( Major )
        *Major = major;

    if( Minor )
        *Minor = minor;

    if( Patch )
        *Patch = patch;

    if( Revision )
        *Revision = revno;

    return true;
}


bool KICAD_PLUGIN_LDR::CheckClassVersion( unsigned char Major, unsigned char Minor,
                                          unsigned char Patch, unsigned char Revision )
{
    m_error.clear();

    if( !ok && !reopen() )
    {
        if( m_error.empty() )
            m_error = "[INFO] no open plugin / plugin could not be opened";

        return false;
    }

    if( nullptr == m_checkClassVersion )
    {
        m_error = "[BUG] CheckClassVersion is not linked";

        wxLogTrace( tracePluginLoader, wxT( "%s:%s:%d\n"
                                            "%s" ),
                    __FILE__, __FUNCTION__, __LINE__, m_error );;

        return false;
    }

    return m_checkClassVersion( Major, Minor, Patch, Revision );
}


const char* KICAD_PLUGIN_LDR::GetKicadPluginName( void )
{
    m_error.clear();

    if( !ok && !reopen() )
    {
        if( m_error.empty() )
            m_error = "[INFO] no open plugin / plugin could not be opened";

        return nullptr;
    }

    if( nullptr == m_getPluginName )
    {
        m_error = "[BUG] GetKicadPluginName is not linked";

        wxLogTrace( tracePluginLoader, wxT( "%s:%s:%d\n"
                                            "%s" ),
                    __FILE__, __FUNCTION__, __LINE__, m_error );;

        return nullptr;
    }

    return m_getPluginName();
}


bool KICAD_PLUGIN_LDR::GetVersion( unsigned char* Major, unsigned char* Minor,
                                   unsigned char* Patch, unsigned char* Revision )
{
    m_error.clear();

    if( !ok && !reopen() )
    {
        if( m_error.empty() )
            m_error = "[INFO] no open plugin / plugin could not be opened";

        return false;
    }

    if( nullptr == m_getVersion )
    {
        m_error = "[BUG] GetKicadPluginName is not linked";

        wxLogTrace( tracePluginLoader, wxT( "%s:%s:%d\n"
                                            "%s" ),
                    __FILE__, __FUNCTION__, __LINE__, m_error );;

        return false;
    }

    m_getVersion( Major, Minor, Patch, Revision );

    return true;
}


void KICAD_PLUGIN_LDR::GetPluginInfo( std::string& aPluginInfo )
{
    aPluginInfo = m_pluginInfo;
}

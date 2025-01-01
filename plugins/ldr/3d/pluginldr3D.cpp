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

#include <iostream>
#include <sstream>
#include <wx/log.h>
#include <wx/translation.h>

#include "plugins/ldr/3d/pluginldr3D.h"

#define PLUGIN_CLASS_3D "PLUGIN_3D"
#define PLUGIN_3D_MAJOR 1
#define PLUGIN_3D_MINOR 0
#define PLUGIN_3D_PATCH 0
#define PLUGIN_3D_REVISION 0


KICAD_PLUGIN_LDR_3D::KICAD_PLUGIN_LDR_3D()
{
    ok = false;
    m_getNExtensions = nullptr;
    m_getModelExtension = nullptr;
    m_getNFilters = nullptr;
    m_getFileFilter = nullptr;
    m_canRender = nullptr;
    m_load = nullptr;

    return;
}


KICAD_PLUGIN_LDR_3D::~KICAD_PLUGIN_LDR_3D()
{
    Close();

    return;
}


bool KICAD_PLUGIN_LDR_3D::Open( const wxString& aFullFileName )
{
    m_error.clear();

    if( ok )
        Close();

    if( !open( aFullFileName, PLUGIN_CLASS_3D ) )
    {
        if( m_error.empty() )
        {
            std::ostringstream ostr;
            ostr << "Failed to open plugin '" << aFullFileName.ToUTF8() << "'";
            m_error = ostr.str();
        }

        wxLogTrace( tracePluginLoader, wxT( "%s:%s:%d\n"
                                            " * [INFO] failed on file '%s'\n"
                                            " * [INFO] error: " ),
                    __FILE__, __FUNCTION__, __LINE__, aFullFileName.ToUTF8(), m_error );

        return false;
    }

    // the version checks passed and the base KICAD_PLUGIN functions have been linked;
    // now we link the remaining functions expected by PLUGIN_3D and confirm that the
    // plugin is loaded
    LINK_ITEM( m_getNExtensions, PLUGIN_3D_GET_N_EXTENSIONS, "GetNExtensions" );
    LINK_ITEM( m_getModelExtension, PLUGIN_3D_GET_MODEL_EXTENSION, "GetModelExtension" );
    LINK_ITEM( m_getNFilters, PLUGIN_3D_GET_N_FILTERS, "GetNFilters" );
    LINK_ITEM( m_getFileFilter, PLUGIN_3D_GET_FILE_FILTER, "GetFileFilter" );
    LINK_ITEM( m_canRender, PLUGIN_3D_CAN_RENDER, "CanRender" );
    LINK_ITEM( m_load, PLUGIN_3D_LOAD, "Load" );

#ifdef DEBUG
    bool fail = false;

    if( !m_getNExtensions )
    {
        wxLogTrace( tracePluginLoader,
                    wxT( "%s:%s:%d\n"
                         "incompatible plugin (missing function 'GetNExtensions')" ),
                    __FILE__, __FUNCTION__, __LINE__ );

        fail = true;
    }

    if( !m_getModelExtension )
    {
        if( !fail )
        {
            wxLogTrace( tracePluginLoader,
                        wxT( "%s:%s:%d\n"
                             "incompatible plugin (missing function 'GetModelExtension')" ),
                        __FILE__, __FUNCTION__, __LINE__ );

            fail = true;
        }
        else
        {
            wxLogTrace( tracePluginLoader, wxT( "missing function 'GetModelExtension'" ) );
        }
    }

    if( !m_getNFilters )
    {
        if( !fail )
        {
            wxLogTrace( tracePluginLoader,
                        wxT( "%s:%s:%d\n"
                             "incompatible plugin (missing function 'GetNFilters')" ),
                        __FILE__, __FUNCTION__, __LINE__ );

            fail = true;
        }
        else
        {
            wxLogTrace( tracePluginLoader, wxT( "missing function 'GetNFilters'" ) );
        }
    }

    if( !m_getFileFilter )
    {
        if( !fail )
        {
            wxLogTrace( tracePluginLoader,
                        wxT( "%s:%s:%d\n"
                             "incompatible plugin (missing function 'GetFileFilter')" ),
                        __FILE__, __FUNCTION__, __LINE__ );

            fail = true;
        }
        else
        {
            wxLogTrace( tracePluginLoader, wxT( "missing function 'GetFileFilter'" ) );
        }
    }

    if( !m_canRender )
    {
        if( !fail )
        {
            wxLogTrace( tracePluginLoader,
                        wxT( "%s:%s:%d\n"
                             "incompatible plugin (missing function 'CanRender')" ),
                        __FILE__, __FUNCTION__, __LINE__ );

            fail = true;
        }
        else
        {
            wxLogTrace( tracePluginLoader, wxT( "missing function 'CanRender'" ) );
        }
    }

    if( !m_load )
    {
        if( !fail )
        {
            wxLogTrace( tracePluginLoader,
                        wxT( "%s:%s:%d\n"
                             "incompatible plugin (missing function 'Load')" ),
                        __FILE__, __FUNCTION__, __LINE__ );
        }
        else
        {
            wxLogTrace( tracePluginLoader, wxT( "missing function 'Load'" ) );
        }
    }

#endif

    if( !m_getNExtensions || !m_getModelExtension || !m_getNFilters
      || !m_getFileFilter || !m_canRender || !m_load )
    {
        Close();

        std::ostringstream ostr;
        ostr << "Failed to open plugin '" << aFullFileName.ToUTF8() << "'; missing functions";
        m_error = ostr.str();

        return false;
    }

    ok = true;
    return true;
}


void KICAD_PLUGIN_LDR_3D::Close( void )
{
#ifdef DEBUG
    if( ok )
    {
        wxLogTrace( tracePluginLoader, wxT( "%s:%s:%d\n"
                                            " * [INFO] closing plugin" ),
                    __FILE__, __FUNCTION__, __LINE__ );
    }
#endif

    ok = false;
    m_getNExtensions = nullptr;
    m_getModelExtension = nullptr;
    m_getNFilters = nullptr;
    m_getFileFilter = nullptr;
    m_canRender = nullptr;
    m_load = nullptr;
    close();

    return;
}


void KICAD_PLUGIN_LDR_3D::GetLoaderVersion( unsigned char* Major, unsigned char* Minor,
                                            unsigned char* Patch, unsigned char* Revision ) const
{
    if( Major )
        *Major = PLUGIN_3D_MAJOR;

    if( Minor )
        *Minor = PLUGIN_3D_MINOR;

    if( Patch )
        *Patch = PLUGIN_3D_PATCH;

    if( Revision )
        *Revision = PLUGIN_3D_REVISION;

    return;
}


int KICAD_PLUGIN_LDR_3D::GetNExtensions( void )
{
    m_error.clear();

    if( !ok && !reopen() )
    {
        if( m_error.empty() )
            m_error = "[INFO] no open plugin / plugin could not be opened";

        return 0;
    }

    if( nullptr == m_getNExtensions )
    {
        m_error = "[BUG] GetNExtensions is not linked";

        wxLogTrace( tracePluginLoader, wxT( "%s:%s:%d\n"
                                            "%s" ),
                    __FILE__, __FUNCTION__, __LINE__, m_error );;

        return 0;
    }

    return m_getNExtensions();
}


char const* KICAD_PLUGIN_LDR_3D::GetModelExtension( int aIndex )
{
    m_error.clear();

    if( !ok && !reopen() )
    {
        if( m_error.empty() )
            m_error = "[INFO] no open plugin / plugin could not be opened";

        return nullptr;
    }

    if( nullptr == m_getModelExtension )
    {
        m_error = "[BUG] GetModelExtension is not linked";

        wxLogTrace( tracePluginLoader, wxT( "%s:%s:%d\n"
                                            "%s" ),
                    __FILE__, __FUNCTION__, __LINE__, m_error );;

        return nullptr;
    }

    return m_getModelExtension( aIndex );
}


int KICAD_PLUGIN_LDR_3D::GetNFilters( void )
{
    m_error.clear();

    if( !ok && !reopen() )
    {
        if( m_error.empty() )
            m_error = "[INFO] no open plugin / plugin could not be opened";

        return 0;
    }

    if( nullptr == m_getNFilters )
    {
        m_error = "[BUG] GetNFilters is not linked";

        wxLogTrace( tracePluginLoader, wxT( "%s:%s:%d\n"
                                            "%s" ),
                    __FILE__, __FUNCTION__, __LINE__, m_error );;

        return 0;
    }

    return m_getNFilters();
}


char const* KICAD_PLUGIN_LDR_3D::GetFileFilter( int aIndex )
{
    m_error.clear();

    if( !ok && !reopen() )
    {
        if( m_error.empty() )
            m_error = "[INFO] no open plugin / plugin could not be opened";

        return nullptr;
    }

    if( nullptr == m_getFileFilter )
    {
        m_error = "[BUG] GetFileFilter is not linked";

        wxLogTrace( tracePluginLoader, wxT( "%s:%s:%d\n"
                                            "%s" ),
                    __FILE__, __FUNCTION__, __LINE__, m_error );;

        return nullptr;
    }

    return m_getFileFilter( aIndex );
}


bool KICAD_PLUGIN_LDR_3D::CanRender( void )
{
    m_error.clear();

    if( !ok && !reopen() )
    {
        if( m_error.empty() )
            m_error = "[INFO] no open plugin / plugin could not be opened";

        return false;
    }

    if( nullptr == m_canRender )
    {
        m_error = "[BUG] CanRender is not linked";

        wxLogTrace( tracePluginLoader, wxT( "%s:%s:%d\n"
                                            "%s" ),
                    __FILE__, __FUNCTION__, __LINE__, m_error );;

        return false;
    }

    return m_canRender();
}


SCENEGRAPH* KICAD_PLUGIN_LDR_3D::Load( char const* aFileName )
{
    m_error.clear();

    if( !ok && !reopen() )
    {
        if( m_error.empty() )
            m_error = "[INFO] no open plugin / plugin could not be opened";

        return nullptr;
    }

    if( nullptr == m_load )
    {
        m_error = "[BUG] Load is not linked";

        wxLogTrace( tracePluginLoader, wxT( "%s:%s:%d\n"
                                            "%s" ),
                    __FILE__, __FUNCTION__, __LINE__, m_error );;

        return nullptr;
    }

    return m_load( aFileName );
}

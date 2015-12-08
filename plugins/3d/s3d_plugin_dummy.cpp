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

/*
 * NOTES:
 *
 * 1. Notice that the plugin class is instantiated as a static; this
 * ensures that it created an destroyed.
 */

#include <iostream>
#include <s3d_plugin_dummy.h>


S3D_PLUGIN_DUMMY::S3D_PLUGIN_DUMMY()
{
    m_extensions.push_back( wxString::FromUTF8Unchecked( "wrl" ) );
    m_extensions.push_back( wxString::FromUTF8Unchecked( "x3d" ) );
    m_extensions.push_back( wxString::FromUTF8Unchecked( "idf" ) );
    m_extensions.push_back( wxString::FromUTF8Unchecked( "iges" ) );
    m_extensions.push_back( wxString::FromUTF8Unchecked( "igs" ) );
    m_extensions.push_back( wxString::FromUTF8Unchecked( "stp" ) );
    m_extensions.push_back( wxString::FromUTF8Unchecked( "step" ) );

#ifdef _WIN32
    // assume a case-insensitive file system
    m_filters.push_back( wxT( "VRML 1.0/2.0 (*.wrl)|*.wrl" ) );
    m_filters.push_back( wxT( "X3D (*.x3d)|*.x3d" ) );
    m_filters.push_back( wxT( "IDF 2.0/3.0 (*.idf)|*.idf" ) );
    m_filters.push_back( wxT( "IGESv5.3 (*.igs;*.iges)|*.igs;*.iges" ) );
    m_filters.push_back( wxT( "STEP (*.stp;*.step)|*.stp;*.step" ) );
#else
    // assume the filesystem is case sensitive
    m_extensions.push_back( wxString::FromUTF8Unchecked( "WRL" ) );
    m_extensions.push_back( wxString::FromUTF8Unchecked( "X3D" ) );
    m_extensions.push_back( wxString::FromUTF8Unchecked( "IDF" ) );
    m_extensions.push_back( wxString::FromUTF8Unchecked( "IGES" ) );
    m_extensions.push_back( wxString::FromUTF8Unchecked( "IGS" ) );
    m_extensions.push_back( wxString::FromUTF8Unchecked( "STP" ) );
    m_extensions.push_back( wxString::FromUTF8Unchecked( "STEP" ) );

    m_filters.push_back( wxT( "VRML 1.0/2.0 (*.wrl;*.WRL)|*.wrl;*.WRL" ) );
    m_filters.push_back( wxT( "X3D (*.x3d;*.X3D)|*.x3d;*.X3D" ) );
    m_filters.push_back( wxT( "IDF 2.0/3.0 (*.idf;*.IDF)|*.idf;*.IDF" ) );
    m_filters.push_back( wxT( "IGESv5.3 (*.igs;*.iges;*.IGS;*.IGES)|*.igs;*.iges;*.IGS;*.IGES" ) );
    m_filters.push_back( wxT( "STEP (*.stp;*.step;*.STP;*.STEP)|*.stp;*.step;*.STP;*.STEP" ) );
#endif


    return;
}


S3D_PLUGIN_DUMMY::~S3D_PLUGIN_DUMMY()
{
    return;
}


int S3D_PLUGIN_DUMMY::GetNExtensions( void ) const
{
    return (int) m_extensions.size();
}


const wxString S3D_PLUGIN_DUMMY::GetModelExtension( int aIndex ) const
{
    if( aIndex < 0 || aIndex >= (int) m_extensions.size() )
        return wxString( "" );

    return m_extensions[aIndex];
}


int S3D_PLUGIN_DUMMY::GetNFilters( void ) const
{
    return (int)m_filters.size();
}


const wxString S3D_PLUGIN_DUMMY::GetFileFilter( int aIndex ) const
{
    if( aIndex < 0 || aIndex >= (int)m_filters.size() )
        return wxEmptyString;

    return m_filters[aIndex];
}


bool S3D_PLUGIN_DUMMY::CanRender( void ) const
{
    // this dummy plugin does not support rendering of any models
    return false;
}


SCENEGRAPH* S3D_PLUGIN_DUMMY::Load( const wxString& aFileName )
{
    // this dummy plugin does not support rendering of any models
    return NULL;
}


static S3D_PLUGIN_DUMMY dummy;

#ifndef _WIN32
    extern "C" __attribute__((__visibility__("default"))) S3D_PLUGIN* Get3DPlugin( void )
    {
        return &dummy;
    }
#else
extern "C" __declspec( dllexport ) S3D_PLUGIN* Get3DPlugin( void )
    {
        return &dummy;
    }
#endif

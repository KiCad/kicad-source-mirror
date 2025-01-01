/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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

/*
 * This plugin implements a STEP/IGES model renderer for KiCad via OCE
 */

#include <wx/filename.h>
#include "plugins/3d/3d_plugin.h"
#include "plugins/3dapi/ifsg_all.h"

#include <string>
#include <vector>

SCENEGRAPH* LoadModel( char const* filename );

#define PLUGIN_OCE_MAJOR 1
#define PLUGIN_OCE_MINOR 4
#define PLUGIN_OCE_PATCH 2
#define PLUGIN_OCE_REVNO 0


const char* GetKicadPluginName( void )
{
    return "PLUGIN_3D_OCE";
}


void GetPluginVersion( unsigned char* Major,
                       unsigned char* Minor, unsigned char* Patch, unsigned char* Revision )
{
    if( Major )
        *Major = PLUGIN_OCE_MAJOR;

    if( Minor )
        *Minor = PLUGIN_OCE_MINOR;

    if( Patch )
        *Patch = PLUGIN_OCE_PATCH;

    if( Revision )
        *Revision = PLUGIN_OCE_REVNO;

    return;
}


static struct FILE_DATA
{
    std::vector<std::string> extensions;
    std::vector<std::string> filters;

    FILE_DATA()
    {
#ifdef _WIN32
        extensions = { "stp","step","stpz","stp.gz","step.gz","igs","iges" };
        filters = {
            "STEP (*.stp;*.step;*.stpz;*.stp.gz;*.step.gz)|*.stp;*.step;*.stpz;*stp.gz;*.step.gz",
            "IGES (*.igs;*.iges)|*.igs;*.iges" };
#else
        extensions = {
            "stp","STP","stpZ","stpz","STPZ","step","STEP","stp.gz","STP.GZ","step.gz","STEP.GZ",
            "igs","IGS","iges","IGES"
        };

        filters = {
            "STEP (*.stp;*.STP;*.stpZ;*.stpz;*.STPZ;*.step;*.STEP;*.stp.gz;*.STP.GZ;*.step.gz;"
            "*.STEP.GZ)|*.stp;*.STP;*.stpZ;*.stpz;*.STPZ;*.step;*.STEP;*.stp.gz;*.STP.GZ;"
            "*.step.gz;*.STEP.GZ",
            "IGES (*.igs;*.IGS;*.iges;*.IGES)|*.igs;*.IGS;*.iges;*.IGES"
        };
#endif
    }

} file_data;


int GetNExtensions( void )
{
    return file_data.extensions.size();
}


char const* GetModelExtension( int aIndex )
{
    if( aIndex < 0 || aIndex >= int( file_data.extensions.size() ) )
        return nullptr;

    return file_data.extensions[aIndex].c_str();
}


int GetNFilters( void )
{
    return file_data.filters.size();
}


char const* GetFileFilter( int aIndex )
{
    if( aIndex < 0 || aIndex >= int( file_data.filters.size() ) )
        return nullptr;

    return file_data.filters[aIndex].c_str();
}


bool CanRender( void )
{
    // this plugin supports rendering of IDF component outlines
    return true;
}


SCENEGRAPH* Load( char const* aFileName )
{
    if( nullptr == aFileName )
        return nullptr;

    wxString fname = wxString::FromUTF8Unchecked( aFileName );

    if( !wxFileName::FileExists( fname ) )
        return nullptr;

    return LoadModel( aFileName );
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-3.0.html
 */

#include "plugins/3d/3d_plugin.h"
#include "plugins/3dapi/ifsg_api.h"
#include "loadmodel.h"

#include <string>
#include <vector>

#define PLUGIN_VRML_MAJOR 1
#define PLUGIN_VRML_MINOR 3
#define PLUGIN_VRML_PATCH 2
#define PLUGIN_VRML_REVNO 2


const char* GetKicadPluginName()
{
    return "PLUGIN_3D_VRML";
}


void GetPluginVersion( unsigned char* aMajor, unsigned char* aMinor, unsigned char* aPatch,
                       unsigned char* aRevision )
{
    if( aMajor )
        *aMajor = PLUGIN_VRML_MAJOR;

    if( aMinor )
        *aMinor = PLUGIN_VRML_MINOR;

    if( aPatch )
        *aPatch = PLUGIN_VRML_PATCH;

    if( aRevision )
        *aRevision = PLUGIN_VRML_REVNO;
}


// Windows file matching is case insensitive, so the case variants would only duplicate entries.
#ifdef _WIN32
static const std::vector<std::string> extensions = { "wrl", "wrz", "x3d" };
static const std::vector<std::string> filters = { "VRML 1.0/2.0 (*.wrl;*.wrz)|*.wrl;*.wrz",
                                                  "X3D (*.x3d)|*.x3d" };
#else
static const std::vector<std::string> extensions = { "wrl", "WRL", "wrz", "WRZ", "x3d", "X3D" };
static const std::vector<std::string> filters = {
    "VRML 1.0/2.0 (*.wrl;*.WRL;*.wrz;*.WRZ)|*.wrl;*.WRL;*.wrz;*.WRZ",
    "X3D (*.x3d;*.X3D)|*.x3d;*.X3D"
};
#endif


int GetNExtensions()
{
    return static_cast<int>( extensions.size() );
}


char const* GetModelExtension( int aIndex )
{
    return aIndex >= 0 && aIndex < GetNExtensions() ? extensions[aIndex].c_str() : nullptr;
}


int GetNFilters()
{
    return static_cast<int>( filters.size() );
}


char const* GetFileFilter( int aIndex )
{
    return aIndex >= 0 && aIndex < GetNFilters() ? filters[aIndex].c_str() : nullptr;
}


bool CanRender()
{
    return true;
}


SCENEGRAPH* Load( char const* aFileName )
{
    return aFileName ? LoadVRMLModel( aFileName ) : nullptr;
}

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * Description:
 *  This plugin implements a STEP/IGES model renderer for KiCad via OCE
 */

#include <wx/filename.h>
#include "plugins/3d/3d_plugin.h"
#include "plugins/3dapi/ifsg_all.h"

SCENEGRAPH* LoadModel( char const* filename );

#define PLUGIN_OCE_MAJOR 1
#define PLUGIN_OCE_MINOR 1
#define PLUGIN_OCE_PATCH 1
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

// number of extensions supported
#ifdef _WIN32
#define NEXTS 4
#else
#define NEXTS 8
#endif

// number of filter sets supported
#define NFILS 2

static char ext0[] = "stp";
static char ext1[] = "step";
static char ext2[] = "igs";
static char ext3[] = "iges";

#ifdef _WIN32
static char fil0[] = "STEP (*.stp;*.step)|*.stp;*.step";
static char fil1[] = "IGES (*.igs;*.iges)|*.igs;*.iges";
#else
static char ext4[] = "STP";
static char ext5[] = "STEP";
static char ext6[] = "IGS";
static char ext7[] = "IGES";
static char fil0[] = "STEP (*.stp;*.STP;*.step;*.STEP)|*.stp;*.STP;*.step;*.STEP";
static char fil1[] = "IGES (*.igs;*.IGS;*.iges;*.IGES)|*.igs;*.IGS;*.iges;*.IGES";
#endif

static struct FILE_DATA
{
    char const* extensions[NEXTS];
    char const* filters[NFILS];

    FILE_DATA()
    {
        extensions[0] = ext0;
        extensions[1] = ext1;
        extensions[2] = ext2;
        extensions[3] = ext3;
        filters[0] = fil0;
        filters[1] = fil1;

#ifndef _WIN32
        extensions[4] = ext4;
        extensions[5] = ext5;
        extensions[6] = ext6;
        extensions[7] = ext7;
#endif

        return;
    }

} file_data;


int GetNExtensions( void )
{
    return NEXTS;
}


char const* GetModelExtension( int aIndex )
{
    if( aIndex < 0 || aIndex >= NEXTS )
        return NULL;

    return file_data.extensions[aIndex];
}


int GetNFilters( void )
{
    return NFILS;
}


char const* GetFileFilter( int aIndex )
{
    if( aIndex < 0 || aIndex >= NFILS )
        return NULL;

    return file_data.filters[aIndex];
}


bool CanRender( void )
{
    // this plugin supports rendering of IDF component outlines
    return true;
}


SCENEGRAPH* Load( char const* aFileName )
{
    if( NULL == aFileName )
        return NULL;

    wxString fname = wxString::FromUTF8Unchecked( aFileName );

    if( !wxFileName::FileExists( fname ) )
        return NULL;

    return LoadModel( aFileName );
}

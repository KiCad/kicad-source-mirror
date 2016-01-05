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
 * Description:
 *  This plugin implements the legacy kicad VRML1/VRML2 parsers.
 *  This VRML plugin will invoke a VRML1 or VRML2 parser depending
 *  on the identifying information in the file header:
 *
 *  #VRML V1.0 ascii
 *  #VRML V2.0 utf8
 */

#include <locale.h>
#include <fstream>
#include <iostream>
#include <cmath>
#include <string>
#include <wx/string.h>
#include <wx/filename.h>
#include "plugins/3d/3d_plugin.h"
#include "plugins/3dapi/ifsg_all.h"
#include "wrlproc.h"
#include "vrml1_base.h"
#include "vrml2_base.h"


#define PLUGIN_VRML_MAJOR 1
#define PLUGIN_VRML_MINOR 0
#define PLUGIN_VRML_PATCH 0
#define PLUGIN_VRML_REVNO 0


const char* GetKicadPluginName( void )
{
    return "PLUGIN_3D_VRML";
}


void GetPluginVersion( unsigned char* Major,
                       unsigned char* Minor, unsigned char* Patch, unsigned char* Revision )
{
    if( Major )
        *Major = PLUGIN_VRML_MAJOR;

    if( Minor )
        *Minor = PLUGIN_VRML_MINOR;

    if( Patch )
        *Patch = PLUGIN_VRML_PATCH;

    if( Revision )
        *Revision = PLUGIN_VRML_REVNO;

    return;
}

// number of extensions supported
#ifdef _WIN32
#define NEXTS 1
#else
#define NEXTS 2
#endif

// number of filter sets supported
#define NFILS 1

static char ext0[] = "wrl";

#ifdef _WIN32
static char fil0[] = "VRML 1.0/2.0 (*.wrl)|*.wrl";
#else
static char ext1[] = "WRL";
static char fil0[] = "VRML 1.0/2.0 (*.wrl;*.WRL)|*.wrl;*.WRL";
#endif

static struct FILE_DATA
{
    char const* extensions[NEXTS];
    char const* filters[NFILS];

    FILE_DATA()
    {
        extensions[0] = ext0;
        filters[0] = fil0;

#ifndef _WIN32
        extensions[1] = ext1;
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


class LOCALESWITCH
{
public:
    LOCALESWITCH()
    {
        setlocale( LC_NUMERIC, "C" );
    }

    ~LOCALESWITCH()
    {
        setlocale( LC_NUMERIC, "" );
    }
};


SCENEGRAPH* Load( char const* aFileName )
{
    if( NULL == aFileName )
        return NULL;

    wxString fname = wxString::FromUTF8Unchecked( aFileName );

    if( !wxFileName::FileExists( fname ) )
        return NULL;

    LOCALESWITCH switcher;
    SCENEGRAPH* scene = NULL;

    // VRML file processor
    WRLPROC proc;

    if( !proc.Open( std::string( fname.ToUTF8() ) ) )
        return NULL;

    if( proc.GetVRMLType() == VRML_V1 )
    {
        #ifdef DEBUG
        std::cout << " * [INFO] Processing VRML 1.0 file\n";
        #endif

        WRL1BASE* bp = new WRL1BASE;

        // allow Inline{} files to be included
        bp->SetEnableInline( true );

        if( !bp->Read( proc ) )
        {
            #ifdef DEBUG
            std::cout << " * [INFO] load failed\n";
            #endif
        }
        else
        {
            #ifdef DEBUG
            std::cout << " * [INFO] load completed\n";
            #endif

            // for now we recalculate all normals per-vertex per-face
            scene = (SCENEGRAPH*)bp->TranslateToSG( NULL, true );
        }

        delete bp;
    }
    else
    {
        #ifdef DEBUG
        std::cout << " * [INFO] Processing VRML 2.0 file\n";
        #endif

        WRL2BASE* bp = new WRL2BASE;

        // allow Inline{} files to be included
        bp->SetEnableInline( true );

        if( !bp->Read( proc ) )
        {
            #ifdef DEBUG
            std::cout << " * [INFO] load failed\n";
            #endif
        }
        else
        {
            #ifdef DEBUG
            std::cout << " * [INFO] load completed\n";
            #endif

            // for now we recalculate all normals per-vertex per-face
            scene = (SCENEGRAPH*)bp->TranslateToSG( NULL, true );
        }

        delete bp;
    }

    // DEBUG: WRITE OUT VRML2 FILE TO CONFIRM STRUCTURE
    #if defined( DEBUG_VRML1 ) || defined( DEBUG_VRML2 )
    if( scene )
    {
        wxFileName fn( wxString::FromUTF8Unchecked( aFileName ) );
        wxString output;

        if( proc.GetVRMLType() == VRML_V1 )
            output = wxT( "_vrml1-" );
        else
            output = wxT( "_vrml2-" );

        output.append( fn.GetName() );
        output.append( wxT(".wrl") );
        S3D::WriteVRML( output, true, (SGNODE*)(scene), true, true );
    }
    #endif

    return scene;
}

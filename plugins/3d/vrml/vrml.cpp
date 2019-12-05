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

/*
 * Description:
 *  This plugin implements the legacy kicad VRML1/VRML2 and X3D parsers
 *  The plugin will invoke a VRML1 or VRML2 parser depending on the
 *  identifying information in the file header:
 *
 *  #VRML V1.0 ascii
 *  #VRML V2.0 utf8
 */

#include "plugins/3d/3d_plugin.h"
#include "plugins/3dapi/ifsg_all.h"
#include "richio.h"
#include "vrml1_base.h"
#include "vrml2_base.h"
#include "wrlproc.h"
#include "x3d.h"
#include <clocale>
#include <wx/filename.h>
#include <wx/log.h>


#define PLUGIN_VRML_MAJOR 1
#define PLUGIN_VRML_MINOR 3
#define PLUGIN_VRML_PATCH 2
#define PLUGIN_VRML_REVNO 2


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
#define NEXTS 2
#else
#define NEXTS 4
#endif

// number of filter sets supported
#define NFILS 2

static char ext0[] = "wrl";
static char ext1[] = "x3d";

#ifdef _WIN32
static char fil0[] = "VRML 1.0/2.0 (*.wrl)|*.wrl";
static char fil1[] = "X3D (*.x3d)|*.x3d";
#else
static char ext2[] = "WRL";
static char ext3[] = "X3D";
static char fil0[] = "VRML 1.0/2.0 (*.wrl;*.WRL)|*.wrl;*.WRL";
static char fil1[] = "X3D (*.x3d;*.X3D)|*.x3d;*.X3D";
#endif

static struct FILE_DATA
{
    char const* extensions[NEXTS];
    char const* filters[NFILS];

    FILE_DATA()
    {
        extensions[0] = ext0;
        extensions[1] = ext1;
        filters[0] = fil0;
        filters[1] = fil1;

#ifndef _WIN32
        extensions[2] = ext2;
        extensions[3] = ext3;
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
    // Store the user locale name, to restore this locale later, in dtor
    std::string m_locale;

public:
    LOCALESWITCH()
    {
        m_locale = setlocale( LC_NUMERIC, 0 );
        setlocale( LC_NUMERIC, "C" );
    }

    ~LOCALESWITCH()
    {
        setlocale( LC_NUMERIC, m_locale.c_str() );
    }
};


SCENEGRAPH* LoadVRML( const wxString& aFileName, bool useInline )
{
    FILE_LINE_READER* modelFile = NULL;
    SCENEGRAPH* scene = NULL;

    try
    {
        // set the max char limit to 8MB; if a VRML file contains
        // longer lines then perhaps it shouldn't be used
        modelFile = new FILE_LINE_READER( aFileName, 0, 8388608 );
    }
    catch( IO_ERROR & )
    {
        wxLogError( _( " * [INFO] load failed: input line too long\n" ) );
        return NULL;
    }


    // VRML file processor
    WRLPROC proc( modelFile );

    if( proc.GetVRMLType() == VRML_V1 )
    {
        wxLogTrace( MASK_VRML, " * [INFO] Processing VRML 1.0 file\n" );

        WRL1BASE* bp = new WRL1BASE;

        if( !bp->Read( proc ) )
        {
            wxLogTrace( MASK_VRML, " * [INFO] load failed\n" );
        }
        else
        {
            wxLogTrace( MASK_VRML, " * [INFO] load completed\n" );

            scene = (SCENEGRAPH*)bp->TranslateToSG( NULL, NULL );
        }

        delete bp;
    }
    else
    {
        wxLogTrace( MASK_VRML, " * [INFO] Processing VRML 2.0 file\n" );

        WRL2BASE* bp = new WRL2BASE;

        // allow Inline{} files to be included
        bp->SetEnableInline( true );

        if( !bp->Read( proc ) )
        {
            wxLogTrace( MASK_VRML, " * [INFO] load failed\n" );
        }
        else
        {
            wxLogTrace( MASK_VRML, " * [INFO] load completed\n" );

            // for now we recalculate all normals per-vertex per-face
            scene = (SCENEGRAPH*)bp->TranslateToSG( NULL );
        }

        delete bp;
    }

    if( NULL != modelFile )
        delete modelFile;

    // DEBUG: WRITE OUT VRML2 FILE TO CONFIRM STRUCTURE
    #if ( defined( DEBUG_VRML1 ) && DEBUG_VRML1 > 3 ) \
        || ( defined( DEBUG_VRML2 ) && DEBUG_VRML2 > 3 )
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
        S3D::WriteVRML( output.ToUTF8(), true, (SGNODE*)(scene), true, true );
    }
    #endif

    return scene;
}


SCENEGRAPH* LoadX3D( const wxString& aFileName )
{
    SCENEGRAPH* scene = NULL;
    X3DPARSER model;
    scene = model.Load( aFileName );

    return scene;
}


SCENEGRAPH* Load( char const* aFileName )
{
    if( NULL == aFileName )
        return NULL;

    wxString fname = wxString::FromUTF8Unchecked( aFileName );

    if( !wxFileName::FileExists( fname ) )
        return NULL;

    LOCALESWITCH switcher;

    SCENEGRAPH* scene = NULL;
    wxString ext = wxFileName( fname ).GetExt();

    if( ext == "x3d" || ext == "X3D" )
        scene = LoadX3D( fname );
    else
        scene = LoadVRML( fname, true );

    return scene;
}

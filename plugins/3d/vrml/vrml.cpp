/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015, 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 *  This plugin implements the legacy KiCad VRML1/VRML2 and X3D parsers
 *  The plugin will invoke a VRML1 or VRML2 parser depending on the
 *  identifying information in the file header:
 *
 *  #VRML V1.0 ASCII
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
#include <wx/stdpaths.h>
#include <wx/string.h>
#include <wx/wfstream.h>
#include <wx/log.h>

#include <decompress.hpp>


#define PLUGIN_VRML_MAJOR 1
#define PLUGIN_VRML_MINOR 3
#define PLUGIN_VRML_PATCH 2
#define PLUGIN_VRML_REVNO 2


/**
 * Flag to enable VRML plugin trace output.
 *
 * @ingroup trace_env_vars
 */
const wxChar* const traceVrmlPlugin = wxT( "KICAD_VRML_PLUGIN" );


const char* GetKicadPluginName( void )
{
    return "PLUGIN_3D_VRML";
}


void GetPluginVersion( unsigned char* Major, unsigned char* Minor, unsigned char* Patch,
                       unsigned char* Revision )
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


static struct FILE_DATA
{
    std::vector<std::string> extensions;
    std::vector<std::string> filters;

    FILE_DATA()
    {

#ifdef _WIN32
        extensions = { "wrl", "wrz", "x3d" };
        filters = { "VRML 1.0/2.0 (*.wrl;*.wrz)|*.wrl;*.wrz",
                    "X3D (*.x3d)|*.x3d" };
#else
        extensions = { "wrl", "WRL", "wrz", "WRZ", "x3d", "X3D" };
        filters = { "VRML 1.0/2.0 (*.wrl;*.WRL;*.wrz;*.WRZ)|*.wrl;*.WRL;*.wrz;*.WRZ",
                    "X3D (*.x3d;*.X3D)|*.x3d;*.X3D" };
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


class LOCALESWITCH
{
public:
    LOCALESWITCH()
    {
        m_locale = setlocale( LC_NUMERIC, nullptr );
        setlocale( LC_NUMERIC, "C" );
    }

    ~LOCALESWITCH()
    {
        setlocale( LC_NUMERIC, m_locale.c_str() );
    }

private:
    // Store the user locale name, to restore this locale later, in dtor
    std::string m_locale;
};


SCENEGRAPH* LoadVRML( const wxString& aFileName, bool useInline )
{
    FILE_LINE_READER* modelFile = nullptr;
    SCENEGRAPH* scene = nullptr;
    wxString filename = aFileName;
    wxFileName tmpfilename;

    if( aFileName.Upper().EndsWith( wxT( "WRZ" ) ) )
    {
        wxFFileInputStream ifile( aFileName );
        tmpfilename = wxFileName( aFileName );

        tmpfilename.SetPath( wxStandardPaths::Get().GetTempDir() );
        tmpfilename.SetExt( wxT( "WRL" ) );

        wxFileOffset size = ifile.GetLength();

        if( size == wxInvalidOffset )
            return nullptr;

        {
            wxFFileOutputStream ofile( tmpfilename.GetFullPath() );

            if( !ofile.IsOk() )
                return nullptr;

            char *buffer = new char[size];

            ifile.Read( buffer, size);
            std::string expanded;

            try
            {
                expanded = gzip::decompress( buffer, size );
            }
            catch( std::runtime_error& e )
            {
                wxLogTrace( traceVrmlPlugin, wxS( " * [INFO] wrz load failed: %s" ),
                            wxString::FromUTF8Unchecked( e.what() ) );

                delete[] buffer;
                return nullptr;
            }
            catch( ... )
            {
                wxLogTrace( traceVrmlPlugin, wxS( " * [INFO] wrz load failed: unknown error" ) );

                delete[] buffer;
                return nullptr;
            }

            delete[] buffer;

            ofile.Write( expanded.data(), expanded.size() );
            ofile.Close();
        }

        filename = tmpfilename.GetFullPath();
    }

    try
    {
        // set the max char limit to 8MB; if a VRML file contains
        // longer lines then perhaps it shouldn't be used
        modelFile = new FILE_LINE_READER( filename, 0, 8388608 );
    }
    catch( IO_ERROR& e )
    {
        wxLogError( wxString( wxS( " * " ) )
                    << wxString::Format( _( "[INFO] load failed: %s" ), e.What() ) );

        return nullptr;
    }


    // VRML file processor
    WRLPROC proc( modelFile );

    // Cleanup our temporary file
    if( tmpfilename.IsOk() )
        wxRemoveFile( tmpfilename.GetFullPath() );

    if( proc.GetVRMLType() == WRLVERSION::VRML_V1 )
    {
        wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Processing VRML 1.0 file" ) );

        WRL1BASE* bp = new WRL1BASE;

        if( !bp->Read( proc ) )
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] load failed" ) );
        }
        else
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] load completed" ) );

            scene = (SCENEGRAPH*)bp->TranslateToSG( nullptr, nullptr );
        }

        delete bp;
    }
    else
    {
        wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Processing VRML 2.0 file" ) );

        WRL2BASE* bp = new WRL2BASE;

        // allow Inline{} files to be included (controlled by the useInline parameter
        // to prevent infinite recursion when loading referenced VRML files)
        bp->SetEnableInline( useInline );

        if( !bp->Read( proc ) )
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] load failed" ) );
        }
        else
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] load completed" ) );

            // for now we recalculate all normals per-vertex per-face
            scene = (SCENEGRAPH*)bp->TranslateToSG( nullptr );
        }

        delete bp;
    }

    if( nullptr != modelFile )
        delete modelFile;

    // DEBUG: WRITE OUT VRML2 FILE TO CONFIRM STRUCTURE
#if ( defined( DEBUG_VRML1 ) && DEBUG_VRML1 > 3 )           \
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
    SCENEGRAPH* scene = nullptr;
    X3DPARSER model;
    scene = model.Load( aFileName );

    return scene;
}


SCENEGRAPH* Load( char const* aFileName )
{
    if( nullptr == aFileName )
        return nullptr;

    wxString fname = wxString::FromUTF8Unchecked( aFileName );

    if( !wxFileName::FileExists( fname ) )
        return nullptr;

    LOCALESWITCH switcher;

    SCENEGRAPH* scene = nullptr;
    wxString ext = wxFileName( fname ).GetExt();

    if( ext == wxT( "x3d" ) || ext == wxT( "X3D" ) )
        scene = LoadX3D( fname );
    else
        scene = LoadVRML( fname, true );

    return scene;
}

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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
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
#include "vrml_line_reader.h"
#include "vrml1_base.h"
#include "vrml2_base.h"
#include "wrlproc.h"
#include "x3d.h"
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/string.h>
#include <wx/wfstream.h>
#include <wx/log.h>

#include <algorithm>
#include <limits>
#include <memory>
#include <vector>
#include "loadmodel.h"
#include "../model_import_internal.h"

/**
 * Flag to enable VRML plugin trace output.
 *
 * @ingroup trace_env_vars
 */
const wxChar* const traceVrmlPlugin = wxT( "KICAD_VRML_PLUGIN" );

namespace
{
thread_local std::vector<wxString> inlineLoadStack;

class INLINE_LOAD_GUARD
{
public:
    explicit INLINE_LOAD_GUARD( wxString aPath ) { inlineLoadStack.emplace_back( std::move( aPath ) ); }

    ~INLINE_LOAD_GUARD() { inlineLoadStack.pop_back(); }
};
} // namespace


SCENEGRAPH* LoadVRML( const wxString& aFileName, const S3D::MODEL_IMPORT_OPTIONS& aOptions,
                      MODEL_IMPORT_STATUS* aStatus )
{
    wxFileName sourcePath( aFileName );
    sourcePath.Normalize( wxPATH_NORM_DOTS | wxPATH_NORM_TILDE | wxPATH_NORM_ABSOLUTE | wxPATH_NORM_ENV_VARS );
    wxString canonicalPath = sourcePath.GetFullPath();

    if( inlineLoadStack.size() >= 64 )
    {
        if( aStatus )
            aStatus->Set( S3D::MODEL_IMPORT_ERROR::TOO_LARGE, "VRML Inline nesting exceeds the configured limit" );

        return nullptr;
    }

    if( std::find( inlineLoadStack.begin(), inlineLoadStack.end(), canonicalPath ) != inlineLoadStack.end() )
    {
        if( aStatus )
            aStatus->Set( S3D::MODEL_IMPORT_ERROR::INVALID, "VRML Inline dependency cycle detected" );

        return nullptr;
    }

    INLINE_LOAD_GUARD inlineGuard( canonicalPath );
    MODEL_IMPORT_TEMP_FILE cleanup;
    std::unique_ptr<VRML_LINE_READER> modelFile;
    SCENEGRAPH* scene = nullptr;
    wxString filename = aFileName;
    wxFileName tmpfilename;

    if( aFileName.Upper().EndsWith( wxT( "WRZ" ) ) )
    {
        wxFFileInputStream ifile( aFileName );
        tmpfilename = wxFileName( CreateModelImportTempFileName( wxS( "kicad-vrml-" ) ) );
        cleanup.path = tmpfilename.GetFullPath();

        if( cleanup.path.empty() || !ifile.IsOk() )
        {
            if( aStatus )
                aStatus->Set( S3D::MODEL_IMPORT_ERROR::UNREADABLE, "Unable to open compressed VRML" );

            return nullptr;
        }

        wxFileOffset size = ifile.GetLength();

        if( size == wxInvalidOffset )
            return nullptr;

        if( static_cast<std::uint64_t>( size ) > aOptions.maxCompressedBytes
            || static_cast<std::uint64_t>( size ) > std::numeric_limits<std::size_t>::max() )
        {
            if( aStatus )
                aStatus->Set( S3D::MODEL_IMPORT_ERROR::TOO_LARGE, "Compressed VRML exceeds the configured limit" );

            return nullptr;
        }

        {
            wxFFileOutputStream ofile( tmpfilename.GetFullPath() );

            if( !ofile.IsOk() )
                return nullptr;

            std::vector<char> buffer( static_cast<size_t>( size ) );

            ifile.Read( buffer.data(), size );

            if( ifile.LastRead() != static_cast<size_t>( size ) )
                return nullptr;

            std::string expanded;

            switch( DecompressBoundedModel( buffer.data(), static_cast<size_t>( size ),
                                            aOptions.maxExpandedBytes, expanded ) )
            {
            case MODEL_IMPORT_GZIP_RESULT::OK:
                break;

            case MODEL_IMPORT_GZIP_RESULT::TOO_LARGE:
                if( aStatus )
                    aStatus->Set( S3D::MODEL_IMPORT_ERROR::TOO_LARGE, "Expanded VRML exceeds the configured limit" );

                return nullptr;

            case MODEL_IMPORT_GZIP_RESULT::CORRUPT:
                if( aStatus )
                    aStatus->Set( S3D::MODEL_IMPORT_ERROR::INVALID, "Compressed VRML archive is corrupt" );

                wxLogTrace( traceVrmlPlugin, wxS( " * [INFO] wrz load failed" ) );

                return nullptr;
            }

            ofile.Write( expanded.data(), expanded.size() );

            if( ofile.LastWrite() != expanded.size() || !ofile.Close() )
                return nullptr;
        }

        filename = tmpfilename.GetFullPath();
    }

    try
    {
        // set the max char limit to 8MB; if a VRML file contains
        // longer lines then perhaps it shouldn't be used
        modelFile = std::make_unique<VRML_LINE_READER>( filename, 0, 8388608, aOptions );
    }
    catch( const std::exception& e )
    {
        wxLogTrace( traceVrmlPlugin, wxS( " * [INFO] load failed: %s" ), wxString::FromUTF8Unchecked( e.what() ) );

        return nullptr;
    }


    // VRML file processor
    WRLPROC proc( modelFile.get() );

    if( proc.GetVRMLType() == WRLVERSION::VRML_V1 )
    {
        wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Processing VRML 1.0 file" ) );

        auto bp = std::make_unique<WRL1BASE>();

        if( !bp->Read( proc ) )
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] load failed" ) );
        }
        else
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] load completed" ) );

            scene = (SCENEGRAPH*)bp->TranslateToSG( nullptr, nullptr );
        }
    }
    else if( proc.GetVRMLType() == WRLVERSION::VRML_V2 )
    {
        wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] Processing VRML 2.0 file" ) );

        auto bp = std::make_unique<WRL2BASE>();

        // Inline{} inclusion is disabled for nested loads to bound recursion.
        bp->SetEnableInline( aOptions.vrmlEnableInline );

        // Top-level files start in legacy mode and drop out of it when a top-level scale
        // transform is found.  Inline submodels inherit the parent's setting.
        bp->SetApplyUnitConversion( aOptions.vrmlApplyLegacyUnitConversion );
        bp->SetImportOptions( aOptions, aStatus );

        if( !bp->Read( proc ) )
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] load failed" ) );
        }
        else
        {
            wxLogTrace( traceVrmlPlugin, wxT( " * [INFO] load completed" ) );

            // for now we recalculate all normals per-vertex per-face
            scene = (SCENEGRAPH*)bp->TranslateToSG( nullptr );

            if( bp->HadIncompleteInline() )
            {
                if( aStatus && aStatus->diagnostic.empty() )
                    aStatus->Set( S3D::MODEL_IMPORT_ERROR::IMPORT_FAILED,
                                  "VRML Inline dependency was denied or failed; geometry would be incomplete" );

                bp.reset();

                if( scene )
                    S3D::DestroyNode( reinterpret_cast<SGNODE*>( scene ) );

                return nullptr;
            }
        }
    }

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


SCENEGRAPH* LoadX3D( const wxString& aFileName, const S3D::MODEL_IMPORT_OPTIONS& aOptions,
                     MODEL_IMPORT_STATUS* aStatus )
{
    if( aOptions.IsCanceled() )
        throw MODEL_IMPORT_CANCELED();

    SCENEGRAPH* scene = nullptr;
    X3DPARSER model;
    scene = model.Load( aFileName );

    if( aOptions.IsCanceled() )
    {
        if( scene )
            S3D::DestroyNode( reinterpret_cast<SGNODE*>( scene ) );

        throw MODEL_IMPORT_CANCELED();
    }

    if( !scene && aStatus )
        aStatus->Set( S3D::MODEL_IMPORT_ERROR::INVALID, "X3D parser rejected the file" );

    return scene;
}


SCENEGRAPH* LoadVRMLModel( const char* aFileName, const S3D::MODEL_IMPORT_OPTIONS* aOptions,
                           MODEL_IMPORT_STATUS* aStatus )
{
    if( nullptr == aFileName )
        return nullptr;

    wxString fname = wxString::FromUTF8Unchecked( aFileName );

    if( !wxFileName::FileExists( fname ) )
        return nullptr;

    S3D::MODEL_IMPORT_OPTIONS options;

    if( aOptions )
        options = *aOptions;

    SCENEGRAPH* scene = nullptr;
    wxString ext = wxFileName( fname ).GetExt();

    if( options.format == S3D::MODEL_IMPORT_FORMAT::X3D
        || ( options.format == S3D::MODEL_IMPORT_FORMAT::AUTO && ( ext == wxT( "x3d" ) || ext == wxT( "X3D" ) ) ) )
        scene = LoadX3D( fname, options, aStatus );
    else if( options.format == S3D::MODEL_IMPORT_FORMAT::VRML || options.format == S3D::MODEL_IMPORT_FORMAT::AUTO )
        scene = LoadVRML( fname, options, aStatus );

    if( !scene && aStatus && aStatus->diagnostic.empty() )
        aStatus->Set( S3D::MODEL_IMPORT_ERROR::IMPORT_FAILED, "VRML parser rejected the file" );

    return scene;
}


SCENEGRAPH* LoadVRMLModel( const char* aFileName, const S3D::MODEL_IMPORT_OPTIONS* aOptions )
{
    S3D::MODEL_IMPORT_LOCK      lock;
    MODEL_IMPORT_NUMERIC_LOCALE locale;
    MODEL_IMPORT_STATUS         status;

    try
    {
        if( SCENEGRAPH* scene = LoadVRMLModel( aFileName, aOptions, &status ) )
            return scene;
    }
    catch( ... )
    {
        return nullptr;
    }

    wxLogTrace( traceVrmlPlugin, wxT( "Model import failed: %s" ), status.diagnostic.c_str() );
    return nullptr;
}

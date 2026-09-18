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

#include <plugins/3dapi/model_import.h>

#include <plugins/3dapi/ifsg_api.h>
#include <plugins/3dapi/ifsg_all.h>

#include "oce/loadmodel.h"
#include "vrml/loadmodel.h"
#include "model_import_internal.h"

#include <wx/filename.h>
#include <wx/log.h>

#ifdef _WIN32
#include <shlobj.h>
#endif

#include <algorithm>
#include <cmath>
#include <exception>
#include <limits>
#include <new>
#include <string>
#include <vector>

#include <decompress.hpp>

#include <Standard_Failure.hxx>

#ifdef _WIN32
namespace
{
class TOKEN_HANDLE
{
public:
    ~TOKEN_HANDLE()
    {
        if( value )
            CloseHandle( value );
    }

    HANDLE value = nullptr;
};

class COM_ALLOCATED_PATH
{
public:
    ~COM_ALLOCATED_PATH() { CoTaskMemFree( value ); }

    PWSTR value = nullptr;
};

bool IsLowIntegrityProcess()
{
    TOKEN_HANDLE token;

    if( !OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &token.value ) )
        return false;

    DWORD size = 0;
    GetTokenInformation( token.value, TokenIntegrityLevel, nullptr, 0, &size );
    std::vector<BYTE> buffer( size );
    bool lowIntegrity = false;

    if( size > 0 && GetTokenInformation( token.value, TokenIntegrityLevel, buffer.data(), size, &size ) )
    {
        const auto* label = reinterpret_cast<const TOKEN_MANDATORY_LABEL*>( buffer.data() );
        PSID        sid = label->Label.Sid;
        DWORD       count = *GetSidSubAuthorityCount( sid );

        if( count > 0 )
            lowIntegrity = *GetSidSubAuthority( sid, count - 1 ) < SECURITY_MANDATORY_MEDIUM_RID;
    }

    return lowIntegrity;
}
} // namespace
#endif


wxString CreateModelImportTempFileName( const wxString& aPrefix )
{
#ifdef _WIN32
    if( !IsLowIntegrityProcess() )
        return wxFileName::CreateTempFileName( aPrefix );

    COM_ALLOCATED_PATH localLowPath;
    HRESULT result = SHGetKnownFolderPath( FOLDERID_LocalAppDataLow, KF_FLAG_CREATE, nullptr, &localLowPath.value );

    if( FAILED( result ) )
        return {};

    wxFileName directory( localLowPath.value, wxEmptyString );
    directory.AppendDir( wxS( "KiCadPreview" ) );

    if( !directory.DirExists() && !directory.Mkdir( wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL )
        && !directory.DirExists() )
        return {};

    return wxFileName::CreateTempFileName( wxFileName( directory.GetFullPath(), aPrefix ).GetFullPath() );
#else
    return wxFileName::CreateTempFileName( aPrefix );
#endif
}

MODEL_IMPORT_GZIP_RESULT DecompressBoundedModel( const char* aData, std::size_t aSize,
                                                 std::uint64_t aMaxExpandedBytes, std::string& aExpanded )
{
    const std::size_t cap = static_cast<std::size_t>(
            std::min<std::uint64_t>( aMaxExpandedBytes, std::numeric_limits<std::size_t>::max() ) );

    try
    {
        aExpanded = gzip::decompress( aData, aSize, cap, std::min<std::size_t>( cap, 64U * 1024U ) );
    }
    catch( const gzip::limit_error& )
    {
        return MODEL_IMPORT_GZIP_RESULT::TOO_LARGE;
    }
    catch( ... )
    {
        return MODEL_IMPORT_GZIP_RESULT::CORRUPT;
    }

    return MODEL_IMPORT_GZIP_RESULT::OK;
}


namespace
{
class SCENE_OWNER
{
public:
    explicit SCENE_OWNER( SCENEGRAPH* aScene ) :
            m_scene( aScene )
    {
    }
    ~SCENE_OWNER() { S3D::DestroyNode( reinterpret_cast<SGNODE*>( m_scene ) ); }

    SCENEGRAPH* Get() const { return m_scene; }

private:
    SCENEGRAPH* m_scene;
};

enum class VALIDATION_RESULT
{
    VALID,
    INVALID,
    TOO_LARGE
};

bool addBudget( std::uint64_t& aTotal, std::uint64_t aCount, std::uint64_t aElementSize, std::uint64_t aLimit )
{
    if( aTotal > aLimit || ( aCount && aElementSize > ( aLimit - aTotal ) / aCount ) )
        return false;

    aTotal += aCount * aElementSize;
    return true;
}


bool isFinite( const SFVEC3F& aValue )
{
    return std::isfinite( aValue.x ) && std::isfinite( aValue.y ) && std::isfinite( aValue.z );
}


VALIDATION_RESULT validateModel( S3DMODEL& aModel, const S3D::MODEL_IMPORT_OPTIONS& aOptions )
{
    std::uint64_t total = 0;
    bool          hasValidFaces = false;
    unsigned int  outputMesh = 0;

    if( !aModel.m_Meshes || !aModel.m_MeshesSize || ( aModel.m_MaterialsSize && !aModel.m_Materials ) )
        return VALIDATION_RESULT::INVALID;

    if( !addBudget( total, aModel.m_MeshesSize, sizeof( SMESH ), aOptions.maxMeshBytes )
        || !addBudget( total, aModel.m_MaterialsSize, sizeof( SMATERIAL ), aOptions.maxMeshBytes ) )
        return VALIDATION_RESULT::TOO_LARGE;

    for( unsigned int i = 0; i < aModel.m_MaterialsSize; ++i )
    {
        const SMATERIAL& material = aModel.m_Materials[i];

        if( !isFinite( material.m_Ambient ) || !isFinite( material.m_Diffuse ) || !isFinite( material.m_Emissive )
            || !isFinite( material.m_Specular ) || !std::isfinite( material.m_Shininess )
            || !std::isfinite( material.m_Transparency ) || material.m_Shininess < 0.0f || material.m_Shininess > 1.0f
            || material.m_Transparency < 0.0f || material.m_Transparency > 1.0f )
            return VALIDATION_RESULT::INVALID;
    }

    for( unsigned int i = 0; i < aModel.m_MeshesSize; ++i )
    {
        SMESH& mesh = aModel.m_Meshes[i];

        if( !mesh.m_Positions || !mesh.m_Normals || !mesh.m_VertexSize || !mesh.m_FaceIdx || mesh.m_FaceIdxSize % 3
            || mesh.m_MaterialIdx >= aModel.m_MaterialsSize )
        {
            S3D::Free3DMesh( mesh );
            continue;
        }

        if( !addBudget( total, mesh.m_VertexSize, sizeof( SFVEC3F ) * 2, aOptions.maxMeshBytes )
            || !addBudget( total, mesh.m_FaceIdxSize, sizeof( unsigned int ), aOptions.maxMeshBytes )
            || ( mesh.m_Color && !addBudget( total, mesh.m_VertexSize, sizeof( SFVEC3F ), aOptions.maxMeshBytes ) )
            || ( mesh.m_Texcoords
                 && !addBudget( total, mesh.m_VertexSize, sizeof( SFVEC2F ), aOptions.maxMeshBytes ) ) )
            return VALIDATION_RESULT::TOO_LARGE;

        bool finiteVertices = true;

        for( unsigned int vertex = 0; vertex < mesh.m_VertexSize; ++vertex )
        {
            if( !isFinite( mesh.m_Positions[vertex] ) || !isFinite( mesh.m_Normals[vertex] )
                || ( mesh.m_Color && !isFinite( mesh.m_Color[vertex] ) )
                || ( mesh.m_Texcoords
                     && ( !std::isfinite( mesh.m_Texcoords[vertex].x )
                          || !std::isfinite( mesh.m_Texcoords[vertex].y ) ) ) )
            {
                finiteVertices = false;
                break;
            }
        }

        if( !finiteVertices )
        {
            S3D::Free3DMesh( mesh );
            continue;
        }

        unsigned int output = 0;

        for( unsigned int triangle = 0; triangle + 3 <= mesh.m_FaceIdxSize; triangle += 3 )
        {
            if( IsTriangleInRange( mesh.m_FaceIdx, triangle, mesh.m_VertexSize ) )
            {
                mesh.m_FaceIdx[output++] = mesh.m_FaceIdx[triangle];
                mesh.m_FaceIdx[output++] = mesh.m_FaceIdx[triangle + 1];
                mesh.m_FaceIdx[output++] = mesh.m_FaceIdx[triangle + 2];
            }
        }

        mesh.m_FaceIdxSize = output;
        hasValidFaces = hasValidFaces || output > 0;

        if( output == 0 )
        {
            S3D::Free3DMesh( mesh );
            continue;
        }

        if( outputMesh != i )
        {
            aModel.m_Meshes[outputMesh] = mesh;
            S3D::Init3DMesh( mesh );
        }

        ++outputMesh;
    }

    aModel.m_MeshesSize = outputMesh;

    return hasValidFaces && outputMesh > 0 ? VALIDATION_RESULT::VALID : VALIDATION_RESULT::INVALID;
}
} // namespace


S3D::MODEL_IMPORT_RESULT::MODEL_IMPORT_RESULT( MODEL_IMPORT_RESULT&& aOther ) noexcept :
        m_model( aOther.m_model ),
        m_error( aOther.m_error ),
        m_diagnostic( std::move( aOther.m_diagnostic ) )
{
    aOther.m_model = nullptr;
}


S3D::MODEL_IMPORT_RESULT& S3D::MODEL_IMPORT_RESULT::operator=( MODEL_IMPORT_RESULT&& aOther ) noexcept
{
    if( this != &aOther )
    {
        Destroy3DModel( &m_model );
        m_model = aOther.m_model;
        m_error = aOther.m_error;
        m_diagnostic = std::move( aOther.m_diagnostic );
        aOther.m_model = nullptr;
    }

    return *this;
}


S3D::MODEL_IMPORT_RESULT::~MODEL_IMPORT_RESULT()
{
    Destroy3DModel( &m_model );
}


S3D::MODEL_IMPORT_RESULT::operator bool() const noexcept
{
    return m_model;
}


const S3DMODEL* S3D::MODEL_IMPORT_RESULT::GetModel() const noexcept
{
    return m_model;
}


S3D::MODEL_IMPORT_ERROR S3D::MODEL_IMPORT_RESULT::GetError() const noexcept
{
    return m_error;
}


std::string_view S3D::MODEL_IMPORT_RESULT::GetDiagnostic() const noexcept
{
    return m_diagnostic;
}


S3D::MODEL_IMPORT_RESULT S3D::ImportModel( const std::string& aFileName, const MODEL_IMPORT_OPTIONS& aOptions ) noexcept
{
    MODEL_IMPORT_RESULT result;

    try
    {
        S3D::MODEL_IMPORT_LOCK      lock;
        MODEL_IMPORT_NUMERIC_LOCALE numericLocale;

        // Preview imports must not reach wxWidgets' GUI log target.
        wxLogNull                   logGuard;

        if( aOptions.IsCanceled() )
        {
            result.m_error = MODEL_IMPORT_ERROR::CANCELED;
            return result;
        }

        wxFileName fileName( wxString::FromUTF8( aFileName ) );

        if( !fileName.FileExists() || !fileName.IsFileReadable() )
        {
            result.m_error = MODEL_IMPORT_ERROR::UNREADABLE;
            result.m_diagnostic = "Model file is missing or unreadable";
            return result;
        }

        if( aOptions.format < MODEL_IMPORT_FORMAT::AUTO || aOptions.format > MODEL_IMPORT_FORMAT::X3D
            || !std::isfinite( aOptions.stepLinearDeflection ) || aOptions.stepLinearDeflection <= 0.0
            || !std::isfinite( aOptions.stepAngularDeflectionDegrees ) || aOptions.stepAngularDeflectionDegrees <= 0.0 )
        {
            result.m_error = MODEL_IMPORT_ERROR::INVALID;
            result.m_diagnostic = "Invalid model import options";
            return result;
        }

        wxULongLong inputSize = fileName.GetSize();

        if( inputSize == wxInvalidSize || inputSize.GetValue() > aOptions.maxExpandedBytes )
        {
            result.m_error = MODEL_IMPORT_ERROR::TOO_LARGE;
            result.m_diagnostic = "Model input exceeds the configured limit";
            return result;
        }

        std::string         extension = fileName.GetExt().Lower().ToStdString();
        std::string         lowerName = fileName.GetFullName().Lower().ToStdString();
        MODEL_IMPORT_FORMAT format = aOptions.format;

        if( format == MODEL_IMPORT_FORMAT::AUTO )
        {
            if( extension == "stp" || extension == "step" || extension == "stpz" || extension == "stepz"
                || lowerName.ends_with( ".stp.gz" ) || lowerName.ends_with( ".step.gz" ) )
                format = MODEL_IMPORT_FORMAT::STEP;
            else if( extension == "igs" || extension == "iges" )
                format = MODEL_IMPORT_FORMAT::IGES;
            else if( extension == "wrl" || extension == "wrz" )
                format = MODEL_IMPORT_FORMAT::VRML;
            else if( extension == "x3d" )
                format = MODEL_IMPORT_FORMAT::X3D;
            else
            {
                result.m_error = MODEL_IMPORT_ERROR::UNSUPPORTED;
                result.m_diagnostic = "Unsupported model format";
                return result;
            }
        }

        MODEL_IMPORT_STATUS status;
        SCENEGRAPH*         scene = nullptr;

        if( format == MODEL_IMPORT_FORMAT::STEP || format == MODEL_IMPORT_FORMAT::IGES )
            scene = LoadModel( aFileName.c_str(), &aOptions, &status );
        else if( format == MODEL_IMPORT_FORMAT::VRML || format == MODEL_IMPORT_FORMAT::X3D )
            scene = LoadVRMLModel( aFileName.c_str(), &aOptions, &status );
        else
        {
            result.m_error = MODEL_IMPORT_ERROR::UNSUPPORTED;
            return result;
        }

        if( !scene )
        {
            result.m_error = status.error;
            result.m_diagnostic = std::move( status.diagnostic );

            if( result.m_diagnostic.empty() )
                result.m_diagnostic = "Model parser rejected the file";

            return result;
        }

        SCENE_OWNER sceneOwner( scene );
        result.m_model = GetModel( sceneOwner.Get() );

        if( aOptions.IsCanceled() )
        {
            Destroy3DModel( &result.m_model );
            result.m_error = MODEL_IMPORT_ERROR::CANCELED;
            return result;
        }

        VALIDATION_RESULT validation =
                result.m_model ? validateModel( *result.m_model, aOptions ) : VALIDATION_RESULT::INVALID;

        if( validation != VALIDATION_RESULT::VALID )
        {
            Destroy3DModel( &result.m_model );
            result.m_error = validation == VALIDATION_RESULT::TOO_LARGE ? MODEL_IMPORT_ERROR::TOO_LARGE
                                                                        : MODEL_IMPORT_ERROR::INVALID;
            result.m_diagnostic = validation == VALIDATION_RESULT::TOO_LARGE
                                          ? "Model exceeds the configured mesh limit"
                                          : "Model contains invalid or no renderable geometry";
            return result;
        }

        result.m_error = MODEL_IMPORT_ERROR::NONE;
        return result;
    }
    catch( const std::bad_alloc& )
    {
        Destroy3DModel( &result.m_model );
        result.m_error = MODEL_IMPORT_ERROR::OUT_OF_MEMORY;
    }
    catch( const MODEL_IMPORT_CANCELED& )
    {
        Destroy3DModel( &result.m_model );
        result.m_error = MODEL_IMPORT_ERROR::CANCELED;
    }
    catch( const Standard_Failure& e )
    {
        Destroy3DModel( &result.m_model );
        result.m_error = MODEL_IMPORT_ERROR::IMPORT_FAILED;

        try
        {
            result.m_diagnostic = e.GetMessageString() ? e.GetMessageString() : "OCCT import failure";
        }
        catch( ... )
        {
        }
    }
    catch( const std::exception& e )
    {
        Destroy3DModel( &result.m_model );
        result.m_error = MODEL_IMPORT_ERROR::IMPORT_FAILED;

        try
        {
            result.m_diagnostic = e.what();
        }
        catch( ... )
        {
        }
    }
    catch( ... )
    {
        Destroy3DModel( &result.m_model );
        result.m_error = MODEL_IMPORT_ERROR::INTERNAL;
    }

    return result;
}

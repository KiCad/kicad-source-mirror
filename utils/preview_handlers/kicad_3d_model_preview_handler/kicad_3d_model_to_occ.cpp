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

#include "kicad_3d_model_to_occ.h"

#include <plugins/3dapi/c3dmodel.h>

#include <AIS_Triangulation.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Triangle.hxx>
#include <Quantity_Color.hxx>
#include <TColStd_HArray1OfInteger.hxx>
#include <gp_Dir.hxx>
#include <gp_Pnt.hxx>

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace
{
Standard_Integer PackColor( const SFVEC3F& aColor )
{
    auto component = []( float aValue )
    {
        return static_cast<std::uint32_t>( std::lround( std::clamp( aValue, 0.0f, 1.0f ) * 255.0f ) );
    };

    // Transparency is carried by the presentation, so the packed alpha is always opaque.
    return static_cast<Standard_Integer>( component( aColor.r ) | component( aColor.g ) << 8
                                          | component( aColor.b ) << 16 | 0xFFu << 24 );
}
} // namespace


bool DisplayS3DModel( const S3DMODEL& aModel, const Handle( AIS_InteractiveContext ) & aContext )
{
    if( aContext.IsNull() || !aModel.m_Meshes )
        return false;

    bool displayed = false;

    for( unsigned int meshIndex = 0; meshIndex < aModel.m_MeshesSize; ++meshIndex )
    {
        const SMESH& mesh = aModel.m_Meshes[meshIndex];

        if( !mesh.m_Positions || !mesh.m_FaceIdx || mesh.m_VertexSize == 0 )
            continue;

        Standard_Integer triangleCount = 0;

        for( unsigned int offset = 0; offset + 2 < mesh.m_FaceIdxSize; offset += 3 )
        {
            if( IsTriangleInRange( mesh.m_FaceIdx, offset, mesh.m_VertexSize ) )
                ++triangleCount;
        }

        if( triangleCount == 0 )
            continue;

        const bool hasNormals = mesh.m_Normals != nullptr;
        Handle( Poly_Triangulation ) triangulation = new Poly_Triangulation(
                static_cast<Standard_Integer>( mesh.m_VertexSize ), triangleCount, false, hasNormals );

        bool validPositions = true;

        for( unsigned int vertex = 0; vertex < mesh.m_VertexSize; ++vertex )
        {
            const SFVEC3F& position = mesh.m_Positions[vertex];

            if( !std::isfinite( position.x ) || !std::isfinite( position.y ) || !std::isfinite( position.z ) )
            {
                validPositions = false;
                break;
            }

            triangulation->SetNode( static_cast<Standard_Integer>( vertex + 1 ),
                                    gp_Pnt( position.x, position.y, position.z ) );

            if( hasNormals )
            {
                const SFVEC3F& normal = mesh.m_Normals[vertex];
                const double   lengthSquared = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;

                if( lengthSquared > 0.0 && std::isfinite( lengthSquared ) )
                    triangulation->SetNormal( static_cast<Standard_Integer>( vertex + 1 ),
                                              gp_Dir( normal.x, normal.y, normal.z ) );
                else
                    triangulation->SetNormal( static_cast<Standard_Integer>( vertex + 1 ), gp_Dir( 0, 0, 1 ) );
            }
        }

        if( !validPositions )
            continue;

        Standard_Integer triangle = 1;

        for( unsigned int offset = 0; offset + 2 < mesh.m_FaceIdxSize; offset += 3 )
        {
            if( !IsTriangleInRange( mesh.m_FaceIdx, offset, mesh.m_VertexSize ) )
                continue;

            triangulation->SetTriangle(
                    triangle++, Poly_Triangle( static_cast<Standard_Integer>( mesh.m_FaceIdx[offset] + 1 ),
                                               static_cast<Standard_Integer>( mesh.m_FaceIdx[offset + 1] + 1 ),
                                               static_cast<Standard_Integer>( mesh.m_FaceIdx[offset + 2] + 1 ) ) );
        }

        Handle( AIS_Triangulation ) presentation = new AIS_Triangulation( triangulation );
        const SMATERIAL* material = mesh.m_MaterialIdx < aModel.m_MaterialsSize && aModel.m_Materials
                                            ? &aModel.m_Materials[mesh.m_MaterialIdx]
                                            : nullptr;
        const SFVEC3F    fallbackColor = material ? material->m_Diffuse : SFVEC3F( 0.7f );
        const float      transparency = material ? std::clamp( material->m_Transparency, 0.0f, 1.0f ) : 0.0f;

        // A per-vertex colour array is a whole extra VBO attribute, so only build one when the
        // mesh actually carries per-vertex colours.
        if( mesh.m_Color )
        {
            Handle( TColStd_HArray1OfInteger ) colors =
                    new TColStd_HArray1OfInteger( 1, static_cast<Standard_Integer>( mesh.m_VertexSize ) );

            for( unsigned int vertex = 0; vertex < mesh.m_VertexSize; ++vertex )
                colors->SetValue( static_cast<Standard_Integer>( vertex + 1 ), PackColor( mesh.m_Color[vertex] ) );

            presentation->SetColors( colors );
        }
        else
        {
            presentation->SetColor( Quantity_Color( fallbackColor.x, fallbackColor.y, fallbackColor.z,
                                                    Quantity_TOC_sRGB ) );
        }

        if( transparency > 0.0f )
            presentation->SetTransparency( transparency );

        aContext->Display( presentation, 0, -1, false );
        displayed = true;
    }

    return displayed;
}

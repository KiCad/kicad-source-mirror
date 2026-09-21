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
#include <Graphic3d_MaterialAspect.hxx>
#include <Poly_Triangulation.hxx>
#include <Poly_Triangle.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_ShadingAspect.hxx>
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


Quantity_Color ToQuantityColor( const SFVEC3F& aColor )
{
    return Quantity_Color( std::clamp( aColor.r, 0.0f, 1.0f ), std::clamp( aColor.g, 0.0f, 1.0f ),
                           std::clamp( aColor.b, 0.0f, 1.0f ), Quantity_TOC_sRGB );
}


/**
 * Give @p aPresentation its own shading aspect carrying @p aMaterial.
 *
 * AIS_Triangulation inherits PrsMgr_PresentableObject::SetColor(), which only records a highlight
 * colour and never reaches the shading aspect, so a triangulation coloured that way still renders
 * in OpenCascade's default brass material.  Driving the drawer is the only way to colour one.
 */
void ApplyMaterial( const Handle( AIS_Triangulation ) & aPresentation, const SMATERIAL* aMaterial,
                    bool aHasVertexColors )
{
    const Handle( Prs3d_Drawer )& drawer = aPresentation->Attributes();

    drawer->SetupOwnShadingAspect();

    Graphic3d_MaterialAspect occMaterial( Graphic3d_NameOfMaterial_UserDefined );

    if( aMaterial )
    {
        // Per-vertex colours supply the ambient and diffuse terms, so a white diffuse keeps the
        // material from tinting them
        occMaterial.SetAmbientColor( ToQuantityColor( aMaterial->m_Ambient ) );
        occMaterial.SetDiffuseColor( aHasVertexColors ? Quantity_Color( Quantity_NOC_WHITE )
                                                      : ToQuantityColor( aMaterial->m_Diffuse ) );
        occMaterial.SetSpecularColor( ToQuantityColor( aMaterial->m_Specular ) );
        occMaterial.SetEmissiveColor( ToQuantityColor( aMaterial->m_Emissive ) );
        occMaterial.SetShininess( std::clamp( aMaterial->m_Shininess, 0.0f, 1.0f ) );
    }
    else
    {
        occMaterial.SetDiffuseColor( ToQuantityColor( SFVEC3F( 0.7f ) ) );
    }

    drawer->ShadingAspect()->SetMaterial( occMaterial );
}
} // namespace


Handle( AIS_Triangulation ) BuildS3DMeshPresentation( const S3DMODEL& aModel, const SMESH& aMesh )
{
    if( !aMesh.m_Positions || !aMesh.m_FaceIdx || aMesh.m_VertexSize == 0 )
        return {};

    Standard_Integer triangleCount = 0;

    for( unsigned int offset = 0; offset + 2 < aMesh.m_FaceIdxSize; offset += 3 )
    {
        if( IsTriangleInRange( aMesh.m_FaceIdx, offset, aMesh.m_VertexSize ) )
            ++triangleCount;
    }

    if( triangleCount == 0 )
        return {};

    const bool hasNormals = aMesh.m_Normals != nullptr;
    Handle( Poly_Triangulation ) triangulation = new Poly_Triangulation(
            static_cast<Standard_Integer>( aMesh.m_VertexSize ), triangleCount, false, hasNormals );

    for( unsigned int vertex = 0; vertex < aMesh.m_VertexSize; ++vertex )
    {
        const SFVEC3F& position = aMesh.m_Positions[vertex];

        if( !std::isfinite( position.x ) || !std::isfinite( position.y ) || !std::isfinite( position.z ) )
            return {};

        triangulation->SetNode( static_cast<Standard_Integer>( vertex + 1 ),
                                gp_Pnt( position.x, position.y, position.z ) );

        if( hasNormals )
        {
            const SFVEC3F& normal = aMesh.m_Normals[vertex];
            const double   lengthSquared = normal.x * normal.x + normal.y * normal.y + normal.z * normal.z;

            if( lengthSquared > 0.0 && std::isfinite( lengthSquared ) )
                triangulation->SetNormal( static_cast<Standard_Integer>( vertex + 1 ),
                                          gp_Dir( normal.x, normal.y, normal.z ) );
            else
                triangulation->SetNormal( static_cast<Standard_Integer>( vertex + 1 ), gp_Dir( 0, 0, 1 ) );
        }
    }

    Standard_Integer triangle = 1;

    for( unsigned int offset = 0; offset + 2 < aMesh.m_FaceIdxSize; offset += 3 )
    {
        if( !IsTriangleInRange( aMesh.m_FaceIdx, offset, aMesh.m_VertexSize ) )
            continue;

        triangulation->SetTriangle(
                triangle++, Poly_Triangle( static_cast<Standard_Integer>( aMesh.m_FaceIdx[offset] + 1 ),
                                           static_cast<Standard_Integer>( aMesh.m_FaceIdx[offset + 1] + 1 ),
                                           static_cast<Standard_Integer>( aMesh.m_FaceIdx[offset + 2] + 1 ) ) );
    }

    Handle( AIS_Triangulation ) presentation = new AIS_Triangulation( triangulation );
    const SMATERIAL* material = aMesh.m_MaterialIdx < aModel.m_MaterialsSize && aModel.m_Materials
                                        ? &aModel.m_Materials[aMesh.m_MaterialIdx]
                                        : nullptr;
    const float      transparency = material ? std::clamp( material->m_Transparency, 0.0f, 1.0f ) : 0.0f;

    ApplyMaterial( presentation, material, aMesh.m_Color != nullptr );

    // A per-vertex colour array is a whole extra VBO attribute, so only build one when the
    // mesh actually carries per-vertex colours.
    if( aMesh.m_Color )
    {
        Handle( TColStd_HArray1OfInteger ) colors =
                new TColStd_HArray1OfInteger( 1, static_cast<Standard_Integer>( aMesh.m_VertexSize ) );

        for( unsigned int vertex = 0; vertex < aMesh.m_VertexSize; ++vertex )
            colors->SetValue( static_cast<Standard_Integer>( vertex + 1 ), PackColor( aMesh.m_Color[vertex] ) );

        presentation->SetColors( colors );
    }

    if( transparency > 0.0f )
        presentation->SetTransparency( transparency );

    return presentation;
}


bool DisplayS3DModel( const S3DMODEL& aModel, const Handle( AIS_InteractiveContext ) & aContext )
{
    if( aContext.IsNull() || !aModel.m_Meshes )
        return false;

    bool displayed = false;

    for( unsigned int meshIndex = 0; meshIndex < aModel.m_MeshesSize; ++meshIndex )
    {
        Handle( AIS_Triangulation ) presentation = BuildS3DMeshPresentation( aModel, aModel.m_Meshes[meshIndex] );

        if( presentation.IsNull() )
            continue;

        aContext->Display( presentation, 0, -1, false );
        displayed = true;
    }

    return displayed;
}

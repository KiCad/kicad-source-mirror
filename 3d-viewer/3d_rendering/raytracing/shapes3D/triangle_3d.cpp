/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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


/**
 * @file triangle_3d.cpp
 */


#include "triangle_3d.h"


void TRIANGLE::pre_calc_const()
{
    const SFVEC3F& A = m_vertex[0];
    const SFVEC3F& B = m_vertex[1];
    const SFVEC3F& C = m_vertex[2];
    const SFVEC3F c = B - A;
    const SFVEC3F b = C - A;

    m_bbox.Reset();
    m_bbox.Set( A );
    m_bbox.Union( B );
    m_bbox.Union( C );
    m_bbox.ScaleNextUp();
    m_centroid = m_bbox.GetCenter();

    m_n = glm::cross( b, c );

    if( glm::abs( m_n.x ) > glm::abs( m_n.y ) )
    {
        if( glm::abs( m_n.x ) > glm::abs( m_n.z ) )
            m_k = 0;
        else
            m_k = 2;
    }
    else
    {
        if( glm::abs( m_n.y ) > glm::abs( m_n.z ) )
            m_k = 1;
        else
            m_k = 2;
    }

    int u = ( m_k + 1 ) % 3;
    int v = ( m_k + 2 ) % 3;

    // precomp
    float krec = 1.0f / m_n[m_k];

    m_nu = m_n[u] * krec;
    m_nv = m_n[v] * krec;
    m_nd = glm::dot( m_n, A ) * krec;

    // first line equation
    float reci = 1.0f / ( b[u] * c[v] - b[v] * c[u] );

    m_bnu =  b[u] * reci;
    m_bnv = -b[v] * reci;

    // second line equation
    m_cnu =  c[v] * reci;
    m_cnv = -c[u] * reci;

    // finalize normal
    m_n = glm::normalize( m_n );

    m_normal[0] = m_n;
    m_normal[1] = m_n;
    m_normal[2] = m_n;
}


TRIANGLE::TRIANGLE( const SFVEC3F& aV1, const SFVEC3F& aV2, const SFVEC3F& aV3 )
        : OBJECT_3D( OBJECT_3D_TYPE::TRIANGLE )
{
    m_vertex[0] = aV1;
    m_vertex[1] = aV2;
    m_vertex[2] = aV3;

    m_vertexColorRGBA[0] = 0xFFFFFFFF;
    m_vertexColorRGBA[1] = 0xFFFFFFFF;
    m_vertexColorRGBA[2] = 0xFFFFFFFF;

    pre_calc_const();
}


TRIANGLE::TRIANGLE( const SFVEC3F& aV1, const SFVEC3F& aV2, const SFVEC3F& aV3,
                    const SFVEC3F& aFaceNormal )
        : OBJECT_3D( OBJECT_3D_TYPE::TRIANGLE )
{
    m_vertex[0] = aV1;
    m_vertex[1] = aV2;
    m_vertex[2] = aV3;

    m_vertexColorRGBA[0] = 0xFFFFFFFF;
    m_vertexColorRGBA[1] = 0xFFFFFFFF;
    m_vertexColorRGBA[2] = 0xFFFFFFFF;

    pre_calc_const();

    m_normal[0] = aFaceNormal;
    m_normal[1] = aFaceNormal;
    m_normal[2] = aFaceNormal;
}


TRIANGLE::TRIANGLE( const SFVEC3F& aV1, const SFVEC3F& aV2, const SFVEC3F& aV3,
                    const SFVEC3F& aN1, const SFVEC3F& aN2, const SFVEC3F& aN3 )
        : OBJECT_3D( OBJECT_3D_TYPE::TRIANGLE )
{
    m_vertex[0] = aV1;
    m_vertex[1] = aV2;
    m_vertex[2] = aV3;

    m_vertexColorRGBA[0] = 0xFFFFFFFF;
    m_vertexColorRGBA[1] = 0xFFFFFFFF;
    m_vertexColorRGBA[2] = 0xFFFFFFFF;

    pre_calc_const();

    m_normal[0] = aN1;
    m_normal[1] = aN2;
    m_normal[2] = aN3;
}


void TRIANGLE::SetColor( const SFVEC3F& aColor )
{
    m_vertexColorRGBA[0] = ( (unsigned int) ( aColor.r * 255 ) << 24 )
                           | ( (unsigned int) ( aColor.g * 255 ) << 16 )
                           | ( (unsigned int) ( aColor.b * 255 ) << 8 ) | 0xFF;
    m_vertexColorRGBA[1] = m_vertexColorRGBA[0];
    m_vertexColorRGBA[2] = m_vertexColorRGBA[0];
}


void TRIANGLE::SetColor( const SFVEC3F& aVC0, const SFVEC3F& aVC1, const SFVEC3F& aVC2 )
{
    m_vertexColorRGBA[0] = ( (unsigned int) ( aVC0.r * 255 ) << 24 )
                           | ( (unsigned int) ( aVC0.g * 255 ) << 16 )
                           | ( (unsigned int) ( aVC0.b * 255 ) << 8 ) | 0xFF;
    m_vertexColorRGBA[1] = ( (unsigned int) ( aVC1.r * 255 ) << 24 )
                           | ( (unsigned int) ( aVC1.g * 255 ) << 16 )
                           | ( (unsigned int) ( aVC1.b * 255 ) << 8 ) | 0xFF;
    m_vertexColorRGBA[2] = ( (unsigned int) ( aVC2.r * 255 ) << 24 )
                           | ( (unsigned int) ( aVC2.g * 255 ) << 16 )
                           | ( (unsigned int) ( aVC2.b * 255 ) << 8 ) | 0xFF;
}


void TRIANGLE::SetColor( unsigned int aFaceColorRGBA )
{
    m_vertexColorRGBA[0] = aFaceColorRGBA;
    m_vertexColorRGBA[1] = aFaceColorRGBA;
    m_vertexColorRGBA[2] = aFaceColorRGBA;
}


void TRIANGLE::SetColor( unsigned int aVertex1ColorRGBA, unsigned int aVertex2ColorRGBA,
                         unsigned int aVertex3ColorRGBA )
{
    m_vertexColorRGBA[0] = aVertex1ColorRGBA;
    m_vertexColorRGBA[1] = aVertex2ColorRGBA;
    m_vertexColorRGBA[2] = aVertex3ColorRGBA;
}


void TRIANGLE::SetUV( const SFVEC2F& aUV1, const SFVEC2F& aUV2, const SFVEC2F& aUV3 )
{
    m_uv[0] = aUV1;
    m_uv[1] = aUV2;
    m_uv[2] = aUV3;
}


static const unsigned int s_modulo[] = { 0, 1, 2, 0, 1 };


bool TRIANGLE::Intersect( const RAY& aRay, HITINFO& aHitInfo ) const
{
    //!TODO: precalc this, improve it
#define ku s_modulo[m_k + 1]
#define kv s_modulo[m_k + 2]

    const SFVEC3F& O = aRay.m_Origin;
    const SFVEC3F& D = aRay.m_Dir;
    const SFVEC3F& A = m_vertex[0];

    const float lnd = 1.0f / ( D[m_k] + m_nu * D[ku] + m_nv * D[kv] );
    const float t = ( m_nd - O[m_k] - m_nu * O[ku] - m_nv * O[kv] ) * lnd;

    if( !( ( aHitInfo.m_tHit > t ) && ( t > 0.0f ) ) )
        return false;

    const float hu = O[ku] + t * D[ku] - A[ku];
    const float hv = O[kv] + t * D[kv] - A[kv];
    const float beta = hv * m_bnu + hu * m_bnv;

    if( beta < 0.0f )
        return false;

    const float gamma = hu * m_cnu + hv * m_cnv;

    if( gamma < 0 )
        return false;

    const float v = gamma;
    const float u = beta;

    if( ( u + v ) > 1.0f )
        return false;

    if( glm::dot( D, m_n ) > 0.0f )
        return false;

    aHitInfo.m_tHit = t;
    aHitInfo.m_HitPoint = aRay.at( t );

    // interpolate vertex normals with UVW using Gouraud's shading
    aHitInfo.m_HitNormal =
            glm::normalize( ( 1.0f - u - v ) * m_normal[0] + u * m_normal[1] + v * m_normal[2] );

    m_material->Generate( aHitInfo.m_HitNormal, aRay, aHitInfo );

    aHitInfo.pHitObject = this;

    return true;
#undef ku
#undef kv
}


bool TRIANGLE::IntersectP( const RAY& aRay, float aMaxDistance ) const
{
    //!TODO: precalc this
#define ku s_modulo[m_k + 1]
#define kv s_modulo[m_k + 2]

    const SFVEC3F O = aRay.m_Origin;
    const SFVEC3F D = aRay.m_Dir;
    const SFVEC3F A = m_vertex[0];

    const float lnd = 1.0f / ( D[m_k] + m_nu * D[ku] + m_nv * D[kv] );
    const float t = ( m_nd - O[m_k] - m_nu * O[ku] - m_nv * O[kv] ) * lnd;

    if( !( ( aMaxDistance > t ) && ( t > 0.0f ) ) )
        return false;

    const float hu = O[ku] + t * D[ku] - A[ku];
    const float hv = O[kv] + t * D[kv] - A[kv];
    const float beta = hv * m_bnu + hu * m_bnv;

    if( beta < 0.0f )
        return false;

    const float gamma = hu * m_cnu + hv * m_cnv;

    if( gamma < 0.0f )
        return false;

    const float v = gamma;
    const float u = beta;

    if( ( u + v ) > 1.0f )
        return false;

    if( glm::dot( D, m_n ) > 0.0f )
        return false;

    return true;
#undef ku
#undef kv
}


bool TRIANGLE::Intersects( const BBOX_3D& aBBox ) const
{
    //!TODO: improve
    return m_bbox.Intersects( aBBox );
}


SFVEC3F TRIANGLE::GetDiffuseColor( const HITINFO& aHitInfo ) const
{
    const unsigned int rgbC1 = m_vertexColorRGBA[0];
    const unsigned int rgbC2 = m_vertexColorRGBA[1];
    const unsigned int rgbC3 = m_vertexColorRGBA[2];

    const SFVEC3F c1 = SFVEC3F( (float) ( ( rgbC1 >> 24 ) & 0xFF ) / 255.0f,
                                (float) ( ( rgbC1 >> 16 ) & 0xFF ) / 255.0f,
                                (float) ( ( rgbC1 >> 8 ) & 0xFF ) / 255.0f );
    const SFVEC3F c2 = SFVEC3F( (float) ( ( rgbC2 >> 24 ) & 0xFF ) / 255.0f,
                                (float) ( ( rgbC2 >> 16 ) & 0xFF ) / 255.0f,
                                (float) ( ( rgbC2 >> 8 ) & 0xFF ) / 255.0f );
    const SFVEC3F c3 = SFVEC3F( (float) ( ( rgbC3 >> 24 ) & 0xFF ) / 255.0f,
                                (float) ( ( rgbC3 >> 16 ) & 0xFF ) / 255.0f,
                                (float) ( ( rgbC3 >> 8 ) & 0xFF ) / 255.0f );

    const float u = aHitInfo.m_UV.x;
    const float v = aHitInfo.m_UV.y;
    const float w = 1.0f - u - v;

    return w * c1 + u * c2 + v * c3;
}

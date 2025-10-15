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

#include "layer_triangles.h"
#include "../raytracing/ray.h"
#include <wx/debug.h>   // For the wxASSERT
#include <mutex>


TRIANGLE_LIST::TRIANGLE_LIST( unsigned int aNrReservedTriangles, bool aReserveNormals )
{
    m_vertexs.clear();
    m_normals.clear();

    if( aNrReservedTriangles > 0 )
        m_vertexs.reserve( aNrReservedTriangles * 3 );

    if( aReserveNormals )
        m_normals.reserve( aNrReservedTriangles * 3 );
}


void TRIANGLE_LIST::Reserve_More( unsigned int aNrReservedTriangles, bool aReserveNormals )
{
    m_vertexs.reserve( m_vertexs.size() + aNrReservedTriangles * 3 );

    if( aReserveNormals )
        m_normals.reserve( m_normals.size() + aNrReservedTriangles * 3 );
}


void TRIANGLE_LIST::AddQuad( const SFVEC3F& aV1, const SFVEC3F& aV2, const SFVEC3F& aV3,
                             const SFVEC3F& aV4 )
{
    m_vertexs.push_back( aV1 );
    m_vertexs.push_back( aV2 );
    m_vertexs.push_back( aV3 );

    m_vertexs.push_back( aV3 );
    m_vertexs.push_back( aV4 );
    m_vertexs.push_back( aV1 );
}


void TRIANGLE_LIST::AddTriangle( const SFVEC3F& aV1, const SFVEC3F& aV2, const SFVEC3F& aV3 )
{
    m_vertexs.push_back( aV1 );
    m_vertexs.push_back( aV2 );
    m_vertexs.push_back( aV3 );
}


void TRIANGLE_LIST::AddNormal( const SFVEC3F& aN1, const SFVEC3F& aN2, const SFVEC3F& aN3 )
{
    m_normals.push_back( aN1 );
    m_normals.push_back( aN2 );
    m_normals.push_back( aN3 );
}

void TRIANGLE_LIST::AddNormal( const SFVEC3F& aN1, const SFVEC3F& aN2, const SFVEC3F& aN3,
                               const SFVEC3F& aN4 )
{
    m_normals.push_back( aN1 );
    m_normals.push_back( aN2 );
    m_normals.push_back( aN3 );

    m_normals.push_back( aN3 );
    m_normals.push_back( aN4 );
    m_normals.push_back( aN1 );
}


TRIANGLE_DISPLAY_LIST::TRIANGLE_DISPLAY_LIST( unsigned int aNrReservedTriangles )
{
    m_layer_top_segment_ends       = new TRIANGLE_LIST( aNrReservedTriangles, false );
    m_layer_top_triangles          = new TRIANGLE_LIST( aNrReservedTriangles, false );
    m_layer_middle_contours_quads  = new TRIANGLE_LIST( aNrReservedTriangles, true  );
    m_layer_bot_triangles          = new TRIANGLE_LIST( aNrReservedTriangles, false );
    m_layer_bot_segment_ends       = new TRIANGLE_LIST( aNrReservedTriangles, false );
}


TRIANGLE_DISPLAY_LIST::~TRIANGLE_DISPLAY_LIST()
{
    delete m_layer_top_segment_ends;
    m_layer_top_segment_ends = nullptr;

    delete m_layer_top_triangles;
    m_layer_top_triangles = nullptr;

    delete m_layer_middle_contours_quads;
    m_layer_middle_contours_quads = nullptr;

    delete m_layer_bot_triangles;
    m_layer_bot_triangles = nullptr;

    delete m_layer_bot_segment_ends;
    m_layer_bot_segment_ends = nullptr;
}


void TRIANGLE_DISPLAY_LIST::AddToMiddleContours( const std::vector<SFVEC2F>& aContourPoints,
                                                 float zBot, float zTop, bool aInvertFaceDirection,
                                                 const BVH_CONTAINER_2D* aThroughHoles )
{
    if( aContourPoints.size() >= 4 )
    {
        // Calculate normals of each segment of the contour
        std::vector< SFVEC2F > contourNormals;

        contourNormals.clear();
        contourNormals.resize( aContourPoints.size() - 1 );

        if( aInvertFaceDirection )
        {
            for( unsigned int i = 0; i < ( aContourPoints.size() - 1 ); ++i )
            {
                const SFVEC2F& v0 = aContourPoints[i + 0];
                const SFVEC2F& v1 = aContourPoints[i + 1];
                const SFVEC2F n = glm::normalize( v1 - v0 );

                contourNormals[i] = SFVEC2F( n.y, -n.x );
            }
        }
        else
        {
            for( unsigned int i = 0; i < ( aContourPoints.size() - 1 ); ++i )
            {
                const SFVEC2F& v0 = aContourPoints[i + 0];
                const SFVEC2F& v1 = aContourPoints[i + 1];
                const SFVEC2F n = glm::normalize( v1 - v0 );

                contourNormals[i] = SFVEC2F( -n.y, n.x );
            }
        }


        if( aInvertFaceDirection )
            std::swap( zBot, zTop );

        const unsigned int nContoursToProcess = ( aContourPoints.size() - 1 );

        for( unsigned int i = 0; i < nContoursToProcess; ++i )
        {
            SFVEC2F lastNormal;

            if( i > 0 )
                lastNormal = contourNormals[i - 1];
            else
                lastNormal = contourNormals[nContoursToProcess - 1];

            SFVEC2F n0 = contourNormals[i];

            // Only interpolate the normal if the angle is closer
            if( glm::dot( n0, lastNormal ) > 0.5f )
                n0 = glm::normalize( n0 + lastNormal );

            SFVEC2F nextNormal;

            if( i < ( nContoursToProcess - 1) )
                nextNormal = contourNormals[i + 1];
            else
                nextNormal = contourNormals[0];

            SFVEC2F n1 = contourNormals[i];

            if( glm::dot( n1, nextNormal ) > 0.5f )
                n1 = glm::normalize( n1 + nextNormal );

            const SFVEC3F n3d0 = SFVEC3F( n0.x, n0.y, 0.0f );
            const SFVEC3F n3d1 = SFVEC3F( n1.x, n1.y, 0.0f );

            const SFVEC2F& v0 = aContourPoints[i + 0];
            const SFVEC2F& v1 = aContourPoints[i + 1];

            if( aThroughHoles && aThroughHoles->IntersectAny( RAYSEG2D( v0, v1 ) ) )
            {
                continue;
            }
            else
            {
                std::lock_guard<std::mutex> lock( m_middle_layer_lock );
                m_layer_middle_contours_quads->AddQuad( SFVEC3F( v0.x, v0.y, zTop ),
                                                        SFVEC3F( v1.x, v1.y, zTop ),
                                                        SFVEC3F( v1.x, v1.y, zBot ),
                                                        SFVEC3F( v0.x, v0.y, zBot ) );

                m_layer_middle_contours_quads->AddNormal( n3d0, n3d1, n3d1, n3d0 );
            }
        }
    }
}


void TRIANGLE_DISPLAY_LIST::AddToMiddleContours( const SHAPE_LINE_CHAIN& outlinePath, float zBot, float zTop,
                                                 double aBiuTo3Du, bool aInvertFaceDirection,
                                                 const BVH_CONTAINER_2D* aThroughHoles )
{
    std::vector<SFVEC2F> contourPoints;

    contourPoints.clear();
    contourPoints.reserve( outlinePath.PointCount() + 2 );

    const VECTOR2I& firstV = outlinePath.CPoint( 0 );

    SFVEC2F lastV = SFVEC2F( firstV.x * aBiuTo3Du, -firstV.y * aBiuTo3Du );

    contourPoints.push_back( lastV );

    for( unsigned int i = 1; i < (unsigned int)outlinePath.PointCount(); ++i )
    {
        const VECTOR2I&  v = outlinePath.CPoint( i );

        const SFVEC2F vf = SFVEC2F(  v.x * aBiuTo3Du, -v.y * aBiuTo3Du );

        if( vf != lastV ) // Do not add repeated points
        {
            lastV = vf;
            contourPoints.push_back( vf );
        }
    }

    // Add first position of the list to close the path.
    if( lastV != contourPoints[0] )
        contourPoints.push_back( contourPoints[0] );

    AddToMiddleContours( contourPoints, zBot, zTop, aInvertFaceDirection, aThroughHoles );
}


void TRIANGLE_DISPLAY_LIST::AddToMiddleContours( const SHAPE_POLY_SET& aPolySet, float zBot, float zTop,
                                                 double aBiuTo3Du, bool aInvertFaceDirection,
                                                 const BVH_CONTAINER_2D* aThroughHoles )
{
    if( aPolySet.OutlineCount() == 0 )
        return;

    // Calculate an estimation of points to reserve
    unsigned int nrContourPointsToReserve = 0;

    for( int i = 0; i < aPolySet.OutlineCount(); ++i )
    {
        const SHAPE_LINE_CHAIN& pathOutline = aPolySet.COutline( i );

        nrContourPointsToReserve += pathOutline.PointCount();

        for( int h = 0; h < aPolySet.HoleCount( i ); ++h )
        {
            const SHAPE_LINE_CHAIN& hole = aPolySet.CHole( i, h );

            nrContourPointsToReserve += hole.PointCount();
        }
    }

    // Request to reserve more space
    m_layer_middle_contours_quads->Reserve_More( nrContourPointsToReserve * 2, true );

    for( int i = 0; i < aPolySet.OutlineCount(); i++ )
    {
        // Add outline
        const SHAPE_LINE_CHAIN& pathOutline = aPolySet.COutline( i );

        AddToMiddleContours( pathOutline, zBot, zTop, aBiuTo3Du, aInvertFaceDirection, aThroughHoles );

        // Add holes for this outline
        for( int h = 0; h < aPolySet.HoleCount( i ); ++h )
        {
            const SHAPE_LINE_CHAIN& hole = aPolySet.CHole( i, h );
            AddToMiddleContours( hole, zBot, zTop, aBiuTo3Du, aInvertFaceDirection, aThroughHoles );
        }
    }
}


OPENGL_RENDER_LIST::OPENGL_RENDER_LIST( const TRIANGLE_DISPLAY_LIST& aLayerTriangles,
                                        GLuint aTextureIndexForSegEnds, float aZBot, float aZTop )
{
    m_zBot = aZBot;
    m_zTop = aZTop;

    m_layer_top_segment_ends       = 0;
    m_layer_top_triangles          = 0;
    m_layer_middle_contours_quads  = 0;
    m_layer_bot_triangles          = 0;
    m_layer_bot_segment_ends       = 0;

    if( aTextureIndexForSegEnds )
    {
        wxASSERT( glIsTexture( aTextureIndexForSegEnds ) );

        if( glIsTexture( aTextureIndexForSegEnds ) )
        {
            m_layer_top_segment_ends = generate_top_or_bot_seg_ends( aLayerTriangles.m_layer_top_segment_ends,
                                                                     true, aTextureIndexForSegEnds );

            m_layer_bot_segment_ends = generate_top_or_bot_seg_ends( aLayerTriangles.m_layer_bot_segment_ends,
                                                                     false, aTextureIndexForSegEnds );
        }
    }

    m_layer_top_triangles = generate_top_or_bot_triangles( aLayerTriangles.m_layer_top_triangles, true );

    m_layer_bot_triangles = generate_top_or_bot_triangles( aLayerTriangles.m_layer_bot_triangles, false );


    if( aLayerTriangles.m_layer_middle_contours_quads->GetVertexSize() > 0 )
        m_layer_middle_contours_quads = generate_middle_triangles( aLayerTriangles.m_layer_middle_contours_quads );

    m_draw_it_transparent = false;
    m_haveTransformation  = false;
    m_zPositionTransformation = 0.0f;
    m_zScaleTransformation    = 0.0f;
}


OPENGL_RENDER_LIST::~OPENGL_RENDER_LIST()
{
    if( glIsList( m_layer_top_segment_ends ) )
        glDeleteLists( m_layer_top_segment_ends, 1 );

    if( glIsList( m_layer_top_triangles ) )
        glDeleteLists( m_layer_top_triangles, 1 );

    if( glIsList( m_layer_middle_contours_quads ) )
        glDeleteLists( m_layer_middle_contours_quads, 1 );

    if( glIsList( m_layer_bot_triangles ) )
        glDeleteLists( m_layer_bot_triangles, 1 );

    if( glIsList( m_layer_bot_segment_ends ) )
        glDeleteLists( m_layer_bot_segment_ends, 1 );

    m_layer_top_segment_ends        = 0;
    m_layer_top_triangles           = 0;
    m_layer_middle_contours_quads  = 0;
    m_layer_bot_triangles           = 0;
    m_layer_bot_segment_ends        = 0;
}


void OPENGL_RENDER_LIST::DrawTopAndMiddle() const
{
    beginTransformation();

    if( glIsList( m_layer_middle_contours_quads ) )
        glCallList( m_layer_middle_contours_quads );

    if( glIsList( m_layer_top_triangles ) )
        glCallList( m_layer_top_triangles );

    if( glIsList( m_layer_top_segment_ends ) )
        glCallList( m_layer_top_segment_ends );

    endTransformation();
}


void OPENGL_RENDER_LIST::DrawBotAndMiddle() const
{
    beginTransformation();

    if( glIsList( m_layer_middle_contours_quads ) )
        glCallList( m_layer_middle_contours_quads );

    if( glIsList( m_layer_bot_triangles ) )
        glCallList( m_layer_bot_triangles );

    if( glIsList( m_layer_bot_segment_ends ) )
        glCallList( m_layer_bot_segment_ends );

    endTransformation();
}


void OPENGL_RENDER_LIST::DrawTop() const
{
    beginTransformation();

    if( glIsList( m_layer_top_triangles ) )
        glCallList( m_layer_top_triangles );

    if( glIsList( m_layer_top_segment_ends ) )
        glCallList( m_layer_top_segment_ends );

    endTransformation();
}


void OPENGL_RENDER_LIST::DrawBot() const
{
    beginTransformation();

    if( glIsList( m_layer_bot_triangles ) )
        glCallList( m_layer_bot_triangles );

    if( glIsList( m_layer_bot_segment_ends ) )
        glCallList( m_layer_bot_segment_ends );

    endTransformation();
}


void OPENGL_RENDER_LIST::DrawMiddle() const
{
    beginTransformation();

    if( glIsList( m_layer_middle_contours_quads ) )
        glCallList( m_layer_middle_contours_quads );

    endTransformation();
}


void OPENGL_RENDER_LIST::DrawAll( bool aDrawMiddle ) const
{
    beginTransformation();

    if( aDrawMiddle )
        if( glIsList( m_layer_middle_contours_quads ) )
            glCallList( m_layer_middle_contours_quads );

    if( glIsList( m_layer_top_triangles ) )
        glCallList( m_layer_top_triangles );

    if( glIsList( m_layer_bot_triangles ) )
        glCallList( m_layer_bot_triangles );

    if( glIsList( m_layer_top_segment_ends ) )
        glCallList( m_layer_top_segment_ends );

    if( glIsList( m_layer_bot_segment_ends ) )
        glCallList( m_layer_bot_segment_ends );

    endTransformation();
}


void OPENGL_RENDER_LIST::DrawCulled( bool aDrawMiddle,
                                     const OPENGL_RENDER_LIST* aSubtractList,
                                     const OPENGL_RENDER_LIST* bSubtractList,
                                     const OPENGL_RENDER_LIST* cSubtractList,
                                     const OPENGL_RENDER_LIST* dSubtractList ) const
{
    glClearStencil( 0x00 );
    glClear( GL_STENCIL_BUFFER_BIT );

    glEnable( GL_CULL_FACE );
    glCullFace( GL_BACK );

    glDisable( GL_DEPTH_TEST );
    glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
    glDepthMask( GL_FALSE );
    glEnable( GL_STENCIL_TEST );
    glStencilFunc( GL_ALWAYS, 1, 0 );
    glStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );

    if( aSubtractList )
        aSubtractList->DrawBot();

    if( bSubtractList )
        bSubtractList->DrawBot();

    if( cSubtractList )
        cSubtractList->DrawBot();

    if( dSubtractList )
        dSubtractList->DrawBot();

    glEnable( GL_DEPTH_TEST );
    glDepthMask( GL_TRUE );

    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
    glStencilFunc( GL_EQUAL, 0, 1 );
    glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
    DrawBot();

    glDisable( GL_DEPTH_TEST );
    glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
    glDepthMask( GL_FALSE );
    glEnable( GL_STENCIL_TEST );
    glStencilFunc( GL_ALWAYS, 2, 0 );
    glStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );

    if( aSubtractList )
        aSubtractList->DrawTop();

    if( bSubtractList )
        bSubtractList->DrawTop();

    if( cSubtractList )
        cSubtractList->DrawTop();

    if( dSubtractList )
        dSubtractList->DrawTop();

    glEnable( GL_DEPTH_TEST );
    glDepthMask( GL_TRUE );
    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
    glStencilFunc( GL_NOTEQUAL, 2, 0x03 );
    glStencilOp( GL_KEEP, GL_KEEP, GL_INCR );
    DrawTop();

    if( aDrawMiddle )
        DrawMiddle();

    glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE );

    glCullFace( GL_FRONT );
    glStencilFunc( GL_GEQUAL, 3, 0x03 );
    glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
    glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

    if( aDrawMiddle )
    {
        if( aSubtractList )
            aSubtractList->DrawMiddle();
    }

    glLightModeli( GL_LIGHT_MODEL_TWO_SIDE, GL_FALSE );

    glCullFace( GL_BACK );
    glDisable( GL_STENCIL_TEST );
}


void OPENGL_RENDER_LIST::ApplyScalePosition( float aZposition, float aZscale )
{
    wxCHECK2( aZscale > FLT_EPSILON, aZscale = FLT_EPSILON + 1 );

    m_zPositionTransformation = aZposition;
    m_zScaleTransformation = aZscale;
    m_haveTransformation = true;
}


void OPENGL_RENDER_LIST::ApplyScalePosition( OPENGL_RENDER_LIST* aOtherList )
{
    ApplyScalePosition( aOtherList->GetZBot(), aOtherList->GetZTop() - aOtherList->GetZBot() );
}


void OPENGL_RENDER_LIST::SetItIsTransparent( bool aSetTransparent )
{
    m_draw_it_transparent = aSetTransparent;
}


GLuint OPENGL_RENDER_LIST::generate_top_or_bot_seg_ends(
        const TRIANGLE_LIST* aTriangleContainer, bool aIsNormalUp, GLuint aTextureId ) const
{
    wxCHECK( aTriangleContainer != nullptr, 0 );

    wxASSERT( ( aTriangleContainer->GetVertexSize() % 3 ) == 0 );

    // Top and Bot don't have normals array stored in container
    wxASSERT( aTriangleContainer->GetNormalsSize() == 0 );

    if( ( aTriangleContainer->GetVertexSize() > 0 )
      && ( ( aTriangleContainer->GetVertexSize() % 3 ) == 0 ) )
    {
        GLuint listIdx = glGenLists( 1 );

        if( glIsList( listIdx ) )
        {
            // Prepare an array of UV text coordinates
            SFVEC2F* uvArray = new SFVEC2F[aTriangleContainer->GetVertexSize()];

            for( unsigned int i = 0; i < aTriangleContainer->GetVertexSize(); i += 3 )
            {
                uvArray[i + 0] = SFVEC2F( 1.0f, 0.0f );
                uvArray[i + 1] = SFVEC2F( 0.0f, 1.0f );
                uvArray[i + 2] = SFVEC2F( 0.0f, 0.0f );
            }

            glEnableClientState( GL_TEXTURE_COORD_ARRAY );
            glDisableClientState( GL_COLOR_ARRAY );
            glDisableClientState( GL_NORMAL_ARRAY );
            glEnableClientState( GL_VERTEX_ARRAY );
            glVertexPointer( 3, GL_FLOAT, 0, aTriangleContainer->GetVertexPointer() );
            glTexCoordPointer( 2, GL_FLOAT, 0, uvArray );

            glNewList( listIdx, GL_COMPILE );

            glDisable( GL_COLOR_MATERIAL );

            glEnable( GL_TEXTURE_2D );
            glBindTexture( GL_TEXTURE_2D, aTextureId );

            glAlphaFunc( GL_GREATER, 0.2f );
            glEnable( GL_ALPHA_TEST );

            glNormal3f( 0.0f, 0.0f, aIsNormalUp?1.0f:-1.0f );

            glDrawArrays( GL_TRIANGLES, 0, aTriangleContainer->GetVertexSize() );

            glBindTexture( GL_TEXTURE_2D, 0 );
            glDisable( GL_TEXTURE_2D );
            glDisable( GL_ALPHA_TEST );
            glDisable( GL_BLEND );

            glEndList();

            glDisableClientState( GL_VERTEX_ARRAY );
            glDisableClientState( GL_TEXTURE_COORD_ARRAY );

            delete [] uvArray;
            return listIdx;
        }
    }

    return 0;
}


GLuint OPENGL_RENDER_LIST::generate_top_or_bot_triangles( const TRIANGLE_LIST* aTriangleContainer,
                                                          bool aIsNormalUp ) const
{
    wxCHECK( aTriangleContainer != nullptr, 0 );

    wxASSERT( ( aTriangleContainer->GetVertexSize() % 3 ) == 0 );

    // Top and Bot dint have normals array stored in container
    wxASSERT( aTriangleContainer->GetNormalsSize() == 0 );

    if( ( aTriangleContainer->GetVertexSize() > 0 )
      && ( ( aTriangleContainer->GetVertexSize() % 3 ) == 0 ) )
    {
        const GLuint listIdx = glGenLists( 1 );

        if( glIsList( listIdx ) )
        {
            glDisableClientState( GL_TEXTURE_COORD_ARRAY );
            glDisableClientState( GL_COLOR_ARRAY );
            glDisableClientState( GL_NORMAL_ARRAY );
            glEnableClientState( GL_VERTEX_ARRAY );
            glVertexPointer( 3, GL_FLOAT, 0, aTriangleContainer->GetVertexPointer() );

            glNewList( listIdx, GL_COMPILE );

            setBlendfunction();

            glNormal3f( 0.0f, 0.0f, aIsNormalUp?1.0f:-1.0f );

            glDrawArrays( GL_TRIANGLES, 0, aTriangleContainer->GetVertexSize() );

            glDisable( GL_BLEND );
            glEndList();

            glDisableClientState( GL_VERTEX_ARRAY );

            return listIdx;
        }
    }

    return 0;
}


GLuint OPENGL_RENDER_LIST::generate_middle_triangles(
        const TRIANGLE_LIST* aTriangleContainer ) const
{
    wxCHECK( aTriangleContainer != nullptr, 0 );

    // We expect that it is a multiple of 3 vertex
    wxASSERT( ( aTriangleContainer->GetVertexSize() % 3 ) == 0 );

    // We expect that it is a multiple of 6 vertex (because we expect to add quads)
    wxASSERT( (aTriangleContainer->GetVertexSize() % 6 ) == 0 );

    // We expect that there are normals with same size as vertex
    wxASSERT( aTriangleContainer->GetNormalsSize() == aTriangleContainer->GetVertexSize() );

    if( ( aTriangleContainer->GetVertexSize() > 0 )
      && ( ( aTriangleContainer->GetVertexSize() % 3 ) == 0 )
      && ( ( aTriangleContainer->GetVertexSize() % 6 ) == 0 )
      && ( aTriangleContainer->GetNormalsSize() == aTriangleContainer->GetVertexSize() ) )
    {
        const GLuint listIdx = glGenLists( 1 );

        if( glIsList( listIdx ) )
        {
            glDisableClientState( GL_TEXTURE_COORD_ARRAY );
            glDisableClientState( GL_COLOR_ARRAY );
            glEnableClientState( GL_NORMAL_ARRAY );
            glEnableClientState( GL_VERTEX_ARRAY );
            glVertexPointer( 3, GL_FLOAT, 0, aTriangleContainer->GetVertexPointer() );
            glNormalPointer( GL_FLOAT, 0, aTriangleContainer->GetNormalsPointer() );

            glNewList( listIdx, GL_COMPILE );

            setBlendfunction();

            glDrawArrays( GL_TRIANGLES, 0, aTriangleContainer->GetVertexSize() );

            glDisable( GL_BLEND );
            glEndList();

            glDisableClientState( GL_VERTEX_ARRAY );
            glDisableClientState( GL_NORMAL_ARRAY );

            return listIdx;
        }
    }

    return 0;
}


void OPENGL_RENDER_LIST::endTransformation() const
{
    if( m_haveTransformation )
    {
        glPopMatrix();
    }
}


void OPENGL_RENDER_LIST::setBlendfunction() const
{
    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
}


void OPENGL_RENDER_LIST::beginTransformation() const
{
    if( m_haveTransformation )
    {
        glPushMatrix();
        glTranslatef( 0.0f, 0.0f, m_zPositionTransformation );
        glScalef( 1.0f, 1.0f, m_zScaleTransformation );
    }
}

//  LocalWords:  EDA Luzeiro txt MERCHANTABILITY wxASSERT normals fo

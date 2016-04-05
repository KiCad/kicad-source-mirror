/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  clayer_triangles.cpp
 * @brief
 */


#include "clayer_triangles.h"
#include <wx/debug.h>                                                           // For the wxASSERT


CLAYER_TRIANGLE_CONTAINER::CLAYER_TRIANGLE_CONTAINER(unsigned int aNrReservedTriangles, bool aReserveNormals)
{
    wxASSERT( aNrReservedTriangles > 0 );

    m_vertexs.clear();
    m_normals.clear();

    m_vertexs.reserve( aNrReservedTriangles * 3 );

    if( aReserveNormals )
        m_normals.reserve( aNrReservedTriangles * 3 );
}


void CLAYER_TRIANGLE_CONTAINER::AddQuad( const SFVEC3F &aV1, const SFVEC3F &aV2, const SFVEC3F &aV3, const SFVEC3F &aV4 )
{
    m_vertexs.push_back( aV1 );
    m_vertexs.push_back( aV2 );
    m_vertexs.push_back( aV3 );

    m_vertexs.push_back( aV3 );
    m_vertexs.push_back( aV4 );
    m_vertexs.push_back( aV1 );
}


void CLAYER_TRIANGLE_CONTAINER::AddTriangle( const SFVEC3F &aV1, const SFVEC3F &aV2, const SFVEC3F &aV3 )
{
    m_vertexs.push_back( aV1 );
    m_vertexs.push_back( aV2 );
    m_vertexs.push_back( aV3 );
}


void CLAYER_TRIANGLE_CONTAINER::AddNormal( const SFVEC3F &aN1, const SFVEC3F &aN2, const SFVEC3F &aN3 )
{
    m_normals.push_back( aN1 );
    m_normals.push_back( aN2 );
    m_normals.push_back( aN3 );
}

void CLAYER_TRIANGLE_CONTAINER::AddNormal( const SFVEC3F &aN1, const SFVEC3F &aN2, const SFVEC3F &aN3, const SFVEC3F &aN4 )
{
    m_normals.push_back( aN1 );
    m_normals.push_back( aN2 );
    m_normals.push_back( aN3 );

    m_normals.push_back( aN3 );
    m_normals.push_back( aN4 );
    m_normals.push_back( aN1 );
}


CLAYER_TRIANGLES::CLAYER_TRIANGLES( unsigned int aNrReservedTriangles )
{
    wxASSERT( aNrReservedTriangles > 0 );

    m_layer_top_segment_ends            = new CLAYER_TRIANGLE_CONTAINER( aNrReservedTriangles, false );
    m_layer_top_triangles               = new CLAYER_TRIANGLE_CONTAINER( aNrReservedTriangles, false );
    m_layer_middle_contourns_quads      = new CLAYER_TRIANGLE_CONTAINER( aNrReservedTriangles, true  );
    m_layer_bot_triangles               = new CLAYER_TRIANGLE_CONTAINER( aNrReservedTriangles, false );
    m_layer_bot_segment_ends            = new CLAYER_TRIANGLE_CONTAINER( aNrReservedTriangles, false );
}


CLAYER_TRIANGLES::~CLAYER_TRIANGLES()
{
    delete m_layer_top_segment_ends;
    m_layer_top_segment_ends = 0;

    delete m_layer_top_triangles;
    m_layer_top_triangles = 0;

    delete m_layer_middle_contourns_quads;
    m_layer_middle_contourns_quads = 0;

    delete m_layer_bot_triangles;
    m_layer_bot_triangles = 0;

    delete m_layer_bot_segment_ends;
    m_layer_bot_segment_ends = 0;
}


CLAYERS_OGL_DISP_LISTS::CLAYERS_OGL_DISP_LISTS(const CLAYER_TRIANGLES &aLayerTriangles,
                                               GLuint aTextureIndexForSegEnds,
                                               const SFVEC3F& aLayerColor )
{
    wxASSERT( glIsTexture( aTextureIndexForSegEnds ) );

    m_layer_top_segment_ends        = 0;
    m_layer_top_triangles           = 0;
    m_layer_middle_contourns_quads  = 0;
    m_layer_bot_triangles           = 0;
    m_layer_bot_segment_ends        = 0;

    m_layer_top_segment_ends = generate_top_or_bot_seg_ends(  aLayerTriangles.m_layer_top_segment_ends, aLayerColor, true, aTextureIndexForSegEnds );
    m_layer_top_triangles    = generate_top_or_bot_triangles( aLayerTriangles.m_layer_top_triangles,    aLayerColor, true  );
    m_layer_bot_triangles    = generate_top_or_bot_triangles( aLayerTriangles.m_layer_bot_triangles,    aLayerColor, false );
    m_layer_bot_segment_ends = generate_top_or_bot_seg_ends(  aLayerTriangles.m_layer_bot_segment_ends, aLayerColor, false, aTextureIndexForSegEnds );

    m_layer_middle_contourns_quads = generate_middle_triangles( aLayerTriangles.m_layer_middle_contourns_quads, aLayerColor );
}


CLAYERS_OGL_DISP_LISTS::~CLAYERS_OGL_DISP_LISTS()
{
    if( glIsList( m_layer_top_segment_ends ) )
        glDeleteLists( m_layer_top_segment_ends, 1 );

    if( glIsList( m_layer_top_triangles ) )
        glDeleteLists( m_layer_top_triangles, 1 );

    if( glIsList( m_layer_middle_contourns_quads ) )
        glDeleteLists( m_layer_middle_contourns_quads, 1 );

    if( glIsList( m_layer_bot_triangles ) )
        glDeleteLists( m_layer_bot_triangles, 1 );

    if( glIsList( m_layer_bot_segment_ends ) )
        glDeleteLists( m_layer_bot_segment_ends, 1 );
}


void CLAYERS_OGL_DISP_LISTS::DrawTopAndMiddle() const
{
    if( glIsList( m_layer_top_triangles ) )
        glCallList( m_layer_top_triangles );

    if( glIsList( m_layer_middle_contourns_quads ) )
        glCallList( m_layer_middle_contourns_quads );

    if( glIsList( m_layer_top_segment_ends ) )
        glCallList( m_layer_top_segment_ends );

}


void CLAYERS_OGL_DISP_LISTS::DrawBotAndMiddle() const
{
    if( glIsList( m_layer_bot_triangles ) )
        glCallList( m_layer_bot_triangles );

    if( glIsList( m_layer_middle_contourns_quads ) )
        glCallList( m_layer_middle_contourns_quads );

    if( glIsList( m_layer_bot_segment_ends ) )
        glCallList( m_layer_bot_segment_ends );

}


void CLAYERS_OGL_DISP_LISTS::DrawTop() const
{
    if( glIsList( m_layer_top_triangles ) )
        glCallList( m_layer_top_triangles );

    if( glIsList( m_layer_top_segment_ends ) )
        glCallList( m_layer_top_segment_ends );
}


void CLAYERS_OGL_DISP_LISTS::DrawBot() const
{
    if( glIsList( m_layer_bot_triangles ) )
        glCallList( m_layer_bot_triangles );

    if( glIsList( m_layer_bot_segment_ends ) )
        glCallList( m_layer_bot_segment_ends );
}


void CLAYERS_OGL_DISP_LISTS::DrawMiddle() const
{
    if( glIsList( m_layer_middle_contourns_quads ) )
        glCallList( m_layer_middle_contourns_quads );
}


void CLAYERS_OGL_DISP_LISTS::DrawAll() const
{

    if( glIsList( m_layer_top_triangles ) )
        glCallList( m_layer_top_triangles );

    if( glIsList( m_layer_middle_contourns_quads ) )
        glCallList( m_layer_middle_contourns_quads );

    if( glIsList( m_layer_bot_triangles ) )
        glCallList( m_layer_bot_triangles );

    if( glIsList( m_layer_top_segment_ends ) )
        glCallList( m_layer_top_segment_ends );

    if( glIsList( m_layer_bot_segment_ends ) )
        glCallList( m_layer_bot_segment_ends );

}


GLuint CLAYERS_OGL_DISP_LISTS::generate_top_or_bot_seg_ends(const CLAYER_TRIANGLE_CONTAINER *aTriangleContainer, const SFVEC3F &aLayerColor, bool aIsNormalUp, GLuint aTextureId ) const
{
    wxASSERT( aTriangleContainer != NULL );

    wxASSERT( (aTriangleContainer->GetVertexSize() % 3) == 0 );

    wxASSERT( aTriangleContainer->GetNormalsSize() == 0 );

    if( (aTriangleContainer->GetVertexSize() > 0) &&
        ((aTriangleContainer->GetVertexSize() % 3) == 0) )
    {
        GLuint listIdx = glGenLists( 1 );

        if( glIsList( listIdx ) )
        {
            // Prepare an array of UV text coordinates
            SFVEC2F *uvArray = new SFVEC2F[aTriangleContainer->GetVertexSize()];

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

            glEnable( GL_TEXTURE_2D );
            glBindTexture( GL_TEXTURE_2D, aTextureId );

            glAlphaFunc( GL_GREATER, 0.60f );
            glEnable( GL_ALPHA_TEST );

            glEnable( GL_BLEND );
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

            //SFVEC4F layerColor4 = SFVEC4F( aLayerColor.x, aLayerColor.y, aLayerColor.z, 1.0f );
            //glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,  &layerColor4.x );
            //glDisable( GL_COLOR_MATERIAL );
            glEnable( GL_COLOR_MATERIAL );
            glColor4f( aLayerColor.x, aLayerColor.y, aLayerColor.z, 1.0f );

            // OGL_SetMaterial()
            glNormal3f( 0.0f, 0.0f, aIsNormalUp?1.0f:-1.0f );

            glDrawArrays( GL_TRIANGLES, 0, aTriangleContainer->GetVertexSize() );

            glDisable( GL_TEXTURE_2D );
            glDisable( GL_ALPHA_TEST );
            glDisable( GL_BLEND );
            glDisable( GL_COLOR_MATERIAL );

            glEndList();

            glDisableClientState( GL_VERTEX_ARRAY );
            glDisableClientState( GL_TEXTURE_COORD_ARRAY );

            delete [] uvArray;
            return listIdx;
        }
    }

    return 0;
}


GLuint CLAYERS_OGL_DISP_LISTS::generate_top_or_bot_triangles(const CLAYER_TRIANGLE_CONTAINER *aTriangleContainer, const SFVEC3F &aLayerColor , bool aIsNormalUp) const
{
    wxASSERT( aTriangleContainer != NULL );

    wxASSERT( (aTriangleContainer->GetVertexSize() % 3) == 0 );

    wxASSERT( aTriangleContainer->GetNormalsSize() == 0 );

    if( (aTriangleContainer->GetVertexSize() > 0) &&
        ((aTriangleContainer->GetVertexSize() % 3) == 0) )
    {
        GLuint listIdx = glGenLists( 1 );

        if( glIsList( listIdx ) )
        {
            glDisableClientState( GL_TEXTURE_COORD_ARRAY );
            glDisableClientState( GL_COLOR_ARRAY );
            glDisableClientState( GL_NORMAL_ARRAY );
            glEnableClientState( GL_VERTEX_ARRAY );
            glVertexPointer( 3, GL_FLOAT, 0, aTriangleContainer->GetVertexPointer() );

            glNewList( listIdx, GL_COMPILE );

            //SFVEC4F layerColor4 = SFVEC4F( aLayerColor.x, aLayerColor.y, aLayerColor.z, 1.0f );
            //glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,  &layerColor4.x );
            glEnable( GL_COLOR_MATERIAL );
            glColor4f( aLayerColor.x, aLayerColor.y, aLayerColor.z, 1.0f );

            // OGL_SetMaterial()
            glNormal3f( 0.0f, 0.0f, aIsNormalUp?1.0f:-1.0f );

            glDrawArrays( GL_TRIANGLES, 0, aTriangleContainer->GetVertexSize() );
/*
            glEnable( GL_COLOR_MATERIAL );
            glColor3f( 0.0,1.0,1.0);
            for( unsigned int i=0; i < aTriangleContainer->GetVertexSize(); ++i )
            {
                const SFVEC3F &v1 = aTriangleContainer->GetVertexPointer()[ i * 3];

                glBegin(GL_LINES);
                glVertex3f( v1.x, v1.y, v1.z );
                if( aIsNormalUp )
                {
                    glVertex3f( v1.x, v1.y, v1.z+1.0f*.0051f );
                }
                else
                {
                    glVertex3f( v1.x, v1.y, v1.z-1.0f*.0051f );
                }
                glEnd();
            }
*/
            glEndList();

            glDisableClientState( GL_VERTEX_ARRAY );

            return listIdx;
        }
    }

    return 0;
}

GLuint CLAYERS_OGL_DISP_LISTS::generate_middle_triangles( const CLAYER_TRIANGLE_CONTAINER *aTriangleContainer, const SFVEC3F &aLayerColor ) const
{
    wxASSERT( aTriangleContainer != NULL );

    // We expect that it is a multiple of 3 vertex
    wxASSERT( (aTriangleContainer->GetVertexSize() % 3) == 0 );

    // We expect that it is a multiple of 6 vertex (because we expect to add quads)
    wxASSERT( (aTriangleContainer->GetVertexSize() % 6) == 0 );

    // We expect that there are normals with same size as vertex
    wxASSERT( aTriangleContainer->GetNormalsSize() == aTriangleContainer->GetVertexSize() );


    if( ( aTriangleContainer->GetVertexSize() > 0 ) &&
        ( (aTriangleContainer->GetVertexSize() % 3) == 0 ) &&
        ( (aTriangleContainer->GetVertexSize() % 6) == 0 ) &&
        ( aTriangleContainer->GetNormalsSize() == aTriangleContainer->GetVertexSize() ) )
    {
        GLuint listIdx = glGenLists( 1 );

        if( glIsList( listIdx ) )
        {
            glDisableClientState( GL_TEXTURE_COORD_ARRAY );
            glDisableClientState( GL_COLOR_ARRAY );
            glEnableClientState( GL_NORMAL_ARRAY );
            glEnableClientState( GL_VERTEX_ARRAY );
            glVertexPointer( 3, GL_FLOAT, 0, aTriangleContainer->GetVertexPointer() );
            glNormalPointer( GL_FLOAT, 0, aTriangleContainer->GetNormalsPointer() );

            glNewList( listIdx, GL_COMPILE );
/*
            const SFVEC4F specular = SFVEC4F( 0.5f, 0.5f, 0.5f, 1.0f );

            glMaterialfv( GL_FRONT, GL_SPECULAR, &specular.r );
            glMaterialf(  GL_FRONT, GL_SHININESS, 10.0f );*/

            //SFVEC4F layerColor4 = SFVEC4F( aLayerColor.x, aLayerColor.y, aLayerColor.z, 1.0f );
            //glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,  &layerColor4.x );
            //glDisable( GL_COLOR_MATERIAL );
            glEnable( GL_COLOR_MATERIAL );
            glColor4f( aLayerColor.x, aLayerColor.y, aLayerColor.z, 1.0f );

            // OGL_SetMaterial()

            glDrawArrays( GL_TRIANGLES, 0, aTriangleContainer->GetVertexSize() );
/*
            glEnable( GL_COLOR_MATERIAL );
            glColor3f( 1.0,0.0,1.0);
            for( unsigned int i=0; i < aTriangleContainer->GetVertexSize() / 3 ; ++i )
            {
                const SFVEC3F &v1 = ((const SFVEC3F*)aTriangleContainer->GetVertexPointer())[i * 3];
                const SFVEC3F &n1 = ((const SFVEC3F*)aTriangleContainer->GetNormalsPointer())[i * 3];
                glBegin(GL_LINES);
                glVertex3f( v1.x, v1.y, v1.z );
                glVertex3f( v1.x+n1.x*.01f, v1.y+n1.y*.01f, v1.z+n1.z*.01f );
                glEnd();
            }
*/
            glEndList();

            glDisableClientState( GL_VERTEX_ARRAY );
            glDisableClientState( GL_NORMAL_ARRAY );

            return listIdx;
        }
    }

    return 0;
}


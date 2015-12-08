/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@gmail.com>
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras@wanadoo.fr
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
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
 * @file 3d_read_mesh.cpp
 */

#include <fctsys.h>
#include <common.h>
#include <macros.h>
#include <kicad_string.h>
#include <pgm_base.h>
#define GLM_FORCE_RADIANS
#include <glm/gtc/matrix_transform.hpp>
#include <3d_viewer.h>
#include <info3d_visu.h>
#include "3d_struct.h"
#include "modelparsers.h"


S3D_MODEL_PARSER *S3D_MODEL_PARSER::Create( S3D_MASTER* aMaster,
                                            const wxString aExtension )
{
    if ( aExtension == wxT( "x3d" ) )
        return new X3D_MODEL_PARSER( aMaster );
    else if ( aExtension == wxT( "wrl" ) )
        return new VRML_MODEL_PARSER( aMaster );

    return NULL;
 }


int S3D_MASTER::ReadData( S3D_MODEL_PARSER* aParser )
{
    if( m_Shape3DFullFilename.IsEmpty() || aParser == NULL )
        return -1;

    wxString filename = m_Shape3DFullFilename;

#ifdef __WINDOWS__
    filename.Replace( wxT( "/" ), wxT( "\\" ) );
#else
    filename.Replace( wxT( "\\" ), wxT( "/" ) );
#endif

    if( wxFileName::FileExists( filename ) )
    {
        wxFileName fn( filename );

        if( aParser->Load( filename ) )
        {
            // Invalidate bounding boxes
            m_fastAABBox.Reset();
            m_BBox.Reset();

            m_parser = aParser;

            return 0;
        }
    }

    wxLogDebug( wxT( "3D shape '%s' not found, even tried '%s' after env var substitution." ),
        GetChars( m_Shape3DName ),
        GetChars( filename ) );

    return -1;
}


void S3D_MASTER::Render( bool aIsRenderingJustNonTransparentObjects,
                         bool aIsRenderingJustTransparentObjects )
{
    if( m_parser == NULL )
        return;

    double aVrmlunits_to_3Dunits = g_Parm_3D_Visu.m_BiuTo3Dunits * UNITS3D_TO_UNITSPCB;

    glScalef( aVrmlunits_to_3Dunits, aVrmlunits_to_3Dunits, aVrmlunits_to_3Dunits );

    glTranslatef( m_MatPosition.x * SCALE_3D_CONV,
                  m_MatPosition.y * SCALE_3D_CONV,
                  m_MatPosition.z * SCALE_3D_CONV );

    glRotatef( -m_MatRotation.z, 0.0f, 0.0f, 1.0f );
    glRotatef( -m_MatRotation.y, 0.0f, 1.0f, 0.0f );
    glRotatef( -m_MatRotation.x, 1.0f, 0.0f, 0.0f );

    glScalef( m_MatScale.x, m_MatScale.y, m_MatScale.z );

    for( unsigned int idx = 0; idx < m_parser->childs.size(); idx++ )
        m_parser->childs[idx]->openGL_RenderAllChilds( aIsRenderingJustNonTransparentObjects,
                                                       aIsRenderingJustTransparentObjects );
}


CBBOX &S3D_MASTER::getBBox( )
{
    if( !m_BBox.IsInitialized() )
        calcBBox();

    return m_BBox;
}


CBBOX &S3D_MASTER::getFastAABBox( )
{
    if( !m_fastAABBox.IsInitialized() )
        calcBBox();

    return m_fastAABBox;
}


void S3D_MASTER::calcBBox()
{
    if( m_parser == NULL )
        return;

    bool firstBBox = true;

    for( unsigned int idx = 0; idx < m_parser->childs.size(); idx++ )
        if( firstBBox )
        {
            firstBBox = false;
            m_BBox = m_parser->childs[idx]->getBBox();
        }
        else
            m_BBox.Union( m_parser->childs[idx]->getBBox() );

    // Calc transformation matrix to apply in AABBox

    float aVrmlunits_to_3Dunits = g_Parm_3D_Visu.m_BiuTo3Dunits * UNITS3D_TO_UNITSPCB;

    glm::mat4 fullTransformMatrix;

    fullTransformMatrix = glm::scale( glm::mat4(), S3D_VERTEX(  aVrmlunits_to_3Dunits,
                                                                aVrmlunits_to_3Dunits,
                                                                aVrmlunits_to_3Dunits ) );

    fullTransformMatrix = glm::translate( fullTransformMatrix,  S3D_VERTEX( m_MatPosition.x * SCALE_3D_CONV,
                                                                            m_MatPosition.y * SCALE_3D_CONV,
                                                                            m_MatPosition.z * SCALE_3D_CONV) );

    if( m_MatRotation.z != 0.0 )
        fullTransformMatrix = glm::rotate( fullTransformMatrix, glm::radians(-(float)m_MatRotation.z), S3D_VERTEX( 0.0f, 0.0f, 1.0f ) );
    if( m_MatRotation.y != 0.0 )
        fullTransformMatrix = glm::rotate( fullTransformMatrix, glm::radians(-(float)m_MatRotation.y), S3D_VERTEX( 0.0f, 1.0f, 0.0f ) );
    if( m_MatRotation.x != 0.0 )
        fullTransformMatrix = glm::rotate( fullTransformMatrix, glm::radians(-(float)m_MatRotation.x), S3D_VERTEX( 1.0f, 0.0f, 0.0f ) );

     fullTransformMatrix = glm::scale( fullTransformMatrix, S3D_VERTEX( m_MatScale.x, m_MatScale.y, m_MatScale.z ) );

    // Apply transformation
    m_fastAABBox = m_BBox;
    m_fastAABBox.ApplyTransformationAA( fullTransformMatrix );
}

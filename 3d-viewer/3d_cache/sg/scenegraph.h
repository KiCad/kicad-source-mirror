/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file scenegraph.h
 */


#ifndef SCENE_GRAPH_H
#define SCENE_GRAPH_H

#include <vector>
#include "3d_cache/sg/sg_node.h"

class SGSHAPE;

/**
 * Define the basic data set required to represent a 3D model.
 *
 * This model must remain compatible with VRML2.0 in order to facilitate VRML export of
 * scene graph data created by available 3D plugins.
 */
class SCENEGRAPH : public SGNODE
{
public:
    void unlinkChildNode( const SGNODE* aNode ) override;
    void unlinkRefNode( const SGNODE* aNode ) override;

    SCENEGRAPH( SGNODE* aParent );
    virtual ~SCENEGRAPH();

    virtual bool SetParent( SGNODE* aParent, bool notify = true ) override;
    SGNODE* FindNode(const char *aNodeName, const SGNODE *aCaller) override;
    bool AddRefNode( SGNODE* aNode ) override;
    bool AddChildNode( SGNODE* aNode ) override;

    void ReNameNodes( void ) override;
    bool WriteVRML( std::ostream& aFile, bool aReuseFlag ) override;

    bool WriteCache( std::ostream& aFile, SGNODE* parentNode ) override;
    bool ReadCache( std::istream& aFile, SGNODE* parentNode ) override;

    bool Prepare( const glm::dmat4* aTransform, S3D::MATLIST& materials,
                  std::vector< SMESH >& meshes );

private:
    void unlinkNode( const SGNODE* aNode, bool isChild );
    bool addNode( SGNODE* aNode, bool isChild );

public:
    // note: order of transformation is Translate, Rotate, Offset
    SGPOINT  center;
    SGPOINT  translation;
    SGVECTOR rotation_axis;
    double   rotation_angle;    // radians
    SGPOINT  scale;
    SGVECTOR scale_axis;
    double   scale_angle;       // radians

private:
    // The following are items which may be defined for reuse
    // in a VRML output file. They do not necessarily correspond
    // to the use of DEF within a VRML input file; it is the
    // responsibility of the plugin to perform any necessary
    // conversions to comply with the restrictions imposed by
    // this scene graph structure
    std::vector< SCENEGRAPH* > m_Transforms;   // local Transform nodes
    std::vector< SGSHAPE* > m_Shape;           // local Shape nodes

    std::vector< SCENEGRAPH* > m_RTransforms;   // referenced Transform nodes
    std::vector< SGSHAPE* > m_RShape;           // referenced Shape nodes
};

/*
    p.120
    Transform {
        center              0 0 0
        children            []
        rotation            0 0 1 0
        scale               1 1 1
        scaleOrientation    0 0 1 0
        translation         0 0 0
        bboxCenter          0 0 0
        bboxSize            -1 -1 -1
    }
*/

#endif  // SCENE_GRAPH_H

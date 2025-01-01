/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2017 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file sg_shape.h
 */


#ifndef SG_SHAPE_H
#define SG_SHAPE_H

#include <vector>
#include "3d_cache/sg/sg_node.h"

class SGAPPEARANCE;
class SGFACESET;

/**
 * Define a complex 3D shape for a scenegraph object.
 */
class SGSHAPE : public SGNODE
{
public:
    SGSHAPE( SGNODE* aParent );
    virtual ~SGSHAPE();

    virtual bool SetParent( SGNODE* aParent, bool notify = true ) override;

    SGNODE* FindNode(const char* aNodeName, const SGNODE* aCaller) override;
    bool AddRefNode( SGNODE* aNode ) override;
    bool AddChildNode( SGNODE* aNode ) override;

    void ReNameNodes( void ) override;
    bool WriteVRML( std::ostream& aFile, bool aReuseFlag ) override;

    bool WriteCache( std::ostream& aFile, SGNODE* parentNode ) override;
    bool ReadCache( std::istream& aFile, SGNODE* parentNode ) override;

    bool Prepare( const glm::dmat4* aTransform, S3D::MATLIST& materials,
                  std::vector< SMESH >& meshes );

    void unlinkChildNode( const SGNODE* aNode ) override;
    void unlinkRefNode( const SGNODE* aNode ) override;

private:
    void unlinkNode( const SGNODE* aNode, bool isChild );
    bool addNode( SGNODE* aNode, bool isChild );

public:
    // owned node
    SGAPPEARANCE* m_Appearance;
    SGFACESET*    m_FaceSet;

    // referenced nodes
    SGAPPEARANCE* m_RAppearance;
    SGFACESET*    m_RFaceSet;
};

/*
    p.107
    Shape {
        appearance  NULL
        geometry    NULL
    }
*/

#endif  // SG_SHAPE_H

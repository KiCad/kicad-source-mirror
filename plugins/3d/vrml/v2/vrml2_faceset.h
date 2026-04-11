/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file vrml2_faceset.h
 */


#ifndef VRML2_FACESET_H
#define VRML2_FACESET_H

#include <vector>

#include "vrml2_node.h"

class WRL2BASE;
class SGNODE;

class WRL2FACESET : public WRL2NODE
{
public:
    WRL2FACESET();
    WRL2FACESET( WRL2NODE* aParent );
    virtual ~WRL2FACESET();

    bool Read( WRLPROC& proc, WRL2BASE* aTopNode ) override;
    bool AddRefNode( WRL2NODE* aNode ) override;
    bool AddChildNode( WRL2NODE* aNode ) override;
    SGNODE* TranslateToSG( SGNODE* aParent ) override;

    /**
     * @return true if the face set has a color node.
     */
    bool HasColors( void );

    bool isDangling( void ) override;

    void unlinkChildNode( const WRL2NODE* aNode ) override;
    void unlinkRefNode( const WRL2NODE* aNode ) override;

private:
    /**
     * @return true if the node type is a valid subnode of FaceSet.
     */
    bool checkNodeType( WRL2NODES aType );

    void setDefaults( void );

    WRL2NODE* color;
    WRL2NODE* coord;
    WRL2NODE* normal;
    WRL2NODE* texCoord;

    bool ccw;
    bool colorPerVertex;
    bool convex;
    bool normalPerVertex;
    bool solid;

    std::vector< int > colorIndex;
    std::vector< int > coordIndex;
    std::vector< int > normalIndex;

    float creaseAngle;
    float creaseLimit;
};

#endif  // VRML2_FACESET_H

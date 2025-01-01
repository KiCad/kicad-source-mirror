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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file x3d_transform.h
 */


#ifndef X3D_TRANSFORM_H
#define X3D_TRANSFORM_H

#include <vector>

#include "x3d_base.h"
#include "wrltypes.h"


class X3DTRANSFORM : public X3DNODE
{
public:
    X3DTRANSFORM();
    X3DTRANSFORM( X3DNODE* aParent );
    virtual ~X3DTRANSFORM();

    bool Read( wxXmlNode* aNode, X3DNODE* aTopNode, X3D_DICT& aDict ) override;
    bool SetParent( X3DNODE* aParent, bool doUnlink = true ) override;
    bool AddChildNode( X3DNODE* aNode ) override;
    bool AddRefNode( X3DNODE* aNode ) override;
    SGNODE* TranslateToSG( SGNODE* aParent ) override;

private:
    void init();
    void readFields( wxXmlNode* aNode );

    WRLVEC3F    center;
    WRLVEC3F    scale;
    WRLVEC3F    translation;
    WRLROTATION rotation;
    WRLROTATION scaleOrientation;
    WRLVEC3F    bboxCenter;
    WRLVEC3F    bboxSize;
};

#endif  // X3D_TRANSFORM_H

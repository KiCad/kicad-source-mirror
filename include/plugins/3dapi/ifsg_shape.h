/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Cirilo Bernardo <cirilo.bernardo@gmail.com>
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
 * @file ifsg_shape.h
 * defines the wrapper for the SGSHAPE class
 */


#ifndef IFSG_SHAPE_H
#define IFSG_SHAPE_H

#include "plugins/3dapi/ifsg_node.h"


/**
 * The wrapper for the SGSHAPE class.
 */
class SGLIB_API IFSG_SHAPE : public IFSG_NODE
{
public:
    IFSG_SHAPE( bool create );
    IFSG_SHAPE( SGNODE* aParent );
    IFSG_SHAPE( IFSG_NODE& aParent );

    bool Attach( SGNODE* aNode ) override;
    bool NewNode( SGNODE* aParent ) override;
    bool NewNode( IFSG_NODE& aParent ) override;
};

#endif  // IFSG_SHAPE_H

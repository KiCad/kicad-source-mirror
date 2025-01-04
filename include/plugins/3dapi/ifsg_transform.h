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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file ifsg_transform.h
 * defines the wrapper for the SCENEGRAPH class
 */


#ifndef IFSG_TRANSFORM_H
#define IFSG_TRANSFORM_H

#include "plugins/3dapi/ifsg_node.h"


/**
 * The wrapper for the VRML compatible #TRANSFORM block class #SCENEGRAPH.
 */
class SGLIB_API IFSG_TRANSFORM : public IFSG_NODE
{
public:
    /**
     * @note IFSG_TRANSFORM( IFSG_NODE& aParent ) does not exist since a transform may own
     *       another transform and that construct invites accidental misuse of the copy
     *       constructor./
     */
    IFSG_TRANSFORM( bool create );
    IFSG_TRANSFORM( SGNODE* aParent );

    bool Attach( SGNODE* aNode ) override;
    bool NewNode( SGNODE* aParent ) override;
    bool NewNode( IFSG_NODE& aParent ) override;

    bool SetScaleOrientation( const SGVECTOR& aScaleAxis, double aAngle );
    bool SetRotation( const SGVECTOR& aRotationAxis, double aAngle );
    bool SetScale( const SGPOINT& aScale ) noexcept;
    bool SetScale( double aScale );
    bool SetCenter( const SGPOINT& aCenter ) noexcept;
    bool SetTranslation( const SGPOINT& aTranslation ) noexcept;
};

#endif  // IFSG_TRANSFORM_H

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
 * @file ifsg_coordindex.h
 * defines the CoordIndex node wrapper
 */


#ifndef IFSG_NORMALINDEX_H
#define IFSG_NORMALINDEX_H

#include "plugins/3dapi/ifsg_index.h"


/**
 * Class IFSG_NORMALINDEX
 * is the wrapper for SGNORMALINDEX
 */
class SGLIB_API IFSG_NORMALINDEX : public IFSG_INDEX
{
public:
    IFSG_NORMALINDEX( bool create );
    IFSG_NORMALINDEX( SGNODE* aParent );
    IFSG_NORMALINDEX( IFSG_NODE& aParent );

    bool Attach( SGNODE* aNode );
    bool NewNode( SGNODE* aParent );
    bool NewNode( IFSG_NODE& aParent );
};

#endif  // IFSG_NORMALINDEX_H

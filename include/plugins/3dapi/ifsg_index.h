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
 * @file ifsg_index.h
 * defines the index nodes wrapper
 */


#ifndef IFSG_INDEX_H
#define IFSG_INDEX_H

#include "plugins/3dapi/ifsg_node.h"


/**
 * Class IFSG_INDEX
 * is the wrapper for SGINDEX
 */
class SGLIB_API IFSG_INDEX : public IFSG_NODE
{
public:
    IFSG_INDEX();

    virtual bool Attach( SGNODE* aNode ) override = 0;
    virtual bool NewNode( SGNODE* aParent ) override = 0;
    virtual bool NewNode( IFSG_NODE& aParent ) override = 0;

    bool GetIndices( size_t& nIndices, int*& aIndexList );

    /**
     * Function SetIndices
     * sets the number of indices and creates a copy of the given index data.
     *
     * @param nIndices [in] the number of indices to be stored
     * @param aIndexList [in] the index data
     */
    bool SetIndices( size_t nIndices, int* aIndexList );


    /**
     * Function AddIndex
     * adds a single index to the list
     *
     * @param nIndices [in] the number of indices to be stored
     * @param aIndexList [in] the index data
     */
    bool AddIndex( int aIndex );
};

#endif  // IFSG_INDEX_H

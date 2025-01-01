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
 * @file sg_index.h
 */

#ifndef SG_INDEX_H
#define SG_INDEX_H

#include <vector>
#include "3d_cache/sg/sg_node.h"

/**
 * Define a generic index interface for a scenegraph object.
 */
class SGINDEX : public SGNODE
{
public:
    SGINDEX( SGNODE* aParent );
    virtual ~SGINDEX();

    virtual bool SetParent( SGNODE* aParent, bool notify = true ) override;

    SGNODE* FindNode(const char* aNodeName, const SGNODE* aCaller) noexcept override;
    bool AddRefNode( SGNODE* aNode ) noexcept override;
    bool AddChildNode( SGNODE* aNode ) noexcept override;

    void unlinkChildNode( const SGNODE* aCaller ) noexcept override;
    void unlinkRefNode( const SGNODE* aCaller ) noexcept override;

    /**
     * Retrieve the number of indices and a pointer to the list.
     *
     * @note The returned pointer may be invalidated by future operations on the SGNODE.  The
     *       caller must make immediate use of the data and must not rely on the pointer's
     *       validity in the future.
     *
     * @param nIndices will hold the number of indices in the list.
     * @param aIndexList will store a pointer to the data.
     * @return true if there was available data (nIndices > 0) otherwise false.
     */
    bool GetIndices( size_t& nIndices, int*& aIndexList );

    /**
     * Set the number of indices and creates a copy of the given index data.
     *
     * @param nIndices the number of indices to be stored.
     * @param aIndexList the index data.
     */
    void SetIndices( size_t nIndices, int* aIndexList );


    /**
     * Add a single index to the list.
     *
     * @param aIndex is the index to add.
     */
    void AddIndex( int aIndex );

    void ReNameNodes( void ) override;
    bool WriteVRML( std::ostream& aFile, bool aReuseFlag ) override;

    bool WriteCache( std::ostream& aFile, SGNODE* parentNode ) override;
    bool ReadCache( std::istream& aFile, SGNODE* parentNode ) override;

protected:
    bool writeCoordIndex( std::ostream& aFile );
    bool writeColorIndex( std::ostream& aFile );
    bool writeIndexList( std::ostream& aFile );

public:
    // for internal SG consumption only
    std::vector< int > index;
};

#endif  // SG_INDEX_H

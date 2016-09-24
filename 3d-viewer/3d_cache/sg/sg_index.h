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
 * @file sg_index.h
 * defines a generic Index interface for a scenegraph object
 */

#ifndef SG_INDEX_H
#define SG_INDEX_H

#include <vector>
#include "3d_cache/sg/sg_node.h"

class SGINDEX : public SGNODE
{
protected:
    bool writeCoordIndex( std::ofstream& aFile );
    bool writeColorIndex( std::ofstream& aFile );
    bool writeIndexList( std::ofstream& aFile );

public:
    // for internal SG consumption only
    std::vector< int > index;
    void unlinkChildNode( const SGNODE* aCaller ) override;
    void unlinkRefNode( const SGNODE* aCaller ) override;

public:
    SGINDEX( SGNODE* aParent );
    virtual ~SGINDEX();

    virtual bool SetParent( SGNODE* aParent, bool notify = true ) override;

    SGNODE* FindNode(const char *aNodeName, const SGNODE *aCaller) override;
    bool AddRefNode( SGNODE* aNode ) override;
    bool AddChildNode( SGNODE* aNode ) override;

    /**
     * Function GetIndices
     * retrieves the number of indices and a pointer to
     * the list. Note: the returned pointer may be invalidated
     * by future operations on the SGNODE; the caller must make
     * immediate use of the data and must not rely on the pointer's
     * validity in the future.
     *
     * @param nIndices [out] will hold the number of indices in the list
     * @param aIndexList [out] will store a pointer to the data
     * @return true if there was available data (nIndices > 0) otherwise false
     */
    bool GetIndices( size_t& nIndices, int*& aIndexList );

    /**
     * Function SetIndices
     * sets the number of indices and creates a copy of the given index data.
     *
     * @param nIndices [in] the number of indices to be stored
     * @param aIndexList [in] the index data
     */
    void SetIndices( size_t nIndices, int* aIndexList );


    /**
     * Function AddIndex
     * adds a single index to the list
     *
     * @param nIndices [in] the number of indices to be stored
     * @param aIndexList [in] the index data
     */
    void AddIndex( int aIndex );

    void ReNameNodes( void ) override;
    bool WriteVRML( std::ofstream& aFile, bool aReuseFlag ) override;

    bool WriteCache( std::ofstream& aFile, SGNODE* parentNode ) override;
    bool ReadCache( std::ifstream& aFile, SGNODE* parentNode ) override;
};

#endif  // SG_INDEX_H

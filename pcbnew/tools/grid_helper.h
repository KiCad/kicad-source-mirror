/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#ifndef __GRID_HELPER_H
#define __GRID_HELPER_H

#include <vector>

#include <math/vector2d.h>
#include <boost/optional.hpp>

#include <layers_id_colors_and_visibility.h>

class PCB_BASE_FRAME;

class GRID_HELPER {
public:

    GRID_HELPER( PCB_BASE_FRAME* aFrame );
    ~GRID_HELPER();

    void SetGrid( int aSize );
    void SetOrigin( const VECTOR2I& aOrigin );

    VECTOR2I GetGrid();
    VECTOR2I GetOrigin();

    void SetAuxAxes( bool aEnable, const VECTOR2I aOrigin = VECTOR2I( 0, 0 ), bool aEnableDiagonal = false );

    VECTOR2I Align( const VECTOR2I& aPoint );

    VECTOR2I BestDragOrigin ( const VECTOR2I &aMousePos, BOARD_ITEM* aItem );
    VECTOR2I BestSnapAnchor ( const VECTOR2I &aOrigin, BOARD_ITEM* aDraggedItem );

private:
    enum ANCHOR_FLAGS {
        CORNER = 0x1,
        OUTLINE = 0x2,
        SNAPPABLE = 0x4,
        ORIGIN = 0x8
    };

    struct ANCHOR
    {
        ANCHOR( VECTOR2I aPos, int aFlags = CORNER | SNAPPABLE, BOARD_ITEM* aItem = NULL ):
            pos( aPos ), flags( aFlags ), item( aItem ) {} ;

        VECTOR2I pos;
        int flags;
        BOARD_ITEM* item;

        double Distance( const VECTOR2I& aP )
        {
            return ( aP - pos ).EuclideanNorm();
        }

        bool CanSnapItem( const BOARD_ITEM* aItem );
    };

    std::vector<ANCHOR> m_anchors;

    std::set<BOARD_ITEM*> queryVisible( const BOX2I& aArea );

    void addAnchor( VECTOR2I aPos, int aFlags = CORNER | SNAPPABLE, BOARD_ITEM* aItem = NULL )
    {
        m_anchors.push_back( ANCHOR( aPos, aFlags, aItem ) );
    }

    ANCHOR* nearestAnchor( VECTOR2I aPos, int aFlags, LSET aMatchLayers );

    void computeAnchors( BOARD_ITEM* aItem, const VECTOR2I& aRefPos );

    void clearAnchors ()
    {
        m_anchors.clear();
    }

    PCB_BASE_FRAME* m_frame;
    boost::optional<VECTOR2I> m_auxAxis;
    bool m_diagonalAuxAxesEnable;
};

#endif

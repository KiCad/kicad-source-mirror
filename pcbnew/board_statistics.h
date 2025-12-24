/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef PCBNEW_BOARD_STATISTICS_H
#define PCBNEW_BOARD_STATISTICS_H

#include <layer_ids.h>
#include <padstack.h>

class BOARD;

struct DRILL_LINE_ITEM
{
    enum COL_ID
    {
        COL_COUNT,
        COL_SHAPE,
        COL_X_SIZE,
        COL_Y_SIZE,
        COL_PLATED,
        COL_VIA_PAD,
        COL_START_LAYER,
        COL_STOP_LAYER
    };

    DRILL_LINE_ITEM( int aXSize, int aYSize, PAD_DRILL_SHAPE aShape, bool aIsPlated,
                     bool aIsPad, PCB_LAYER_ID aStartLayer, PCB_LAYER_ID aStopLayer ) :
            xSize( aXSize ),
            ySize( aYSize ),
            shape( aShape ),
            isPlated( aIsPlated ),
            isPad( aIsPad ),
            startLayer( aStartLayer ),
            stopLayer( aStopLayer ),
            m_Qty( 0 )
    {
    }

    bool operator==( const DRILL_LINE_ITEM& other ) const
    {
        return xSize == other.xSize
                && ySize == other.ySize
                && shape == other.shape
                && isPlated == other.isPlated
                && isPad == other.isPad
                && startLayer == other.startLayer
                && stopLayer == other.stopLayer;
    }

    struct COMPARE
    {
        COMPARE( COL_ID aColId, bool aAscending ) :
                colId( aColId ),
                ascending( aAscending )
        {}

        bool operator()( const DRILL_LINE_ITEM& aLeft, const DRILL_LINE_ITEM& aRight )
        {
            switch( colId )
            {
            case COL_COUNT:
                return compareDrillParameters( aLeft.m_Qty, aRight.m_Qty );
            case COL_SHAPE:
                return compareDrillParameters( static_cast<int>( aLeft.shape ), static_cast<int>( aRight.shape ) );
            case COL_X_SIZE:
                return compareDrillParameters( aLeft.xSize, aRight.xSize );
            case COL_Y_SIZE:
                return compareDrillParameters( aLeft.ySize, aRight.ySize );
            case COL_PLATED:
                return ascending ? aLeft.isPlated : aRight.isPlated;
            case COL_VIA_PAD:
                return ascending ? aLeft.isPad : aRight.isPad;
            case COL_START_LAYER:
                return compareDrillParameters( aLeft.startLayer, aRight.startLayer );
            case COL_STOP_LAYER:
                return compareDrillParameters( aLeft.stopLayer, aRight.stopLayer );
            }

            return false;
        }

        bool compareDrillParameters( int aLeft, int aRight )
        {
            return ascending ? aLeft < aRight : aLeft > aRight;
        }

        COL_ID colId;
        bool   ascending;
    };

    int             xSize;
    int             ySize;
    PAD_DRILL_SHAPE shape;
    bool            isPlated;
    bool            isPad;
    PCB_LAYER_ID    startLayer;
    PCB_LAYER_ID    stopLayer;
    int             m_Qty;
};

void CollectDrillLineItems( BOARD* board, std::vector<DRILL_LINE_ITEM>& out );

#endif

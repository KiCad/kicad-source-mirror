/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@gmail.com>
 *
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


#ifndef __AR_AUTOPLACER_H
#define __AR_AUTOPLACER_H

#include "ar_matrix.h"

#include <board.h>
#include <footprint.h>
#include <lset.h>

#include <connectivity/connectivity_data.h>

#include <view/view_overlay.h>

enum AR_CELL_STATE
{
    AR_OUT_OF_BOARD = -2,
    AR_OCCUIPED_BY_MODULE = -1,
    AR_FREE_CELL = 0
};

enum AR_RESULT
{
    AR_COMPLETED = 1,
    AR_CANCELLED,
    AR_FAILURE
};

class PROGRESS_REPORTER;

class AR_AUTOPLACER
{
public:
    AR_AUTOPLACER( BOARD* aBoard );

    AR_RESULT AutoplaceFootprints( std::vector<FOOTPRINT*>& aFootprints, BOARD_COMMIT* aCommit,
                                   bool aPlaceOffboardModules = false );

    /**
     * Set a VIEW overlay to draw items during a autoplace session.
     */
    void SetOverlay( std::shared_ptr<KIGFX::VIEW_OVERLAY> aOverlay )
    {
        m_overlay = aOverlay;
    }

    /**
     * Callback to redraw on screen the view after changes, for instance after moving a footprint.
     */
    void SetRefreshCallback( std::function<int( FOOTPRINT* aFootprint )> aCallback )
    {
        m_refreshCallback = aCallback;
    }

    void SetProgressReporter( PROGRESS_REPORTER* aReporter )
    {
        m_progressReporter = aReporter;
    }

private:
    void drawPlacementRoutingMatrix();  // draw the working area (shows free and occupied areas)
    int genPlacementRoutingMatrix();

    /**
     * Fill m_matrix cells from m_boardShape.
     *
     * Cells inside m_boardShape are set to CELL_IS_ZONE.
     */
    bool fillMatrix();
    void genModuleOnRoutingMatrix( FOOTPRINT* aFootprint );

    int testRectangle( const BOX2I& aRect, int side );
    unsigned int calculateKeepOutArea( const  BOX2I& aRect, int side );
    int testFootprintOnBoard( FOOTPRINT* aFootprint, bool TstOtherSide, const VECTOR2I& aOffset );
    int getOptimalFPPlacement( FOOTPRINT* aFootprint );
    double computePlacementRatsnestCost( FOOTPRINT* aFootprint, const VECTOR2I& aOffset );

    /**
     * Find the "best" footprint place. The criteria are:
     * - Maximum ratsnest with footprints already placed
     * - Max size, and number of pads max
     */
    FOOTPRINT* pickFootprint();

    void placeFootprint( FOOTPRINT* aFootprint, bool aDoNotRecreateRatsnest, const VECTOR2I& aPos );

    const PAD* nearestPad( FOOTPRINT* aRefFP, PAD* aRefPad, const VECTOR2I& aOffset );

    // Add a polygonal shape (rectangle) to m_fpAreaFront and/or m_fpAreaBack
    void addFpBody( const VECTOR2I& aStart, const VECTOR2I& aEnd, const LSET& aLayerMask );

    // Add a polygonal shape (rectangle) to m_fpAreaFront and/or m_fpAreaBack
    void addPad( PAD* aPad, int aClearance );

    // Build m_fpAreaTop and m_fpAreaBottom polygonal shapes for aFootprint.
    // aFpClearance is a mechanical clearance.
    void buildFpAreas( FOOTPRINT* aFootprint, int aFpClearance );

    AR_MATRIX m_matrix;
    SHAPE_POLY_SET m_topFreeArea;       // The polygonal description of the top side free areas;
    SHAPE_POLY_SET m_bottomFreeArea;    // The polygonal description of the bottom side free areas;
    SHAPE_POLY_SET m_boardShape;        // The polygonal description of the board;
    SHAPE_POLY_SET m_fpAreaTop;         // The polygonal description of the footprint to place,
                                        // top side;
    SHAPE_POLY_SET m_fpAreaBottom;      // The polygonal description of the footprint to place,
                                        // bottom side;

    BOARD* m_board;

    VECTOR2I m_curPosition;
    double   m_minCost;
    int      m_gridSize;

    std::shared_ptr<KIGFX::VIEW_OVERLAY>        m_overlay;
    std::unique_ptr<CONNECTIVITY_DATA>          m_connectivity;
    std::function<int( FOOTPRINT* aFootprint )> m_refreshCallback;
    PROGRESS_REPORTER*                          m_progressReporter;
};

#endif

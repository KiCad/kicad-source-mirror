/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Jean-Pierre Charras, jean-pierre.charras@ujf-grenoble.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2011 Wayne Stambaugh <stambaughw@verizon.net>
 *
 * Copyright (C) 1992-2012 KiCad Developers, see change_log.txt for contributors.
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

#include "ar_cell.h"
#include "ar_matrix.h"

#include <class_board.h>
#include <class_module.h>

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

    AR_RESULT AutoplaceModules( std::vector<MODULE*> aModules, BOARD_COMMIT* aCommit,
            bool aPlaceOffboardModules = false );

    const std::vector<MODULE*> QueryOffboardModules();

    void SetPlacementGrid( int aGrid )
    {
        m_gridSize = aGrid;
    }

    void SetOverlay( std::shared_ptr<KIGFX::VIEW_OVERLAY> aOverlay )
    {
        m_overlay = aOverlay;
    }

    void SetRefreshCallback( std::function<int()> aCallback )
    {
        m_refreshCallback = aCallback;
    }

    void SetProgressReporter( PROGRESS_REPORTER* aReporter )
    {
        m_progressReporter = aReporter;
    }

private:
    void         drawPlacementRoutingMatrix();
    void         rotateModule( MODULE* module, double angle, bool incremental );
    int          genPlacementRoutingMatrix();
    void         genModuleOnRoutingMatrix( MODULE* Module );
    int          propagate();
    int          testRectangle( const EDA_RECT& aRect, int side );
    unsigned int calculateKeepOutArea( const EDA_RECT& aRect, int side );
    int          testModuleOnBoard( MODULE* aModule, bool TstOtherSide, const wxPoint& aOffset );
    int          getOptimalModulePlacement( MODULE* aModule );
    double       computePlacementRatsnestCost( MODULE* aModule, const wxPoint& aOffset );
    MODULE*      pickModule();
    void         placeModule( MODULE* aModule, bool aDoNotRecreateRatsnest, const wxPoint& aPos );
    const D_PAD* nearestPad( MODULE* aRefModule, D_PAD* aRefPad, const wxPoint& aOffset );

    AR_MATRIX m_matrix;

    BOARD* m_board;

    wxPoint m_curPosition;
    wxPoint m_moduleOffset;
    double  m_minCost;
    int     m_gridSize;

    std::shared_ptr<KIGFX::VIEW_OVERLAY> m_overlay;
    std::unique_ptr<CONNECTIVITY_DATA>   m_connectivity;
    std::function<int()>                 m_refreshCallback;
    PROGRESS_REPORTER*                   m_progressReporter;
};

#endif

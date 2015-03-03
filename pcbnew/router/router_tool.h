/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * Author: Maciej Suminski <maciej.suminski@cern.ch>
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

#ifndef __ROUTER_TOOL_H
#define __ROUTER_TOOL_H

#include "pns_tool_base.h"

class APIEXPORT ROUTER_TOOL : public PNS_TOOL_BASE
{
public:
    ROUTER_TOOL();
    ~ROUTER_TOOL();

    void Reset( RESET_REASON aReason );

    int RouteSingleTrace ( const TOOL_EVENT& aEvent );
    int RouteDiffPair ( const TOOL_EVENT& aEvent );
    int InlineDrag ( const TOOL_EVENT& aEvent );

    int DpDimensionsDialog ( const TOOL_EVENT& aEvent );
    int SettingsDialog ( const TOOL_EVENT& aEvent );

private:

    int mainLoop( PNS_ROUTER_MODE aMode );

    int getDefaultWidth( int aNetCode );

    void performRouting();
    void performDragging();

    void getNetclassDimensions( int aNetCode, int& aWidth, int& aViaDiameter, int& aViaDrill );
    void handleCommonEvents( const TOOL_EVENT& evt );

    int getStartLayer( const PNS_ITEM* aItem );
    void switchLayerOnViaPlacement();
    bool onViaCommand( VIATYPE_T aType );

    bool prepareInteractive( );
    bool finishInteractive( bool aSaveUndoBuffer );
};

#endif

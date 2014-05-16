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

#include <set>
#include <boost/shared_ptr.hpp>

#include <import_export.h>

#include <math/vector2d.h>
#include <tool/tool_interactive.h>

#include <wxstruct.h>
#include <msgpanel.h>

#include "pns_layerset.h"
#include "pns_routing_settings.h"

class PNS_ROUTER;
class PNS_ITEM;

class APIEXPORT ROUTER_TOOL : public TOOL_INTERACTIVE
{
public:
    ROUTER_TOOL();
    ~ROUTER_TOOL();

    void Reset( RESET_REASON aReason );
    int Main( TOOL_EVENT& aEvent );

private:
    PNS_ITEM* pickSingleItem( const VECTOR2I& aWhere, int aNet = -1, int aLayer = -1 );

    int getDefaultWidth( int aNetCode );
    
    void performRouting();
    void performDragging();
    
    void highlightNet( bool aEnabled, int aNetcode = -1 );

    void updateStartItem( TOOL_EVENT& aEvent );
    void updateEndItem( TOOL_EVENT& aEvent );

    void getNetclassDimensions( int aNetCode, int& aWidth, int& aViaDiameter, int& aViaDrill );

    void handleCommonEvents( TOOL_EVENT& evt );

    MSG_PANEL_ITEMS m_panelItems;

    PNS_ROUTER* m_router;
    PNS_ROUTING_SETTINGS m_settings;     ///< Stores routing settings between router invocations

    PNS_ITEM* m_startItem;
    int m_startLayer;
    VECTOR2I m_startSnapPoint;

    PNS_ITEM* m_endItem;
    VECTOR2I m_endSnapPoint;

    CONTEXT_MENU* m_menu;

    ///> Flag marking that the router's world needs syncing.
    bool m_needsSync;
};

#endif

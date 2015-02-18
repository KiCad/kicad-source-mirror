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

#ifndef __PNS_TOOL_BASE_H
#define __PNS_TOOL_BASE_H

#include <import_export.h>

#include <math/vector2d.h>
#include <tool/tool_interactive.h>

#include <msgpanel.h>

#include "pns_router.h"

class PNS_TUNE_STATUS_POPUP;

class APIEXPORT PNS_TOOL_BASE : public TOOL_INTERACTIVE
{
public:
    static TOOL_ACTION ACT_RouterOptions;

    PNS_TOOL_BASE( const std::string& aToolName );
    virtual ~PNS_TOOL_BASE();

    virtual void Reset( RESET_REASON aReason );

protected:

    virtual PNS_ITEM* pickSingleItem( const VECTOR2I& aWhere, int aNet = -1, int aLayer = -1 );
    virtual void highlightNet( bool aEnabled, int aNetcode = -1 );
    virtual void updateStartItem( TOOL_EVENT& aEvent );
    virtual void updateEndItem( TOOL_EVENT& aEvent );
    
    MSG_PANEL_ITEMS m_panelItems;

    PNS_ROUTER* m_router;
    PNS_ROUTING_SETTINGS m_savedSettings;     ///< Stores routing settings between router invocations
    PNS_SIZES_SETTINGS m_savedSizes;          ///< Stores sizes settings between router invocations
    PNS_ITEM* m_startItem;
    int m_startLayer;
    VECTOR2I m_startSnapPoint;

    PNS_ITEM* m_endItem;
    VECTOR2I m_endSnapPoint;

    ///> Flag marking that the router's world needs syncing.
    bool m_needsSync;
    
    PCB_EDIT_FRAME *m_frame;
    KIGFX::VIEW_CONTROLS *m_ctls;
    BOARD *m_board;

};

#endif

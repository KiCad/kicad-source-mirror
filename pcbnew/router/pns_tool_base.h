/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <memory>
#include <import_export.h>

#include <math/vector2d.h>
#include <tools/pcb_tool_base.h>
#include <board_commit.h>

#include <msgpanel.h>

#include "pns_router.h"

class GRID_HELPER;

class PNS_KICAD_IFACE;
class PNS_TUNE_STATUS_POPUP;

namespace PNS {

class APIEXPORT TOOL_BASE : public PCB_TOOL_BASE
{
public:
    TOOL_BASE( const std::string& aToolName );
    virtual ~TOOL_BASE();

    virtual void Reset( RESET_REASON aReason ) override;

    ROUTER* Router() const;

protected:
    bool checkSnap( ITEM* aItem );
    const VECTOR2I snapToItem( bool aEnabled, ITEM* aItem, VECTOR2I aP);
    virtual ITEM* pickSingleItem( const VECTOR2I& aWhere, int aNet = -1, int aLayer = -1,
                                  bool aIgnorePads = false, const std::vector<ITEM*> aAvoidItems = {} );
    virtual void highlightNet( bool aEnabled, int aNetcode = -1 );
    virtual void updateStartItem( const TOOL_EVENT& aEvent, bool aIgnorePads = false );
    virtual void updateEndItem( const TOOL_EVENT& aEvent );
    void deleteTraces( ITEM* aStartItem, bool aWholeTrack );

    MSG_PANEL_ITEMS m_panelItems;

    ROUTING_SETTINGS m_savedSettings;     ///< Stores routing settings between router invocations
    SIZES_SETTINGS m_savedSizes;          ///< Stores sizes settings between router invocations
    ITEM* m_startItem;
    int m_startLayer;
    VECTOR2I m_startSnapPoint;
    bool m_startHighlight;                ///< Keeps track of whether the net was highlighted before routing

    ITEM* m_endItem;
    VECTOR2I m_endSnapPoint;

    GRID_HELPER* m_gridHelper;
    PNS_KICAD_IFACE* m_iface;
    ROUTER* m_router;

    bool m_cancelled;
};

}

#endif

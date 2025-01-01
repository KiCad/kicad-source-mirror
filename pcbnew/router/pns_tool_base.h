/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2014 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <math/vector2d.h>
#include <tools/pcb_tool_base.h>
#include <board_commit.h>

#include <widgets/msgpanel.h>

#include "pns_router.h"

class PCB_GRID_HELPER;

class PNS_KICAD_IFACE;
class PNS_TUNE_STATUS_POPUP;

namespace PNS
{

class TOOL_BASE : public PCB_TOOL_BASE
{
public:
    TOOL_BASE( const std::string& aToolName );
    virtual ~TOOL_BASE();

    virtual void Reset( RESET_REASON aReason ) override;

    ROUTER* Router() const;

    PNS_KICAD_IFACE* GetInterface() const;

protected:
    bool checkSnap( ITEM* aItem );

    const VECTOR2I snapToItem( ITEM* aSnapToItem, const VECTOR2I& aP);

    virtual ITEM* pickSingleItem( const VECTOR2I& aWhere, NET_HANDLE aNet = nullptr,
                                  int aLayer = -1, bool aIgnorePads = false,
                                  const std::vector<ITEM*> aAvoidItems = {} );

    virtual void highlightNets( bool aEnabled, std::set<NET_HANDLE> aNetcodes = {} );

    virtual void updateStartItem( const TOOL_EVENT& aEvent, bool aIgnorePads = false );
    virtual void updateEndItem( const TOOL_EVENT& aEvent );

    SIZES_SETTINGS   m_savedSizes;       // Stores sizes settings between router invocations
    ITEM*            m_startItem;
    VECTOR2I         m_startSnapPoint;
    std::set<int>    m_startHighlightNetcodes; // The set of nets highlighted before routing

    ITEM*            m_endItem;
    VECTOR2I         m_endSnapPoint;

    PCB_GRID_HELPER* m_gridHelper;
    PNS_KICAD_IFACE* m_iface;
    ROUTER*          m_router;

    bool             m_cancelled;

    static const unsigned int COORDS_PADDING; // Padding from coordinates limits for this tool
};

}

#endif

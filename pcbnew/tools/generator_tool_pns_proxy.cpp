/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Alex Shvartzkop <dudesuchamazing@gmail.com>
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

#include "generator_tool_pns_proxy.h"

#include <pad.h>
#include <pcb_shape.h>
#include <tools/pcb_grid_helper.h>

#include <router/pns_kicad_iface.h>
#include <router/pns_solid.h>
#include <router/pns_router.h>


class PNS_KICAD_IFACE_GENERATOR : public PNS_KICAD_IFACE
{
public:
    void SetHostTool( PCB_TOOL_BASE* aTool ) override
    {
        m_tool = aTool;
        m_commit = nullptr;

        ClearCommits();
    }

    void Commit() override
    { //
        m_changes.emplace_back();
    }

    void ClearCommits()
    {
        m_changes.clear();
        m_changes.emplace_back();
    }

    void AddItem( PNS::ITEM* aItem ) override
    {
        BOARD_ITEM* brdItem = createBoardItem( aItem );

        if( brdItem )
        {
            aItem->SetParent( brdItem );
            brdItem->ClearFlags();

            m_changes.back().addedItems.emplace( brdItem );
        }
    }

    void UpdateItem( PNS::ITEM* aItem ) override
    { //
        modifyBoardItem( aItem );
    }

    void RemoveItem( PNS::ITEM* aItem ) override
    {
        BOARD_ITEM* parent = aItem->Parent();

        if( aItem->OfKind( PNS::ITEM::SOLID_T ) )
        {
            PAD*     pad = static_cast<PAD*>( parent );
            VECTOR2I pos = static_cast<PNS::SOLID*>( aItem )->Pos();

            m_fpOffsets[pad].p_old = pos;
            return;
        }

        if( parent )
        {
            m_changes.back().removedItems.emplace( parent );
        }
    }

    std::vector<GENERATOR_PNS_CHANGES>& Changes() { return m_changes; };

private:
    std::vector<GENERATOR_PNS_CHANGES> m_changes;
};


void GENERATOR_TOOL_PNS_PROXY::ClearRouterChanges()
{
    static_cast<PNS_KICAD_IFACE_GENERATOR*>( GetInterface() )->ClearCommits();
}


const std::vector<GENERATOR_PNS_CHANGES>& GENERATOR_TOOL_PNS_PROXY::GetRouterChanges()
{
    return static_cast<PNS_KICAD_IFACE_GENERATOR*>( GetInterface() )->Changes();
}


GENERATOR_TOOL_PNS_PROXY::GENERATOR_TOOL_PNS_PROXY( const std::string& aToolName ) :
        PNS::TOOL_BASE( aToolName )
{
}


GENERATOR_TOOL_PNS_PROXY::~GENERATOR_TOOL_PNS_PROXY()
{
}


void GENERATOR_TOOL_PNS_PROXY::Reset( RESET_REASON aReason )
{
    delete m_gridHelper;
    delete m_router;
    delete m_iface; // Delete after m_router because PNS::NODE dtor needs m_ruleResolver

    if( aReason == RESET_REASON::SHUTDOWN )
    {
        m_iface = nullptr;
        m_router = nullptr;
        m_gridHelper = nullptr;
        return;
    }

    m_iface = new PNS_KICAD_IFACE_GENERATOR;
    m_iface->SetBoard( board() );
    m_iface->SetView( getView() );
    m_iface->SetHostTool( this );

    m_router = new PNS::ROUTER;
    m_router->SetInterface( m_iface );
    m_router->ClearWorld();
    m_router->SyncWorld();

    m_router->UpdateSizes( m_savedSizes );

    PCBNEW_SETTINGS* settings = frame()->GetPcbNewSettings();

    if( !settings->m_PnsSettings )
        settings->m_PnsSettings = std::make_unique<PNS::ROUTING_SETTINGS>( settings, "tools.pns" );

    m_router->LoadSettings( settings->m_PnsSettings.get() );

    m_gridHelper = new PCB_GRID_HELPER( m_toolMgr, frame()->GetMagneticItemsSettings() );
}
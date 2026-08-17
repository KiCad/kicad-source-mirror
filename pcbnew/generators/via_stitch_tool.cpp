/*
 * This program source code file is part of KiCad, a free EDA CAD application.
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
 */

#include "via_stitch_tool.h"

#include <board_commit.h>
#include <pcb_track.h>
#include <generators/pcb_via_stitch.h>
#include <tool/conditional_menu.h>
#include <tool/tool_manager.h>
#include <tool/tool_menu.h>
#include <tools/generator_tool.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>


bool VIA_STITCH_TOOL::Init()
{
    auto hasExclusionsCondition = []( const SELECTION& aSel )
    {
        for( EDA_ITEM* item : aSel )
        {
            if( PCB_VIA_STITCH* stitch = dynamic_cast<PCB_VIA_STITCH*>( item ) )
            {
                if( stitch->HasExclusions() )
                    return true;
            }
        }

        return false;
    };

    if( PCB_SELECTION_TOOL* selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>() )
    {
        CONDITIONAL_MENU& menu = selTool->GetToolMenu().GetMenu();

        menu.AddItem( PCB_ACTIONS::clearStitchViaExclusions, hasExclusionsCondition, 100 );
    }

    return true;
}


int VIA_STITCH_TOOL::ExcludeStitchVia( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION& sel = selTool->GetSelection();

    BOARD_COMMIT              commit( this );
    std::set<PCB_VIA_STITCH*> toRegenerate;

    for( EDA_ITEM* item : sel )
    {
        if( item->Type() != PCB_VIA_T )
            continue;

        PCB_VIA*   via = static_cast<PCB_VIA*>( item );
        EDA_GROUP* parent = via->GetParentGroup();

        if( !parent )
            continue;

        PCB_VIA_STITCH* stitch = dynamic_cast<PCB_VIA_STITCH*>( parent->AsEdaItem() );

        if( !stitch )
            continue;

        if( commit.GetStatus( stitch ) != CHT_MODIFY )
            commit.Modify( stitch );

        stitch->ExcludePosition( via->GetPosition() );
        toRegenerate.insert( stitch );
    }

    if( toRegenerate.empty() )
        return 0;

    selTool->ClearSelection();

    GENERATOR_TOOL* genTool = m_toolMgr->GetTool<GENERATOR_TOOL>();

    for( PCB_VIA_STITCH* stitch : toRegenerate )
    {
        stitch->EditStart( genTool, board(), &commit );
        stitch->Update( genTool, board(), &commit );
        stitch->EditFinish( genTool, board(), &commit );
    }

    commit.Push( _( "Exclude Via From Stitching" ) );
    frame()->RefreshCanvas();
    return 0;
}


int VIA_STITCH_TOOL::ClearStitchViaExclusions( const TOOL_EVENT& aEvent )
{
    PCB_SELECTION_TOOL*  selTool = m_toolMgr->GetTool<PCB_SELECTION_TOOL>();
    const PCB_SELECTION& sel = selTool->GetSelection();

    BOARD_COMMIT              commit( this );
    std::set<PCB_VIA_STITCH*> toRegenerate;

    for( EDA_ITEM* item : sel )
    {
        PCB_VIA_STITCH* stitch = dynamic_cast<PCB_VIA_STITCH*>( item );

        if( !stitch || !stitch->HasExclusions() )
            continue;

        if( commit.GetStatus( stitch ) != CHT_MODIFY )
            commit.Modify( stitch );

        stitch->ClearAllExclusions();
        toRegenerate.insert( stitch );
    }

    if( toRegenerate.empty() )
        return 0;

    GENERATOR_TOOL* genTool = m_toolMgr->GetTool<GENERATOR_TOOL>();

    for( PCB_VIA_STITCH* stitch : toRegenerate )
    {
        stitch->EditStart( genTool, board(), &commit );
        stitch->Update( genTool, board(), &commit );
        stitch->EditFinish( genTool, board(), &commit );
    }

    commit.Push( _( "Clear Via Stitching Exclusions" ) );
    frame()->RefreshCanvas();
    return 0;
}


void VIA_STITCH_TOOL::setTransitions()
{
    Go( &VIA_STITCH_TOOL::ExcludeStitchVia, PCB_ACTIONS::excludeStitchVia.MakeEvent() );
    Go( &VIA_STITCH_TOOL::ClearStitchViaExclusions, PCB_ACTIONS::clearStitchViaExclusions.MakeEvent() );
}

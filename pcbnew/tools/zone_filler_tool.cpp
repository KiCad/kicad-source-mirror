/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright (C) 2014-2018 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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
#include <cstdint>
#include <thread>
#include <mutex>

#include <class_zone.h>
#include <class_module.h>
#include <connectivity/connectivity_data.h>
#include <board_commit.h>

#include <widgets/progress_reporter.h>
#include <tool/tool_manager.h>
#include <bitmaps.h>
#include <hotkeys.h>

#include "pcb_actions.h"
#include "selection_tool.h"
#include "zone_filler_tool.h"
#include "zone_filler.h"

// Zone actions
TOOL_ACTION PCB_ACTIONS::zoneFill( "pcbnew.ZoneFiller.zoneFill",
        AS_GLOBAL, 0,
        _( "Fill" ), _( "Fill zone(s)" ), fill_zone_xpm );

TOOL_ACTION PCB_ACTIONS::zoneFillAll( "pcbnew.ZoneFiller.zoneFillAll",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZONE_FILL_OR_REFILL ),
        _( "Fill All" ), _( "Fill all zones" ) );

TOOL_ACTION PCB_ACTIONS::zoneUnfill( "pcbnew.ZoneFiller.zoneUnfill",
        AS_GLOBAL, 0,
        _( "Unfill" ), _( "Unfill zone(s)" ), zone_unfill_xpm );

TOOL_ACTION PCB_ACTIONS::zoneUnfillAll( "pcbnew.ZoneFiller.zoneUnfillAll",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZONE_REMOVE_FILLED ),
        _( "Unfill All" ), _( "Unfill all zones" ) );

TOOL_ACTION PCB_ACTIONS::zoneDeleteSegzone( "pcbnew.ZoneFiller.zoneDeleteSegzone",
        AS_GLOBAL, 0,
        _( "Delete Zone Filling" ), _( "Delete Zone Filling" ), delete_xpm );

ZONE_FILLER_TOOL::ZONE_FILLER_TOOL() :
    PCB_TOOL( "pcbnew.ZoneFiller" )
{
}


ZONE_FILLER_TOOL::~ZONE_FILLER_TOOL()
{
}


void ZONE_FILLER_TOOL::Reset( RESET_REASON aReason )
{
}

// Zone actions
int ZONE_FILLER_TOOL::ZoneFill( const TOOL_EVENT& aEvent )
{
    std::vector<ZONE_CONTAINER*> toFill;

    BOARD_COMMIT commit( this );

    for( auto item : selection() )
    {
        assert( item->Type() == PCB_ZONE_AREA_T );

        ZONE_CONTAINER* zone = static_cast<ZONE_CONTAINER*> ( item );

        toFill.push_back(zone);
    }

    std::unique_ptr<WX_PROGRESS_REPORTER> progressReporter(
            new WX_PROGRESS_REPORTER( frame(), _( "Fill Zone" ), 4 )
            );

    ZONE_FILLER filler( board(), &commit );
    filler.SetProgressReporter( progressReporter.get() );
    filler.Fill( toFill );

    canvas()->Refresh();

    return 0;
}


int ZONE_FILLER_TOOL::ZoneFillAll( const TOOL_EVENT& aEvent )
{
    std::vector<ZONE_CONTAINER*> toFill;

    BOARD_COMMIT commit( this );

    for( auto zone : board()->Zones() )
    {
        toFill.push_back(zone);
    }

    std::unique_ptr<WX_PROGRESS_REPORTER> progressReporter(
            new WX_PROGRESS_REPORTER( frame(), _( "Fill All Zones" ), 4 )
            );

    ZONE_FILLER filler( board(), &commit );
    filler.SetProgressReporter( progressReporter.get() );

    if( filler.Fill( toFill ) )
        frame()->m_ZoneFillsDirty = false;

    canvas()->Refresh();

    return 0;
}


int ZONE_FILLER_TOOL::ZoneUnfill( const TOOL_EVENT& aEvent )
{
    BOARD_COMMIT commit( this );

    for( auto item : selection() )
    {
        assert( item->Type() == PCB_ZONE_AREA_T );

        ZONE_CONTAINER* zone = static_cast<ZONE_CONTAINER*>( item );

        commit.Modify( zone );

        zone->SetIsFilled( false );
        zone->ClearFilledPolysList();
    }

    commit.Push( _( "Unfill Zone" ) );
    canvas()->Refresh();

    return 0;
}


int ZONE_FILLER_TOOL::SegzoneDeleteFill( const TOOL_EVENT& aEvent )
{
    BOARD_COMMIT commit( this );
    BOARD* board = (BOARD*) m_toolMgr->GetModel();

    for( auto item : selection() )
    {
        assert( item->Type() == PCB_SEGZONE_T );

        timestamp_t timestamp = item->GetTimeStamp(); // Save reference time stamp (aZone will be deleted)
        SEGZONE* next;

        for( SEGZONE* zone = board->m_SegZoneDeprecated; zone; zone = next )
        {
            next = zone->Next();

            if( timestamp == zone->GetTimeStamp() )
                commit.Remove( zone );
        }
    }

    commit.Push( _( "Delete Zone Filling" ) );
    canvas()->Refresh();

    return 0;
}


int ZONE_FILLER_TOOL::ZoneUnfillAll( const TOOL_EVENT& aEvent )
{
    BOARD_COMMIT commit( this );

    for ( auto zone : board()->Zones() )
    {
        commit.Modify( zone );

        zone->SetIsFilled( false );
        zone->ClearFilledPolysList();
    }

    commit.Push( _( "Unfill All Zones" ) );
    canvas()->Refresh();

    return 0;
}


void ZONE_FILLER_TOOL::setTransitions()
{
    // Zone actions
    Go( &ZONE_FILLER_TOOL::ZoneFill, PCB_ACTIONS::zoneFill.MakeEvent() );
    Go( &ZONE_FILLER_TOOL::ZoneFillAll, PCB_ACTIONS::zoneFillAll.MakeEvent() );
    Go( &ZONE_FILLER_TOOL::ZoneUnfill, PCB_ACTIONS::zoneUnfill.MakeEvent() );
    Go( &ZONE_FILLER_TOOL::ZoneUnfillAll, PCB_ACTIONS::zoneUnfillAll.MakeEvent() );
    Go( &ZONE_FILLER_TOOL::SegzoneDeleteFill, PCB_ACTIONS::zoneDeleteSegzone.MakeEvent() );
}

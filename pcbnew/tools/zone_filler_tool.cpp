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
#include <class_zone.h>
#include <connectivity/connectivity_data.h>
#include <board_commit.h>
#include <widgets/progress_reporter.h>
#include <wx/event.h>
#include <tool/tool_manager.h>
#include <bitmaps.h>
#include "pcb_actions.h"
#include "selection_tool.h"
#include "zone_filler_tool.h"
#include "zone_filler.h"


ZONE_FILLER_TOOL::ZONE_FILLER_TOOL() :
    PCB_TOOL_BASE( "pcbnew.ZoneFiller" )
{
}


ZONE_FILLER_TOOL::~ZONE_FILLER_TOOL()
{
}


void ZONE_FILLER_TOOL::Reset( RESET_REASON aReason )
{
}


void ZONE_FILLER_TOOL::CheckAllZones( wxWindow* aCaller )
{
    if( !getEditFrame<PCB_EDIT_FRAME>()->m_ZoneFillsDirty )
        return;

    std::vector<ZONE_CONTAINER*> toFill;

    for( auto zone : board()->Zones() )
        toFill.push_back(zone);

    BOARD_COMMIT commit( this );

    ZONE_FILLER filler( frame()->GetBoard(), &commit );
    filler.InstallNewProgressReporter( aCaller, _( "Checking Zones" ), 4 );

    if( filler.Fill( toFill, true ) )
    {
        getEditFrame<PCB_EDIT_FRAME>()->m_ZoneFillsDirty = false;
        canvas()->Refresh();
    }
}


void ZONE_FILLER_TOOL::singleShotRefocus( wxIdleEvent& )
{
    canvas()->SetFocus();
    canvas()->Unbind( wxEVT_IDLE, &ZONE_FILLER_TOOL::singleShotRefocus, this );
}


void ZONE_FILLER_TOOL::FillAllZones( wxWindow* aCaller )
{
    std::vector<ZONE_CONTAINER*> toFill;

    BOARD_COMMIT commit( this );

    for( auto zone : board()->Zones() )
        toFill.push_back(zone);

    ZONE_FILLER filler( board(), &commit );
    filler.InstallNewProgressReporter( aCaller, _( "Fill All Zones" ),  4 );

    if( filler.Fill( toFill ) )
        getEditFrame<PCB_EDIT_FRAME>()->m_ZoneFillsDirty = false;

    canvas()->Refresh();

    // wxWidgets has keyboard focus issues after the progress reporter.  Re-setting the focus
    // here doesn't work, so we delay it to an idle event.
    canvas()->Bind( wxEVT_IDLE, &ZONE_FILLER_TOOL::singleShotRefocus, this );
}


int ZONE_FILLER_TOOL::ZoneFill( const TOOL_EVENT& aEvent )
{
    std::vector<ZONE_CONTAINER*> toFill;

    BOARD_COMMIT commit( this );

    if( auto passedZone = aEvent.Parameter<ZONE_CONTAINER*>() )
    {
        if( passedZone->Type() == PCB_ZONE_AREA_T )
            toFill.push_back( passedZone );
    }
    else
    {
        for( auto item : selection() )
        {
            if( auto zone = dyn_cast<ZONE_CONTAINER*>( item ) )
                toFill.push_back( zone );
        }
    }

    ZONE_FILLER filler( board(), &commit );
    filler.InstallNewProgressReporter( frame(), _( "Fill Zone" ), 4 );
    filler.Fill( toFill );

    canvas()->Refresh();
    return 0;
}


int ZONE_FILLER_TOOL::ZoneFillAll( const TOOL_EVENT& aEvent )
{
    FillAllZones( frame() );
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
}

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
#include <zone.h>
#include <connectivity/connectivity_data.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <progress_reporter.h>
#include <widgets/infobar.h>
#include <widgets/wx_progress_reporters.h>
#include <wx/event.h>
#include <wx/hyperlink.h>
#include <tool/tool_manager.h>
#include "pcb_actions.h"
#include "zone_filler_tool.h"
#include "zone_filler.h"


ZONE_FILLER_TOOL::ZONE_FILLER_TOOL() :
    PCB_TOOL_BASE( "pcbnew.ZoneFiller" ),
    m_fillInProgress( false )
{
}


ZONE_FILLER_TOOL::~ZONE_FILLER_TOOL()
{
}


void ZONE_FILLER_TOOL::Reset( RESET_REASON aReason )
{
}


void ZONE_FILLER_TOOL::CheckAllZones( wxWindow* aCaller, PROGRESS_REPORTER* aReporter )
{
    if( !getEditFrame<PCB_EDIT_FRAME>()->m_ZoneFillsDirty || m_fillInProgress )
        return;

    m_fillInProgress = true;

    std::vector<ZONE*> toFill;

    for( ZONE* zone : board()->Zones() )
        toFill.push_back(zone);

    BOARD_COMMIT                          commit( this );
    std::unique_ptr<WX_PROGRESS_REPORTER> reporter;
    ZONE_FILLER                           filler( frame()->GetBoard(), &commit );

    if( aReporter )
    {
        filler.SetProgressReporter( aReporter );
    }
    else
    {
        reporter = std::make_unique<WX_PROGRESS_REPORTER>( aCaller, _( "Checking Zones" ), 4 );
        filler.SetProgressReporter( reporter.get() );
    }

    if( filler.Fill( toFill, true, aCaller ) )
    {
        board()->GetConnectivity()->Build( board() );
        commit.Push( _( "Fill Zone(s)" ), false );
        getEditFrame<PCB_EDIT_FRAME>()->m_ZoneFillsDirty = false;
    }
    else
    {
        commit.Revert();
    }

    canvas()->Refresh();
    m_fillInProgress = false;
}


void ZONE_FILLER_TOOL::singleShotRefocus( wxIdleEvent& )
{
    canvas()->SetFocus();
    canvas()->Unbind( wxEVT_IDLE, &ZONE_FILLER_TOOL::singleShotRefocus, this );
}


void ZONE_FILLER_TOOL::FillAllZones( wxWindow* aCaller, PROGRESS_REPORTER* aReporter )
{
    PCB_EDIT_FRAME*    frame = getEditFrame<PCB_EDIT_FRAME>();
    std::vector<ZONE*> toFill;

    if( m_fillInProgress )
        return;

    m_fillInProgress = true;

    for( ZONE* zone : board()->Zones() )
        toFill.push_back( zone );

    board()->IncrementTimeStamp();    // Clear caches

    BOARD_COMMIT                          commit( this );
    std::unique_ptr<WX_PROGRESS_REPORTER> reporter;
    ZONE_FILLER                           filler( board(), &commit );

    if( !board()->GetDesignSettings().m_DRCEngine->RulesValid() )
    {
        WX_INFOBAR* infobar = frame->GetInfoBar();
        wxHyperlinkCtrl* button = new wxHyperlinkCtrl( infobar, wxID_ANY, _("Show DRC rules"),
                                                       wxEmptyString );

        button->Bind( wxEVT_COMMAND_HYPERLINK,
                      std::function<void( wxHyperlinkEvent& aEvent )>(
                              [frame]( wxHyperlinkEvent& aEvent )
                              {
                                  frame->ShowBoardSetupDialog( _( "Rules" ) );
                              } ) );

        infobar->RemoveAllButtons();
        infobar->AddButton( button );

        infobar->ShowMessageFor( _( "Zone fills may be inaccurate.  DRC rules contain errors." ),
                                 10000, wxICON_WARNING );
    }

    if( aReporter )
    {
        filler.SetProgressReporter( aReporter );
    }
    else
    {
        reporter = std::make_unique<WX_PROGRESS_REPORTER>( aCaller, _( "Fill All Zones" ), 3 );
        filler.SetProgressReporter( reporter.get() );
    }

    {
        if( filler.Fill( toFill ) )
        {
            board()->GetConnectivity()->Build( board() );
            commit.Push( _( "Fill Zone(s)" ), true ); // Allow undoing zone fill
            frame->m_ZoneFillsDirty = false;
        }
        else
        {
            commit.Revert();
        }

        if( filler.IsDebug() )
            frame->UpdateUserInterface();
    }

    canvas()->Refresh();
    m_fillInProgress = false;

    // wxWidgets has keyboard focus issues after the progress reporter.  Re-setting the focus
    // here doesn't work, so we delay it to an idle event.
    canvas()->Bind( wxEVT_IDLE, &ZONE_FILLER_TOOL::singleShotRefocus, this );
}


int ZONE_FILLER_TOOL::ZoneFill( const TOOL_EVENT& aEvent )
{
    if( m_fillInProgress )
    {
        wxBell();
        return -1;
    }

    m_fillInProgress = true;

    std::vector<ZONE*> toFill;

    if( ZONE* passedZone = aEvent.Parameter<ZONE*>() )
    {
        toFill.push_back( passedZone );
    }
    else
    {
        for( EDA_ITEM* item : selection() )
        {
            if( ZONE* zone = dynamic_cast<ZONE*>( item ) )
                toFill.push_back( zone );
        }
    }

    BOARD_COMMIT                          commit( this );
    std::unique_ptr<WX_PROGRESS_REPORTER> reporter;
    ZONE_FILLER                           filler( board(), &commit );

    reporter = std::make_unique<WX_PROGRESS_REPORTER>( frame(), _( "Fill Zone" ), 4 );
    filler.SetProgressReporter( reporter.get() );

    if( filler.Fill( toFill ) )
    {
        board()->GetConnectivity()->Build( board() );
        commit.Push( _( "Fill Zone(s)" ), true );  // Allow undoing zone fill
    }
    else
        commit.Revert();

    canvas()->Refresh();
    m_fillInProgress = false;
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

    for( EDA_ITEM* item : selection() )
    {
        assert( item->Type() == PCB_ZONE_T || item->Type() == PCB_FP_ZONE_T );

        ZONE* zone = static_cast<ZONE*>( item );

        commit.Modify( zone );

        zone->UnFill();
    }

    commit.Push( _( "Unfill Zone" ) );
    canvas()->Refresh();

    return 0;
}


int ZONE_FILLER_TOOL::ZoneUnfillAll( const TOOL_EVENT& aEvent )
{
    BOARD_COMMIT commit( this );

    for( ZONE* zone : board()->Zones() )
    {
        commit.Modify( zone );

        zone->UnFill();
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

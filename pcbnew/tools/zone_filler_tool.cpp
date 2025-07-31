/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <footprint.h>
#include <pcb_track.h>
#include <pad.h>
#include <pcb_group.h>
#include <board_design_settings.h>
#include <progress_reporter.h>
#include <widgets/wx_infobar.h>
#include <widgets/wx_progress_reporters.h>
#include <wx/event.h>
#include <wx/hyperlink.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <tools/pcb_selection_tool.h>
#include "pcb_actions.h"
#include "zone_filler_tool.h"
#include "zone_filler.h"
#include "teardrop/teardrop.h"
#include <core/profile.h>

ZONE_FILLER_TOOL::ZONE_FILLER_TOOL() :
    PCB_TOOL_BASE( ZONE_FILLER_TOOL_NAME ),
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
        toFill.push_back( zone );

    BOARD_COMMIT                          commit( this );
    std::unique_ptr<WX_PROGRESS_REPORTER> reporter;

    m_filler = std::make_unique<ZONE_FILLER>( frame()->GetBoard(), &commit );

    if( aReporter )
    {
        m_filler->SetProgressReporter( aReporter );
    }
    else
    {
        reporter = std::make_unique<WX_PROGRESS_REPORTER>( aCaller, _( "Check Zones" ), 4, PR_CAN_ABORT );
        m_filler->SetProgressReporter( reporter.get() );
    }

    if( m_filler->Fill( toFill, true, aCaller ) )
    {
        commit.Push( _( "Fill Zone(s)" ), SKIP_CONNECTIVITY | ZONE_FILL_OP );
        getEditFrame<PCB_EDIT_FRAME>()->m_ZoneFillsDirty = false;
    }
    else
    {
        commit.Revert();
    }

    rebuildConnectivity();
    refresh();

    m_fillInProgress = false;
    m_filler.reset( nullptr );
}


void ZONE_FILLER_TOOL::singleShotRefocus( wxIdleEvent& )
{
    canvas()->SetFocus();
    canvas()->Unbind( wxEVT_IDLE, &ZONE_FILLER_TOOL::singleShotRefocus, this );
}


void ZONE_FILLER_TOOL::FillAllZones( wxWindow* aCaller, PROGRESS_REPORTER* aReporter, bool aHeadless )
{
    if( m_fillInProgress )
        return;

    m_fillInProgress = true;

    PCB_EDIT_FRAME*                       frame = nullptr;
    if( !aHeadless )
        frame = getEditFrame<PCB_EDIT_FRAME>();

    BOARD_COMMIT                          commit( this );
    std::unique_ptr<WX_PROGRESS_REPORTER> reporter;
    TEARDROP_MANAGER                      teardropMgr( board(), m_toolMgr );
    std::vector<ZONE*>                    toFill;

    teardropMgr.UpdateTeardrops( commit, nullptr, nullptr, true /* forceFullUpdate */ );

    board()->IncrementTimeStamp();    // Clear caches

    for( ZONE* zone : board()->Zones() )
        toFill.push_back( zone );

    m_filler = std::make_unique<ZONE_FILLER>( board(), &commit );

    if( !aHeadless && !board()->GetDesignSettings().m_DRCEngine->RulesValid() )
    {
        WX_INFOBAR* infobar = frame->GetInfoBar();
        wxHyperlinkCtrl* button = new wxHyperlinkCtrl( infobar, wxID_ANY, _( "Show DRC rules" ), wxEmptyString );

        button->Bind( wxEVT_COMMAND_HYPERLINK,
                      std::function<void( wxHyperlinkEvent& aEvent )>(
                      [frame]( wxHyperlinkEvent& aEvent )
                      {
                          frame->ShowBoardSetupDialog( _( "Rules" ) );
                      } ) );

        infobar->RemoveAllButtons();
        infobar->AddButton( button );

        infobar->ShowMessageFor( _( "Zone fills may be inaccurate.  DRC rules contain errors." ), 10000,
                                 wxICON_WARNING );
    }

    if( aReporter )
    {
        m_filler->SetProgressReporter( aReporter );
    }
    else if( !aHeadless )
    {
        reporter = std::make_unique<WX_PROGRESS_REPORTER>( aCaller, _( "Fill All Zones" ), 5, PR_CAN_ABORT );
        m_filler->SetProgressReporter( reporter.get() );
    }

    if( m_filler->Fill( toFill ) )
    {
        if( m_filler->GetProgressReporter() )
            m_filler->GetProgressReporter()->AdvancePhase();

        commit.Push( _( "Fill Zone(s)" ), SKIP_CONNECTIVITY | ZONE_FILL_OP );
        if( !aHeadless )
            frame->m_ZoneFillsDirty = false;
    }
    else
    {
        commit.Revert();
    }

    rebuildConnectivity( aHeadless );

    if( !aHeadless )
    {
        refresh();

        if( m_filler->IsDebug() )
            frame->UpdateUserInterface();
    }

    m_fillInProgress = false;
    m_filler.reset( nullptr );

    if( !aHeadless )
        // wxWidgets has keyboard focus issues after the progress reporter.  Re-setting the focus
        // here doesn't work, so we delay it to an idle event.
        canvas()->Bind( wxEVT_IDLE, &ZONE_FILLER_TOOL::singleShotRefocus, this );
}


int ZONE_FILLER_TOOL::ZoneFillDirty( const TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME*    frame = getEditFrame<PCB_EDIT_FRAME>();
    std::vector<ZONE*> toFill;

    for( ZONE* zone : board()->Zones() )
    {
        if( !zone->IsFilled() || m_dirtyZoneIDs.count( zone->m_Uuid ) )
            toFill.push_back( zone );
    }

    if( toFill.empty() )
        return 0;

    if( m_fillInProgress )
        return 0;

    int64_t startTime = GetRunningMicroSecs();
    m_fillInProgress = true;

    m_dirtyZoneIDs.clear();

    board()->IncrementTimeStamp();    // Clear caches

    BOARD_COMMIT                          commit( this );
    std::unique_ptr<WX_PROGRESS_REPORTER> reporter;
    int                                   pts = 0;

    m_filler = std::make_unique<ZONE_FILLER>( board(), &commit );

    if( !board()->GetDesignSettings().m_DRCEngine->RulesValid() )
    {
        WX_INFOBAR* infobar = frame->GetInfoBar();
        wxHyperlinkCtrl* button = new wxHyperlinkCtrl( infobar, wxID_ANY, _( "Show DRC rules" ), wxEmptyString );

        button->Bind( wxEVT_COMMAND_HYPERLINK,
                      std::function<void( wxHyperlinkEvent& aLocEvent )>(
                      [frame]( wxHyperlinkEvent& aLocEvent )
                      {
                          frame->ShowBoardSetupDialog( _( "Rules" ) );
                      } ) );

        infobar->RemoveAllButtons();
        infobar->AddButton( button );

        infobar->ShowMessageFor( _( "Zone fills may be inaccurate.  DRC rules contain errors." ), 10000,
                                 wxICON_WARNING );
    }

    for( ZONE* zone : toFill )
    {
        zone->GetLayerSet().RunOnLayers(
                [&]( PCB_LAYER_ID layer )
                {
                    pts += zone->GetFilledPolysList( layer )->FullPointCount();
                } );

        if( pts > 1000 )
        {
            wxString title = wxString::Format( _( "Refill %d Zones" ), (int) toFill.size() );

            reporter = std::make_unique<WX_PROGRESS_REPORTER>( frame, title, 5, PR_CAN_ABORT );
            m_filler->SetProgressReporter( reporter.get() );
            break;
        }
    }

    if( m_filler->Fill( toFill ) )
        commit.Push( _( "Auto-fill Zone(s)" ), APPEND_UNDO | SKIP_CONNECTIVITY | ZONE_FILL_OP );
    else
        commit.Revert();

    rebuildConnectivity();
    refresh();

    if( GetRunningMicroSecs() - startTime > 3000000 )   // 3 seconds
    {
        WX_INFOBAR* infobar = frame->GetInfoBar();

        wxHyperlinkCtrl* button = new wxHyperlinkCtrl( infobar, wxID_ANY, _( "Open Preferences" ),
                                                       wxEmptyString );

        button->Bind( wxEVT_COMMAND_HYPERLINK, std::function<void( wxHyperlinkEvent& )>(
                [this]( wxHyperlinkEvent& )
                {
                    getEditFrame<PCB_EDIT_FRAME>()->ShowPreferences( _( "Editing Options" ), _( "PCB Editor" ) );
                } ) );

        infobar->RemoveAllButtons();
        infobar->AddButton( button );
        infobar->ShowMessageFor( _( "Automatic refill of zones can be turned off in Preferences if it becomes "
                                    "too slow." ),
                                 10000, wxICON_INFORMATION, WX_INFOBAR::MESSAGE_TYPE::GENERIC );
    }

    if( m_filler->IsDebug() )
        frame->UpdateUserInterface();

    m_fillInProgress = false;
    m_filler.reset( nullptr );

    // wxWidgets has keyboard focus issues after the progress reporter.  Re-setting the focus
    // here doesn't work, so we delay it to an idle event.
    canvas()->Bind( wxEVT_IDLE, &ZONE_FILLER_TOOL::singleShotRefocus, this );

    return 0;
}


int ZONE_FILLER_TOOL::ZoneFill( const TOOL_EVENT& aEvent )
{
    if( m_fillInProgress )
    {
        wxBell();
        return -1;
    }

    std::vector<ZONE*> toFill;

    if( ZONE* passedZone = aEvent.Parameter<ZONE*>() )
    {
        toFill.push_back( passedZone );
    }
    else
    {
        const PCB_SELECTION& sel = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->RequestSelection(
                []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
                {
                } );

        for( EDA_ITEM* item : sel )
        {
            if( ZONE* zone = dynamic_cast<ZONE*>( item ) )
                toFill.push_back( zone );
        }
    }

    // Bail out of the filler if there is nothing to fill
    if( toFill.empty() )
    {
        wxBell();
        return -1;
    }

    m_fillInProgress = true;

    BOARD_COMMIT                          commit( this );
    std::unique_ptr<WX_PROGRESS_REPORTER> reporter;

    m_filler = std::make_unique<ZONE_FILLER>( board(), &commit );

    reporter = std::make_unique<WX_PROGRESS_REPORTER>( frame(), _( "Fill Zone" ), 5, PR_CAN_ABORT );
    m_filler->SetProgressReporter( reporter.get() );

    if( m_filler->Fill( toFill ) )
    {
        reporter->AdvancePhase();
        commit.Push( _( "Fill Zone(s)" ), SKIP_CONNECTIVITY | ZONE_FILL_OP );
    }
    else
    {
        commit.Revert();
    }

    rebuildConnectivity();
    refresh();

    m_fillInProgress = false;
    m_filler.reset( nullptr );
    return 0;
}


int ZONE_FILLER_TOOL::ZoneFillAll( const TOOL_EVENT& aEvent )
{
    FillAllZones( frame() );
    return 0;
}


int ZONE_FILLER_TOOL::ZoneUnfill( const TOOL_EVENT& aEvent )
{
    const PCB_SELECTION& sel = m_toolMgr->GetTool<PCB_SELECTION_TOOL>()->RequestSelection(
            []( const VECTOR2I& aPt, GENERAL_COLLECTOR& aCollector, PCB_SELECTION_TOOL* sTool )
            {
            } );

    std::vector<ZONE*> toUnfill;

    for( EDA_ITEM* item : sel )
    {
        if( ZONE* zone = dynamic_cast<ZONE*>( item ) )
            toUnfill.push_back( zone );
    }

    // Bail out if there are no zones
    if( toUnfill.empty() )
    {
        wxBell();
        return -1;
    }

    BOARD_COMMIT commit( this );

    for( ZONE* zone : toUnfill )
    {
        commit.Modify( zone );

        zone->UnFill();
    }

    commit.Push( _( "Unfill Zone" ), ZONE_FILL_OP );

    refresh();

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

    commit.Push( _( "Unfill All Zones" ), ZONE_FILL_OP );

    refresh();

    return 0;
}


PROGRESS_REPORTER* ZONE_FILLER_TOOL::GetProgressReporter()
{
    if( m_fillInProgress && m_filler )
        return m_filler->GetProgressReporter();
    else
        return nullptr;
}


void ZONE_FILLER_TOOL::rebuildConnectivity( bool aHeadless )
{
    board()->BuildConnectivity();
    m_toolMgr->PostEvent( EVENTS::ConnectivityChangedEvent );
    if( !aHeadless )
        canvas()->RedrawRatsnest();
}


void ZONE_FILLER_TOOL::refresh()
{
    // Note: KIGFX::REPAINT isn't enough for things that go from invisible to visible as
    // they won't be found in the view layer's itemset for re-painting.
    canvas()->GetView()->UpdateAllItemsConditionally( KIGFX::ALL,
            [&]( KIGFX::VIEW_ITEM* aItem ) -> bool
            {
                if( PCB_VIA* via = dynamic_cast<PCB_VIA*>( aItem ) )
                {
                    return via->GetRemoveUnconnected();
                }
                else if( PAD* pad = dynamic_cast<PAD*>( aItem ) )
                {
                    return pad->GetRemoveUnconnected();
                }

                return false;
            } );

    canvas()->Refresh();
}


bool ZONE_FILLER_TOOL::IsZoneFillAction( const TOOL_EVENT* aEvent )
{
    return aEvent->IsAction( &PCB_ACTIONS::zoneFill )
           || aEvent->IsAction( &PCB_ACTIONS::zoneFillAll )
           || aEvent->IsAction( &PCB_ACTIONS::zoneUnfill )
           || aEvent->IsAction( &PCB_ACTIONS::zoneUnfillAll );

    // Don't include zoneFillDirty; that's a system action not a user action
}


void ZONE_FILLER_TOOL::setTransitions()
{
    // Zone actions
    Go( &ZONE_FILLER_TOOL::ZoneFill,      PCB_ACTIONS::zoneFill.MakeEvent() );
    Go( &ZONE_FILLER_TOOL::ZoneFillAll,   PCB_ACTIONS::zoneFillAll.MakeEvent() );
    Go( &ZONE_FILLER_TOOL::ZoneFillDirty, PCB_ACTIONS::zoneFillDirty.MakeEvent() );
    Go( &ZONE_FILLER_TOOL::ZoneUnfill,    PCB_ACTIONS::zoneUnfill.MakeEvent() );
    Go( &ZONE_FILLER_TOOL::ZoneUnfillAll, PCB_ACTIONS::zoneUnfillAll.MakeEvent() );
}

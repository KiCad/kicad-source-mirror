/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2015  CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <boost/foreach.hpp>
#include <boost/optional.hpp>

#include "class_draw_panel_gal.h"
#include "class_board.h"

#include <wxPcbStruct.h>
#include <pcbnew_id.h>
#include <view/view_controls.h>
#include <pcb_painter.h>
#include <dialogs/dialog_pns_settings.h>
#include <dialogs/dialog_pns_length_tuning_settings.h>

#include <tool/context_menu.h>
#include <tools/common_actions.h>

#include "pns_segment.h"
#include "pns_router.h"
#include "pns_meander_placer.h" // fixme: move settings to separate header
#include "pns_tune_status_popup.h"

#include "length_tuner_tool.h"

#include "trace.h"

using namespace KIGFX;
using boost::optional;

static TOOL_ACTION ACT_StartTuning( "pcbnew.LengthTuner.StartTuning",
                                 AS_CONTEXT, 'X',
                                 "New Track", "Starts laying a new track.");
static TOOL_ACTION ACT_EndTuning( "pcbnew.LengthTuner.EndTuning",
                                 AS_CONTEXT, WXK_END,
                                 "End Track", "Stops laying the current meander.");

static TOOL_ACTION ACT_Settings( "pcbnew.LengthTuner.Settings",
                                      AS_CONTEXT, 'L',
                                      "Length Tuning Settings", "Sets the length tuning parameters for currently routed item.");

static TOOL_ACTION ACT_SpacingIncrease( "pcbnew.LengthTuner.SpacingIncrease",
                                      AS_CONTEXT, '1',
                                      "Increase spacing", "Increase meander spacing by one step.");

static TOOL_ACTION ACT_SpacingDecrease( "pcbnew.LengthTuner.SpacingDecrease",
                                      AS_CONTEXT, '2',
                                      "Decrease spacing ", "Decrease meander spacing by one step.");

static TOOL_ACTION ACT_AmplIncrease( "pcbnew.LengthTuner.AmplIncrease",
                                      AS_CONTEXT, '3',
                                      "Increase amplitude", "Increase meander amplitude by one step.");

static TOOL_ACTION ACT_AmplDecrease( "pcbnew.LengthTuner.AmplDecrease",
                                      AS_CONTEXT, '4',
                                      "Decrease amplitude", "Decrease meander amplitude by one step.");


LENGTH_TUNER_TOOL::LENGTH_TUNER_TOOL() :
    PNS_TOOL_BASE( "pcbnew.LengthTuner" )
{
}


class TUNER_TOOL_MENU: public CONTEXT_MENU
{
public:
    TUNER_TOOL_MENU( BOARD* aBoard )
    {
        SetTitle( wxT( "Length Tuner" ) );

        //Add( ACT_StartTuning );
        //Add( ACT_EndTuning );

        //AppendSeparator();

        Add( ACT_SpacingIncrease );
        Add( ACT_SpacingDecrease );
        Add( ACT_AmplIncrease );
        Add( ACT_AmplDecrease );
        Add( ACT_Settings );
    }
};


LENGTH_TUNER_TOOL::~LENGTH_TUNER_TOOL()
{
    delete m_router;
}


void LENGTH_TUNER_TOOL::Reset( RESET_REASON aReason )
{
    PNS_TOOL_BASE::Reset( aReason );

    Go( &LENGTH_TUNER_TOOL::TuneSingleTrace, COMMON_ACTIONS::routerActivateTuneSingleTrace.MakeEvent() );
    Go( &LENGTH_TUNER_TOOL::TuneDiffPair, COMMON_ACTIONS::routerActivateTuneDiffPair.MakeEvent() );
    Go( &LENGTH_TUNER_TOOL::TuneDiffPairSkew, COMMON_ACTIONS::routerActivateTuneDiffPairSkew.MakeEvent() );
}


void LENGTH_TUNER_TOOL::handleCommonEvents( const TOOL_EVENT& aEvent )
{
    if( aEvent.IsAction( &ACT_RouterOptions ) )
    {
        DIALOG_PNS_SETTINGS settingsDlg( m_frame, m_router->Settings() );

        if( settingsDlg.ShowModal() )
        {
            // FIXME: do we need an explicit update?
        }
    }

    PNS_MEANDER_PLACER_BASE* placer = static_cast<PNS_MEANDER_PLACER_BASE*>( m_router->Placer() );

    if( !placer )
        return;

    if( aEvent.IsAction( &ACT_Settings ) )
    {
        PNS_MEANDER_SETTINGS settings = placer->MeanderSettings();
        DIALOG_PNS_LENGTH_TUNING_SETTINGS settingsDlg( m_frame, settings, m_router->Mode() );

        if( settingsDlg.ShowModal() )
        {
            placer->UpdateSettings ( settings );
        }

        m_savedMeanderSettings = placer->MeanderSettings();
    }
}


void LENGTH_TUNER_TOOL::performTuning()
{
    bool saveUndoBuffer = true;

    if( m_startItem )
    {
        m_frame->SetActiveLayer( ToLAYER_ID ( m_startItem->Layers().Start() ) );

        if( m_startItem->Net() >= 0 )
            highlightNet( true, m_startItem->Net() );
    }

    m_ctls->ForceCursorPosition( false );
    m_ctls->SetAutoPan( true );

    if( !m_router->StartRouting( m_startSnapPoint, m_startItem, 0 ) )
    {
        wxMessageBox( m_router->FailureReason(), _( "Error" ) );
        highlightNet( false );
        return;
    }

    PNS_TUNE_STATUS_POPUP statusPopup( m_frame );
    statusPopup.Popup();

    PNS_MEANDER_PLACER* placer = static_cast<PNS_MEANDER_PLACER*>( m_router->Placer() );
    VECTOR2I end;

    placer->UpdateSettings( m_savedMeanderSettings );

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsCancel() || evt->IsActivate() )
            break;
        else if( evt->Action() == TA_UNDO_REDO )
        {
            saveUndoBuffer = false;
            break;
        }
        else if( evt->IsMotion() )
        {
            end = evt->Position();
            m_router->Move( end, NULL );

            wxPoint p = wxGetMousePosition();

            p.x += 20;
            p.y += 20;

            statusPopup.Update( m_router );
            statusPopup.Move( p );
        }
        else if( evt->IsClick( BUT_LEFT ) )
        {
            if( m_router->FixRoute( evt->Position(), NULL ) )
                break;
        }
        else if( evt->IsAction( &ACT_EndTuning ) )
        {
            if( m_router->FixRoute( end, NULL ) )
                break;
        }
        else if( evt->IsAction( &ACT_AmplDecrease ) )
        {
            placer->AmplitudeStep( -1 );
            m_router->Move( end, NULL );
        }
        else if( evt->IsAction( &ACT_AmplIncrease ) )
        {
            placer->AmplitudeStep( 1 );
            m_router->Move( end, NULL );
        }
        else if(evt->IsAction( &ACT_SpacingDecrease ) )
        {
            placer->SpacingStep( -1 );
            m_router->Move( end, NULL );
        }
        else if( evt->IsAction( &ACT_SpacingIncrease ) )
        {
            placer->SpacingStep( 1 );
            m_router->Move( end, NULL );
        }

        handleCommonEvents( *evt );
    }

    m_router->StopRouting();

    if( saveUndoBuffer )
    {
        // Save the recent changes in the undo buffer
        m_frame->SaveCopyInUndoList( m_router->GetUndoBuffer(), UR_UNSPECIFIED );
        m_router->ClearUndoBuffer();
        m_frame->OnModify();
    }
    else
    {
        // It was interrupted by TA_UNDO_REDO event, so we have to sync the world now
        m_needsSync = true;
    }

    m_ctls->SetAutoPan( false );
    m_ctls->ForceCursorPosition( false );
    highlightNet( false );
}


int LENGTH_TUNER_TOOL::TuneSingleTrace( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_TRACK_BUTT, wxCURSOR_PENCIL, _( "Tune Trace Length" ) );
    return mainLoop( PNS_MODE_TUNE_SINGLE );
}


int LENGTH_TUNER_TOOL::TuneDiffPair( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_TRACK_BUTT, wxCURSOR_PENCIL, _( "Tune Diff Pair Length" ) );
    return mainLoop( PNS_MODE_TUNE_DIFF_PAIR );
}


int LENGTH_TUNER_TOOL::TuneDiffPairSkew( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_TRACK_BUTT, wxCURSOR_PENCIL, _( "Tune Diff Pair Skew" ) );
    return mainLoop( PNS_MODE_TUNE_DIFF_PAIR_SKEW );
}


int LENGTH_TUNER_TOOL::mainLoop( PNS_ROUTER_MODE aMode )
{
    // Deselect all items
    m_toolMgr->RunAction( COMMON_ACTIONS::selectionClear, true );

    Activate();

    m_router->SetMode( aMode );

    m_ctls->SetSnapping( true );
    m_ctls->ShowCursor( true );

    std::auto_ptr<TUNER_TOOL_MENU> ctxMenu( new TUNER_TOOL_MENU( m_board ) );
    SetContextMenu( ctxMenu.get() );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( m_needsSync )
        {
            m_router->SyncWorld();
            m_router->SetView( getView() );
            m_needsSync = false;
        }

        if( evt->IsCancel() || evt->IsActivate() )
            break; // Finish
        else if( evt->Action() == TA_UNDO_REDO )
            m_needsSync = true;
        else if( evt->IsMotion() )
            updateStartItem( *evt );
        else if( evt->IsClick( BUT_LEFT ) || evt->IsAction( &ACT_StartTuning ) )
        {
            updateStartItem( *evt );
            performTuning( );
        }

        handleCommonEvents( *evt );
    }

    // Restore the default settings
    m_ctls->SetAutoPan( false );
    m_ctls->ShowCursor( false );
    m_frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    // Store routing settings till the next invocation
    m_savedSettings = m_router->Settings();
    m_savedSizes = m_router->Sizes();

    return 0;
}

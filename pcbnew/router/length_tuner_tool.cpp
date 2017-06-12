/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2016 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>

#include "pns_segment.h"
#include "pns_router.h"
#include "pns_meander_placer.h" // fixme: move settings to separate header
#include "pns_tune_status_popup.h"

#include "length_tuner_tool.h"

using namespace KIGFX;
using boost::optional;

static TOOL_ACTION ACT_StartTuning( "pcbnew.LengthTuner.StartTuning", AS_CONTEXT, 'X',
    _( "New Track" ), _( "Starts laying a new track." ) );

static TOOL_ACTION ACT_EndTuning( "pcbnew.LengthTuner.EndTuning", AS_CONTEXT, WXK_END,
    _( "End Track" ), _( "Stops laying the current meander." ) );

static TOOL_ACTION ACT_Settings( "pcbnew.LengthTuner.Settings", AS_CONTEXT, 'L',
    _( "Length Tuning Settings" ), _( "Sets the length tuning parameters for currently routed item." ) );

static TOOL_ACTION ACT_SpacingIncrease( "pcbnew.LengthTuner.SpacingIncrease", AS_CONTEXT, '1',
    _( "Increase spacing" ), _( "Increase meander spacing by one step." ) );

static TOOL_ACTION ACT_SpacingDecrease( "pcbnew.LengthTuner.SpacingDecrease", AS_CONTEXT, '2',
    _( "Decrease spacing" ), _( "Decrease meander spacing by one step." ) );

static TOOL_ACTION ACT_AmplIncrease( "pcbnew.LengthTuner.AmplIncrease", AS_CONTEXT, '3',
    _( "Increase amplitude" ), _( "Increase meander amplitude by one step." ) );

static TOOL_ACTION ACT_AmplDecrease( "pcbnew.LengthTuner.AmplDecrease", AS_CONTEXT, '4',
    _( "Decrease amplitude" ), _( "Decrease meander amplitude by one step." ) );


LENGTH_TUNER_TOOL::LENGTH_TUNER_TOOL() :
    TOOL_BASE( "pcbnew.LengthTuner" )
{
}


class TUNER_TOOL_MENU : public CONTEXT_MENU
{
public:
    TUNER_TOOL_MENU()
    {
        SetTitle( _( "Length Tuner" ) );
        DisplayTitle( true );

        //Add( ACT_StartTuning );
        //Add( ACT_EndTuning );

        //AppendSeparator();

        Add( ACT_SpacingIncrease );
        Add( ACT_SpacingDecrease );
        Add( ACT_AmplIncrease );
        Add( ACT_AmplDecrease );
        Add( ACT_Settings );
    }

private:
    CONTEXT_MENU* create() const override
    {
        return new TUNER_TOOL_MENU();
    }
};


LENGTH_TUNER_TOOL::~LENGTH_TUNER_TOOL()
{
}


void LENGTH_TUNER_TOOL::Reset( RESET_REASON aReason )
{
    TOOL_BASE::Reset( aReason );

    Go( &LENGTH_TUNER_TOOL::TuneSingleTrace, PCB_ACTIONS::routerActivateTuneSingleTrace.MakeEvent() );
    Go( &LENGTH_TUNER_TOOL::TuneDiffPair, PCB_ACTIONS::routerActivateTuneDiffPair.MakeEvent() );
    Go( &LENGTH_TUNER_TOOL::TuneDiffPairSkew, PCB_ACTIONS::routerActivateTuneDiffPairSkew.MakeEvent() );
}


void LENGTH_TUNER_TOOL::updateStatusPopup( PNS_TUNE_STATUS_POPUP& aPopup )
{
    wxPoint p = wxGetMousePosition();

    p.x += 20;
    p.y += 20;

    aPopup.UpdateStatus( m_router );
    aPopup.Move( p );
}


void LENGTH_TUNER_TOOL::performTuning()
{
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

    PNS::MEANDER_PLACER_BASE* placer = static_cast<PNS::MEANDER_PLACER_BASE*>(
        m_router->Placer() );

    placer->UpdateSettings( m_savedMeanderSettings );

    VECTOR2I end( m_startSnapPoint );

    PNS_TUNE_STATUS_POPUP statusPopup( m_frame );
    statusPopup.Popup();

    m_router->Move( end, NULL );
    updateStatusPopup( statusPopup );

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsCancel() || evt->IsActivate() )
            break;
        else if( evt->IsMotion() )
        {
            end = evt->Position();
            m_router->Move( end, NULL );
            updateStatusPopup( statusPopup );
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
    }

    m_router->StopRouting();
    highlightNet( false );
}


int LENGTH_TUNER_TOOL::TuneSingleTrace( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_TRACK_BUTT, wxCURSOR_PENCIL, _( "Tune Trace Length" ) );
    return mainLoop( PNS::PNS_MODE_TUNE_SINGLE );
}


int LENGTH_TUNER_TOOL::TuneDiffPair( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_TRACK_BUTT, wxCURSOR_PENCIL, _( "Tune Diff Pair Length" ) );
    return mainLoop( PNS::PNS_MODE_TUNE_DIFF_PAIR );
}


int LENGTH_TUNER_TOOL::TuneDiffPairSkew( const TOOL_EVENT& aEvent )
{
    m_frame->SetToolID( ID_TRACK_BUTT, wxCURSOR_PENCIL, _( "Tune Diff Pair Skew" ) );
    return mainLoop( PNS::PNS_MODE_TUNE_DIFF_PAIR_SKEW );
}


void LENGTH_TUNER_TOOL::SetTransitions()
{
    Go( &LENGTH_TUNER_TOOL::routerOptionsDialog, ACT_RouterOptions.MakeEvent() );
    Go( &LENGTH_TUNER_TOOL::meanderSettingsDialog, ACT_Settings.MakeEvent() );
}


int LENGTH_TUNER_TOOL::mainLoop( PNS::ROUTER_MODE aMode )
{
    // Deselect all items
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    Activate();

    m_router->SetMode( aMode );

    m_ctls->SetSnapping( true );
    m_ctls->ShowCursor( true );
    m_frame->UndoRedoBlock( true );

    std::unique_ptr<TUNER_TOOL_MENU> ctxMenu( new TUNER_TOOL_MENU );
    SetContextMenu( ctxMenu.get() );

    // Main loop: keep receiving events
    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsCancel() || evt->IsActivate() )
        {
            break; // Finish
        }
        else if( evt->IsMotion() )
        {
            updateStartItem( *evt );
        }
        else if( evt->IsClick( BUT_LEFT ) || evt->IsAction( &ACT_StartTuning ) )
        {
            updateStartItem( *evt );
            performTuning();
        }
    }

    m_frame->SetNoToolSelected();
    m_frame->UndoRedoBlock( false );

    // Store routing settings till the next invocation
    m_savedSettings = m_router->Settings();
    m_savedSizes = m_router->Sizes();

    return 0;
}


int LENGTH_TUNER_TOOL::routerOptionsDialog( const TOOL_EVENT& aEvent )
{
    DIALOG_PNS_SETTINGS settingsDlg( m_frame, m_router->Settings() );

    if( settingsDlg.ShowModal() == wxID_OK )
    {
        // FIXME: do we need an explicit update?
    }

    return 0;
}


int LENGTH_TUNER_TOOL::meanderSettingsDialog( const TOOL_EVENT& aEvent )
{
    PNS::MEANDER_PLACER_BASE* placer = static_cast<PNS::MEANDER_PLACER_BASE*>( m_router->Placer() );

    if( !placer )
        return 0;

    PNS::MEANDER_SETTINGS settings = placer->MeanderSettings();
    DIALOG_PNS_LENGTH_TUNING_SETTINGS settingsDlg( m_frame, settings, m_router->Mode() );

    if( settingsDlg.ShowModal() )
        placer->UpdateSettings( settings );

    m_savedMeanderSettings = placer->MeanderSettings();

    return 0;
}

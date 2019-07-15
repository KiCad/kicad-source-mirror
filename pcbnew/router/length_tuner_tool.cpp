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

#include <core/optional.h>

#include "class_draw_panel_gal.h"
#include "class_board.h"

#include <pcb_edit_frame.h>
#include <pcbnew_id.h>
#include <view/view_controls.h>
#include <pcb_painter.h>
#include <dialogs/dialog_pns_settings.h>
#include <dialogs/dialog_pns_length_tuning_settings.h>
#include <tool/action_menu.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include "pns_segment.h"
#include "pns_router.h"
#include "pns_meander_placer.h" // fixme: move settings to separate header
#include "pns_tune_status_popup.h"

#include "length_tuner_tool.h"
#include <bitmaps.h>
#include <tools/tool_event_utils.h>

using namespace KIGFX;

// Actions, being statically-defined, require specialized I18N handling.  We continue to
// use the _() macro so that string harvesting by the I18N framework doesn't have to be
// specialized, but we don't translate on initialization and instead do it in the getters.

#undef _
#define _(s) s

static TOOL_ACTION ACT_StartTuning( "pcbnew.LengthTuner.StartTuning",
        AS_CONTEXT,
        'X', LEGACY_HK_NAME( "Add New Track" ),
        _( "New Track" ), _( "Starts laying a new track." ) );

static TOOL_ACTION ACT_EndTuning( "pcbnew.LengthTuner.EndTuning", 
        AS_CONTEXT,
        WXK_END, LEGACY_HK_NAME( "Stop laying the current track." ),
        _( "End Track" ), _( "Stops laying the current meander." ) );

static TOOL_ACTION ACT_Settings( "pcbnew.LengthTuner.Settings", 
        AS_CONTEXT,
        // Don't be tempted to remove "Modern Toolset only".  It's in the legacy property name.
        MD_CTRL + 'L', LEGACY_HK_NAME( "Length Tuning Settings (Modern Toolset only)" ),
        _( "Length Tuning Settings..." ), _( "Sets the length tuning parameters for currently routed item." ),
        router_len_tuner_setup_xpm );

static TOOL_ACTION ACT_SpacingIncrease( "pcbnew.LengthTuner.SpacingIncrease", 
        AS_CONTEXT,
        '1', LEGACY_HK_NAME( "Increase meander spacing by one step." ),
        _( "Increase Spacing" ), _( "Increase meander spacing by one step." ),
        router_len_tuner_dist_incr_xpm );

static TOOL_ACTION ACT_SpacingDecrease( "pcbnew.LengthTuner.SpacingDecrease", 
        AS_CONTEXT,
        '2', LEGACY_HK_NAME( "Decrease meander spacing by one step." ),
        _( "Decrease Spacing" ), _( "Decrease meander spacing by one step." ),
        router_len_tuner_dist_decr_xpm );

static TOOL_ACTION ACT_AmplIncrease( "pcbnew.LengthTuner.AmplIncrease", 
        AS_CONTEXT,
        '3', LEGACY_HK_NAME( "Increase meander amplitude by one step." ),
        _( "Increase Amplitude" ), _( "Increase meander amplitude by one step." ),
        router_len_tuner_amplitude_incr_xpm );

static TOOL_ACTION ACT_AmplDecrease( "pcbnew.LengthTuner.AmplDecrease", 
        AS_CONTEXT,
        '4', LEGACY_HK_NAME( "Decrease meander amplitude by one step." ),
        _( "Decrease Amplitude" ), _( "Decrease meander amplitude by one step." ),
        router_len_tuner_amplitude_decr_xpm );

#undef _
#define _(s) wxGetTranslation((s))


LENGTH_TUNER_TOOL::LENGTH_TUNER_TOOL() :
    TOOL_BASE( "pcbnew.LengthTuner" )
{
}


class TUNER_TOOL_MENU : public ACTION_MENU
{
public:
    TUNER_TOOL_MENU() :
        ACTION_MENU( true )
    {
        SetTitle( _( "Length Tuner" ) );
        SetIcon( router_len_tuner_xpm );
        DisplayTitle( true );

        Add( ACTIONS::cancelInteractive );

        AppendSeparator();

        Add( ACT_SpacingIncrease );
        Add( ACT_SpacingDecrease );
        Add( ACT_AmplIncrease );
        Add( ACT_AmplDecrease );
        Add( ACT_Settings );
    }

private:
    ACTION_MENU* create() const override
    {
        return new TUNER_TOOL_MENU();
    }
};


LENGTH_TUNER_TOOL::~LENGTH_TUNER_TOOL()
{
}


void LENGTH_TUNER_TOOL::Reset( RESET_REASON aReason )
{
    if( aReason == RUN )
        TOOL_BASE::Reset( aReason );
}


void LENGTH_TUNER_TOOL::updateStatusPopup( PNS_TUNE_STATUS_POPUP& aPopup )
{
    // fixme: wx code not allowed inside tools!
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
        frame()->SetActiveLayer( ToLAYER_ID ( m_startItem->Layers().Start() ) );

        if( m_startItem->Net() >= 0 )
            highlightNet( true, m_startItem->Net() );
    }

    controls()->ForceCursorPosition( false );
    controls()->SetAutoPan( true );

    if( !m_router->StartRouting( m_startSnapPoint, m_startItem, 0 ) )
    {
        wxMessageBox( m_router->FailureReason(), _( "Error" ) );
        highlightNet( false );
        return;
    }

    auto placer = static_cast<PNS::MEANDER_PLACER_BASE*>( m_router->Placer() );

    placer->UpdateSettings( m_savedMeanderSettings );

    VECTOR2I end( m_startSnapPoint );

    PNS_TUNE_STATUS_POPUP statusPopup( frame() );
    statusPopup.Popup();

    m_router->Move( end, NULL );
    updateStatusPopup( statusPopup );

    while( TOOL_EVENT* evt = Wait() )
    {
        frame()->GetCanvas()->SetCurrentCursor( wxCURSOR_ARROW );

        if( evt->IsCancelInteractive() || evt->IsActivate() )
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
            updateStatusPopup( statusPopup );
        }
        else if( evt->IsAction( &ACT_AmplIncrease ) )
        {
            placer->AmplitudeStep( 1 );
            m_router->Move( end, NULL );
            updateStatusPopup( statusPopup );
        }
        else if(evt->IsAction( &ACT_SpacingDecrease ) )
        {
            placer->SpacingStep( -1 );
            m_router->Move( end, NULL );
            updateStatusPopup( statusPopup );
        }
        else if( evt->IsAction( &ACT_SpacingIncrease ) )
        {
            placer->SpacingStep( 1 );
            m_router->Move( end, NULL );
            updateStatusPopup( statusPopup );
        }
        else if( evt->IsAction( &ACT_Settings ) )
        {
            statusPopup.Hide();
            TOOL_EVENT dummy;
            meanderSettingsDialog( dummy );
            statusPopup.Show();
        }
    }

    m_router->StopRouting();
    controls()->SetAutoPan( false );
    controls()->ForceCursorPosition( false );
    highlightNet( false );
}


void LENGTH_TUNER_TOOL::setTransitions()
{
    Go( &LENGTH_TUNER_TOOL::MainLoop, PCB_ACTIONS::routerTuneSingleTrace.MakeEvent() );
    Go( &LENGTH_TUNER_TOOL::MainLoop, PCB_ACTIONS::routerTuneDiffPair.MakeEvent() );
    Go( &LENGTH_TUNER_TOOL::MainLoop, PCB_ACTIONS::routerTuneDiffPairSkew.MakeEvent() );
}


int LENGTH_TUNER_TOOL::MainLoop( const TOOL_EVENT& aEvent )
{
    // Deselect all items
    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    std::string tool = aEvent.GetCommandStr().get();
    frame()->PushTool( tool );
    Activate();

    m_router->SetMode( aEvent.Parameter<PNS::ROUTER_MODE>() );

    controls()->SetSnapping( true );
    controls()->ShowCursor( true );
    frame()->UndoRedoBlock( true );

    std::unique_ptr<TUNER_TOOL_MENU> ctxMenu( new TUNER_TOOL_MENU );
    SetContextMenu( ctxMenu.get() );

    // Main loop: keep receiving events
    while( TOOL_EVENT* evt = Wait() )
    {
        frame()->GetCanvas()->SetCurrentCursor( wxCURSOR_ARROW );

        if( evt->IsCancelInteractive() || evt->IsActivate() )
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
        else if( evt->IsAction( &ACT_Settings ) )
        {
            TOOL_EVENT dummy;
            meanderSettingsDialog( dummy );
        }
    }

    frame()->UndoRedoBlock( false );

    // Store routing settings till the next invocation
    m_savedSettings = m_router->Settings();
    m_savedSizes = m_router->Sizes();

    frame()->PopTool( tool );
    return 0;
}

int LENGTH_TUNER_TOOL::meanderSettingsDialog( const TOOL_EVENT& aEvent )
{
    PNS::MEANDER_PLACER_BASE* placer = static_cast<PNS::MEANDER_PLACER_BASE*>( m_router->Placer() );

    PNS::MEANDER_SETTINGS settings = placer ? placer->MeanderSettings() : m_savedMeanderSettings;
    DIALOG_PNS_LENGTH_TUNING_SETTINGS settingsDlg( frame(), settings, m_router->Mode() );

    if( settingsDlg.ShowModal() )
    {
        if( placer )
            placer->UpdateSettings( settings );

        m_savedMeanderSettings = settings;
    }

    return 0;
}

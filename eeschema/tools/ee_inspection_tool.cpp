/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <view/view_controls.h>
#include <sch_component.h>
#include <sch_marker.h>
#include <id.h>
#include <kiway.h>
#include <confirm.h>
#include <tool/conditional_menu.h>
#include <tool/selection_conditions.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/ee_inspection_tool.h>
#include <tools/ee_selection_tool.h>
#include <tools/ee_selection.h>
#include <search_stack.h>
#include <sim/sim_plot_frame.h>
#include <sch_view.h>
#include <sch_edit_frame.h>
#include <lib_edit_frame.h>
#include <viewlib_frame.h>
#include <eda_doc.h>
#include <invoke_sch_dialog.h>
#include <project.h>
#include <dialogs/dialog_display_info_HTML_base.h>


EE_INSPECTION_TOOL::EE_INSPECTION_TOOL()
    : EE_TOOL_BASE<SCH_BASE_FRAME>( "eeschema.InspectionTool" )
{
}


bool EE_INSPECTION_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    auto singleMarkerCondition = SELECTION_CONDITIONS::OnlyType( SCH_MARKER_T )
                              && SELECTION_CONDITIONS::Count( 1 );

    // Add inspection actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( EE_ACTIONS::showDatasheet, EE_CONDITIONS::SingleSymbol && EE_CONDITIONS::Idle, 220 );
    selToolMenu.AddItem( EE_ACTIONS::showMarkerInfo, singleMarkerCondition && EE_CONDITIONS::Idle, 220 );

    return true;
}


int EE_INSPECTION_TOOL::RunERC( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
    {
        checkPart( static_cast<LIB_EDIT_FRAME*>( m_frame )->GetCurPart() );
    }
    else if( m_frame->IsType( FRAME_SCH ) )
    {
        wxWindow* erc = wxWindow::FindWindowById( ID_DIALOG_ERC, m_frame );

        if( erc )
            // Bring it to the top if already open.  Dual monitor users need this.
            erc->Raise();
        else
            InvokeDialogERC( static_cast<SCH_EDIT_FRAME*>( m_frame ) );
    }

    return 0;
}


// helper function to sort pins by pin num
bool sort_by_pin_number( const LIB_PIN* ref, const LIB_PIN* tst )
{
    // Use number as primary key
    int test = ref->GetNumber().Cmp( tst->GetNumber() );

    // Use DeMorgan variant as secondary key
    if( test == 0 )
        test = ref->GetConvert() - tst->GetConvert();

    // Use unit as tertiary key
    if( test == 0 )
        test = ref->GetUnit() - tst->GetUnit();

    return test < 0;
}


void EE_INSPECTION_TOOL::checkPart( LIB_PART* aPart )
{
    if( !aPart )
        return;

    wxString    msg;
    wxRealPoint curr_grid_size = m_frame->GetScreen()->GetGridSize();
    const int   min_grid_size = 25;
    const int   grid_size = KiROUND( curr_grid_size.x );
    const int   clamped_grid_size = ( grid_size < min_grid_size ) ? min_grid_size : grid_size;
    LIB_PINS    pinList;

    aPart->GetPins( pinList );

    if( pinList.empty() )
    {
        DisplayInfoMessage( m_frame, _( "No pins!" ) );
        return;
    }

    // Sort pins by pin num, so 2 duplicate pins
    // (pins with the same number) will be consecutive in list
    sort( pinList.begin(), pinList.end(), sort_by_pin_number );

    // Test for duplicates:
    DIALOG_DISPLAY_HTML_TEXT_BASE error_display( m_frame, wxID_ANY, _( "Marker Information" ),
                                                 wxDefaultPosition, wxSize( 750, 600 ) );

    int dup_error = 0;

    for( unsigned ii = 1; ii < pinList.size(); ii++ )
    {
        LIB_PIN* pin  = pinList[ii - 1];
        LIB_PIN* next = pinList[ii];

        if( pin->GetNumber() != next->GetNumber() || pin->GetConvert() != next->GetConvert() )
            continue;

        dup_error++;

        /* TODO I dare someone to find a way to make happy translators on this thing! Lorenzo */

        msg = wxString::Format( _( "<b>Duplicate pin %s</b> \"%s\" at location <b>(%.3f, %.3f)</b>"
                                   " conflicts with pin %s \"%s\" at location <b>(%.3f, %.3f)</b>" ),
                                next->GetNumber(),
                                next->GetName(),
                                next->GetPosition().x / 1000.0, -next->GetPosition().y / 1000.0,
                                pin->GetNumber(),
                                pin->GetName(),
                                pin->GetPosition().x / 1000.0, -pin->GetPosition().y / 1000.0 );

        if( aPart->GetUnitCount() > 1 )
        {
            msg += wxString::Format( _( " in units %c and %c" ),
                                     'A' + next->GetUnit() - 1,
                                     'A' + pin->GetUnit() - 1 );
        }

        if( aPart->HasConversion() )
        {
            if( next->GetConvert() )
                msg += _( "  of converted" );
            else
                msg += _( "  of normal" );
        }

        msg += wxT( ".<br>" );

        error_display.m_htmlWindow->AppendToPage( msg );
    }

    // Test for off grid pins:
    int offgrid_error = 0;

    for( LIB_PIN* pin : pinList )
    {
        if( ( (pin->GetPosition().x % clamped_grid_size) == 0 ) &&
            ( (pin->GetPosition().y % clamped_grid_size) == 0 ) )
            continue;

        // "pin" is off grid here.
        offgrid_error++;

        msg = wxString::Format( _( "<b>Off grid pin %s</b> \"%s\" at location <b>(%.3f, %.3f)</b>" ),
                                pin->GetNumber(),
                                pin->GetName(),
                                pin->GetPosition().x / 1000.0, -pin->GetPosition().y / 1000.0 );

        if( aPart->GetUnitCount() > 1 )
            msg += wxString::Format( _( " in symbol %c" ), 'A' + pin->GetUnit() - 1 );

        if( aPart->HasConversion() )
        {
            if( pin->GetConvert() )
                msg += _( " of converted" );
            else
                msg += _( " of normal" );
        }

        msg += wxT( ".<br>" );

        error_display.m_htmlWindow->AppendToPage( msg );
    }

    if( !dup_error && !offgrid_error )
        DisplayInfoMessage( m_frame, _( "No off grid or duplicate pins were found." ) );
    else
        error_display.ShowModal();
}


int EE_INSPECTION_TOOL::RunSimulation( const TOOL_EVENT& aEvent )
{
#ifdef KICAD_SPICE
    SIM_PLOT_FRAME* simFrame = (SIM_PLOT_FRAME*) m_frame->Kiway().Player( FRAME_SIMULATOR, true );
    simFrame->Show( true );

    // On Windows, Raise() does not bring the window on screen, when iconized
    if( simFrame->IsIconized() )
        simFrame->Iconize( false );

    simFrame->Raise();
#endif /* KICAD_SPICE */
    return 0;
}


int EE_INSPECTION_TOOL::ShowDatasheet( const TOOL_EVENT& aEvent )
{
    wxString datasheet;

    if( m_frame->IsType( FRAME_SCH_LIB_EDITOR ) )
    {
        LIB_PART* part = static_cast<LIB_EDIT_FRAME*>( m_frame )->GetCurPart();

        if( !part )
            return 0;

        if( part->GetAliasCount() > 1 )
        {
            ACTION_MENU  popup( true );
            wxString     msg;
            int          id = 0;

            for( LIB_ALIAS* alias : part->GetAliases() )
            {
                msg.Printf( wxT( "%s (%s)" ), alias->GetName(), alias->GetDocFileName() );
                popup.Append( id++, msg );
            }

            m_frame->PopupMenu( &popup );

            if( popup.GetSelected() >= 0 )
                datasheet = part->GetAlias( (unsigned) popup.GetSelected() )->GetDocFileName();
        }
        else
            datasheet = part->GetAlias( 0 )->GetDocFileName();
    }
    else if( m_frame->IsType( FRAME_SCH_VIEWER ) || m_frame->IsType( FRAME_SCH_VIEWER_MODAL ) )
    {
        LIB_ALIAS* entry = static_cast<LIB_VIEW_FRAME*>( m_frame )->GetSelectedAlias();

        if( !entry )
            return 0;

        datasheet = entry->GetDocFileName();
    }
    else if( m_frame->IsType( FRAME_SCH ) )
    {
        EE_SELECTION& selection = m_selectionTool->RequestSelection( EE_COLLECTOR::ComponentsOnly );

        if( selection.Empty() )
            return 0;

        SCH_COMPONENT* component = (SCH_COMPONENT*) selection.Front();

        datasheet = component->GetField( DATASHEET )->GetText();
    }

    if( !datasheet.IsEmpty() && datasheet != wxT( "~" ) )
    {
        SEARCH_STACK* lib_search = m_frame->Prj().SchSearchS();

        GetAssociatedDocument( m_frame, datasheet, lib_search );
    }

    return 0;
}


int EE_INSPECTION_TOOL::ShowMarkerInfo( const TOOL_EVENT& aEvent )
{
    EE_SELECTION& selection = m_selectionTool->GetSelection();

    if( selection.Empty() )
        return 0;

    SCH_MARKER* marker = dynamic_cast<SCH_MARKER*>( selection.Front() );

    if( marker )
        marker->DisplayMarkerInfo( m_frame );

    return 0;
}


int EE_INSPECTION_TOOL::UpdateMessagePanel( const TOOL_EVENT& aEvent )
{
    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    EE_SELECTION&      selection = selTool->GetSelection();

    if( selection.GetSize() == 1 )
    {
        EDA_ITEM* item = (EDA_ITEM*) selection.Front();

        MSG_PANEL_ITEMS msgItems;
        item->GetMsgPanelInfo( m_frame->GetUserUnits(), msgItems );
        m_frame->SetMsgPanel( msgItems );
    }
    else
    {
        m_frame->ClearMsgPanel();
    }

    return 0;
}


void EE_INSPECTION_TOOL::setTransitions()
{
    Go( &EE_INSPECTION_TOOL::RunERC,              EE_ACTIONS::runERC.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::RunSimulation,       EE_ACTIONS::runSimulation.MakeEvent() );

    Go( &EE_INSPECTION_TOOL::ShowDatasheet,       EE_ACTIONS::showDatasheet.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::ShowMarkerInfo,      EE_ACTIONS::showMarkerInfo.MakeEvent() );

    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::SelectedEvent );
    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::UnselectedEvent );
    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::ClearedEvent );
    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::SelectedItemsModified );
}



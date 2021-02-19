/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_component.h>
#include <id.h>
#include <kiway.h>
#include <confirm.h>
#include <tool/conditional_menu.h>
#include <tool/selection_conditions.h>
#include <tools/ee_actions.h>
#include <tools/ee_inspection_tool.h>
#include <tools/ee_selection_tool.h>
#include <tools/ee_selection.h>
#include <sim/sim_plot_frame.h>
#include <sch_edit_frame.h>
#include <symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <eda_doc.h>
#include <sch_marker.h>
#include <project.h>
#include <dialogs/dialog_display_info_HTML_base.h>
#include <dialogs/dialog_erc.h>
#include <math/util.h>      // for KiROUND


EE_INSPECTION_TOOL::EE_INSPECTION_TOOL() :
    EE_TOOL_BASE<SCH_BASE_FRAME>( "eeschema.InspectionTool" ),
    m_ercDialog( nullptr )
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

    selToolMenu.AddItem( EE_ACTIONS::excludeMarker, singleMarkerCondition, 100 );

    selToolMenu.AddItem( EE_ACTIONS::showDatasheet, EE_CONDITIONS::SingleSymbol && EE_CONDITIONS::Idle, 220 );

    return true;
}


void EE_INSPECTION_TOOL::Reset( RESET_REASON aReason )
{
    EE_TOOL_BASE::Reset( aReason );

    if( aReason == MODEL_RELOAD )
    {
        DestroyERCDialog();
    }
}


int EE_INSPECTION_TOOL::RunERC( const TOOL_EVENT& aEvent )
{
    ShowERCDialog();
    return 0;
}


void EE_INSPECTION_TOOL::ShowERCDialog()
{
    if( m_frame->IsType( FRAME_SCH ) )
    {
        if( m_ercDialog )
        {
            // Needed at least on Windows. Raise() is not enough
            m_ercDialog->Show( true );
            // Bring it to the top if already open.  Dual monitor users need this.
            m_ercDialog->Raise();
        }
        else
        {
            // This is a modeless dialog, so new it rather than instantiating on stack.
            m_ercDialog = new DIALOG_ERC( static_cast<SCH_EDIT_FRAME*>( m_frame ) );

            m_ercDialog->Show( true );
        }
    }
}


void EE_INSPECTION_TOOL::DestroyERCDialog()
{
    if( m_ercDialog )
        m_ercDialog->Destroy();

    m_ercDialog = nullptr;
}


int EE_INSPECTION_TOOL::PrevMarker( const TOOL_EVENT& aEvent )
{
    if( m_ercDialog )
    {
        m_ercDialog->Show( true );
        m_ercDialog->Raise();
        m_ercDialog->PrevMarker();
    }
    else
    {
        ShowERCDialog();
    }

    return 0;
}


int EE_INSPECTION_TOOL::NextMarker( const TOOL_EVENT& aEvent )
{
    if( m_ercDialog )
    {
        m_ercDialog->Show( true );
        m_ercDialog->Raise();
        m_ercDialog->NextMarker();
    }
    else
    {
        ShowERCDialog();
    }

    return 0;
}


int EE_INSPECTION_TOOL::ExcludeMarker( const TOOL_EVENT& aEvent )
{
    if( m_ercDialog )
    {
        // Let the ERC dialog handle it since it has more update hassles to worry about
        m_ercDialog->ExcludeMarker();
    }
    else
    {
        EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
        EE_SELECTION&      selection = selTool->GetSelection();

        if( selection.GetSize() == 1 && selection.Front()->Type() == SCH_MARKER_T )
        {
            SCH_MARKER* marker = static_cast<SCH_MARKER*>( selection.Front() );

            marker->SetExcluded( true );
            m_frame->GetCanvas()->GetView()->Update( marker );
            m_frame->GetCanvas()->Refresh();
            m_frame->OnModify();
        }
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


int EE_INSPECTION_TOOL::CheckSymbol( const TOOL_EVENT& aEvent )
{
    LIB_PART* part = static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurPart();

    if( !part )
        return 0;

    LIB_PINS                  pinList;
    std::unique_ptr<LIB_PART> flattenedPart = part->Flatten();

    flattenedPart->GetPins( pinList );

    // Sort pins by pin num, so 2 duplicate pins
    // (pins with the same number) will be consecutive in list
    sort( pinList.begin(), pinList.end(), sort_by_pin_number );

    // Test for duplicates:
    DIALOG_DISPLAY_HTML_TEXT_BASE error_display( m_frame, wxID_ANY, _( "Symbol Warnings" ),
                                                 wxDefaultPosition, wxSize( 750, 600 ) );

    const int min_grid_size = 25;
    const int grid_size = KiROUND( getView()->GetGAL()->GetGridSize().x );
    const int clamped_grid_size = ( grid_size < min_grid_size ) ? min_grid_size : grid_size;

    std::vector<wxString> messages;
    wxString              msg;

    for( unsigned ii = 1; ii < pinList.size(); ii++ )
    {
        LIB_PIN* pin  = pinList[ii - 1];
        LIB_PIN* next = pinList[ii];

        if( pin->GetNumber() != next->GetNumber() || pin->GetConvert() != next->GetConvert() )
            continue;

        wxString pinName;
        wxString nextName;

        if( pin->GetName() != "~"  && !pin->GetName().IsEmpty() )
            pinName = " '" + pin->GetName() + "'";

        if( next->GetName() != "~"  && !next->GetName().IsEmpty() )
            nextName = " '" + next->GetName() + "'";

        if( part->HasConversion() && next->GetConvert() )
        {
            if( part->GetUnitCount() <= 1 )
            {
                msg.Printf( _( "<b>Duplicate pin %s</b> %s at location <b>(%.3f, %.3f)</b>"
                               " conflicts with pin %s%s at location <b>(%.3f, %.3f)</b>"
                               " of converted." ),
                            next->GetNumber(),
                            nextName,
                            next->GetPosition().x / 1000.0, -next->GetPosition().y / 1000.0,
                            pin->GetNumber(),
                            pin->GetName(),
                            pin->GetPosition().x / 1000.0, -pin->GetPosition().y / 1000.0 );
            }
            else
            {
                msg.Printf( _( "<b>Duplicate pin %s</b> %s at location <b>(%.3f, %.3f)</b>"
                               " conflicts with pin %s%s at location <b>(%.3f, %.3f)</b>"
                               " in units %c and %c of converted." ),
                            next->GetNumber(),
                            nextName,
                            next->GetPosition().x / 1000.0, -next->GetPosition().y / 1000.0,
                            pin->GetNumber(),
                            pinName,
                            pin->GetPosition().x / 1000.0, -pin->GetPosition().y / 1000.0,
                            'A' + next->GetUnit() - 1,
                            'A' + pin->GetUnit() - 1 );
            }
        }
        else
        {
            if( part->GetUnitCount() <= 1 )
            {
                msg.Printf( _( "<b>Duplicate pin %s</b> %s at location <b>(%.3f, %.3f)</b>"
                               " conflicts with pin %s%s at location <b>(%.3f, %.3f)</b>." ),
                            next->GetNumber(),
                            nextName,
                            next->GetPosition().x / 1000.0, -next->GetPosition().y / 1000.0,
                            pin->GetNumber(),
                            pinName,
                            pin->GetPosition().x / 1000.0, -pin->GetPosition().y / 1000.0 );
            }
            else
            {
                msg.Printf( _( "<b>Duplicate pin %s</b> %s at location <b>(%.3f, %.3f)</b>"
                               " conflicts with pin %s%s at location <b>(%.3f, %.3f)</b>"
                               " in units %c and %c." ),
                            next->GetNumber(),
                            nextName,
                            next->GetPosition().x / 1000.0, -next->GetPosition().y / 1000.0,
                            pin->GetNumber(),
                            pinName,
                            pin->GetPosition().x / 1000.0, -pin->GetPosition().y / 1000.0,
                            'A' + next->GetUnit() - 1,
                            'A' + pin->GetUnit() - 1 );
            }
        }

        msg += wxT( "<br><br>" );
        messages.push_back( msg );
    }

    for( LIB_PIN* pin : pinList )
    {
        wxString pinName = pin->GetName();

        if( pinName.IsEmpty() || pinName == "~" )
            pinName = "";
        else
            pinName = "'" + pinName + "'";

        if( !part->IsPower()
                && pin->GetType() == ELECTRICAL_PINTYPE::PT_POWER_IN
                && !pin->IsVisible() )
        {
            // hidden power pin
            if( part->HasConversion() && pin->GetConvert() )
            {
                if( part->GetUnitCount() <= 1 )
                {
                    msg.Printf( _( "<b>Hidden power pin %s</b> %s at location <b>(%.3f, %.3f)</b>"
                                   " of converted." ),
                                pin->GetNumber(),
                                pinName,
                                pin->GetPosition().x / 1000.0, -pin->GetPosition().y / 1000.0 );
                }
                else
                {
                    msg.Printf( _( "<b>Hidden power pin %s</b> %s at location <b>(%.3f, %.3f)</b>"
                                   " in unit %c of converted." ),
                                pin->GetNumber(),
                                pinName,
                                pin->GetPosition().x / 1000.0, -pin->GetPosition().y / 1000.0,
                                'A' + pin->GetUnit() - 1 );
                }
            }
            else
            {
                if( part->GetUnitCount() <= 1 )
                {
                    msg.Printf( _( "<b>Hidden power pin %s</b> %s at location <b>(%.3f, %.3f)</b>." ),
                                pin->GetNumber(),
                                pinName,
                                pin->GetPosition().x / 1000.0, -pin->GetPosition().y / 1000.0 );
                }
                else
                {
                    msg.Printf( _( "<b>Hidden power pin %s</b> %s at location <b>(%.3f, %.3f)</b>"
                                   " in unit %c." ),
                                pin->GetNumber(),
                                pinName,
                                pin->GetPosition().x / 1000.0, -pin->GetPosition().y / 1000.0,
                                'A' + pin->GetUnit() - 1 );
                }
            }

            msg += wxT( "<br>" );
            msg += _( "(Hidden power pins will drive their pin names on to any connected nets.)" );
            msg += wxT( "<br><br>" );
            messages.push_back( msg );
        }

        if( ( (pin->GetPosition().x % clamped_grid_size) != 0 )
                || ( (pin->GetPosition().y % clamped_grid_size) != 0 ) )
        {
            // pin is off grid
            if( part->HasConversion() && pin->GetConvert() )
            {
                if( part->GetUnitCount() <= 1 )
                {
                    msg.Printf( _( "<b>Off grid pin %s</b> %s at location <b>(%.3f, %.3f)</b>"
                                   " of converted." ),
                                pin->GetNumber(),
                                pinName,
                                pin->GetPosition().x / 1000.0, -pin->GetPosition().y / 1000.0 );
                }
                else
                {
                    msg.Printf( _( "<b>Off grid pin %s</b> %s at location <b>(%.3f, %.3f)</b>"
                                   " in unit %c of converted." ),
                                pin->GetNumber(),
                                pinName,
                                pin->GetPosition().x / 1000.0, -pin->GetPosition().y / 1000.0,
                                'A' + pin->GetUnit() - 1 );
                }
            }
            else
            {
                if( part->GetUnitCount() <= 1 )
                {
                    msg.Printf( _( "<b>Off grid pin %s</b> %s at location <b>(%.3f, %.3f)</b>." ),
                                pin->GetNumber(),
                                pinName,
                                pin->GetPosition().x / 1000.0, -pin->GetPosition().y / 1000.0 );
                }
                else
                {
                    msg.Printf( _( "<b>Off grid pin %s</b> %s at location <b>(%.3f, %.3f)</b>"
                                   " in unit %c." ),
                                pin->GetNumber(),
                                pinName,
                                pin->GetPosition().x / 1000.0, -pin->GetPosition().y / 1000.0,
                                'A' + pin->GetUnit() - 1 );
                }
            }

            msg += wxT( "<br><br>" );
            messages.push_back( msg );
        }
    }

    if( messages.empty() )
    {
        DisplayInfoMessage( m_frame, _( "No symbol issues found." ) );
    }
    else
    {
        wxColour bgcolor = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOW );
        wxColour fgcolor = wxSystemSettings::GetColour( wxSYS_COLOUR_WINDOWTEXT );
        wxString outmsg = wxString::Format( "<html><body bgcolor='%s' text='%s'>",
                                            bgcolor.GetAsString( wxC2S_HTML_SYNTAX ),
                                            fgcolor.GetAsString( wxC2S_HTML_SYNTAX ) );

        for( auto& msgPart : messages )
            outmsg += msgPart;

        outmsg += "</body></html>";

        error_display.m_htmlWindow->SetPage( outmsg );
        error_display.ShowModal();
    }

    return 0;
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

    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        LIB_PART* part = static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurPart();

        if( !part )
            return 0;

        datasheet = part->GetDatasheetField().GetText();
    }
    else if( m_frame->IsType( FRAME_SCH_VIEWER ) || m_frame->IsType( FRAME_SCH_VIEWER_MODAL ) )
    {
        LIB_PART* entry = static_cast<SYMBOL_VIEWER_FRAME*>( m_frame )->GetSelectedSymbol();

        if( !entry )
            return 0;

        datasheet = entry->GetDatasheetField().GetText();
    }
    else if( m_frame->IsType( FRAME_SCH ) )
    {
        EE_SELECTION& selection = m_selectionTool->RequestSelection( EE_COLLECTOR::ComponentsOnly );

        if( selection.Empty() )
            return 0;

        SCH_COMPONENT* component = (SCH_COMPONENT*) selection.Front();

        datasheet = component->GetField( DATASHEET_FIELD )->GetText();
    }

    if( !datasheet.IsEmpty() && datasheet != wxT( "~" ) )
        GetAssociatedDocument( m_frame, datasheet, &m_frame->Prj() );

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
        item->GetMsgPanelInfo( m_frame, msgItems );
        m_frame->SetMsgPanel( msgItems );
    }
    else
    {
        m_frame->ClearMsgPanel();
    }

    if( SCH_EDIT_FRAME* editFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame ) )
        editFrame->UpdateNetHighlightStatus();

    return 0;
}


void EE_INSPECTION_TOOL::setTransitions()
{
    Go( &EE_INSPECTION_TOOL::RunERC,              EE_ACTIONS::runERC.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::PrevMarker,          EE_ACTIONS::prevMarker.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::NextMarker,          EE_ACTIONS::nextMarker.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::ExcludeMarker,       EE_ACTIONS::excludeMarker.MakeEvent() );

    Go( &EE_INSPECTION_TOOL::CheckSymbol,         EE_ACTIONS::checkSymbol.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::RunSimulation,       EE_ACTIONS::runSimulation.MakeEvent() );

    Go( &EE_INSPECTION_TOOL::ShowDatasheet,       EE_ACTIONS::showDatasheet.MakeEvent() );

    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::SelectedEvent );
    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::UnselectedEvent );
    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::ClearedEvent );
    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::SelectedItemsModified );
}



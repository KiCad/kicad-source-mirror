/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_symbol.h>
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
#include <dialogs/html_message_box.h>
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

    // Add inspection actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( EE_ACTIONS::excludeMarker, EE_CONDITIONS::SingleNonExcludedMarker, 100 );

    selToolMenu.AddItem( EE_ACTIONS::showDatasheet,
                         EE_CONDITIONS::SingleSymbol && EE_CONDITIONS::Idle, 220 );

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
    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    EE_SELECTION&      selection = selTool->GetSelection();
    SCH_MARKER*        marker = nullptr;

    if( selection.GetSize() == 1 && selection.Front()->Type() == SCH_MARKER_T )
        marker = static_cast<SCH_MARKER*>( selection.Front() );

    if( m_ercDialog )
    {
        // Let the ERC dialog handle it since it has more update hassles to worry about
        // Note that if marker is nullptr the dialog will exclude whichever marker is selected
        // in the dialog itself
        m_ercDialog->ExcludeMarker( marker );
    }
    else if( marker != nullptr )
    {
        marker->SetExcluded( true );
        m_frame->GetCanvas()->GetView()->Update( marker );
        m_frame->GetCanvas()->Refresh();
        m_frame->OnModify();
    }

    return 0;
}


extern void CheckLibSymbol( LIB_SYMBOL* aSymbol, std::vector<wxString>& aMessages,
                           int aGridForPins, EDA_UNITS aDisplayUnits );

int EE_INSPECTION_TOOL::CheckSymbol( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL* symbol = static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();

    if( !symbol )
        return 0;

    EDA_UNITS units = m_frame->GetUserUnits();
    std::vector<wxString> messages;
    const int grid_size = KiROUND( getView()->GetGAL()->GetGridSize().x );

    CheckLibSymbol( symbol, messages, grid_size, units );

    if( messages.empty() )
    {
        DisplayInfoMessage( m_frame, _( "No symbol issues found." ) );
    }
    else
    {
        HTML_MESSAGE_BOX dlg( m_frame, _( "Symbol Warnings" ) );

        for( const wxString& single_msg : messages )
            dlg.AddHTML_Text( single_msg );

        dlg.ShowModal();
    }

    return 0;
}


int EE_INSPECTION_TOOL::RunSimulation( const TOOL_EVENT& aEvent )
{
#ifdef KICAD_SPICE
    SIM_PLOT_FRAME* simFrame = (SIM_PLOT_FRAME*) m_frame->Kiway().Player( FRAME_SIMULATOR, true );

    if( !simFrame )
        return -1;

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
        LIB_SYMBOL* symbol = static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();

        if( !symbol )
            return 0;

        datasheet = symbol->GetDatasheetField().GetText();
    }
    else if( m_frame->IsType( FRAME_SCH_VIEWER ) || m_frame->IsType( FRAME_SCH_VIEWER_MODAL ) )
    {
        LIB_SYMBOL* entry = static_cast<SYMBOL_VIEWER_FRAME*>( m_frame )->GetSelectedSymbol();

        if( !entry )
            return 0;

        datasheet = entry->GetDatasheetField().GetText();
    }
    else if( m_frame->IsType( FRAME_SCH ) )
    {
        EE_SELECTION& selection = m_selectionTool->RequestSelection( EE_COLLECTOR::SymbolsOnly );

        if( selection.Empty() )
            return 0;

        SCH_SYMBOL* symbol = (SCH_SYMBOL*) selection.Front();

        // Use GetShownText() to resolve any text variables
        datasheet = symbol->GetField( DATASHEET_FIELD )->GetShownText();
    }

    if( datasheet.IsEmpty() || datasheet == wxT( "~" ) )
    {
        m_frame->ShowInfoBarError( _( "No datasheet defined." ) );
    }
    else
    {
        GetAssociatedDocument( m_frame, datasheet, &m_frame->Prj() );
    }

    return 0;
}


int EE_INSPECTION_TOOL::UpdateMessagePanel( const TOOL_EVENT& aEvent )
{
    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    EE_SELECTION&      selection = selTool->GetSelection();

    if( selection.GetSize() == 1 )
    {
        EDA_ITEM* item = (EDA_ITEM*) selection.Front();

        std::vector<MSG_PANEL_ITEM> msgItems;
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



/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2023 CERN
 * Copyright (C) 2019-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "tools/ee_inspection_tool.h"

#include <sch_symbol.h>
#include <id.h>
#include <kiway.h>
#include <kiplatform/ui.h>
#include <confirm.h>
#include <string_utils.h>
#include <gal/graphics_abstraction_layer.h>
#include <tool/conditional_menu.h>
#include <tool/selection_conditions.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>
#include <tools/ee_selection.h>
#include <sim/simulator_frame.h>
#include <sch_edit_frame.h>
#include <symbol_edit_frame.h>
#include <symbol_viewer_frame.h>
#include <symbol_lib_table.h>
#include <eda_doc.h>
#include <sch_marker.h>
#include <project.h>
#include <project_sch.h>
#include <dialogs/html_message_box.h>
#include <dialogs/dialog_erc.h>
#include <dialogs/dialog_book_reporter.h>
#include <widgets/wx_html_report_box.h>
#include <widgets/symbol_diff_widget.h>
#include <math/util.h>      // for KiROUND


EE_INSPECTION_TOOL::EE_INSPECTION_TOOL() :
        EE_TOOL_BASE<SCH_BASE_FRAME>( "eeschema.InspectionTool" ), m_busSyntaxHelp( nullptr )
{
}


bool EE_INSPECTION_TOOL::Init()
{
    EE_TOOL_BASE::Init();

    // Add inspection actions to the selection tool menu
    //
    CONDITIONAL_MENU& selToolMenu = m_selectionTool->GetToolMenu().GetMenu();

    selToolMenu.AddItem( EE_ACTIONS::excludeMarker, EE_CONDITIONS::SingleNonExcludedMarker, 100 );

    selToolMenu.AddItem( ACTIONS::showDatasheet,
                         EE_CONDITIONS::SingleSymbol && EE_CONDITIONS::Idle, 220 );

    return true;
}


void EE_INSPECTION_TOOL::Reset( RESET_REASON aReason )
{
    EE_TOOL_BASE::Reset( aReason );

    if( aReason == SUPERMODEL_RELOAD || aReason == RESET_REASON::SHUTDOWN )
    {
        wxCommandEvent* evt = new wxCommandEvent( EDA_EVT_CLOSE_ERC_DIALOG, wxID_ANY );

        wxQueueEvent( m_frame, evt );
    }
}


int EE_INSPECTION_TOOL::RunERC( const TOOL_EVENT& aEvent )
{
    ShowERCDialog();
    return 0;
}


void EE_INSPECTION_TOOL::ShowERCDialog()
{
    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( frame, /* void */ );

    DIALOG_ERC* dlg = frame->GetErcDialog();

    wxCHECK( dlg, /* void */ );

    // Needed at least on Windows. Raise() is not enough
    dlg->Show( true );

    // Bring it to the top if already open.  Dual monitor users need this.
    dlg->Raise();

    KIPLATFORM::UI::ForceFocus( dlg->FindWindow( wxID_OK ) );
}


int EE_INSPECTION_TOOL::PrevMarker( const TOOL_EVENT& aEvent )
{
    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( frame, 0 );

    DIALOG_ERC* dlg = frame->GetErcDialog();

    if( dlg )
    {
        dlg->Show( true );
        dlg->Raise();
        dlg->PrevMarker();
    }

    return 0;
}


int EE_INSPECTION_TOOL::NextMarker( const TOOL_EVENT& aEvent )
{
    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( frame, 0 );

    DIALOG_ERC* dlg = frame->GetErcDialog();

    wxCHECK( dlg, 0 );

    dlg->Show( true );
    dlg->Raise();
    dlg->NextMarker();

    return 0;
}


int EE_INSPECTION_TOOL::CrossProbe( const TOOL_EVENT& aEvent )
{
    EE_SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();

    wxCHECK( selectionTool, 0 );

    EE_SELECTION&      selection = selectionTool->GetSelection();

    if( selection.GetSize() == 1 && selection.Front()->Type() == SCH_MARKER_T )
    {
        SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );
        DIALOG_ERC* dlg = frame ? frame->GetErcDialog() : nullptr;

        if( dlg && dlg->IsShownOnScreen() )
            dlg->SelectMarker( static_cast<SCH_MARKER*>( selection.Front() ) );
    }

    // Show the item info on a left click on this item
    UpdateMessagePanel( aEvent );

    return 0;
}


void EE_INSPECTION_TOOL::CrossProbe( const SCH_MARKER* aMarker )
{
    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( frame, /* void */ );

    DIALOG_ERC* dlg = frame->GetErcDialog();

    if( dlg )
    {
        if( !dlg->IsShownOnScreen() )
        {
            dlg->Show( true );
            dlg->Raise();
        }

        dlg->SelectMarker( aMarker );
    }
}


wxString EE_INSPECTION_TOOL::InspectERCErrorMenuText( const std::shared_ptr<RC_ITEM>& aERCItem )
{
    auto menuDescription =
            [&]( const TOOL_ACTION& aAction )
            {
                wxString   menuItemLabel = aAction.GetMenuLabel();
                wxMenuBar* menuBar = m_frame->GetMenuBar();

                for( size_t ii = 0; ii < menuBar->GetMenuCount(); ++ii )
                {
                    for( wxMenuItem* menuItem : menuBar->GetMenu( ii )->GetMenuItems() )
                    {
                        if( menuItem->GetItemLabelText() == menuItemLabel )
                        {
                            wxString menuTitleLabel = menuBar->GetMenuLabelText( ii );

                            menuTitleLabel.Replace( wxS( "&" ), wxS( "&&" ) );
                            menuItemLabel.Replace( wxS( "&" ), wxS( "&&" ) );

                            return wxString::Format( _( "Run %s > %s" ),
                                                     menuTitleLabel,
                                                     menuItemLabel );
                        }
                    }
                }

                return wxString::Format( _( "Run %s" ), aAction.GetFriendlyName() );
            };

    if( aERCItem->GetErrorCode() == ERCE_BUS_TO_NET_CONFLICT )
    {
        return menuDescription( EE_ACTIONS::showBusSyntaxHelp );
    }
    else if( aERCItem->GetErrorCode() == ERCE_LIB_SYMBOL_MISMATCH )
    {
        return menuDescription( EE_ACTIONS::diffSymbol );
    }

    return wxEmptyString;
}


void EE_INSPECTION_TOOL::InspectERCError( const std::shared_ptr<RC_ITEM>& aERCItem )
{
    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( frame, /* void */ );

    EDA_ITEM* a = frame->GetItem( aERCItem->GetMainItemID() );

    if( aERCItem->GetErrorCode() == ERCE_BUS_TO_NET_CONFLICT )
    {
        m_toolMgr->RunAction( EE_ACTIONS::showBusSyntaxHelp );
    }
    else if( aERCItem->GetErrorCode() == ERCE_LIB_SYMBOL_MISMATCH )
    {
        if( SCH_SYMBOL* symbol = dynamic_cast<SCH_SYMBOL*>( a ) )
            DiffSymbol( symbol );
    }
}


int EE_INSPECTION_TOOL::ExcludeMarker( const TOOL_EVENT& aEvent )
{
    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    EE_SELECTION&      selection = selTool->GetSelection();
    SCH_MARKER*        marker = nullptr;

    if( selection.GetSize() == 1 && selection.Front()->Type() == SCH_MARKER_T )
        marker = static_cast<SCH_MARKER*>( selection.Front() );

    SCH_EDIT_FRAME* frame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( frame, 0 );

    DIALOG_ERC* dlg = frame->GetErcDialog();

    wxCHECK( dlg, 0 );

    // Let the ERC dialog handle it since it has more update hassles to worry about
    // Note that if marker is nullptr the dialog will exclude whichever marker is selected
    // in the dialog itself
    dlg->ExcludeMarker( marker );

    if( marker != nullptr )
    {
        marker->SetExcluded( true );
        m_frame->GetCanvas()->GetView()->Update( marker );
        m_frame->GetCanvas()->Refresh();
        m_frame->OnModify();
    }

    return 0;
}


extern void CheckLibSymbol( LIB_SYMBOL* aSymbol, std::vector<wxString>& aMessages,
                            int aGridForPins, UNITS_PROVIDER* aUnitsProvider );

int EE_INSPECTION_TOOL::CheckSymbol( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL* symbol = static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();

    if( !symbol )
        return 0;

    std::vector<wxString> messages;
    const int grid_size = KiROUND( getView()->GetGAL()->GetGridSize().x );

    CheckLibSymbol( symbol, messages, grid_size, m_frame );

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


int EE_INSPECTION_TOOL::ShowBusSyntaxHelp( const TOOL_EVENT& aEvent )
{
    if( m_busSyntaxHelp )
    {
        m_busSyntaxHelp->Raise();
        m_busSyntaxHelp->Show( true );
        return 0;
    }

    m_busSyntaxHelp = SCH_TEXT::ShowSyntaxHelp( m_frame );
    return 0;
}


int EE_INSPECTION_TOOL::DiffSymbol( const TOOL_EVENT& aEvent )
{
    SCH_EDIT_FRAME* schEditorFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( schEditorFrame, 0 );

    EE_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SYMBOL_T } );

    if( selection.Empty() )
    {
        m_frame->ShowInfoBarError( _( "Select a symbol to diff against its library equivalent." ) );
        return 0;
    }

    DiffSymbol( static_cast<SCH_SYMBOL*>( selection.Front() ) );
    return 0;
}


void EE_INSPECTION_TOOL::DiffSymbol( SCH_SYMBOL* symbol )
{
    SCH_EDIT_FRAME* schEditorFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );

    wxCHECK( schEditorFrame, /* void */ );

    DIALOG_BOOK_REPORTER* dialog = schEditorFrame->GetSymbolDiffDialog();

    wxCHECK( dialog, /* void */ );

    dialog->DeleteAllPages();

    wxString symbolDesc = wxString::Format( _( "Symbol %s" ),
                                            symbol->GetField( REFERENCE_FIELD )->GetText() );
    LIB_ID   libId = symbol->GetLibId();
    wxString libName = libId.GetLibNickname();
    wxString symbolName = libId.GetLibItemName();

    WX_HTML_REPORT_BOX* r = dialog->AddHTMLPage( _( "Summary" ) );

    r->Report( wxS( "<h7>" ) + _( "Schematic vs library diff for:" ) + wxS( "</h7>" ) );
    r->Report( wxS( "<ul><li>" ) + EscapeHTML( symbolDesc ) + wxS( "</li>" )
             + wxS( "<li>" ) + _( "Library: " ) + EscapeHTML( libName ) + wxS( "</li>" )
             + wxS( "<li>" ) + _( "Library item: " ) + EscapeHTML( symbolName )
             + wxS( "</li></ul>" ) );

    r->Report( "" );

    SYMBOL_LIB_TABLE*    libTable = PROJECT_SCH::SchSymbolLibTable( &m_frame->Prj() );
    const LIB_TABLE_ROW* libTableRow = libTable->FindRow( libName );

    if( !libTableRow )
    {
        r->Report( _( "The library is not included in the current configuration." )
                   + wxS( "&nbsp;&nbsp;&nbsp" )
                   + wxS( "<a href='$CONFIG'>" ) + _( "Manage Symbol Libraries" ) + wxS( "</a>" ) );
    }
    else if( !libTable->HasLibrary( libName, true ) )
    {
        r->Report( _( "The library is not enabled in the current configuration." )
                   + wxS( "&nbsp;&nbsp;&nbsp" )
                   + wxS( "<a href='$CONFIG'>" ) + _( "Manage Symbol Libraries" ) + wxS( "</a>" ) );
    }
    else
    {
        std::unique_ptr<LIB_SYMBOL> flattenedLibSymbol;
        std::unique_ptr<LIB_SYMBOL> flattenedSchSymbol = symbol->GetLibSymbolRef()->Flatten();

        try
        {
            if( LIB_SYMBOL* libAlias = libTable->LoadSymbol( libName, symbolName ) )
                flattenedLibSymbol = libAlias->Flatten();
        }
        catch( const IO_ERROR& )
        {
        }

        if( !flattenedLibSymbol )
        {
            r->Report( wxString::Format( _( "The library no longer contains the item %s." ),
                                         symbolName ) );
        }
        else
        {
            std::vector<SCH_FIELD> fields;

            for( SCH_FIELD& field : symbol->GetFields() )
            {
                fields.emplace_back( SCH_FIELD( flattenedLibSymbol.get(), field.GetId(),
                                                field.GetName( false ) ) );
                fields.back().CopyText( field );
                fields.back().SetAttributes( field );
                fields.back().Move( -symbol->GetPosition() );
            }

            flattenedSchSymbol->SetFields( fields );

            if( flattenedSchSymbol->Compare( *flattenedLibSymbol, SCH_ITEM::COMPARE_FLAGS::ERC,
                                             r ) == 0 )
            {
                r->Report( _( "No relevant differences detected." ) );
            }

            wxPanel*            panel = dialog->AddBlankPage( _( "Visual" ) );
            SYMBOL_DIFF_WIDGET* diff = constructDiffPanel( panel );

            diff->DisplayDiff( flattenedSchSymbol.release(), flattenedLibSymbol.release(),
                               symbol->GetUnit(), symbol->GetBodyStyle() );
        }
    }

    r->Flush();

    dialog->Raise();
    dialog->Show( true );
}


SYMBOL_DIFF_WIDGET* EE_INSPECTION_TOOL::constructDiffPanel( wxPanel* aParentPanel )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );

    EDA_DRAW_PANEL_GAL::GAL_TYPE backend = m_frame->GetCanvas()->GetBackend();
    SYMBOL_DIFF_WIDGET*          diffWidget = new SYMBOL_DIFF_WIDGET( aParentPanel, backend );

   	sizer->Add( diffWidget, 1, wxEXPAND | wxALL, 5 );
    aParentPanel->SetSizer( sizer );
    aParentPanel->Layout();

    return diffWidget;
}


int EE_INSPECTION_TOOL::RunSimulation( const TOOL_EVENT& aEvent )
{
    SIMULATOR_FRAME* simFrame = (SIMULATOR_FRAME*) m_frame->Kiway().Player( FRAME_SIMULATOR, true );

    if( !simFrame )
        return -1;

    if( wxWindow* blocking_win = simFrame->Kiway().GetBlockingDialog() )
        blocking_win->Close( true );

    simFrame->Show( true );

    // On Windows, Raise() does not bring the window on screen, when iconized
    if( simFrame->IsIconized() )
        simFrame->Iconize( false );

    simFrame->Raise();

    return 0;
}


int EE_INSPECTION_TOOL::ShowDatasheet( const TOOL_EVENT& aEvent )
{
    wxString datasheet;
    EMBEDDED_FILES* files = nullptr;

    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        LIB_SYMBOL* symbol = static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->GetCurSymbol();

        if( !symbol )
            return 0;

        datasheet = symbol->GetDatasheetField().GetText();
        files = symbol;
    }
    else if( m_frame->IsType( FRAME_SCH_VIEWER ) )
    {
        LIB_SYMBOL* entry = static_cast<SYMBOL_VIEWER_FRAME*>( m_frame )->GetSelectedSymbol();

        if( !entry )
            return 0;

        datasheet = entry->GetDatasheetField().GetText();
        files = entry;
    }
    else if( m_frame->IsType( FRAME_SCH ) )
    {
        EE_SELECTION& selection = m_selectionTool->RequestSelection( { SCH_SYMBOL_T } );

        if( selection.Empty() )
            return 0;

        SCH_SYMBOL* symbol = (SCH_SYMBOL*) selection.Front();
        SCH_FIELD*  field = symbol->GetField( DATASHEET_FIELD );

        // Use GetShownText() to resolve any text variables, but don't allow adding extra text
        // (ie: the field name)
        datasheet = field->GetShownText( &symbol->Schematic()->CurrentSheet(), false );
        files = symbol->Schematic();
    }

    if( datasheet.IsEmpty() || datasheet == wxS( "~" ) )
    {
        m_frame->ShowInfoBarError( _( "No datasheet defined." ) );
    }
    else
    {
        GetAssociatedDocument( m_frame, datasheet, &m_frame->Prj(),
                               PROJECT_SCH::SchSearchS( &m_frame->Prj() ), files );
    }

    return 0;
}


int EE_INSPECTION_TOOL::UpdateMessagePanel( const TOOL_EVENT& aEvent )
{
    SYMBOL_EDIT_FRAME* symbolEditFrame = dynamic_cast<SYMBOL_EDIT_FRAME*>( m_frame );
    SCH_EDIT_FRAME*    schEditFrame = dynamic_cast<SCH_EDIT_FRAME*>( m_frame );
    EE_SELECTION_TOOL* selTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    EE_SELECTION&      selection = selTool->GetSelection();

    // Note: the symbol viewer manages its own message panel

    if( symbolEditFrame || schEditFrame )
    {
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
    }

    if( schEditFrame )
    {
        schEditFrame->UpdateNetHighlightStatus();
        schEditFrame->UpdateHierarchySelection();
    }

    return 0;
}


void EE_INSPECTION_TOOL::setTransitions()
{
    Go( &EE_INSPECTION_TOOL::RunERC,              EE_ACTIONS::runERC.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::PrevMarker,          EE_ACTIONS::prevMarker.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::NextMarker,          EE_ACTIONS::nextMarker.MakeEvent() );
    // See note 1:
    Go( &EE_INSPECTION_TOOL::CrossProbe,          EVENTS::PointSelectedEvent );
    Go( &EE_INSPECTION_TOOL::CrossProbe,          EVENTS::SelectedEvent );
    Go( &EE_INSPECTION_TOOL::ExcludeMarker,       EE_ACTIONS::excludeMarker.MakeEvent() );

    Go( &EE_INSPECTION_TOOL::CheckSymbol,         EE_ACTIONS::checkSymbol.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::DiffSymbol,          EE_ACTIONS::diffSymbol.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::RunSimulation,       EE_ACTIONS::showSimulator.MakeEvent() );
    Go( &EE_INSPECTION_TOOL::ShowBusSyntaxHelp,   EE_ACTIONS::showBusSyntaxHelp.MakeEvent() );

    Go( &EE_INSPECTION_TOOL::ShowDatasheet,       ACTIONS::showDatasheet.MakeEvent() );

    // Note 1: tUpdateMessagePanel is called by CrossProbe. So uncomment this line if
    // call to CrossProbe is modifiied
    // Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::SelectedEvent );
    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::UnselectedEvent );
    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::ClearedEvent );
    Go( &EE_INSPECTION_TOOL::UpdateMessagePanel,  EVENTS::SelectedItemsModified );
}



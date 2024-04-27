/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2019 CERN
 * Copyright (C) 2019-2022 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <advanced_config.h>
#include "footprint_editor_control.h"
#include <wx/generic/textdlgg.h>
#include <string_utils.h>
#include <pgm_base.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <footprint_edit_frame.h>
#include <pcbnew_id.h>
#include <confirm.h>
#include <kidialog.h>
#include <gestfich.h> // To open with a text editor
#include <widgets/wx_infobar.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_group.h>
#include <zone.h>
#include <fp_lib_table.h>
#include <dialogs/dialog_cleanup_graphics.h>
#include <dialogs/dialog_footprint_checker.h>
#include <footprint_wizard_frame.h>
#include <kiway.h>
#include <drc/drc_item.h>
#include <project_pcb.h>
#include <view/view_controls.h>

#include <memory>


FOOTPRINT_EDITOR_CONTROL::FOOTPRINT_EDITOR_CONTROL() :
    PCB_TOOL_BASE( "pcbnew.ModuleEditor" ),
    m_frame( nullptr ),
    m_checkerDialog( nullptr )
{
}


FOOTPRINT_EDITOR_CONTROL::~FOOTPRINT_EDITOR_CONTROL()
{
}


void FOOTPRINT_EDITOR_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<FOOTPRINT_EDIT_FRAME>();

    if( m_checkerDialog )
        DestroyCheckerDialog();
}


bool FOOTPRINT_EDITOR_CONTROL::Init()
{
    // Build a context menu for the footprint tree
    //
    CONDITIONAL_MENU& ctxMenu = m_menu.GetMenu();

    auto libSelectedCondition =
            [ this ]( const SELECTION& aSel )
            {
                LIB_ID sel = m_frame->GetTreeFPID();
                return !sel.GetLibNickname().empty() && sel.GetLibItemName().empty();
            };

    // The libInferredCondition allows you to do things like New Symbol and Paste with a
    // symbol selected (in other words, when we know the library context even if the library
    // itself isn't selected.
    auto libInferredCondition =
            [ this ]( const SELECTION& aSel )
            {
                LIB_ID sel = m_frame->GetTreeFPID();
                return !sel.GetLibNickname().empty();
            };
    auto pinnedLibSelectedCondition =
            [ this ]( const SELECTION& aSel )
            {
                LIB_TREE_NODE* current = m_frame->GetCurrentTreeNode();
                return current && current->m_Type == LIB_TREE_NODE::LIBRARY && current->m_Pinned;
            };
    auto unpinnedLibSelectedCondition =
            [ this ](const SELECTION& aSel )
            {
                LIB_TREE_NODE* current = m_frame->GetCurrentTreeNode();
                return current && current->m_Type == LIB_TREE_NODE::LIBRARY && !current->m_Pinned;
            };
    auto fpSelectedCondition =
            [ this ]( const SELECTION& aSel )
            {
                LIB_ID sel = m_frame->GetTreeFPID();
                return !sel.GetLibNickname().empty() && !sel.GetLibItemName().empty();
            };

    auto fpExportCondition =
            [ this ]( const SELECTION& aSel )
            {
                FOOTPRINT* fp = m_frame->GetBoard()->GetFirstFootprint();
                return fp != nullptr;
            };

    auto canOpenWithTextEditor =
            [ this ]( const SELECTION& aSel )
            {
                // The option is shown if the editor has no current edits,
                // dumb/simple guard against opening a new file that does not exist on disk
                bool ret = !m_frame->IsContentModified();
                return ret;
            };

    ctxMenu.AddItem( ACTIONS::pinLibrary,             unpinnedLibSelectedCondition );
    ctxMenu.AddItem( ACTIONS::unpinLibrary,           pinnedLibSelectedCondition );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( PCB_ACTIONS::newFootprint,       libSelectedCondition );
    ctxMenu.AddItem( PCB_ACTIONS::createFootprint,    libSelectedCondition );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( ACTIONS::save,                   SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( ACTIONS::saveAs,                 libSelectedCondition || fpSelectedCondition );
    ctxMenu.AddItem( ACTIONS::revert,                 libSelectedCondition || libInferredCondition );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( PCB_ACTIONS::cutFootprint,       fpSelectedCondition );
    ctxMenu.AddItem( PCB_ACTIONS::copyFootprint,      fpSelectedCondition );
    ctxMenu.AddItem( PCB_ACTIONS::pasteFootprint,     libInferredCondition );
    ctxMenu.AddItem( PCB_ACTIONS::duplicateFootprint, fpSelectedCondition );
    ctxMenu.AddItem( PCB_ACTIONS::renameFootprint,    fpSelectedCondition );
    ctxMenu.AddItem( PCB_ACTIONS::deleteFootprint,    fpSelectedCondition );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( PCB_ACTIONS::importFootprint,    libInferredCondition );
    ctxMenu.AddItem( PCB_ACTIONS::exportFootprint,    fpExportCondition );

    // If we've got nothing else to show, at least show a hide tree option
    ctxMenu.AddItem( PCB_ACTIONS::hideFootprintTree,  !libInferredCondition );

    if( ADVANCED_CFG::GetCfg().m_EnableLibWithText )
    {
        ctxMenu.AddSeparator();
        ctxMenu.AddItem( PCB_ACTIONS::openWithTextEditor,
                         canOpenWithTextEditor && fpSelectedCondition );
    }

    return true;
}


int FOOTPRINT_EDITOR_CONTROL::NewFootprint( const TOOL_EVENT& aEvent )
{
    LIB_ID     selected = m_frame->GetTreeFPID();
    wxString   libraryName = selected.GetUniStringLibNickname();
    FOOTPRINT* newFootprint = m_frame->CreateNewFootprint( wxEmptyString, libraryName, false );

    if( !newFootprint )
        return 0;

    if( !m_frame->Clear_Pcb( true ) )
        return 0;

    canvas()->GetViewControls()->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
    m_frame->AddFootprintToBoard( newFootprint );

    // Initialize data relative to nets and netclasses (for a new footprint the defaults are
    // used).  This is mandatory to handle and draw pads.
    board()->BuildListOfNets();
    newFootprint->SetPosition( VECTOR2I( 0, 0 ) );
    newFootprint->ClearFlags();

    m_frame->Zoom_Automatique( false );
    m_frame->GetScreen()->SetContentModified();

    // If selected from the library tree then go ahead and save it there
    if( !selected.GetLibNickname().empty() )
    {
        LIB_ID fpid = newFootprint->GetFPID();
        fpid.SetLibNickname( selected.GetLibNickname() );
        newFootprint->SetFPID( fpid );
        m_frame->SaveFootprint( newFootprint );
        m_frame->ClearModify();
    }

    m_frame->UpdateView();
    m_frame->Update3DView( true, true );

    m_frame->SyncLibraryTree( false );
    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::CreateFootprint( const TOOL_EVENT& aEvent )
{
    LIB_ID selected = m_frame->GetTreeFPID();

    if( m_frame->IsContentModified() )
    {
        if( !HandleUnsavedChanges( m_frame, _( "The current footprint has been modified.  "
                                               "Save changes?" ),
                                   [&]() -> bool
                                   {
                                       return m_frame->SaveFootprint( footprint() );
                                   } ) )
        {
            return 0;
        }
    }

    auto* wizard = (FOOTPRINT_WIZARD_FRAME*) m_frame->Kiway().Player( FRAME_FOOTPRINT_WIZARD,
                                                                      true, m_frame );

    if( wizard->ShowModal( nullptr, m_frame ) )
    {
        // Creates the new footprint from python script wizard
        FOOTPRINT* newFootprint = wizard->GetBuiltFootprint();

        if( newFootprint )    // i.e. if create footprint command is OK
        {
            m_frame->Clear_Pcb( false );

            canvas()->GetViewControls()->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
            //  Add the new object to board
            m_frame->AddFootprintToBoard( newFootprint );

            // Initialize data relative to nets and netclasses (for a new footprint the defaults
            // are used).  This is mandatory to handle and draw pads.
            board()->BuildListOfNets();
            newFootprint->SetPosition( VECTOR2I( 0, 0 ) );
            newFootprint->ClearFlags();

            m_frame->Zoom_Automatique( false );
            m_frame->GetScreen()->SetContentModified();
            m_frame->OnModify();

            // If selected from the library tree then go ahead and save it there
            if( !selected.GetLibNickname().empty() )
            {
                LIB_ID fpid = newFootprint->GetFPID();
                fpid.SetLibNickname( selected.GetLibNickname() );
                newFootprint->SetFPID( fpid );
                m_frame->SaveFootprint( newFootprint );
                m_frame->ClearModify();
            }

            m_frame->UpdateView();
            canvas()->Refresh();
            m_frame->Update3DView( true, true );

            m_frame->SyncLibraryTree( false );
        }
    }

    wizard->Destroy();
    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::Save( const TOOL_EVENT& aEvent )
{
    if( !footprint() )     // no loaded footprint
        return 0;

    if( m_frame->GetTargetFPID() == m_frame->GetLoadedFPID() )
    {
        if( m_frame->SaveFootprint( footprint() ) )
        {
            view()->Update( footprint() );

            canvas()->ForceRefresh();
            m_frame->ClearModify();
            m_frame->UpdateTitle();
        }
    }

    m_frame->RefreshLibraryTree();
    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::SaveAs( const TOOL_EVENT& aEvent )
{
    if( m_frame->GetTargetFPID().GetLibItemName().empty() )
    {
        // Save Library As
        const wxString& src_libNickname = m_frame->GetTargetFPID().GetLibNickname();
        wxString src_libFullName = PROJECT_PCB::PcbFootprintLibs( &m_frame->Prj() )->GetFullURI( src_libNickname );

        if( m_frame->SaveLibraryAs( src_libFullName ) )
            m_frame->SyncLibraryTree( true );
    }
    else if( m_frame->GetTargetFPID() == m_frame->GetLoadedFPID() )
    {
        // Save Footprint As
        if( footprint() && m_frame->SaveFootprintAs( footprint() ) )
        {
            view()->Update( footprint() );
            m_frame->ClearModify();

            // Get rid of the save-will-update-board-only (or any other dismissable warning)
            WX_INFOBAR* infobar = m_frame->GetInfoBar();

            if( infobar->IsShownOnScreen() && infobar->HasCloseButton() )
                infobar->Dismiss();

            canvas()->ForceRefresh();
            m_frame->SyncLibraryTree( true );
        }
    }
    else
    {
        // Save Selected Footprint As
        FOOTPRINT* footprint = m_frame->LoadFootprint( m_frame->GetTargetFPID() );

        if( footprint && m_frame->SaveFootprintAs( footprint ) )
        {
            m_frame->SyncLibraryTree( true );
            m_frame->FocusOnLibID( footprint->GetFPID() );
        }
    }

    m_frame->RefreshLibraryTree();
    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::Revert( const TOOL_EVENT& aEvent )
{
    getEditFrame<FOOTPRINT_EDIT_FRAME>()->RevertFootprint();
    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::CutCopyFootprint( const TOOL_EVENT& aEvent )
{
    LIB_ID fpID = m_frame->GetTreeFPID();

    if( fpID == m_frame->GetLoadedFPID() )
    {
        m_copiedFootprint = std::make_unique<FOOTPRINT>( *m_frame->GetBoard()->GetFirstFootprint() );
        m_copiedFootprint->SetParent( nullptr );
    }
    else
    {
        m_copiedFootprint.reset( m_frame->LoadFootprint( fpID ) );
    }

    if( aEvent.IsAction( &PCB_ACTIONS::cutFootprint ) )
        DeleteFootprint( aEvent );

    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::PasteFootprint( const TOOL_EVENT& aEvent )
{
    if( m_copiedFootprint && !m_frame->GetTreeFPID().GetLibNickname().empty() )
    {
        wxString newLib = m_frame->GetTreeFPID().GetLibNickname();
        wxString newName = m_copiedFootprint->GetFPID().GetLibItemName();

        while( PROJECT_PCB::PcbFootprintLibs( &m_frame->Prj() )->FootprintExists( newLib, newName ) )
            newName += _( "_copy" );

        m_copiedFootprint->SetFPID( LIB_ID( newLib, newName ) );
        m_frame->SaveFootprintInLibrary( m_copiedFootprint.get(), newLib );

        m_frame->SyncLibraryTree( true );
        m_frame->LoadFootprintFromLibrary( m_copiedFootprint->GetFPID() );
        m_frame->FocusOnLibID( m_copiedFootprint->GetFPID() );
        m_frame->RefreshLibraryTree();
    }

    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::DuplicateFootprint( const TOOL_EVENT& aEvent )
{
    LIB_ID     fpID = m_frame->GetTreeFPID();
    FOOTPRINT* footprint;

    if( fpID == m_frame->GetLoadedFPID() )
        footprint = new FOOTPRINT( *m_frame->GetBoard()->GetFirstFootprint() );
    else
        footprint = m_frame->LoadFootprint( m_frame->GetTargetFPID() );

    if( footprint && m_frame->DuplicateFootprint( footprint ) )
    {
        m_frame->SyncLibraryTree( true );
        m_frame->LoadFootprintFromLibrary( footprint->GetFPID() );
        m_frame->FocusOnLibID( footprint->GetFPID() );
        m_frame->RefreshLibraryTree();
    }

    return 0;
}


class RENAME_DIALOG : public wxTextEntryDialog
{
public:
    RENAME_DIALOG( wxWindow* aParent, const wxString& aName,
                   std::function<bool( wxString newName )> aValidator ) :
            wxTextEntryDialog( aParent, _( "New name:" ), _( "Change Footprint Name" ), aName ),
            m_validator( std::move( aValidator ) )
    { }

    wxString GetFPName()
    {
        wxString name = m_textctrl->GetValue();
        name.Trim( true ).Trim( false );
        return name;
    }

protected:
    bool TransferDataFromWindow() override
    {
        return m_validator( GetFPName() );
    }

private:
    std::function<bool( wxString newName )> m_validator;
};


int FOOTPRINT_EDITOR_CONTROL::RenameFootprint( const TOOL_EVENT& aEvent )
{
    FP_LIB_TABLE* tbl = PROJECT_PCB::PcbFootprintLibs( &m_frame->Prj() );
    LIB_ID        fpID = m_frame->GetTreeFPID();
    wxString      libraryName = fpID.GetLibNickname();
    wxString      oldName = fpID.GetLibItemName();
    wxString      msg;

    RENAME_DIALOG dlg( m_frame, oldName,
            [&]( wxString newName )
            {
                if( newName.IsEmpty() )
                {
                    wxMessageBox( _( "Footprint must have a name." ) );
                    return false;
                }

                if( tbl->FootprintExists( libraryName, newName ) )
                {
                    msg = wxString::Format( _( "Footprint '%s' already exists in library '%s'." ),
                                            newName, libraryName );

                    KIDIALOG errorDlg( m_frame, msg, _( "Confirmation" ),
                                       wxOK | wxCANCEL | wxICON_WARNING );
                    errorDlg.SetOKLabel( _( "Overwrite" ) );

                    return errorDlg.ShowModal() == wxID_OK;
                }

                return true;
            } );

    if( dlg.ShowModal() != wxID_OK )
        return 0;   // canceled by user

    wxString   newName = dlg.GetFPName();
    FOOTPRINT* footprint = nullptr;

    if( fpID == m_frame->GetLoadedFPID() )
    {
        footprint = m_frame->GetBoard()->GetFirstFootprint();

        if( footprint )
        {
            footprint->SetFPID( LIB_ID( libraryName, newName ) );

            if( footprint->GetValue() == oldName )
                footprint->SetValue( newName );

            m_frame->OnModify();
            m_frame->UpdateView();
        }
    }
    else
    {
        footprint = m_frame->LoadFootprint( fpID );

        if( footprint )
        {
            try
            {
                footprint->SetFPID( LIB_ID( libraryName, newName ) );

                if( footprint->GetValue() == oldName )
                    footprint->SetValue( newName );

                m_frame->SaveFootprintInLibrary( footprint, libraryName );

                PROJECT_PCB::PcbFootprintLibs( &m_frame->Prj() )->FootprintDelete( libraryName, oldName );
            }
            catch( const IO_ERROR& ioe )
            {
                DisplayError( m_frame, ioe.What() );
            }
            catch( ... )
            {
                // Best efforts...
            }
        }
    }

    wxDataViewItem treeItem = m_frame->GetLibTreeAdapter()->FindItem( fpID );
    m_frame->UpdateLibraryTree( treeItem, footprint );
    m_frame->FocusOnLibID( LIB_ID( libraryName, newName ) );

    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::DeleteFootprint( const TOOL_EVENT& aEvent )
{
    FOOTPRINT_EDIT_FRAME* frame = getEditFrame<FOOTPRINT_EDIT_FRAME>();

    if( frame->DeleteFootprintFromLibrary( frame->GetTargetFPID(), true ) )
    {
        if( frame->GetTargetFPID() == frame->GetLoadedFPID() )
            frame->Clear_Pcb( false );

        frame->SyncLibraryTree( true );
    }

    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::ImportFootprint( const TOOL_EVENT& aEvent )
{
    bool is_last_fp_from_brd = m_frame->IsCurrentFPFromBoard();

    if( !m_frame->Clear_Pcb( true ) )
        return -1;                  // this command is aborted

    getViewControls()->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
    m_frame->ImportFootprint();

    if( m_frame->GetBoard()->GetFirstFootprint() )
        m_frame->GetBoard()->GetFirstFootprint()->ClearFlags();

    frame()->ClearUndoRedoList();

    // Update the save items if needed.
    if( is_last_fp_from_brd )
    {
        m_frame->ReCreateMenuBar();
        m_frame->ReCreateHToolbar();
    }

    m_toolMgr->RunAction( ACTIONS::zoomFitScreen );
    m_frame->OnModify();
    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::ExportFootprint( const TOOL_EVENT& aEvent )
{
    if( FOOTPRINT* fp = m_frame->GetBoard()->GetFirstFootprint() )
        m_frame->ExportFootprint( fp );

    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::OpenWithTextEditor( const TOOL_EVENT& aEvent )
{
    wxString fullEditorName = Pgm().GetTextEditor();

    if( fullEditorName.IsEmpty() )
    {
        wxMessageBox( _( "No text editor selected in KiCad. Please choose one." ) );
        return 0;
    }

    FP_LIB_TABLE* globalTable = dynamic_cast<FP_LIB_TABLE*>( &GFootprintTable );
    FP_LIB_TABLE* projectTable = PROJECT_PCB::PcbFootprintLibs( &m_frame->Prj() );
    LIB_ID        libId = m_frame->GetTreeFPID();

    const char* libName = libId.GetLibNickname().c_str();
    wxString    libItemName = wxEmptyString;

    for( FP_LIB_TABLE* table : { globalTable, projectTable } )
    {
        if( !table )
            break;

        libItemName = table->FindRow( libName, true )->GetFullURI( true ).c_str();

        libItemName = libItemName + "/" + libId.GetLibItemName() + ".kicad_mod";

    }

    if( !libItemName.IsEmpty() )
    {
        ExecuteFile( fullEditorName, libItemName.wc_str(), nullptr, false );
    }

    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::EditFootprint( const TOOL_EVENT& aEvent )
{
    m_frame->LoadFootprintFromLibrary( m_frame->GetTreeFPID() );
    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::PinLibrary( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* currentNode = m_frame->GetCurrentTreeNode();

    if( currentNode && !currentNode->m_Pinned )
    {
        m_frame->Prj().PinLibrary( currentNode->m_LibId.GetLibNickname(), false );

        currentNode->m_Pinned = true;
        m_frame->RegenerateLibraryTree();
    }

    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::UnpinLibrary( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* currentNode = m_frame->GetCurrentTreeNode();

    if( currentNode && currentNode->m_Pinned )
    {
        m_frame->Prj().UnpinLibrary( currentNode->m_LibId.GetLibNickname(), false );

        currentNode->m_Pinned = false;
        m_frame->RegenerateLibraryTree();
    }

    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::ToggleFootprintTree( const TOOL_EVENT& aEvent )
{
    m_frame->ToggleSearchTree();
    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::ToggleLayersManager( const TOOL_EVENT& aEvent )
{
    m_frame->ToggleLayersManager();
    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::ToggleProperties( const TOOL_EVENT& aEvent )
{
    m_frame->ToggleProperties();
    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::Properties( const TOOL_EVENT& aEvent )
{
    if( FOOTPRINT* footprint = m_frame->GetBoard()->GetFirstFootprint() )
    {
        getEditFrame<FOOTPRINT_EDIT_FRAME>()->OnEditItemRequest( footprint );
        m_frame->GetCanvas()->Refresh();
    }

    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::DefaultPadProperties( const TOOL_EVENT& aEvent )
{
    getEditFrame<FOOTPRINT_EDIT_FRAME>()->ShowPadPropertiesDialog( nullptr );
    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::CleanupGraphics( const TOOL_EVENT& aEvent )
{
    FOOTPRINT_EDIT_FRAME* editFrame = getEditFrame<FOOTPRINT_EDIT_FRAME>();
    DIALOG_CLEANUP_GRAPHICS dlg( editFrame, true );

    dlg.ShowModal();
    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::CheckFootprint( const TOOL_EVENT& aEvent )
{
    if( !m_checkerDialog )
    {
        m_checkerDialog = new DIALOG_FOOTPRINT_CHECKER( m_frame );
        m_checkerDialog->Show( true );
    }
    else // The dialog is just not visible (because the user has double clicked on an error item)
    {
        m_checkerDialog->Show( true );
    }

    return 0;
}


void FOOTPRINT_EDITOR_CONTROL::CrossProbe( const PCB_MARKER* aMarker )
{
    if( !m_checkerDialog )
        m_checkerDialog = new DIALOG_FOOTPRINT_CHECKER( m_frame );

    if( !m_checkerDialog->IsShownOnScreen() )
        m_checkerDialog->Show( true );

    m_checkerDialog->SelectMarker( aMarker );
}


void FOOTPRINT_EDITOR_CONTROL::DestroyCheckerDialog()
{
    if( m_checkerDialog )
    {
        m_checkerDialog->Destroy();
        m_checkerDialog = nullptr;
    }
}


int FOOTPRINT_EDITOR_CONTROL::RepairFootprint( const TOOL_EVENT& aEvent )
{
    FOOTPRINT* footprint = board()->Footprints().front();
    int        errors = 0;
    wxString   details;

    // Repair duplicate IDs and missing nets.
    std::set<KIID> ids;
    int            duplicates = 0;

    auto processItem =
            [&]( EDA_ITEM* aItem )
            {
                if( ids.count( aItem->m_Uuid ) )
                {
                    duplicates++;
                    const_cast<KIID&>( aItem->m_Uuid ) = KIID();
                }

                ids.insert( aItem->m_Uuid );
            };

    // Footprint IDs are the most important, so give them the first crack at "claiming" a
    // particular KIID.

    processItem( footprint );

    // After that the principal use is for DRC marker pointers, which are most likely to pads.

    for( PAD* pad : footprint->Pads() )
        processItem( pad );

    // From here out I don't think order matters much.

    processItem( &footprint->Reference() );
    processItem( &footprint->Value() );

    for( BOARD_ITEM* item : footprint->GraphicalItems() )
        processItem( item );

    for( ZONE* zone : footprint->Zones() )
        processItem( zone );

    for( PCB_GROUP* group : footprint->Groups() )
        processItem( group );

    if( duplicates )
    {
        errors += duplicates;
        details += wxString::Format( _( "%d duplicate IDs replaced.\n" ), duplicates );
    }

    /*******************************
     * Your test here
     */

    /*******************************
     * Inform the user
     */

    if( errors )
    {
        m_frame->OnModify();

        wxString msg = wxString::Format( _( "%d potential problems repaired." ), errors );
        DisplayInfoMessage( m_frame, msg, details );
    }
    else
    {
        DisplayInfoMessage( m_frame, _( "No footprint problems found." ) );
    }

    return 0;
}


void FOOTPRINT_EDITOR_CONTROL::setTransitions()
{
    Go( &FOOTPRINT_EDITOR_CONTROL::NewFootprint,         PCB_ACTIONS::newFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::CreateFootprint,      PCB_ACTIONS::createFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::Save,                 ACTIONS::save.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::SaveAs,               ACTIONS::saveAs.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::Revert,               ACTIONS::revert.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::DuplicateFootprint,   PCB_ACTIONS::duplicateFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::RenameFootprint,      PCB_ACTIONS::renameFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::DeleteFootprint,      PCB_ACTIONS::deleteFootprint.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_CONTROL::EditFootprint,        PCB_ACTIONS::editFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::CutCopyFootprint,     PCB_ACTIONS::cutFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::CutCopyFootprint,     PCB_ACTIONS::copyFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::PasteFootprint,       PCB_ACTIONS::pasteFootprint.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_CONTROL::ImportFootprint,      PCB_ACTIONS::importFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::ExportFootprint,      PCB_ACTIONS::exportFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::OpenWithTextEditor,   PCB_ACTIONS::openWithTextEditor.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_CONTROL::EditTextAndGraphics,  PCB_ACTIONS::editTextAndGraphics.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::CleanupGraphics,      PCB_ACTIONS::cleanupGraphics.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_CONTROL::CheckFootprint,       PCB_ACTIONS::checkFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::RepairFootprint,      PCB_ACTIONS::repairFootprint.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_CONTROL::PinLibrary,           ACTIONS::pinLibrary.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::UnpinLibrary,         ACTIONS::unpinLibrary.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::ToggleFootprintTree,  PCB_ACTIONS::showFootprintTree.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::ToggleFootprintTree,  PCB_ACTIONS::hideFootprintTree.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::Properties,           PCB_ACTIONS::footprintProperties.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::DefaultPadProperties, PCB_ACTIONS::defaultPadProperties.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::ToggleLayersManager,  PCB_ACTIONS::showLayersManager.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::ToggleProperties,     PCB_ACTIONS::showProperties.MakeEvent() );
}

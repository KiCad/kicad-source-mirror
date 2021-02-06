/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2019 CERN
 * Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "footprint_editor_control.h"
#include "kicad_clipboard.h"
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <view/view_controls.h>
#include <footprint_edit_frame.h>
#include <pcbnew_id.h>
#include <confirm.h>
#include <bitmaps.h>
#include <footprint.h>
#include <project.h>
#include <fp_lib_table.h>
#include <dialogs/dialog_cleanup_graphics.h>
#include <dialogs/dialog_footprint_checker.h>
#include <footprint_wizard_frame.h>
#include <kiway.h>
#include <drc/drc_results_provider.h>


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
    auto pinnedLibSelectedCondition =
            [ this ]( const SELECTION& aSel )
            {
                LIB_TREE_NODE* current = m_frame->GetCurrentTreeNode();
                return current && current->m_Type == LIB_TREE_NODE::LIB && current->m_Pinned;
            };
    auto unpinnedLibSelectedCondition =
            [ this ](const SELECTION& aSel )
            {
                LIB_TREE_NODE* current = m_frame->GetCurrentTreeNode();
                return current && current->m_Type == LIB_TREE_NODE::LIB && !current->m_Pinned;
            };
    auto fpSelectedCondition =
            [ this ]( const SELECTION& aSel )
            {
                LIB_ID sel = m_frame->GetTreeFPID();
                return !sel.GetLibNickname().empty() && !sel.GetLibItemName().empty();
            };

    ctxMenu.AddItem( ACTIONS::pinLibrary,            unpinnedLibSelectedCondition );
    ctxMenu.AddItem( ACTIONS::unpinLibrary,          pinnedLibSelectedCondition );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( ACTIONS::newLibrary,            SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( ACTIONS::addLibrary,            SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( ACTIONS::save,                  libSelectedCondition );
    ctxMenu.AddItem( ACTIONS::saveAs,                libSelectedCondition );
    ctxMenu.AddItem( ACTIONS::revert,                libSelectedCondition );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( PCB_ACTIONS::newFootprint,      libSelectedCondition );
#ifdef KICAD_SCRIPTING
    ctxMenu.AddItem( PCB_ACTIONS::createFootprint,   libSelectedCondition );
#endif
    ctxMenu.AddItem( PCB_ACTIONS::editFootprint,     fpSelectedCondition );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( ACTIONS::save,                  fpSelectedCondition );
    ctxMenu.AddItem( ACTIONS::saveCopyAs,            fpSelectedCondition );
    ctxMenu.AddItem( ACTIONS::revert,                fpSelectedCondition );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( PCB_ACTIONS::cutFootprint,      fpSelectedCondition );
    ctxMenu.AddItem( PCB_ACTIONS::copyFootprint,     fpSelectedCondition );
    ctxMenu.AddItem( PCB_ACTIONS::pasteFootprint,    SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( PCB_ACTIONS::deleteFootprint,   fpSelectedCondition );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( PCB_ACTIONS::importFootprint,   libSelectedCondition );
    ctxMenu.AddItem( PCB_ACTIONS::exportFootprint,   fpSelectedCondition );

    return true;
}


int FOOTPRINT_EDITOR_CONTROL::NewFootprint( const TOOL_EVENT& aEvent )
{
    LIB_ID     selected = m_frame->GetTreeFPID();
    FOOTPRINT* newFootprint = m_frame->CreateNewFootprint( wxEmptyString );

    if( !newFootprint )
        return 0;

    if( !m_frame->Clear_Pcb( true ) )
        return 0;

    canvas()->GetViewControls()->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
    m_frame->AddFootprintToBoard( newFootprint );

    // Initialize data relative to nets and netclasses (for a new footprint the defaults are
    // used).  This is mandatory to handle and draw pads.
    board()->BuildListOfNets();
    newFootprint->SetPosition( wxPoint( 0, 0 ) );
    newFootprint->ClearFlags();

    m_frame->Zoom_Automatique( false );
    m_frame->GetScreen()->SetModify();

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
    m_frame->Update3DView( true );

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

    if( wizard->ShowModal( NULL, m_frame ) )
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
            newFootprint->SetPosition( wxPoint( 0, 0 ) );
            newFootprint->ClearFlags();

            m_frame->Zoom_Automatique( false );
            m_frame->GetScreen()->SetModify();

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
            m_frame->Update3DView( true );

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
        wxString src_libFullName = m_frame->Prj().PcbFootprintLibs()->GetFullURI( src_libNickname );

        if( m_frame->SaveLibraryAs( src_libFullName ) )
            m_frame->SyncLibraryTree( true );
    }
    else if( m_frame->GetTargetFPID() == m_frame->GetLoadedFPID() )
    {
        // Save Board Footprint As
        if( footprint() && m_frame->SaveFootprintAs( footprint() ) )
        {
            view()->Update( footprint() );
            m_frame->ClearModify();

            canvas()->ForceRefresh();
            m_frame->SyncLibraryTree( true );
        }
    }
    else
    {
        // Save Selected Footprint As
        FOOTPRINT* footprint = m_frame->LoadFootprint( m_frame->GetTargetFPID() );

        if( footprint && m_frame->SaveFootprintAs( footprint ) )
            m_frame->SyncLibraryTree( true );
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
        m_copiedFootprint.reset( new FOOTPRINT( *m_frame->GetBoard()->GetFirstFootprint() ) );
    else
        m_copiedFootprint.reset( m_frame->LoadFootprint( fpID ) );

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

        while( m_frame->Prj().PcbFootprintLibs()->FootprintExists( newLib, newName ) )
            newName += _( "_copy" );

        m_copiedFootprint->SetFPID( LIB_ID( newLib, newName ) );
        m_frame->SaveFootprintInLibrary( m_copiedFootprint.get(), newLib );

        m_frame->SyncLibraryTree( true );
        m_frame->FocusOnLibID( m_copiedFootprint->GetFPID() );
    }

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

    m_toolMgr->RunAction( ACTIONS::zoomFitScreen, true );
    m_frame->OnModify();
    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::ExportFootprint( const TOOL_EVENT& aEvent )
{
    LIB_ID     fpID = m_frame->GetTreeFPID();
    FOOTPRINT* fp;

    if( !fpID.IsValid() )
        fp = m_frame->GetBoard()->GetFirstFootprint();
    else
        fp = m_frame->LoadFootprint( fpID );

    m_frame->ExportFootprint( fp );
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


int FOOTPRINT_EDITOR_CONTROL::Properties( const TOOL_EVENT& aEvent )
{
    FOOTPRINT* footprint = m_frame->GetBoard()->GetFirstFootprint();

    if( footprint )
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
        m_checkerDialog->SetMarkersProvider( new BOARD_DRC_ITEMS_PROVIDER( m_frame->GetBoard() ) );
        m_checkerDialog->Show( true );
    }
    return 0;
}


void FOOTPRINT_EDITOR_CONTROL::DestroyCheckerDialog()
{
    if( m_checkerDialog )
    {
        m_checkerDialog->Destroy();
        m_checkerDialog = nullptr;
    }
}


void FOOTPRINT_EDITOR_CONTROL::setTransitions()
{
    Go( &FOOTPRINT_EDITOR_CONTROL::NewFootprint,         PCB_ACTIONS::newFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::CreateFootprint,      PCB_ACTIONS::createFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::Save,                 ACTIONS::save.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::SaveAs,               ACTIONS::saveAs.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::SaveAs,               ACTIONS::saveCopyAs.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::Revert,               ACTIONS::revert.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::DeleteFootprint,      PCB_ACTIONS::deleteFootprint.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_CONTROL::EditFootprint,        PCB_ACTIONS::editFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::CutCopyFootprint,     PCB_ACTIONS::cutFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::CutCopyFootprint,     PCB_ACTIONS::copyFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::PasteFootprint,       PCB_ACTIONS::pasteFootprint.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_CONTROL::ImportFootprint,      PCB_ACTIONS::importFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::ExportFootprint,      PCB_ACTIONS::exportFootprint.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_CONTROL::CleanupGraphics,      PCB_ACTIONS::cleanupGraphics.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_CONTROL::CheckFootprint,       PCB_ACTIONS::checkFootprint.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_CONTROL::PinLibrary,           ACTIONS::pinLibrary.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::UnpinLibrary,         ACTIONS::unpinLibrary.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::ToggleFootprintTree,  PCB_ACTIONS::toggleFootprintTree.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::Properties,           PCB_ACTIONS::footprintProperties.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::DefaultPadProperties, PCB_ACTIONS::defaultPadProperties.MakeEvent() );
}

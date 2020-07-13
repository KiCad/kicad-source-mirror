/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2019 CERN
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "footprint_editor_tools.h"
#include "kicad_clipboard.h"
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <view/view_controls.h>
#include <footprint_edit_frame.h>
#include <pcbnew_id.h>
#include <confirm.h>
#include <bitmaps.h>
#include <class_module.h>
#include <class_edge_mod.h>
#include <project.h>
#include <fp_lib_table.h>
#include <dialogs/dialog_cleanup_graphics.h>


FOOTPRINT_EDITOR_TOOLS::FOOTPRINT_EDITOR_TOOLS() :
    PCB_TOOL_BASE( "pcbnew.ModuleEditor" ),
    m_frame( nullptr )
{
}


FOOTPRINT_EDITOR_TOOLS::~FOOTPRINT_EDITOR_TOOLS()
{
}


void FOOTPRINT_EDITOR_TOOLS::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<FOOTPRINT_EDIT_FRAME>();
}


bool FOOTPRINT_EDITOR_TOOLS::Init()
{
    // Build a context menu for the footprint tree
    //
    CONDITIONAL_MENU& ctxMenu = m_menu.GetMenu();

    auto libSelectedCondition = [ this ] ( const SELECTION& aSel ) {
        LIB_ID sel = m_frame->GetTreeFPID();
        return !sel.GetLibNickname().empty() && sel.GetLibItemName().empty();
    };
    auto pinnedLibSelectedCondition = [ this ] ( const SELECTION& aSel ) {
        LIB_TREE_NODE* current = m_frame->GetCurrentTreeNode();
        return current && current->m_Type == LIB_TREE_NODE::LIB && current->m_Pinned;
    };
    auto unpinnedLibSelectedCondition = [ this ] (const SELECTION& aSel ) {
        LIB_TREE_NODE* current = m_frame->GetCurrentTreeNode();
        return current && current->m_Type == LIB_TREE_NODE::LIB && !current->m_Pinned;
    };
    auto fpSelectedCondition = [ this ] ( const SELECTION& aSel ) {
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
    ctxMenu.AddItem( PCB_ACTIONS::newFootprint,      SELECTION_CONDITIONS::ShowAlways );
#ifdef KICAD_SCRIPTING
    ctxMenu.AddItem( PCB_ACTIONS::createFootprint,   SELECTION_CONDITIONS::ShowAlways );
#endif
    ctxMenu.AddItem( PCB_ACTIONS::editFootprint,     fpSelectedCondition );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( ACTIONS::save,                  fpSelectedCondition );
    ctxMenu.AddItem( ACTIONS::saveCopyAs,            fpSelectedCondition );
    ctxMenu.AddItem( PCB_ACTIONS::deleteFootprint,   fpSelectedCondition );
    ctxMenu.AddItem( ACTIONS::revert,                fpSelectedCondition );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( PCB_ACTIONS::cutFootprint,      fpSelectedCondition );
    ctxMenu.AddItem( PCB_ACTIONS::copyFootprint,     fpSelectedCondition );
    ctxMenu.AddItem( PCB_ACTIONS::pasteFootprint,    SELECTION_CONDITIONS::ShowAlways );

    ctxMenu.AddSeparator();
    ctxMenu.AddItem( PCB_ACTIONS::importFootprint,   SELECTION_CONDITIONS::ShowAlways );
    ctxMenu.AddItem( PCB_ACTIONS::exportFootprint,   fpSelectedCondition );

    return true;
}


int FOOTPRINT_EDITOR_TOOLS::NewFootprint( const TOOL_EVENT& aEvent )
{
    wxCommandEvent evt( wxEVT_NULL, ID_MODEDIT_NEW_MODULE );
    getEditFrame<FOOTPRINT_EDIT_FRAME>()->Process_Special_Functions( evt );
    return 0;
}


int FOOTPRINT_EDITOR_TOOLS::CreateFootprint( const TOOL_EVENT& aEvent )
{
    wxCommandEvent evt( wxEVT_NULL, ID_MODEDIT_NEW_MODULE_FROM_WIZARD );
    getEditFrame<FOOTPRINT_EDIT_FRAME>()->Process_Special_Functions( evt );
    return 0;
}


int FOOTPRINT_EDITOR_TOOLS::Save( const TOOL_EVENT& aEvent )
{
    wxCommandEvent evt( wxEVT_NULL, ID_MODEDIT_SAVE );
    getEditFrame<FOOTPRINT_EDIT_FRAME>()->Process_Special_Functions( evt );
    return 0;
}


int FOOTPRINT_EDITOR_TOOLS::SaveAs( const TOOL_EVENT& aEvent )
{
    wxCommandEvent evt( wxEVT_NULL, ID_MODEDIT_SAVE_AS );
    getEditFrame<FOOTPRINT_EDIT_FRAME>()->Process_Special_Functions( evt );
    return 0;
}


int FOOTPRINT_EDITOR_TOOLS::Revert( const TOOL_EVENT& aEvent )
{
    getEditFrame<FOOTPRINT_EDIT_FRAME>()->RevertFootprint();
    return 0;
}


int FOOTPRINT_EDITOR_TOOLS::CutCopyFootprint( const TOOL_EVENT& aEvent )
{
    LIB_ID fpID = m_frame->GetTreeFPID();

    if( fpID == m_frame->GetLoadedFPID() )
        m_copiedModule.reset( new MODULE( *m_frame->GetBoard()->GetFirstModule() ) );
    else
        m_copiedModule.reset( m_frame->LoadFootprint( fpID ) );

    if( aEvent.IsAction( &PCB_ACTIONS::cutFootprint ) )
        DeleteFootprint(aEvent );

    return 0;
}


int FOOTPRINT_EDITOR_TOOLS::PasteFootprint( const TOOL_EVENT& aEvent )
{
    if( m_copiedModule && !m_frame->GetTreeFPID().GetLibNickname().empty() )
    {
        wxString newLib = m_frame->GetTreeFPID().GetLibNickname();
        MODULE*  newModule( m_copiedModule.get() );
        wxString newName = newModule->GetFPID().GetLibItemName();

        while( m_frame->Prj().PcbFootprintLibs()->FootprintExists( newLib, newName ) )
            newName += _( "_copy" );

        newModule->SetFPID( LIB_ID( newLib, newName ) );
        m_frame->SaveFootprintInLibrary( newModule, newLib );

        m_frame->SyncLibraryTree( true );
        m_frame->FocusOnLibID( newModule->GetFPID() );
    }

    return 0;
}


int FOOTPRINT_EDITOR_TOOLS::DeleteFootprint( const TOOL_EVENT& aEvent )
{
    FOOTPRINT_EDIT_FRAME* frame = getEditFrame<FOOTPRINT_EDIT_FRAME>();

    if( frame->DeleteModuleFromLibrary( frame->GetTargetFPID(), true ) )
    {
        if( frame->GetTargetFPID() == frame->GetLoadedFPID() )
            frame->Clear_Pcb( false );

        frame->SyncLibraryTree( true );
    }

    return 0;
}


int FOOTPRINT_EDITOR_TOOLS::ImportFootprint( const TOOL_EVENT& aEvent )
{
    if( !m_frame->Clear_Pcb( true ) )
        return -1;                  // this command is aborted

    getViewControls()->SetCrossHairCursorPosition( VECTOR2D( 0, 0 ), false );
    m_frame->Import_Module();

    if( m_frame->GetBoard()->GetFirstModule() )
        m_frame->GetBoard()->GetFirstModule()->ClearFlags();

    frame()->ClearUndoRedoList();

    m_toolMgr->RunAction( ACTIONS::zoomFitScreen, true );
    m_frame->OnModify();
    return 0;
}


int FOOTPRINT_EDITOR_TOOLS::ExportFootprint( const TOOL_EVENT& aEvent )
{
    LIB_ID  fpID = m_frame->GetTreeFPID();
    MODULE* fp;

    if( !fpID.IsValid() )
        fp = m_frame->GetBoard()->GetFirstModule();
    else
        fp = m_frame->LoadFootprint( fpID );

    m_frame->Export_Module( fp );
    return 0;
}


int FOOTPRINT_EDITOR_TOOLS::EditFootprint( const TOOL_EVENT& aEvent )
{
    m_frame->LoadModuleFromLibrary( m_frame->GetTreeFPID() );
    return 0;
}


int FOOTPRINT_EDITOR_TOOLS::PinLibrary( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* currentNode = m_frame->GetCurrentTreeNode();

    if( currentNode && !currentNode->m_Pinned )
    {
        currentNode->m_Pinned = true;
        m_frame->RegenerateLibraryTree();
    }

    return 0;
}


int FOOTPRINT_EDITOR_TOOLS::UnpinLibrary( const TOOL_EVENT& aEvent )
{
    LIB_TREE_NODE* currentNode = m_frame->GetCurrentTreeNode();

    if( currentNode && currentNode->m_Pinned )
    {
        currentNode->m_Pinned = false;
        m_frame->RegenerateLibraryTree();
    }

    return 0;
}


int FOOTPRINT_EDITOR_TOOLS::ToggleFootprintTree( const TOOL_EVENT& aEvent )
{
    m_frame->ToggleSearchTree();
    return 0;
}


int FOOTPRINT_EDITOR_TOOLS::Properties( const TOOL_EVENT& aEvent )
{
    MODULE* footprint = m_frame->GetBoard()->GetFirstModule();

    if( footprint )
    {
        getEditFrame<FOOTPRINT_EDIT_FRAME>()->OnEditItemRequest( footprint );
        m_frame->GetCanvas()->Refresh();
    }
    return 0;
}


int FOOTPRINT_EDITOR_TOOLS::DefaultPadProperties( const TOOL_EVENT& aEvent )
{
    getEditFrame<FOOTPRINT_EDIT_FRAME>()->InstallPadOptionsFrame( nullptr );
    return 0;
}


int FOOTPRINT_EDITOR_TOOLS::CleanupGraphics( const TOOL_EVENT& aEvent )
{
    FOOTPRINT_EDIT_FRAME* editFrame = getEditFrame<FOOTPRINT_EDIT_FRAME>();
    DIALOG_CLEANUP_GRAPHICS dlg( editFrame, true );

    dlg.ShowModal();
    return 0;
}


void FOOTPRINT_EDITOR_TOOLS::setTransitions()
{
    Go( &FOOTPRINT_EDITOR_TOOLS::NewFootprint,         PCB_ACTIONS::newFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_TOOLS::CreateFootprint,      PCB_ACTIONS::createFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_TOOLS::Save,                 ACTIONS::save.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_TOOLS::Save,                 PCB_ACTIONS::saveToBoard.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_TOOLS::Save,                 PCB_ACTIONS::saveToLibrary.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_TOOLS::SaveAs,               ACTIONS::saveAs.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_TOOLS::SaveAs,               ACTIONS::saveCopyAs.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_TOOLS::Revert,               ACTIONS::revert.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_TOOLS::DeleteFootprint,      PCB_ACTIONS::deleteFootprint.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_TOOLS::EditFootprint,        PCB_ACTIONS::editFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_TOOLS::CutCopyFootprint,     PCB_ACTIONS::cutFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_TOOLS::CutCopyFootprint,     PCB_ACTIONS::copyFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_TOOLS::PasteFootprint,       PCB_ACTIONS::pasteFootprint.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_TOOLS::ImportFootprint,      PCB_ACTIONS::importFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_TOOLS::ExportFootprint,      PCB_ACTIONS::exportFootprint.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_TOOLS::CleanupGraphics,      PCB_ACTIONS::cleanupGraphics.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_TOOLS::PinLibrary,           ACTIONS::pinLibrary.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_TOOLS::UnpinLibrary,         ACTIONS::unpinLibrary.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_TOOLS::ToggleFootprintTree,  PCB_ACTIONS::toggleFootprintTree.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_TOOLS::Properties,           PCB_ACTIONS::footprintProperties.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_TOOLS::DefaultPadProperties, PCB_ACTIONS::defaultPadProperties.MakeEvent() );
}

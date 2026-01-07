/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <string_utils.h>
#include <pgm_base.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <tool/library_editor_control.h>
#include <tools/pcb_actions.h>
#include <footprint_editor_settings.h>
#include <eda_doc.h>
#include <footprint_edit_frame.h>
#include <generate_footprint_info.h>
#include <pcbnew_settings.h>
#include <pcbnew_id.h>
#include <confirm.h>
#include <kidialog.h>
#include <wx/filename.h>
#include <wildcards_and_files_ext.h>
#include <launch_ext.h> // To default when file manager setting is empty
#include <gestfich.h> // To open with a text editor
#include <widgets/wx_infobar.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_group.h>
#include <zone.h>
#include <footprint_library_adapter.h>
#include <dialogs/dialog_cleanup_graphics.h>
#include <dialogs/dialog_footprint_checker.h>
#include <dialogs/dialog_footprint_properties_fp_editor.h>
#include <footprint_wizard_frame.h>
#include <kiway.h>
#include <project_pcb.h>
#include <view/view_controls.h>

#include <memory>

#include "footprint_editor_control.h"


FOOTPRINT_EDITOR_CONTROL::FOOTPRINT_EDITOR_CONTROL() :
    PCB_TOOL_BASE( "pcbnew.ModuleEditor" ),
    m_frame( nullptr ),
    m_checkerDialog( nullptr )
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
    LIBRARY_EDITOR_CONTROL* libraryTreeTool = m_toolMgr->GetTool<LIBRARY_EDITOR_CONTROL>();

    // Build a context menu for the footprint tree
    //
    CONDITIONAL_MENU& ctxMenu = m_menu->GetMenu();

    auto libSelectedCondition =
            [this]( const SELECTION& aSel )
            {
                LIB_ID sel = m_frame->GetLibTree()->GetSelectedLibId();
                return !sel.GetLibNickname().empty() && sel.GetLibItemName().empty();
            };

    // The libInferredCondition allows you to do things like New Symbol and Paste with a
    // symbol selected (in other words, when we know the library context even if the library
    // itself isn't selected.
    auto libInferredCondition =
            [this]( const SELECTION& aSel )
            {
                LIB_ID sel = m_frame->GetLibTree()->GetSelectedLibId();
                return !sel.GetLibNickname().empty();
            };

    auto fpSelectedCondition =
            [this]( const SELECTION& aSel )
            {
                LIB_ID sel = m_frame->GetLibTree()->GetSelectedLibId();
                return !sel.GetLibNickname().empty() && !sel.GetLibItemName().empty();
            };

    auto fpExportCondition =
            [this]( const SELECTION& aSel )
            {
                FOOTPRINT* fp = m_frame->GetBoard()->GetFirstFootprint();
                return fp != nullptr;
            };

    auto canOpenExternally =
            [this]( const SELECTION& aSel )
            {
                // The option is shown if the editor has no current edits,
                // dumb/simple guard against opening a new file that does not exist on disk
                bool ret = !m_frame->IsContentModified();
                return ret;
            };

// clang-format off
    ctxMenu.AddItem( PCB_ACTIONS::newFootprint,       libSelectedCondition, 10 );
    ctxMenu.AddItem( PCB_ACTIONS::createFootprint,    libSelectedCondition, 10 );

    ctxMenu.AddSeparator( 10 );
    ctxMenu.AddItem( ACTIONS::save,                   SELECTION_CONDITIONS::ShowAlways, 10 );
    ctxMenu.AddItem( ACTIONS::saveAs,                 libSelectedCondition || fpSelectedCondition, 10 );
    ctxMenu.AddItem( ACTIONS::revert,                 libSelectedCondition || libInferredCondition, 10 );

    ctxMenu.AddSeparator( 10 );
    ctxMenu.AddItem( PCB_ACTIONS::cutFootprint,       fpSelectedCondition, 10 );
    ctxMenu.AddItem( PCB_ACTIONS::copyFootprint,      fpSelectedCondition, 10 );
    ctxMenu.AddItem( PCB_ACTIONS::pasteFootprint,     libInferredCondition, 10 );
    ctxMenu.AddItem( PCB_ACTIONS::duplicateFootprint, fpSelectedCondition, 10 );
    ctxMenu.AddItem( PCB_ACTIONS::renameFootprint,    fpSelectedCondition, 10 );
    ctxMenu.AddItem( PCB_ACTIONS::deleteFootprint,    fpSelectedCondition, 10 );
    ctxMenu.AddItem( PCB_ACTIONS::footprintProperties, fpSelectedCondition, 10 );

    ctxMenu.AddSeparator( 100 );
    ctxMenu.AddItem( PCB_ACTIONS::importFootprint,    libInferredCondition, 100 );
    ctxMenu.AddItem( PCB_ACTIONS::exportFootprint,    fpExportCondition, 100 );

    if( ADVANCED_CFG::GetCfg().m_EnableLibWithText )
    {
        ctxMenu.AddSeparator( 200 );
        ctxMenu.AddItem( ACTIONS::openWithTextEditor, canOpenExternally && fpSelectedCondition, 200 );
    }

    if( ADVANCED_CFG::GetCfg().m_EnableLibDir )
    {
        ctxMenu.AddSeparator( 200 );
        ctxMenu.AddItem( ACTIONS::openDirectory,  canOpenExternally && ( libSelectedCondition || fpSelectedCondition ), 200 );
    }
// clang-format on

    libraryTreeTool->AddContextMenuItems( &ctxMenu );

    // Ensure the left toolbar's Line modes group reflects the current setting at startup
    if( m_toolMgr )
        m_toolMgr->RunAction( PCB_ACTIONS::angleSnapModeChanged );

    return true;
}


void FOOTPRINT_EDITOR_CONTROL::tryToSaveFootprintInLibrary( FOOTPRINT&    aFootprint,
                                                            const LIB_ID& aTargetLib )
{
    const wxString libraryName = aTargetLib.GetUniStringLibNickname();

    if( aTargetLib.GetLibNickname().empty() )
    {
        // Do nothing - the footprint will need to be saved manually to assign
        // to a library.
    }
    else
    {
        if( !PROJECT_PCB::FootprintLibAdapter( &m_frame->Prj() )->IsFootprintLibWritable( libraryName ) )
        {
            // If the library is not writeable, we'll give the user a
            // footprint not in a library. But add a warning to let them know
            // they didn't quite get what they wanted.
            m_frame->ShowInfoBarWarning(
                    wxString::Format(
                            _( "The footprint could not be added to the selected library ('%s'). "
                               "This library is read-only." ),
                            libraryName ),
                    false );
            // And the footprint will need to be saved manually
        }
        else
        {
            // Go ahead and save it to the library
            LIB_ID fpid = aFootprint.GetFPID();
            fpid.SetLibNickname( aTargetLib.GetLibNickname() );
            aFootprint.SetFPID( fpid );
            m_frame->SaveFootprint( &aFootprint );
            m_frame->ClearModify();
        }
    }
}


int FOOTPRINT_EDITOR_CONTROL::NewFootprint( const TOOL_EVENT& aEvent )
{
    const LIB_ID   selected = m_frame->GetTargetFPID();
    const wxString libraryName = selected.GetUniStringLibNickname();
    FOOTPRINT*     newFootprint = m_frame->CreateNewFootprint( wxEmptyString, libraryName );

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

    tryToSaveFootprintInLibrary( *newFootprint, selected );

    m_frame->UpdateView();
    m_frame->GetCanvas()->ForceRefresh();
    m_frame->Update3DView( true, true );

    m_frame->SyncLibraryTree( false );
    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::CreateFootprint( const TOOL_EVENT& aEvent )
{
    LIB_ID selected = m_frame->GetLibTree()->GetSelectedLibId();

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

    if( KIWAY_PLAYER* frame = m_frame->Kiway().Player( FRAME_FOOTPRINT_WIZARD, true, m_frame ) )
    {
        FOOTPRINT_WIZARD_FRAME* wizard = static_cast<FOOTPRINT_WIZARD_FRAME*>( frame );

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

                // Initialize data relative to nets and netclasses (for a new footprint the
                // defaults are used).  This is mandatory to handle and draw pads.
                board()->BuildListOfNets();
                newFootprint->SetPosition( VECTOR2I( 0, 0 ) );
                newFootprint->ClearFlags();

                m_frame->Zoom_Automatique( false );
                m_frame->GetScreen()->SetContentModified();
                m_frame->OnModify();

                tryToSaveFootprintInLibrary( *newFootprint, selected );

                m_frame->UpdateView();
                canvas()->Refresh();
                m_frame->Update3DView( true, true );

                m_frame->SyncLibraryTree( false );
            }
        }

        wizard->Destroy();
    }

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
        LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();

        // Save Library As
        const wxString& src_libNickname = m_frame->GetTargetFPID().GetLibNickname();
        std::optional<wxString> optUri = manager.GetFullURI( LIBRARY_TABLE_TYPE::FOOTPRINT, src_libNickname, true );
        wxCHECK( optUri, 0 );

        if( m_frame->SaveLibraryAs( *optUri ) )
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
    LIB_ID fpID = m_frame->GetLibTree()->GetSelectedLibId();

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
    if( m_copiedFootprint && !m_frame->GetLibTree()->GetSelectedLibId().GetLibNickname().empty() )
    {
        wxString newLib = m_frame->GetLibTree()->GetSelectedLibId().GetLibNickname();
        wxString newName = m_copiedFootprint->GetFPID().GetLibItemName();

        while( PROJECT_PCB::FootprintLibAdapter( &m_frame->Prj() )->FootprintExists( newLib, newName ) )
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
    LIB_ID     fpID = m_frame->GetLibTree()->GetSelectedLibId();
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


int FOOTPRINT_EDITOR_CONTROL::RenameFootprint( const TOOL_EVENT& aEvent )
{
    LIBRARY_EDITOR_CONTROL* libTool   = m_toolMgr->GetTool<LIBRARY_EDITOR_CONTROL>();
    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &m_frame->Prj() );

    LIB_ID   fpID = m_frame->GetLibTree()->GetSelectedLibId();
    wxString libraryName = fpID.GetLibNickname();
    wxString oldName = fpID.GetLibItemName();
    wxString newName;
    wxString msg;

    if( !libTool->RenameLibrary( _( "Change Footprint Name" ), oldName,
            [&]( const wxString& aNewName )
            {
                newName = aNewName;

                if( newName.IsEmpty() )
                {
                    wxMessageBox( _( "Footprint must have a name." ) );
                    return false;
                }

                // If no change, accept it without prompting
                if( oldName != newName && adapter->FootprintExists( libraryName, newName ) )
                {
                    msg = wxString::Format( _( "Footprint '%s' already exists in library '%s'." ),
                                            newName, libraryName );

                    KIDIALOG errorDlg( m_frame, msg, _( "Confirmation" ),
                                       wxOK | wxCANCEL | wxICON_WARNING );
                    errorDlg.SetOKLabel( _( "Overwrite" ) );

                    return errorDlg.ShowModal() == wxID_OK;
                }

                return true;
            } ) )
    {
        return 0;   // canceled by user
    }

    if( newName == oldName )
        return 0;

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

                adapter->DeleteFootprint( libraryName, oldName );
            }
            catch( const IO_ERROR& ioe )
            {
                DisplayErrorMessage( m_frame, _( "Error renaming footprint" ), ioe.What() );
            }
            catch( ... )
            {
                // Best efforts...
            }
        }
    }

    wxDataViewItem treeItem = m_frame->GetLibTreeAdapter()->FindItem( fpID );

    if( footprint )
    {
        m_frame->UpdateLibraryTree( treeItem, footprint );
        m_frame->FocusOnLibID( LIB_ID( libraryName, newName ) );
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


int FOOTPRINT_EDITOR_CONTROL::OpenDirectory( const TOOL_EVENT& aEvent )
{
    // No check for multi selection since the context menu option must be hidden in that case
    LIB_ID        libId = m_frame->GetTargetFPID();

    wxString    libName = libId.GetLibNickname();
    wxString    libItemName = libId.GetLibItemName();
    wxString    path = wxEmptyString;

    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();
    std::optional<wxString> optUri = manager.GetFullURI( LIBRARY_TABLE_TYPE::FOOTPRINT, libName, true );

    if( !optUri )
        return 0;

    path = *optUri;

    wxString fileExt = wxEmptyString;

    // If selection is footprint
    if( !libItemName.IsEmpty() )
        fileExt = FILEEXT::KiCadFootprintFileExtension;

    wxFileName fileName( path, libItemName, fileExt );
    wxString   explorerCommand;

    if( COMMON_SETTINGS* cfg = Pgm().GetCommonSettings() )
        explorerCommand = cfg->m_System.file_explorer;

    if( explorerCommand.IsEmpty() )
    {
        path = fileName.GetFullPath().BeforeLast( wxFileName::GetPathSeparator() );

        if( !path.IsEmpty() && wxDirExists( path ) )
            LaunchExternal( path );

        return 0;
    }

    if( !explorerCommand.EndsWith( "%F" ) )
    {
        wxMessageBox( _( "Missing/malformed file explorer argument '%F' in common settings." ) );
        return 0;
    }

    wxString escapedFilePath = fileName.GetFullPath();
    escapedFilePath.Replace( wxS( "\"" ), wxS( "_" ) );

    wxString fileArg = wxEmptyString;
    fileArg << '"' << escapedFilePath << '"';

    explorerCommand.Replace( wxT( "%F" ), fileArg );

    if( !explorerCommand.IsEmpty() )
        wxExecute( explorerCommand );

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

    // No check for multi selection since the context menu option must be hidden in that case
    LIB_ID        libId = m_frame->GetLibTree()->GetSelectedLibId();

    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();

    wxString    libName = libId.GetLibNickname();
    wxString    libItemName = wxEmptyString;

    std::optional<wxString> optUri = manager.GetFullURI( LIBRARY_TABLE_TYPE::FOOTPRINT, libName, true );

    if( !optUri )
        return 0;

    libItemName = *optUri;
    libItemName << wxFileName::GetPathSeparator();
    libItemName << libId.GetLibItemName();
    libItemName << '.' + FILEEXT::KiCadFootprintFileExtension;

    if( !wxFileName::FileExists( libItemName ) )
        return 0;

    ExecuteFile( fullEditorName, libItemName.wc_str(), nullptr, false );

    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::ShowDatasheet( const TOOL_EVENT& aEvent )
{
    if( FOOTPRINT* footprint = m_frame->GetBoard()->GetFirstFootprint() )
    {
        std::optional<wxString> url = GetFootprintDocumentationURL( *footprint );

        if( !url.has_value() )
        {
            frame()->ShowInfoBarMsg( _( "No datasheet found in the footprint." ) );
        }
        else
        {
            // Only absolute URLs are supported
            SEARCH_STACK* searchStack = nullptr;
            GetAssociatedDocument( m_frame, *url, &m_frame->Prj(), searchStack,
                                   { m_frame->GetBoard(), footprint } );
        }
    }
    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::EditFootprint( const TOOL_EVENT& aEvent )
{
    m_frame->LoadFootprintFromLibrary( m_frame->GetLibTree()->GetSelectedLibId() );
    return 0;
}


int FOOTPRINT_EDITOR_CONTROL::EditLibraryFootprint( const TOOL_EVENT& aEvent )
{
    FOOTPRINT* footprint = m_frame->GetBoard()->GetFirstFootprint();

    if( !footprint || !m_frame->IsCurrentFPFromBoard() )
    {
        wxBell();
        return 0;
    }

    m_frame->LoadFootprintFromLibrary( footprint->GetFPID() );

    if( !m_frame->IsLibraryTreeShown() )
        m_frame->ToggleLibraryTree();

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
    // Check if called from tree context menu
    if( aEvent.IsAction( &PCB_ACTIONS::footprintProperties ) )
    {
        LIB_ID treeLibId = m_frame->GetLibTree()->GetSelectedLibId();

        // Check if a different footprint is selected in the tree
        if( treeLibId.IsValid()
                && (  !m_frame->GetBoard()->GetFirstFootprint()
                    || m_frame->GetBoard()->GetFirstFootprint()->GetFPID() != treeLibId ) )
        {
            // Edit properties directly from library without loading to canvas
            editFootprintPropertiesFromLibrary( treeLibId );
            return 0;
        }
    }

    if( FOOTPRINT* footprint = m_frame->GetBoard()->GetFirstFootprint() )
    {
        getEditFrame<FOOTPRINT_EDIT_FRAME>()->OnEditItemRequest( footprint );
        m_frame->GetCanvas()->Refresh();
    }

    return 0;
}


void FOOTPRINT_EDITOR_CONTROL::editFootprintPropertiesFromLibrary( const LIB_ID& aLibId )
{
    // Load the footprint from the library (without adding it to the canvas)
    FOOTPRINT* libraryFootprint = m_frame->LoadFootprint( aLibId );

    if( !libraryFootprint )
        return;

    // Create a temporary board to hold the footprint (required by the dialog)
    std::unique_ptr<BOARD> tempBoard( new BOARD() );

    // Set up the temp board with the current project and board settings.
    // Use reference-only mode to avoid modifying the project's settings.
    tempBoard->SetDesignSettings( m_frame->GetBoard()->GetDesignSettings() );
    tempBoard->SetProject( &m_frame->Prj(), true );
    tempBoard->SetBoardUse( BOARD_USE::FPHOLDER );
    tempBoard->SynchronizeProperties();

    // Create a copy to work with and add it to the temporary board
    FOOTPRINT* tempFootprint = static_cast<FOOTPRINT*>( libraryFootprint->Clone() );
    delete libraryFootprint;

    tempBoard->Add( tempFootprint );
    tempFootprint->SetParent( tempBoard.get() );

    LIB_ID oldFPID = tempFootprint->GetFPID();

    // Open the properties dialog
    DIALOG_FOOTPRINT_PROPERTIES_FP_EDITOR dialog( m_frame, tempFootprint );

    if( dialog.ShowQuasiModal() != wxID_OK )
        return;

    // Remove from temporary board before saving (to avoid double-delete)
    tempBoard->Remove( tempFootprint );

    // Save the modified footprint back to the library
    FOOTPRINT_LIBRARY_ADAPTER* adapter = PROJECT_PCB::FootprintLibAdapter( &m_frame->Prj() );
    wxString libName = aLibId.GetLibNickname();

    try
    {
        adapter->SaveFootprint( libName, tempFootprint, true );

        // Update the tree view
        wxDataViewItem treeItem = m_frame->GetLibTreeAdapter()->FindItem( oldFPID );
        m_frame->UpdateLibraryTree( treeItem, tempFootprint );
        m_frame->SyncLibraryTree( true );

        // Clean up
        delete tempFootprint;
    }
    catch( const IO_ERROR& ioe )
    {
        delete tempFootprint;
        DisplayError( m_frame, ioe.What() );
    }
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
    // clang-format off
    Go( &FOOTPRINT_EDITOR_CONTROL::NewFootprint,         PCB_ACTIONS::newFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::CreateFootprint,      PCB_ACTIONS::createFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::Save,                 ACTIONS::save.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::SaveAs,               ACTIONS::saveAs.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::Revert,               ACTIONS::revert.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::DuplicateFootprint,   PCB_ACTIONS::duplicateFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::RenameFootprint,      PCB_ACTIONS::renameFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::DeleteFootprint,      PCB_ACTIONS::deleteFootprint.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_CONTROL::EditFootprint,        PCB_ACTIONS::editFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::EditLibraryFootprint, PCB_ACTIONS::editLibFpInFpEditor.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::CutCopyFootprint,     PCB_ACTIONS::cutFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::CutCopyFootprint,     PCB_ACTIONS::copyFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::PasteFootprint,       PCB_ACTIONS::pasteFootprint.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_CONTROL::ImportFootprint,      PCB_ACTIONS::importFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::ExportFootprint,      PCB_ACTIONS::exportFootprint.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_CONTROL::OpenWithTextEditor,   ACTIONS::openWithTextEditor.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::OpenDirectory,        ACTIONS::openDirectory.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::ShowDatasheet,        ACTIONS::showDatasheet.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_CONTROL::EditTextAndGraphics,  PCB_ACTIONS::editTextAndGraphics.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::CleanupGraphics,      PCB_ACTIONS::cleanupGraphics.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_CONTROL::CheckFootprint,       PCB_ACTIONS::checkFootprint.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::RepairFootprint,      PCB_ACTIONS::repairFootprint.MakeEvent() );

    Go( &FOOTPRINT_EDITOR_CONTROL::Properties,           PCB_ACTIONS::footprintProperties.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::DefaultPadProperties, PCB_ACTIONS::defaultPadProperties.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::ToggleLayersManager,  PCB_ACTIONS::showLayersManager.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::ToggleProperties,     ACTIONS::showProperties.MakeEvent() );
    // clang-format on

    // Line modes for the footprint editor: explicit modes, next-mode, and toolbar sync
    Go( &FOOTPRINT_EDITOR_CONTROL::ChangeLineMode,       PCB_ACTIONS::lineModeFree.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::ChangeLineMode,       PCB_ACTIONS::lineMode90.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::ChangeLineMode,       PCB_ACTIONS::lineMode45.MakeEvent() );
    Go( &FOOTPRINT_EDITOR_CONTROL::OnAngleSnapModeChanged,
                                                   PCB_ACTIONS::angleSnapModeChanged.MakeEvent() );
}

int FOOTPRINT_EDITOR_CONTROL::ChangeLineMode( const TOOL_EVENT& aEvent )
{
    LEADER_MODE mode = aEvent.Parameter<LEADER_MODE>();
    GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" )->m_AngleSnapMode = mode;
    m_toolMgr->PostAction( ACTIONS::refreshPreview );
    m_toolMgr->RunAction( PCB_ACTIONS::angleSnapModeChanged );
    return 0;
}

int FOOTPRINT_EDITOR_CONTROL::OnAngleSnapModeChanged( const TOOL_EVENT& aEvent )
{
    FOOTPRINT_EDIT_FRAME* f = getEditFrame<FOOTPRINT_EDIT_FRAME>();

    if( !f )
        return 0;

    LEADER_MODE mode = GetAppSettings<FOOTPRINT_EDITOR_SETTINGS>( "fpedit" )->m_AngleSnapMode;

    switch( mode )
    {
    case LEADER_MODE::DIRECT: f->SelectToolbarAction( PCB_ACTIONS::lineModeFree ); break;
    case LEADER_MODE::DEG90:  f->SelectToolbarAction( PCB_ACTIONS::lineMode90 );   break;
    default:
    case LEADER_MODE::DEG45:  f->SelectToolbarAction( PCB_ACTIONS::lineMode45 );   break;
    }

    return 0;
}

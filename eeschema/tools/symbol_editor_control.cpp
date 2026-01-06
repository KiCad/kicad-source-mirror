/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "tools/symbol_editor_control.h"

#include <advanced_config.h>
#include <bitmaps/bitmap_types.h>
#include <confirm.h>
#include <dialogs/dialog_lib_fields_table.h>
#include <gestfich.h> // To open with a text editor
#include <kidialog.h>
#include <kiway.h>
#include <launch_ext.h> // To default when file manager setting is empty
#include <lib_symbol_library_manager.h>
#include <libraries/library_manager.h>
#include <pgm_base.h>
#include <sch_painter.h>
#include <string_utils.h>
#include <symbol_editor/symbol_editor_settings.h>
#include <symbol_tree_model_adapter.h>
#include <symbol_viewer_frame.h>
#include <tool/library_editor_control.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <wildcards_and_files_ext.h>

#include <wx/filedlg.h>


bool SYMBOL_EDITOR_CONTROL::Init()
{
    m_frame = getEditFrame<SCH_BASE_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<SCH_SELECTION_TOOL>();
    m_isSymbolEditor = m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR );

    if( m_isSymbolEditor )
    {
        LIBRARY_EDITOR_CONTROL* libraryTreeTool = m_toolMgr->GetTool<LIBRARY_EDITOR_CONTROL>();
        CONDITIONAL_MENU&       ctxMenu = m_menu->GetMenu();

        auto libSelectedCondition =
                [this]( const SELECTION& aSel )
                {
                    if( SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>() )
                    {
                        LIB_ID sel = editFrame->GetTreeLIBID();
                        return !sel.GetLibNickname().empty() && sel.GetLibItemName().empty();
                    }

                    return false;
                };

        // The libInferredCondition allows you to do things like New Symbol and Paste with a
        // symbol selected (in other words, when we know the library context even if the library
        // itself isn't selected.
        auto libInferredCondition =
                [this]( const SELECTION& aSel )
                {
                    if( SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>() )
                    {
                        LIB_ID sel = editFrame->GetTreeLIBID();
                        return !sel.GetLibNickname().empty();
                    }

                    return false;
                };

        auto symbolSelectedCondition =
                [this]( const SELECTION& aSel )
                {
                    if( SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>() )
                    {
                        LIB_ID sel = editFrame->GetTargetLibId();
                        return !sel.GetLibNickname().empty() && !sel.GetLibItemName().empty();
                    }

                    return false;
                };

        auto derivedSymbolSelectedCondition =
                [this]( const SELECTION& aSel )
                {
                    if( SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>() )
                    {
                        LIB_ID sel = editFrame->GetTargetLibId();

                        if( sel.GetLibNickname().empty() || sel.GetLibItemName().empty() )
                            return false;

                        LIB_SYMBOL_LIBRARY_MANAGER& libMgr = editFrame->GetLibManager();
                        const LIB_SYMBOL* sym = libMgr.GetSymbol( sel.GetLibItemName(), sel.GetLibNickname() );

                        return sym && sym->IsDerived();
                    }

                    return false;
                };

        auto relatedSymbolSelectedCondition =
                [this]( const SELECTION& aSel )
                {
                    if( SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>() )
                    {
                        LIB_ID sel = editFrame->GetTargetLibId();

                        if( sel.GetLibNickname().empty() || sel.GetLibItemName().empty() )
                            return false;

                        LIB_SYMBOL_LIBRARY_MANAGER& libMgr = editFrame->GetLibManager();
                        const LIB_SYMBOL* sym = libMgr.GetSymbol( sel.GetLibItemName(), sel.GetLibNickname() );
                        wxArrayString     derived;

                        libMgr.GetDerivedSymbolNames( sel.GetLibItemName(), sel.GetLibNickname(), derived );

                        return ( sym && sym->IsDerived() ) || !derived.IsEmpty();
                    }

                    return false;
                };

        auto multiSymbolSelectedCondition =
                [this]( const SELECTION& aSel )
                {
                    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();

                    if( editFrame && editFrame->GetTreeSelectionCount() > 1 )
                    {
                        for( LIB_ID& sel : editFrame->GetSelectedLibIds() )
                        {
                            if( !sel.IsValid() )
                                return false;
                        }

                        return true;
                    }

                    return false;
                };
/* not used, yet
        auto multiLibrarySelectedCondition =
                [this]( const SELECTION& aSel )
                {
                    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();

                    if( editFrame && editFrame->GetTreeSelectionCount() > 1 )
                    {
                        for( LIB_ID& sel : editFrame->GetSelectedLibIds() )
                        {
                            if( sel.IsValid() )
                                return false;
                        }

                        return true;
                    }

                    return false;
                };
*/
        auto canOpenExternally =
                [this]( const SELECTION& aSel )
                {
                    // The option is shown if the lib has no current edits
                    if( SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>() )
                    {
                        LIB_SYMBOL_LIBRARY_MANAGER& libMgr = editFrame->GetLibManager();
                        wxString                    libName = editFrame->GetTargetLibId().GetLibNickname();
                        return !libMgr.IsLibraryModified( libName );
                    }

                    return false;
                };


// clang-format off
        ctxMenu.AddItem( SCH_ACTIONS::newSymbol,                libInferredCondition, 10 );
        ctxMenu.AddItem( SCH_ACTIONS::deriveFromExistingSymbol, symbolSelectedCondition, 10 );

        ctxMenu.AddSeparator( 10 );
        ctxMenu.AddItem( ACTIONS::save,                         symbolSelectedCondition || libInferredCondition, 10 );
        ctxMenu.AddItem( SCH_ACTIONS::saveLibraryAs,            libSelectedCondition, 10 );
        ctxMenu.AddItem( SCH_ACTIONS::saveSymbolAs,             symbolSelectedCondition, 10 );
        ctxMenu.AddItem( SCH_ACTIONS::saveSymbolCopyAs,         symbolSelectedCondition, 10 );
        ctxMenu.AddItem( ACTIONS::revert,                       symbolSelectedCondition || libInferredCondition, 10 );

        ctxMenu.AddSeparator( 20 );
        ctxMenu.AddItem( SCH_ACTIONS::importSymbol,             libInferredCondition, 20 );
        ctxMenu.AddItem( SCH_ACTIONS::exportSymbol,             symbolSelectedCondition, 20 );

        ctxMenu.AddSeparator( 100 );
        ctxMenu.AddItem( SCH_ACTIONS::cutSymbol,                 symbolSelectedCondition || multiSymbolSelectedCondition, 100 );
        ctxMenu.AddItem( SCH_ACTIONS::copySymbol,                symbolSelectedCondition || multiSymbolSelectedCondition, 100 );
        ctxMenu.AddItem( SCH_ACTIONS::pasteSymbol,               libInferredCondition, 100 );
        ctxMenu.AddItem( SCH_ACTIONS::duplicateSymbol,           symbolSelectedCondition, 100 );
        ctxMenu.AddItem( SCH_ACTIONS::deleteSymbol,              symbolSelectedCondition || multiSymbolSelectedCondition, 100 );

        ctxMenu.AddSeparator( 120 );
        ctxMenu.AddItem( SCH_ACTIONS::renameSymbol,              symbolSelectedCondition, 120 );
        ctxMenu.AddItem( SCH_ACTIONS::symbolProperties,          symbolSelectedCondition, 120 );
        ctxMenu.AddItem( SCH_ACTIONS::flattenSymbol,             derivedSymbolSelectedCondition, 120 );

        if( ADVANCED_CFG::GetCfg().m_EnableLibWithText || ADVANCED_CFG::GetCfg().m_EnableLibDir )
            ctxMenu.AddSeparator( 200 );

        if( ADVANCED_CFG::GetCfg().m_EnableLibWithText )
            ctxMenu.AddItem( ACTIONS::openWithTextEditor,        canOpenExternally && ( symbolSelectedCondition || libSelectedCondition ), 200 );

        if( ADVANCED_CFG::GetCfg().m_EnableLibDir )
            ctxMenu.AddItem( ACTIONS::openDirectory,             canOpenExternally && ( symbolSelectedCondition || libSelectedCondition ), 200 );

        ctxMenu.AddSeparator( 300 );
        ctxMenu.AddItem( SCH_ACTIONS::showLibFieldsTable,        libInferredCondition, 300 );
        ctxMenu.AddItem( SCH_ACTIONS::showRelatedLibFieldsTable, relatedSymbolSelectedCondition,  300 );

        libraryTreeTool->AddContextMenuItems( &ctxMenu );
    }
// clang-format on

    return true;
}


int SYMBOL_EDITOR_CONTROL::AddLibrary( const TOOL_EVENT& aEvent )
{
    bool createNew = aEvent.IsAction( &ACTIONS::newLibrary );

    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
        static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->AddLibraryFile( createNew );

    return 0;
}


int SYMBOL_EDITOR_CONTROL::DdAddLibrary( const TOOL_EVENT& aEvent )
{
    wxString libFile = *aEvent.Parameter<wxString*>();

    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
        static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->DdAddLibrary( libFile );

    return 0;
}


int SYMBOL_EDITOR_CONTROL::EditSymbol( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    int                unit = 0;
    LIB_ID             partId = editFrame->GetTreeLIBID( &unit );

    editFrame->LoadSymbol( partId.GetLibItemName(), partId.GetLibNickname(), unit );
    return 0;
}


int SYMBOL_EDITOR_CONTROL::EditLibrarySymbol( const TOOL_EVENT& aEvent )
{
    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    const LIB_SYMBOL*  symbol = editFrame->GetCurSymbol();

    if( !symbol || !editFrame->IsSymbolFromSchematic() )
    {
        wxBell();
        return 0;
    }

    const LIB_ID& libId = symbol->GetLibId();

    if( editFrame->LoadSymbol( libId, editFrame->GetUnit(), editFrame->GetBodyStyle() ) )
    {
        if( !editFrame->IsLibraryTreeShown() )
            editFrame->ToggleLibraryTree();
    }
    else
    {
        const wxString libName = libId.GetLibNickname();
        const wxString symbolName = libId.GetLibItemName();

        DisplayError( editFrame,
                      wxString::Format( _( "Failed to load symbol %s from "
                                           "library %s." ),
                                        UnescapeString( symbolName ), UnescapeString( libName ) ) );
    }
    return 0;
}


int SYMBOL_EDITOR_CONTROL::AddSymbol( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    LIB_ID             target = editFrame->GetTargetLibId();
    const wxString&    libName = target.GetLibNickname();
    wxString           msg;

    if( libName.IsEmpty() )
    {
        msg.Printf( _( "No symbol library selected." ) );
        m_frame->ShowInfoBarError( msg );
        return 0;
    }

    if( !editFrame->GetLibManager().LibraryExists( libName ) )
    {
        msg.Printf( _( "Symbol library '%s' not found." ), libName );
        m_frame->ShowInfoBarError( msg );
        return 0;
    }

    if( editFrame->GetLibManager().IsLibraryReadOnly( libName ) )
    {
        msg.Printf( _( "Symbol library '%s' is not writable." ), libName );
        m_frame->ShowInfoBarError( msg );
        return 0;
    }

    if( aEvent.IsAction( &SCH_ACTIONS::newSymbol ) )
        editFrame->CreateNewSymbol();
    else if( aEvent.IsAction( &SCH_ACTIONS::deriveFromExistingSymbol ) )
        editFrame->CreateNewSymbol( target.GetLibItemName() );
    else if( aEvent.IsAction( &SCH_ACTIONS::importSymbol ) )
        editFrame->ImportSymbol();

    return 0;
}


int SYMBOL_EDITOR_CONTROL::Save( const TOOL_EVENT& aEvt )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();

    if( aEvt.IsAction( &SCH_ACTIONS::save ) )
        editFrame->Save();
    else if( aEvt.IsAction( &SCH_ACTIONS::saveLibraryAs ) )
        editFrame->SaveLibraryAs();
    else if( aEvt.IsAction( &SCH_ACTIONS::saveSymbolAs ) )
        editFrame->SaveSymbolCopyAs( true );
    else if( aEvt.IsAction( &SCH_ACTIONS::saveSymbolCopyAs ) )
        editFrame->SaveSymbolCopyAs( false );
    else if( aEvt.IsAction( &SCH_ACTIONS::saveAll ) )
        editFrame->SaveAll();

    return 0;
}


int SYMBOL_EDITOR_CONTROL::Revert( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
        static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->Revert();

    return 0;
}


int SYMBOL_EDITOR_CONTROL::OpenDirectory( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();
    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();

    LIB_ID libId = editFrame->GetTreeLIBID();

    wxString libName = libId.GetLibNickname();
    std::optional<wxString> libItemName =
        manager.GetFullURI( LIBRARY_TABLE_TYPE::SYMBOL, libName, true );

    wxCHECK( libItemName, 0 );

    wxFileName fileName( *libItemName );

    wxString filePath = wxEmptyString;
    wxString explorerCommand;

    if( COMMON_SETTINGS* cfg = Pgm().GetCommonSettings() )
        explorerCommand = cfg->m_System.file_explorer;

    if( explorerCommand.IsEmpty() )
    {
        filePath = fileName.GetFullPath().BeforeLast( wxFileName::GetPathSeparator() );

        if( !filePath.IsEmpty() && wxDirExists( filePath ) )
            LaunchExternal( filePath );

        return 0;
    }

    if( !explorerCommand.EndsWith( "%F" ) )
    {
        wxMessageBox( _( "Missing/malformed file explorer argument '%F' in common settings." ) );
        return 0;
    }

    filePath = fileName.GetFullPath();
    filePath.Replace( wxS( "\"" ), wxS( "_" ) );

    wxString fileArg = '"' + filePath + '"';

    explorerCommand.Replace( wxT( "%F" ), fileArg );

    if( !explorerCommand.IsEmpty() )
        wxExecute( explorerCommand );

    return 0;
}


int SYMBOL_EDITOR_CONTROL::OpenWithTextEditor( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    wxString           textEditorName = Pgm().GetTextEditor();

    if( textEditorName.IsEmpty() )
    {
        wxMessageBox( _( "No text editor selected in KiCad. Please choose one." ) );
        return 0;
    }

    LIBRARY_MANAGER& manager = Pgm().GetLibraryManager();

    LIB_ID   libId = editFrame->GetTreeLIBID();
    wxString libName = libId.GetLibNickname();

    std::optional<wxString> optUri =
        manager.GetFullURI( LIBRARY_TABLE_TYPE::SYMBOL, libName, true );

    wxCHECK( optUri, 0 );

    wxString tempFName = ( *optUri ).wc_str();

    if( !tempFName.IsEmpty() )
        ExecuteFile( textEditorName, tempFName, nullptr, false );

    return 0;
}


int SYMBOL_EDITOR_CONTROL::CutCopyDelete( const TOOL_EVENT& aEvt )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();

    if( aEvt.IsAction( &SCH_ACTIONS::cutSymbol ) || aEvt.IsAction( &SCH_ACTIONS::copySymbol ) )
        editFrame->CopySymbolToClipboard();

    if( aEvt.IsAction( &SCH_ACTIONS::cutSymbol ) || aEvt.IsAction( &SCH_ACTIONS::deleteSymbol ) )
    {
        bool hasWritableLibs = false;
        wxString msg;

        for( LIB_ID& sel : editFrame->GetSelectedLibIds() )
        {
            const wxString& libName = sel.GetLibNickname();

            if( !editFrame->GetLibManager().LibraryExists( libName ) )
                msg.Printf( _( "Symbol library '%s' not found." ), libName );
            else if( editFrame->GetLibManager().IsLibraryReadOnly( libName ) )
                msg.Printf( _( "Symbol library '%s' is not writable." ), libName );
            else
                hasWritableLibs = true;
        }

        if( !msg.IsEmpty() )
            m_frame->ShowInfoBarError( msg );

        if( !hasWritableLibs )
            return 0;

        editFrame->DeleteSymbolFromLibrary();
    }

    return 0;
}


int SYMBOL_EDITOR_CONTROL::DuplicateSymbol( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    LIB_ID             sel = editFrame->GetTargetLibId();
    // DuplicateSymbol() is called to duplicate a symbol, or to paste a previously
    // saved symbol in clipboard
    bool               isPasteAction = aEvent.IsAction( &SCH_ACTIONS::pasteSymbol );
    wxString           msg;

    if( !sel.IsValid() && !isPasteAction )
    {
        // When duplicating a symbol, a source symbol must exists.
        msg.Printf( _( "No symbol selected" ) );
        m_frame->ShowInfoBarError( msg );
        return 0;
    }

    const wxString& libName = sel.GetLibNickname();

    if( !editFrame->GetLibManager().LibraryExists( libName ) )
    {
        msg.Printf( _( "Symbol library '%s' not found." ), libName );
        m_frame->ShowInfoBarError( msg );
        return 0;
    }

    if( editFrame->GetLibManager().IsLibraryReadOnly( libName ) )
    {
        msg.Printf( _( "Symbol library '%s' is not writable." ), libName );
        m_frame->ShowInfoBarError( msg );
        return 0;
    }

    editFrame->DuplicateSymbol( isPasteAction );
    return 0;
}


int SYMBOL_EDITOR_CONTROL::RenameSymbol( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME*          editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    LIB_SYMBOL_LIBRARY_MANAGER& libMgr    = editFrame->GetLibManager();
    LIBRARY_EDITOR_CONTROL*     libTool   = m_toolMgr->GetTool<LIBRARY_EDITOR_CONTROL>();

    LIB_ID   libId = editFrame->GetTreeLIBID();
    wxString libName = libId.GetLibNickname();
    wxString oldName = libId.GetLibItemName();
    wxString newName;
    wxString msg;

    if( !libMgr.LibraryExists( libName ) )
        return 0;

    if( !libTool->RenameLibrary( _( "Change Symbol Name" ), oldName,
            [&]( const wxString& aNewName )
            {
                newName = EscapeString( aNewName, CTX_LIBID );

                if( newName.IsEmpty() )
                {
                    wxMessageBox( _( "Symbol must have a name." ) );
                    return false;
                }

                // If no change, accept it without prompting
                if( newName != oldName && libMgr.SymbolNameInUse( newName, libName ) )
                {
                    msg.Printf( _( "Symbol '%s' already exists in library '%s'." ),
                                UnescapeString( newName ),
                                libName );

                    KIDIALOG errorDlg( m_frame, msg, _( "Confirmation" ),
                                       wxOK | wxCANCEL | wxICON_WARNING );

                    errorDlg.SetOKLabel( _( "Overwrite" ) );

                    return errorDlg.ShowModal() == wxID_OK;
                }

                return true;
            } ) )
    {
        return 0;   // cancelled by user
    }

    if( newName == oldName )
        return 0;

    LIB_SYMBOL* libSymbol = libMgr.GetBufferedSymbol( oldName, libName );

    if( !libSymbol )
        return 0;

    // Renaming the current symbol
    const bool isCurrentSymbol = editFrame->IsCurrentSymbol( libId );
    bool       overwritingCurrentSymbol = false;

    if( libMgr.SymbolExists( newName, libName ) )
    {
        // Overwriting the current symbol also need to update the open symbol
        LIB_SYMBOL* const overwrittenSymbol = libMgr.GetBufferedSymbol( newName, libName );
        overwritingCurrentSymbol = editFrame->IsCurrentSymbol( overwrittenSymbol->GetLibId() );
        libMgr.RemoveSymbol( newName, libName );
    }

    libSymbol->SetName( newName );

    if( libSymbol->GetValueField().GetText() == oldName )
        libSymbol->GetValueField().SetText( newName );

    libMgr.UpdateSymbolAfterRename( libSymbol, newName, libName );
    libMgr.SetSymbolModified( newName, libName );

    if( overwritingCurrentSymbol )
    {
        // We overwrite the old current symbol with the renamed one, so show
        // the renamed one now
        editFrame->SetCurSymbol( new LIB_SYMBOL( *libSymbol ), false );
    }
    else if( isCurrentSymbol && editFrame->GetCurSymbol() )
    {
        // Renamed the current symbol - follow it
        libSymbol = editFrame->GetCurSymbol();

        libSymbol->SetName( newName );

        if( libSymbol->GetValueField().GetText() == oldName )
            libSymbol->GetValueField().SetText( newName );

        editFrame->RebuildView();
        editFrame->OnModify();
        editFrame->UpdateTitle();

        // N.B. The view needs to be rebuilt first as the Symbol Properties change may
        // invalidate the view pointers by rebuilting the field table
        editFrame->UpdateMsgPanel();
    }

    wxDataViewItem treeItem = libMgr.GetAdapter()->FindItem( libId );
    editFrame->UpdateLibraryTree( treeItem, libSymbol );
    editFrame->FocusOnLibId( LIB_ID( libName, newName ) );
    return 0;
}


int SYMBOL_EDITOR_CONTROL::ToggleProperties( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
        getEditFrame<SYMBOL_EDIT_FRAME>()->ToggleProperties();

    return 0;
}


int SYMBOL_EDITOR_CONTROL::ShowElectricalTypes( const TOOL_EVENT& aEvent )
{
    SCH_RENDER_SETTINGS* renderSettings = m_frame->GetRenderSettings();
    renderSettings->m_ShowPinsElectricalType = !renderSettings->m_ShowPinsElectricalType;

    // Update canvas
    m_frame->GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SYMBOL_EDITOR_CONTROL::ShowPinNumbers( const TOOL_EVENT& aEvent )
{
    SCH_RENDER_SETTINGS* renderSettings = m_frame->GetRenderSettings();
    renderSettings->m_ShowPinNumbers = !renderSettings->m_ShowPinNumbers;

    // Update canvas
    m_frame->GetCanvas()->GetView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int SYMBOL_EDITOR_CONTROL::ToggleSyncedPinsMode( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    editFrame->m_SyncPinEdit = !editFrame->m_SyncPinEdit;

    return 0;
}


int SYMBOL_EDITOR_CONTROL::ToggleHiddenPins( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDITOR_SETTINGS* cfg = m_frame->libeditconfig();
    cfg->m_ShowHiddenPins = !cfg->m_ShowHiddenPins;

    getEditFrame<SYMBOL_EDIT_FRAME>()->GetRenderSettings()->m_ShowHiddenPins =
            cfg->m_ShowHiddenPins;

    getView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();
    return 0;
}


int SYMBOL_EDITOR_CONTROL::ToggleHiddenFields( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDITOR_SETTINGS* cfg = m_frame->libeditconfig();
    cfg->m_ShowHiddenFields = !cfg->m_ShowHiddenFields;

    // TODO: Why is this needed in symbol edit and not in schematic edit?
    getEditFrame<SYMBOL_EDIT_FRAME>()->GetRenderSettings()->m_ShowHiddenFields =
            cfg->m_ShowHiddenFields;

    getView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();
    return 0;
}


int SYMBOL_EDITOR_CONTROL::TogglePinAltIcons( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDITOR_SETTINGS& cfg = *m_frame->libeditconfig();
    cfg.m_ShowPinAltIcons = !cfg.m_ShowPinAltIcons;

    m_frame->GetRenderSettings()->m_ShowPinAltIcons = cfg.m_ShowPinAltIcons;

    getView()->UpdateAllItems( KIGFX::REPAINT );
    m_frame->GetCanvas()->Refresh();
    return 0;
}


int SYMBOL_EDITOR_CONTROL::ExportView( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    LIB_SYMBOL*        symbol = editFrame->GetCurSymbol();

    if( !symbol )
    {
        wxMessageBox( _( "No symbol to export" ) );
        return 0;
    }

    wxFileName fn( symbol->GetName() );
    fn.SetExt( "png" );

    wxString projectPath = wxPathOnly( m_frame->Prj().GetProjectFullName() );

    wxFileDialog dlg( editFrame, _( "Export View as PNG" ), projectPath, fn.GetFullName(),
                      FILEEXT::PngFileWildcard(), wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_OK && !dlg.GetPath().IsEmpty() )
    {
        // calling wxYield is mandatory under Linux, after closing the file selector dialog
        // to refresh the screen before creating the PNG or JPEG image from screen
        wxYield();

        if( !editFrame->SaveCanvasImageToFile( dlg.GetPath(), BITMAP_TYPE::PNG ) )
        {
            wxMessageBox( wxString::Format( _( "Can't save file '%s'." ), dlg.GetPath() ) );
        }
    }

    return 0;
}


int SYMBOL_EDITOR_CONTROL::ExportSymbol( const TOOL_EVENT& aEvent )
{
    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
        static_cast<SYMBOL_EDIT_FRAME*>( m_frame )->ExportSymbol();

    return 0;
}


int SYMBOL_EDITOR_CONTROL::ExportSymbolAsSVG( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    LIB_SYMBOL*        symbol = editFrame->GetCurSymbol();

    if( !symbol )
    {
        wxMessageBox( _( "No symbol to export" ) );
        return 0;
    }

    wxFileName fn( symbol->GetName() );
    fn.SetExt( FILEEXT::SVGFileExtension );

    wxString pro_dir = wxPathOnly( m_frame->Prj().GetProjectFullName() );

    wxString fullFileName = wxFileSelector( _( "SVG File Name" ), pro_dir, fn.GetFullName(),
                                            FILEEXT::SVGFileExtension, FILEEXT::SVGFileWildcard(),
                                            wxFD_SAVE,
                                            m_frame );

    if( !fullFileName.IsEmpty() )
    {
        PAGE_INFO pageSave = editFrame->GetScreen()->GetPageSettings();
        PAGE_INFO pageTemp = pageSave;

        BOX2I symbolBBox = symbol->GetUnitBoundingBox( editFrame->GetUnit(),
                                                       editFrame->GetBodyStyle(), false );

        // Add a small margin (10% of size)to the plot bounding box
        symbolBBox.Inflate( symbolBBox.GetSize().x * 0.1, symbolBBox.GetSize().y * 0.1 );

        pageTemp.SetWidthMils( schIUScale.IUToMils( symbolBBox.GetSize().x ) );
        pageTemp.SetHeightMils( schIUScale.IUToMils( symbolBBox.GetSize().y ) );

        // Add an offet to plot the symbol centered on the page.
        VECTOR2I plot_offset = symbolBBox.GetOrigin();

        editFrame->GetScreen()->SetPageSettings( pageTemp );
        editFrame->SVGPlotSymbol( fullFileName, -plot_offset );
        editFrame->GetScreen()->SetPageSettings( pageSave );
    }

    return 0;
}


int SYMBOL_EDITOR_CONTROL::FlattenSymbol( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME*          editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    LIB_SYMBOL_LIBRARY_MANAGER& libMgr = editFrame->GetLibManager();
    LIB_ID                      symId = editFrame->GetTargetLibId();

    if( !symId.IsValid() )
    {
        wxMessageBox( _( "No symbol to flatten" ) );
        return 0;
    }

    const LIB_SYMBOL*           symbol = libMgr.GetBufferedSymbol( symId.GetLibItemName(), symId.GetLibNickname() );
    std::unique_ptr<LIB_SYMBOL> flatSymbol = symbol->Flatten();
    wxCHECK_MSG( flatSymbol, 0, _( "Failed to flatten symbol" ) );

    if( !libMgr.UpdateSymbol( flatSymbol.get(), symId.GetLibNickname() ) )
    {
        wxMessageBox( _( "Failed to update library with flattened symbol" ) );
        return 0;
    }

    wxDataViewItem treeItem = libMgr.GetAdapter()->FindItem( symId );
    editFrame->UpdateLibraryTree( treeItem, flatSymbol.get() );

    return 0;
}


int SYMBOL_EDITOR_CONTROL::AddSymbolToSchematic( const TOOL_EVENT& aEvent )
{
    LIB_SYMBOL* libSymbol = nullptr;
    LIB_ID      libId;
    int         unit, bodyStyle;

    if( m_isSymbolEditor )
    {
        SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();

        libSymbol = editFrame->GetCurSymbol();
        unit      = editFrame->GetUnit();
        bodyStyle = editFrame->GetBodyStyle();

        if( libSymbol )
            libId = libSymbol->GetLibId();
    }
    else
    {
        SYMBOL_VIEWER_FRAME* viewerFrame = getEditFrame<SYMBOL_VIEWER_FRAME>();

        libSymbol = viewerFrame->GetSelectedSymbol();
        unit      = viewerFrame->GetUnit();
        bodyStyle = viewerFrame->GetBodyStyle();

        if( libSymbol )
            libId = libSymbol->GetLibId();
    }

    if( libSymbol )
    {
        SCH_EDIT_FRAME* schframe = (SCH_EDIT_FRAME*) m_frame->Kiway().Player( FRAME_SCH, false );

        if( !schframe )      // happens when the schematic editor is not active (or closed)
        {
            DisplayErrorMessage( m_frame, _( "No schematic currently open." ) );
            return 0;
        }

        wxWindow* blocking_dialog = schframe->Kiway().GetBlockingDialog();

        if( blocking_dialog )
        {
            blocking_dialog->Raise();
            wxBell();
            return 0;
        }

        SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libId, &schframe->GetCurrentSheet(),
                                             unit, bodyStyle );

        symbol->SetParent( schframe->GetScreen() );

        if( schframe->eeconfig()->m_AutoplaceFields.enable )
        {
            // Not placed yet, so pass a nullptr screen reference
            symbol->AutoplaceFields( nullptr, AUTOPLACE_AUTO );
        }

        schframe->Raise();
        schframe->GetToolManager()->PostAction( SCH_ACTIONS::placeSymbol,
                                                SCH_ACTIONS::PLACE_SYMBOL_PARAMS{ symbol, true } );
    }

    return 0;
}


int SYMBOL_EDITOR_CONTROL::ChangeUnit( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    const int          deltaUnit = aEvent.Parameter<int>();

    const int nUnits = editFrame->GetCurSymbol()->GetUnitCount();
    const int newUnit = ( ( editFrame->GetUnit() - 1 + deltaUnit + nUnits ) % nUnits ) + 1;

    editFrame->SetUnit( newUnit );

    return 0;
}


int SYMBOL_EDITOR_CONTROL::ShowLibraryTable( const TOOL_EVENT& aEvent )
{
    DIALOG_LIB_FIELDS_TABLE::SCOPE scope = DIALOG_LIB_FIELDS_TABLE::SCOPE_LIBRARY;

    if( aEvent.IsAction( &SCH_ACTIONS::showRelatedLibFieldsTable ) )
        scope = DIALOG_LIB_FIELDS_TABLE::SCOPE_RELATED_SYMBOLS;

    DIALOG_LIB_FIELDS_TABLE dlg( getEditFrame<SYMBOL_EDIT_FRAME>(), scope );

    dlg.ShowModal();
    return 0;
}


void SYMBOL_EDITOR_CONTROL::setTransitions()
{
    Go( &SYMBOL_EDITOR_CONTROL::AddLibrary,            ACTIONS::newLibrary.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::AddLibrary,            ACTIONS::addLibrary.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::AddSymbol,             SCH_ACTIONS::newSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::AddSymbol,             SCH_ACTIONS::deriveFromExistingSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::AddSymbol,             SCH_ACTIONS::importSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::EditSymbol,            SCH_ACTIONS::editSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::EditLibrarySymbol,     SCH_ACTIONS::editLibSymbolWithLibEdit.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::DdAddLibrary,          ACTIONS::ddAddLibrary.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::Save,                  ACTIONS::save.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::Save,                  SCH_ACTIONS::saveLibraryAs.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::Save,                  SCH_ACTIONS::saveSymbolAs.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::Save,                  SCH_ACTIONS::saveSymbolCopyAs.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::Save,                  ACTIONS::saveAll.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::Revert,                ACTIONS::revert.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::DuplicateSymbol,       SCH_ACTIONS::duplicateSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::RenameSymbol,          SCH_ACTIONS::renameSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::CutCopyDelete,         SCH_ACTIONS::deleteSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::CutCopyDelete,         SCH_ACTIONS::cutSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::CutCopyDelete,         SCH_ACTIONS::copySymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::DuplicateSymbol,       SCH_ACTIONS::pasteSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ExportSymbol,          SCH_ACTIONS::exportSymbol.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::FlattenSymbol,         SCH_ACTIONS::flattenSymbol.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::OpenWithTextEditor,    ACTIONS::openWithTextEditor.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::OpenDirectory,         ACTIONS::openDirectory.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::ExportView,            SCH_ACTIONS::exportSymbolView.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ExportSymbolAsSVG,     SCH_ACTIONS::exportSymbolAsSVG.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::AddSymbolToSchematic,  SCH_ACTIONS::addSymbolToSchematic.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::ShowElectricalTypes,   SCH_ACTIONS::showElectricalTypes.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ShowPinNumbers,        SCH_ACTIONS::showPinNumbers.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ToggleSyncedPinsMode,  SCH_ACTIONS::toggleSyncedPinsMode.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::ToggleProperties,      ACTIONS::showProperties.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ToggleHiddenPins,      SCH_ACTIONS::showHiddenPins.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ToggleHiddenFields,    SCH_ACTIONS::showHiddenFields.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::TogglePinAltIcons,     SCH_ACTIONS::togglePinAltIcons.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::ShowLibraryTable,      SCH_ACTIONS::showLibFieldsTable.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ShowLibraryTable,      SCH_ACTIONS::showRelatedLibFieldsTable.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::ChangeUnit,            SCH_ACTIONS::previousUnit.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ChangeUnit,            SCH_ACTIONS::nextUnit.MakeEvent() );
}

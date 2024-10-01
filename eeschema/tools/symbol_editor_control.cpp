/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 CERN
 * Copyright (C) 2019-2024 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiway.h>
#include <pgm_base.h>
#include <sch_painter.h>
#include <tool/tool_manager.h>
#include <tool/library_editor_control.h>
#include <tools/ee_actions.h>
#include <tools/symbol_editor_control.h>
#include <lib_symbol_library_manager.h>
#include <symbol_viewer_frame.h>
#include <symbol_tree_model_adapter.h>
#include <wildcards_and_files_ext.h>
#include <bitmaps/bitmap_types.h>
#include <confirm.h>
#include <kidialog.h>
#include <launch_ext.h> // To default when file manager setting is empty
#include <gestfich.h> // To open with a text editor
#include <wx/filedlg.h>
#include "string_utils.h"

bool SYMBOL_EDITOR_CONTROL::Init()
{
    m_frame = getEditFrame<SCH_BASE_FRAME>();
    m_selectionTool = m_toolMgr->GetTool<EE_SELECTION_TOOL>();
    m_isSymbolEditor = m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR );

    if( m_isSymbolEditor )
    {
        LIBRARY_EDITOR_CONTROL* libraryTreeTool = m_toolMgr->GetTool<LIBRARY_EDITOR_CONTROL>();
        CONDITIONAL_MENU&       ctxMenu = m_menu->GetMenu();
        SYMBOL_EDIT_FRAME*      editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();

        wxCHECK( editFrame, false );

        auto libSelectedCondition =
                [ editFrame ]( const SELECTION& aSel )
                {
                    LIB_ID sel = editFrame->GetTreeLIBID();
                    return !sel.GetLibNickname().empty() && sel.GetLibItemName().empty();
                };

        // The libInferredCondition allows you to do things like New Symbol and Paste with a
        // symbol selected (in other words, when we know the library context even if the library
        // itself isn't selected.
        auto libInferredCondition =
                [ editFrame ]( const SELECTION& aSel )
                {
                    LIB_ID sel = editFrame->GetTreeLIBID();
                    return !sel.GetLibNickname().empty();
                };

        auto symbolSelectedCondition =
                [ editFrame ]( const SELECTION& aSel )
                {
                    LIB_ID sel = editFrame->GetTargetLibId();
                    return !sel.GetLibNickname().empty() && !sel.GetLibItemName().empty();
                };

/* not used, but used to be used
        auto multiSelectedCondition =
                [ editFrame ]( const SELECTION& aSel )
                {
                    return editFrame->GetTreeSelectionCount() > 1;
                };
*/
        auto multiSymbolSelectedCondition =
                [ editFrame ]( const SELECTION& aSel )
                {
                    if( editFrame->GetTreeSelectionCount() > 1 )
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
                [ editFrame ]( const SELECTION& aSel )
                {
                    if( editFrame->GetTreeSelectionCount() > 1 )
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
                [ editFrame ]( const SELECTION& aSel )
                {
                    // The option is shown if the lib has no current edits
                    LIB_SYMBOL_LIBRARY_MANAGER& libMgr = editFrame->GetLibManager();
                    wxString libName = editFrame->GetTargetLibId().GetLibNickname();
                    bool     ret = !libMgr.IsLibraryModified( libName );
                    return ret;
                };

// clang-format off
        ctxMenu.AddItem( EE_ACTIONS::newSymbol,           libInferredCondition, 10 );
        ctxMenu.AddItem( EE_ACTIONS::deriveFromExistingSymbol, symbolSelectedCondition, 10 );

        ctxMenu.AddSeparator( 10 );
        ctxMenu.AddItem( ACTIONS::save,                   symbolSelectedCondition || libInferredCondition, 10 );
        ctxMenu.AddItem( EE_ACTIONS::saveLibraryAs,       libSelectedCondition, 10 );
        ctxMenu.AddItem( EE_ACTIONS::saveSymbolCopyAs,    symbolSelectedCondition, 10 );
        ctxMenu.AddItem( ACTIONS::revert,                 symbolSelectedCondition || libInferredCondition, 10 );

        ctxMenu.AddSeparator( 10 );
        ctxMenu.AddItem( EE_ACTIONS::cutSymbol,           symbolSelectedCondition || multiSymbolSelectedCondition, 10 );
        ctxMenu.AddItem( EE_ACTIONS::copySymbol,          symbolSelectedCondition || multiSymbolSelectedCondition, 10 );
        ctxMenu.AddItem( EE_ACTIONS::pasteSymbol,         libInferredCondition, 10 );
        ctxMenu.AddItem( EE_ACTIONS::duplicateSymbol,     symbolSelectedCondition, 10 );
        ctxMenu.AddItem( EE_ACTIONS::renameSymbol,        symbolSelectedCondition, 10 );
        ctxMenu.AddItem( EE_ACTIONS::deleteSymbol,        symbolSelectedCondition || multiSymbolSelectedCondition, 10 );

        ctxMenu.AddSeparator( 100 );
        ctxMenu.AddItem( EE_ACTIONS::importSymbol,        libInferredCondition, 100 );

        if( ADVANCED_CFG::GetCfg().m_EnableLibWithText )
        {
            ctxMenu.AddSeparator( 200 );
            ctxMenu.AddItem( ACTIONS::openWithTextEditor, canOpenExternally && ( symbolSelectedCondition || libSelectedCondition ), 200 );
        }

        if( ADVANCED_CFG::GetCfg().m_EnableLibDir )
        {
            ctxMenu.AddSeparator( 200 );
            ctxMenu.AddItem( ACTIONS::openDirectory,      canOpenExternally && ( symbolSelectedCondition || libSelectedCondition ), 200 );
        }

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

    if( editFrame->GetLibManager().IsLibraryReadOnly( libName ) )
    {
        msg.Printf( _( "Symbol library '%s' is not writable." ), libName );
        m_frame->ShowInfoBarError( msg );
        return 0;
    }

    if( aEvent.IsAction( &EE_ACTIONS::newSymbol ) )
        editFrame->CreateNewSymbol();
    else if( aEvent.IsAction( &EE_ACTIONS::deriveFromExistingSymbol ) )
        editFrame->CreateNewSymbol( target.GetLibItemName() );
    else if( aEvent.IsAction( &EE_ACTIONS::importSymbol ) )
        editFrame->ImportSymbol();

    return 0;
}


int SYMBOL_EDITOR_CONTROL::Save( const TOOL_EVENT& aEvt )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();

    if( aEvt.IsAction( &EE_ACTIONS::save ) )
        editFrame->Save();
    else if( aEvt.IsAction( &EE_ACTIONS::saveLibraryAs ) )
        editFrame->SaveLibraryAs();
    else if( aEvt.IsAction( &EE_ACTIONS::saveSymbolCopyAs ) )
        editFrame->SaveSymbolCopyAs();
    else if( aEvt.IsAction( &EE_ACTIONS::saveAll ) )
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

    SYMBOL_EDIT_FRAME*          editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    LIB_SYMBOL_LIBRARY_MANAGER& libMgr = editFrame->GetLibManager();

    LIB_ID libId = editFrame->GetTreeLIBID();

    wxString libName = libId.GetLibNickname();
    wxString libItemName = libMgr.GetLibrary( libName )->GetFullURI( true );

    wxFileName fileName( libItemName );

    wxString filePath = wxEmptyString;

    COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();

    wxString explCommand = cfg->m_System.file_explorer;

    if( explCommand.IsEmpty() )
    {
        filePath = fileName.GetFullPath().BeforeLast( wxFileName::GetPathSeparator() );

        if( !filePath.IsEmpty() && wxDirExists( filePath ) )
            LaunchExternal( filePath );
        return 0;
    }

    if( !explCommand.EndsWith( "%F" ) )
    {
        wxMessageBox( _( "Missing/malformed file explorer argument '%F' in common settings." ) );
        return 0;
    }

    filePath = fileName.GetFullPath();
    filePath.Replace( wxS( "\"" ), wxS( "_" ) );

    wxString fileArg = '"' + filePath + '"';

    explCommand.Replace( wxT( "%F" ), fileArg );

    if( !explCommand.IsEmpty() )
        wxExecute( explCommand );

    return 0;
}


int SYMBOL_EDITOR_CONTROL::OpenWithTextEditor( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME*          editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    LIB_SYMBOL_LIBRARY_MANAGER& libMgr = editFrame->GetLibManager();
    wxString                    textEditorName = Pgm().GetTextEditor();

    if( textEditorName.IsEmpty() )
    {
        wxMessageBox( _( "No text editor selected in KiCad. Please choose one." ) );
        return 0;
    }

    LIB_ID   libId = editFrame->GetTreeLIBID();
    wxString libName = libId.GetLibNickname();
    wxString tempFName = libMgr.GetLibrary( libName )->GetFullURI( true ).wc_str();

    if( !tempFName.IsEmpty() )
        ExecuteFile( textEditorName, tempFName, nullptr, false );

    return 0;
}


int SYMBOL_EDITOR_CONTROL::CutCopyDelete( const TOOL_EVENT& aEvt )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();

    if( aEvt.IsAction( &EE_ACTIONS::cutSymbol ) || aEvt.IsAction( &EE_ACTIONS::copySymbol ) )
        editFrame->CopySymbolToClipboard();

    if( aEvt.IsAction( &EE_ACTIONS::cutSymbol ) || aEvt.IsAction( &EE_ACTIONS::deleteSymbol ) )
    {
        bool hasWritableLibs = false;
        wxString msg;

        for( LIB_ID& sel : editFrame->GetSelectedLibIds() )
        {
            const wxString& libName = sel.GetLibNickname();

            if( editFrame->GetLibManager().IsLibraryReadOnly( libName ) )
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
    bool               isPasteAction = aEvent.IsAction( &EE_ACTIONS::pasteSymbol );
    wxString           msg;

    if( !sel.IsValid() && !isPasteAction )
    {
        // When duplicating a symbol, a source symbol must exists.
        msg.Printf( _( "No symbol selected" ) );
        m_frame->ShowInfoBarError( msg );
        return 0;
    }

    const wxString& libName = sel.GetLibNickname();

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

                if( libMgr.SymbolExists( newName, libName ) )
                {
                    msg = wxString::Format( _( "Symbol '%s' already exists in library '%s'." ),
                                            newName, libName );

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

    LIB_SYMBOL* libSymbol = libMgr.GetBufferedSymbol( oldName, libName );
    bool        isCurrentSymbol = editFrame->IsCurrentSymbol( libId );

    if( !libSymbol )
        return 0;

    libSymbol->SetName( newName );

    if( libSymbol->GetFieldById( VALUE_FIELD )->GetText() == oldName )
        libSymbol->GetFieldById( VALUE_FIELD )->SetText( newName );

    libMgr.UpdateSymbolAfterRename( libSymbol, newName, libName );
    libMgr.SetSymbolModified( newName, libName );

    if( isCurrentSymbol && editFrame->GetCurSymbol())
    {
        libSymbol = editFrame->GetCurSymbol();

        libSymbol->SetName( newName );

        if( libSymbol->GetFieldById( VALUE_FIELD )->GetText() == oldName )
            libSymbol->GetFieldById( VALUE_FIELD )->SetText( newName );

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


int SYMBOL_EDITOR_CONTROL::OnDeMorgan( const TOOL_EVENT& aEvent )
{
    int bodyStyle = aEvent.IsAction( &EE_ACTIONS::showDeMorganStandard ) ? BODY_STYLE::BASE
                                                                         : BODY_STYLE::DEMORGAN;

    if( m_frame->IsType( FRAME_SCH_SYMBOL_EDITOR ) )
    {
        m_toolMgr->RunAction( ACTIONS::cancelInteractive );
        m_toolMgr->RunAction( EE_ACTIONS::clearSelection );

        SYMBOL_EDIT_FRAME* symbolEditor = static_cast<SYMBOL_EDIT_FRAME*>( m_frame );
        symbolEditor->SetBodyStyle( bodyStyle );

        m_toolMgr->ResetTools( TOOL_BASE::MODEL_RELOAD );
        symbolEditor->RebuildView();
    }
    else if( m_frame->IsType( FRAME_SCH_VIEWER ) )
    {
        SYMBOL_VIEWER_FRAME* symbolViewer = static_cast<SYMBOL_VIEWER_FRAME*>( m_frame );
        symbolViewer->SetUnitAndBodyStyle( symbolViewer->GetUnit(), bodyStyle );
    }

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

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    editFrame->GetRenderSettings()->m_ShowHiddenPins =
                    !editFrame->GetRenderSettings()->m_ShowHiddenPins;

    getView()->UpdateAllItems( KIGFX::REPAINT );
    editFrame->GetCanvas()->Refresh();
    return 0;
}


int SYMBOL_EDITOR_CONTROL::ToggleHiddenFields( const TOOL_EVENT& aEvent )
{
    if( !m_isSymbolEditor )
        return 0;

    SYMBOL_EDIT_FRAME* editFrame = getEditFrame<SYMBOL_EDIT_FRAME>();
    editFrame->GetRenderSettings()->m_ShowHiddenFields =
                    !editFrame->GetRenderSettings()->m_ShowHiddenFields;

    getView()->UpdateAllItems( KIGFX::REPAINT );
    editFrame->GetCanvas()->Refresh();
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

        wxCHECK( libSymbol->GetLibId().IsValid(), 0 );

        SCH_SYMBOL* symbol = new SCH_SYMBOL( *libSymbol, libId, &schframe->GetCurrentSheet(),
                                             unit, bodyStyle );

        symbol->SetParent( schframe->GetScreen() );

        if( schframe->eeconfig()->m_AutoplaceFields.enable )
            symbol->AutoplaceFields( /* aScreen */ nullptr, /* aManual */ false );

        schframe->Raise();
        schframe->GetToolManager()->PostAction( EE_ACTIONS::placeSymbol, symbol );
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


void SYMBOL_EDITOR_CONTROL::setTransitions()
{
    // clang-format off
    Go( &SYMBOL_EDITOR_CONTROL::AddLibrary,            ACTIONS::newLibrary.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::AddLibrary,            ACTIONS::addLibrary.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::AddSymbol,             EE_ACTIONS::newSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::AddSymbol,             EE_ACTIONS::deriveFromExistingSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::AddSymbol,             EE_ACTIONS::importSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::EditSymbol,            EE_ACTIONS::editSymbol.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::DdAddLibrary,          ACTIONS::ddAddLibrary.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::Save,                  ACTIONS::save.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::Save,                  EE_ACTIONS::saveLibraryAs.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::Save,                  EE_ACTIONS::saveSymbolCopyAs.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::Save,                  ACTIONS::saveAll.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::Revert,                ACTIONS::revert.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::DuplicateSymbol,       EE_ACTIONS::duplicateSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::RenameSymbol,          EE_ACTIONS::renameSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::CutCopyDelete,         EE_ACTIONS::deleteSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::CutCopyDelete,         EE_ACTIONS::cutSymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::CutCopyDelete,         EE_ACTIONS::copySymbol.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::DuplicateSymbol,       EE_ACTIONS::pasteSymbol.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::OpenWithTextEditor,    ACTIONS::openWithTextEditor.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::OpenDirectory,         ACTIONS::openDirectory.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::ExportView,            EE_ACTIONS::exportSymbolView.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ExportSymbolAsSVG,     EE_ACTIONS::exportSymbolAsSVG.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::AddSymbolToSchematic,  EE_ACTIONS::addSymbolToSchematic.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::OnDeMorgan,            EE_ACTIONS::showDeMorganStandard.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::OnDeMorgan,            EE_ACTIONS::showDeMorganAlternate.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::ShowElectricalTypes,   EE_ACTIONS::showElectricalTypes.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ShowPinNumbers,        EE_ACTIONS::showPinNumbers.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ToggleSyncedPinsMode,  EE_ACTIONS::toggleSyncedPinsMode.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::ToggleProperties,      ACTIONS::showProperties.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ToggleHiddenPins,      EE_ACTIONS::showHiddenPins.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ToggleHiddenFields,    EE_ACTIONS::showHiddenFields.MakeEvent() );

    Go( &SYMBOL_EDITOR_CONTROL::ChangeUnit,            EE_ACTIONS::previousUnit.MakeEvent() );
    Go( &SYMBOL_EDITOR_CONTROL::ChangeUnit,            EE_ACTIONS::nextUnit.MakeEvent() );
    // clang-format on
}

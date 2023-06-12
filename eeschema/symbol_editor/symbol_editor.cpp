/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pgm_base.h>
#include <confirm.h>
#include <kiway.h>
#include <widgets/wx_infobar.h>
#include <tools/ee_actions.h>
#include <tools/symbol_editor_drawing_tools.h>
#include <symbol_edit_frame.h>
#include <symbol_library.h>
#include <template_fieldnames.h>
#include <wildcards_and_files_ext.h>
#include <symbol_lib_table.h>
#include <lib_symbol_library_manager.h>
#include <symbol_tree_pane.h>
#include <project/project_file.h>
#include <widgets/lib_tree.h>
#include <sch_plugins/legacy/sch_legacy_plugin.h>
#include <sch_plugins/kicad/sch_sexpr_plugin.h>
#include <dialogs/dialog_lib_new_symbol.h>
#include <eda_list_dialog.h>
#include <wx/clipbrd.h>
#include <wx/filedlg.h>
#include <wx/log.h>
#include <string_utils.h>
#include "symbol_saveas_type.h"

#if wxCHECK_VERSION( 3, 1, 7 )
#include <widgets/symbol_filedlg_save_as.h>
#else
#include <widgets/symbol_legacyfiledlg_save_as.h>
SYMBOL_SAVEAS_TYPE SYMBOL_LEGACYFILEDLG_SAVE_AS::m_option = SYMBOL_SAVEAS_TYPE::NORMAL_SAVE_AS;
#endif


void SYMBOL_EDIT_FRAME::updateTitle()
{
    wxString title;

    if( GetCurSymbol() && IsSymbolFromSchematic() )
    {
        if( GetScreen() && GetScreen()->IsContentModified() )
            title = wxT( "*" );

        title += m_reference;
        title += wxS( " " ) + _( "[from schematic]" );
    }
    else if( GetCurSymbol() )
    {
        if( GetScreen() && GetScreen()->IsContentModified() )
            title = wxT( "*" );

        title += UnescapeString( GetCurSymbol()->GetLibId().Format() );

        if( m_libMgr && m_libMgr->IsLibraryReadOnly( GetCurLib() ) )
            title += wxS( " " ) + _( "[Read Only Library]" );
    }
    else
    {
        title = _( "[no symbol loaded]" );
    }

    title += wxT( " \u2014 " ) + _( "Symbol Editor" );
    SetTitle( title );
}


void SYMBOL_EDIT_FRAME::SelectActiveLibrary( const wxString& aLibrary )
{
    wxString selectedLib = aLibrary;

    if( selectedLib.empty() )
        selectedLib = SelectLibraryFromList();

    if( !selectedLib.empty() )
        SetCurLib( selectedLib );

    updateTitle();
}


bool SYMBOL_EDIT_FRAME::saveCurrentSymbol()
{
    if( GetCurSymbol() )
    {
        if( IsSymbolFromSchematic() )
        {
            SCH_EDIT_FRAME* schframe = (SCH_EDIT_FRAME*) Kiway().Player( FRAME_SCH, false );

            if( !schframe )      // happens when the schematic editor has been closed
            {
                DisplayErrorMessage( this, _( "No schematic currently open." ) );
                return false;
            }
            else
            {
                schframe->SaveSymbolToSchematic( *m_symbol, m_schematicSymbolUUID );
                GetScreen()->SetContentModified( false );
                return true;
            }
        }
        else
        {
            LIB_ID libId = GetCurSymbol()->GetLibId();
            const wxString& libName = libId.GetLibNickname();
            const wxString& symbolName = libId.GetLibItemName();

            if( m_libMgr->FlushSymbol( symbolName, libName ) )
            {
                m_libMgr->ClearSymbolModified( symbolName, libName );
                return true;
            }
        }
    }

    return false;
}


bool SYMBOL_EDIT_FRAME::LoadSymbol( const LIB_ID& aLibId, int aUnit, int aConvert )
{
    LIB_ID libId = aLibId;

    // Some libraries can't be edited, so load the underlying chosen symbol
    if( SYMBOL_LIB_TABLE_ROW* lib = m_libMgr->GetLibrary( aLibId.GetLibNickname() ) )
    {
        if( lib->SchLibType() == SCH_IO_MGR::SCH_DATABASE
            || lib->SchLibType() == SCH_IO_MGR::SCH_CADSTAR_ARCHIVE )
        {
            try
            {
                LIB_SYMBOL* readOnlySym = Prj().SchSymbolLibTable()->LoadSymbol( aLibId );

                if( readOnlySym && readOnlySym->GetSourceLibId().IsValid() )
                    libId = readOnlySym->GetSourceLibId();
            }
            catch( const IO_ERROR& ioe )
            {
                wxString msg;

                msg.Printf( _( "Error loading symbol %s from library '%s'." ),
                            aLibId.GetUniStringLibId(), aLibId.GetUniStringLibItemName() );
                DisplayErrorMessage( this, msg, ioe.What() );
                return false;
            }
        }
    }

    if( GetCurSymbol() && !IsSymbolFromSchematic()
            && GetCurSymbol()->GetLibId() == libId
            && GetUnit() == aUnit
            && GetConvert() == aConvert )
    {
        return true;
    }

    if( GetCurSymbol() && IsSymbolFromSchematic() && GetScreen()->IsContentModified() )
    {
        if( !HandleUnsavedChanges( this, _( "The current symbol has been modified.  Save changes?" ),
                                   [&]() -> bool
                                   {
                                       return saveCurrentSymbol();
                                   } ) )
        {
            return false;
        }
    }

    SelectActiveLibrary( libId.GetLibNickname() );

    if( LoadSymbolFromCurrentLib( libId.GetLibItemName(), aUnit, aConvert ) )
    {
        m_treePane->GetLibTree()->SelectLibId( libId );
        m_treePane->GetLibTree()->ExpandLibId( libId );

        m_centerItemOnIdle = libId;
        Bind( wxEVT_IDLE, &SYMBOL_EDIT_FRAME::centerItemIdleHandler, this );
        setSymWatcher( &libId );

        return true;
    }

    return false;
}


void SYMBOL_EDIT_FRAME::centerItemIdleHandler( wxIdleEvent& aEvent )
{
    m_treePane->GetLibTree()->CenterLibId( m_centerItemOnIdle );
    Unbind( wxEVT_IDLE, &SYMBOL_EDIT_FRAME::centerItemIdleHandler, this );
}


bool SYMBOL_EDIT_FRAME::LoadSymbolFromCurrentLib( const wxString& aAliasName, int aUnit,
                                                  int aConvert )
{
    LIB_SYMBOL* alias = nullptr;

    try
    {
        alias = Prj().SchSymbolLibTable()->LoadSymbol( GetCurLib(), aAliasName );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg;

        msg.Printf( _( "Error loading symbol %s from library '%s'." ),
                    aAliasName,
                    GetCurLib() );
        DisplayErrorMessage( this, msg, ioe.What() );
        return false;
    }

    if( !alias || !LoadOneLibrarySymbolAux( alias, GetCurLib(), aUnit, aConvert ) )
        return false;

    // Enable synchronized pin edit mode for symbols with interchangeable units
    m_SyncPinEdit = GetCurSymbol()->IsMulti() && !GetCurSymbol()->UnitsLocked();

    ClearUndoRedoList();
    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
    SetShowDeMorgan( GetCurSymbol()->Flatten()->HasConversion() );

    if( aUnit > 0 )
        RebuildSymbolUnitsList();

    return true;
}


bool SYMBOL_EDIT_FRAME::LoadOneLibrarySymbolAux( LIB_SYMBOL* aEntry, const wxString& aLibrary,
                                                 int aUnit, int aConvert )
{
    bool rebuildMenuAndToolbar = false;

    if( !aEntry || aLibrary.empty() )
        return false;

    if( aEntry->GetName().IsEmpty() )
    {
        wxLogWarning( "Symbol in library '%s' has empty name field.", aLibrary );
        return false;
    }

    m_toolManager->RunAction( ACTIONS::cancelInteractive, true );

    // Symbols from the schematic are edited in place and not managed by the library manager.
    if( IsSymbolFromSchematic() )
    {
        delete m_symbol;
        m_symbol = nullptr;

        SCH_SCREEN* screen = GetScreen();
        delete screen;
        SetScreen( m_dummyScreen );
        m_isSymbolFromSchematic = false;
        rebuildMenuAndToolbar = true;
    }

    LIB_SYMBOL* lib_symbol = m_libMgr->GetBufferedSymbol( aEntry->GetName(), aLibrary );
    wxCHECK( lib_symbol, false );

    m_unit = aUnit > 0 ? aUnit : 1;
    m_convert = aConvert > 0 ? aConvert : 1;

    // The buffered screen for the symbol
    SCH_SCREEN* symbol_screen = m_libMgr->GetScreen( lib_symbol->GetName(), aLibrary );

    SetScreen( symbol_screen );
    SetCurSymbol( new LIB_SYMBOL( *lib_symbol ), true );
    SetCurLib( aLibrary );

    if( rebuildMenuAndToolbar )
    {
        ReCreateMenuBar();
        ReCreateHToolbar();
        GetInfoBar()->Dismiss();
    }

    updateTitle();
    RebuildSymbolUnitsList();
    SetShowDeMorgan( GetCurSymbol()->HasConversion() );

    ClearUndoRedoList();

    if( !IsSymbolFromSchematic() )
    {
        LIB_ID libId = GetCurSymbol()->GetLibId();
        setSymWatcher( &libId );
    }

    // Display the document information based on the entry selected just in
    // case the entry is an alias.
    UpdateMsgPanel();
    Refresh();

    return true;
}


void SYMBOL_EDIT_FRAME::SaveAll()
{
    saveAllLibraries( false );
    m_treePane->GetLibTree()->RefreshLibTree();
}


void SYMBOL_EDIT_FRAME::CreateNewSymbol( const wxString& inheritFromSymbolName )
{
    m_toolManager->RunAction( ACTIONS::cancelInteractive, true );

    wxArrayString rootSymbols;
    wxString lib = getTargetLib();

    if( !m_libMgr->LibraryExists( lib ) )
    {
        lib = SelectLibraryFromList();

        if( !m_libMgr->LibraryExists( lib ) )
            return;
    }

    m_libMgr->GetRootSymbolNames( lib, rootSymbols );

    rootSymbols.Sort();

    wxString _inheritSymbolName;
    wxString _infoMessage;

    // if the symbol being inherited from isn't a root symbol, find its root symbol
    // and use that symbol instead
    if( !inheritFromSymbolName.IsEmpty() )
    {
        LIB_SYMBOL* inheritFromSymbol = m_libMgr->GetBufferedSymbol( inheritFromSymbolName, lib );

        if( inheritFromSymbol && !inheritFromSymbol->IsRoot() )
        {
            std::shared_ptr<LIB_SYMBOL> parent = inheritFromSymbol->GetParent().lock();
            wxString rootSymbolName = parent->GetName();
            _inheritSymbolName = rootSymbolName;
            _infoMessage = wxString::Format( _( "Deriving from '%s', the root symbol of '%s'." ),
                                            _inheritSymbolName,
                                            inheritFromSymbolName);
        }
        else
        {
            _inheritSymbolName = inheritFromSymbolName;
        }
    }

    DIALOG_LIB_NEW_SYMBOL dlg( this, _infoMessage, &rootSymbols, _inheritSymbolName );
    dlg.SetMinSize( dlg.GetSize() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    if( dlg.GetName().IsEmpty() )
    {
        wxMessageBox( _( "This new symbol has no name and cannot be created." ) );
        return;
    }

    wxString name = dlg.GetName();

    // Currently, symbol names cannot include a space, that breaks libraries:
    name.Replace( " ", "_" );

    // Test if there is a symbol with this name already.
    if( !lib.empty() && m_libMgr->SymbolExists( name, lib ) )
    {
        wxString msg = wxString::Format( _( "Symbol '%s' already exists in library '%s'." ),
                                         name,
                                         lib );

        KIDIALOG errorDlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
        errorDlg.SetOKLabel( _( "Overwrite" ) );
        errorDlg.DoNotShowCheckbox( __FILE__, __LINE__ );

        if( errorDlg.ShowModal() == wxID_CANCEL )
            return;
    }

    LIB_SYMBOL new_symbol( name );  // do not create symbol on the heap, it will be buffered soon

    wxString parentSymbolName = dlg.GetParentSymbolName();

    if( parentSymbolName.IsEmpty() )
    {
        new_symbol.GetReferenceField().SetText( dlg.GetReference() );
        new_symbol.SetUnitCount( dlg.GetUnitCount() );

        // Initialize new_symbol.m_TextInside member:
        // if 0, pin text is outside the body (on the pin)
        // if > 0, pin text is inside the body
        if( dlg.GetPinNameInside() )
        {
            new_symbol.SetPinNameOffset( dlg.GetPinTextPosition() );

            if( new_symbol.GetPinNameOffset() == 0 )
                new_symbol.SetPinNameOffset( 1 );
        }
        else
        {
            new_symbol.SetPinNameOffset( 0 );
        }

        ( dlg.GetPowerSymbol() ) ? new_symbol.SetPower() : new_symbol.SetNormal();
        new_symbol.SetShowPinNumbers( dlg.GetShowPinNumber() );
        new_symbol.SetShowPinNames( dlg.GetShowPinName() );
        new_symbol.LockUnits( !dlg.GetUnitsInterchangeable() );
        new_symbol.SetIncludeInBom( dlg.GetIncludeInBom() );
        new_symbol.SetIncludeOnBoard( dlg.GetIncludeOnBoard() );

        if( dlg.GetUnitCount() < 2 )
            new_symbol.LockUnits( false );

        new_symbol.SetConversion( dlg.GetAlternateBodyStyle() );
    }
    else
    {
        LIB_SYMBOL* parent = m_libMgr->GetAlias( parentSymbolName, lib );
        wxCHECK( parent, /* void */ );
        new_symbol.SetParent( parent );

        // Inherit the parent mandatory field attributes.
        for( int id = 0; id < MANDATORY_FIELDS; ++id )
        {
            LIB_FIELD* field = new_symbol.GetFieldById( id );

            // the MANDATORY_FIELDS are exactly that in RAM.
            wxCHECK( field, /* void */ );

            LIB_FIELD* parentField = parent->GetFieldById( id );

            wxCHECK( parentField, /* void */ );

            *field = *parentField;

            switch( id )
            {
            case REFERENCE_FIELD:
                // parent's reference already copied
                break;

            case VALUE_FIELD:
                if( parent->IsPower() )
                    field->SetText( name );
                break;

            case FOOTPRINT_FIELD:
            case DATASHEET_FIELD:
                // - footprint might be the same as parent, but might not
                // - datasheet is most likely different
                // - probably best to play it safe and copy neither
                field->SetText( wxEmptyString );
                break;
            }

            field->SetParent( &new_symbol );
        }
    }

    m_libMgr->UpdateSymbol( &new_symbol, lib );
    SyncLibraries( false );
    LoadSymbol( name, lib, 1 );

    // must be called after loadSymbol, that calls SetShowDeMorgan, but
    // because the symbol is empty,it looks like it has no alternate body
    // and a derived symbol inherits its parent body.
    if( !new_symbol.GetParent().lock() )
        SetShowDeMorgan( dlg.GetAlternateBodyStyle() );
    else
        SetShowDeMorgan( new_symbol.HasConversion() );
}


void SYMBOL_EDIT_FRAME::Save()
{
    wxString libName;

    if( IsSymbolTreeShown() )
        libName = GetTreeLIBID().GetLibNickname();

    if( libName.empty() )
    {
        saveCurrentSymbol();
    }
    else if( m_libMgr->IsLibraryReadOnly( libName ) )
    {
        wxString msg = wxString::Format( _( "Symbol library '%s' is not writable." ),
                                         libName );
        wxString msg2 = _( "You must save to a different location." );

        if( OKOrCancelDialog( this, _( "Warning" ), msg, msg2 ) == wxID_OK )
            saveLibrary( libName, true );
    }
    else
    {
        saveLibrary( libName, false );
    }

    if( IsSymbolTreeShown() )
        m_treePane->GetLibTree()->RefreshLibTree();

    updateTitle();
}


void SYMBOL_EDIT_FRAME::SaveLibraryAs()
{
    wxCHECK( !GetTargetLibId().GetLibNickname().empty(), /* void */ );

    const wxString& libName = GetTargetLibId().GetLibNickname();

    saveLibrary( libName, true );
    m_treePane->GetLibTree()->RefreshLibTree();
}


void SYMBOL_EDIT_FRAME::SaveSymbolAs()
{
    saveSymbolAs();

    m_treePane->GetLibTree()->RefreshLibTree();
}


static int ID_SAVE_AS_NAME     = 4172;
static int ID_MAKE_NEW_LIBRARY = 4173;


EDA_LIST_DIALOG* SYMBOL_EDIT_FRAME::buildSaveAsDialog( const wxString& aSymbolName,
                                                       const wxString& aLibraryPreselect )
{
    COMMON_SETTINGS*           cfg = Pgm().GetCommonSettings();
    PROJECT_FILE&              project = Kiway().Prj().GetProjectFile();
    SYMBOL_LIB_TABLE*          tbl = Prj().SchSymbolLibTable();
    std::vector<wxString>      libNicknames = tbl->GetLogicalLibs();
    wxArrayString              headers;
    std::vector<wxArrayString> itemsToDisplay;

    headers.Add( _( "Nickname" ) );
    headers.Add( _( "Description" ) );

    for( const wxString& nickname : libNicknames )
    {
        if( alg::contains( project.m_PinnedSymbolLibs, nickname )
                || alg::contains( cfg->m_Session.pinned_symbol_libs, nickname ) )
        {
            wxArrayString item;
            item.Add( LIB_TREE_MODEL_ADAPTER::GetPinningSymbol() + nickname );
            item.Add( tbl->GetDescription( nickname ) );
            itemsToDisplay.push_back( item );
        }
    }

    for( const wxString& nickname : libNicknames )
    {
        if( !alg::contains( project.m_PinnedSymbolLibs, nickname )
                && !alg::contains( cfg->m_Session.pinned_symbol_libs, nickname ) )
        {
            wxArrayString item;
            item.Add( nickname );
            item.Add( tbl->GetDescription( nickname ) );
            itemsToDisplay.push_back( item );
        }
    }

    EDA_LIST_DIALOG* dlg = new EDA_LIST_DIALOG( this, _( "Save Symbol As" ), headers,
                                                itemsToDisplay, aLibraryPreselect, false );

    dlg->SetListLabel( _( "Save in library:" ) );
    dlg->SetOKLabel( _( "Save" ) );

    wxBoxSizer* bNameSizer = new wxBoxSizer( wxHORIZONTAL );

    wxStaticText* label = new wxStaticText( dlg, wxID_ANY, _( "Name:" ) );
    bNameSizer->Add( label, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

    wxTextCtrl* nameTextCtrl = new wxTextCtrl( dlg, ID_SAVE_AS_NAME, aSymbolName );
    bNameSizer->Add( nameTextCtrl, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

    wxButton* newLibraryButton = new wxButton( dlg, ID_MAKE_NEW_LIBRARY, _( "New Library..." ) );
    dlg->m_ButtonsSizer->Prepend( 80, 20 );
    dlg->m_ButtonsSizer->Prepend( newLibraryButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 10 );

    dlg->GetSizer()->Prepend( bNameSizer, 0, wxEXPAND|wxTOP|wxLEFT|wxRIGHT, 5 );

    dlg->Bind( wxEVT_BUTTON,
            [dlg]( wxCommandEvent& )
            {
                dlg->EndModal( ID_MAKE_NEW_LIBRARY );
            }, ID_MAKE_NEW_LIBRARY );

    // Move nameTextCtrl to the head of the tab-order
    if( dlg->GetChildren().DeleteObject( nameTextCtrl ) )
        dlg->GetChildren().Insert( nameTextCtrl );

    dlg->SetInitialFocus( nameTextCtrl );

    dlg->Layout();
    dlg->GetSizer()->Fit( dlg );

    return dlg;
}


void SYMBOL_EDIT_FRAME::saveSymbolAs()
{
    LIB_SYMBOL* symbol = getTargetSymbol();

    if( symbol )
    {
        LIB_ID   old_lib_id = symbol->GetLibId();
        wxString symbolName = old_lib_id.GetLibItemName();
        wxString libraryName = old_lib_id.GetLibNickname();
        bool     done = false;

        std::unique_ptr<EDA_LIST_DIALOG> dlg;

        while( !done )
        {
            dlg.reset( buildSaveAsDialog( symbolName, libraryName ) );

            int ret = dlg->ShowModal();

            if( ret == wxID_CANCEL )
            {
                return;
            }
            else if( ret == wxID_OK )
            {
                done = true;
            }
            else if( ret == ID_MAKE_NEW_LIBRARY )
            {
                wxFileName newLibrary( AddLibraryFile( true ) );
                libraryName = newLibrary.GetName();
            }
        }

        libraryName = dlg->GetTextSelection();

        if( libraryName.IsEmpty() )
        {
            DisplayError( this, _( "No library specified.  Symbol could not be saved." ) );
            return;
        }

        // @todo Either check the selecteced library to see if the parent symbol name is in
        //       the new library and/or copy the parent symbol as well.  This is the lazy
        //       solution to ensure derived symbols do not get orphaned.
        if( symbol->IsAlias() && libraryName != old_lib_id.GetLibNickname() )
        {
            DisplayError( this, _( "Derived symbols must be saved in the same library as their "
                                   "parent symbol." ) );
            return;
        }

        symbolName = static_cast<wxTextCtrl*>( dlg->FindWindow( ID_SAVE_AS_NAME ) )->GetValue();
        symbolName.Trim( true );
        symbolName.Trim( false );
        symbolName.Replace( " ", "_" );

        if( symbolName.IsEmpty() )
        {
            // This is effectively a cancel.  No need to nag the user about it.
            return;
        }

        // Test if there is a symbol with this name already.
        if( m_libMgr->SymbolExists( symbolName, libraryName ) )
        {
            wxString msg = wxString::Format( _( "Symbol '%s' already exists in library '%s'" ),
                                             symbolName,
                                             libraryName );

            KIDIALOG errorDlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
            errorDlg.SetOKLabel( _( "Overwrite" ) );
            errorDlg.DoNotShowCheckbox( __FILE__, __LINE__ );

            if( errorDlg.ShowModal() == wxID_CANCEL )
                return;
        }

        LIB_SYMBOL new_symbol( *symbol );
        new_symbol.SetName( symbolName );

        m_libMgr->UpdateSymbol( &new_symbol, libraryName );
        SyncLibraries( false );
        m_treePane->GetLibTree()->SelectLibId( LIB_ID( libraryName, new_symbol.GetName() ) );
        LoadSymbol( symbolName, libraryName, m_unit );
    }
}


void SYMBOL_EDIT_FRAME::UpdateAfterSymbolProperties( wxString* aOldName )
{
    wxCHECK( m_symbol, /* void */ );

    wxString lib = GetCurLib();

    if( !lib.IsEmpty() && aOldName && *aOldName != m_symbol->GetName() )
    {
        // Test the current library for name conflicts
        if( m_libMgr->SymbolExists( m_symbol->GetName(), lib ) )
        {
            wxString msg = wxString::Format( _( "Symbol name '%s' already in use." ),
                                             UnescapeString( m_symbol->GetName() ) );

            DisplayErrorMessage( this, msg );
            m_symbol->SetName( *aOldName );
        }
        else
        {
            m_libMgr->UpdateSymbolAfterRename( m_symbol, *aOldName, lib );
        }

        // Reselect the renamed symbol
        m_treePane->GetLibTree()->SelectLibId( LIB_ID( lib, m_symbol->GetName() ) );
    }

    RebuildSymbolUnitsList();
    SetShowDeMorgan( GetCurSymbol()->Flatten()->HasConversion() );
    updateTitle();

    // N.B. The view needs to be rebuilt first as the Symbol Properties change may invalidate
    // the view pointers by rebuilting the field table
    RebuildView();
    UpdateMsgPanel();

    OnModify();
}


void SYMBOL_EDIT_FRAME::DeleteSymbolFromLibrary()
{
    std::vector<LIB_ID> toDelete = GetSelectedLibIds();

    if( toDelete.empty() )
        toDelete.emplace_back( GetTargetLibId() );

    for( LIB_ID& libId : toDelete )
    {
        if( m_libMgr->IsSymbolModified( libId.GetLibItemName(), libId.GetLibNickname() )
            && !IsOK( this, wxString::Format( _( "The symbol '%s' has been modified.\n"
                                                 "Do you want to remove it from the library?" ),
                                              libId.GetUniStringLibItemName() ) ) )
        {
            continue;
        }

        if( m_libMgr->HasDerivedSymbols( libId.GetLibItemName(), libId.GetLibNickname() ) )
        {
            wxString msg;

            msg.Printf(
                    _( "The symbol %s is used to derive other symbols.\n"
                       "Deleting this symbol will delete all of the symbols derived from it.\n\n"
                       "Do you wish to delete this symbol and all of its derivatives?" ),
                    libId.GetLibItemName().wx_str() );

            wxMessageDialog::ButtonLabel yesButtonLabel( _( "Delete Symbol" ) );
            wxMessageDialog::ButtonLabel noButtonLabel( _( "Keep Symbol" ) );

            wxMessageDialog dlg( this, msg, _( "Warning" ),
                                 wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION | wxCENTER );
            dlg.SetYesNoLabels( yesButtonLabel, noButtonLabel );

            if( dlg.ShowModal() == wxID_NO )
                continue;
        }

        if( IsCurrentSymbol( libId ) )
            emptyScreen();

        m_libMgr->RemoveSymbol( libId.GetLibItemName(), libId.GetLibNickname() );
    }

    m_treePane->GetLibTree()->RefreshLibTree();
}


void SYMBOL_EDIT_FRAME::CopySymbolToClipboard()
{
    std::vector<LIB_ID> symbols;

    if( GetTreeLIBIDs( symbols ) == 0 )
        return;

    STRING_FORMATTER formatter;

    for( LIB_ID& libId : symbols )
    {
        LIB_SYMBOL* symbol = m_libMgr->GetBufferedSymbol( libId.GetLibItemName(),
                                                          libId.GetLibNickname() );

        if( !symbol )
            continue;

        std::unique_ptr<LIB_SYMBOL> tmp = symbol->Flatten();
        SCH_SEXPR_PLUGIN::FormatLibSymbol( tmp.get(), formatter );
    }

    wxLogNull doNotLog; // disable logging of failed clipboard actions

    auto clipboard = wxTheClipboard;
    wxClipboardLocker clipboardLock( clipboard );

    if( !clipboardLock || !clipboard->IsOpened() )
        return;

    auto data = new wxTextDataObject( wxString( formatter.GetString().c_str(), wxConvUTF8 ) );
    clipboard->SetData( data );

    clipboard->Flush();
}


void SYMBOL_EDIT_FRAME::DuplicateSymbol( bool aFromClipboard )
{
    LIB_ID libId = GetTargetLibId();
    wxString lib = libId.GetLibNickname();

    if( !m_libMgr->LibraryExists( lib ) )
        return;

    std::vector<LIB_SYMBOL*> newSymbols;

    if( aFromClipboard )
    {
        std::string clipboardData = m_toolManager->GetClipboardUTF8();

        try
        {
            newSymbols = SCH_SEXPR_PLUGIN::ParseLibSymbols( clipboardData, "Clipboard" );
        }
        catch( IO_ERROR& e )
        {
            wxLogMessage( wxS( "Can not paste: %s" ), e.Problem() );
        }
    }
    else
    {
        LIB_SYMBOL*  srcSymbol = m_libMgr->GetBufferedSymbol( libId.GetLibItemName(), lib );

        wxCHECK( srcSymbol, /* void */ );

        newSymbols.emplace_back( new LIB_SYMBOL( *srcSymbol ) );

        // Derive from same parent.
        if( srcSymbol->IsAlias() )
        {
            std::shared_ptr< LIB_SYMBOL > srcParent = srcSymbol->GetParent().lock();

            wxCHECK( srcParent, /* void */ );

            newSymbols.back()->SetParent( srcParent.get() );
        }
    }

    if( newSymbols.empty() )
        return;

    for( LIB_SYMBOL* symbol : newSymbols )
    {
        ensureUniqueName( symbol, lib );
        m_libMgr->UpdateSymbol( symbol, lib );

        LoadOneLibrarySymbolAux( symbol, lib, GetUnit(), GetConvert() );
    }

    SyncLibraries( false );
    m_treePane->GetLibTree()->SelectLibId( LIB_ID( lib, newSymbols[0]->GetName() ) );

    for( LIB_SYMBOL* symbol : newSymbols )
        delete symbol;
}


void SYMBOL_EDIT_FRAME::ensureUniqueName( LIB_SYMBOL* aSymbol, const wxString& aLibrary )
{
    wxCHECK( aSymbol, /* void */ );

    int      i = 1;
    wxString newName = aSymbol->GetName();

    // Append a number to the name until the name is unique in the library.
    while( m_libMgr->SymbolExists( newName, aLibrary ) )
        newName.Printf( "%s_%d", aSymbol->GetName(), i++ );

    aSymbol->SetName( newName );
}


void SYMBOL_EDIT_FRAME::Revert( bool aConfirm )
{
    LIB_ID libId = GetTargetLibId();
    const wxString& libName = libId.GetLibNickname();

    // Empty if this is the library itself that is selected.
    const wxString& symbolName = libId.GetLibItemName();

    wxString msg = wxString::Format( _( "Revert '%s' to last version saved?" ),
                                     symbolName.IsEmpty() ? libName : symbolName );

    if( aConfirm && !ConfirmRevertDialog( this, msg ) )
        return;

    bool reload_currentSymbol = false;
    wxString curr_symbolName = symbolName;

    if( GetCurSymbol() )
    {
        // the library itself is reverted: the current symbol will be reloaded only if it is
        // owned by this library
        if( symbolName.IsEmpty() )
        {
            LIB_ID curr_libId = GetCurSymbol()->GetLibId();
            reload_currentSymbol = libName == curr_libId.GetLibNickname();

            if( reload_currentSymbol )
                curr_symbolName = curr_libId.GetLibItemName();
        }
        else
        {
            reload_currentSymbol = IsCurrentSymbol( libId );
        }
    }

    int unit = m_unit;

    if( reload_currentSymbol )
        emptyScreen();

    if( symbolName.IsEmpty() )
    {
        m_libMgr->RevertLibrary( libName );
    }
    else
    {
        libId = m_libMgr->RevertSymbol( libId.GetLibItemName(), libId.GetLibNickname() );

        m_treePane->GetLibTree()->SelectLibId( libId );
        m_libMgr->ClearSymbolModified( libId.GetLibItemName(), libId.GetLibNickname() );
    }

    if( reload_currentSymbol && m_libMgr->SymbolExists( curr_symbolName, libName ) )
        LoadSymbol( curr_symbolName, libName, unit );

    m_treePane->Refresh();
}


void SYMBOL_EDIT_FRAME::RevertAll()
{
    wxCHECK_RET( m_libMgr, "Library manager object not created." );

    Revert( false );
    m_libMgr->RevertAll();
}


void SYMBOL_EDIT_FRAME::LoadSymbol( const wxString& aAlias, const wxString& aLibrary, int aUnit )
{
    if( GetCurSymbol() && IsSymbolFromSchematic() && GetScreen()->IsContentModified() )
    {
        if( !HandleUnsavedChanges( this, _( "The current symbol has been modified.  Save changes?" ),
                                   [&]() -> bool
                                   {
                                       return saveCurrentSymbol();
                                   } ) )
        {
            return;
        }
    }

    LIB_SYMBOL* symbol = m_libMgr->GetBufferedSymbol( aAlias, aLibrary );

    if( !symbol )
    {
        DisplayError( this, wxString::Format( _( "Symbol %s not found in library '%s'." ),
                                              aAlias,
                                              aLibrary ) );
        return;
    }

    // Optimize default edit options for this symbol
    // Usually if units are locked, graphic items are specific to each unit
    // and if units are interchangeable, graphic items are common to units
    SYMBOL_EDITOR_DRAWING_TOOLS* tools = GetToolManager()->GetTool<SYMBOL_EDITOR_DRAWING_TOOLS>();
    tools->SetDrawSpecificUnit( symbol->UnitsLocked() );

    LoadOneLibrarySymbolAux( symbol, aLibrary, aUnit, 0 );
}


bool SYMBOL_EDIT_FRAME::saveLibrary( const wxString& aLibrary, bool aNewFile )
{
    wxFileName fn;
    wxString   msg;
    SYMBOL_SAVEAS_TYPE     type = SYMBOL_SAVEAS_TYPE::NORMAL_SAVE_AS;
    SCH_IO_MGR::SCH_FILE_T fileType = SCH_IO_MGR::SCH_FILE_T::SCH_KICAD;
    PROJECT&   prj = Prj();

    m_toolManager->RunAction( ACTIONS::cancelInteractive, true );

    if( !aNewFile && ( aLibrary.empty() || !prj.SchSymbolLibTable()->HasLibrary( aLibrary ) ) )
    {
        ShowInfoBarError( _( "No library specified." ) );
        return false;
    }

    if( aNewFile )
    {
        SEARCH_STACK* search = prj.SchSearchS();

        // Get a new name for the library
        wxString default_path = prj.GetRString( PROJECT::SCH_LIB_PATH );

        if( !default_path )
            default_path = search->LastVisitedPath();

        fn.SetName( aLibrary );
        fn.SetExt( KiCadSymbolLibFileExtension );

        wxString wildcards = KiCadSymbolLibFileWildcard();

        wxFileDialog dlg( this, wxString::Format( _( "Save Library '%s' As..." ), aLibrary ),
                          default_path, fn.GetFullName(), wildcards,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

#if wxCHECK_VERSION( 3, 1, 7 )
        SYMBOL_FILEDLG_SAVE_AS saveAsHook( type );
        dlg.SetCustomizeHook( saveAsHook );
#else
        dlg.SetExtraControlCreator( &SYMBOL_LEGACYFILEDLG_SAVE_AS::Create );
#endif

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        fn = dlg.GetPath();

        prj.SetRString( PROJECT::SCH_LIB_PATH, fn.GetPath() );

        if( fn.GetExt().IsEmpty() )
            fn.SetExt( KiCadSymbolLibFileExtension );

#if wxCHECK_VERSION( 3, 1, 7 )
        type = saveAsHook.GetOption();
#else
        const SYMBOL_LEGACYFILEDLG_SAVE_AS* sah =
                dynamic_cast<const SYMBOL_LEGACYFILEDLG_SAVE_AS*>( dlg.GetExtraControl() );
        wxCHECK( sah, false );

        type = sah->GetOption();
#endif
    }
    else
    {
        fn = prj.SchSymbolLibTable()->GetFullURI( aLibrary );
        fileType = SCH_IO_MGR::GuessPluginTypeFromLibPath( fn.GetFullPath() );
    }

    // Verify the user has write privileges before attempting to save the library file.
    if( !aNewFile && m_libMgr->IsLibraryReadOnly( aLibrary ) )
        return false;

    ClearMsgPanel();

    // Copy .kicad_symb file to .bak.
    if( !backupFile( fn, "bak" ) )
        return false;

    if( !m_libMgr->SaveLibrary( aLibrary, fn.GetFullPath(), fileType ) )
    {
        msg.Printf( _( "Failed to save changes to symbol library file '%s'." ),
                    fn.GetFullPath() );
        DisplayErrorMessage( this, _( "Error Saving Library" ), msg );
        return false;
    }

    if( !aNewFile )
    {
        m_libMgr->ClearLibraryModified( aLibrary );
    }
    else
    {
        bool resyncLibTree = false;
        wxString originalLibNickname = getTargetLib();
        wxString forceRefresh;

        switch( type )
        {
        case SYMBOL_SAVEAS_TYPE::REPLACE_TABLE_ENTRY:
            resyncLibTree = replaceLibTableEntry( originalLibNickname, fn.GetFullPath() );
            forceRefresh = originalLibNickname;
            break;

        case SYMBOL_SAVEAS_TYPE::ADD_GLOBAL_TABLE_ENTRY:
            resyncLibTree = addLibTableEntry( fn.GetFullPath() );
            break;

        case SYMBOL_SAVEAS_TYPE::ADD_PROJECT_TABLE_ENTRY:
            resyncLibTree = addLibTableEntry( fn.GetFullPath(), PROJECT_LIB_TABLE );
            break;

        default:
            break;
        }

        if( resyncLibTree )
        {
            FreezeLibraryTree();
            SyncLibraries( true, false, forceRefresh );
            ThawLibraryTree();
        }
    }

    ClearMsgPanel();
    msg.Printf( _( "Symbol library file '%s' saved." ), fn.GetFullPath() );
    RebuildSymbolUnitsList();

    return true;
}


bool SYMBOL_EDIT_FRAME::saveAllLibraries( bool aRequireConfirmation )
{
    wxString msg, msg2;
    bool     doSave = true;
    int      dirtyCount = 0;
    bool     applyToAll = false;
    bool     retv = true;

    for( const wxString& libNickname : m_libMgr->GetLibraryNames() )
    {
        if( m_libMgr->IsLibraryModified( libNickname ) )
            dirtyCount++;
    }

    for( const wxString& libNickname : m_libMgr->GetLibraryNames() )
    {
        if( m_libMgr->IsLibraryModified( libNickname ) )
        {
            if( aRequireConfirmation && !applyToAll )
            {
                msg.Printf( _( "Save changes to '%s' before closing?" ), libNickname );

                switch( UnsavedChangesDialog( this, msg, dirtyCount > 1 ? &applyToAll : nullptr ) )
                {
                case wxID_YES: doSave = true;  break;
                case wxID_NO:  doSave = false; break;
                default:
                case wxID_CANCEL: return false;
                }
            }

            if( doSave )
            {
                // If saving under existing name fails then do a Save As..., and if that
                // fails then cancel close action.
                if( !m_libMgr->IsLibraryReadOnly( libNickname ) )
                {
                    if( saveLibrary( libNickname, false ) )
                        continue;
                }
                else
                {
                    msg.Printf( _( "Symbol library '%s' is not writable." ), libNickname );
                    msg2 = _( "You must save to a different location." );

                    if( dirtyCount == 1 )
                    {
                        if( OKOrCancelDialog( this, _( "Warning" ), msg, msg2 ) != wxID_OK )
                        {
                            retv = false;
                            continue;
                        }
                    }
                    else
                    {
                        m_infoBar->Dismiss();
                        m_infoBar->ShowMessageFor( msg + wxS( "  " ) + msg2,
                                                   2000, wxICON_EXCLAMATION );

                        while( m_infoBar->IsShown() )
                            wxSafeYield();

                        retv = false;
                        continue;
                    }
                }

                if( !saveLibrary( libNickname, true ) )
                    retv = false;
            }
        }
    }

    updateTitle();
    return retv;
}


void SYMBOL_EDIT_FRAME::UpdateSymbolMsgPanelInfo()
{
    EDA_DRAW_FRAME::ClearMsgPanel();

    if( !m_symbol )
        return;

    wxString msg = m_symbol->GetName();

    AppendMsgPanel( _( "Name" ), UnescapeString( msg ), 8 );

    if( m_symbol->IsAlias() )
    {
        LIB_SYMBOL_SPTR parent = m_symbol->GetParent().lock();

        msg = parent ? parent->GetName() : _( "Undefined!" );
        AppendMsgPanel( _( "Parent" ), UnescapeString( msg ), 8 );
    }

    static wxChar UnitLetter[] = wxT( "?ABCDEFGHIJKLMNOPQRSTUVWXYZ" );
    msg = UnitLetter[m_unit];

    AppendMsgPanel( _( "Unit" ), msg, 8 );

    if( m_convert > 1 )
        msg = _( "Convert" );
    else
        msg = _( "Normal" );

    AppendMsgPanel( _( "Body" ), msg, 8 );

    if( m_symbol->IsPower() )
        msg = _( "Power Symbol" );
    else
        msg = _( "Symbol" );

    AppendMsgPanel( _( "Type" ), msg, 8 );
    AppendMsgPanel( _( "Description" ), m_symbol->GetDescription(), 8 );
    AppendMsgPanel( _( "Keywords" ), m_symbol->GetKeyWords() );
    AppendMsgPanel( _( "Datasheet" ), m_symbol->GetDatasheetField().GetText() );
}

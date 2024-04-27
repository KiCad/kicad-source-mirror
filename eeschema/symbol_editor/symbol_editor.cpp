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
#include <kidialog.h>
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
#include <sch_io/kicad_legacy/sch_io_kicad_legacy.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <dialogs/dialog_lib_new_symbol.h>
#include <eda_list_dialog.h>
#include <wx/clipbrd.h>
#include <wx/filedlg.h>
#include <wx/log.h>
#include <project_sch.h>
#include <string_utils.h>
#include "symbol_saveas_type.h"

#include <widgets/symbol_filedlg_save_as.h>


void SYMBOL_EDIT_FRAME::UpdateTitle()
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

    UpdateTitle();
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
            const wxString& libName = GetCurSymbol()->GetLibId().GetLibNickname();

            if( m_libMgr->IsLibraryReadOnly( libName ) )
            {
                wxString msg = wxString::Format( _( "Symbol library '%s' is not writable." ),
                                                 libName );
                wxString msg2 = _( "You must save to a different location." );

                if( OKOrCancelDialog( this, _( "Warning" ), msg, msg2 ) == wxID_OK )
                    return saveLibrary( libName, true );
            }
            else
            {
                return saveLibrary( libName, false );
            }
        }
    }

    return false;
}


bool SYMBOL_EDIT_FRAME::LoadSymbol( const LIB_ID& aLibId, int aUnit, int aBodyStyle )
{
    LIB_ID libId = aLibId;

    // Some libraries can't be edited, so load the underlying chosen symbol
    if( SYMBOL_LIB_TABLE_ROW* lib = m_libMgr->GetLibrary( aLibId.GetLibNickname() ) )
    {
        if( lib->SchLibType() == SCH_IO_MGR::SCH_DATABASE
            || lib->SchLibType() == SCH_IO_MGR::SCH_CADSTAR_ARCHIVE
            || lib->SchLibType() == SCH_IO_MGR::SCH_HTTP )

        {
            try
            {
                LIB_SYMBOL* readOnlySym = PROJECT_SCH::SchSymbolLibTable( &Prj() )->LoadSymbol( aLibId );

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
            && GetBodyStyle() == aBodyStyle )
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

    if( LoadSymbolFromCurrentLib( libId.GetLibItemName(), aUnit, aBodyStyle ) )
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
                                                  int aBodyStyle )
{
    LIB_SYMBOL* alias = nullptr;

    try
    {
        alias = PROJECT_SCH::SchSymbolLibTable( &Prj() )->LoadSymbol( GetCurLib(), aAliasName );
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

    if( !alias || !LoadOneLibrarySymbolAux( alias, GetCurLib(), aUnit, aBodyStyle ) )
        return false;

    // Enable synchronized pin edit mode for symbols with interchangeable units
    m_SyncPinEdit = GetCurSymbol()->IsMulti() && !GetCurSymbol()->UnitsLocked();

    ClearUndoRedoList();
    m_toolManager->RunAction( ACTIONS::zoomFitScreen );
    SetShowDeMorgan( GetCurSymbol()->Flatten()->HasAlternateBodyStyle() );

    if( aUnit > 0 )
        RebuildSymbolUnitsList();

    return true;
}


bool SYMBOL_EDIT_FRAME::LoadOneLibrarySymbolAux( LIB_SYMBOL* aEntry, const wxString& aLibrary,
                                                 int aUnit, int aBodyStyle )
{
    bool rebuildMenuAndToolbar = false;

    if( !aEntry || aLibrary.empty() )
        return false;

    if( aEntry->GetName().IsEmpty() )
    {
        wxLogWarning( "Symbol in library '%s' has empty name field.", aLibrary );
        return false;
    }

    m_toolManager->RunAction( ACTIONS::cancelInteractive );

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
    m_bodyStyle = aBodyStyle > 0 ? aBodyStyle : 1;

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

    UpdateTitle();
    RebuildSymbolUnitsList();
    SetShowDeMorgan( GetCurSymbol()->HasAlternateBodyStyle() );

    ClearUndoRedoList();

    if( !IsSymbolFromSchematic() )
    {
        LIB_ID libId = GetCurSymbol()->GetLibId();
        setSymWatcher( &libId );
    }

    // Let tools add things to the view if necessary
    if( m_toolManager )
        m_toolManager->ResetTools( TOOL_BASE::MODEL_RELOAD );

    GetCanvas()->GetView()->UpdateAllItems( KIGFX::ALL );

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


void SYMBOL_EDIT_FRAME::CreateNewSymbol( const wxString& aInheritFrom )
{
    m_toolManager->RunAction( ACTIONS::cancelInteractive );

    wxArrayString symbolNames;
    wxString lib = getTargetLib();

    if( !m_libMgr->LibraryExists( lib ) )
    {
        lib = SelectLibraryFromList();

        if( !m_libMgr->LibraryExists( lib ) )
            return;
    }

    m_libMgr->GetSymbolNames( lib, symbolNames );

    symbolNames.Sort();

    wxString _inheritSymbolName;
    wxString _infoMessage;
    wxString msg;

    // if the symbol being inherited from isn't a root symbol, find its root symbol
    // and use that symbol instead
    if( !aInheritFrom.IsEmpty() )
    {
        LIB_SYMBOL* inheritFromSymbol = m_libMgr->GetBufferedSymbol( aInheritFrom, lib );

        if( inheritFromSymbol )
        {
            _inheritSymbolName = aInheritFrom;
            _infoMessage = wxString::Format( _( "Deriving from symbol '%s'." ),
                                             _inheritSymbolName );
        }
        else
        {
            _inheritSymbolName = aInheritFrom;
        }
    }

    DIALOG_LIB_NEW_SYMBOL dlg( this, _infoMessage, &symbolNames, _inheritSymbolName,
            [&]( wxString newName )
            {
                if( newName.IsEmpty() )
                {
                    wxMessageBox( _( "Symbol must have a name." ) );
                    return false;
                }

                if( !lib.empty() && m_libMgr->SymbolExists( newName, lib ) )
                {
                    msg = wxString::Format( _( "Symbol '%s' already exists in library '%s'." ),
                                            newName,
                                            lib );

                    KIDIALOG errorDlg( this, msg, _( "Confirmation" ),
                                       wxOK | wxCANCEL | wxICON_WARNING );
                    errorDlg.SetOKLabel( _( "Overwrite" ) );

                    return errorDlg.ShowModal() == wxID_OK;
                }

                return true;
            } );

    dlg.SetMinSize( dlg.GetSize() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    wxString name = dlg.GetName();

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
        new_symbol.SetExcludedFromBOM( !dlg.GetIncludeInBom() );
        new_symbol.SetExcludedFromBoard( !dlg.GetIncludeOnBoard() );

        if( dlg.GetUnitCount() < 2 )
            new_symbol.LockUnits( false );

        new_symbol.SetHasAlternateBodyStyle( dlg.GetAlternateBodyStyle() );
    }
    else
    {
        LIB_SYMBOL* parent = m_libMgr->GetAlias( parentSymbolName, lib );
        wxCHECK( parent, /* void */ );
        new_symbol.SetParent( parent );

        // Inherit the parent mandatory field attributes.
        for( int id = 0; id < MANDATORY_FIELDS; ++id )
        {
            SCH_FIELD* field = new_symbol.GetFieldById( id );

            // the MANDATORY_FIELDS are exactly that in RAM.
            wxCHECK( field, /* void */ );

            SCH_FIELD* parentField = parent->GetFieldById( id );

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
        SetShowDeMorgan( new_symbol.HasAlternateBodyStyle() );
}


void SYMBOL_EDIT_FRAME::Save()
{
    wxString libName;

    if( IsSymbolTreeShown() )
        libName = GetTreeLIBID().GetUniStringLibNickname();

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

    UpdateTitle();
}


void SYMBOL_EDIT_FRAME::SaveLibraryAs()
{
    wxCHECK( !GetTargetLibId().GetLibNickname().empty(), /* void */ );

    const wxString& libName = GetTargetLibId().GetLibNickname();

    saveLibrary( libName, true );
    m_treePane->GetLibTree()->RefreshLibTree();
}


void SYMBOL_EDIT_FRAME::SaveSymbolCopyAs()
{
    saveSymbolCopyAs();

    m_treePane->GetLibTree()->RefreshLibTree();
}


static int ID_MAKE_NEW_LIBRARY = 4173;


class SAVE_AS_DIALOG : public EDA_LIST_DIALOG
{
public:
    SAVE_AS_DIALOG( SYMBOL_EDIT_FRAME* aParent, const wxString& aSymbolName,
                    const wxString& aLibraryPreselect,
                    std::function<bool( wxString libName, wxString symbolName )> aValidator ) :
            EDA_LIST_DIALOG( aParent, _( "Save Symbol As" ), false ),
            m_validator( std::move( aValidator ) )
    {
        COMMON_SETTINGS*           cfg = Pgm().GetCommonSettings();
        PROJECT_FILE&              project = aParent->Prj().GetProjectFile();
        SYMBOL_LIB_TABLE*          tbl = PROJECT_SCH::SchSymbolLibTable( &Prj() );
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

        initDialog( headers, itemsToDisplay, aLibraryPreselect );

        SetListLabel( _( "Save in library:" ) );
        SetOKLabel( _( "Save" ) );

        wxBoxSizer* bNameSizer = new wxBoxSizer( wxHORIZONTAL );

        wxStaticText* label = new wxStaticText( this, wxID_ANY, _( "Name:" ) );
        bNameSizer->Add( label, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

        m_symbolNameCtrl = new wxTextCtrl( this, wxID_ANY, aSymbolName );
        bNameSizer->Add( m_symbolNameCtrl, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

        wxButton* newLibraryButton = new wxButton( this, ID_MAKE_NEW_LIBRARY, _( "New Library..." ) );
        m_ButtonsSizer->Prepend( 80, 20 );
        m_ButtonsSizer->Prepend( newLibraryButton, 0, wxALIGN_CENTER_VERTICAL|wxLEFT|wxRIGHT, 10 );

        GetSizer()->Prepend( bNameSizer, 0, wxEXPAND|wxTOP|wxLEFT|wxRIGHT, 5 );

        Bind( wxEVT_BUTTON,
                [this]( wxCommandEvent& )
                {
                    EndModal( ID_MAKE_NEW_LIBRARY );
                }, ID_MAKE_NEW_LIBRARY );

        // Move nameTextCtrl to the head of the tab-order
        if( GetChildren().DeleteObject( m_symbolNameCtrl ) )
            GetChildren().Insert( m_symbolNameCtrl );

        SetInitialFocus( m_symbolNameCtrl );

        SetupStandardButtons();

        Layout();
        GetSizer()->Fit( this );

        Centre();
    }

    wxString GetSymbolName()
    {
        wxString symbolName = m_symbolNameCtrl->GetValue();
        symbolName.Trim( true );
        symbolName.Trim( false );
        symbolName.Replace( " ", "_" );
        return symbolName;
    }

protected:
    bool TransferDataFromWindow() override
    {
        return m_validator( GetTextSelection(), GetSymbolName() );
    }

private:
    wxTextCtrl*                                                  m_symbolNameCtrl;
    std::function<bool( wxString libName, wxString symbolName )> m_validator;
};


void SYMBOL_EDIT_FRAME::saveSymbolCopyAs()
{
    LIB_SYMBOL* symbol = getTargetSymbol();

    if( !symbol )
        return;

    LIB_ID   old_lib_id = symbol->GetLibId();
    wxString symbolName = old_lib_id.GetLibItemName();
    wxString libraryName = old_lib_id.GetLibNickname();
    bool     valueFollowsName = symbol->GetValueField().GetText() == symbolName;
    wxString msg;
    bool     done = false;

    while( !done )
    {
        SAVE_AS_DIALOG dlg( this, symbolName, libraryName,
                [&]( const wxString& newLib, const wxString& newName )
                {
                    if( newLib.IsEmpty() )
                    {
                        wxMessageBox( _( "A library must be specified." ) );
                        return false;
                    }

                    if( newName.IsEmpty() )
                    {
                        wxMessageBox( _( "Symbol must have a name." ) );
                        return false;
                    }

                    if( m_libMgr->SymbolExists( newName, newLib ) )
                    {
                        msg = wxString::Format( _( "Symbol '%s' already exists in library '%s'." ),
                                                newName, newLib );

                        KIDIALOG errorDlg( this, msg, _( "Confirmation" ),
                                           wxOK | wxCANCEL | wxICON_WARNING );
                        errorDlg.SetOKLabel( _( "Overwrite" ) );

                        return errorDlg.ShowModal() == wxID_OK;
                    }

                    // @todo Either check the selecteced library to see if the parent symbol name
                    //       is in the new library and/or copy the parent symbol as well.  This is
                    //       the lazy solution to ensure derived symbols do not get orphaned.
                    if( symbol->IsAlias() && newLib != old_lib_id.GetLibNickname().wx_str() )
                    {
                        DisplayError( this, _( "Derived symbols must be saved in the same library "
                                               "as their parent symbol." ) );
                        return false;
                    }

                    return true;
                } );

        int ret = dlg.ShowModal();

        if( ret == wxID_CANCEL )
        {
            return;
        }
        else if( ret == wxID_OK )
        {
            symbolName = dlg.GetSymbolName();
            libraryName = dlg.GetTextSelection();
            done = true;
        }
        else if( ret == ID_MAKE_NEW_LIBRARY )
        {
            wxFileName newLibrary( AddLibraryFile( true ) );
            libraryName = newLibrary.GetName();
        }
    }

    LIB_SYMBOL new_symbol( *symbol );
    new_symbol.SetName( symbolName );

    if( valueFollowsName )
        new_symbol.GetValueField().SetText( symbolName );

    m_libMgr->UpdateSymbol( &new_symbol, libraryName );
    SyncLibraries( false );
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
    SetShowDeMorgan( GetCurSymbol()->Flatten()->HasAlternateBodyStyle() );
    UpdateTitle();

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
        SCH_IO_KICAD_SEXPR::FormatLibSymbol( tmp.get(), formatter );
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
            newSymbols = SCH_IO_KICAD_SEXPR::ParseLibSymbols( clipboardData, "Clipboard" );
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

        LoadOneLibrarySymbolAux( symbol, lib, GetUnit(), GetBodyStyle() );
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
            reload_currentSymbol = libName == curr_libId.GetLibNickname().wx_str();

            if( reload_currentSymbol )
                curr_symbolName = curr_libId.GetUniStringLibItemName();
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
        m_treePane->GetLibTree()->RefreshLibTree();
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

    m_toolManager->RunAction( ACTIONS::cancelInteractive );

    if( !aNewFile && ( aLibrary.empty() || !PROJECT_SCH::SchSymbolLibTable( &prj )->HasLibrary( aLibrary ) ) )
    {
        ShowInfoBarError( _( "No library specified." ) );
        return false;
    }

    if( aNewFile )
    {
        SEARCH_STACK* search = PROJECT_SCH::SchSearchS( &prj );

        // Get a new name for the library
        wxString default_path = prj.GetRString( PROJECT::SCH_LIB_PATH );

        if( !default_path )
            default_path = search->LastVisitedPath();

        fn.SetName( aLibrary );
        fn.SetExt( FILEEXT::KiCadSymbolLibFileExtension );

        wxString wildcards = FILEEXT::KiCadSymbolLibFileWildcard();

        wxFileDialog dlg( this, wxString::Format( _( "Save Library '%s' As..." ), aLibrary ),
                          default_path, fn.GetFullName(), wildcards,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        SYMBOL_FILEDLG_SAVE_AS saveAsHook( type );
        dlg.SetCustomizeHook( saveAsHook );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        fn = dlg.GetPath();

        prj.SetRString( PROJECT::SCH_LIB_PATH, fn.GetPath() );

        if( fn.GetExt().IsEmpty() )
            fn.SetExt( FILEEXT::KiCadSymbolLibFileExtension );

        type = saveAsHook.GetOption();
    }
    else
    {
        fn = PROJECT_SCH::SchSymbolLibTable( &prj )->GetFullURI( aLibrary );
        fileType = SCH_IO_MGR::GuessPluginTypeFromLibPath( fn.GetFullPath() );

        if( fileType == SCH_IO_MGR::SCH_FILE_UNKNOWN )
            fileType = SCH_IO_MGR::SCH_KICAD;
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

        // Update the library modification time so that we don't reload based on the watcher
        if( aLibrary == getTargetLib() )
            SetSymModificationTime( fn.GetModificationTime() );
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
                if( m_libMgr->IsLibraryReadOnly( libNickname ) )
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

                        while( m_infoBar->IsShownOnScreen() )
                            wxSafeYield();

                        retv = false;
                        continue;
                    }
                }
                else if( saveLibrary( libNickname, false ) )
                {
                    continue;
                }

                if( !saveLibrary( libNickname, true ) )
                    retv = false;
            }
        }
    }

    UpdateTitle();
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

    if( m_bodyStyle == BODY_STYLE::DEMORGAN )
        msg = _( "Alternate" );
    else if( m_bodyStyle == BODY_STYLE::BASE )
        msg = _( "Standard" );
    else
        msg = wxT( "?" );

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

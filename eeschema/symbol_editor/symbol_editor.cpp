/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <confirm.h>
#include <kiway.h>
#include <widgets/infobar.h>
#include <tools/ee_actions.h>
#include <tools/symbol_editor_drawing_tools.h>
#include <symbol_edit_frame.h>
#include <symbol_library.h>
#include <template_fieldnames.h>
#include <wildcards_and_files_ext.h>
#include <symbol_lib_table.h>
#include <symbol_library_manager.h>
#include <symbol_tree_pane.h>
#include <widgets/lib_tree.h>
#include <sch_plugins/legacy/sch_legacy_plugin.h>
#include <sch_plugins/kicad/sch_sexpr_plugin.h>
#include <dialogs/dialog_lib_new_symbol.h>
#include <eda_list_dialog.h>
#include <wx/clipbrd.h>
#include <wx/filedlg.h>
#include <wx/log.h>
#include <string_utils.h>


/**
 * Helper control to inquire user what to do on library save as operation.
 */
class SAVE_AS_HELPER : public wxPanel
{
public:
    SAVE_AS_HELPER( wxWindow* aParent ) :
        wxPanel( aParent )
    {
        m_simpleSaveAs = new wxRadioButton( this, wxID_ANY, _( "Normal save as operation" ),
                                            wxDefaultPosition, wxDefaultSize, wxRB_GROUP );
        m_simpleSaveAs->SetToolTip( _( "Do not perform any additional operations after saving "
                                       "library." ) );
        m_replaceTableEntry = new wxRadioButton( this, wxID_ANY,
                                                 _( "Replace library table entry" ) );
        m_replaceTableEntry->SetToolTip( _( "Replace symbol library table entry with new library."
                                            "\n\nThe original library will no longer be available "
                                            "for use." ) );
        m_addGlobalTableEntry = new wxRadioButton( this, wxID_ANY,
                                                   _( "Add new global library table entry" ) );
        m_addGlobalTableEntry->SetToolTip( _( "Add new entry to the global symbol library table."
                                              "\n\nThe symbol library table nickname is suffixed "
                                              "with\nan integer to ensure no duplicate table "
                                              "entries." ) );
        m_addProjectTableEntry = new wxRadioButton( this, wxID_ANY,
                                                    _( "Add new project library table entry" ) );
        m_addProjectTableEntry->SetToolTip( _( "Add new entry to the project symbol library table."
                                               "\n\nThe symbol library table nickname is suffixed "
                                               "with\nan integer to ensure no duplicate table "
                                               "entries." ) );

        wxBoxSizer* sizer = new wxBoxSizer( wxHORIZONTAL );
        sizer->Add( m_simpleSaveAs, 0, wxLEFT | wxRIGHT | wxALIGN_CENTER_VERTICAL, 5 );
        sizer->Add( m_replaceTableEntry, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 5 );
        sizer->Add( m_addGlobalTableEntry, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 5 );
        sizer->Add( m_addProjectTableEntry, 0, wxRIGHT | wxALIGN_CENTER_VERTICAL, 5 );

        SetSizerAndFit( sizer );
    }

    enum SAH_TYPE
    {
        UNDEFINED = -1,
        NORMAL_SAVE_AS,
        REPLACE_TABLE_ENTRY,
        ADD_GLOBAL_TABLE_ENTRY,
        ADD_PROJECT_TABLE_ENTRY
    };

    SAH_TYPE GetOption() const
    {
        if( m_simpleSaveAs->GetValue() )
            return SAH_TYPE::NORMAL_SAVE_AS;
        else if( m_replaceTableEntry->GetValue() )
            return SAH_TYPE::REPLACE_TABLE_ENTRY;
        else if( m_addGlobalTableEntry->GetValue() )
            return ADD_GLOBAL_TABLE_ENTRY;
        else if( m_addProjectTableEntry->GetValue() )
            return ADD_PROJECT_TABLE_ENTRY;
        else
            return UNDEFINED;
    }

    /**
     * Create a new panel to add to a wxFileDialog object.
     *
     * The caller owns the created object and is responsible for deleting it.
     *
     * @param aParent is the parent window that will own the created object.
     * @return the newly created panel to add to the wxFileDialog.
     */
    static wxWindow* Create( wxWindow* aParent )
    {
        wxCHECK( aParent, nullptr );

        return new SAVE_AS_HELPER( aParent );
    }

private:
    wxRadioButton* m_simpleSaveAs;
    wxRadioButton* m_replaceTableEntry;
    wxRadioButton* m_addGlobalTableEntry;
    wxRadioButton* m_addProjectTableEntry;
};


void SYMBOL_EDIT_FRAME::updateTitle()
{
    wxString lib = GetCurLib();
    wxString title;

    if( IsSymbolFromSchematic() )
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


wxString SYMBOL_EDIT_FRAME::SelectLibraryFromList()
{
    PROJECT& prj = Prj();

    if( prj.SchSymbolLibTable()->IsEmpty() )
    {
        ShowInfoBarError( _( "No symbol libraries are loaded." ) );
        return wxEmptyString;
    }

    wxArrayString headers;

    headers.Add( _( "Library" ) );

    std::vector< wxArrayString > itemsToDisplay;
    std::vector< wxString > libNicknames = prj.SchSymbolLibTable()->GetLogicalLibs();

    // Conversion from wxArrayString to vector of ArrayString
    for( const wxString& name : libNicknames )
    {
        wxArrayString item;

        // Exclude read only libraries.
        if( m_libMgr->IsLibraryReadOnly( name ) )
            continue;

        item.Add( name );
        itemsToDisplay.push_back( item );
    }

    wxString oldLibName = prj.GetRString( PROJECT::SCH_LIB_SELECT );

    EDA_LIST_DIALOG dlg( this, _( "Select Symbol Library" ), headers, itemsToDisplay, oldLibName );

    if( dlg.ShowModal() != wxID_OK )
        return wxEmptyString;

    wxString libName = dlg.GetTextSelection();

    if( !libName.empty() )
    {
        if( prj.SchSymbolLibTable()->HasLibrary( libName ) )
            prj.SetRString( PROJECT::SCH_LIB_SELECT, libName );
        else
            libName = wxEmptyString;
    }

    return libName;
}


bool SYMBOL_EDIT_FRAME::saveCurrentSymbol()
{
    if( GetCurSymbol() )
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

    return false;
}


bool SYMBOL_EDIT_FRAME::LoadSymbol( const LIB_ID& aLibId, int aUnit, int aConvert )
{
    if( GetCurSymbol() && GetCurSymbol()->GetLibId() == aLibId
            && GetUnit() == aUnit && GetConvert() == aConvert )
    {
        return true;
    }

    if( GetScreen()->IsContentModified() && GetCurSymbol() )
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

    SelectActiveLibrary( aLibId.GetLibNickname() );

    if( LoadSymbolFromCurrentLib( aLibId.GetLibItemName(), aUnit, aConvert ) )
    {
        m_treePane->GetLibTree()->SelectLibId( aLibId );
        m_treePane->GetLibTree()->ExpandLibId( aLibId );

        m_centerItemOnIdle = aLibId;
        Bind( wxEVT_IDLE, &SYMBOL_EDIT_FRAME::centerItemIdleHandler, this );

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
    m_SyncPinEdit = !GetCurSymbol()->UnitsLocked();

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
    wxString msg, rootName;
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


void SYMBOL_EDIT_FRAME::CreateNewSymbol()
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

    DIALOG_LIB_NEW_SYMBOL dlg( this, &rootSymbols );
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
        new_symbol.LockUnits( dlg.GetLockItems() );
        new_symbol.SetIncludeInBom( dlg.GetIncludeInBom() );
        new_symbol.SetIncludeOnBoard( dlg.GetIncludeOnBoard() );

        if( dlg.GetUnitCount() < 2 )
            new_symbol.LockUnits( false );

        new_symbol.SetConversion( dlg.GetAlternateBodyStyle() );

        // must be called after loadSymbol, that calls SetShowDeMorgan, but
        // because the symbol is empty,it looks like it has no alternate body
        SetShowDeMorgan( dlg.GetAlternateBodyStyle() );
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
}


void SYMBOL_EDIT_FRAME::Save()
{
    if( getTargetSymbol() == m_symbol )
    {
        if( IsSymbolFromSchematic() )
        {
            SCH_EDIT_FRAME* schframe = (SCH_EDIT_FRAME*) Kiway().Player( FRAME_SCH, false );

            if( !schframe )      // happens when the schematic editor is not active (or closed)
            {
                DisplayErrorMessage( this, _( "No schematic currently open." ) );
            }
            else
            {
                schframe->SaveSymbolToSchematic( *m_symbol );
                GetScreen()->SetContentModified( false );
            }
        }
        else
        {
            saveCurrentSymbol();
        }
    }
    else if( !GetTargetLibId().GetLibNickname().empty() )
    {
        LIB_ID          libId   = GetTargetLibId();
        const wxString& libName = libId.GetLibNickname();

        if( m_libMgr->IsLibraryReadOnly( libName ) )
        {
            wxString msg = wxString::Format( _( "Symbol library '%s' is not writeable." ),
                                             libName );
            wxString msg2 = _( "You must save to a different location." );

            if( OKOrCancelDialog( this, _( "Warning" ), msg, msg2 ) == wxID_OK )
                saveLibrary( libName, true );
        }
        else
        {
            saveLibrary( libName, false );
        }
    }

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
    wxCHECK( GetTargetLibId().IsValid(), /* void */ );

    saveSymbolAs();

    m_treePane->GetLibTree()->RefreshLibTree();
}


void SYMBOL_EDIT_FRAME::saveSymbolAs()
{
    LIB_SYMBOL* symbol = getTargetSymbol();

    if( symbol )
    {
        LIB_ID   old_lib_id = symbol->GetLibId();
        wxString old_name = old_lib_id.GetLibItemName();
        wxString old_lib = old_lib_id.GetLibNickname();

        SYMBOL_LIB_TABLE*            tbl = Prj().SchSymbolLibTable();
        wxArrayString                headers;
        std::vector< wxArrayString > itemsToDisplay;
        std::vector< wxString >      libNicknames = tbl->GetLogicalLibs();

        headers.Add( _( "Nickname" ) );
        headers.Add( _( "Description" ) );

        for( const wxString& name : libNicknames )
        {
            wxArrayString item;
            item.Add( name );
            item.Add( tbl->GetDescription( name ) );
            itemsToDisplay.push_back( item );
        }

        EDA_LIST_DIALOG dlg( this, _( "Save Symbol As" ), headers, itemsToDisplay, old_lib );
        dlg.SetListLabel( _( "Save in library:" ) );
        dlg.SetOKLabel( _( "Save" ) );

        wxBoxSizer* bNameSizer = new wxBoxSizer( wxHORIZONTAL );

        wxStaticText* label = new wxStaticText( &dlg, wxID_ANY, _( "Name:" ),
                                                wxDefaultPosition, wxDefaultSize, 0 );
        bNameSizer->Add( label, 0, wxALIGN_CENTER_VERTICAL|wxTOP|wxBOTTOM|wxLEFT, 5 );

        wxTextCtrl* nameTextCtrl = new wxTextCtrl( &dlg, wxID_ANY, old_name,
                                                   wxDefaultPosition, wxDefaultSize, 0 );
        bNameSizer->Add( nameTextCtrl, 1, wxALIGN_CENTER_VERTICAL|wxALL, 5 );

        wxSizer* mainSizer = dlg.GetSizer();
        mainSizer->Prepend( bNameSizer, 0, wxEXPAND|wxTOP|wxLEFT|wxRIGHT, 5 );

        // Move nameTextCtrl to the head of the tab-order
        if( dlg.GetChildren().DeleteObject( nameTextCtrl ) )
            dlg.GetChildren().Insert( nameTextCtrl );

        dlg.SetInitialFocus( nameTextCtrl );

        dlg.Layout();
        mainSizer->Fit( &dlg );

        if( dlg.ShowModal() != wxID_OK )
            return;                   // canceled by user

        wxString new_lib = dlg.GetTextSelection();

        if( new_lib.IsEmpty() )
        {
            DisplayError( this, _( "No library specified.  Symbol could not be saved." ) );
            return;
        }

        // @todo Either check the selecteced library to see if the parent symbol name is in
        //       the new library and/or copy the parent symbol as well.  This is the lazy
        //       solution to ensure derived symbols do not get orphaned.
        if( symbol->IsAlias() && new_lib != old_lib )
        {
            DisplayError( this, _( "Derived symbols must be saved in the same library as their "
                                   "parent symbol." ) );
            return;
        }

        wxString new_name = nameTextCtrl->GetValue();
        new_name.Trim( true );
        new_name.Trim( false );
        new_name.Replace( " ", "_" );

        if( new_name.IsEmpty() )
        {
            // This is effectively a cancel.  No need to nag the user about it.
            return;
        }

        // Test if there is a symbol with this name already.
        if( m_libMgr->SymbolExists( new_name, new_lib ) )
        {
            wxString msg = wxString::Format( _( "Symbol '%s' already exists in library '%s'" ),
                                             new_name,
                                             new_lib );

            KIDIALOG errorDlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
            errorDlg.SetOKLabel( _( "Overwrite" ) );
            errorDlg.DoNotShowCheckbox( __FILE__, __LINE__ );

            if( errorDlg.ShowModal() == wxID_CANCEL )
                return;
        }

        LIB_SYMBOL new_symbol( *symbol );
        new_symbol.SetName( new_name );

        m_libMgr->UpdateSymbol( &new_symbol, new_lib );
        SyncLibraries( false );
        m_treePane->GetLibTree()->SelectLibId( LIB_ID( new_lib, new_symbol.GetName() ) );
        LoadSymbol( new_name, new_lib, m_unit );
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
    UpdateMsgPanel();

    RebuildView();
    OnModify();
}


void SYMBOL_EDIT_FRAME::DeleteSymbolFromLibrary()
{
    LIB_ID libId = GetTargetLibId();

    if( m_libMgr->IsSymbolModified( libId.GetLibItemName(), libId.GetLibNickname() )
        && !IsOK( this, wxString::Format( _( "The symbol '%s' has been modified.\n"
                                             "Do you want to remove it from the library?" ),
                                          libId.GetUniStringLibItemName() ) ) )
    {
        return;
    }

    if( m_libMgr->HasDerivedSymbols( libId.GetLibItemName(), libId.GetLibNickname() ) )
    {
        wxString msg;

        msg.Printf( _( "The symbol %s is used to derive other symbols.\n"
                       "Deleting this symbol will delete all of the symbols derived from it.\n\n"
                       "Do you wish to delete this symbol and all of its derivatives?" ),
                    libId.GetLibItemName().wx_str() );

        wxMessageDialog::ButtonLabel yesButtonLabel( _( "Delete Symbol" ) );
        wxMessageDialog::ButtonLabel noButtonLabel( _( "Keep Symbol" ) );

        wxMessageDialog dlg( this, msg, _( "Warning" ),
                             wxYES_NO | wxYES_DEFAULT | wxICON_QUESTION | wxCENTER );
        dlg.SetYesNoLabels( yesButtonLabel, noButtonLabel );

        if( dlg.ShowModal() == wxID_NO )
            return;
    }

    if( isCurrentSymbol( libId ) )
        emptyScreen();

    m_libMgr->RemoveSymbol( libId.GetLibItemName(), libId.GetLibNickname() );

    m_treePane->GetLibTree()->RefreshLibTree();
}


void SYMBOL_EDIT_FRAME::CopySymbolToClipboard()
{
    int dummyUnit;
    LIB_ID libId = m_treePane->GetLibTree()->GetSelectedLibId( &dummyUnit );
    LIB_SYMBOL* symbol = m_libMgr->GetBufferedSymbol( libId.GetLibItemName(),
                                                      libId.GetLibNickname() );

    if( !symbol )
        return;

    std::unique_ptr< LIB_SYMBOL> tmp = symbol->Flatten();
    STRING_FORMATTER formatter;
    SCH_SEXPR_PLUGIN::FormatLibSymbol( tmp.get(), formatter );

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
    int dummyUnit;
    LIB_ID libId = m_treePane->GetLibTree()->GetSelectedLibId( &dummyUnit );
    wxString lib = libId.GetLibNickname();

    if( !m_libMgr->LibraryExists( lib ) )
        return;

    LIB_SYMBOL* srcSymbol = nullptr;
    LIB_SYMBOL* newSymbol = nullptr;

    if( aFromClipboard )
    {
        wxLogNull doNotLog; // disable logging of failed clipboard actions

        auto clipboard = wxTheClipboard;
        wxClipboardLocker clipboardLock( clipboard );

        if( !clipboardLock || ! clipboard->IsSupported( wxDF_TEXT ) )
            return;

        wxTextDataObject data;
        clipboard->GetData( data );
        wxString symbolSource = data.GetText();

        STRING_LINE_READER reader( TO_UTF8( symbolSource ), "Clipboard" );

        try
        {
            newSymbol = SCH_SEXPR_PLUGIN::ParseLibSymbol( reader );
        }
        catch( IO_ERROR& e )
        {
            wxLogMessage( "Can not paste: %s", e.Problem() );
            return;
        }
    }
    else
    {
        srcSymbol = m_libMgr->GetBufferedSymbol( libId.GetLibItemName(), lib );

        wxCHECK( srcSymbol, /* void */ );

        newSymbol = new LIB_SYMBOL( *srcSymbol );

        // Derive from same parent.
        if( srcSymbol->IsAlias() )
        {
            std::shared_ptr< LIB_SYMBOL > srcParent = srcSymbol->GetParent().lock();

            wxCHECK( srcParent, /* void */ );

            newSymbol->SetParent( srcParent.get() );
        }
    }

    if( !newSymbol )
        return;

    ensureUniqueName( newSymbol, lib );
    m_libMgr->UpdateSymbol( newSymbol, lib );

    LoadOneLibrarySymbolAux( newSymbol, lib, GetUnit(), GetConvert() );

    SyncLibraries( false );
    m_treePane->GetLibTree()->SelectLibId( LIB_ID( lib, newSymbol->GetName() ) );

    delete newSymbol;
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
            reload_currentSymbol = isCurrentSymbol( libId );
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
    LIB_SYMBOL* symbol = m_libMgr->GetBufferedSymbol( aAlias, aLibrary );

    if( !symbol )
    {
        wxString msg;

        msg.Printf( _( "Symbol %s not found in library '%s'." ),
                    aAlias,
                    aLibrary );
        DisplayError( this, msg );
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
    SAVE_AS_HELPER::SAH_TYPE type = SAVE_AS_HELPER::SAH_TYPE::UNDEFINED;
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

        dlg.SetExtraControlCreator( &SAVE_AS_HELPER::Create );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        fn = dlg.GetPath();

        prj.SetRString( PROJECT::SCH_LIB_PATH, fn.GetPath() );

        if( fn.GetExt().IsEmpty() )
            fn.SetExt( KiCadSymbolLibFileExtension );

        const SAVE_AS_HELPER* sah = dynamic_cast<const SAVE_AS_HELPER*>( dlg.GetExtraControl() );
        wxCHECK( sah, false );

        type = sah->GetOption();
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
        case SAVE_AS_HELPER::SAH_TYPE::REPLACE_TABLE_ENTRY:
            resyncLibTree = replaceLibTableEntry( originalLibNickname, fn.GetFullPath() );
            forceRefresh = originalLibNickname;
            break;

        case SAVE_AS_HELPER::SAH_TYPE::ADD_GLOBAL_TABLE_ENTRY:
            resyncLibTree = addLibTableEntry( fn.GetFullPath() );
            break;

        case SAVE_AS_HELPER::SAH_TYPE::ADD_PROJECT_TABLE_ENTRY:
            resyncLibTree = addLibTableEntry( fn.GetFullPath(), PROJECT_LIB_TABLE );
            break;

        case SAVE_AS_HELPER::SAH_TYPE::NORMAL_SAVE_AS:
        default:
            break;
        }

        if( resyncLibTree )
        {
            FreezeLibraryTree();
            SyncLibraries( true, forceRefresh );
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
                    msg.Printf( _( "Symbol library '%s' is not writeable." ), libNickname );
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

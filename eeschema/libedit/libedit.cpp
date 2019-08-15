/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <pgm_base.h>
#include <confirm.h>
#include <gestfich.h>
#include <tools/ee_actions.h>
#include <lib_edit_frame.h>
#include <class_library.h>
#include <template_fieldnames.h>
#include <wildcards_and_files_ext.h>
#include <sch_edit_frame.h>
#include <symbol_lib_table.h>
#include <lib_manager.h>
#include <symbol_tree_pane.h>
#include <widgets/lib_tree.h>
#include <sch_legacy_plugin.h>
#include <dialog_choose_component.h>
#include <symbol_tree_model_adapter.h>
#include <tool/tool_manager.h>
#include <dialogs/dialog_lib_new_component.h>
#include <dialog_helpers.h>
#include <wx/clipbrd.h>

void LIB_EDIT_FRAME::updateTitle()
{
    wxString lib = GetCurLib();
    wxString title = _( "Symbol Editor" );

    if( GetCurPart() )
        title += wxT( " \u2014 " ) + GetCurPart()->GetLibId().Format();

    SetTitle( title );
}


void LIB_EDIT_FRAME::SelectActiveLibrary( const wxString& aLibrary )
{
    wxString selectedLib = aLibrary;

    if( selectedLib.empty() )
        selectedLib = SelectLibraryFromList();

    if( !selectedLib.empty() )
        SetCurLib( selectedLib );

    updateTitle();
}


wxString LIB_EDIT_FRAME::SelectLibraryFromList()
{
    PROJECT& prj = Prj();

    if( prj.SchSymbolLibTable()->IsEmpty() )
    {
        DisplayError( this, _( "No symbol libraries are loaded." ) );
        return wxEmptyString;
    }

    wxArrayString headers;

    headers.Add( _( "Library" ) );

    std::vector< wxArrayString > itemsToDisplay;
    std::vector< wxString > libNicknames = prj.SchSymbolLibTable()->GetLogicalLibs();

    // Conversion from wxArrayString to vector of ArrayString
    for( const auto& name : libNicknames )
    {
        wxArrayString item;

        item.Add( name );
        itemsToDisplay.push_back( item );
    }

    wxString old_lib_name = prj.GetRString( PROJECT::SCH_LIB_SELECT );

    EDA_LIST_DIALOG dlg( this, _( "Select Symbol Library" ), headers, itemsToDisplay,
                         old_lib_name );

    if( dlg.ShowModal() != wxID_OK )
        return wxEmptyString;

    wxString libname = dlg.GetTextSelection();

    if( !libname.empty() )
    {
        if( prj.SchSymbolLibTable()->HasLibrary( libname ) )
            prj.SetRString( PROJECT::SCH_LIB_SELECT, libname );
        else
            libname = wxEmptyString;
    }

    return libname;
}


bool LIB_EDIT_FRAME::saveCurrentPart()
{
    if( GetCurPart() )
    {
        LIB_ID libId = GetCurPart()->GetLibId();
        const wxString& libName = libId.GetLibNickname();
        const wxString& partName = libId.GetLibItemName();

        if( m_libMgr->FlushPart( partName, libName ) )
        {
            m_libMgr->ClearPartModified( partName, libName );
            return true;
        }
    }

    return false;
}


bool LIB_EDIT_FRAME::LoadComponentAndSelectLib( const LIB_ID& aLibId, int aUnit, int aConvert )
{
    if( GetCurPart() && GetCurPart()->GetLibId() == aLibId
            && GetUnit() == aUnit && GetConvert() == aConvert )
    {
        return true;
    }

    if( GetScreen()->IsModify() && GetCurPart() )
    {
        if( !HandleUnsavedChanges( this, _( "The current symbol has been modified.  "
                                            "Save changes?" ),
                                   [&]()->bool { return saveCurrentPart(); } ) )
        {
            return false;
        }
    }

    SelectActiveLibrary( aLibId.GetLibNickname() );
    return LoadComponentFromCurrentLib( aLibId.GetLibItemName(), aUnit, aConvert );
}


bool LIB_EDIT_FRAME::LoadComponentFromCurrentLib( const wxString& aAliasName, int aUnit,
                                                  int aConvert )
{
    LIB_ALIAS* alias = nullptr;

    try
    {
        alias = Prj().SchSymbolLibTable()->LoadSymbol( GetCurLib(), aAliasName );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg;

        msg.Printf( _( "Error occurred loading symbol \"%s\" from library \"%s\"." ),
                    aAliasName, GetCurLib() );
        DisplayErrorMessage( this, msg, ioe.What() );
        return false;
    }

    if( !alias || !LoadOneLibraryPartAux( alias, GetCurLib(), aUnit, aConvert ) )
        return false;

    // Enable synchronized pin edit mode for symbols with interchangeable units
    m_SyncPinEdit = !GetCurPart()->UnitsLocked();

    GetScreen()->ClearUndoRedoList();
    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
    SetShowDeMorgan( GetCurPart()->HasConversion() );

    if( aUnit > 0 )
        RebuildSymbolUnitsList();

    return true;
}

/**
 * Synchronize screen settings from a current screen into another screen.
 *
 * This can be used, for example, when loading a new screen into a frame,
 * but you want the new screen to inherit some settings (e.g. grids) from the
 * frame's current screen.
 *
 * @param aCurrentScreen    the existing frame screen
 * @param aIncomingScreen   a screen that is intended to replace the current screen
 */
static void synchronizeLibEditScreenSettings( const SCH_SCREEN& aCurrentScreen,
                                              SCH_SCREEN& aIncomingScreen )
{
    aIncomingScreen.SetGrid( aCurrentScreen.GetGridSize() );
}


bool LIB_EDIT_FRAME::LoadOneLibraryPartAux( LIB_ALIAS* aEntry, const wxString& aLibrary,
                                            int aUnit, int aConvert )
{
    wxString msg, rootName;

    if( !aEntry || aLibrary.empty() )
        return false;

    if( aEntry->GetName().IsEmpty() )
    {
        wxLogWarning( "Symbol in library \"%s\" has empty name field.", aLibrary );
        return false;
    }

    LIB_PART* lib_part = m_libMgr->GetBufferedPart( aEntry->GetName(), aLibrary );
    wxASSERT( lib_part );

    m_unit = aUnit > 0 ? aUnit : 1;
    m_convert = aConvert > 0 ? aConvert : 1;

    // The buffered screen for the part
    SCH_SCREEN* part_screen = m_libMgr->GetScreen( lib_part->GetName(), aLibrary );

    const SCH_SCREEN* curr_screen = GetScreen();

    // Before we set the frame screen, transfer any settings from the current
    // screen that we want to keep to the incoming (buffered) part's screen
    // which could be out of date relative to the current screen.
    if( curr_screen )
        synchronizeLibEditScreenSettings( *curr_screen, *part_screen );

    SetScreen( part_screen );
    SetCurPart( new LIB_PART( *lib_part ) );
    SetCurLib( aLibrary );

    m_toolManager->RunAction( ACTIONS::zoomFitScreen, true );
    updateTitle();
    RebuildSymbolUnitsList();
    SetShowDeMorgan( GetCurPart()->HasConversion() );
    SyncToolbars();

    // Display the document information based on the entry selected just in
    // case the entry is an alias.
    DisplayCmpDoc();
    Refresh();

    return true;
}


void LIB_EDIT_FRAME::SaveAll()
{
    saveAllLibraries( false );
    m_treePane->Refresh();
    refreshSchematic();
}


void LIB_EDIT_FRAME::CreateNewPart()
{
    m_toolManager->RunAction( ACTIONS::cancelInteractive, true );
    wxString lib = getTargetLib();

    if( !m_libMgr->LibraryExists( lib ) )
    {
        lib = SelectLibraryFromList();

        if( !m_libMgr->LibraryExists( lib ) )
            return;
    }

    DIALOG_LIB_NEW_COMPONENT dlg( this );
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

    // Test if there is a component with this name already.
    if( !lib.empty() && m_libMgr->PartExists( name, lib ) )
    {
        wxString msg = wxString::Format( _( "Symbol \"%s\" already exists in library \"%s\"" ),
                                         name, lib );
        DisplayError( this, msg );
        return;
    }

    LIB_PART new_part( name );      // do not create part on the heap, it will be buffered soon
    new_part.GetReferenceField().SetText( dlg.GetReference() );
    new_part.SetUnitCount( dlg.GetUnitCount() );

    // Initialize new_part.m_TextInside member:
    // if 0, pin text is outside the body (on the pin)
    // if > 0, pin text is inside the body

    if( dlg.GetPinNameInside() )
    {
        new_part.SetPinNameOffset( dlg.GetPinTextPosition() );

        if( new_part.GetPinNameOffset() == 0 )
            new_part.SetPinNameOffset( 1 );
    }
    else
    {
        new_part.SetPinNameOffset( 0 );
    }

    ( dlg.GetPowerSymbol() ) ? new_part.SetPower() : new_part.SetNormal();
    new_part.SetShowPinNumbers( dlg.GetShowPinNumber() );
    new_part.SetShowPinNames( dlg.GetShowPinName() );
    new_part.LockUnits( dlg.GetLockItems() );

    if( dlg.GetUnitCount() < 2 )
        new_part.LockUnits( false );

    m_libMgr->UpdatePart( &new_part, lib );
    SyncLibraries( false );
    LoadPart( name, lib, 1 );

    new_part.SetConversion( dlg.GetAlternateBodyStyle() );
    // must be called after loadPart, that calls SetShowDeMorgan, but
    // because the symbol is empty,it looks like it has no alternate body
    SetShowDeMorgan( dlg.GetAlternateBodyStyle() );
}


void LIB_EDIT_FRAME::Save()
{
    LIB_ID libId = getTargetLibId();
    const wxString& libName = libId.GetLibNickname();
    const wxString& partName = libId.GetLibItemName();

    if( partName.IsEmpty() )
    {
        saveLibrary( libName, false );
    }
    else
    {
        // Save Part
        if( m_libMgr->FlushPart( partName, libName ) )
            m_libMgr->ClearPartModified( partName, libName );
    }

    m_treePane->Refresh();
    refreshSchematic();
}


void LIB_EDIT_FRAME::SaveAs()
{
    LIB_ID libId = getTargetLibId();
    const wxString& libName = libId.GetLibNickname();
    const wxString& partName = libId.GetLibItemName();

    if( partName.IsEmpty() )
        saveLibrary( libName, true );
    else
        savePartAs();

    m_treePane->Refresh();
    refreshSchematic();
}


void LIB_EDIT_FRAME::savePartAs()
{
    LIB_ID old_lib_id = getTargetLibId();
    wxString old_name = old_lib_id.GetLibItemName();
    wxString old_lib = old_lib_id.GetLibNickname();
    LIB_PART* part = m_libMgr->GetBufferedPart( old_name, old_lib );

    if( part )
    {
        SYMBOL_LIB_TABLE* tbl = Prj().SchSymbolLibTable();
        wxArrayString headers;
        std::vector< wxArrayString > itemsToDisplay;
        std::vector< wxString > libNicknames = tbl->GetLogicalLibs();

        headers.Add( _( "Nickname" ) );
        headers.Add( _( "Description" ) );

        for( const auto& name : libNicknames )
        {
            wxArrayString item;
            item.Add( name );
            item.Add( tbl->GetDescription( name ) );
            itemsToDisplay.push_back( item );
        }

        EDA_LIST_DIALOG dlg( this, _( "Save Copy of Symbol" ), headers, itemsToDisplay, old_lib,
                             nullptr, nullptr, /* sort */ false, /* show headers */ false );
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
            DisplayError( NULL, _( "No library specified.  Symbol could not be saved." ) );
            return;
        }

        wxString new_name = nameTextCtrl->GetValue();
        new_name.Trim( true );
        new_name.Trim( false );
        new_name.Replace( " ", "_" );

        if( new_name.IsEmpty() )
        {
            DisplayError( NULL, _( "No symbol name specified.  Symbol could not be saved." ) );
            return;
        }

        // Test if there is a component with this name already.
        if( m_libMgr->PartExists( new_name, new_lib ) )
        {
            wxString msg = wxString::Format( _( "Symbol \"%s\" already exists in library \"%s\"" ),
                                             new_name, new_lib );
            DisplayError( this, msg );
            return;
        }

        LIB_PART new_part( *part );
        new_part.SetName( new_name );

        fixDuplicateAliases( &new_part, new_lib );
        m_libMgr->UpdatePart( &new_part, new_lib );
        SyncLibraries( false );
        m_treePane->GetLibTree()->SelectLibId( LIB_ID( new_lib, new_part.GetName() ) );

        if( isCurrentPart( old_lib_id ) )
            LoadPart( new_name, new_lib, m_unit );
    }
}


void LIB_EDIT_FRAME::UpdateAfterSymbolProperties( wxString* aOldName, wxArrayString* aOldAliases )
{
    wxString  msg;
    wxString  lib = GetCurLib();
    LIB_PART* part = GetCurPart();

    if( aOldName && *aOldName != part->GetName() )
    {
        // Test the current library for name conflicts
        if( !lib.empty() && m_libMgr->PartExists( part->GetName(), lib ) )
        {
            msg.Printf( _( "The name '%s' conflicts with an existing entry in the library '%s'." ),
                        part->GetName(),
                        lib );

            DisplayErrorMessage( this, msg );
            part->SetName( *aOldName );
        }
        else
            m_libMgr->UpdatePartAfterRename( part, *aOldName, lib );
    }

    if( aOldAliases && *aOldAliases != part->GetAliasNames( false ) )
        SyncLibraries( false );

    // Reselect the renamed part
    m_treePane->GetLibTree()->SelectLibId( LIB_ID( lib, part->GetName() ) );

    RebuildSymbolUnitsList();
    SetShowDeMorgan( part->HasConversion() );
    updateTitle();
    DisplayCmpDoc();

    RebuildView();
    OnModify();
}


void LIB_EDIT_FRAME::DeletePartFromLibrary()
{
    LIB_ID libId = getTargetLibId();

    if( m_libMgr->IsPartModified( libId.GetLibItemName(), libId.GetLibNickname() )
        && !IsOK( this, _( wxString::Format( "Component %s has been modified\n"
                                             "Do you want to remove it from the library?",
                                             libId.GetUniStringLibItemName() ) ) ) )
    {
        return;
    }

    if( isCurrentPart( libId ) )
        emptyScreen();

    m_libMgr->RemovePart( libId.GetLibItemName(), libId.GetLibNickname() );

    refreshSchematic();
}


void LIB_EDIT_FRAME::CopyPartToClipboard()
{
    int dummyUnit;
    LIB_ID libId = m_treePane->GetLibTree()->GetSelectedLibId( &dummyUnit );
    LIB_PART* part = m_libMgr->GetBufferedPart( libId.GetLibItemName(), libId.GetLibNickname() );

    if( !part )
        return;

    STRING_FORMATTER formatter;
    SCH_LEGACY_PLUGIN::FormatPart( part, formatter );

    auto clipboard = wxTheClipboard;
    wxClipboardLocker clipboardLock( clipboard );

    if( !clipboardLock || !clipboard->IsOpened() )
        return;

    auto data = new wxTextDataObject( wxString( formatter.GetString().c_str(), wxConvUTF8 ) );
    clipboard->SetData( data );

    clipboard->Flush();
}


void LIB_EDIT_FRAME::DuplicatePart( bool aFromClipboard )
{
    int dummyUnit;
    LIB_ID libId = m_treePane->GetLibTree()->GetSelectedLibId( &dummyUnit );
    wxString lib = libId.GetLibNickname();

    if( !m_libMgr->LibraryExists( lib ) )
        return;

    LIB_PART* srcPart = nullptr;
    LIB_PART* newPart = nullptr;

    if( aFromClipboard )
    {
        auto clipboard = wxTheClipboard;
        wxClipboardLocker clipboardLock( clipboard );

        if( !clipboardLock || ! clipboard->IsSupported( wxDF_TEXT ) )
            return;

        wxTextDataObject data;
        clipboard->GetData( data );
        wxString partSource = data.GetText();

        STRING_LINE_READER reader( TO_UTF8( partSource ), "Clipboard" );

        try
        {
            reader.ReadLine();
            newPart = SCH_LEGACY_PLUGIN::ParsePart( reader );
        }
        catch( IO_ERROR& e )
        {
            wxLogMessage( "Can not paste: %s", GetChars( e.Problem() ) );
            return;
        }
    }
    else
    {
        srcPart = m_libMgr->GetBufferedPart( libId.GetLibItemName(), lib );
        newPart = new LIB_PART( *srcPart );
    }

    if( !newPart )
        return;

    fixDuplicateAliases( newPart, lib );
    m_libMgr->UpdatePart( newPart, lib );
    SyncLibraries( false );
    m_treePane->GetLibTree()->SelectLibId( LIB_ID( lib, newPart->GetName() ) );

    delete newPart;
}


void LIB_EDIT_FRAME::fixDuplicateAliases( LIB_PART* aPart, const wxString& aLibrary )
{
    wxString newName;

    for( unsigned int i = 0; i < aPart->GetAliasCount(); ++i )
    {
        LIB_ALIAS* alias = aPart->GetAlias( i );
        int sfx = 0;
        newName = alias->GetName();

        while( m_libMgr->PartExists( newName, aLibrary ) )
        {
            if( sfx == 0 )
                newName = wxString::Format( "%s_copy", alias->GetName() );
            else
                newName = wxString::Format( "%s_copy%d", alias->GetName(), sfx );
            ++sfx;
        }

        if( i == 0 )
            aPart->SetName( newName );
        else
            alias->SetName( newName );
    }
}


void LIB_EDIT_FRAME::Revert( bool aConfirm )
{
    LIB_ID libId = getTargetLibId();
    const wxString& libName = libId.GetLibNickname();
    const wxString& partName = libId.GetLibItemName();  // Empty if this is the library itself that is selected

    wxString msg = wxString::Format( _( "Revert \"%s\" to last version saved?" ),
                                     partName.IsEmpty() ? libName : partName );

    if( aConfirm && !ConfirmRevertDialog( this, msg ) )
        return;

    bool reload_currentPart = false;
    wxString curr_partName = partName;

    if( GetCurPart() )
    {
        // the library itself is reverted: the current part will be reloaded only if it is
        // owned by this library
        if( partName.IsEmpty() )
        {
            LIB_ID curr_libId = GetCurPart()->GetLibId();
            reload_currentPart = libName == curr_libId.GetLibNickname();

            if( reload_currentPart )
                curr_partName = curr_libId.GetLibItemName();
        }
        else
            reload_currentPart = isCurrentPart( libId );
    }

    int unit = m_unit;

    if( reload_currentPart )
        emptyScreen();

    if( partName.IsEmpty() )
    {
        m_libMgr->RevertLibrary( libName );
    }
    else
    {
        libId = m_libMgr->RevertPart( libId.GetLibItemName(), libId.GetLibNickname() );

        m_treePane->GetLibTree()->SelectLibId( libId );
        m_libMgr->ClearPartModified( libId.GetLibItemName(), libId.GetLibNickname() );
    }

    if( reload_currentPart && m_libMgr->PartExists( curr_partName, libName ) )
        LoadPart( curr_partName, libName, unit );

    m_treePane->Refresh();
    refreshSchematic();
}


void LIB_EDIT_FRAME::RevertAll()
{
    wxCHECK_RET( m_libMgr, "Library manager object not created." );

    Revert( false );
    m_libMgr->RevertAll();
}


void LIB_EDIT_FRAME::LoadPart( const wxString& aAlias, const wxString& aLibrary, int aUnit )
{
    wxCHECK( m_libMgr->PartExists( aAlias, aLibrary ), /* void */ );
    LIB_PART* part = m_libMgr->GetBufferedPart( aAlias, aLibrary );
    LIB_ALIAS* alias = part ? part->GetAlias( aAlias ) : nullptr;

    if( !alias )
    {
        wxString msg = wxString::Format( _( "Symbol name \"%s\" not found in library \"%s\"" ),
                                         GetChars( aAlias ),
                                         GetChars( aLibrary ) );
        DisplayError( this, msg );
        return;
    }

    // Optimize default edit options for this symbol
    // Usually if units are locked, graphic items are specific to each unit
    // and if units are interchangeable, graphic items are common to units
    m_DrawSpecificUnit = part->UnitsLocked();

    LoadOneLibraryPartAux( alias, aLibrary, aUnit, 0 );
}


bool LIB_EDIT_FRAME::saveLibrary( const wxString& aLibrary, bool aNewFile )
{
    wxFileName fn;
    wxString   msg;
    PROJECT&   prj = Prj();

    m_toolManager->RunAction( ACTIONS::cancelInteractive, true );

    if( !aNewFile && ( aLibrary.empty() || !prj.SchSymbolLibTable()->HasLibrary( aLibrary ) ) )
    {
        DisplayError( this, _( "No library specified." ) );
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
        fn.SetExt( SchematicLibraryFileExtension );

        wxFileDialog dlg( this, wxString::Format( _( "Save Library \"%s\" As..." ), aLibrary ),
                          default_path, fn.GetFullName(), SchematicLibraryFileWildcard(),
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        fn = dlg.GetPath();

        // The GTK file chooser doesn't return the file extension added to
        // file name so add it here.
        if( fn.GetExt().IsEmpty() )
            fn.SetExt( SchematicLibraryFileExtension );
    }
    else
    {
        fn = prj.SchSymbolLibTable()->GetFullURI( aLibrary );
    }

    // Verify the user has write privileges before attempting to save the library file.
    if( !IsWritable( fn ) )
        return false;

    ClearMsgPanel();

    // Copy .lib file to .bak.
    if( !backupFile( fn, "bak" ) )
        return false;

    wxFileName docFileName = fn;
    docFileName.SetExt( DOC_EXT );

    // Copy .dcm file to .bck.
    if( !backupFile( docFileName, "bck" ) )
        return false;

    if( !m_libMgr->SaveLibrary( aLibrary, fn.GetFullPath() ) )
    {
        msg.Printf( _( "Failed to save changes to symbol library file \"%s\"" ),
                    fn.GetFullPath() );
        DisplayErrorMessage( this, _( "Error saving library" ), msg );
        return false;
    }

    if( !aNewFile )
        m_libMgr->ClearLibraryModified( aLibrary );

    msg.Printf( _( "Symbol library file \"%s\" saved" ), fn.GetFullPath() );
    wxString msg1;
    msg1.Printf( _( "Symbol library documentation file \"%s\" saved" ), docFileName.GetFullPath() );
    AppendMsgPanel( msg, msg1, BLUE );
    RebuildSymbolUnitsList();

    return true;
}


bool LIB_EDIT_FRAME::saveAllLibraries( bool aRequireConfirmation )
{
    bool doSave = true;
    int dirtyCount = 0;
    bool applyToAll = false;

    for( const auto& libNickname : m_libMgr->GetLibraryNames() )
    {
        if( m_libMgr->IsLibraryModified( libNickname ) )
            dirtyCount++;
    }

    for( const auto& libNickname : m_libMgr->GetLibraryNames() )
    {
        if( m_libMgr->IsLibraryModified( libNickname ) )
        {
            if( aRequireConfirmation && !applyToAll )
            {
                wxString msg = wxString::Format( _( "Save changes to \"%s\" before closing?" ),
                                                 libNickname );

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
                if( !saveLibrary( libNickname, false ) && !saveLibrary( libNickname, true ) )
                    return false;
            }
        }
    }

    return true;
}

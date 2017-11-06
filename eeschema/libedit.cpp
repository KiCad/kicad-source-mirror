/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file libedit.cpp
 * @brief Eeschema component library editor.
 */

#include <fctsys.h>
#include <kiway.h>
#include <gr_basic.h>
#include <macros.h>
#include <pgm_base.h>
#include <class_drawpanel.h>
#include <confirm.h>
#include <gestfich.h>
#include <class_sch_screen.h>

#include <eeschema_id.h>
#include <general.h>
#include <libeditframe.h>
#include <class_library.h>
#include <template_fieldnames.h>
#include <wildcards_and_files_ext.h>
#include <schframe.h>
#include <symbol_lib_table.h>

#include <dialog_choose_component.h>
#include <cmp_tree_model_adapter.h>

#include <dialogs/dialog_lib_new_component.h>


void LIB_EDIT_FRAME::DisplayLibInfos()
{
    wxString lib = GetCurLib();
    wxString title = _( "Symbol Library Editor - " );

    if( !lib.empty() && Prj().SchSymbolLibTable()->HasLibrary( lib ) )
    {
        wxString fileName = Prj().SchSymbolLibTable()->GetFullURI( lib );

        title += lib + " (" + fileName + ")";

        if( !wxFileName::IsFileWritable( fileName ) )
            title += " " + _( "[Read Only]" );
    }
    else
        title += _( "no library selected" );

    SetTitle( title );
}


void LIB_EDIT_FRAME::SelectActiveLibrary( const wxString& aLibrary )
{
    wxString selectedLib = aLibrary;

    if( selectedLib.empty() )
        selectedLib = SelectLibraryFromList();

    if( !selectedLib.empty() )
        SetCurLib( selectedLib );

    DisplayLibInfos();
}


bool LIB_EDIT_FRAME::LoadComponentAndSelectLib( const LIB_ID& aLibId )
{
    if( GetScreen()->IsModify()
        && !IsOK( this, _( "The current symbol is not saved.\n\nDiscard current changes?" ) ) )
        return false;

    SelectActiveLibrary( aLibId.GetLibNickname() );
    return LoadComponentFromCurrentLib( aLibId.GetLibItemName() );
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

        msg.Printf( _( "Error occurred loading symbol '%s' from library '%s'." ),
                    aAliasName, GetCurLib() );
        DisplayErrorMessage( this, msg, ioe.What() );
        return false;
    }

    if( !alias || !LoadOneLibraryPartAux( alias, GetCurLib() ) )
        return false;

    if( aUnit > 0 )
        m_unit = aUnit;

    if( aConvert > 0 )
        m_convert = aConvert;

    m_editPinsPerPartOrConvert = GetCurPart()->UnitsLocked() ? true : false;

    GetScreen()->ClearUndoRedoList();
    Zoom_Automatique( false );
    SetShowDeMorgan( GetCurPart()->HasConversion() );

    if( aUnit > 0 )
        UpdatePartSelectList();

    return true;
}


void LIB_EDIT_FRAME::LoadOneLibraryPart( wxCommandEvent& event )
{
    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );

    if( GetScreen()->IsModify()
        && !IsOK( this, _( "The current symbol is not saved.\n\nDiscard current changes?" ) ) )
        return;

    wxString lib = GetCurLib();

    // No current lib, ask user for the library to use.
    if( lib.empty() )
    {
        SelectActiveLibrary();

        lib = GetCurLib();

        if( lib.empty() )
            return;
    }

    // Get the name of the current part to preselect it
    LIB_PART* current_part = GetCurPart();
    LIB_ID id = current_part->GetLibId();

    SCH_BASE_FRAME::HISTORY_LIST dummyHistoryList;
    SCHLIB_FILTER filter;
    filter.LoadFrom( lib );
    auto sel = SelectComponentFromLibrary( &filter, dummyHistoryList, true, 0, 0, &id, false );

    if( sel.LibId.GetLibItemName().empty() )
        return;

    GetScreen()->ClrModify();
    m_lastDrawItem = m_drawItem = NULL;

    // Delete previous library symbol, if any
    SetCurPart( NULL );
    m_aliasName.Empty();

    // Load the new library symbol
    LoadComponentFromCurrentLib( sel.LibId.GetLibItemName(), sel.Unit, sel.Convert );
}


bool LIB_EDIT_FRAME::LoadOneLibraryPartAux( LIB_ALIAS* aEntry, const wxString& aLibrary )
{
    wxString msg, rootName;

    if( !aEntry || aLibrary.empty() )
        return false;

    if( aEntry->GetName().IsEmpty() )
    {
        wxLogWarning( "Symbol in library '%s' has empty name field.", aLibrary );
        return false;
    }

    wxString cmpName = m_aliasName = aEntry->GetName();

    LIB_PART* lib_part = aEntry->GetPart();

    wxASSERT( lib_part );

    LIB_PART* part = new LIB_PART( *lib_part );      // clone it and own it.
    SetCurPart( part );
    m_aliasName = aEntry->GetName();

    m_unit = 1;
    m_convert = 1;

    m_showDeMorgan = false;

    if( part->HasConversion() )
        m_showDeMorgan = true;

    GetScreen()->ClrModify();
    DisplayLibInfos();
    UpdateAliasSelectList();
    UpdatePartSelectList();

    // Display the document information based on the entry selected just in
    // case the entry is an alias.
    DisplayCmpDoc();

    return true;
}


void LIB_EDIT_FRAME::RedrawComponent( wxDC* aDC, wxPoint aOffset  )
{
    LIB_PART* part = GetCurPart();

    if( part )
    {
        // display reference like in schematic (a reference U is shown U? or U?A)
        // although it is stored without ? and part id.
        // So temporary change the reference by a schematic like reference
        LIB_FIELD*  field = part->GetField( REFERENCE );
        wxString    fieldText = field->GetText();
        wxString    fieldfullText = field->GetFullText( m_unit );

        field->EDA_TEXT::SetText( fieldfullText );  // change the field text string only
        auto opts = PART_DRAW_OPTIONS::Default();
        opts.show_elec_type = GetShowElectricalType();
        part->Draw( m_canvas, aDC, aOffset, m_unit, m_convert, opts );
        field->EDA_TEXT::SetText( fieldText );      // restore the field text string
    }
}


void LIB_EDIT_FRAME::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    if( GetScreen() == NULL )
        return;

    m_canvas->DrawBackGround( DC );

    RedrawComponent( DC, wxPoint( 0, 0 ) );

#ifdef USE_WX_OVERLAY
    if( IsShown() )
    {
        m_overlay.Reset();
        wxDCOverlay overlaydc( m_overlay, (wxWindowDC*)DC );
        overlaydc.Clear();
    }
#endif

    if( m_canvas->IsMouseCaptured() )
        m_canvas->CallMouseCapture( DC, wxDefaultPosition, false );

    m_canvas->DrawCrossHair( DC );

    DisplayLibInfos();
    UpdateStatusBar();
}


void LIB_EDIT_FRAME::OnSaveActiveLibrary( wxCommandEvent& event )
{
    SaveActiveLibrary( event.GetId() == ID_LIBEDIT_SAVE_CURRENT_LIB_AS );
}


bool LIB_EDIT_FRAME::SaveActiveLibrary( bool newFile )
{
    wxFileName fn;
    wxString   msg;
    PROJECT&   prj = Prj();

    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );

    wxString lib = GetCurLib();

    if( !newFile && ( lib.empty() || !prj.SchSymbolLibTable()->HasLibrary( lib ) ) )
    {
        DisplayError( this, _( "No library specified." ) );
        return false;
    }

    if( GetScreen()->IsModify() && !IsOK( this, _( "Include current symbol changes?" ) ) )
        return false;

    if( newFile )
    {
        SEARCH_STACK*   search = prj.SchSearchS();

        // Get a new name for the library
        wxString default_path = prj.GetRString( PROJECT::SCH_LIB_PATH );

        if( !default_path )
            default_path = search->LastVisitedPath();

        wxFileDialog dlg( this, _( "Symbol Library Name" ), default_path,
                          wxEmptyString, SchematicLibraryFileWildcard,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        fn = dlg.GetPath();

        // The GTK file chooser doesn't return the file extension added to
        // file name so add it here.
        if( fn.GetExt().IsEmpty() )
            fn.SetExt( SchematicLibraryFileExtension );

        prj.SetRString( PROJECT::SCH_LIB_PATH, fn.GetPath() );
    }
    else
    {
        fn = prj.SchSymbolLibTable()->GetFullURI( lib );

        msg.Printf( _( "Modify symbol library file '%s' ?" ), fn.GetFullPath() );

        if( !IsOK( this, msg ) )
            return false;
    }

    // Verify the user has write privileges before attempting to save the library file.
    if( !IsWritable( fn ) )
        return false;

    ClearMsgPanel();

    wxFileName libFileName = fn;
    wxFileName backupFileName = fn;

    // Copy .lib file to .bak.
    if( libFileName.FileExists() )
    {
        backupFileName.SetExt( "bak" );

        if( backupFileName.FileExists() )
            wxRemoveFile( backupFileName.GetFullPath() );

        if( !wxCopyFile( libFileName.GetFullPath(), backupFileName.GetFullPath() ) )
        {
            libFileName.MakeAbsolute();
            msg = _( "Failed to rename old symbol library to file " ) +
                  backupFileName.GetFullPath();
            DisplayError( this, msg );
            return false;
        }
    }

    wxFileName docFileName = libFileName;

    docFileName.SetExt( DOC_EXT );

    // Copy .dcm file to .bck.
    if( docFileName.FileExists() )
    {
        backupFileName.SetExt( "bck" );

        if( backupFileName.FileExists() )
            wxRemoveFile( backupFileName.GetFullPath() );

        if( !wxCopyFile( docFileName.GetFullPath(), backupFileName.GetFullPath() ) )
        {
            msg = _( "Failed to save old library document to file " ) +
                  backupFileName.GetFullPath();
            DisplayError( this, msg );
            return false;
        }
    }

    // Copy the library and document files to the new destination library files.
    if( newFile )
    {
        wxFileName src = prj.SchSymbolLibTable()->GetFullURI( GetCurLib() );

        if( !wxCopyFile( src.GetFullPath(), libFileName.GetFullPath() ) )
        {
            msg.Printf( _( "Failed to copy symbol library file " ) + libFileName.GetFullPath() );
            DisplayError( this, msg );
            return false;
        }

        src.SetExt( DOC_EXT );

        if( !wxCopyFile( src.GetFullPath(), docFileName.GetFullPath() ) )
        {
            msg.Printf( _( "Failed to copy symbol library document file " ) +
                        docFileName.GetFullPath() );
            DisplayError( this, msg );
            return false;
        }
    }

    // Update symbol changes in library.
    if( GetScreen()->IsModify() )
    {
        SCH_PLUGIN::SCH_PLUGIN_RELEASER pi( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_LEGACY ) );

        try
        {
            pi->SaveSymbol( fn.GetFullPath(), new LIB_PART( *GetCurPart() ) );
        }
        catch( const IO_ERROR& ioe )
        {
            msg.Printf( _( "Failed to save changes to symbol library file '%s'" ),
                        libFileName.GetFullPath() );
            DisplayErrorMessage( this, msg, ioe.What() );
            return false;
        }

        GetScreen()->ClrModify();
    }

    msg.Printf( _( "Symbol library file '%s' saved" ), libFileName.GetFullPath() );
    wxString msg1;
    msg1.Printf( _( "Symbol library documentation file '%s' saved" ), docFileName.GetFullPath() );
    AppendMsgPanel( msg, msg1, BLUE );
    UpdateAliasSelectList();
    UpdatePartSelectList();
    refreshSchematic();

    return true;
}


void LIB_EDIT_FRAME::DisplayCmpDoc()
{
    LIB_ALIAS*      alias;
    LIB_PART*       part = GetCurPart();

    ClearMsgPanel();

    if( !part )
        return;

    wxString msg = part->GetName();

    AppendMsgPanel( _( "Name" ), msg, BLUE, 8 );

    if( m_aliasName == part->GetName() )
        msg = _( "None" );
    else
        msg = m_aliasName;

    alias = part->GetAlias( m_aliasName );

    wxCHECK_RET( alias != NULL, "Alias not found in symbol." );

    AppendMsgPanel( _( "Alias" ), msg, RED, 8 );

    static wxChar UnitLetter[] = wxT( "?ABCDEFGHIJKLMNOPQRSTUVWXYZ" );
    msg = UnitLetter[m_unit];

    AppendMsgPanel( _( "Unit" ), msg, BROWN, 8 );

    if( m_convert > 1 )
        msg = _( "Convert" );
    else
        msg = _( "Normal" );

    AppendMsgPanel( _( "Body" ), msg, GREEN, 8 );

    if( part->IsPower() )
        msg = _( "Power Symbol" );
    else
        msg = _( "Part" );

    AppendMsgPanel( _( "Type" ), msg, MAGENTA, 8 );
    AppendMsgPanel( _( "Description" ), alias->GetDescription(), CYAN, 8 );
    AppendMsgPanel( _( "Key words" ), alias->GetKeyWords(), DARKDARKGRAY );
    AppendMsgPanel( _( "Datasheet" ), alias->GetDocFileName(), DARKDARKGRAY );
}


void LIB_EDIT_FRAME::DeleteOnePart( wxCommandEvent& event )
{
    wxString      cmp_name;
    wxArrayString nameList;
    wxString      msg;

    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );

    m_lastDrawItem = NULL;
    m_drawItem = NULL;

    LIB_PART *part = GetCurPart();
    wxString lib = GetCurLib();

    if( lib.empty() )
    {
        SelectActiveLibrary();

        lib = GetCurLib();

        if( !lib )
        {
            DisplayError( this, _( "Please select a symbol library." ) );
            return;
        }
    }

    auto adapter( CMP_TREE_MODEL_ADAPTER::Create( Prj().SchSymbolLibTable() ) );

    wxString name = part ? part->GetName() : wxString( wxEmptyString );
    adapter->SetPreselectNode( name, /* aUnit */ 0 );
    adapter->ShowUnits( false );
    adapter->AddLibrary( lib );

    wxString dialogTitle;
    dialogTitle.Printf( _( "Delete Symbol (%u items loaded)" ), adapter->GetComponentsCount() );

    DIALOG_CHOOSE_COMPONENT dlg( this, dialogTitle, adapter, m_convert, false );

    if( dlg.ShowQuasiModal() == wxID_CANCEL )
    {
        return;
    }

    LIB_ID id;

    id = dlg.GetSelectedLibId();

    if( !id.IsValid() )
        return;

    LIB_ALIAS* alias = Prj().SchSymbolLibTable()->LoadSymbol( id );

    if( !alias )
        return;

    msg.Printf( _( "Delete symbol '%s' from library '%s'?" ),
                id.GetLibItemName().wx_str(), id.GetLibNickname().wx_str() );

    if( !IsOK( this, msg ) )
        return;

    part = GetCurPart();

    if( !part || !part->HasAlias( id.GetLibItemName() ) )
    {
        Prj().SchSymbolLibTable()->DeleteAlias( id.GetLibNickname(), id.GetLibItemName() );
        m_canvas->Refresh();
        return;
    }

    // If deleting the current entry or removing one of the aliases for
    // the current entry, sync the changes in the current entry as well.

    if( GetScreen()->IsModify() && !IsOK( this, _(
        "The symbol being deleted has been modified."
        " All changes will be lost. Discard changes?" ) ) )
    {
        return;
    }

    try
    {
        Prj().SchSymbolLibTable()->DeleteAlias( id.GetLibNickname(), id.GetLibItemName() );
    }
    catch( ... /* IO_ERROR ioe */ )
    {
        msg.Printf( _( "Error occurred deleting symbol '%s' from library '%s'" ),
                    id.GetLibItemName().wx_str(), id.GetLibNickname().wx_str() );
        DisplayError( this, msg );
        return;
    }

    SetCurPart( NULL );     // delete CurPart
    m_aliasName.Empty();
    m_canvas->Refresh();
}


void LIB_EDIT_FRAME::CreateNewLibraryPart( wxCommandEvent& event )
{
    wxString name;

    if( GetCurPart() && GetScreen()->IsModify() && !IsOK( this, _(
        "All changes to the current symbol will be lost!\n\n"
        "Clear the current symbol from the screen?" ) ) )
    {
        return;
    }

    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );

    m_drawItem = NULL;

    DIALOG_LIB_NEW_COMPONENT dlg( this );

    dlg.SetMinSize( dlg.GetSize() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    if( dlg.GetName().IsEmpty() )
    {
        wxMessageBox( _( "This new symbol has no name and cannot be created." ) );
        return;
    }

    name = dlg.GetName();
    name.Replace( " ", "_" );

    wxString lib = GetCurLib();

    // Test if there a component with this name already.
    if( !lib.empty() && Prj().SchSymbolLibTable()->LoadSymbol( lib, name ) != NULL )
    {
        wxString msg = wxString::Format( _( "Symbol '%s' already exists in library '%s'" ),
                                         name, lib );
        DisplayError( this, msg );
        return;
    }

    LIB_PART* new_part = new LIB_PART( name );

    SetCurPart( new_part );
    m_aliasName = new_part->GetName();

    new_part->GetReferenceField().SetText( dlg.GetReference() );
    new_part->SetUnitCount( dlg.GetUnitCount() );

    // Initialize new_part->m_TextInside member:
    // if 0, pin text is outside the body (on the pin)
    // if > 0, pin text is inside the body
    new_part->SetConversion( dlg.GetAlternateBodyStyle() );
    SetShowDeMorgan( dlg.GetAlternateBodyStyle() );

    if( dlg.GetPinNameInside() )
    {
        new_part->SetPinNameOffset( dlg.GetPinTextPosition() );

        if( new_part->GetPinNameOffset() == 0 )
            new_part->SetPinNameOffset( 1 );
    }
    else
    {
        new_part->SetPinNameOffset( 0 );
    }

    ( dlg.GetPowerSymbol() ) ? new_part->SetPower() : new_part->SetNormal();
    new_part->SetShowPinNumbers( dlg.GetShowPinNumber() );
    new_part->SetShowPinNames( dlg.GetShowPinName() );
    new_part->LockUnits( dlg.GetLockItems() );

    if( dlg.GetUnitCount() < 2 )
        new_part->LockUnits( false );

    m_unit = 1;
    m_convert  = 1;

    DisplayLibInfos();
    DisplayCmpDoc();
    UpdateAliasSelectList();
    UpdatePartSelectList();

    m_editPinsPerPartOrConvert = new_part->UnitsLocked() ? true : false;
    m_lastDrawItem = NULL;

    GetScreen()->ClearUndoRedoList();
    OnModify();

    m_canvas->Refresh();
    m_mainToolBar->Refresh();
}

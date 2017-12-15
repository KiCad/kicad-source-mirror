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
#include <lib_manager.h>
#include <cmp_tree_pane.h>
#include <component_tree.h>

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

        if( wxFileName::FileExists( fileName ) && !wxFileName::IsFileWritable( fileName ) )
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

        msg.Printf( _( "Error occurred loading symbol \"%s\" from library \"%s\"." ),
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


bool LIB_EDIT_FRAME::LoadOneLibraryPartAux( LIB_ALIAS* aEntry, const wxString& aLibrary )
{
    wxString msg, rootName;

    if( !aEntry || aLibrary.empty() )
        return false;

    if( aEntry->GetName().IsEmpty() )
    {
        wxLogWarning( "Symbol in library \"%s\" has empty name field.", aLibrary );
        return false;
    }

    m_aliasName = aEntry->GetName();

    LIB_PART* lib_part = m_libMgr->GetBufferedPart( m_aliasName, aLibrary );
    wxASSERT( lib_part );
    SetScreen( m_libMgr->GetScreen( lib_part->GetName(), aLibrary ) );
    SetCurPart( new LIB_PART( *lib_part ) );
    SetCurLib( aLibrary );

    m_unit = 1;
    m_convert = 1;
    SetShowDeMorgan( GetCurPart()->HasConversion() );

    Zoom_Automatique( false );
    DisplayLibInfos();
    UpdateAliasSelectList();
    UpdatePartSelectList();

    // Display the document information based on the entry selected just in
    // case the entry is an alias.
    DisplayCmpDoc();
    Refresh();

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


void LIB_EDIT_FRAME::OnSaveLibrary( wxCommandEvent& event )
{
    saveLibrary( getTargetLib(), event.GetId() == ID_LIBEDIT_SAVE_LIBRARY_AS );
    m_treePane->Refresh();
}


void LIB_EDIT_FRAME::OnSaveAllLibraries( wxCommandEvent& event )
{
    saveAllLibraries();
}


void LIB_EDIT_FRAME::OnRevertLibrary( wxCommandEvent& aEvent )
{
    wxString libName = getTargetLib();
    bool currentLib = ( libName == GetCurLib() );

    // Save the current part name/unit to reload after revert
    wxString alias = m_aliasName;
    int unit = m_unit;

    if( !IsOK( this, _( "The revert operation cannot be undone!\n\nRevert changes?" ) ) )
        return;

    if( currentLib )
        emptyScreen();

    m_libMgr->RevertLibrary( libName );

    if( currentLib && m_libMgr->PartExists( alias, libName ) )
        loadPart( alias, libName, unit );
}


void LIB_EDIT_FRAME::OnCreateNewPart( wxCommandEvent& event )
{
    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );
    m_drawItem = NULL;
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
    m_aliasName = name;
    new_part.GetReferenceField().SetText( dlg.GetReference() );
    new_part.SetUnitCount( dlg.GetUnitCount() );

    // Initialize new_part.m_TextInside member:
    // if 0, pin text is outside the body (on the pin)
    // if > 0, pin text is inside the body
    new_part.SetConversion( dlg.GetAlternateBodyStyle() );
    SetShowDeMorgan( dlg.GetAlternateBodyStyle() );

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
    loadPart( name, lib, 1 );
}


void LIB_EDIT_FRAME::OnEditPart( wxCommandEvent& aEvent )
{
    int unit = 0;
    LIB_ID partId = m_treePane->GetCmpTree()->GetSelectedLibId( &unit );
    loadPart( partId.GetLibItemName(), partId.GetLibNickname(), unit );
}


void LIB_EDIT_FRAME::OnSavePart( wxCommandEvent& aEvent )
{
    LIB_ID libId = getTargetLibId();

    if( m_libMgr->FlushPart( libId.GetLibItemName(), libId.GetLibNickname() ) )
        m_libMgr->ClearPartModified( libId.GetLibItemName(), libId.GetLibNickname() );

    m_treePane->Refresh();
}


void LIB_EDIT_FRAME::OnRemovePart( wxCommandEvent& aEvent )
{
    LIB_ID libId = getTargetLibId();

    if( m_libMgr->IsPartModified( libId.GetLibItemName(), libId.GetLibNickname() )
        && !IsOK( this, _( wxString::Format( "Component %s has been modified\n"
                        "Do you want to remove it from the library?", libId.GetLibItemName().c_str() ) ) ) )
    {
        return;
    }

    if( isCurrentPart( libId ) )
        emptyScreen();

    m_libMgr->RemovePart( libId.GetLibItemName(), libId.GetLibNickname() );
}


void LIB_EDIT_FRAME::OnCopyCutPart( wxCommandEvent& aEvent )
{
    int unit = 0;
    LIB_ID partId = m_treePane->GetCmpTree()->GetSelectedLibId( &unit );
    LIB_PART* part = m_libMgr->GetBufferedPart( partId.GetLibItemName(), partId.GetLibNickname() );

    if( !part )
        return;

    LIB_ID libId = getTargetLibId();
    m_copiedPart.reset( new LIB_PART( *part ) );

    if( aEvent.GetId() == ID_LIBEDIT_CUT_PART )
    {
        if( isCurrentPart( libId ) )
            emptyScreen();

        m_libMgr->RemovePart( libId.GetLibItemName(), libId.GetLibNickname() );
    }
}


void LIB_EDIT_FRAME::OnPasteDuplicatePart( wxCommandEvent& aEvent )
{
    int unit = 0;
    LIB_ID libId = m_treePane->GetCmpTree()->GetSelectedLibId( &unit );
    wxString lib = libId.GetLibNickname();

    if( !m_libMgr->LibraryExists( lib ) )
        return;

    LIB_PART* srcPart = nullptr;

    if( aEvent.GetId() == ID_LIBEDIT_DUPLICATE_PART )
        srcPart = m_libMgr->GetBufferedPart( libId.GetLibItemName(), lib );
    else if( aEvent.GetId() == ID_LIBEDIT_PASTE_PART )
        srcPart = m_copiedPart.get();
    else
        wxFAIL;

    if( !srcPart )
        return;

    LIB_PART newPart( *srcPart );
    fixDuplicateAliases( &newPart, lib );
    m_libMgr->UpdatePart( &newPart, lib );
    m_treePane->GetCmpTree()->SelectLibId( LIB_ID( lib, newPart.GetName() ) );
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
            newName = wxString::Format( "%s_%d", alias->GetName(), sfx );
            ++sfx;
        }

        if( i == 0 )
            aPart->SetName( newName );
        else
            alias->SetName( newName );
    }
}


void LIB_EDIT_FRAME::OnRevertPart( wxCommandEvent& aEvent )
{
    LIB_ID libId = getTargetLibId();
    bool currentPart = isCurrentPart( libId );
    int unit = m_unit;

    if( currentPart )
        emptyScreen();

    if( m_libMgr->RevertPart( libId.GetLibItemName(), libId.GetLibNickname() ) )
        m_libMgr->ClearPartModified( libId.GetLibItemName(), libId.GetLibNickname() );

    if( currentPart && m_libMgr->PartExists( libId.GetLibItemName(), libId.GetLibNickname() ) )
        loadPart( libId.GetLibItemName(), libId.GetLibNickname(), unit );
}


void LIB_EDIT_FRAME::loadPart( const wxString& aAlias, const wxString& aLibrary, int aUnit )
{
    wxCHECK( m_libMgr->PartExists( aAlias, aLibrary ), /* void */ );
    LIB_PART* part = m_libMgr->GetBufferedPart( aAlias, aLibrary );
    LIB_ALIAS* alias = part ? part->GetAlias( aAlias ) : nullptr;

    if( !alias )
    {
        wxString msg = wxString::Format( _( "Part name \"%s\" not found in library \"%s\"" ),
            GetChars( aAlias ), GetChars( aLibrary ) );
        DisplayError( this, msg );
        return;
    }

    m_lastDrawItem = m_drawItem = nullptr;
    m_aliasName = aAlias;
    m_unit = ( aUnit <= part->GetUnitCount() ? aUnit : 1 );

    LoadOneLibraryPartAux( alias, aLibrary );
}


bool LIB_EDIT_FRAME::saveLibrary( const wxString& aLibrary, bool aNewFile )
{
    wxFileName fn;
    wxString   msg;
    PROJECT&   prj = Prj();

    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );

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
    UpdateAliasSelectList();
    UpdatePartSelectList();
    refreshSchematic();

    return true;
}


bool LIB_EDIT_FRAME::saveAllLibraries()
{
    wxArrayString unsavedLibraries;
    // There are two stages: first try to save libraries to the original files.
    // In case of problems, ask the user to save them in a new location.
    bool firstRun = true;
    bool allSaved = false;

    while( !allSaved )
    {
        allSaved = true;
        unsavedLibraries.Empty();

        for( const auto& lib : m_libMgr->GetLibraryNames() )
        {
            if( m_libMgr->IsLibraryModified( lib ) )
                unsavedLibraries.Add( lib );
        }

        if( !unsavedLibraries.IsEmpty() )
        {
            auto res = SelectMultipleOptions( this, _( "Save Libraries" ),
                    firstRun ? _( "Select libraries to save before closing" )
                             : _( "Some libraries could not be saved to their original files.\n\n"
                                  "Do you want to save them to a new file?" ),
                    unsavedLibraries, true );

            if( !res.first )
                return false;       // dialog has been cancelled

            for( auto libIndex : res.second )
                allSaved &= saveLibrary( unsavedLibraries[libIndex], !firstRun );

            firstRun = false;
        }
    }

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

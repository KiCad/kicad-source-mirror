/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008-2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 2004-2013 KiCad Developers, see change_log.txt for contributors.
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
#include <protos.h>
#include <libeditframe.h>
#include <class_library.h>
#include <template_fieldnames.h>
#include <wildcards_and_files_ext.h>

#include <dialogs/dialog_lib_new_component.h>


void LIB_EDIT_FRAME::DisplayLibInfos()
{
    wxString msg = _( "Component Library Editor: " );

    EnsureActiveLibExists();

    if( m_library )
    {
        msg += m_library->GetFullFileName();

        if( m_library->IsReadOnly() )
            msg += _( " [Read Only]" );
    }
    else
    {
        msg += _( "no library selected" );
    }

    SetTitle( msg );
}


void LIB_EDIT_FRAME::SelectActiveLibrary( CMP_LIBRARY* aLibrary )
{
    if( aLibrary == NULL )
        aLibrary = SelectLibraryFromList( this );

    if( aLibrary )
    {
        m_library = aLibrary;
    }

    DisplayLibInfos();
}


bool LIB_EDIT_FRAME::LoadComponentAndSelectLib( LIB_ALIAS* aLibEntry, CMP_LIBRARY* aLibrary )
{
    if( GetScreen()->IsModify()
        && !IsOK( this, _( "The current component is not saved.\n\nDiscard current changes?" ) ) )
        return false;

    SelectActiveLibrary( aLibrary );
    return LoadComponentFromCurrentLib( aLibEntry );
}


bool LIB_EDIT_FRAME::LoadComponentFromCurrentLib( LIB_ALIAS* aLibEntry )
{
    if( !LoadOneLibraryPartAux( aLibEntry, m_library ) )
        return false;

    m_editPinsPerPartOrConvert = m_component->UnitsLocked() ? true : false;

    GetScreen()->ClearUndoRedoList();
    Zoom_Automatique( false );
    SetShowDeMorgan( m_component->HasConversion() );

    return true;
}


void LIB_EDIT_FRAME::LoadOneLibraryPart( wxCommandEvent& event )
{
    wxString   msg;
    wxString   CmpName;
    LIB_ALIAS* libEntry = NULL;

    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );

    if( GetScreen()->IsModify()
        && !IsOK( this, _( "The current component is not saved.\n\nDiscard current changes?" ) ) )
        return;

     // No current lib, ask user for the library to use.
    if( m_library == NULL )
    {
        SelectActiveLibrary();

        if( m_library == NULL )
            return;
    }

    wxArrayString dummyHistoryList;
    int dummyLastUnit;
    CmpName = SelectComponentFromLibrary( m_library->GetName(), dummyHistoryList, dummyLastUnit,
                                          true, NULL, NULL );

    if( CmpName.IsEmpty() )
        return;

    GetScreen()->ClrModify();
    m_lastDrawItem = m_drawItem = NULL;

    // Delete previous library component, if any
    if( m_component )
    {
        delete m_component;
        m_component = NULL;
        m_aliasName.Empty();
    }

    /* Load the new library component */
    libEntry = m_library->FindEntry( CmpName );
    CMP_LIBRARY* searchLib = m_library;

    if( libEntry == NULL )
    {   // Not found in the active library: search inside the full list
        // (can happen when using Viewlib to load a component)
        libEntry = CMP_LIBRARY::FindLibraryEntry( CmpName );

        if( libEntry )
        {
            searchLib = libEntry->GetLibrary();
            // The entry to load is not in the active lib
            // Ask for a new active lib
            wxString msg;
            msg << _("The selected component is not in the active library");
            msg << wxT("\n\n");
            msg << _("Do you want to change the active library?");

            if( IsOK( this, msg ) )
                SelectActiveLibrary( searchLib );
        }
    }

    if( libEntry == NULL )
    {
        msg.Printf( _( "Component name %s not found in library %s" ),
                    GetChars( CmpName ),
                    GetChars( searchLib->GetName() ) );
        DisplayError( this, msg );
        return;
    }

    EXCHG( searchLib, m_library );
    LoadComponentFromCurrentLib( libEntry );
    EXCHG( searchLib, m_library );
    DisplayLibInfos();
}


bool LIB_EDIT_FRAME::LoadOneLibraryPartAux( LIB_ALIAS* aEntry, CMP_LIBRARY* aLibrary )
{
    wxString msg, cmpName, rootName;
    LIB_COMPONENT* component;

    if( ( aEntry == NULL ) || ( aLibrary == NULL ) )
        return false;

    if( aEntry->GetName().IsEmpty() )
    {
        wxLogWarning( wxT( "Entry in library <%s> has empty name field." ),
                      GetChars( aLibrary->GetName() ) );
        return false;
    }

    cmpName = m_aliasName = aEntry->GetName();

    LIB_ALIAS* alias = (LIB_ALIAS*) aEntry;
    component = alias->GetComponent();

    wxASSERT( component != NULL );

    wxLogDebug( wxT( "\"<%s>\" is alias of \"<%s>\"" ),
                GetChars( cmpName ),
                GetChars( component->GetName() ) );

    if( m_component )
    {
        delete m_component;
        m_aliasName.Empty();
    }

    m_component = new LIB_COMPONENT( *component );

    if( m_component == NULL )
    {
        msg.Printf( _( "Could not create copy of component <%s> in library <%s>." ),
                    GetChars( aEntry->GetName() ),
                    GetChars( aLibrary->GetName() ) );
        DisplayError( this, msg );
        return false;
    }

    m_aliasName = aEntry->GetName();
    m_unit = 1;
    m_convert = 1;

    m_showDeMorgan = false;

    if( m_component->HasConversion() )
        m_showDeMorgan = true;

    GetScreen()->ClrModify();
    DisplayLibInfos();
    UpdateAliasSelectList();
    UpdatePartSelectList();

    /* Display the document information based on the entry selected just in
     * case the entry is an alias. */
    DisplayCmpDoc();

    return true;
}


void LIB_EDIT_FRAME::RedrawComponent( wxDC* aDC, wxPoint aOffset  )
{
    if( m_component )
    {
        // display reference like in schematic (a reference U is shown U? or U?A)
        // although it is stored without ? and part id.
        // So temporary change the reference by a schematic like reference
        LIB_FIELD* field = m_component->GetField( REFERENCE );
        wxString fieldText = field->GetText();
        wxString fieldfullText = field->GetFullText( m_unit );
        field->EDA_TEXT::SetText( fieldfullText );  // change the field text string only
        m_component->Draw( m_canvas, aDC, aOffset, m_unit, m_convert, GR_DEFAULT_DRAWMODE );
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
    bool newFile = false;
    if( event.GetId() == ID_LIBEDIT_SAVE_CURRENT_LIB_AS )
        newFile = true;

    this->SaveActiveLibrary( newFile );
}


bool LIB_EDIT_FRAME::SaveActiveLibrary( bool newFile )
{
    wxFileName fn;
    wxString   msg;

    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );

    if( !m_library )
    {
        DisplayError( this, _( "No library specified." ) );
        return false;
    }

    if( GetScreen()->IsModify() )
    {
        if( IsOK( this, _( "Include last component changes?" ) ) )
            SaveOnePartInMemory();
    }

    if( newFile )
    {
        PROJECT&        prj = Prj();
        SEARCH_STACK&   search = prj.SchSearchS();

        // Get a new name for the library
        wxString default_path = prj.GetRString( PROJECT::SCH_LIB_PATH );
        if( !default_path )
            default_path = search.LastVisitedPath();

        wxFileDialog dlg( this, _( "Component Library Name:" ), default_path,
                          wxEmptyString, SchematicLibraryFileExtension,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        fn = dlg.GetPath();

        /* The GTK file chooser doesn't return the file extension added to
         * file name so add it here. */
        if( fn.GetExt().IsEmpty() )
            fn.SetExt( SchematicLibraryFileExtension );

        prj.SetRString( PROJECT::SCH_LIB_PATH, fn.GetPath() );
    }
    else
    {
        fn = wxFileName( m_library->GetFullFileName() );

        msg.Printf( _( "Modify library file <%s> ?" ),
                    GetChars( fn.GetFullPath() ) );

        if( !IsOK( this, msg ) )
            return false;
    }

    // Verify the user has write privileges before attempting to save the library file.
    if( !IsWritable( fn ) )
        return false;

    ClearMsgPanel();

    wxFileName libFileName = fn;
    wxFileName backupFileName = fn;

    // Rename the old .lib file to .bak.
    if( libFileName.FileExists() )
    {
        backupFileName.SetExt( wxT( "bak" ) );
        if( backupFileName.FileExists() )
            wxRemoveFile( backupFileName.GetFullPath() );

        if( !wxRenameFile( libFileName.GetFullPath(), backupFileName.GetFullPath() ) )
        {
            libFileName.MakeAbsolute();
            msg = wxT( "Failed to rename old component library file " ) +
                  backupFileName.GetFullPath();
            DisplayError( this, msg );
        }
    }

    try
    {
        FILE_OUTPUTFORMATTER    libFormatter( libFileName.GetFullPath() );

        if( !m_library->Save( libFormatter ) )
        {
            msg.Printf( _( "Error occurred while saving library file <%s>" ),
                        GetChars( fn.GetFullPath() ) );
            AppendMsgPanel( _( "*** ERROR: ***" ), msg, RED );
            DisplayError( this, msg );
            return false;
        }
    }
    catch( ... /* IO_ERROR ioe */ )
    {
        libFileName.MakeAbsolute();
        msg.Printf( _( "Failed to create component library file <%s>" ),
                    GetChars( libFileName.GetFullPath() ) );
        DisplayError( this, msg );
        return false;
    }

    wxFileName docFileName = libFileName;

    docFileName.SetExt( DOC_EXT );

    // Rename .doc file to .bck.
    if( docFileName.FileExists() )
    {
        backupFileName.SetExt( wxT( "bck" ) );

        if( backupFileName.FileExists() )
            wxRemoveFile( backupFileName.GetFullPath() );

        if( !wxRenameFile( docFileName.GetFullPath(), backupFileName.GetFullPath() ) )
        {
            msg = wxT( "Failed to save old library document file " ) +
                  backupFileName.GetFullPath();
            DisplayError( this, msg );
        }
    }

    try
    {
        FILE_OUTPUTFORMATTER    docFormatter( docFileName.GetFullPath() );

        if( !m_library->SaveDocs( docFormatter ) )
        {
            msg.Printf( _( "Error occurred while saving library documentation file <%s>" ),
                        GetChars( docFileName.GetFullPath() ) );
            AppendMsgPanel( _( "*** ERROR: ***" ), msg, RED );
            DisplayError( this, msg );
            return false;
        }
    }
    catch( ... /* IO_ERROR ioe */ )
    {
        docFileName.MakeAbsolute();
        msg.Printf( _( "Failed to create component document library file <%s>" ),
                    GetChars( docFileName.GetFullPath() ) );
        DisplayError( this, msg );
        return false;
    }

    msg.Printf( _( "Library file <%s> OK" ), GetChars( fn.GetFullName() ) );
    fn.SetExt( DOC_EXT );
    wxString msg1;
    msg1.Printf( _( "Documentation file <%s> OK" ), GetChars( fn.GetFullPath() ) );
    AppendMsgPanel( msg, msg1, BLUE );

    return true;
}


void LIB_EDIT_FRAME::DisplayCmpDoc()
{
    wxString msg;
    LIB_ALIAS* alias;

    ClearMsgPanel();

    if( m_library == NULL || m_component == NULL )
        return;

    msg = m_component->GetName();

    AppendMsgPanel( _( "Name" ), msg, BLUE, 8 );

    if( m_aliasName == m_component->GetName() )
        msg = _( "None" );
    else
        msg = m_aliasName;

    alias = m_component->GetAlias( m_aliasName );

    wxCHECK_RET( alias != NULL, wxT( "Alias not found in component." ) );

    AppendMsgPanel( _( "Alias" ), msg, RED, 8 );

    static wxChar UnitLetter[] = wxT( "?ABCDEFGHIJKLMNOPQRSTUVWXYZ" );
    msg = UnitLetter[m_unit];

    AppendMsgPanel( _( "Unit" ), msg, BROWN, 8 );

    if( m_convert > 1 )
        msg = _( "Convert" );
    else
        msg = _( "Normal" );

    AppendMsgPanel( _( "Body" ), msg, GREEN, 8 );

    if( m_component->IsPower() )
        msg = _( "Power Symbol" );
    else
        msg = _( "Component" );

    AppendMsgPanel( _( "Type" ), msg, MAGENTA, 8 );
    AppendMsgPanel( _( "Description" ), alias->GetDescription(), CYAN, 8 );
    AppendMsgPanel( _( "Key words" ), alias->GetKeyWords(), DARKDARKGRAY );
    AppendMsgPanel( _( "Datasheet" ), alias->GetDocFileName(), DARKDARKGRAY );
}


void LIB_EDIT_FRAME::DeleteOnePart( wxCommandEvent& event )
{
    wxString      CmpName;
    LIB_ALIAS*    LibEntry;
    wxArrayString ListNames;
    wxString      msg;

    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );

    m_lastDrawItem = NULL;
    m_drawItem = NULL;

    if( m_library == NULL )
    {
        SelectActiveLibrary();

        if( m_library == NULL )
        {
            DisplayError( this, _( "Please select a component library." ) );
            return;
        }
    }

    m_library->GetEntryNames( ListNames );

    if( ListNames.IsEmpty() )
    {
        msg.Printf( _( "Component library <%s> is empty." ), GetChars( m_library->GetName() ) );
        wxMessageBox( msg, _( "Delete Entry Error" ), wxID_OK | wxICON_EXCLAMATION, this );
        return;
    }

    msg.Printf( _( "Select 1 of %d components to delete\nfrom library <%s>." ),
                ListNames.GetCount(),
                GetChars( m_library->GetName() ) );

    wxSingleChoiceDialog dlg( this, msg, _( "Delete Component" ), ListNames );

    if( dlg.ShowModal() == wxID_CANCEL || dlg.GetStringSelection().IsEmpty() )
        return;

    LibEntry = m_library->FindEntry( dlg.GetStringSelection() );

    if( LibEntry == NULL )
    {
        msg.Printf( _( "Entry <%s> not found in library <%s>." ),
                    GetChars( dlg.GetStringSelection() ),
                    GetChars( m_library->GetName() ) );
        DisplayError( this, msg );
        return;
    }

    msg.Printf( _( "Delete component %s from library %s?" ),
                GetChars( LibEntry->GetName() ),
                GetChars( m_library->GetName() ) );

    if( !IsOK( this, msg ) )
        return;

    if( m_component == NULL || !m_component->HasAlias( LibEntry->GetName() ) )
    {
        m_library->RemoveEntry( LibEntry );
        return;
    }

    /* If deleting the current entry or removing one of the aliases for
     * the current entry, sync the changes in the current entry as well.
     */

    if( GetScreen()->IsModify()
        && !IsOK( this, _( "The component being deleted has been modified. \
All changes will be lost. Discard changes?" ) ) )
        return;

    LIB_ALIAS* nextEntry = m_library->RemoveEntry( LibEntry );

    if( nextEntry != NULL )
    {
        if( LoadOneLibraryPartAux( nextEntry, m_library ) )
            Zoom_Automatique( false );
    }
    else
    {
        delete m_component;
        m_component = NULL;
        m_aliasName.Empty();
    }

    m_canvas->Refresh();
}



void LIB_EDIT_FRAME::CreateNewLibraryPart( wxCommandEvent& event )
{
    wxString name;

    if( m_component && GetScreen()->IsModify()
        && !IsOK( this, _( "All changes to the current component will be \
lost!\n\nClear the current component from the screen?" ) ) )
        return;

    m_canvas->EndMouseCapture( ID_NO_TOOL_SELECTED, m_canvas->GetDefaultCursor() );

    m_drawItem = NULL;

    DIALOG_LIB_NEW_COMPONENT dlg( this );
    dlg.SetMinSize( dlg.GetSize() );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    if( dlg.GetName().IsEmpty() )
    {
        wxMessageBox( _( "This new component has no name and cannot be created. Aborted" ) );
        return;
    }

#ifndef KICAD_KEEPCASE
    name = dlg.GetName().MakeUpper();
#else
    name = dlg.GetName();
#endif
    name.Replace( wxT( " " ), wxT( "_" ) );

    /* Test if there a component with this name already. */
    if( m_library && m_library->FindEntry( name ) )
    {
        wxString msg;
        msg.Printf( _( "Component %s already exists in library %s" ),
                    GetChars( name ),
                    GetChars( m_library->GetName() ) );
        DisplayError( this, msg );
        return;
    }

    LIB_COMPONENT* component = new LIB_COMPONENT( name );
    component->GetReferenceField().SetText( dlg.GetReference() );
    component->SetPartCount( dlg.GetPartCount() );

    // Initialize component->m_TextInside member:
    // if 0, pin text is outside the body (on the pin)
    // if > 0, pin text is inside the body
    component->SetConversion( dlg.GetAlternateBodyStyle() );
    SetShowDeMorgan( dlg.GetAlternateBodyStyle() );

    if( dlg.GetPinNameInside() )
    {
        component->SetPinNameOffset( dlg.GetPinTextPosition() );

        if( component->GetPinNameOffset() == 0 )
            component->SetPinNameOffset( 1 );
    }
    else
    {
        component->SetPinNameOffset( 0 );
    }

    ( dlg.GetPowerSymbol() ) ? component->SetPower() : component->SetNormal();
    component->SetShowPinNumbers( dlg.GetShowPinNumber() );
    component->SetShowPinNames( dlg.GetShowPinName() );
    component->LockUnits( dlg.GetLockItems() );

    if( dlg.GetPartCount() < 2 )
        component->LockUnits( false );

    m_aliasName = component->GetName();

    if( m_component )
    {
        delete m_component;
        m_aliasName.Empty();
    }

    m_component = component;
    m_aliasName = m_component->GetName();
    m_unit = 1;
    m_convert  = 1;
    DisplayLibInfos();
    DisplayCmpDoc();
    UpdateAliasSelectList();
    UpdatePartSelectList();
    m_editPinsPerPartOrConvert = m_component->UnitsLocked() ? true : false;
    m_lastDrawItem = NULL;
    GetScreen()->ClearUndoRedoList();
    OnModify();
    m_canvas->Refresh();
    m_mainToolBar->Refresh();
}


void LIB_EDIT_FRAME::SaveOnePartInMemory()
{
    LIB_COMPONENT* oldComponent;
    LIB_COMPONENT* component;
    wxString       msg;

    if( m_component == NULL )
    {
        DisplayError( this, _( "No component to save." ) );
        return;
    }

    if( m_library == NULL )
        SelectActiveLibrary();

    if( m_library == NULL )
    {
        DisplayError( this, _( "No library specified." ) );
        return;
    }

    GetScreen()->ClrModify();

    oldComponent = m_library->FindComponent( m_component->GetName() );

    if( oldComponent != NULL )
    {
        msg.Printf( _( "Component %s already exists. Change it?" ),
                    GetChars( m_component->GetName() ) );

        if( !IsOK( this, msg ) )
            return;
    }

    m_drawItem = m_lastDrawItem = NULL;

    if( oldComponent != NULL )
        component = m_library->ReplaceComponent( oldComponent, m_component );
    else
        component = m_library->AddComponent( m_component );

    if( component == NULL )
        return;

    msg.Printf( _( "Component %s saved in library %s" ),
                GetChars( component->GetName() ),
                GetChars( m_library->GetName() ) );
    SetStatusText( msg );
}

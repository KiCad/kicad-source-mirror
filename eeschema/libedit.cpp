/****************************/
/*  EESchema - libedit.cpp  */
/****************************/

#include "fctsys.h"
#include "gr_basic.h"
#include "common.h"
#include "appl_wxstruct.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "gestfich.h"
#include "eeschema_id.h"

#include "program.h"
#include "general.h"
#include "protos.h"
#include "libeditframe.h"
#include "class_library.h"

#include "dialog_lib_new_component.h"


/* Update the main window title bar with the current library name. */
void WinEDA_LibeditFrame::DisplayLibInfos()
{
    wxString msg = _( "Component Library Editor: " );

    EnsureActiveLibExists();

    if( m_library )
        msg += m_library->GetFullFileName();
    else
        msg += _( "no library selected" );

    SetTitle( msg );
}


/* Function to select the current library (working library) */
void WinEDA_LibeditFrame::SelectActiveLibrary()
{
    CMP_LIBRARY* Lib;

    Lib = SelectLibraryFromList( this );
    if( Lib )
    {
        m_library = Lib;
    }
    DisplayLibInfos();
}


/**
 * Function LoadOneLibraryPart()
 * load a library component from the current selected library
 * Prompt user for component name
 * If there is no current selected library,
 * prompt user for library name and make the selected library the current lib.
 */
void WinEDA_LibeditFrame::LoadOneLibraryPart( wxCommandEvent& event )
{
    int            i;
    wxString       msg;
    wxString       CmpName;
    CMP_LIB_ENTRY* LibEntry = NULL;

    DrawPanel->UnManageCursor( 0, wxCURSOR_ARROW );

    if( GetBaseScreen()->IsModify() && !IsOK( this, _( "Current part not \
saved.\n\nDiscard current changes?" ) ) )
        return;

     // No current lib, ask user for the library to use.
    if( m_library == NULL )
    {
        SelectActiveLibrary();
        if( m_library == NULL )
            return;
    }

    i = GetNameOfPartToLoad( this, m_library, CmpName );
    if( i == 0 )
        return;

    GetBaseScreen()->ClrModify();
    m_lastDrawItem = m_drawItem = NULL;

    // Delete previous library component, if any
    if( m_component )
    {
        SAFE_DELETE( m_component );
        m_aliasName.Empty();
    }

    /* Load the new library component */
    LibEntry = m_library->FindEntry( CmpName );

    if( LibEntry == NULL )
    {
        msg.Printf( _( "Component or alias name \"%s\" not found in library \"%s\"." ),
                    GetChars( CmpName ),
                    GetChars( m_library->GetName() ) );
        DisplayError( this, msg );
        return;
    }

    if( !LoadOneLibraryPartAux( LibEntry, m_library ) )
        return;

    g_EditPinByPinIsOn = m_component->UnitsLocked() ? true : false;
    m_HToolBar->ToggleTool( ID_LIBEDIT_EDIT_PIN_BY_PIN, g_EditPinByPinIsOn );

    GetScreen()->ClearUndoRedoList();
    Zoom_Automatique( false );
    DrawPanel->Refresh();
    SetShowDeMorgan( m_component->HasConversion() );
    m_HToolBar->Refresh();
}


/*
 * Routine to load into memory a copy of 1 library part.
 * Returns
 * 0 if OK
 * 1 if error
 * m_component advanced copy and created
 */
bool WinEDA_LibeditFrame::LoadOneLibraryPartAux( CMP_LIB_ENTRY* aEntry, CMP_LIBRARY* aLibrary )
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

    if( aEntry->isAlias() )
    {
        LIB_ALIAS* alias = (LIB_ALIAS*) aEntry;
        component = alias->GetComponent();

        wxASSERT( component != NULL && component->isComponent() );

        wxLogDebug( wxT( "\"<%s>\" is alias of \"<%s>\"" ),
                    GetChars( cmpName ),
                    GetChars( component->GetName() ) );
    }
    else
    {
        component = (LIB_COMPONENT*) aEntry;
    }

    if( m_component )
    {
        SAFE_DELETE( m_component );
        m_aliasName.Empty();
        m_oldRootName.Empty();
    }

    m_component = new LIB_COMPONENT( *component );

    if( m_component == NULL )
    {
        msg.Printf( _( "Could not create copy of part <%s> in library <%s>." ),
                    GetChars( aEntry->GetName() ),
                    GetChars( aLibrary->GetName() ) );
        DisplayError( this, msg );
        return false;
    }

    m_oldRootName = m_aliasName = aEntry->GetName();
    m_unit = 1;
    m_convert = 1;

    m_showDeMorgan = false;

    if( m_component->HasConversion() )
        m_showDeMorgan = true;

    GetBaseScreen()->ClrModify();
    DisplayLibInfos();
    UpdateAliasSelectList();
    UpdatePartSelectList();

    /* Display the document information based on the entry selected just in
     * case the entry is an alias. */
    DisplayCmpDoc();

    return true;
}


/* Function to redraw the current loaded library component */
void WinEDA_LibeditFrame::RedrawActiveWindow( wxDC* DC, bool EraseBg )
{
    if( GetScreen() == NULL )
        return;

    ActiveScreen = GetScreen();

    DrawPanel->DrawBackGround( DC );

    if( m_component )
    {
        // display reference like in schematic (a reference U is shown U? or U?A)
        // although it is stored without ? and part id.
        // So temporary change the reference by a schematic like reference
        LIB_FIELD* Field = m_component->GetField( REFERENCE );
        wxString fieldText = Field->m_Text;
        wxString fieldfullText = Field->GetFullText( m_unit );
        Field->m_Text = fieldfullText;
        m_component->Draw( DrawPanel, DC, wxPoint( 0, 0 ), m_unit,
                           m_convert, GR_DEFAULT_DRAWMODE );
        Field->m_Text = fieldText;
    }

    GetScreen()->ClrRefreshReq();

    if( DrawPanel->ManageCurseur )
        DrawPanel->ManageCurseur( DrawPanel, DC, false );

    DrawPanel->DrawCursor( DC );

    DisplayLibInfos();
    UpdateStatusBar();
}


/*
 * Save (on disk) the current library
 *  if exists the old file is renamed (.bak)
 */
void WinEDA_LibeditFrame::SaveActiveLibrary( wxCommandEvent& event )
{
    wxFileName fn;
    wxString   msg;

    DrawPanel->UnManageCursor( 0, wxCURSOR_ARROW );

    if( GetScreen()->IsModify() )
    {
        if( IsOK( this, _( "Include last component changes?" ) ) )
            SaveOnePartInMemory();
    }

    if( m_library == NULL )
    {
        DisplayError( this, wxT( "No library specified." ) );
        return;
    }

    if( event.GetId() == ID_LIBEDIT_SAVE_CURRENT_LIB_AS )
    {   // Get a new name for the library
        wxString default_path = wxGetApp().ReturnLastVisitedLibraryPath();
        wxFileDialog dlg( this, _( "Component Library Name:" ), default_path,
                          wxEmptyString, CompLibFileWildcard,
                          wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

        if( dlg.ShowModal() == wxID_CANCEL )
            return;

        fn = dlg.GetPath();

        /* The GTK file chooser doesn't return the file extension added to
         * file name so add it here. */
        if( fn.GetExt().IsEmpty() )
            fn.SetExt( SymbolFileExtension );

        wxGetApp().SaveLastVisitedLibraryPath( fn.GetPath() );
    }
    else
    {
        fn = wxFileName( m_library->GetFullFileName() );

        msg = _( "Modify library file \"" ) + fn.GetFullPath() + _( "\"?" );

        if( !IsOK( this, msg ) )
            return;
    }

    bool success = m_library->Save( fn.GetFullPath(), true );

    ClearMsgPanel();

    if( !success )
    {
        msg = _( "Error while saving library file \"" ) + fn.GetFullPath() + _( "\"." );
        AppendMsgPanel( _( "*** ERROR: ***" ), msg, RED );
        DisplayError( this, msg );
    }
    else
    {
        msg = _( "Library file \"" ) + fn.GetFullName() + wxT( "\" Ok" );
        fn.SetExt( DOC_EXT );
        wxString msg1 = _( "Document file \"" ) + fn.GetFullPath() + wxT( "\" Ok" );
        AppendMsgPanel( msg, msg1, BLUE );
    }
}


/*
 * Display the documentation of the selected component.
 *
 * Used when displaying the list of library components.
 */
void WinEDA_LibeditFrame::DisplayCmpDoc()
{
    wxString msg;
    LIB_ALIAS* alias;

    ClearMsgPanel();

    if( m_library == NULL || m_component == NULL )
        return;

    msg = m_component->GetName();

    AppendMsgPanel( _( "Part" ), msg, BLUE, 8 );

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


/*
 * Delete component in the current library.
 *
 * (Delete only in memory, the file does not change)
 *
 * The entry can be an alias or a component.
 * If an alias:
 *   It is removed, and the list of alias is updated.
 *
 * If a component:
 *   If the list of aliases is zero, it deletes the component
 *   Otherwise the alias becomes the new component name, and the other
 *   aliases become dependent on newly named component.
 */
void WinEDA_LibeditFrame::DeleteOnePart( wxCommandEvent& event )
{
    wxString       CmpName;
    CMP_LIB_ENTRY* LibEntry;
    wxArrayString  ListNames;
    wxString       msg;

    DrawPanel->UnManageCursor( 0, wxCURSOR_ARROW );

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

    msg.Printf( _( "Delete component \"%s\" from library \"%s\"?" ),
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

    CMP_LIB_ENTRY* nextEntry = m_library->RemoveEntry( LibEntry );

    if( nextEntry != NULL )
    {
        if( LoadOneLibraryPartAux( nextEntry, m_library ) )
            Zoom_Automatique( false );
    }
    else
    {
        SAFE_DELETE( m_component );
        m_aliasName.Empty();
        m_oldRootName.Empty();
    }

    DrawPanel->Refresh();
}



/*
 * Routine to create a new library component
 *
 * If an old component is currently in edit, it is deleted.
 */
void WinEDA_LibeditFrame::CreateNewLibraryPart( wxCommandEvent& event )
{
    wxString name;

    if( m_component && GetScreen()->IsModify()
        && !IsOK( this, _( "All changes to the current component will be \
lost!\n\nClear the current component from the screen?" ) ) )
        return;

    DrawPanel->UnManageCursor( 0, wxCURSOR_ARROW );

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
        msg.Printf( _( "Component \"%s\" already exists in library \"%s\"." ),
                    GetChars( name ),
                    GetChars( m_library->GetName() ) );
        DisplayError( this, msg );
        return;
    }

    m_oldRootName.Empty();
    LIB_COMPONENT* component = new LIB_COMPONENT( name );
    component->GetReferenceField().m_Text = dlg.GetReference();
    component->SetPartCount( dlg.GetPartCount() );
    // Initialize component->m_TextInside member:
    // if 0, pin text is outside the body (on the pin)
    // if > 0, pin text is inside the body
    component->SetConversion( dlg.GetAlternateBodyStyle() );
    SetShowDeMorgan( dlg.GetAlternateBodyStyle() );
    if( dlg.GetPinNameInside( ) )
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
        SAFE_DELETE( m_component );
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
    g_EditPinByPinIsOn = m_component->UnitsLocked() ? true : false;
    m_HToolBar->ToggleTool( ID_LIBEDIT_EDIT_PIN_BY_PIN, g_EditPinByPinIsOn );
    m_lastDrawItem = NULL;
    GetScreen()->ClearUndoRedoList();
    OnModify( );
    DrawPanel->Refresh();
    m_HToolBar->Refresh();
}


/*
 * Routine backup of "partlib" current in the current library.
 *
 * Save in memory only and NOT on file.
 * The routine deletes the old component (and / or aliases) to replace
 * If any, and saves the new and creates the corresponding alias.
 */
void WinEDA_LibeditFrame::SaveOnePartInMemory()
{
    LIB_COMPONENT* oldComponent;
    LIB_COMPONENT* Component;
    wxString       msg;
    wxString       rootName;

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

    GetBaseScreen()->ClrModify();

    // If the component root name was changed, that is still the name of the component
    // in the library.
    if( !m_oldRootName.IsEmpty() && m_oldRootName != m_component->GetName() )
        rootName = m_oldRootName;
    else
        rootName = m_component->GetName();

    oldComponent = m_library->FindComponent( rootName );

    if( oldComponent != NULL )
    {
        msg.Printf( _( "Component \"%s\" exists. Change it?" ), GetChars( rootName ) );

        if( !IsOK( this, msg ) )
            return;
    }

    m_drawItem = m_lastDrawItem = NULL;

    wxASSERT( m_component->isComponent() );

    if( oldComponent != NULL )
        Component = m_library->ReplaceComponent( oldComponent, m_component );
    else
        Component = m_library->AddComponent( m_component );

    if( Component == NULL )
        return;

    msg.Printf( _( "Component %s saved in library %s" ),
                GetChars( Component->GetName() ),
                GetChars( m_library->GetName() ) );
    SetStatusText( msg );
}

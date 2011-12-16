/**
 * @file xchgmod.cpp
 */

#include "fctsys.h"
#include "class_drawpanel.h"
#include "confirm.h"
#include "kicad_string.h"
#include "wxPcbStruct.h"
#include "macros.h"
#include "pcbcommon.h"

#include "class_board.h"
#include "class_module.h"

#include "pcbnew.h"
#include "dialog_exchange_modules_base.h"
#include "ar_protos.h"


int s_SelectionMode = 0;    // Remember the last exchange option, when exit dialog.


class DIALOG_EXCHANGE_MODULE : public DIALOG_EXCHANGE_MODULE_BASE
{
private:

    PCB_EDIT_FRAME* m_Parent;
    MODULE*         m_CurrentModule;

public: DIALOG_EXCHANGE_MODULE( PCB_EDIT_FRAME* aParent, MODULE* aModule );
    ~DIALOG_EXCHANGE_MODULE() { };

private:
    void OnSelectionClicked( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnQuit( wxCommandEvent& event );
    void BrowseAndSelectFootprint( wxCommandEvent& event );
    void Init();

    void Change_Current_Module();
    void Change_ModuleId( bool aUseValue );
    void Change_ModuleAll();
    int  Maj_ListeCmp( const wxString& reference, const wxString& old_name,
                       const wxString& new_name, bool ShowError );
    bool Change_1_Module( MODULE*            Module,
                          const wxString&    new_module,
                          PICKED_ITEMS_LIST* aUndoPickList,
                          bool               ShowError );
};


DIALOG_EXCHANGE_MODULE::DIALOG_EXCHANGE_MODULE( PCB_EDIT_FRAME* parent, MODULE* Module ) :
    DIALOG_EXCHANGE_MODULE_BASE( parent )
{
    m_Parent = parent;
    m_CurrentModule = Module;
    Init();
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
}


void PCB_EDIT_FRAME::InstallExchangeModuleFrame( MODULE* Module )
{
    DIALOG_EXCHANGE_MODULE dialog( this, Module );

    dialog.ShowModal();
}


void DIALOG_EXCHANGE_MODULE::OnQuit( wxCommandEvent& event )
{
    s_SelectionMode = m_Selection->GetSelection();
    EndModal( 0 );
}


void DIALOG_EXCHANGE_MODULE::Init()
{
    SetFocus();

    m_OldModule->AppendText( m_CurrentModule->m_LibRef );
    m_NewModule->AppendText( m_CurrentModule->m_LibRef );
    m_OldValue->AppendText( m_CurrentModule->m_Value->m_Text );
    m_Selection->SetSelection( s_SelectionMode );

    // Enable/disable widgets:
    wxCommandEvent event;
    OnSelectionClicked( event );
}


void DIALOG_EXCHANGE_MODULE::OnOkClick( wxCommandEvent& event )
{
    s_SelectionMode = m_Selection->GetSelection();
    switch( m_Selection->GetSelection() )
    {
    case 0:
        Change_Current_Module();
        break;

    case 1:
        Change_ModuleId( false );
        break;

    case 2:
        Change_ModuleId( true );
        break;

    case 3:
        Change_ModuleAll();
        break;
    }
}


void DIALOG_EXCHANGE_MODULE::OnSelectionClicked( wxCommandEvent& event )
{
    switch( m_Selection->GetSelection() )
    {
    case 0:
    case 1:
    case 2:
        m_NewModule->Enable( true );
        m_Browsebutton->Enable( true );
        break;

    case 3:
        m_NewModule->Enable( false );
        m_Browsebutton->Enable( false );
        break;
    }
}


/*
 * Updates the file name.CMP (if any) after an exchange module
 * (By command changeMod), if the modules are managed by this file
 *
 * If ShowError! = 0 displays error message if the file. Cmp is not found.
 * Return 1 if error
 */
int DIALOG_EXCHANGE_MODULE::Maj_ListeCmp( const wxString& reference,
                                          const wxString& old_name,
                                          const wxString& new_name,
                                          bool            ShowError )
{
    wxFileName  fn;
    wxFileName  tmpFileName;
    FILE*       FichCmp, * NewFile;
    char        line[1024];
    wxString    msg;
    char*       rs;

    if( old_name == new_name )
        return 0;

    /* Build CMP file name by changing the extension of NetList filename */
    fn = m_Parent->GetScreen()->GetFileName();
    fn.SetExt( NetCmpExtBuffer );

    FichCmp = wxFopen( fn.GetFullPath(), wxT( "rt" ) );

    if( FichCmp == NULL )
    {
        if( ShowError )
        {
            msg.Printf( _( "file %s not found" ), GetChars( fn.GetFullPath() ) );
            m_WinMessages->AppendText( msg );
        }

        return 1;
    }

    tmpFileName = fn;
    tmpFileName.SetExt( wxT( "$$$" ) );
    NewFile = wxFopen( tmpFileName.GetFullPath(), wxT( "wt" ) );

    if( NewFile == NULL )
    {
        if( ShowError )
        {
            msg.Printf( _( "Unable to create file %s" ),
                        GetChars( tmpFileName.GetFullPath() ) );
            m_WinMessages->AppendText( msg );
        }

        return 1;
    }

    rs = fgets( line, sizeof(line), FichCmp );

    fprintf( NewFile, "Cmp-Mod V01 Genere par PcbNew le %s\n", TO_UTF8( DateAndTime() ) );

    bool start_descr = false;

    while( fgets( line, sizeof(line), FichCmp ) != NULL )
    {
        if( strnicmp( line, "Reference = ", 9 ) == 0 )
        {
            char buf[1024];
            strcpy( buf, line + 12 );
            strtok( buf, ";\n\r" );

            if( stricmp( buf, TO_UTF8( reference ) ) == 0 )
            {
                start_descr = true;
            }
        }

        if( (strnicmp( line, "Begin", 5 ) == 0) || (strnicmp( line, "End", 3 ) == 0) )
        {
            start_descr = false;
        }

        if( start_descr && strnicmp( line, "IdModule", 8 ) == 0 )
        {
            sprintf( line + 8, "  = %s;\n", TO_UTF8( new_name ) );

            msg = wxT( " * in <" ) + fn.GetFullPath() + wxT( ">.\n" );
            m_WinMessages->AppendText( msg );

            start_descr = false;
        }

        fputs( line, NewFile );
    }

    fclose( FichCmp );
    fclose( NewFile );
    wxRemoveFile( fn.GetFullPath() );
    wxRenameFile( tmpFileName.GetFullPath(), fn.GetFullPath() );
    return 0;
}


/* Change the module at the current cursor position.
 * Retains the following:
 * - Same direction
 * - Same position
 * - Same text value and ref
 * - Same NetNames for pads same name
 */
void DIALOG_EXCHANGE_MODULE::Change_Current_Module()
{
    wxString newmodulename = m_NewModule->GetValue();

    if( newmodulename == wxEmptyString )
        return;

    PICKED_ITEMS_LIST pickList;

    if( Change_1_Module( m_CurrentModule, newmodulename, &pickList, true ) )
    {
        if( m_Parent->GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) )
            m_Parent->Compile_Ratsnest( NULL, true );

        m_Parent->DrawPanel->Refresh();
    }

    if( pickList.GetCount() )
        m_Parent->SaveCopyInUndoList( pickList, UR_UNSPECIFIED );
}


/*
 * Change of all modules with the same name as that lib
 * Retains:
 * - Same direction
 * - Same position
 * - Same text value and ref
 * - Same NetNames for pads same name
 * And replacing the old module with the new module
 * Note: m_CurrentModule no longer on the module reference
 * since it has been changed!
 */
void DIALOG_EXCHANGE_MODULE::Change_ModuleId( bool aUseValue )
{
    wxString msg;
    MODULE*  Module, * PtBack;
    bool     change = false;
    wxString newmodulename = m_NewModule->GetValue();
    wxString value, lib_reference;
    bool     check_module_value = false;
    int      ShowErr = 3;           // Post 3 error messages max.

    if( m_Parent->GetBoard()->m_Modules == NULL )
        return;

    if( newmodulename == wxEmptyString )
        return;

    lib_reference = m_CurrentModule->m_LibRef;

    if( aUseValue )
    {
        check_module_value = true;
        value = m_CurrentModule->m_Value->m_Text;
        msg.Printf( _( "Change modules <%s> -> <%s> (val = %s)?" ),
                    GetChars( m_CurrentModule->m_LibRef ),
                    GetChars( newmodulename ),
                    GetChars( m_CurrentModule->m_Value->m_Text ) );
    }
    else
    {
        msg.Printf( _( "Change modules <%s> -> <%s> ?" ),
                    GetChars( lib_reference ), GetChars( newmodulename ) );
    }

    if( !IsOK( this, msg ) )
        return;

    /* The change is done from the last module for the routine
     * Change_1_Module () modifies the last module in the list.
     */
    PICKED_ITEMS_LIST pickList;

    /* note: for the first module in chain (the last here), Module->Back()
     * points the board or is NULL
     */
    Module = m_Parent->GetBoard()->m_Modules.GetLast();

    for( ; Module && ( Module->Type() == PCB_MODULE_T ); Module = PtBack )
    {
        PtBack = Module->Back();

        if( lib_reference.CmpNoCase( Module->m_LibRef ) != 0 )
            continue;

        if( check_module_value )
        {
            if( value.CmpNoCase( Module->m_Value->m_Text ) != 0 )
                continue;
        }

        if( Change_1_Module( Module, newmodulename, &pickList, ShowErr ) )
            change = true;
        else if( ShowErr )
            ShowErr--;
    }

    if( change )
    {
        if( m_Parent->GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) )
            m_Parent->Compile_Ratsnest( NULL, true );

        m_Parent->DrawPanel->Refresh();
    }

    if( pickList.GetCount() )
        m_Parent->SaveCopyInUndoList( pickList, UR_UNSPECIFIED );
}


/*
 * Change all modules with module of the same name in library.
 * Maintains:
 * - Same direction
 * - Same position
 * - Same text value and ref
 * - Same NetNames for pads same name
 */
void DIALOG_EXCHANGE_MODULE::Change_ModuleAll()
{
    MODULE* Module, * PtBack;
    bool    change  = false;
    int     ShowErr = 3;              // Post 3 error messages max.

    if( m_Parent->GetBoard()->m_Modules == NULL )
        return;

    if( !IsOK( this, _( "Change ALL modules ?" ) ) )
        return;

    /* The change is done from the last module for the routine
     * Change_1_Module () modifies the last module in the list
     */
    PICKED_ITEMS_LIST pickList;

    /* note: for the first module in chain (the last here), Module->Back()
     * points the board or is NULL
     */
    Module = m_Parent->GetBoard()->m_Modules.GetLast();

    for( ; Module && ( Module->Type() == PCB_MODULE_T ); Module = PtBack )
    {
        PtBack = Module->Back();

        if( Change_1_Module( Module, Module->m_LibRef, &pickList, ShowErr ) )
            change = true;
        else if( ShowErr )
            ShowErr--;
    }

    if( change )
    {
        if( m_Parent->GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) )
            m_Parent->Compile_Ratsnest( NULL, true );

        m_Parent->DrawPanel->Refresh();
    }

    if( pickList.GetCount() )
        m_Parent->SaveCopyInUndoList( pickList, UR_UNSPECIFIED );
}


/*
 * Change the number empr module with the module name new_module
 * - Same direction
 * - Same position
 * - Same text value and ref
 * - Same NetNames for pads same name
 * Returns:
 * False if no change (if the new module is not free)
 * True if OK
 * Ratsnest must be recalculated after module exchange
 */
bool DIALOG_EXCHANGE_MODULE::Change_1_Module( MODULE*            Module,
                                              const wxString&    new_module,
                                              PICKED_ITEMS_LIST* aUndoPickList,
                                              bool               ShowError )
{
    wxString namecmp, oldnamecmp;
    MODULE*  NewModule;
    wxString line;

    if( Module == NULL )
        return false;

    wxBusyCursor dummy;

    /* Copy parameters from the old module. */
    oldnamecmp = Module->m_LibRef;
    namecmp    = new_module;

    /* Load module. */
    line.Printf( _( "Change module %s (%s)  " ),
                 GetChars( Module->m_Reference->m_Text ),
                 GetChars( oldnamecmp ) );
    m_WinMessages->AppendText( line );

    namecmp.Trim( true );
    namecmp.Trim( false );
    NewModule = m_Parent->GetModuleLibrary( wxEmptyString, namecmp, ShowError );

    if( NewModule == NULL )  /* New module not found, redraw the old one. */
    {
        m_WinMessages->AppendText( wxT( "No\n" ) );
        return false;
    }

    if( Module == m_CurrentModule )
        m_CurrentModule = NewModule;

    m_WinMessages->AppendText( wxT( "Ok\n" ) );

    m_Parent->Exchange_Module( Module, NewModule, aUndoPickList );

    Maj_ListeCmp( NewModule->m_Reference->m_Text, oldnamecmp, namecmp, ShowError );

    return true;
}


/**
 * Function Exchange_Module
 * Replaces OldModule by NewModule, using OldModule settings:
 * position, orientation, pad netnames ...)
 * OldModule is deleted or put in undo list.
 * @param aOldModule = footprint to replace
 * @param aNewModule = footprint to put
 * @param aUndoPickList = the undo list used to save  OldModule. If null,
 * OldModule is deleted
 */
void PCB_EDIT_FRAME::Exchange_Module( MODULE*            aOldModule,
                                      MODULE*            aNewModule,
                                      PICKED_ITEMS_LIST* aUndoPickList )
{
    wxPoint oldpos;
    D_PAD*  pad, * old_pad;

    if( ( aOldModule->Type() != PCB_MODULE_T ) || ( aNewModule->Type() != PCB_MODULE_T ) )
    {
        wxMessageBox( wxT( "PCB_EDIT_FRAME::Exchange_Module() StuctType error" ) );
        return;
    }

    aNewModule->SetParent( GetBoard() );

    GetBoard()->m_Status_Pcb = 0;
    oldpos = GetScreen()->GetCrossHairPosition();
    GetScreen()->SetCrossHairPosition( aOldModule->m_Pos, false );

    /* place module without ratsnest refresh: this will be made later
     * when all modules are on board
     */
    PlaceModule( aNewModule, NULL, true );
    GetScreen()->SetCrossHairPosition( oldpos, false );

    /* Flip footprint if needed */
    if( aOldModule->GetLayer() != aNewModule->GetLayer() )
    {
        aNewModule->Flip( aNewModule->m_Pos );
    }

    /* Rotate footprint if needed */
    if( aOldModule->m_Orient != aNewModule->m_Orient )
    {
        Rotate_Module( NULL, aNewModule, aOldModule->m_Orient, false );
    }

    /* Update reference and value */
    aNewModule->m_Reference->m_Text = aOldModule->m_Reference->m_Text;
    aNewModule->m_Value->m_Text     = aOldModule->m_Value->m_Text;

    /* Updating other parameters */
    aNewModule->SetTimeStamp( aOldModule->GetTimeStamp() );
    aNewModule->m_Path = aOldModule->m_Path;

    /* Update pad netnames ( when possible) */
    pad = aNewModule->m_Pads;

    for( ; pad != NULL; pad = pad->Next() )
    {
        pad->SetNetname( wxEmptyString );
        pad->SetNet( 0 );
        old_pad = aOldModule->m_Pads;

        for( ; old_pad != NULL; old_pad = old_pad->Next() )
        {
            if( pad->PadNameEqual( old_pad ) )
            {
                pad->SetNetname( old_pad->GetNetname() );
                pad->SetNet( old_pad->GetNet() );
            }
        }
    }

    if( aUndoPickList )
    {
        GetBoard()->Remove( aOldModule );
        ITEM_PICKER picker_old( aOldModule, UR_DELETED );
        ITEM_PICKER picker_new( aNewModule, UR_NEW );
        aUndoPickList->PushItem( picker_old );
        aUndoPickList->PushItem( picker_new );
    }
    else
    {
        aOldModule->DeleteStructure();
    }

    GetBoard()->m_Status_Pcb = 0;
    aNewModule->m_Flags = 0;
    OnModify();
}


/*
 * Displays the list of modules in library name and select 1 name.
 */
void DIALOG_EXCHANGE_MODULE::BrowseAndSelectFootprint( wxCommandEvent& event )
{
    wxString newname;

    newname = m_Parent->Select_1_Module_From_List( m_Parent,
                                                   wxEmptyString,
                                                   wxEmptyString,
                                                   wxEmptyString );
    if( newname != wxEmptyString )
        m_NewModule->SetValue( newname );
}


/**
 * Function RecreateBOMFileFromBoard
 * Recreates a .cmp file from the current loaded board
 * this is the same as created by CvPcb.
 * can be used if this file is lost
 */
void PCB_EDIT_FRAME::RecreateCmpFileFromBoard( wxCommandEvent& aEvent )
{
    wxFileName  fn;
    FILE*       FichCmp;
    char        line[1024];
    MODULE*     Module = GetBoard()->m_Modules;
    wxString    msg;
    wxString    wildcard;
    char*       rs;

    if( Module == NULL )
    {
        DisplayError( this, _( "No Modules!" ) );
        return;
    }

    /* Calculation file name by changing the extension name to NetList */
    fn = GetScreen()->GetFileName();
    fn.SetExt( NetCmpExtBuffer );
    wildcard = _( "Component files (." ) + NetCmpExtBuffer + wxT( ")|*." ) + NetCmpExtBuffer;

    wxFileDialog dlg( this, _( "Save Component Files" ), wxGetCwd(),
                      fn.GetFullName(), wildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();

    FichCmp = wxFopen( fn.GetFullPath(), wxT( "wt" ) );

    if( FichCmp == NULL )
    {
        msg = _( "Unable to create file " ) + fn.GetFullPath();
        DisplayError( this, msg );
        return;
    }

    rs = fgets( line, sizeof(line), FichCmp );
    fprintf( FichCmp, "Cmp-Mod V01 Genere par PcbNew le %s\n", TO_UTF8( DateAndTime() ) );

    for( ; Module != NULL; Module = Module->Next() )
    {
        fprintf( FichCmp, "\nBeginCmp\n" );
        fprintf( FichCmp, "TimeStamp = %8.8lX\n", Module->GetTimeStamp() );
        fprintf( FichCmp, "Path = %s\n", TO_UTF8( Module->m_Path ) );
        fprintf( FichCmp, "Reference = %s;\n",
                 !Module->m_Reference->m_Text.IsEmpty() ?
                 TO_UTF8( Module->m_Reference->m_Text ) : "[NoRef]" );
        fprintf( FichCmp, "ValeurCmp = %s;\n",
                 !Module->m_Value->m_Text.IsEmpty() ?
                 TO_UTF8( Module->m_Value->m_Text ) : "[NoVal]" );
        fprintf( FichCmp, "IdModule  = %s;\n", TO_UTF8( Module->m_LibRef ) );
        fprintf( FichCmp, "EndCmp\n" );
    }

    fprintf( FichCmp, "\nEndListe\n" );
    fclose( FichCmp );
}

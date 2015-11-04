/**
 * @file xchgmod.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <class_drawpanel.h>
#include <class_draw_panel_gal.h>
#include <confirm.h>
#include <kicad_string.h>
#include <wxPcbStruct.h>
#include <macros.h>

#include <class_board.h>
#include <class_module.h>
#include <project.h>

#include <pcbnew.h>
#include <dialog_exchange_modules_base.h>
#include <wildcards_and_files_ext.h>
#include <kiway.h>


static bool RecreateCmpFile( BOARD * aBrd, const wxString& aFullCmpFileName );


class DIALOG_EXCHANGE_MODULE : public DIALOG_EXCHANGE_MODULE_BASE
{
private:
    PCB_EDIT_FRAME* m_parent;
    MODULE*         m_currentModule;
    static int      m_selectionMode;    // Remember the last exchange option

public:
    DIALOG_EXCHANGE_MODULE( PCB_EDIT_FRAME* aParent, MODULE* aModule );
    ~DIALOG_EXCHANGE_MODULE() { };

private:
    void OnSelectionClicked( wxCommandEvent& event );
    void OnOkClick( wxCommandEvent& event );
    void OnQuit( wxCommandEvent& event );
    void BrowseAndSelectFootprint( wxCommandEvent& event );
    void ViewAndSelectFootprint( wxCommandEvent& event );
    void RebuildCmpList( wxCommandEvent& event );
    void init();

    bool changeCurrentFootprint();
    bool changeSameFootprints( bool aUseValue);
    bool changeAllFootprints();
    bool change_1_Module( MODULE*            aModule,
                          const FPID&        aNewFootprintFPID,
                          bool               eShowError );

    PICKED_ITEMS_LIST m_undoPickList;
};


int DIALOG_EXCHANGE_MODULE::m_selectionMode = 0;


DIALOG_EXCHANGE_MODULE::DIALOG_EXCHANGE_MODULE( PCB_EDIT_FRAME* parent, MODULE* Module ) :
    DIALOG_EXCHANGE_MODULE_BASE( parent )
{
    m_parent = parent;
    m_currentModule = Module;
    init();
    GetSizer()->Fit( this );
    GetSizer()->SetSizeHints( this );
    Center();
}


int PCB_EDIT_FRAME::InstallExchangeModuleFrame( MODULE* Module )
{
    DIALOG_EXCHANGE_MODULE dialog( this, Module );

    return dialog.ShowQuasiModal();
}


void DIALOG_EXCHANGE_MODULE::OnQuit( wxCommandEvent& event )
{
    m_selectionMode = m_Selection->GetSelection();
    Show( false );
    EndQuasiModal( wxID_CANCEL );
}


void DIALOG_EXCHANGE_MODULE::init()
{
    SetFocus();

    m_CurrentFootprintFPID->AppendText( FROM_UTF8( m_currentModule->GetFPID().Format().c_str() ) );
    m_NewFootprintFPID->AppendText( FROM_UTF8( m_currentModule->GetFPID().Format().c_str() ) );
    m_CmpValue->AppendText( m_currentModule->GetValue() );
    m_CmpReference->AppendText( m_currentModule->GetReference() );
    m_Selection->SetString( 0, wxString::Format(
                            _("Change footprint of '%s'" ),
                            GetChars( m_currentModule->GetReference() ) ) );
    wxString fpname = m_CurrentFootprintFPID->GetValue().AfterLast(':');

    if( fpname.IsEmpty() )    // Happens for old fp names
        fpname = m_CurrentFootprintFPID->GetValue();

    m_Selection->SetString( 1, wxString::Format(
                            _("Change footprints '%s'" ),
                            GetChars( fpname.Left( 12 ) ) ) );

    m_Selection->SetSelection( m_selectionMode );

    // Enable/disable widgets:
    wxCommandEvent event;
    OnSelectionClicked( event );
}


void DIALOG_EXCHANGE_MODULE::OnOkClick( wxCommandEvent& event )
{
    m_undoPickList.ClearItemsList();
    m_selectionMode = m_Selection->GetSelection();
    bool result = false;

    switch( m_Selection->GetSelection() )
    {
    case 0:
        result = changeCurrentFootprint();
        break;

    case 1:
        result = changeSameFootprints( false );
        break;

    case 2:
        result = changeSameFootprints( true );
        break;

    case 3:
        result = changeAllFootprints();
        break;
    }

    if( result )
    {
        if( m_parent->GetBoard()->IsElementVisible( RATSNEST_VISIBLE ) )
            m_parent->Compile_Ratsnest( NULL, true );

        m_parent->GetCanvas()->Refresh();
    }

    if( m_undoPickList.GetCount() )
        m_parent->SaveCopyInUndoList( m_undoPickList, UR_UNSPECIFIED );
}


void DIALOG_EXCHANGE_MODULE::OnSelectionClicked( wxCommandEvent& event )
{
    bool enable = true;

    switch( m_Selection->GetSelection() )
    {
    case 0:
    case 1:
    case 2:
        break;

    case 3:
        enable = false;
        break;
    }

    m_NewFootprintFPID->Enable( enable );
    m_Browsebutton->Enable( enable );
}


/*
 * Rebuild the file name.CMP (if any) after exchanging footprints
 * if the footprint are managed by this file
 * Return false if error
 */
void DIALOG_EXCHANGE_MODULE::RebuildCmpList( wxCommandEvent& event )
{
    wxFileName  fn;
    wxString    msg;

    // Build the .cmp file name from the board name
    fn = m_parent->GetBoard()->GetFileName();
    fn.SetExt( ComponentFileExtension );

    if( RecreateCmpFile( m_parent->GetBoard(), fn.GetFullPath() ) )
    {
        msg.Printf( _( "File '%s' created\n" ),
                    GetChars( fn.GetFullPath() ) );
    }
    else
    {
        msg.Printf( _( "** Could not create file '%s' ***\n" ),
                    GetChars( fn.GetFullPath() ) );
    }

    m_WinMessages->AppendText( msg );
}


/* Change the current footprint at the current cursor position.
 * Retains the following:
 * - position, orientation and side
 * - value and ref
 * - pads net names
 */
bool DIALOG_EXCHANGE_MODULE::changeCurrentFootprint()
{
    wxString newmodulename = m_NewFootprintFPID->GetValue();

    if( newmodulename == wxEmptyString )
        return false;

    return change_1_Module( m_currentModule, newmodulename, true );
}


/*
 * Change all footprints having the same fpid by a new one from lib
 * Retains:
 * - direction, position, side
 * - value and ref
 * - pads net names
 * Note: m_currentModule is no longer the current footprint
 * since it has been changed!
 * if aUseValue is true, footprints having the same fpid should
 * also have the same value
 */
bool DIALOG_EXCHANGE_MODULE::changeSameFootprints( bool aUseValue )
{
    wxString msg;
    MODULE*  Module;
    MODULE*  PtBack;
    bool     change = false;
    wxString newmodulename = m_NewFootprintFPID->GetValue();
    wxString value;
    FPID     lib_reference;
    bool     check_module_value = false;
    int      ShowErr = 3;           // Post 3 error messages max.

    if( m_parent->GetBoard()->m_Modules == NULL )
        return false;

    if( newmodulename == wxEmptyString )
        return false;

    lib_reference = m_currentModule->GetFPID();

    if( aUseValue )
    {
        check_module_value = true;
        value = m_currentModule->GetValue();
        msg.Printf( _( "Change footprint %s -> %s (for value = %s)?" ),
                    GetChars( FROM_UTF8( m_currentModule->GetFPID().Format().c_str() ) ),
                    GetChars( newmodulename ),
                    GetChars( m_currentModule->GetValue() ) );
    }
    else
    {
        msg.Printf( _( "Change footprint %s -> %s ?" ),
                    GetChars( FROM_UTF8( lib_reference.Format().c_str() ) ),
                    GetChars( newmodulename ) );
    }

    if( !IsOK( this, msg ) )
        return false;

    /* The change is done from the last module because
     * change_1_Module () modifies the last item in the list.
     *
     * note: for the first module in chain (the last here), Module->Back()
     * points the board or is NULL
     */
    Module = m_parent->GetBoard()->m_Modules.GetLast();

    for( ; Module && ( Module->Type() == PCB_MODULE_T ); Module = PtBack )
    {
        PtBack = Module->Back();

        if( lib_reference != Module->GetFPID() )
            continue;

        if( check_module_value )
        {
            if( value.CmpNoCase( Module->GetValue() ) != 0 )
                continue;
        }

        if( change_1_Module( Module, newmodulename, ShowErr ) )
            change = true;
        else if( ShowErr )
            ShowErr--;
    }

    return change;
}


/*
 * Change all modules with module of the same name in library.
 * Maintains:
 * - direction, position, side
 * - value and ref
 * - pads net names
 */
bool DIALOG_EXCHANGE_MODULE::changeAllFootprints()
{
    MODULE* Module, * PtBack;
    bool    change  = false;
    int     ShowErr = 3;              // Post 3 error max.

    if( m_parent->GetBoard()->m_Modules == NULL )
        return false;

    if( !IsOK( this, _( "Are you sure you want to change all footprints?" ) ) )
        return false;

    /* The change is done from the last module because the function
     * change_1_Module () modifies the last module in the list
     *
     * note: for the first module in chain (the last here), Module->Back()
     * points the board or is NULL
     */
    Module = m_parent->GetBoard()->m_Modules.GetLast();

    for( ; Module && ( Module->Type() == PCB_MODULE_T ); Module = PtBack )
    {
        PtBack = Module->Back();

        if( change_1_Module( Module, Module->GetFPID(), ShowErr ) )
            change = true;
        else if( ShowErr )
            ShowErr--;
    }

    return change;
}


/*
 * Change aModule to a new, fresh one from lib
 * Retains
 * - direction, position, side
 * - value and ref
 * - pads net names
 * Returns: false if no change (if the new module is not found)
 * true if OK
 */
bool DIALOG_EXCHANGE_MODULE::change_1_Module( MODULE*            aModule,
                                              const FPID&        aNewFootprintFPID,
                                              bool               aShowError )
{
    MODULE*  newModule;
    wxString line;

    if( aModule == NULL )
        return false;

    wxBusyCursor dummy;

    // Copy parameters from the old module.
    FPID oldFootprintFPID = aModule->GetFPID();

    // Load module.
    line.Printf( _( "Change footprint '%s' (from '%s') to '%s'" ),
                 GetChars( aModule->GetReference() ),
                 oldFootprintFPID.Format().c_str(),
                 aNewFootprintFPID.Format().c_str() );
    m_WinMessages->AppendText( line );

    newModule = m_parent->LoadFootprint( aNewFootprintFPID );

    if( newModule == NULL )  // New module not found, redraw the old one.
    {
        m_WinMessages->AppendText( wxT( " No\n" ) );
        return false;
    }

    m_parent->Exchange_Module( aModule, newModule, &m_undoPickList );
    m_parent->GetBoard()->Add( newModule, ADD_APPEND );

    if( aModule == m_currentModule )
        m_currentModule = newModule;

    m_WinMessages->AppendText( wxT( " OK\n" ) );

    return true;
}


void PCB_EDIT_FRAME::Exchange_Module( MODULE*            aOldModule,
                                      MODULE*            aNewModule,
                                      PICKED_ITEMS_LIST* aUndoPickList )
{
    aNewModule->SetParent( GetBoard() );

    /* place module without ratsnest refresh: this will be made later
     * when all modules are on board
     */
    PlaceModule( aNewModule, NULL, true );

    // Copy full placement and pad net names (when possible)
    // but not local settings like clearances (use library values)
    aOldModule->CopyNetlistSettings( aNewModule, false );

    // Copy reference and value
    aNewModule->SetReference( aOldModule->GetReference() );
    aNewModule->SetValue( aOldModule->GetValue() );

    // Updating other parameters
    aNewModule->SetTimeStamp( aOldModule->GetTimeStamp() );
    aNewModule->SetPath( aOldModule->GetPath() );

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
        GetGalCanvas()->GetView()->Remove( aOldModule );
        aOldModule->DeleteStructure();
    }

    GetBoard()->m_Status_Pcb = 0;
    aNewModule->ClearFlags();
    OnModify();
}


// Displays the list of available footprints in library name and select a footprint.
void DIALOG_EXCHANGE_MODULE::BrowseAndSelectFootprint( wxCommandEvent& event )
{
    wxString newname;

    newname = m_parent->SelectFootprint( m_parent, wxEmptyString, wxEmptyString, wxEmptyString,
                                         Prj().PcbFootprintLibs() );

    if( newname != wxEmptyString )
        m_NewFootprintFPID->SetValue( newname );
}


// Runs the footprint viewer to select a footprint.
void DIALOG_EXCHANGE_MODULE::ViewAndSelectFootprint( wxCommandEvent& event )
{
    wxString newname;

    KIWAY_PLAYER* frame = Kiway().Player( FRAME_PCB_MODULE_VIEWER_MODAL, true );

    if( frame->ShowModal( &newname, this ) )
    {
        m_NewFootprintFPID->SetValue( newname );
    }

    frame->Destroy();
}


void PCB_EDIT_FRAME::RecreateCmpFileFromBoard( wxCommandEvent& aEvent )
{
    wxFileName  fn;
    MODULE*     module = GetBoard()->m_Modules;
    wxString    msg;
    wxString    wildcard;

    if( module == NULL )
    {
        DisplayError( this, _( "No footprints!" ) );
        return;
    }

    // Build the .cmp file name from the board name
    fn = GetBoard()->GetFileName();
    fn.SetExt( ComponentFileExtension );
    wildcard = wxGetTranslation( ComponentFileWildcard );

    wxString pro_dir = wxPathOnly( Prj().GetProjectFullName() );

    wxFileDialog dlg( this, _( "Save Footprint Association File" ), pro_dir,
                      fn.GetFullName(), wildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();

    if( ! RecreateCmpFile( GetBoard(), fn.GetFullPath() ) )
    {
        msg.Printf( _( "Could not create file '%s'" ), GetChars(fn.GetFullPath() ) );
        DisplayError( this, msg );
        return;
    }
}


bool RecreateCmpFile( BOARD * aBrd, const wxString& aFullCmpFileName )
{
    FILE* cmpFile;

    cmpFile = wxFopen( aFullCmpFileName, wxT( "wt" ) );

    if( cmpFile == NULL )
        return false;

    fprintf( cmpFile, "Cmp-Mod V01 Created by PcbNew   date = %s\n", TO_UTF8( DateAndTime() ) );

    MODULE* module = aBrd->m_Modules;
    for( ; module != NULL; module = module->Next() )
    {
        fprintf( cmpFile, "\nBeginCmp\n" );
        fprintf( cmpFile, "TimeStamp = %8.8lX\n", module->GetTimeStamp() );
        fprintf( cmpFile, "Path = %s\n", TO_UTF8( module->GetPath() ) );
        fprintf( cmpFile, "Reference = %s;\n",
                 !module->GetReference().IsEmpty() ?
                 TO_UTF8( module->GetReference() ) : "[NoRef]" );
        fprintf( cmpFile, "ValeurCmp = %s;\n",
                 !module->GetValue().IsEmpty() ?
                 TO_UTF8( module->GetValue() ) : "[NoVal]" );
        fprintf( cmpFile, "IdModule  = %s;\n", module->GetFPID().Format().c_str() );
        fprintf( cmpFile, "EndCmp\n" );
    }

    fprintf( cmpFile, "\nEndListe\n" );
    fclose( cmpFile );

    return true;
}

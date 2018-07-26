/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <pcb_edit_frame.h>
#include <macros.h>
#include <board_commit.h>
#include <bitmaps.h>

#include <class_board.h>
#include <class_module.h>
#include <project.h>
#include <wx_html_report_panel.h>

#include <pcbnew.h>
#include <dialog_exchange_footprints.h>
#include <wildcards_and_files_ext.h>
#include <kiway.h>


static bool RecreateCmpFile( BOARD * aBrd, const wxString& aFullCmpFileName );


#define ID_MATCH_FP_ALL 4200
#define ID_MATCH_FP_REF 4201
#define ID_MATCH_FP_VAL 4202
#define ID_MATCH_FP_ID  4203

int DIALOG_EXCHANGE_FOOTPRINTS::m_matchModeForUpdate           = ID_MATCH_FP_ALL;
int DIALOG_EXCHANGE_FOOTPRINTS::m_matchModeForExchange         = ID_MATCH_FP_REF;
int DIALOG_EXCHANGE_FOOTPRINTS::m_matchModeForUpdateSelected   = ID_MATCH_FP_REF;
int DIALOG_EXCHANGE_FOOTPRINTS::m_matchModeForExchangeSelected = ID_MATCH_FP_REF;


DIALOG_EXCHANGE_FOOTPRINTS::DIALOG_EXCHANGE_FOOTPRINTS( PCB_EDIT_FRAME* parent, MODULE* Module,
                                                bool updateMode ) :
    DIALOG_EXCHANGE_FOOTPRINTS_BASE( parent ), m_commit( parent )
{
    m_parent = parent;
    m_currentModule = Module;
    m_updateMode = updateMode;

    init( m_updateMode );

    // DIALOG_SHIM needs a unique hash_key because classname is not sufficient
    // because the update and change versions of this dialog have different controls.
    m_hash_key = TO_UTF8( GetTitle() );

    // Ensure m_closeButton (with id = wxID_CANCEL) has the right label
    // (to fix automatic renaming of button label )
    m_closeButton->SetLabel( _( "Close" ) );

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


void DIALOG_EXCHANGE_FOOTPRINTS::OnQuit( wxCommandEvent& event )
{
    Show( false );
    EndQuasiModal( wxID_CANCEL );
}


void DIALOG_EXCHANGE_FOOTPRINTS::init( bool updateMode )
{
    SetFocus();

    wxString title = updateMode ? _( "Update Footprints from Library" ) : _( "Change Footprints" );
    wxString verb  = updateMode ? _( "Update" )                         : _( "Change" );
    wxString label;

    SetTitle( title );

    if( updateMode )
    {
        label.Printf( m_matchAll->GetLabel(), verb );
        m_matchAll->SetLabel( label );

        m_middleSizer->Show( false );
    }
    else
    {
        m_upperSizer->FindItem( m_matchAll )->Show( false );

        if( m_currentModule )
            m_newID->AppendText( FROM_UTF8( m_currentModule->GetFPID().Format().c_str() ) );
        m_newIDBrowseButton->SetBitmap( KiBitmap( library_browse_xpm ) );
    }

    if( m_currentModule )
    {
        m_upperSizer->FindItem( m_matchSpecifiedRef )->Show( false );
        m_upperSizer->FindItem( m_specifiedRef )->Show( false );

        label.Printf( m_matchCurrentRef->GetLabel(), verb, m_currentModule->GetReference() );
        m_matchCurrentRef->SetLabel( label );

        m_upperSizer->FindItem( m_matchSpecifiedValue )->Show( false );
        m_upperSizer->FindItem( m_specifiedValue )->Show( false );

        label.Printf( m_matchCurrentValue->GetLabel(), verb, m_currentModule->GetValue() );
        m_matchCurrentValue->SetLabel( label );
    }
    else
    {
        m_upperSizer->FindItem( m_matchCurrentRef )->Show( false );

        label.Printf( m_matchSpecifiedRef->GetLabel(), verb );
        m_matchSpecifiedRef->SetLabel( label );

        m_upperSizer->FindItem( m_matchCurrentValue )->Show( false );

        label.Printf( m_matchSpecifiedValue->GetLabel(), verb );
        m_matchSpecifiedValue->SetLabel( label );
    }

    label.Printf( m_matchSpecifiedID->GetLabel(), verb );
    m_matchSpecifiedID->SetLabel( label );

    if( m_currentModule )
        m_specifiedID->AppendText( FROM_UTF8( m_currentModule->GetFPID().Format().c_str() ) );
    m_specifiedIDBrowseButton->SetBitmap( KiBitmap( library_browse_xpm ) );

    m_upperSizer->SetEmptyCellSize( wxSize( 0, 0 ) );
    m_upperSizer->RecalcSizes();

    // initialize match-mode
    wxCommandEvent event;
    switch( getMatchMode() )
    {
    case ID_MATCH_FP_ALL:
        if( m_currentModule )
            OnMatchRefClicked( event );
        else
            OnMatchAllClicked( event );
        break;
    case ID_MATCH_FP_REF:
        OnMatchRefClicked( event );
        break;
    case ID_MATCH_FP_VAL:
        OnMatchValueClicked( event );
        break;
    case ID_MATCH_FP_ID:
        OnMatchIDClicked( event );
    }
}


int DIALOG_EXCHANGE_FOOTPRINTS::getMatchMode()
{
    if( m_updateMode )
        return( m_currentModule ? m_matchModeForUpdateSelected : m_matchModeForUpdate );
    else
        return( m_currentModule ? m_matchModeForExchangeSelected : m_matchModeForExchange );
}


void DIALOG_EXCHANGE_FOOTPRINTS::setMatchMode( int aMatchMode )
{
    if( m_updateMode )
    {
        if( m_currentModule )
            m_matchModeForUpdateSelected = aMatchMode;
        else
            m_matchModeForUpdate = aMatchMode;
    }
    else
    {
        if( m_currentModule )
            m_matchModeForExchangeSelected = aMatchMode;
        else
            m_matchModeForExchange = aMatchMode;
    }
}


bool DIALOG_EXCHANGE_FOOTPRINTS::isMatch( MODULE* aModule )
{
    LIB_ID specifiedID;

    switch( getMatchMode() )
    {
    case ID_MATCH_FP_ALL:
        return true;
    case ID_MATCH_FP_REF:
        // currentModule case goes through changeCurrentFootprint, so we only have
        // to handle specifiedRef case
        return aModule->GetReference() == m_specifiedRef->GetValue();
    case ID_MATCH_FP_VAL:
        // currentValue must also check FPID so we don't get accidental matches that
        // the user didn't intend
        if( m_currentModule )
            return aModule->GetValue() == m_currentModule->GetValue()
                    && aModule->GetFPID() == m_currentModule->GetFPID();
        else
            return aModule->GetValue() == m_specifiedValue->GetValue();
    case ID_MATCH_FP_ID:
        specifiedID.Parse( m_specifiedID->GetValue(), LIB_ID::ID_PCB );
        return aModule->GetFPID() == specifiedID;
    }
    return false;   // just to quiet compiler warnings....
}


wxRadioButton* DIALOG_EXCHANGE_FOOTPRINTS::getRadioButtonForMode()
{
    switch( getMatchMode() )
    {
    case ID_MATCH_FP_ALL:
        return( m_matchAll );
    case ID_MATCH_FP_REF:
        return( m_matchCurrentRef->IsShown() ? m_matchCurrentRef : m_matchSpecifiedRef );
    case ID_MATCH_FP_VAL:
        return( m_matchCurrentValue->IsShown() ? m_matchCurrentValue : m_matchSpecifiedValue );
    case ID_MATCH_FP_ID:
        return( m_matchSpecifiedID );
    default:
        return nullptr;
    }
}


void DIALOG_EXCHANGE_FOOTPRINTS::updateMatchModeRadioButtons( wxUpdateUIEvent& )
{
    wxRadioButton* rb_button = getRadioButtonForMode();

    wxRadioButton* rb_butt_list[] =
    {
        m_matchCurrentRef, m_matchSpecifiedRef,
        m_matchCurrentValue, m_matchCurrentValue,
        m_matchSpecifiedValue, m_matchSpecifiedValue,
        m_matchSpecifiedID, m_matchSpecifiedID,
        nullptr     // end of list
    };

    // Ensure the button state is ok. Only one button can be checked
    // Change button state only if its state is incorrect, otherwise
    // we have issues depending on the platform.
    for( int ii = 0; rb_butt_list[ii]; ++ii )
    {
        bool state = rb_butt_list[ii] == rb_button;

        if( rb_butt_list[ii]->GetValue() != state )
            rb_butt_list[ii]->SetValue( state );
    }
}


void DIALOG_EXCHANGE_FOOTPRINTS::OnMatchAllClicked( wxCommandEvent& event )
{
    setMatchMode( ID_MATCH_FP_ALL );
    m_matchAll->SetFocus();
}


void DIALOG_EXCHANGE_FOOTPRINTS::OnMatchRefClicked( wxCommandEvent& event )
{
    setMatchMode( ID_MATCH_FP_REF );

    if( m_specifiedRef->IsShown() && event.GetEventObject() != m_specifiedRef )
        m_specifiedRef->SetFocus();
}


void DIALOG_EXCHANGE_FOOTPRINTS::OnMatchValueClicked( wxCommandEvent& event )
{
    setMatchMode( ID_MATCH_FP_VAL );

    if( m_specifiedValue->IsShown() && event.GetEventObject() != m_specifiedValue )
        m_specifiedValue->SetFocus();
}


void DIALOG_EXCHANGE_FOOTPRINTS::OnMatchIDClicked( wxCommandEvent& event )
{
    setMatchMode( ID_MATCH_FP_ID );

    if( m_specifiedID->IsShown() && event.GetEventObject() != m_specifiedID )
        m_specifiedID->SetFocus();
}


void DIALOG_EXCHANGE_FOOTPRINTS::OnApplyClick( wxCommandEvent& event )
{
    bool result = false;

    m_MessageWindow->Clear();
    m_MessageWindow->Flush( true );

    if( getMatchMode() == ID_MATCH_FP_REF && m_currentModule )
        result = changeCurrentFootprint();
    else
        result = changeSameFootprints();

    if( result )
    {
        if( m_parent->GetBoard()->IsElementVisible( LAYER_RATSNEST ) )
            m_parent->Compile_Ratsnest( NULL, true );

        m_parent->GetCanvas()->Refresh();
    }

    m_commit.Push( wxT( "Changed footprint" ) );
}


void DIALOG_EXCHANGE_FOOTPRINTS::RebuildCmpList( wxCommandEvent& event )
{
    wxString    msg;
    REPORTER& reporter = m_MessageWindow->Reporter();
    m_MessageWindow->Clear();
    m_MessageWindow->Flush( true );

    // Build the .cmp file name from the board name
    wxFileName fn = m_parent->GetBoard()->GetFileName();
    fn.SetExt( ComponentFileExtension );

    if( RecreateCmpFile( m_parent->GetBoard(), fn.GetFullPath() ) )
    {
        msg.Printf( _( "File \"%s\" created\n" ), GetChars( fn.GetFullPath() ) );
        reporter.Report( msg, REPORTER::RPT_INFO );
    }
    else
    {
        msg.Printf( _( "** Could not create file \"%s\" ***\n" ),
                    GetChars( fn.GetFullPath() ) );
        reporter.Report( msg, REPORTER::RPT_ERROR );
    }
}


bool DIALOG_EXCHANGE_FOOTPRINTS::changeCurrentFootprint()
{
    if( m_updateMode )
        return change_1_Module( m_currentModule, m_currentModule->GetFPID(), true );

    LIB_ID newFPID;
    wxString newFPIDStr = m_newID->GetValue();

    if( newFPIDStr == wxEmptyString )
        return false;

    newFPID.Parse( newFPIDStr, LIB_ID::ID_PCB, true );
    return change_1_Module( m_currentModule, newFPID, true );
}


bool DIALOG_EXCHANGE_FOOTPRINTS::changeSameFootprints()
{
    MODULE*  Module;
    MODULE*  PtBack;
    bool     change = false;
    LIB_ID   newFPID;
    wxString value;
    int      ShowErr = 3;           // Post 3 error messages max.

    if( m_parent->GetBoard()->m_Modules == NULL )
        return false;

    if( !m_updateMode )
    {
        newFPID.Parse( m_newID->GetValue(), LIB_ID::ID_PCB );

        if( !newFPID.IsValid() )
            return false;
    }

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

        if( !isMatch( Module ) )
            continue;

        bool result;
        if( m_updateMode )
            result = change_1_Module( Module, Module->GetFPID(), ShowErr );
        else
            result = change_1_Module( Module, newFPID, ShowErr );

        if( result )
            change = true;
        else if( ShowErr )
            ShowErr--;
    }

    return change;
}


bool DIALOG_EXCHANGE_FOOTPRINTS::change_1_Module( MODULE*            aModule,
                                              const LIB_ID&      aNewFootprintFPID,
                                              bool               aShowError )
{
    MODULE* newModule;
    wxString msg;

    if( aModule == NULL )
        return false;

    wxBusyCursor dummy;
    REPORTER& reporter = m_MessageWindow->Reporter();

    // Copy parameters from the old footprint.
    LIB_ID oldFootprintFPID = aModule->GetFPID();

    // Load module.
    msg.Printf( _( "Change footprint \"%s\" (from \"%s\") to \"%s\"" ),
                 GetChars( aModule->GetReference() ),
                 oldFootprintFPID.Format().c_str(),
                 aNewFootprintFPID.Format().c_str() );

    newModule = m_parent->LoadFootprint( aNewFootprintFPID );

    if( newModule == NULL )  // New module not found.
    {
        msg << ": " << _( "footprint not found" );
        reporter.Report( msg, REPORTER::RPT_ERROR );
        return false;
    }

    m_parent->Exchange_Module( aModule, newModule, m_commit );

    if( aModule == m_currentModule )
        m_currentModule = newModule;
    if( aModule == m_parent->GetCurItem() )
        m_parent->SetCurItem( newModule );

    msg += ": OK";
    reporter.Report( msg, REPORTER::RPT_ACTION );

    return true;
}


void PCB_EDIT_FRAME::Exchange_Module( MODULE* aOldModule,
                                      MODULE* aNewModule,
                                      BOARD_COMMIT& aCommit )
{
    aNewModule->SetParent( GetBoard() );

    /* place module without ratsnest refresh: this will be made later
     * when all modules are on board */
    PlaceModule( aNewModule, NULL, false );

    // Copy full placement and pad net names (when possible)
    // but not local settings like clearances (use library values)
    aOldModule->CopyNetlistSettings( aNewModule, false );

    // Copy reference
    aNewModule->SetReference( aOldModule->GetReference() );

    // Copy value unless it is a proxy for the footprint ID (a good example is replacing a
    // footprint with value "MoutingHole-2.5mm" with one of value "MountingHole-4mm").
    if( aOldModule->GetValue() != aOldModule->GetFPID().GetLibItemName() )
        aNewModule->SetValue( aOldModule->GetValue() );

    // Compare the footprint name only, in case the nickname is empty or in case
    // user moved the footprint to a new library.  Chances are if footprint name is
    // same then the footprint is very nearly the same and the two texts should
    // be kept at same size, position, and rotation.
    if( aNewModule->GetFPID().GetLibItemName() == aOldModule->GetFPID().GetLibItemName() )
    {
        aNewModule->Reference().SetEffects( aOldModule->Reference() );
        aNewModule->Value().SetEffects( aOldModule->Value() );
    }

    // Updating other parameters
    aNewModule->SetTimeStamp( aOldModule->GetTimeStamp() );
    aNewModule->SetPath( aOldModule->GetPath() );

    aCommit.Remove( aOldModule );
    aCommit.Add( aNewModule );

    // @todo LEGACY should be unnecessary
    GetBoard()->m_Status_Pcb = 0;
    aNewModule->ClearFlags();
}


void DIALOG_EXCHANGE_FOOTPRINTS::ViewAndSelectFootprint( wxCommandEvent& event )
{
    wxString newname;

    KIWAY_PLAYER* frame = Kiway().Player( FRAME_PCB_MODULE_VIEWER_MODAL, true );

    if( frame->ShowModal( &newname, this ) )
    {
        if( event.GetEventObject() == m_newIDBrowseButton )
            m_newID->SetValue( newname );
        else
            m_specifiedID->SetValue( newname );
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
    wildcard = ComponentFileWildcard();

    wxString pro_dir = wxPathOnly( Prj().GetProjectFullName() );

    wxFileDialog dlg( this, _( "Save Footprint Association File" ), pro_dir,
                      fn.GetFullName(), wildcard,
                      wxFD_SAVE | wxFD_OVERWRITE_PROMPT );

    if( dlg.ShowModal() == wxID_CANCEL )
        return;

    fn = dlg.GetPath();

    if( ! RecreateCmpFile( GetBoard(), fn.GetFullPath() ) )
    {
        msg.Printf( _( "Could not create file \"%s\"" ), GetChars(fn.GetFullPath() ) );
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
        fprintf( cmpFile, "TimeStamp = %8.8lX\n", (unsigned long)module->GetTimeStamp() );
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

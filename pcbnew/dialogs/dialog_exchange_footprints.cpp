/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2013 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2013 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <board.h>
#include <footprint.h>
#include <pad.h>
#include <dialog_exchange_footprints.h>
#include <string_utils.h>
#include <kiway.h>
#include <kiway_express.h>
#include <macros.h>
#include <pcb_edit_frame.h>
#include <widgets/wx_html_report_panel.h>
#include <widgets/std_bitmap_button.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>

#include <ranges>


#define ID_MATCH_FP_ALL      4200
#define ID_MATCH_FP_SELECTED 4201
#define ID_MATCH_FP_REF      4202
#define ID_MATCH_FP_VAL      4203
#define ID_MATCH_FP_ID       4204

int g_matchModeForUpdate           = ID_MATCH_FP_ALL;
int g_matchModeForUpdateSelected   = ID_MATCH_FP_SELECTED;
int g_matchModeForExchange         = ID_MATCH_FP_REF;
int g_matchModeForExchangeSelected = ID_MATCH_FP_SELECTED;


DIALOG_EXCHANGE_FOOTPRINTS::DIALOG_EXCHANGE_FOOTPRINTS( PCB_EDIT_FRAME* aParent, FOOTPRINT* aFootprint,
                                                        bool updateMode, bool selectedMode ) :
        DIALOG_EXCHANGE_FOOTPRINTS_BASE( aParent ),
        m_commit( aParent ),
        m_parent( aParent ),
        m_currentFootprint( aFootprint ),
        m_updateMode( updateMode )
{
    if( !updateMode )
    {
        SetTitle( _( "Change Footprints" ) );
        m_matchAll->SetLabel( _( "Change all footprints on board" ) );
        m_matchSelected->SetLabel( _( "Change selected footprint(s)" ) );
        m_matchSpecifiedRef->SetLabel( _( "Change footprints matching reference designator:" ) );
        m_matchSpecifiedValue->SetLabel( _( "Change footprints matching value:" ) );
        m_matchSpecifiedID->SetLabel( _( "Change footprints with library id:" ) );
        m_resetTextItemLayers->SetLabel( _( "Update text layers and visibilities" ) );
        m_resetTextItemEffects->SetLabel( _( "Update text sizes and styles" ) );
        m_resetTextItemPositions->SetLabel( _( "Update text positions" ) );
        m_resetTextItemContent->SetLabel( _( "Update text content" ) );
        m_resetFabricationAttrs->SetLabel( _( "Update fabrication attributes" ) );
        m_resetClearanceOverrides->SetLabel( _( "Update clearance overrides" ) );
        m_reset3DModels->SetLabel( _( "Update 3D models" ) );
    }

#if 0  // translator hint
    wxString x = _( "Update/reset strings: there are two cases these descriptions need to cover: "
                    "the user made overrides to a footprint on the PCB and wants to remove them, "
                    "or the user made changes to the library footprint and wants to propagate "
                    "them back to the PCB." );
#endif

    if( m_updateMode )
        m_changeSizer->Show( false );
    else
        m_upperSizer->FindItem( m_matchAll )->Show( false );

    if( !m_currentFootprint )
        m_upperSizer->FindItem( m_matchSelected )->Show( false );

    m_newIDBrowseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_library ) );
    m_specifiedIDBrowseButton->SetBitmap( KiBitmapBundle( BITMAPS::small_library ) );

    m_upperSizer->SetEmptyCellSize( wxSize( 0, 0 ) );
    // The upper sizer has its content modified: re-layout it:
    m_upperSizer->Layout();

    // initialize controls based on update mode in case there is no saved state yet
    m_removeExtraBox->SetValue(          m_updateMode ? false : false );
    m_resetTextItemLayers->SetValue(     m_updateMode ? false : true  );
    m_resetTextItemEffects->SetValue(    m_updateMode ? false : true  );
    m_resetTextItemPositions->SetValue(  m_updateMode ? false : true  );
    m_resetTextItemContent->SetValue(    m_updateMode ? false : true  );
    m_resetFabricationAttrs->SetValue(   m_updateMode ? false : true  );
    m_resetClearanceOverrides->SetValue( m_updateMode ? true  : true  );
    m_reset3DModels->SetValue(           m_updateMode ? true  : true  );

    // initialize match-mode
    if( m_updateMode )
        m_matchMode = selectedMode ? &g_matchModeForUpdateSelected : &g_matchModeForUpdate;
    else
        m_matchMode = selectedMode ? &g_matchModeForExchangeSelected : &g_matchModeForExchange;

    m_MessageWindow->SetLazyUpdate( true );
    m_MessageWindow->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );

    // DIALOG_SHIM needs a unique hash_key because classname is not sufficient
    // because the update and change versions of this dialog have different controls.
    m_hash_key = TO_UTF8( GetTitle() );

    wxString okLabel = m_updateMode ? _( "Update" ) : _( "Change" );

    SetupStandardButtons( { { wxID_OK,     okLabel      },
                            { wxID_CANCEL, _( "Close" ) } } );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


bool DIALOG_EXCHANGE_FOOTPRINTS::TransferDataToWindow()
{
    if( m_currentFootprint )
    {
        if( m_updateMode )
            m_newID->ChangeValue( From_UTF8( m_currentFootprint->GetFPID().Format().c_str() ) );

        // Use ChangeValue() instead of SetValue() so we don't generate events.
        m_specifiedRef->ChangeValue( m_currentFootprint->GetReference() );
        m_specifiedValue->ChangeValue( m_currentFootprint->GetValue() );
        m_specifiedID->ChangeValue( From_UTF8( m_currentFootprint->GetFPID().Format().c_str() ) );
    }

    wxCommandEvent event;
    event.SetEventObject( this );

    switch( *m_matchMode )
    {
    case ID_MATCH_FP_ALL:      OnMatchAllClicked( event );      break;
    case ID_MATCH_FP_SELECTED: OnMatchSelectedClicked( event ); break;
    case ID_MATCH_FP_REF:      OnMatchRefClicked( event );      break;
    case ID_MATCH_FP_VAL:      OnMatchValueClicked( event );    break;
    case ID_MATCH_FP_ID:       OnMatchIDClicked( event );       break;
    default:                                                    break;
    }

    return true;
}


bool DIALOG_EXCHANGE_FOOTPRINTS::isMatch( FOOTPRINT* aFootprint )
{
    LIB_ID specifiedID;

    switch( *m_matchMode )
    {
    case ID_MATCH_FP_ALL:
        return true;
    case ID_MATCH_FP_SELECTED:
        return aFootprint == m_currentFootprint || aFootprint->IsSelected();
    case ID_MATCH_FP_REF:
        return WildCompareString( m_specifiedRef->GetValue(), aFootprint->GetReference(), false );
    case ID_MATCH_FP_VAL:
        return WildCompareString( m_specifiedValue->GetValue(), aFootprint->GetValue(), false );
    case ID_MATCH_FP_ID:
        specifiedID.Parse( m_specifiedID->GetValue() );
        return aFootprint->GetFPID() == specifiedID;
    default:
        return false;   // just to quiet compiler warnings....
    }
}


wxRadioButton* DIALOG_EXCHANGE_FOOTPRINTS::getRadioButtonForMode()
{
    switch( *m_matchMode )
    {
    case ID_MATCH_FP_ALL:      return m_matchAll;
    case ID_MATCH_FP_SELECTED: return m_matchSelected;
    case ID_MATCH_FP_REF:      return m_matchSpecifiedRef;
    case ID_MATCH_FP_VAL:      return m_matchSpecifiedValue;
    case ID_MATCH_FP_ID:       return m_matchSpecifiedID;
    default:                   return nullptr;
    }
}


void DIALOG_EXCHANGE_FOOTPRINTS::updateMatchModeRadioButtons( wxUpdateUIEvent& )
{
    wxRadioButton* rb_button = getRadioButtonForMode();

    wxRadioButton* rb_butt_list[] =
    {
        m_matchAll,
        m_matchSelected,
        m_matchSpecifiedRef,
        m_matchSpecifiedValue,
        m_matchSpecifiedID,
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


void DIALOG_EXCHANGE_FOOTPRINTS::OnMatchAllClicked( wxCommandEvent& aEvent )
{
    *m_matchMode = ID_MATCH_FP_ALL;

    if( aEvent.GetEventObject() == this )
        SetInitialFocus( m_matchAll );
    else
        m_matchAll->SetFocus();
}


void DIALOG_EXCHANGE_FOOTPRINTS::OnMatchSelectedClicked( wxCommandEvent& aEvent )
{
    *m_matchMode = ID_MATCH_FP_SELECTED;

    if( aEvent.GetEventObject() == this )
        SetInitialFocus( m_matchSelected );
    else
        m_matchSelected->SetFocus();
}


void DIALOG_EXCHANGE_FOOTPRINTS::OnMatchRefClicked( wxCommandEvent& aEvent )
{
    *m_matchMode = ID_MATCH_FP_REF;

    if( aEvent.GetEventObject() == this )
        SetInitialFocus( m_specifiedRef );
    else if( aEvent.GetEventObject() != m_specifiedRef )
        m_specifiedRef->SetFocus();
}


void DIALOG_EXCHANGE_FOOTPRINTS::OnMatchValueClicked( wxCommandEvent& aEvent )
{
    *m_matchMode = ID_MATCH_FP_VAL;

    if( aEvent.GetEventObject() == this )
        SetInitialFocus( m_specifiedValue );
    else if( aEvent.GetEventObject() != m_specifiedValue )
        m_specifiedValue->SetFocus();
}


void DIALOG_EXCHANGE_FOOTPRINTS::OnMatchIDClicked( wxCommandEvent& aEvent )
{
    *m_matchMode = ID_MATCH_FP_ID;

    if( aEvent.GetEventObject() == this )
        SetInitialFocus( m_specifiedID );
    else if( aEvent.GetEventObject() != m_specifiedID )
        m_specifiedID->SetFocus();
}


void DIALOG_EXCHANGE_FOOTPRINTS::checkAll( bool aCheck )
{
    m_removeExtraBox->SetValue( aCheck );
        m_resetTextItemLayers->SetValue( aCheck );
        m_resetTextItemEffects->SetValue( aCheck );
        m_resetTextItemPositions->SetValue( aCheck );
        m_resetTextItemContent->SetValue( aCheck );
        m_resetFabricationAttrs->SetValue( aCheck );
        m_resetClearanceOverrides->SetValue( aCheck );
        m_reset3DModels->SetValue( aCheck );
}


void DIALOG_EXCHANGE_FOOTPRINTS::OnOKClicked( wxCommandEvent& aEvent )
{
    PCB_SELECTION_TOOL* selTool = m_parent->GetToolManager()->GetTool<PCB_SELECTION_TOOL>();
    wxBusyCursor        dummy;

    m_MessageWindow->Clear();
    m_MessageWindow->Flush( false );

    m_newFootprints.clear();
    processMatchingFootprints();
    m_commit.Push( m_updateMode ? _( "Update Footprint" ) : _( "Change Footprint" ) );
    selTool->AddItemsToSel( &m_newFootprints );

    m_MessageWindow->Flush( false );

    WINDOW_THAWER thawer( m_parent );
    m_parent->GetCanvas()->Refresh();
}


void DIALOG_EXCHANGE_FOOTPRINTS::processMatchingFootprints()
{
    LIB_ID   newFPID;

    if( m_parent->GetBoard()->Footprints().empty() )
        return;

    if( !m_updateMode )
    {
        newFPID.Parse( m_newID->GetValue() );

        if( !newFPID.IsValid() )
            return;
    }

    /*
     * NB: the change is done from the last footprint because processFootprint() modifies the
     * last item in the list.
     */
    for( FOOTPRINT* footprint : std::ranges::reverse_view( m_parent->GetBoard()->Footprints() ) )
    {
        if( !isMatch( footprint ) )
            continue;

        if( m_updateMode )
            processFootprint( footprint, footprint->GetFPID() );
        else
            processFootprint( footprint, newFPID );
    }
}


void DIALOG_EXCHANGE_FOOTPRINTS::processFootprint( FOOTPRINT* aFootprint, const LIB_ID& aNewFPID )
{
    LIB_ID    oldFPID = aFootprint->GetFPID();
    wxString  msg;

    // Load new footprint.
    if( m_updateMode )
    {
        msg.Printf( _( "Updated footprint %s (%s)" ) + wxS( ": " ),
                    aFootprint->GetReference(),
                    oldFPID.Format().c_str() );
    }
    else
    {
        msg.Printf( _( "Changed footprint %s from '%s' to '%s'" ) + wxS( ": " ),
                    aFootprint->GetReference(),
                    oldFPID.Format().c_str(),
                    aNewFPID.Format().c_str() );
    }

    FOOTPRINT* newFootprint = m_parent->LoadFootprint( aNewFPID );

    if( !newFootprint )
    {
        msg += _( "*** library footprint not found ***" );
        m_MessageWindow->Report( msg, RPT_SEVERITY_ERROR );
        return;
    }

    bool updated = !m_updateMode || aFootprint->FootprintNeedsUpdate( newFootprint );

    m_parent->ExchangeFootprint( aFootprint, newFootprint, m_commit,
                                 m_removeExtraBox->GetValue(),
                                 m_resetTextItemLayers->GetValue(),
                                 m_resetTextItemEffects->GetValue(),
                                 m_resetTextItemPositions->GetValue(),
                                 m_resetTextItemContent->GetValue(),
                                 m_resetFabricationAttrs->GetValue(),
                                 m_resetClearanceOverrides->GetValue(),
                                 m_reset3DModels->GetValue(),
                                 &updated );

    if( aFootprint == m_currentFootprint )
        m_currentFootprint = newFootprint;

    if( newFootprint )
        m_newFootprints.push_back( newFootprint );

    if( m_updateMode && !updated )
    {
        msg += _( ": (no changes)" );
        m_MessageWindow->Report( msg, RPT_SEVERITY_INFO );
    }
    else
    {
        msg += _( ": OK" );
        m_MessageWindow->Report( msg, RPT_SEVERITY_ACTION );
    }
}


void DIALOG_EXCHANGE_FOOTPRINTS::ViewAndSelectFootprint( wxCommandEvent& event )
{
    wxString newname = m_newID->GetValue();

    if( KIWAY_PLAYER* frame = Kiway().Player( FRAME_FOOTPRINT_CHOOSER, true, this ) )
    {
        if( m_currentFootprint )
        {
            /*
             * Symbol netlist format:
             *   pinNumber pinName <tab> pinNumber pinName...
             *   fpFilter fpFilter...
             */
            wxString netlist;

            wxArrayString pins;

            for( const wxString& pad : m_currentFootprint->GetUniquePadNumbers() )
                pins.push_back( pad + ' ' + wxEmptyString /* leave pinName empty */ );

            if( !pins.IsEmpty() )
                netlist << EscapeString( wxJoin( pins, '\t' ), CTX_LINE );

            netlist << wxS( "\r" );

            netlist << EscapeString( m_currentFootprint->GetFilters(), CTX_LINE ) << wxS( "\r" );

            std::string payload( netlist.ToStdString() );
            KIWAY_EXPRESS mail( FRAME_FOOTPRINT_CHOOSER, MAIL_SYMBOL_NETLIST, payload );
            frame->KiwayMailIn( mail );
        }

        if( frame->ShowModal( &newname, this ) )
        {
            if( event.GetEventObject() == m_newIDBrowseButton )
                m_newID->SetValue( newname );
            else
                m_specifiedID->SetValue( newname );
        }

        frame->Destroy();
    }
}



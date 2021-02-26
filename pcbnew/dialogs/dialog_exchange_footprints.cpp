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

#include <bitmaps.h>
#include <board_commit.h>
#include <board.h>
#include <footprint.h>
#include <dialog_exchange_footprints.h>
#include <kicad_string.h>
#include <kiway.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <project.h>
#include <wx_html_report_panel.h>


#define ID_MATCH_FP_ALL      4200
#define ID_MATCH_FP_SELECTED 4201
#define ID_MATCH_FP_REF      4202
#define ID_MATCH_FP_VAL      4203
#define ID_MATCH_FP_ID       4204


int g_matchModeForUpdate           = ID_MATCH_FP_ALL;
int g_matchModeForUpdateSelected   = ID_MATCH_FP_SELECTED;
int g_matchModeForExchange         = ID_MATCH_FP_REF;
int g_matchModeForExchangeSelected = ID_MATCH_FP_SELECTED;

                               // { update, change }
bool g_removeExtraTextItems[2]  = { false,  false  };
bool g_resetTextItemLayers[2]   = { false,  true   };
bool g_resetTextItemEffects[2]  = { false,  true   };
bool g_resetFabricationAttrs[2] = { false,  true   };
bool g_reset3DModels[2]         = { true,   true   };


DIALOG_EXCHANGE_FOOTPRINTS::DIALOG_EXCHANGE_FOOTPRINTS( PCB_EDIT_FRAME* aParent,
                                                        FOOTPRINT* aFootprint,
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
        m_matchSelected->SetLabel( _( "Change selected footprint" ) );
        m_matchSpecifiedRef->SetLabel( _( "Change footprints matching reference designator:" ) );
        m_matchSpecifiedValue->SetLabel( _( "Change footprints matching value:" ) );
        m_matchSpecifiedID->SetLabel( _( "Change footprints with library id:" ) );
        m_resetTextItemLayers->SetLabel( _( "Update text layers and visibilities" ) );
        m_resetTextItemEffects->SetLabel( _( "Update text sizes, styles and positions" ) );
        m_resetFabricationAttrs->SetLabel( _( "Update fabrication attributes" ) );
        m_reset3DModels->SetLabel( _( "Update 3D models" ) );
    }

#if 0  // translator hint
    wxString x = _( "Update/reset strings: there are two cases these descriptions need to cover: "
                    "the user made overrides to a footprint on the PCB and wants to remove them, "
                    "or the user made changes to the library footprint and wants to propagate "
                    "them back to the PCB." );
#endif

    if( m_updateMode )
    {
        m_changeSizer->Show( false );
    }
    else
    {
        m_upperSizer->FindItem( m_matchAll )->Show( false );
        m_newIDBrowseButton->SetBitmap( KiBitmap( small_library_xpm ) );
    }

    if( m_currentFootprint )
    {
        m_newID->AppendText( FROM_UTF8( m_currentFootprint->GetFPID().Format().c_str() ) );
    }
    else
        m_upperSizer->FindItem( m_matchSelected )->Show( false );

    // Use ChangeValue() instead of SetValue() so we don't generate events.
    if( m_currentFootprint )
        m_specifiedRef->ChangeValue( m_currentFootprint->GetReference() );

    if( m_currentFootprint )
        m_specifiedValue->ChangeValue( m_currentFootprint->GetValue() );

    if( m_currentFootprint )
        m_specifiedID->ChangeValue( FROM_UTF8( m_currentFootprint->GetFPID().Format().c_str() ) );

    m_specifiedIDBrowseButton->SetBitmap( KiBitmap( small_library_xpm ) );

    m_upperSizer->SetEmptyCellSize( wxSize( 0, 0 ) );
    // The upper sizer has its content modified: re-layout it:
    m_upperSizer->Layout();

    // initialize match-mode
    if( m_updateMode )
        m_matchMode = selectedMode ? &g_matchModeForUpdateSelected : &g_matchModeForUpdate;
    else
        m_matchMode = selectedMode ? &g_matchModeForExchangeSelected : &g_matchModeForExchange;

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

    m_removeExtraBox->SetValue( g_removeExtraTextItems[ m_updateMode ? 0 : 1 ] );
    m_resetTextItemLayers->SetValue( g_resetTextItemLayers[ m_updateMode ? 0 : 1 ] );
    m_resetTextItemEffects->SetValue( g_resetTextItemEffects[ m_updateMode ? 0 : 1 ] );
    m_resetFabricationAttrs->SetValue( g_resetFabricationAttrs[ m_updateMode ? 0 : 1 ] );
    m_reset3DModels->SetValue( g_reset3DModels[ m_updateMode ? 0 : 1 ] );

    m_MessageWindow->SetLazyUpdate( true );
    m_MessageWindow->SetFileName( Prj().GetProjectPath() + wxT( "report.txt" ) );

    // DIALOG_SHIM needs a unique hash_key because classname is not sufficient
    // because the update and change versions of this dialog have different controls.
    m_hash_key = TO_UTF8( GetTitle() );

    // Ensure m_closeButton (with id = wxID_CANCEL) has the right label
    // (to fix automatic renaming of button label )
    m_sdbSizerCancel->SetLabel( _( "Close" ) );

    if( m_updateMode )
        m_sdbSizerOK->SetLabel( _( "Update" ) );
    else
        m_sdbSizerOK->SetLabel( _( "Change" ) );

    m_sdbSizerOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_EXCHANGE_FOOTPRINTS::~DIALOG_EXCHANGE_FOOTPRINTS()
{
    g_removeExtraTextItems[ m_updateMode ? 0 : 1 ]  = m_removeExtraBox->GetValue();
    g_resetTextItemLayers[ m_updateMode ? 0 : 1 ]   = m_resetTextItemLayers->GetValue();
    g_resetTextItemEffects[ m_updateMode ? 0 : 1 ]  = m_resetTextItemEffects->GetValue();
    g_resetFabricationAttrs[ m_updateMode ? 0 : 1 ] = m_resetFabricationAttrs->GetValue();
    g_reset3DModels[ m_updateMode ? 0 : 1 ]         = m_reset3DModels->GetValue();
}


bool DIALOG_EXCHANGE_FOOTPRINTS::isMatch( FOOTPRINT* aFootprint )
{
    LIB_ID specifiedID;

    switch( *m_matchMode )
    {
    case ID_MATCH_FP_ALL:
        return true;
    case ID_MATCH_FP_SELECTED:
        return aFootprint == m_currentFootprint;
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


void DIALOG_EXCHANGE_FOOTPRINTS::OnMatchAllClicked( wxCommandEvent& event )
{
    *m_matchMode = ID_MATCH_FP_ALL;

    if( event.GetEventObject() == this )
        SetInitialFocus( m_matchAll );
    else
        m_matchAll->SetFocus();
}


void DIALOG_EXCHANGE_FOOTPRINTS::OnMatchSelectedClicked( wxCommandEvent& event )
{
    *m_matchMode = ID_MATCH_FP_SELECTED;

    if( event.GetEventObject() == this )
        SetInitialFocus( m_matchSelected );
    else
        m_matchSelected->SetFocus();
}


void DIALOG_EXCHANGE_FOOTPRINTS::OnMatchRefClicked( wxCommandEvent& event )
{
    *m_matchMode = ID_MATCH_FP_REF;

    if( event.GetEventObject() == this )
        SetInitialFocus( m_specifiedRef );
    else if( event.GetEventObject() != m_specifiedRef )
        m_specifiedRef->SetFocus();
}


void DIALOG_EXCHANGE_FOOTPRINTS::OnMatchValueClicked( wxCommandEvent& event )
{
    *m_matchMode = ID_MATCH_FP_VAL;

    if( event.GetEventObject() == this )
        SetInitialFocus( m_specifiedValue );
    else if( event.GetEventObject() != m_specifiedValue )
        m_specifiedValue->SetFocus();
}


void DIALOG_EXCHANGE_FOOTPRINTS::OnMatchIDClicked( wxCommandEvent& event )
{
    *m_matchMode = ID_MATCH_FP_ID;

    if( event.GetEventObject() == this )
        SetInitialFocus( m_specifiedID );
    else if( event.GetEventObject() != m_specifiedID )
        m_specifiedID->SetFocus();
}


void DIALOG_EXCHANGE_FOOTPRINTS::OnOKClicked( wxCommandEvent& event )
{
    wxBusyCursor dummy;

    m_MessageWindow->Clear();
    m_MessageWindow->Flush( false );

    if( processMatchingFootprints() )
    {
        m_parent->Compile_Ratsnest( true );
        m_parent->GetCanvas()->Refresh();
    }

    m_MessageWindow->Flush( false );

    m_commit.Push( wxT( "Changed footprint" ) );
}


bool DIALOG_EXCHANGE_FOOTPRINTS::processMatchingFootprints()
{
    bool     change = false;
    LIB_ID   newFPID;
    wxString value;

    if( m_parent->GetBoard()->Footprints().empty() )
        return false;

    if( !m_updateMode )
    {
        newFPID.Parse( m_newID->GetValue() );

        if( !newFPID.IsValid() )
            return false;
    }

    /* The change is done from the last footprint because processFootprint() modifies the last
     * item in the list.
     */
    for( auto it = m_parent->GetBoard()->Footprints().rbegin();
            it != m_parent->GetBoard()->Footprints().rend(); it++ )
    {
        auto mod = *it;

        if( !isMatch( mod ) )
            continue;

        if( m_updateMode )
        {
            if( processFootprint( mod, mod->GetFPID() ) )
                change = true;
        }
        else
        {
            if( processFootprint( mod, newFPID ) )
                change = true;
        }
    }

    return change;
}


bool DIALOG_EXCHANGE_FOOTPRINTS::processFootprint( FOOTPRINT* aFootprint, const LIB_ID& aNewFPID )
{
    LIB_ID    oldFPID = aFootprint->GetFPID();
    wxString  msg;

    // Load new footprint.
    if( m_updateMode )
    {
        msg.Printf( _( "Update footprint %s from '%s' to '%s'" ),
                    aFootprint->GetReference(),
                    oldFPID.Format().c_str(),
                    aNewFPID.Format().c_str() );
    }
    else
    {
        msg.Printf( _( "Change footprint %s from '%s' to '%s'" ),
                    aFootprint->GetReference(),
                    oldFPID.Format().c_str(),
                    aNewFPID.Format().c_str() );
    }

    FOOTPRINT* newFootprint = m_parent->LoadFootprint( aNewFPID );

    if( !newFootprint )
    {
        msg << ": " << _( "*** footprint not found ***" );
        m_MessageWindow->Report( msg, RPT_SEVERITY_ERROR );
        return false;
    }

    m_parent->ExchangeFootprint( aFootprint, newFootprint, m_commit,
                                 m_removeExtraBox->GetValue(),
                                 m_resetTextItemLayers->GetValue(),
                                 m_resetTextItemEffects->GetValue(),
                                 m_resetFabricationAttrs->GetValue(),
                                 m_reset3DModels->GetValue() );

    if( aFootprint == m_currentFootprint )
        m_currentFootprint = newFootprint;

    msg += ": OK";
    m_MessageWindow->Report( msg, RPT_SEVERITY_ACTION );

    return true;
}


void processTextItem( const FP_TEXT& aSrc, FP_TEXT& aDest,
                      bool resetText, bool resetTextLayers, bool resetTextEffects )
{
    if( !resetText )
        aDest.SetText( aSrc.GetText() );

    if( !resetTextLayers )
    {
        aDest.SetLayer( aSrc.GetLayer() );
        aDest.SetVisible( aSrc.IsVisible() );
    }

    if( !resetTextEffects )
    {
        // Careful: the visible bit is also in Effects
        bool visible = aDest.IsVisible();
        aDest.SetEffects( aSrc );
        aDest.SetVisible( visible );
    }
}


FP_TEXT* getMatchingTextItem( FP_TEXT* aRefItem, FOOTPRINT* aFootprint )
{
    std::vector<FP_TEXT*> candidates;

    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        FP_TEXT* candidate = dyn_cast<FP_TEXT*>( item );

        if( candidate && candidate->GetText() == aRefItem->GetText() )
            candidates.push_back( candidate );
    }

    if( candidates.size() == 0 )
        return nullptr;

    if( candidates.size() == 1 )
        return candidates[0];

    // Try refining the match by layer
    std::vector<FP_TEXT*> candidatesOnSameLayer;

    for( FP_TEXT* candidate : candidates )
    {
        if( candidate->GetLayer() == aRefItem->GetLayer() )
            candidatesOnSameLayer.push_back( candidate );
    }

    if( candidatesOnSameLayer.size() == 1 )
        return candidatesOnSameLayer[0];

    // Last ditch effort: refine by position
    std::vector<FP_TEXT*> candidatesAtSamePos;

    for( FP_TEXT* candidate : candidatesOnSameLayer.size() ? candidatesOnSameLayer : candidates )
    {
        if( candidate->GetPos0() == aRefItem->GetPos0() )
            candidatesAtSamePos.push_back( candidate );
    }

    if( candidatesAtSamePos.size() > 0 )
        return candidatesAtSamePos[0];
    else if( candidatesOnSameLayer.size() > 0 )
        return candidatesOnSameLayer[0];
    else
        return candidates[0];
}


void PCB_EDIT_FRAME::ExchangeFootprint( FOOTPRINT* aExisting, FOOTPRINT* aNew,
                                        BOARD_COMMIT& aCommit, bool deleteExtraTexts,
                                        bool resetTextLayers, bool resetTextEffects,
                                        bool resetFabricationAttrs, bool reset3DModels )
{
    PCB_GROUP* parentGroup = aExisting->GetParentGroup();

    if( parentGroup )
    {
        parentGroup->RemoveItem( aExisting );
        parentGroup->AddItem( aNew );
    }

    aNew->SetParent( GetBoard() );

    PlaceFootprint( aNew, false );

    // PlaceFootprint will move the footprint to the cursor position, which we don't want.  Copy
    // the original position across.
    aNew->SetPosition( aExisting->GetPosition() );

    if( aNew->GetLayer() != aExisting->GetLayer() )
        aNew->Flip( aNew->GetPosition(), m_settings->m_FlipLeftRight );

    if( aNew->GetOrientation() != aExisting->GetOrientation() )
        aNew->SetOrientation( aExisting->GetOrientation() );

    aNew->SetLocked( aExisting->IsLocked() );

    for( PAD* pad : aNew->Pads() )
    {
        PAD* oldPad = aExisting->FindPadByName( pad->GetName() );

        if( oldPad )
        {
            pad->SetLocalRatsnestVisible( oldPad->GetLocalRatsnestVisible() );
            pad->SetNetCode( oldPad->GetNetCode() );
            pad->SetPinFunction( oldPad->GetPinFunction() );
        }
    }

    // Copy reference
    processTextItem( aExisting->Reference(), aNew->Reference(),
                     // never reset reference text
                     false,
                     resetTextLayers, resetTextEffects );

    // Copy value
    processTextItem( aExisting->Value(), aNew->Value(),
                     // reset value text only when it is a proxy for the footprint ID
                     // (cf replacing value "MountingHole-2.5mm" with "MountingHole-4.0mm")
                     aExisting->GetValue() == aExisting->GetFPID().GetLibItemName(),
                     resetTextLayers, resetTextEffects );

    // Copy fields in accordance with the reset* flags
    for( BOARD_ITEM* item : aExisting->GraphicalItems() )
    {
        FP_TEXT* srcItem = dyn_cast<FP_TEXT*>( item );

        if( srcItem )
        {
            FP_TEXT* destItem = getMatchingTextItem( srcItem, aNew );

            if( destItem )
                processTextItem( *srcItem, *destItem, false, resetTextLayers, resetTextEffects );
            else if( !deleteExtraTexts )
                aNew->Add( new FP_TEXT( *srcItem ) );
        }
    }

    if( !resetFabricationAttrs )
        aNew->SetAttributes( aExisting->GetAttributes() );

    // Copy 3D model settings in accordance with the reset* flag
    if( !reset3DModels )
        aNew->Models() = aExisting->Models();  // Linked list of 3D models.

    // Updating other parameters
    const_cast<KIID&>( aNew->m_Uuid ) = aExisting->m_Uuid;
    aNew->SetProperties( aExisting->GetProperties() );
    aNew->SetPath( aExisting->GetPath() );

    aCommit.Remove( aExisting );
    aCommit.Add( aNew );

    aNew->ClearFlags();
}


void DIALOG_EXCHANGE_FOOTPRINTS::ViewAndSelectFootprint( wxCommandEvent& event )
{
    wxString newname = m_newID->GetValue();

    KIWAY_PLAYER* frame = Kiway().Player( FRAME_FOOTPRINT_VIEWER_MODAL, true );

    if( frame->ShowModal( &newname, this ) )
    {
        if( event.GetEventObject() == m_newIDBrowseButton )
            m_newID->SetValue( newname );
        else
            m_specifiedID->SetValue( newname );
    }

    frame->Destroy();
}



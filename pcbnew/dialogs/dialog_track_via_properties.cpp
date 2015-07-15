/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <dialogs/dialog_track_via_properties.h>
#include <class_pcb_layer_box_selector.h>
#include <tools/selection_tool.h>
#include <class_track.h>
#include <wxPcbStruct.h>
#include <confirm.h>

DIALOG_TRACK_VIA_PROPERTIES::DIALOG_TRACK_VIA_PROPERTIES( PCB_BASE_FRAME* aParent, const SELECTION& aItems ) :
    DIALOG_TRACK_VIA_PROPERTIES_BASE( aParent ), m_items( aItems ),
    m_trackStartX( aParent, m_TrackStartXCtrl, m_TrackStartXUnit ),
    m_trackStartY( aParent, m_TrackStartYCtrl, m_TrackStartYUnit ),
    m_trackEndX( aParent, m_TrackEndXCtrl, m_TrackEndXUnit ),
    m_trackEndY( aParent, m_TrackEndYCtrl, m_TrackEndYUnit ),
    m_trackWidth( aParent, m_TrackWidthCtrl, m_TrackWidthUnit ),
    m_viaX( aParent, m_ViaXCtrl, m_ViaXUnit ), m_viaY( aParent, m_ViaYCtrl, m_ViaYUnit ),
    m_viaDiameter( aParent, m_ViaDiameterCtrl, m_ViaDiameterUnit ),
    m_viaDrill( aParent, m_ViaDrillCtrl, m_ViaDrillUnit ),
    m_tracks( false ), m_vias( false )
{
    assert( !m_items.Empty() );

    boost::optional<int> trackStartX, trackStartY, trackEndX, trackEndY, trackWidth;
    boost::optional<LAYER_ID> trackLayer;
    boost::optional<int> viaX, viaY, viaDiameter, viaDrill;

    // Look for values that are common for every item that is selected
    for( int i = 0; i < m_items.Size(); ++i )
    {
        const BOARD_ITEM* item = m_items.Item<BOARD_ITEM>( i );

        switch( item->Type() )
        {
            case PCB_TRACE_T:
            {
                const TRACK* t = static_cast<const TRACK*>( item );

                if( !m_tracks )     // first track in the list
                {
                    trackStartX = t->GetStart().x;
                    trackStartY = t->GetStart().y;
                    trackEndX   = t->GetEnd().x;
                    trackEndY   = t->GetEnd().y;
                    trackWidth  = t->GetWidth();
                    trackLayer  = t->GetLayer();
                    m_tracks    = true;
                }
                else        // check if values are the same for every selected track
                {
                    if( trackStartX && *trackStartX != t->GetStart().x )
                        trackStartX = boost::none;

                    if( trackStartY && *trackStartY != t->GetStart().y )
                        trackStartY = boost::none;

                    if( trackEndX && *trackEndX != t->GetEnd().x )
                        trackEndX = boost::none;

                    if( trackEndY && *trackEndY != t->GetEnd().y )
                        trackEndY = boost::none;

                    if( trackWidth && *trackWidth != t->GetWidth() )
                        trackWidth = boost::none;

                    if( trackLayer && *trackLayer != t->GetLayer() )
                        trackLayer = boost::none;
                }
                break;
            }

            case PCB_VIA_T:
            {
                const VIA* v = static_cast<const VIA*>( item );

                if( !m_vias )       // first via in the list
                {
                    viaX = v->GetPosition().x;
                    viaY = v->GetPosition().y;
                    viaDiameter = v->GetWidth();
                    viaDrill = v->GetDrillValue();
                    m_vias = true;
                }
                else        // check if values are the same for every selected via
                {
                    if( viaX && *viaX != v->GetPosition().x )
                        viaX = boost::none;

                    if( viaY && *viaY != v->GetPosition().y )
                        viaY = boost::none;

                    if( viaDiameter && *viaDiameter != v->GetWidth() )
                        viaDiameter = boost::none;

                    if( viaDrill && *viaDrill != v->GetDrillValue() )
                        viaDrill = boost::none;
                }
                break;
            }

            default:
                assert( false );
                break;
        }
    }

    assert( m_tracks || m_vias );

    if( m_vias )
    {
        setCommonVal( viaX, m_ViaXCtrl, m_viaX );
        setCommonVal( viaY, m_ViaYCtrl, m_viaY );
        setCommonVal( viaDiameter, m_ViaDiameterCtrl, m_viaDiameter );
        setCommonVal( viaDrill, m_ViaDrillCtrl, m_viaDrill );
        m_ViaDiameterCtrl->SetFocus();
    }
    else
    {
        // you cannot access sizers directly if the code was generated by wxFormBuilder
        wxSizer* s = m_viaStaticLine->GetContainingSizer();
        m_mainSizerAccessor->GetContainingSizer()->Hide( s, true );
    }

    if( m_tracks )
    {
        setCommonVal( trackStartX, m_TrackStartXCtrl, m_trackStartX );
        setCommonVal( trackStartY, m_TrackStartYCtrl, m_trackStartY );
        setCommonVal( trackEndX, m_TrackEndXCtrl, m_trackEndX );
        setCommonVal( trackEndY, m_TrackEndYCtrl, m_trackEndY );
        setCommonVal( trackWidth, m_TrackWidthCtrl, m_trackWidth );

        m_TrackLayerCtrl->SetLayersHotkeys( false );
        m_TrackLayerCtrl->SetLayerSet( LSET::AllNonCuMask() );
        m_TrackLayerCtrl->SetBoardFrame( aParent );
        m_TrackLayerCtrl->Resync();

        if( trackLayer )
            m_TrackLayerCtrl->SetLayerSelection( *trackLayer );

        m_TrackWidthCtrl->SetFocus();
    }
    else
    {
        // you cannot access sizers directly if the code was generated by wxFormBuilder
        wxSizer* s = m_trackStaticLine->GetContainingSizer();
        m_mainSizerAccessor->GetContainingSizer()->Hide( s, true );
    }

    m_StdButtonsOK->SetDefault();

    Layout();
    Fit();

    // Pressing ENTER when any of the text input fields is active applies changes
    #if wxCHECK_VERSION( 3, 0, 0 )
        Connect( wxEVT_TEXT_ENTER, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES::onOkClick ), NULL, this );
    #else
        Connect( wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES::onOkClick ), NULL, this );
    #endif
}


bool DIALOG_TRACK_VIA_PROPERTIES::Apply()
{
    if( !check() )
        return false;

    for( int i = 0; i < m_items.Size(); ++i )
    {
        BOARD_ITEM* item = m_items.Item<BOARD_ITEM>( i );

        switch( item->Type() )
        {
            case PCB_TRACE_T:
            {
                assert( m_tracks );
                TRACK* t = static_cast<TRACK*>( item );

                if( m_trackStartX.Valid() || m_trackStartY.Valid() )
                {
                    wxPoint start = t->GetStart();

                    if( m_trackStartX.Valid() )
                        start.x = m_trackStartX.GetValue();

                    if( m_trackStartY.Valid() )
                        start.y = m_trackStartY.GetValue();

                    t->SetStart( start );
                }

                if( m_trackEndX.Valid() || m_trackEndY.Valid() )
                {
                    wxPoint end = t->GetEnd();

                    if( m_trackEndX.Valid() )
                        end.x = m_trackEndX.GetValue();

                    if( m_trackEndY.Valid() )
                        end.y = m_trackEndY.GetValue();

                    t->SetEnd( end );
                }

                if( m_trackNetclass->IsChecked() )
                {
                    t->SetWidth( t->GetNetClass()->GetTrackWidth() );
                }
                else if( m_trackWidth.Valid() )
                {
                    t->SetWidth( m_trackWidth.GetValue() );
                }

                LAYER_NUM layer = m_TrackLayerCtrl->GetLayerSelection();

                if( layer != UNDEFINED_LAYER )
                    t->SetLayer( (LAYER_ID) layer );

                break;
            }

            case PCB_VIA_T:
            {
                assert( m_vias );

                VIA* v = static_cast<VIA*>( item );

                if( m_viaX.Valid() || m_viaY.Valid() )
                {
                    wxPoint pos = v->GetPosition();

                    if( m_viaX.Valid() )
                        pos.x = m_viaX.GetValue();

                    if( m_viaY.Valid() )
                        pos.y = m_viaY.GetValue();

                    v->SetPosition( pos );
                }

                if( m_viaNetclass->IsChecked() )
                {
                    v->SetWidth( v->GetNetClass()->GetViaDiameter() );
                    v->SetDrill( v->GetNetClass()->GetViaDrill() );
                }
                else
                {
                    if( m_viaDiameter.Valid() )
                        v->SetWidth( m_viaDiameter.GetValue() );

                    if( m_viaDrill.Valid() )
                        v->SetDrill( m_viaDrill.GetValue() );
                }

                break;
            }

            default:
                assert( false );
                break;
        }
    }

    return true;
}


void DIALOG_TRACK_VIA_PROPERTIES::onClose( wxCloseEvent& aEvent )
{
    EndModal( 0 );
}


void DIALOG_TRACK_VIA_PROPERTIES::onTrackNetclassCheck( wxCommandEvent& aEvent )
{
    bool enableNC = aEvent.IsChecked();

    m_TrackWidthLabel->Enable( !enableNC );
    m_TrackWidthCtrl->Enable( !enableNC );
    m_TrackWidthUnit->Enable( !enableNC );
}


void DIALOG_TRACK_VIA_PROPERTIES::onViaNetclassCheck( wxCommandEvent& aEvent )
{
    bool enableNC = aEvent.IsChecked();

    m_ViaDiameterLabel->Enable( !enableNC );
    m_ViaDiameterCtrl->Enable( !enableNC );
    m_ViaDiameterUnit->Enable( !enableNC );

    m_ViaDrillLabel->Enable( !enableNC );
    m_ViaDrillCtrl->Enable( !enableNC );
    m_ViaDrillUnit->Enable( !enableNC );
}


void DIALOG_TRACK_VIA_PROPERTIES::onCancelClick( wxCommandEvent& aEvent )
{
    EndModal( 0 );
}


void DIALOG_TRACK_VIA_PROPERTIES::onOkClick( wxCommandEvent& aEvent )
{
    if( check() )
        EndModal( 1 );
}


bool DIALOG_TRACK_VIA_PROPERTIES::check() const
{
    bool trackNetclass = m_trackNetclass->IsChecked();
    bool viaNetclass = m_trackNetclass->IsChecked();

    if( m_tracks && !trackNetclass && m_trackWidth.Valid() && m_trackWidth.GetValue() <= 0 )
    {
        DisplayError( GetParent(), _( "Invalid track width" ) );
        m_TrackWidthCtrl->SetFocus();
        return false;
    }

    if( m_vias && !viaNetclass )
    {
        if( m_viaDiameter.Valid() && m_viaDiameter.GetValue() <= 0 )
        {
            DisplayError( GetParent(), _( "Invalid via diameter" ) );
            m_ViaDiameterCtrl->SetFocus();
            return false;
        }

        if( m_viaDrill.Valid() && m_viaDrill.GetValue() <= 0 )
        {
            DisplayError( GetParent(), _( "Invalid via drill size" ) );
            m_ViaDrillCtrl->SetFocus();
            return false;
        }

        if( m_viaDiameter.Valid() && m_viaDrill.Valid() && m_viaDiameter.GetValue() <= m_viaDrill.GetValue() )
        {
            DisplayError( GetParent(), _( "Via drill size has to be smaller than via diameter" ) );
            m_ViaDrillCtrl->SetFocus();
            return false;
        }
    }

    return true;
}

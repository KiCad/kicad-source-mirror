/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <kiface_i.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <class_zone.h>
#include <zones.h>
#include <zone_settings.h>
#include <dialog_keepout_area_properties_base.h>

#define LAYER_LIST_COLUMN_CHECK 0
#define LAYER_LIST_COLUMN_ICON 1
#define LAYER_LIST_COLUMN_NAME 2
#define LAYER_LIST_ROW_ALL_INNER_LAYERS 1

class DIALOG_KEEPOUT_AREA_PROPERTIES : public DIALOG_KEEPOUT_AREA_PROPERTIES_BASE
{
public:
    DIALOG_KEEPOUT_AREA_PROPERTIES( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings );

private:
    PCB_BASE_FRAME* m_parent;
    wxConfigBase*   m_config;               ///< Current config
    ZONE_SETTINGS   m_zonesettings;         ///< the working copy of zone settings
    ZONE_SETTINGS*  m_ptr;                  ///< the pointer to the zone settings
                                            ///< of the zone to edit

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnLayerSelection( wxDataViewEvent& event ) override;
};


int InvokeKeepoutAreaEditor( PCB_BASE_FRAME* aCaller, ZONE_SETTINGS* aSettings )
{
    DIALOG_KEEPOUT_AREA_PROPERTIES dlg( aCaller, aSettings );

    return dlg.ShowModal();
}


DIALOG_KEEPOUT_AREA_PROPERTIES::DIALOG_KEEPOUT_AREA_PROPERTIES( PCB_BASE_FRAME* aParent,
                                                                ZONE_SETTINGS* aSettings ) :
    DIALOG_KEEPOUT_AREA_PROPERTIES_BASE( aParent )
{
    m_parent = aParent;
    m_config = Kiface().KifaceSettings();

    m_ptr = aSettings;
    m_zonesettings = *aSettings;

    bool fpEditorMode = m_parent->IsType( FRAME_FOOTPRINT_EDITOR );
    m_zonesettings.SetupLayersList( m_layers, m_parent, true, fpEditorMode );

    m_sdbSizerButtonsOK->SetDefault();

    FinishDialogSettings();
}


bool DIALOG_KEEPOUT_AREA_PROPERTIES::TransferDataToWindow()
{
    // Init keepout parameters:
    m_cbTracksCtrl->SetValue( m_zonesettings.GetDoNotAllowTracks() );
    m_cbViasCtrl->SetValue( m_zonesettings.GetDoNotAllowVias() );
    m_cbCopperPourCtrl->SetValue( m_zonesettings.GetDoNotAllowCopperPour() );

    m_cbConstrainCtrl->SetValue( m_zonesettings.m_Zone_45_Only );

    switch( m_zonesettings.m_Zone_HatchingStyle )
    {
    case ZONE_CONTAINER::NO_HATCH:      m_OutlineAppearanceCtrl->SetSelection( 0 ); break;
    case ZONE_CONTAINER::DIAGONAL_EDGE: m_OutlineAppearanceCtrl->SetSelection( 1 ); break;
    case ZONE_CONTAINER::DIAGONAL_FULL: m_OutlineAppearanceCtrl->SetSelection( 2 ); break;
    }

    SetInitialFocus( m_OutlineAppearanceCtrl );

    return true;
}


void DIALOG_KEEPOUT_AREA_PROPERTIES::OnLayerSelection( wxDataViewEvent& event )
{
    if( event.GetColumn() != 0 )
        return;

    int row = m_layers->ItemToRow( event.GetItem() );
    wxVariant layerID;
    m_layers->GetValue( layerID, row, LAYER_LIST_COLUMN_NAME );
    bool selected = m_layers->GetToggleValue( row, LAYER_LIST_COLUMN_CHECK );

    if( row == LAYER_LIST_ROW_ALL_INNER_LAYERS )
    {
        if( selected )
            m_zonesettings.m_Layers |= LSET::InternalCuMask();
        else
            m_zonesettings.m_Layers &= ~LSET::InternalCuMask();
    }
    else
        m_zonesettings.m_Layers.set( ToLAYER_ID( layerID.GetInteger() ), selected );
}


bool DIALOG_KEEPOUT_AREA_PROPERTIES::TransferDataFromWindow()
{
    // Init keepout parameters:
    m_zonesettings.SetIsKeepout( true );
    m_zonesettings.SetDoNotAllowTracks( m_cbTracksCtrl->GetValue() );
    m_zonesettings.SetDoNotAllowVias( m_cbViasCtrl->GetValue() );
    m_zonesettings.SetDoNotAllowCopperPour( m_cbCopperPourCtrl->GetValue() );

    // Test for not allowed items: should have at least one item not allowed:
    if( ! m_zonesettings.GetDoNotAllowTracks() &&
        ! m_zonesettings.GetDoNotAllowVias() &&
        ! m_zonesettings.GetDoNotAllowCopperPour() )
    {
        DisplayError( NULL, _("Tracks, vias, and pads are allowed. The keepout will have no effect." ) );
        return false;
    }

    if( m_zonesettings.m_Layers.count() == 0 )
    {
        DisplayError( NULL, _( "No layers selected." ) );
        return false;
    }

    switch( m_OutlineAppearanceCtrl->GetSelection() )
    {
    case 0: m_zonesettings.m_Zone_HatchingStyle = ZONE_CONTAINER::NO_HATCH;      break;
    case 1: m_zonesettings.m_Zone_HatchingStyle = ZONE_CONTAINER::DIAGONAL_EDGE; break;
    case 2: m_zonesettings.m_Zone_HatchingStyle = ZONE_CONTAINER::DIAGONAL_FULL; break;
    }

    if( m_config )
        m_config->Write( ZONE_NET_OUTLINES_STYLE_KEY, (long) m_zonesettings.m_Zone_HatchingStyle );

    m_zonesettings.m_Zone_45_Only = m_cbConstrainCtrl->GetValue();
    m_zonesettings.m_ZonePriority = 0;  // for a keepout, this param is not used.

    *m_ptr = m_zonesettings;
    return true;
}



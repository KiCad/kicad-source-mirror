/**
 * @file dialog_non_copper_zones_properties.cpp
 */
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
#include <widgets/unit_binder.h>
#include <class_zone.h>
#include <zones.h>

#include <dialog_non_copper_zones_properties_base.h>


class DIALOG_NON_COPPER_ZONES_EDITOR : public DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE
{
private:
    PCB_BASE_FRAME* m_parent;
    ZONE_SETTINGS*  m_ptr;
    ZONE_SETTINGS   m_settings;     // working copy of zone settings
    UNIT_BINDER     m_minWidth;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnLayerSelection( wxDataViewEvent& event ) override;

public:
    DIALOG_NON_COPPER_ZONES_EDITOR( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings );
};


int InvokeNonCopperZonesEditor( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings )
{
    DIALOG_NON_COPPER_ZONES_EDITOR  dlg( aParent, aSettings );

    return dlg.ShowModal();
}


DIALOG_NON_COPPER_ZONES_EDITOR::DIALOG_NON_COPPER_ZONES_EDITOR( PCB_BASE_FRAME* aParent,
                                                                ZONE_SETTINGS* aSettings ) :
    DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE( aParent ),
    m_minWidth( aParent, m_MinWidthLabel, m_MinWidthCtrl, m_MinWidthUnits, true )
{
    m_parent = aParent;

    m_ptr  = aSettings;
    m_settings = *aSettings;
    m_settings.SetupLayersList( m_layers, m_parent, false );

    m_sdbSizerButtonsOK->SetDefault();

    FinishDialogSettings();
}


bool DIALOG_NON_COPPER_ZONES_EDITOR::TransferDataToWindow()
{
    m_minWidth.SetValue( m_settings.m_ZoneMinThickness );
    m_ConstrainOpt->SetValue( m_settings.m_Zone_45_Only );

    switch( m_settings.m_Zone_HatchingStyle )
    {
    case ZONE_CONTAINER::NO_HATCH:      m_OutlineAppearanceCtrl->SetSelection( 0 ); break;
    case ZONE_CONTAINER::DIAGONAL_EDGE: m_OutlineAppearanceCtrl->SetSelection( 1 ); break;
    case ZONE_CONTAINER::DIAGONAL_FULL: m_OutlineAppearanceCtrl->SetSelection( 2 ); break;
    }

    SetInitialFocus( m_OutlineAppearanceCtrl );

    return true;
}


void DIALOG_NON_COPPER_ZONES_EDITOR::OnLayerSelection( wxDataViewEvent& event )
{
    if( event.GetColumn() != 0 )
        return;

    int row = m_layers->ItemToRow( event.GetItem() );

    if( m_layers->GetToggleValue( row, 0 ) )
    {
        wxVariant layerID;
        m_layers->GetValue( layerID, row, 2 );
        m_settings.m_CurrentZone_Layer = ToLAYER_ID( layerID.GetInteger() );

        // Turn all other checkboxes off.
        for( int ii = 0; ii < m_layers->GetItemCount(); ++ii )
        {
            if( ii != row )
                m_layers->SetToggleValue( false, ii, 0 );
        }
    }
}


bool DIALOG_NON_COPPER_ZONES_EDITOR::TransferDataFromWindow()
{
    m_settings.m_ZoneMinThickness = m_minWidth.GetValue();

    m_settings.m_FillMode = ZFM_POLYGONS;  // Use always polygon fill mode

    switch( m_OutlineAppearanceCtrl->GetSelection() )
    {
    case 0: m_settings.m_Zone_HatchingStyle = ZONE_CONTAINER::NO_HATCH;      break;
    case 1: m_settings.m_Zone_HatchingStyle = ZONE_CONTAINER::DIAGONAL_EDGE; break;
    case 2: m_settings.m_Zone_HatchingStyle = ZONE_CONTAINER::DIAGONAL_FULL; break;
    }

    wxConfigBase* cfg = Kiface().KifaceSettings();
    wxASSERT( cfg );

    cfg->Write( ZONE_NET_OUTLINES_STYLE_KEY, (long) m_settings.m_Zone_HatchingStyle );

    m_settings.m_Zone_45_Only = m_ConstrainOpt->GetValue();

    // Get the layer selection for this zone
    int layer = -1;
    for( int ii = 0; ii < m_layers->GetItemCount(); ++ii )
    {
        if( m_layers->GetToggleValue( (unsigned) ii, 0 ) )
        {
            layer = ii;
            break;
        }
    }

    if( layer < 0 )
    {
        DisplayError( this, _( "No layer selected." ) );
        return false;
    }

    *m_ptr = m_settings;
    return true;
}



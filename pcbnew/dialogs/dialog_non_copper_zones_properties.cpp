/**
 * @file dialog_non_copper_zones_properties.cpp
 */
/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <kiface_i.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <widgets/unit_binder.h>
#include <zones.h>

#include <dialog_non_copper_zones_properties_base.h>


class DIALOG_NON_COPPER_ZONES_EDITOR : public DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE
{
private:
    PCB_BASE_FRAME* m_parent;
    ZONE_SETTINGS*  m_ptr;
    ZONE_SETTINGS   m_settings;     // working copy of zone settings
    UNIT_BINDER     m_minWidth;
    UNIT_BINDER     m_gridStyleRotation;
    UNIT_BINDER     m_gridStyleThickness;
    UNIT_BINDER     m_gridStyleGap;
    int             m_cornerSmoothingType;
    UNIT_BINDER     m_cornerRadius;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnStyleSelection( wxCommandEvent& event ) override;
    void OnLayerSelection( wxDataViewEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& ) override;

public:
    DIALOG_NON_COPPER_ZONES_EDITOR( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings );
};


int InvokeNonCopperZonesEditor( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings )
{
    DIALOG_NON_COPPER_ZONES_EDITOR  dlg( aParent, aSettings );

    return dlg.ShowQuasiModal();
}

#define MIN_THICKNESS 10*IU_PER_MILS

DIALOG_NON_COPPER_ZONES_EDITOR::DIALOG_NON_COPPER_ZONES_EDITOR( PCB_BASE_FRAME* aParent,
                                                                ZONE_SETTINGS* aSettings ) :
    DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE( aParent ),
    m_minWidth( aParent, m_MinWidthLabel, m_MinWidthCtrl, m_MinWidthUnits ),
    m_gridStyleRotation( aParent, m_staticTextGrindOrient, m_tcGridStyleOrientation, m_staticTextRotUnits ),
    m_gridStyleThickness( aParent, m_staticTextStyleThickness, m_tcGridStyleThickness, m_GridStyleThicknessUnits),
    m_gridStyleGap( aParent, m_staticTextGridGap, m_tcGridStyleGap, m_GridStyleGapUnits ),
    m_cornerSmoothingType( ZONE_SETTINGS::SMOOTHING_UNDEFINED ),
    m_cornerRadius( aParent, m_cornerRadiusLabel, m_cornerRadiusCtrl, m_cornerRadiusUnits )
{
    m_parent = aParent;

    m_ptr  = aSettings;
    m_settings = *aSettings;
    m_settings.SetupLayersList( m_layers, m_parent, false );

    m_sdbSizerButtonsOK->SetDefault();

    finishDialogSettings();
}


void DIALOG_NON_COPPER_ZONES_EDITOR::OnUpdateUI( wxUpdateUIEvent& )
{
    if( m_cornerSmoothingType != m_cornerSmoothingChoice->GetSelection() )
    {
        m_cornerSmoothingType = m_cornerSmoothingChoice->GetSelection();

        if( m_cornerSmoothingChoice->GetSelection() == ZONE_SETTINGS::SMOOTHING_CHAMFER )
            m_cornerRadiusLabel->SetLabel( _( "Chamfer distance:" ) );
        else
            m_cornerRadiusLabel->SetLabel( _( "Fillet radius:" ) );
    }

    m_cornerRadiusCtrl->Enable(m_cornerSmoothingType > ZONE_SETTINGS::SMOOTHING_NONE );
}


bool DIALOG_NON_COPPER_ZONES_EDITOR::TransferDataToWindow()
{
    m_cornerSmoothingChoice->SetSelection( m_settings.GetCornerSmoothingType() );
    m_cornerRadius.SetValue( m_settings.GetCornerRadius() );

    m_minWidth.SetValue( m_settings.m_ZoneMinThickness );
    m_ConstrainOpt->SetValue( m_settings.m_Zone_45_Only );

    switch( m_settings.m_ZoneBorderDisplayStyle )
    {
    case ZONE_BORDER_DISPLAY_STYLE::NO_HATCH:      m_OutlineDisplayCtrl->SetSelection( 0 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE: m_OutlineDisplayCtrl->SetSelection( 1 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL: m_OutlineDisplayCtrl->SetSelection( 2 ); break;
    }

    SetInitialFocus( m_OutlineDisplayCtrl );

    switch( m_settings.m_FillMode )
    {
    case ZONE_FILL_MODE::HATCH_PATTERN: m_GridStyleCtrl->SetSelection( 1 ); break;
    default:                            m_GridStyleCtrl->SetSelection( 0 ); break;
    }

    m_gridStyleRotation.SetUnits( EDA_UNITS::DEGREES );
    m_gridStyleRotation.SetValue( m_settings.m_HatchOrientation * 10 ); // IU is decidegree

    // Gives a reasonable value to grid style parameters, if currently there are no defined
    // parameters for grid pattern thickness and gap (if the value is 0)
    // the grid pattern thickness default value is (arbitrary) m_ZoneMinThickness * 4
    // or 1mm
    // the grid pattern gap default value is (arbitrary) m_ZoneMinThickness * 6
    // or 1.5 mm
    int bestvalue = m_settings.m_HatchThickness;

    if( bestvalue <= 0 )     // No defined value for m_hatchThickness
        bestvalue = std::max( m_settings.m_ZoneMinThickness * 4, Millimeter2iu( 1.0 ) );

    m_gridStyleThickness.SetValue( std::max( bestvalue, m_settings.m_ZoneMinThickness ) );

    bestvalue = m_settings.m_HatchGap;

    if( bestvalue <= 0 )     // No defined value for m_hatchGap
        bestvalue = std::max( m_settings.m_ZoneMinThickness * 6, Millimeter2iu( 1.5 ) );

    m_gridStyleGap.SetValue( std::max( bestvalue, m_settings.m_ZoneMinThickness ) );

    m_spinCtrlSmoothLevel->SetValue( m_settings.m_HatchSmoothingLevel );
    m_spinCtrlSmoothValue->SetValue( m_settings.m_HatchSmoothingValue );

    // Enable/Disable some widgets
    wxCommandEvent event;
    OnStyleSelection( event );

    return true;
}


void DIALOG_NON_COPPER_ZONES_EDITOR::OnStyleSelection( wxCommandEvent& event )
{
    bool enable = m_GridStyleCtrl->GetSelection() >= 1;
    m_tcGridStyleThickness->Enable( enable );
    m_tcGridStyleGap->Enable( enable );
    m_tcGridStyleOrientation->Enable( enable );
    m_spinCtrlSmoothLevel->Enable( enable );
    m_spinCtrlSmoothValue->Enable( enable );
}


void DIALOG_NON_COPPER_ZONES_EDITOR::OnLayerSelection( wxDataViewEvent& event )
{
    if( event.GetColumn() != 0 )
        return;

    int row  = m_layers->ItemToRow( event.GetItem() );
    bool val = m_layers->GetToggleValue( row, 0 );

    wxVariant layerID;
    m_layers->GetValue( layerID, row, 2 );
    m_settings.m_Layers.set( ToLAYER_ID( layerID.GetInteger() ), val );
}


bool DIALOG_NON_COPPER_ZONES_EDITOR::TransferDataFromWindow()
{
    m_settings.SetCornerSmoothingType( m_cornerSmoothingChoice->GetSelection() );

    m_settings.SetCornerRadius( m_settings.GetCornerSmoothingType() == ZONE_SETTINGS::SMOOTHING_NONE
                                ? 0 : m_cornerRadius.GetValue() );

    if( !m_gridStyleRotation.Validate( -1800, 1800 ) )
        return false;

    m_settings.m_ZoneMinThickness = m_minWidth.GetValue();

    switch( m_OutlineDisplayCtrl->GetSelection() )
    {
    case 0: m_settings.m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::NO_HATCH;      break;
    case 1: m_settings.m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE; break;
    case 2: m_settings.m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL; break;
    }

    if( m_GridStyleCtrl->GetSelection() > 0 )
        m_settings.m_FillMode = ZONE_FILL_MODE::HATCH_PATTERN;
    else
        m_settings.m_FillMode = ZONE_FILL_MODE::POLYGONS;


    if( m_settings.m_FillMode == ZONE_FILL_MODE::HATCH_PATTERN )
    {
        int minThickness = m_minWidth.GetValue();

        if( !m_gridStyleThickness.Validate( minThickness, INT_MAX ) )
            return false;

        if( !m_gridStyleGap.Validate( minThickness, INT_MAX ) )
            return false;
    }


    m_settings.m_HatchOrientation = m_gridStyleRotation.GetValue() / 10.0; // value is returned in deci-degree
    m_settings.m_HatchThickness = m_gridStyleThickness.GetValue();
    m_settings.m_HatchGap = m_gridStyleGap.GetValue();
    m_settings.m_HatchSmoothingLevel = m_spinCtrlSmoothLevel->GetValue();
    m_settings.m_HatchSmoothingValue = m_spinCtrlSmoothValue->GetValue();

    auto cfg = m_parent->GetPcbNewSettings();
    cfg->m_Zones.hatching_style = static_cast<int>( m_settings.m_ZoneBorderDisplayStyle );

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



/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <kiface_base.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <zone_settings.h>
#include <widgets/unit_binder.h>
#include <wx/statbox.h>
#include <wx/statline.h>
#include <wx/radiobut.h>
#include <zones.h>

#include <dialog_non_copper_zones_properties_base.h>


class DIALOG_NON_COPPER_ZONES_EDITOR : public DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE
{
public:
    DIALOG_NON_COPPER_ZONES_EDITOR( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings,
                                    CONVERT_SETTINGS* aConvertSettings );

    ~DIALOG_NON_COPPER_ZONES_EDITOR()
    { delete m_gap; }

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnStyleSelection( wxCommandEvent& event ) override;
    void OnLayerSelection( wxDataViewEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& ) override;

private:
    PCB_BASE_FRAME*   m_parent;
    ZONE_SETTINGS*    m_ptr;
    ZONE_SETTINGS     m_settings;     // working copy of zone settings
    UNIT_BINDER       m_outlineHatchPitch;
    UNIT_BINDER       m_minWidth;
    UNIT_BINDER       m_hatchRotation;
    UNIT_BINDER       m_hatchWidth;
    UNIT_BINDER       m_hatchGap;
    int               m_cornerSmoothingType;
    UNIT_BINDER       m_cornerRadius;
    wxStaticText*     m_gapLabel;
    wxTextCtrl*       m_gapCtrl;
    wxStaticText*     m_gapUnits;
    UNIT_BINDER*      m_gap;

    CONVERT_SETTINGS* m_convertSettings;
    wxRadioButton*    m_rbCenterline;
    wxRadioButton*    m_rbEnvelope;
    wxCheckBox*       m_cbDeleteOriginals;
};


int InvokeNonCopperZonesEditor( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings,
                                CONVERT_SETTINGS* aConvertSettings )
{
    DIALOG_NON_COPPER_ZONES_EDITOR  dlg( aParent, aSettings, aConvertSettings );

    // TODO: why does this require QuasiModal?
    return dlg.ShowQuasiModal();
}

#define MIN_THICKNESS 10*pcbIUScale.IU_PER_MILS

DIALOG_NON_COPPER_ZONES_EDITOR::DIALOG_NON_COPPER_ZONES_EDITOR( PCB_BASE_FRAME* aParent,
                                                                ZONE_SETTINGS* aSettings,
                                                                CONVERT_SETTINGS* aConvertSettings ) :
    DIALOG_NONCOPPER_ZONES_PROPERTIES_BASE( aParent ),
    m_outlineHatchPitch( aParent, m_stBorderHatchPitchText,
                         m_outlineHatchPitchCtrl, m_outlineHatchUnits ),
    m_minWidth( aParent, m_MinWidthLabel, m_MinWidthCtrl, m_MinWidthUnits ),
    m_hatchRotation( aParent, m_hatchOrientLabel, m_hatchOrientCtrl, m_hatchOrientUnits ),
    m_hatchWidth( aParent, m_hatchWidthLabel, m_hatchWidthCtrl, m_hatchWidthUnits),
    m_hatchGap( aParent, m_hatchGapLabel, m_hatchGapCtrl, m_hatchGapUnits ),
    m_cornerSmoothingType( ZONE_SETTINGS::SMOOTHING_UNDEFINED ),
    m_cornerRadius( aParent, m_cornerRadiusLabel, m_cornerRadiusCtrl, m_cornerRadiusUnits ),
    m_convertSettings( aConvertSettings ),
    m_rbCenterline( nullptr ),
    m_rbEnvelope( nullptr ),
    m_cbDeleteOriginals( nullptr )
{
    m_parent = aParent;

    m_ptr  = aSettings;
    m_settings = *aSettings;

    if( aConvertSettings )
    {
        wxStaticBox*      bConvertBox = new wxStaticBox( this, wxID_ANY, _( "Conversion Settings" ) );
        wxStaticBoxSizer* bConvertSizer = new wxStaticBoxSizer( bConvertBox, wxVERTICAL  );

        m_rbCenterline = new wxRadioButton( this, wxID_ANY, _( "Use centerlines" ) );
        bConvertSizer->Add( m_rbCenterline, 0, wxLEFT|wxRIGHT, 5 );

        bConvertSizer->AddSpacer( 2 );
        m_rbEnvelope = new wxRadioButton( this, wxID_ANY, _( "Create bounding hull" ) );
        bConvertSizer->Add( m_rbEnvelope, 0, wxLEFT|wxRIGHT, 5 );

        m_gapLabel = new wxStaticText( this, wxID_ANY, _( "Gap:" ) );
        m_gapCtrl = new wxTextCtrl( this, wxID_ANY );
        m_gapUnits = new wxStaticText( this, wxID_ANY, _( "mm" ) );
        m_gap = new UNIT_BINDER( m_parent, m_gapLabel, m_gapCtrl, m_gapUnits );
        m_gap->SetValue( m_convertSettings->m_Gap );

        wxBoxSizer* hullParamsSizer = new wxBoxSizer( wxHORIZONTAL );
        hullParamsSizer->Add( m_gapLabel, 0, wxALIGN_CENTRE_VERTICAL|wxRIGHT, 5 );
        hullParamsSizer->Add( m_gapCtrl, 1, wxALIGN_CENTRE_VERTICAL|wxLEFT|wxRIGHT, 5 );
        hullParamsSizer->Add( m_gapUnits, 0, wxALIGN_CENTRE_VERTICAL|wxLEFT, 5 );
        bConvertSizer->AddSpacer( 2 );
        bConvertSizer->Add( hullParamsSizer, 0, wxLEFT, 26 );

        bConvertSizer->AddSpacer( 6 );
        m_cbDeleteOriginals = new wxCheckBox( this, wxID_ANY,
                                              _( "Delete source objects after conversion" ) );
        bConvertSizer->Add( m_cbDeleteOriginals, 0, wxALL, 5 );

        GetSizer()->Insert( 0, bConvertSizer, 0, wxALL|wxEXPAND, 10 );

        wxStaticLine* line =  new wxStaticLine( this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                                wxLI_HORIZONTAL );
        GetSizer()->Insert( 1, line, 0, wxLEFT|wxRIGHT|wxEXPAND, 10 );

        SetTitle( _( "Convert to Non Copper Zone" ) );
    }
    else
    {
        m_gapLabel = nullptr;
        m_gapCtrl = nullptr;
        m_gapUnits = nullptr;
        m_gap = nullptr;
    }

    m_staticTextLayerSelection->SetFont( KIUI::GetStatusFont( this ) );

    m_settings.SetupLayersList( m_layers, m_parent, LSET::AllNonCuMask() );

    SetupStandardButtons();

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

    if( m_gap )
        m_gap->Enable( m_rbEnvelope->GetValue() );
}


bool DIALOG_NON_COPPER_ZONES_EDITOR::TransferDataToWindow()
{
    if( m_convertSettings )
    {
        if( m_convertSettings->m_Strategy == BOUNDING_HULL )
            m_rbEnvelope->SetValue( true );
        else
            m_rbCenterline->SetValue( true );

        m_cbDeleteOriginals->SetValue( m_convertSettings->m_DeleteOriginals );

        m_gap->Enable( m_rbEnvelope->GetValue() );
    }

    m_cornerSmoothingChoice->SetSelection( m_settings.GetCornerSmoothingType() );
    m_cornerRadius.SetValue( m_settings.GetCornerRadius() );

    m_minWidth.SetValue( m_settings.m_ZoneMinThickness );
    m_cbLocked->SetValue( m_settings.m_Locked );

    switch( m_settings.m_ZoneBorderDisplayStyle )
    {
    case ZONE_BORDER_DISPLAY_STYLE::NO_HATCH:      m_OutlineDisplayCtrl->SetSelection( 0 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE: m_OutlineDisplayCtrl->SetSelection( 1 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL: m_OutlineDisplayCtrl->SetSelection( 2 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::INVISIBLE_BORDER: break;    // Not used for standard zones
    }

    m_outlineHatchPitch.SetValue( m_settings.m_BorderHatchPitch );

    SetInitialFocus( m_OutlineDisplayCtrl );

    switch( m_settings.m_FillMode )
    {
    case ZONE_FILL_MODE::HATCH_PATTERN: m_GridStyleCtrl->SetSelection( 1 ); break;
    default:                            m_GridStyleCtrl->SetSelection( 0 ); break;
    }

    m_hatchRotation.SetUnits( EDA_UNITS::DEGREES );
    m_hatchRotation.SetAngleValue( m_settings.m_HatchOrientation );

    // Gives a reasonable value to grid style parameters, if currently there are no defined
    // parameters for grid pattern thickness and gap (if the value is 0)
    // the grid pattern thickness default value is (arbitrary) m_ZoneMinThickness * 4
    // or 1mm
    // the grid pattern gap default value is (arbitrary) m_ZoneMinThickness * 6
    // or 1.5 mm
    int bestvalue = m_settings.m_HatchThickness;

    if( bestvalue <= 0 )     // No defined value for m_hatchWidth
        bestvalue = std::max( m_settings.m_ZoneMinThickness * 4, pcbIUScale.mmToIU( 1.0 ) );

    m_hatchWidth.SetValue( std::max( bestvalue, m_settings.m_ZoneMinThickness ) );

    bestvalue = m_settings.m_HatchGap;

    if( bestvalue <= 0 )     // No defined value for m_hatchGap
        bestvalue = std::max( m_settings.m_ZoneMinThickness * 6, pcbIUScale.mmToIU( 1.5 ) );

    m_hatchGap.SetValue( std::max( bestvalue, m_settings.m_ZoneMinThickness ) );

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
    m_hatchRotation.Enable( enable );
    m_hatchWidth.Enable( enable );
    m_hatchGap.Enable( enable );
    m_smoothLevelLabel->Enable( enable );
    m_spinCtrlSmoothLevel->Enable( enable );
    m_smoothValueLabel->Enable( enable );
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
    if( m_convertSettings )
    {
        if( m_rbEnvelope->GetValue() )
            m_convertSettings->m_Strategy = BOUNDING_HULL;
        else
            m_convertSettings->m_Strategy = CENTERLINE;

        m_convertSettings->m_DeleteOriginals = m_cbDeleteOriginals->GetValue();
        m_convertSettings->m_Gap = m_gap->GetIntValue();
    }

    m_settings.SetCornerSmoothingType( m_cornerSmoothingChoice->GetSelection() );

    m_settings.SetCornerRadius( m_settings.GetCornerSmoothingType() == ZONE_SETTINGS::SMOOTHING_NONE
                                ? 0 : m_cornerRadius.GetValue() );

    m_settings.m_ZoneMinThickness = m_minWidth.GetValue();

    switch( m_OutlineDisplayCtrl->GetSelection() )
    {
    case 0: m_settings.m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::NO_HATCH;      break;
    case 1: m_settings.m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE; break;
    case 2: m_settings.m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL; break;
    }

    if( !m_outlineHatchPitch.Validate( pcbIUScale.mmToIU( ZONE_BORDER_HATCH_MINDIST_MM ),
                                       pcbIUScale.mmToIU( ZONE_BORDER_HATCH_MAXDIST_MM ) ) )
        return false;

    m_settings.m_BorderHatchPitch = m_outlineHatchPitch.GetValue();

    if( m_GridStyleCtrl->GetSelection() > 0 )
        m_settings.m_FillMode = ZONE_FILL_MODE::HATCH_PATTERN;
    else
        m_settings.m_FillMode = ZONE_FILL_MODE::POLYGONS;


    if( m_settings.m_FillMode == ZONE_FILL_MODE::HATCH_PATTERN )
    {
        int minThickness = m_minWidth.GetValue();

        if( !m_hatchWidth.Validate( minThickness, INT_MAX ) )
            return false;

        if( !m_hatchGap.Validate( minThickness, INT_MAX ) )
            return false;
    }


    m_settings.m_HatchOrientation = m_hatchRotation.GetAngleValue();
    m_settings.m_HatchThickness = m_hatchWidth.GetValue();
    m_settings.m_HatchGap = m_hatchGap.GetValue();
    m_settings.m_HatchSmoothingLevel = m_spinCtrlSmoothLevel->GetValue();
    m_settings.m_HatchSmoothingValue = m_spinCtrlSmoothValue->GetValue();

    m_settings.m_Locked = m_cbLocked->GetValue();

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

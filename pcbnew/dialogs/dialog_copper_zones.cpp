/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
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

#include <grid_tricks.h>
#include <wx/radiobut.h>
#include <wx/statbox.h>
#include <kiface_base.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <zones.h>
#include <widgets/unit_binder.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>
#include <zone.h>
#include <pad.h>
#include <board.h>
#include <grid_layer_box_helpers.h>

#include <dialog_copper_zones_base.h>


class DIALOG_COPPER_ZONE : public DIALOG_COPPER_ZONE_BASE
{
public:
    DIALOG_COPPER_ZONE( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings, CONVERT_SETTINGS* aConvertSettings );

    ~DIALOG_COPPER_ZONE() override;

private:
    static constexpr int INVALID_NET_CODE{ 0 };

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    /**
     * @return bool - false if incorrect options, true if ok.
     */
    bool AcceptOptions();

    void onHatched( wxCommandEvent& event ) override;
    void OnLayerSelection( wxDataViewEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& ) override;
    void onNetSelector( wxCommandEvent& aEvent );
    void OnRemoveIslandsSelection( wxCommandEvent& event ) override;
    void OnCornerSmoothingSelection( wxCommandEvent& event ) override;
    void OnAddLayerItem( wxCommandEvent& event ) override;
    void OnDeleteLayerItem( wxCommandEvent& event ) override;

    void updateInfoBar();

private:
    PCB_BASE_FRAME*  m_Parent;

    ZONE_SETTINGS    m_settings;
    ZONE_SETTINGS*   m_ptr;

    LAYER_PROPERTIES_GRID_TABLE* m_layerPropsTable;

    UNIT_BINDER      m_outlineHatchPitch;

    UNIT_BINDER      m_cornerRadius;
    UNIT_BINDER      m_clearance;
    UNIT_BINDER      m_minWidth;
    UNIT_BINDER      m_antipadClearance;
    UNIT_BINDER      m_spokeWidth;

    UNIT_BINDER      m_gridStyleRotation;
    UNIT_BINDER      m_gridStyleThickness;
    UNIT_BINDER      m_gridStyleGap;
    UNIT_BINDER      m_islandThreshold;
    bool             m_isTeardrop;

    wxStaticText*     m_gapLabel;
    wxTextCtrl*       m_gapCtrl;
    wxStaticText*     m_gapUnits;
    UNIT_BINDER*      m_gap;

    CONVERT_SETTINGS* m_convertSettings;
    wxRadioButton*    m_rbCenterline;
    wxRadioButton*    m_rbEnvelope;
    wxCheckBox*       m_cbDeleteOriginals;
};


int InvokeCopperZonesEditor( PCB_BASE_FRAME* aCaller, ZONE_SETTINGS* aSettings, CONVERT_SETTINGS* aConvertSettings )
{
    DIALOG_COPPER_ZONE dlg( aCaller, aSettings, aConvertSettings );

    // TODO: why does this need QuasiModal?
    return dlg.ShowQuasiModal();
}


DIALOG_COPPER_ZONE::DIALOG_COPPER_ZONE( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings,
                                        CONVERT_SETTINGS* aConvertSettings ) :
        DIALOG_COPPER_ZONE_BASE( aParent ),
        m_outlineHatchPitch( aParent, m_stBorderHatchPitchText, m_outlineHatchPitchCtrl, m_outlineHatchUnits ),
        m_cornerRadius( aParent, m_cornerRadiusLabel, m_cornerRadiusCtrl, m_cornerRadiusUnits ),
        m_clearance( aParent, m_clearanceLabel, m_clearanceCtrl, m_clearanceUnits ),
        m_minWidth( aParent, m_minWidthLabel, m_minWidthCtrl, m_minWidthUnits ),
        m_antipadClearance( aParent, m_antipadLabel, m_antipadCtrl, m_antipadUnits ),
        m_spokeWidth( aParent, m_spokeWidthLabel, m_spokeWidthCtrl, m_spokeWidthUnits ),
        m_gridStyleRotation( aParent, m_staticTextGrindOrient, m_tcGridStyleOrientation, m_staticTextRotUnits ),
        m_gridStyleThickness( aParent, m_staticTextStyleThickness, m_tcGridStyleThickness, m_GridStyleThicknessUnits ),
        m_gridStyleGap( aParent, m_staticTextGridGap, m_tcGridStyleGap, m_GridStyleGapUnits ),
        m_islandThreshold( aParent, m_islandThresholdLabel, m_tcIslandThreshold, m_islandThresholdUnits ),
        m_convertSettings( aConvertSettings ),
        m_rbCenterline( nullptr ),
        m_rbEnvelope( nullptr ),
        m_cbDeleteOriginals( nullptr )
{
    m_Parent = aParent;

    m_ptr = aSettings;
    m_settings = *aSettings;
    m_settings.SetupLayersList( m_layers, m_Parent, LSET::AllCuMask( aParent->GetBoard()->GetCopperLayerCount() ) );
    m_isTeardrop = m_settings.m_TeardropType != TEARDROP_TYPE::TD_NONE;

    if( m_isTeardrop )
        SetTitle( _( "Legacy Teardrop Properties" ) );

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
        m_gap = new UNIT_BINDER( m_Parent, m_gapLabel, m_gapCtrl, m_gapUnits );
        m_gap->SetValue( m_convertSettings->m_Gap );

        wxBoxSizer* hullParamsSizer = new wxBoxSizer( wxHORIZONTAL );
        hullParamsSizer->Add( m_gapLabel, 0, wxALIGN_CENTRE_VERTICAL|wxRIGHT, 5 );
        hullParamsSizer->Add( m_gapCtrl, 1, wxALIGN_CENTRE_VERTICAL|wxLEFT|wxRIGHT, 5 );
        hullParamsSizer->Add( m_gapUnits, 0, wxALIGN_CENTRE_VERTICAL|wxLEFT, 5 );
        bConvertSizer->AddSpacer( 2 );
        bConvertSizer->Add( hullParamsSizer, 0, wxLEFT, 26 );

        bConvertSizer->AddSpacer( 6 );
        m_cbDeleteOriginals = new wxCheckBox( this, wxID_ANY, _( "Delete source objects after conversion" ) );
        bConvertSizer->Add( m_cbDeleteOriginals, 0, wxALL, 5 );

        GetSizer()->Insert( 0, bConvertSizer, 0, wxALL|wxEXPAND, 10 );
        SetTitle( _( "Convert to Copper Zone" ) );
    }
    else
    {
        m_gapLabel = nullptr;
        m_gapCtrl = nullptr;
        m_gapUnits = nullptr;
        m_gap = nullptr;
    }

    m_netSelector->SetNetInfo( &aParent->GetBoard()->GetNetInfo() );

    m_layerPropsTable = new LAYER_PROPERTIES_GRID_TABLE( m_Parent,
            [&]() -> LSET
            {
                LSET layers;

                for( int ii = 0; ii < m_layers->GetItemCount(); ++ii )
                {
                    if( m_layers->GetToggleValue( ii, 0 ) )
                    {
                        wxVariant layerID;
                        m_layers->GetValue( layerID, ii, 2 );
                        layers.set( ToLAYER_ID( (int) layerID.GetInteger() ) );
                    }
                }

                return layers;
            } );

    m_layerSpecificOverrides->SetTable( m_layerPropsTable, true );
    m_layerSpecificOverrides->PushEventHandler( new GRID_TRICKS( m_layerSpecificOverrides ) );
    m_layerSpecificOverrides->SetSelectionMode( wxGrid::wxGridSelectRows );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_LAYER_RENDERER( nullptr ) );
    LSET forbiddenLayers = LSET::AllNonCuMask();

    for( PCB_LAYER_ID copper : LSET::AllCuMask() )
    {
        if( !m_Parent->GetBoard()->IsLayerEnabled( copper ) )
            forbiddenLayers.set( copper );
    }

    attr->SetEditor( new GRID_CELL_LAYER_SELECTOR( nullptr, forbiddenLayers ) );
    m_layerSpecificOverrides->SetColAttr( 0, attr );
    m_layerSpecificOverrides->SetupColumnAutosizer( 0 );

    m_bpAddCustomLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpDeleteCustomLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_netSelector->Bind( FILTERED_ITEM_SELECTED, &DIALOG_COPPER_ZONE::onNetSelector, this );

    SetupStandardButtons();

    finishDialogSettings();
}


DIALOG_COPPER_ZONE::~DIALOG_COPPER_ZONE()
{
    // we passed ownership of table to grid
    // delete m_layerPropsTable;

    m_layerSpecificOverrides->PopEventHandler( true );

    delete m_gap;
    m_netSelector->Unbind( FILTERED_ITEM_SELECTED, &DIALOG_COPPER_ZONE::onNetSelector, this );
}


bool DIALOG_COPPER_ZONE::TransferDataToWindow()
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

    m_tcZoneName->SetValue( m_settings.m_Name );
    m_netSelector->SetSelectedNetcode( std::max( 0, m_settings.m_Netcode ) );
    m_cbLocked->SetValue( m_settings.m_Locked );
    m_cornerSmoothingChoice->SetSelection( m_settings.GetCornerSmoothingType() );
    m_cornerRadius.SetValue( m_settings.GetCornerRadius() );

    if( m_isTeardrop )  // outlines are never smoothed: they have already the right shape
    {
        m_cornerSmoothingChoice->SetSelection( 0 );
        m_cornerSmoothingChoice->Enable( false );
        m_cornerRadius.Show( false );
    }

    switch( m_settings.m_ZoneBorderDisplayStyle )
    {
    case ZONE_BORDER_DISPLAY_STYLE::NO_HATCH:         m_OutlineDisplayCtrl->SetSelection( 0 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE:    m_OutlineDisplayCtrl->SetSelection( 1 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL:    m_OutlineDisplayCtrl->SetSelection( 2 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::INVISIBLE_BORDER: break;    // Not used for standard zones
    }

    m_outlineHatchPitch.SetValue( m_settings.m_BorderHatchPitch );

    m_clearance.SetValue( m_settings.m_ZoneClearance );
    m_minWidth.SetValue( m_settings.m_ZoneMinThickness );

    switch( m_settings.GetPadConnection() )
    {
    default:
    case ZONE_CONNECTION::THERMAL:     m_PadInZoneOpt->SetSelection( 1 ); break;
    case ZONE_CONNECTION::THT_THERMAL: m_PadInZoneOpt->SetSelection( 2 ); break;
    case ZONE_CONNECTION::NONE:        m_PadInZoneOpt->SetSelection( 3 ); break;
    case ZONE_CONNECTION::FULL:        m_PadInZoneOpt->SetSelection( 0 ); break;
    }

    if( m_isTeardrop )
    {
        m_PadInZoneOpt->SetSelection( 0 );
        m_PadInZoneOpt->Enable( false );
        m_antipadClearance.Enable( false );
        m_spokeWidth.Enable( false );
    }

    // Do not enable/disable antipad clearance and spoke width.  They might be needed if
    // a footprint or pad overrides the zone to specify a thermal connection.
    m_antipadClearance.SetValue( m_settings.m_ThermalReliefGap );
    m_spokeWidth.SetValue( m_settings.m_ThermalReliefSpokeWidth );

    m_islandThreshold.SetDataType( EDA_DATA_TYPE::AREA );
    m_islandThreshold.SetDoubleValue( static_cast<double>( m_settings.GetMinIslandArea() ) );

    m_cbRemoveIslands->SetSelection( static_cast<int>( m_settings.GetIslandRemovalMode() ) );

    m_cbHatched->SetValue( m_settings.m_FillMode == ZONE_FILL_MODE::HATCH_PATTERN && !m_isTeardrop );

    m_gridStyleRotation.SetUnits( EDA_UNITS::DEGREES );
    m_gridStyleRotation.SetAngleValue( m_settings.m_HatchOrientation );
    m_gridStyleThickness.SetValue( m_settings.m_HatchThickness );
    m_gridStyleGap.SetValue( m_settings.m_HatchGap );

    m_spinCtrlSmoothLevel->SetValue( m_settings.m_HatchSmoothingLevel );
    m_spinCtrlSmoothValue->SetValue( m_settings.m_HatchSmoothingValue );

    for( const auto& [layer, props] : m_settings.m_LayerProperties )
    {
        if( props.hatching_offset.has_value() )
            m_layerPropsTable->AddItem( layer, props );
    }

    // Enable/Disable some widgets
    wxCommandEvent aEvent;
    onNetSelector( aEvent );
    OnCornerSmoothingSelection( aEvent );
    OnRemoveIslandsSelection( aEvent );
    onHatched( aEvent );

    Fit();

    return true;
}


void DIALOG_COPPER_ZONE::OnUpdateUI( wxUpdateUIEvent& )
{
    if( m_gap )
        m_gap->Enable( m_rbEnvelope->GetValue() );
}


void DIALOG_COPPER_ZONE::OnRemoveIslandsSelection( wxCommandEvent& event )
{
    m_islandThreshold.Show( m_cbRemoveIslands->GetSelection() == 2 );
    Layout();
}


void DIALOG_COPPER_ZONE::OnCornerSmoothingSelection( wxCommandEvent& event )
{
    switch( m_cornerSmoothingChoice->GetSelection() )
    {
    case ZONE_SETTINGS::SMOOTHING_CHAMFER:
        m_cornerRadiusLabel->SetLabel( _( "Chamfer:" ) );
        m_cornerRadius.Show( true );
        break;

    case ZONE_SETTINGS::SMOOTHING_FILLET:
        m_cornerRadiusLabel->SetLabel( _( "Fillet:" ) );
        m_cornerRadius.Show( true );
        break;

    default:
        m_cornerRadius.Show( false );
        break;
    }

    Layout();
}


bool DIALOG_COPPER_ZONE::TransferDataFromWindow()
{
    if( !m_layerSpecificOverrides->CommitPendingChanges() )
        return false;

    if( !AcceptOptions() )
        return false;

    if( m_convertSettings )
    {
        if( m_rbEnvelope->GetValue() )
            m_convertSettings->m_Strategy = BOUNDING_HULL;
        else
            m_convertSettings->m_Strategy = CENTERLINE;

        m_convertSettings->m_DeleteOriginals = m_cbDeleteOriginals->GetValue();
        m_convertSettings->m_Gap = m_gap->GetIntValue();
    }

    *m_ptr = m_settings;
    return true;
}


bool DIALOG_COPPER_ZONE::AcceptOptions()
{
    if( !m_clearance.Validate( 0, pcbIUScale.mmToIU( ZONE_CLEARANCE_MAX_VALUE_MM ) ) )
        return false;

    if( !m_minWidth.Validate( pcbIUScale.mmToIU( ZONE_THICKNESS_MIN_VALUE_MM ), INT_MAX ) )
        return false;

    if( !m_cornerRadius.Validate( 0, INT_MAX ) )
        return false;

    if( !m_spokeWidth.Validate( 0, INT_MAX ) )
        return false;

    m_gridStyleRotation.SetValue( NormalizeAngle180( m_gridStyleRotation.GetValue() ) );

    if( m_cbHatched->GetValue() )
    {
        int minThickness = m_minWidth.GetIntValue();

        if( !m_gridStyleThickness.Validate( minThickness, INT_MAX ) )
            return false;

        if( !m_gridStyleGap.Validate( minThickness, INT_MAX ) )
            return false;
    }

    switch( m_PadInZoneOpt->GetSelection() )
    {
    case 3: m_settings.SetPadConnection( ZONE_CONNECTION::NONE );        break;
    case 2: m_settings.SetPadConnection( ZONE_CONNECTION::THT_THERMAL ); break;
    case 1: m_settings.SetPadConnection( ZONE_CONNECTION::THERMAL );     break;
    case 0: m_settings.SetPadConnection( ZONE_CONNECTION::FULL );        break;
    }

    switch( m_OutlineDisplayCtrl->GetSelection() )
    {
    case 0: m_settings.m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::NO_HATCH;      break;
    case 1: m_settings.m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE; break;
    case 2: m_settings.m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL; break;
    }

    if( !m_outlineHatchPitch.Validate( pcbIUScale.mmToIU( ZONE_BORDER_HATCH_MINDIST_MM ),
                                       pcbIUScale.mmToIU( ZONE_BORDER_HATCH_MAXDIST_MM ) ) )
    {
        return false;
    }

    m_settings.m_BorderHatchPitch = m_outlineHatchPitch.GetIntValue();

    m_settings.m_ZoneClearance = m_clearance.GetIntValue();
    m_settings.m_ZoneMinThickness = m_minWidth.GetIntValue();

    m_settings.SetCornerSmoothingType( m_cornerSmoothingChoice->GetSelection() );

    if( m_settings.GetCornerSmoothingType() == ZONE_SETTINGS::SMOOTHING_NONE )
        m_settings.SetCornerRadius( 0 );
    else
        m_settings.SetCornerRadius( m_cornerRadius.GetIntValue() );

    m_settings.m_Locked = m_cbLocked->GetValue();

    m_settings.m_ThermalReliefGap = m_antipadClearance.GetValue();
    m_settings.m_ThermalReliefSpokeWidth = m_spokeWidth.GetValue();

    if( m_settings.m_ThermalReliefSpokeWidth < m_settings.m_ZoneMinThickness )
    {
        m_notebook->SetSelection( 0 );
        DisplayError( this, _( "Thermal spoke width cannot be smaller than the minimum width." ) );
        return false;
    }

    m_settings.SetIslandRemovalMode( (ISLAND_REMOVAL_MODE) m_cbRemoveIslands->GetSelection() );
    m_settings.SetMinIslandArea( m_islandThreshold.GetValue() );

    // Get the layer selection for this zone
    int layers = 0;

    for( int ii = 0; ii < m_layers->GetItemCount(); ++ii )
    {
        if( m_layers->GetToggleValue( (unsigned) ii, 0 ) )
            layers++;
    }

    if( layers == 0 )
    {
        DisplayError( this, _( "No layer selected." ) );
        return false;
    }

    m_settings.m_Netcode = m_netSelector->GetSelectedNetcode();
    m_settings.m_Name = m_tcZoneName->GetValue();

    m_settings.m_FillMode = m_cbHatched->GetValue() ? ZONE_FILL_MODE::HATCH_PATTERN : ZONE_FILL_MODE::POLYGONS;
    m_settings.m_HatchOrientation = m_gridStyleRotation.GetAngleValue();
    m_settings.m_HatchThickness = m_gridStyleThickness.GetIntValue();
    m_settings.m_HatchGap = m_gridStyleGap.GetIntValue();
    m_settings.m_HatchSmoothingLevel = m_spinCtrlSmoothLevel->GetValue();
    m_settings.m_HatchSmoothingValue = m_spinCtrlSmoothValue->GetValue();

    for( auto& [layer, props] : m_settings.m_LayerProperties )
        props.hatching_offset = std::nullopt;

    for( const auto& [layer, props] : m_layerPropsTable->GetItems() )
        m_settings.m_LayerProperties[layer] = props;

    return true;
}


void DIALOG_COPPER_ZONE::onNetSelector( wxCommandEvent& aEvent )
{
    updateInfoBar();

    // Zones with no net never have islands removed
    if( m_netSelector->GetSelectedNetcode() == INVALID_NET_CODE )
    {
        if( m_cbRemoveIslands->IsEnabled() )
            m_settings.SetIslandRemovalMode( (ISLAND_REMOVAL_MODE) m_cbRemoveIslands->GetSelection() );

        m_cbRemoveIslands->SetSelection( 1 );
        m_staticText40->Enable( false );
        m_cbRemoveIslands->Enable( false );
    }
    else if( !m_cbRemoveIslands->IsEnabled() )
    {
        m_cbRemoveIslands->SetSelection( static_cast<int>( m_settings.GetIslandRemovalMode() ) );
        m_staticText40->Enable( true );
        m_cbRemoveIslands->Enable( true );
    }
}


void DIALOG_COPPER_ZONE::onHatched( wxCommandEvent& event )
{
    bool enable = m_cbHatched->GetValue();
    m_gridStyleThickness.Enable( enable );
    m_gridStyleGap.Enable( enable );
    m_gridStyleRotation.Enable( enable );
    m_staticTextGridSmoothingLevel->Enable( enable );
    m_spinCtrlSmoothLevel->Enable( enable );
    m_staticTextGridSmootingVal->Enable( enable );
    m_spinCtrlSmoothValue->Enable( enable );
    m_offsetOverridesLabel->Enable( enable );
    m_layerSpecificOverrides->Enable( enable );
    m_bpAddCustomLayer->Enable( enable );
    m_bpDeleteCustomLayer->Enable( enable );
}


void DIALOG_COPPER_ZONE::OnLayerSelection( wxDataViewEvent& event )
{
    if( event.GetColumn() != 0 )
        return;

    int row = m_layers->ItemToRow( event.GetItem() );

    bool checked = m_layers->GetToggleValue( row, 0 );

    wxVariant layerID;
    m_layers->GetValue( layerID, row, 2 );

    m_settings.m_Layers.set( ToLAYER_ID( layerID.GetInteger() ), checked );
}


void DIALOG_COPPER_ZONE::updateInfoBar()
{
    if( m_netSelector->GetSelectedNetcode() <= INVALID_NET_CODE
            && !m_copperZoneInfo->IsShown()
            && !m_convertSettings )
    {
        m_copperZoneInfo->ShowMessage( _( "<no net> will result in an isolated copper island." ),
                                       wxICON_WARNING );
    }
    else if( m_copperZoneInfo->IsShown() )
    {
        m_copperZoneInfo->Dismiss();
    }
}


void DIALOG_COPPER_ZONE::OnAddLayerItem( wxCommandEvent& event )
{
    m_layerSpecificOverrides->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                m_layerSpecificOverrides->GetTable()->AppendRows( 1 );
                return { m_layerSpecificOverrides->GetNumberRows() - 1, -1 };
            } );
}


void DIALOG_COPPER_ZONE::OnDeleteLayerItem( wxCommandEvent& event )
{
    m_layerSpecificOverrides->OnDeleteRows(
            [&]( int row )
            {
                m_layerSpecificOverrides->GetTable()->DeleteRows( row, 1 );
            } );
}



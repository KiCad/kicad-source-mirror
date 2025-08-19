/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
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

#include <advanced_config.h>
#include <kiface_base.h>
#include <confirm.h>
#include <board.h>
#include <eda_group.h>
#include <footprint.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <zone_settings.h>
#include <dialog_rule_area_properties_base.h>
#include <panel_rule_area_properties_keepout_base.h>
#include <panel_rule_area_properties_placement_base.h>
#include <widgets/unit_binder.h>
#include <wx/statbox.h>
#include <wx/statline.h>
#include <wx/radiobut.h>

#define LAYER_LIST_COLUMN_CHECK 0
#define LAYER_LIST_COLUMN_ICON 1
#define LAYER_LIST_COLUMN_NAME 2
#define LAYER_LIST_ROW_ALL_INNER_LAYERS 1


class DIALOG_RULE_AREA_PROPERTIES : public DIALOG_RULE_AREA_PROPERTIES_BASE
{
public:
    DIALOG_RULE_AREA_PROPERTIES( PCB_BASE_FRAME* aParent, ZONE_SETTINGS* aSettings,
                                 CONVERT_SETTINGS* aConvertSettings, BOARD* aBoard );
    ~DIALOG_RULE_AREA_PROPERTIES();

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnLayerSelection( wxDataViewEvent& event ) override;
    void OnSheetNameClicked( wxCommandEvent& event );
    void OnComponentClassClicked( wxCommandEvent& event );
    void OnGroupClicked( wxCommandEvent& event );

private:
    BOARD*             m_board;
    UNIT_BINDER        m_outlineHatchPitch;
    PCB_BASE_FRAME*    m_parent;
    ZONE_SETTINGS      m_zonesettings;    ///< the working copy of zone settings
    ZONE_SETTINGS*     m_ptr;             ///< the pointer to the zone settings of the zone to edit
    bool               m_isFpEditor;

    CONVERT_SETTINGS*  m_convertSettings;
    wxRadioButton*     m_rbCenterline;
    wxRadioButton*     m_rbBoundingHull;
    wxStaticText*      m_gapLabel;
    wxTextCtrl*        m_gapCtrl;
    wxStaticText*      m_gapUnits;
    UNIT_BINDER*       m_gap;
    wxCheckBox*        m_cbDeleteOriginals;

    // The name of a rule area source that is not now found on the board (e.g. after a netlist
    // update). This is used to re-populate the zone settings if the selection is not changed.
    bool               m_notFoundPlacementSource;
    wxString           m_notFoundPlacementSourceName;
    PLACEMENT_SOURCE_T m_originalPlacementSourceType;
    PLACEMENT_SOURCE_T m_lastPlacementSourceType;

    PANEL_RULE_AREA_PROPERTIES_KEEPOUT_BASE*   m_keepoutProperties;
    PANEL_RULE_AREA_PROPERTIES_PLACEMENT_BASE* m_placementProperties;
};


int InvokeRuleAreaEditor( PCB_BASE_FRAME* aCaller, ZONE_SETTINGS* aZoneSettings, BOARD* aBoard,
                          CONVERT_SETTINGS* aConvertSettings )
{
    DIALOG_RULE_AREA_PROPERTIES dlg( aCaller, aZoneSettings, aConvertSettings, aBoard );

    return dlg.ShowModal();
}


DIALOG_RULE_AREA_PROPERTIES::DIALOG_RULE_AREA_PROPERTIES( PCB_BASE_FRAME*   aParent,
                                                          ZONE_SETTINGS*    aSettings,
                                                          CONVERT_SETTINGS* aConvertSettings,
                                                          BOARD*            aBoard ) :
        DIALOG_RULE_AREA_PROPERTIES_BASE( aParent ),
        m_board( aBoard ),
        m_outlineHatchPitch( aParent, m_stBorderHatchPitchText, m_outlineHatchPitchCtrl, m_outlineHatchUnits ),
        m_convertSettings( aConvertSettings ),
        m_rbCenterline( nullptr ),
        m_rbBoundingHull( nullptr ),
        m_gapLabel( nullptr ),
        m_gapCtrl( nullptr ),
        m_gapUnits( nullptr ),
        m_gap( nullptr ),
        m_cbDeleteOriginals( nullptr ),
        m_notFoundPlacementSource( false ),
        m_originalPlacementSourceType( PLACEMENT_SOURCE_T::SHEETNAME ),
        m_lastPlacementSourceType( PLACEMENT_SOURCE_T::SHEETNAME )
{
    m_isFpEditor = aParent->GetFrameType() == FRAME_FOOTPRINT_EDITOR;
    m_parent = aParent;

    m_ptr = aSettings;
    m_zonesettings = *aSettings;

    m_keepoutProperties = new PANEL_RULE_AREA_PROPERTIES_KEEPOUT_BASE( m_areaPropertiesNb );
    m_areaPropertiesNb->AddPage( m_keepoutProperties, _( "Keepouts" ), true );

    m_placementProperties = new PANEL_RULE_AREA_PROPERTIES_PLACEMENT_BASE( m_areaPropertiesNb );
    m_areaPropertiesNb->AddPage( m_placementProperties, _( "Placement" ) );

    m_placementProperties->m_SheetRb->Connect(
            wxEVT_CHECKBOX,
            wxCommandEventHandler( DIALOG_RULE_AREA_PROPERTIES::OnSheetNameClicked ), nullptr,
            this );
    m_placementProperties->m_ComponentsRb->Connect(
            wxEVT_CHECKBOX,
            wxCommandEventHandler( DIALOG_RULE_AREA_PROPERTIES::OnComponentClassClicked ), nullptr,
            this );
    m_placementProperties->m_GroupRb->Connect(
            wxEVT_CHECKBOX,
            wxCommandEventHandler( DIALOG_RULE_AREA_PROPERTIES::OnGroupClicked ), nullptr,
            this );

    if( aConvertSettings )
    {
        wxStaticBox*      bConvertBox = new wxStaticBox( this, wxID_ANY,
                                                         _( "Conversion Settings" ) );
        wxStaticBoxSizer* bConvertSizer = new wxStaticBoxSizer( bConvertBox, wxVERTICAL  );

        m_rbCenterline = new wxRadioButton( this, wxID_ANY, _( "Use centerlines" ) );
        bConvertSizer->Add( m_rbCenterline, 0, wxLEFT|wxRIGHT, 5 );

        bConvertSizer->AddSpacer( 2 );
        m_rbBoundingHull = new wxRadioButton( this, wxID_ANY, _( "Create bounding hull" ) );
        bConvertSizer->Add( m_rbBoundingHull, 0, wxLEFT|wxRIGHT, 5 );

        m_gapLabel = new wxStaticText( this, wxID_ANY, _( "Gap:" ) );
        m_gapCtrl = new wxTextCtrl( this, wxID_ANY );
        m_gapUnits = new wxStaticText( this, wxID_ANY, _( "mm" ) );
        m_gap = new UNIT_BINDER( aParent, m_gapLabel, m_gapCtrl, m_gapUnits );

        wxBoxSizer* hullParamsSizer = new wxBoxSizer( wxHORIZONTAL );
        hullParamsSizer->Add( m_gapLabel, 0, wxALIGN_CENTRE_VERTICAL, 5 );
        hullParamsSizer->Add( m_gapCtrl, 1, wxALIGN_CENTRE_VERTICAL|wxLEFT|wxRIGHT, 3 );
        hullParamsSizer->Add( m_gapUnits, 0, wxALIGN_CENTRE_VERTICAL, 5 );

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

        SetTitle( _( "Convert to Rule Area" ) );
    }

    BOARD* board = m_parent->GetBoard();
    LSET   layers = LSET::AllNonCuMask() | LSET::AllCuMask( board->GetCopperLayerCount() );

    m_zonesettings.SetupLayersList( m_layers, m_parent, layers );

    SetupStandardButtons();

    finishDialogSettings();
}


DIALOG_RULE_AREA_PROPERTIES::~DIALOG_RULE_AREA_PROPERTIES()
{
    m_placementProperties->m_SheetRb->Disconnect(
            wxEVT_CHECKBOX,
            wxCommandEventHandler( DIALOG_RULE_AREA_PROPERTIES::OnSheetNameClicked ), nullptr,
            this );
    m_placementProperties->m_ComponentsRb->Disconnect(
            wxEVT_CHECKBOX,
            wxCommandEventHandler( DIALOG_RULE_AREA_PROPERTIES::OnComponentClassClicked ), nullptr,
            this );
    m_placementProperties->m_GroupRb->Disconnect(
            wxEVT_CHECKBOX,
            wxCommandEventHandler( DIALOG_RULE_AREA_PROPERTIES::OnGroupClicked ), nullptr,
            this );
}


void DIALOG_RULE_AREA_PROPERTIES::OnSheetNameClicked( wxCommandEvent& event )
{
    m_lastPlacementSourceType = PLACEMENT_SOURCE_T::SHEETNAME;
}


void DIALOG_RULE_AREA_PROPERTIES::OnComponentClassClicked( wxCommandEvent& event )
{
    m_lastPlacementSourceType = PLACEMENT_SOURCE_T::COMPONENT_CLASS;
}


void DIALOG_RULE_AREA_PROPERTIES::OnGroupClicked( wxCommandEvent& event )
{
    m_lastPlacementSourceType = PLACEMENT_SOURCE_T::GROUP_PLACEMENT;
}


bool DIALOG_RULE_AREA_PROPERTIES::TransferDataToWindow()
{
    if( m_convertSettings )
    {
        if( m_convertSettings->m_Strategy == BOUNDING_HULL )
            m_rbBoundingHull->SetValue( true );
        else
            m_rbCenterline->SetValue( true );

        m_cbDeleteOriginals->SetValue( m_convertSettings->m_DeleteOriginals );
    }

    // Init keepout parameters:
    m_keepoutProperties->m_cbTracksCtrl->SetValue( m_zonesettings.GetDoNotAllowTracks() );
    m_keepoutProperties->m_cbViasCtrl->SetValue( m_zonesettings.GetDoNotAllowVias() );
    m_keepoutProperties->m_cbPadsCtrl->SetValue( m_zonesettings.GetDoNotAllowPads() );
    m_keepoutProperties->m_cbFootprintsCtrl->SetValue( m_zonesettings.GetDoNotAllowFootprints() );
    m_keepoutProperties->m_cbCopperPourCtrl->SetValue( m_zonesettings.GetDoNotAllowZoneFills() );

    // Init placement parameters:
    m_placementProperties->m_DisabledRb->SetValue( true );
    m_placementProperties->m_SheetRb->SetValue( false );
    m_placementProperties->m_sheetCombo->Clear();

    m_placementProperties->m_ComponentsRb->SetValue( false );
    m_placementProperties->m_componentClassCombo->Clear();

    m_placementProperties->m_GroupRb->SetValue( false );
    m_placementProperties->m_groupCombo->Clear();

    wxString curSourceName = m_zonesettings.GetPlacementAreaSource();

    // Load schematic sheet and component class lists
    if( m_board )
    {
        // Fetch component classes
        std::set<wxString> classNames;

        for( const wxString& className : m_board->GetComponentClassManager().GetClassNames() )
            classNames.insert( className );

        for( const wxString& sourceName : classNames )
            m_placementProperties->m_componentClassCombo->Append( sourceName );

        if( !classNames.empty() )
            m_placementProperties->m_componentClassCombo->Select( 0 );

        // Fetch sheet names and groups
        std::set<wxString> sheetNames;
        std::set<wxString> groupNames;

        for( FOOTPRINT* fp : m_board->Footprints() )
        {
            sheetNames.insert( fp->GetSheetname() );

            if( fp->GetParentGroup() && !fp->GetParentGroup()->GetName().IsEmpty() )
                groupNames.insert( fp->GetParentGroup()->GetName() );
        }

        for( const wxString& sourceName : sheetNames )
            m_placementProperties->m_sheetCombo->Append( sourceName );

        for( const wxString& groupName : groupNames )
            m_placementProperties->m_groupCombo->Append( groupName );

        if( !sheetNames.empty() )
            m_placementProperties->m_sheetCombo->Select( 0 );

        if( !groupNames.empty() )
            m_placementProperties->m_groupCombo->Select( 0 );
    }

    auto setupCurrentSourceSelection = [&]( wxComboBox* cb )
    {
        if( curSourceName == wxEmptyString )
            return;

        if( !cb->SetStringSelection( curSourceName ) )
        {
            m_notFoundPlacementSource = true;
            m_notFoundPlacementSourceName = curSourceName;
            wxString notFoundDisplayName = _( "Not found on board: " ) + curSourceName;
            cb->Insert( notFoundDisplayName, 0 );
            cb->Select( 0 );
        }
    };

    if( m_zonesettings.GetPlacementAreaSourceType() == PLACEMENT_SOURCE_T::SHEETNAME )
    {
        if( m_zonesettings.GetPlacementAreaEnabled() )
            m_placementProperties->m_SheetRb->SetValue( true );

        setupCurrentSourceSelection( m_placementProperties->m_sheetCombo );
        m_originalPlacementSourceType = PLACEMENT_SOURCE_T::SHEETNAME;
        m_lastPlacementSourceType = PLACEMENT_SOURCE_T::SHEETNAME;
    }
    else if( m_zonesettings.GetPlacementAreaSourceType() == PLACEMENT_SOURCE_T::COMPONENT_CLASS )
    {
        if( m_zonesettings.GetPlacementAreaEnabled() )
            m_placementProperties->m_ComponentsRb->SetValue( true );

        setupCurrentSourceSelection( m_placementProperties->m_componentClassCombo );
        m_originalPlacementSourceType = PLACEMENT_SOURCE_T::COMPONENT_CLASS;
        m_lastPlacementSourceType = PLACEMENT_SOURCE_T::COMPONENT_CLASS;
    }
    else
    {
        if( m_zonesettings.GetPlacementAreaEnabled() )
            m_placementProperties->m_GroupRb->SetValue( true );

        setupCurrentSourceSelection( m_placementProperties->m_groupCombo );
        m_originalPlacementSourceType = PLACEMENT_SOURCE_T::GROUP_PLACEMENT;
        m_lastPlacementSourceType = PLACEMENT_SOURCE_T::GROUP_PLACEMENT;
    }

    // Handle most-useful notebook page selection
    m_areaPropertiesNb->SetSelection( 0 );

    if( !m_zonesettings.HasKeepoutParametersSet() && m_zonesettings.GetPlacementAreaEnabled() )
        m_areaPropertiesNb->SetSelection( 1 );


    m_cbLocked->SetValue( m_zonesettings.m_Locked );
    m_tcName->SetValue( m_zonesettings.m_Name );

    switch( m_zonesettings.m_ZoneBorderDisplayStyle )
    {
    case ZONE_BORDER_DISPLAY_STYLE::INVISIBLE_BORDER:   // Not used for standard zones. Here use NO_HATCH
    case ZONE_BORDER_DISPLAY_STYLE::NO_HATCH:      m_OutlineDisplayCtrl->SetSelection( 0 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE: m_OutlineDisplayCtrl->SetSelection( 1 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL: m_OutlineDisplayCtrl->SetSelection( 2 ); break;
    }

    m_outlineHatchPitch.SetValue( m_zonesettings.m_BorderHatchPitch );

    SetInitialFocus( m_OutlineDisplayCtrl );
    Layout();

    return true;
}


void DIALOG_RULE_AREA_PROPERTIES::OnLayerSelection( wxDataViewEvent& event )
{
    if( event.GetColumn() != 0 )
        return;

    int row = m_layers->ItemToRow( event.GetItem() );
    wxVariant layerID;
    m_layers->GetValue( layerID, row, LAYER_LIST_COLUMN_NAME );
    bool selected = m_layers->GetToggleValue( row, LAYER_LIST_COLUMN_CHECK );

    const auto setSelectedLayer = [&]()
    {
        m_zonesettings.m_Layers.set( ToLAYER_ID( layerID.GetInteger() ), selected );
    };

    // In footprint editor, we may in "expand inner layer" mode, where we
    // have only 3 possible layer selection: C_Cu, inner layers, B_Cu.
    // So row LAYER_LIST_ROW_ALL_INNER_LAYERS selection is fp editor specific.
    // in board editor, this row is a normal selection
    if( m_isFpEditor && row == LAYER_LIST_ROW_ALL_INNER_LAYERS )
    {
        const FOOTPRINT* fp = static_cast<FOOTPRINT*>( m_parent->GetModel() );

        if( !fp || fp->GetStackupMode() == FOOTPRINT_STACKUP::EXPAND_INNER_LAYERS )
        {
            if( selected )
                m_zonesettings.m_Layers |= LSET::InternalCuMask();
            else
                m_zonesettings.m_Layers &= ~LSET::InternalCuMask();
        }
        else
        {
            // We have a custom stackup footprint, so select just that one layer
            setSelectedLayer();
        }
    }
    else
    {
        setSelectedLayer();
    }
}


bool DIALOG_RULE_AREA_PROPERTIES::TransferDataFromWindow()
{
    if( m_convertSettings )
    {
        if( m_rbBoundingHull->GetValue() )
        {
            m_convertSettings->m_Strategy = BOUNDING_HULL;
            m_convertSettings->m_Gap = m_gap->GetIntValue();
        }
        else
        {
            m_convertSettings->m_Strategy = CENTERLINE;
        }

        m_convertSettings->m_DeleteOriginals = m_cbDeleteOriginals->GetValue();
    }

    // Set keepout parameters:
    m_zonesettings.SetIsRuleArea( true );
    m_zonesettings.SetDoNotAllowTracks( m_keepoutProperties->m_cbTracksCtrl->GetValue() );
    m_zonesettings.SetDoNotAllowVias( m_keepoutProperties->m_cbViasCtrl->GetValue() );
    m_zonesettings.SetDoNotAllowZoneFills( m_keepoutProperties->m_cbCopperPourCtrl->GetValue() );
    m_zonesettings.SetDoNotAllowPads( m_keepoutProperties->m_cbPadsCtrl->GetValue() );
    m_zonesettings.SetDoNotAllowFootprints( m_keepoutProperties->m_cbFootprintsCtrl->GetValue() );

    // Set placement parameters
    m_zonesettings.SetPlacementAreaEnabled( false );
    m_zonesettings.SetPlacementAreaSource( wxEmptyString );

    auto setPlacementSource =
            [this]( PLACEMENT_SOURCE_T sourceType )
            {
                m_zonesettings.SetPlacementAreaSourceType( sourceType );

                wxComboBox* cb;

                if( sourceType == PLACEMENT_SOURCE_T::SHEETNAME )
                    cb = m_placementProperties->m_sheetCombo;
                else if( sourceType == PLACEMENT_SOURCE_T::COMPONENT_CLASS )
                    cb = m_placementProperties->m_componentClassCombo;
                else
                    cb = m_placementProperties->m_groupCombo;

                int selectedSourceIdx = cb->GetSelection();

                if( selectedSourceIdx != wxNOT_FOUND )
                {
                    if( selectedSourceIdx == 0 && m_notFoundPlacementSource
                        && m_originalPlacementSourceType == sourceType )
                    {
                        m_zonesettings.SetPlacementAreaSource( m_notFoundPlacementSourceName );
                    }
                    else
                    {
                        m_zonesettings.SetPlacementAreaSource( cb->GetString( selectedSourceIdx ) );
                    }
                }
            };

    if( m_placementProperties->m_SheetRb->GetValue() )
    {
        m_zonesettings.SetPlacementAreaEnabled( true );
        setPlacementSource( PLACEMENT_SOURCE_T::SHEETNAME );
    }
    else if( m_placementProperties->m_ComponentsRb->GetValue() )
    {
        m_zonesettings.SetPlacementAreaEnabled( true );
        setPlacementSource( PLACEMENT_SOURCE_T::COMPONENT_CLASS );
    }
    else if( m_placementProperties->m_GroupRb->GetValue() )
    {
        m_zonesettings.SetPlacementAreaEnabled( true );
        setPlacementSource( PLACEMENT_SOURCE_T::GROUP_PLACEMENT );
    }
    else
    {
        setPlacementSource( m_lastPlacementSourceType );
    }

    if( m_zonesettings.m_Layers.count() == 0 )
    {
        DisplayError( this, _( "No layers selected." ) );
        return false;
    }

    switch( m_OutlineDisplayCtrl->GetSelection() )
    {
    case 0:
        m_zonesettings.m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::NO_HATCH;
        break;
    case 1:
        m_zonesettings.m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE;
        break;
    case 2:
        m_zonesettings.m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL;
        break;
    }

    if( !m_outlineHatchPitch.Validate( pcbIUScale.mmToIU( ZONE_BORDER_HATCH_MINDIST_MM ),
                                       pcbIUScale.mmToIU( ZONE_BORDER_HATCH_MAXDIST_MM ) ) )
    {
        return false;
    }

    m_zonesettings.m_BorderHatchPitch = m_outlineHatchPitch.GetIntValue();

    m_zonesettings.m_Locked = m_cbLocked->GetValue();
    m_zonesettings.m_ZonePriority = 0;  // for a keepout, this param is not used.

    m_zonesettings.m_Name = m_tcName->GetValue();

    *m_ptr = m_zonesettings;
    return true;
}

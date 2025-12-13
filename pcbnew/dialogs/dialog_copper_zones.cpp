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

#include <wx/radiobut.h>
#include <wx/statbox.h>
#include <kiface_base.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <zones.h>
#include <widgets/unit_binder.h>
#include <widgets/std_bitmap_button.h>
#include <zone.h>
#include <zone_settings_bag.h>
#include <pad.h>
#include <grid_layer_box_helpers.h>
#include <tool/tool_manager.h>
#include <panel_zone_properties.h>
#include <dialog_copper_zones_base.h>
#include <tools/pcb_actions.h>


class DIALOG_COPPER_ZONE : public DIALOG_COPPER_ZONE_BASE
{
public:
    // The dialog can be closed for several reasons.
    enum RETVAL
    {
        COPPER_ZONE_CANCEL,
        COPPER_ZONE_OK,
        COPPER_ZONE_OPEN_ZONE_MANAGER
    };

    DIALOG_COPPER_ZONE( PCB_BASE_FRAME* aParent, ZONE* aZone, ZONE_SETTINGS* aSettings,
                        CONVERT_SETTINGS* aConvertSettings );

    ~DIALOG_COPPER_ZONE() override;

    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    RETVAL GetReturnValue() { return m_returnValue; }

private:
    void OnLayerSelection( wxDataViewEvent& event ) override;
    void OnUpdateUI( wxUpdateUIEvent& ) override;
    void onZoneManager( wxCommandEvent& event ) override;

private:
    PCB_BASE_FRAME*        m_Parent;
    ZONE*                  m_zone;
    ZONE_SETTINGS*         m_ptr;
    ZONE_SETTINGS_BAG      m_zoneSettingsBag;       // Local storage of settings

    CONVERT_SETTINGS*      m_convertSettings;
    wxRadioButton*         m_rbCenterline;
    wxRadioButton*         m_rbEnvelope;
    wxStaticText*          m_gapLabel;
    wxTextCtrl*            m_gapCtrl;
    wxStaticText*          m_gapUnits;
    UNIT_BINDER*           m_gap;
    wxCheckBox*            m_cbDeleteOriginals;
    PANEL_ZONE_PROPERTIES* m_panelZoneProperties;

    RETVAL                 m_returnValue;
};


int InvokeCopperZonesEditor( PCB_BASE_FRAME* aCaller, ZONE* aZone, ZONE_SETTINGS* aSettings,
                             CONVERT_SETTINGS* aConvertSettings )
{
    DIALOG_COPPER_ZONE dlg( aCaller, aZone, aSettings, aConvertSettings );

    // TODO: why does this need QuasiModal?
    dlg.ShowQuasiModal();

    switch( dlg.GetReturnValue() )
    {
    case DIALOG_COPPER_ZONE::COPPER_ZONE_OK:
        return wxID_OK;

    case DIALOG_COPPER_ZONE::COPPER_ZONE_OPEN_ZONE_MANAGER:
        aCaller->CallAfter(
                [aCaller]()
                {
                    aCaller->GetToolManager()->RunAction( PCB_ACTIONS::zonesManager );
                } );

        return wxID_OK;

    default:
    case DIALOG_COPPER_ZONE::COPPER_ZONE_CANCEL:
        return wxID_CANCEL;
    }
}


DIALOG_COPPER_ZONE::DIALOG_COPPER_ZONE( PCB_BASE_FRAME* aParent, ZONE* aZone, ZONE_SETTINGS* aSettings,
                                        CONVERT_SETTINGS* aConvertSettings ) :
        DIALOG_COPPER_ZONE_BASE( aParent ),
        m_Parent( aParent ),
        m_zone( aZone ),
        m_zoneSettingsBag( aZone, aSettings ),
        m_convertSettings( aConvertSettings ),
        m_rbCenterline( nullptr ),
        m_rbEnvelope( nullptr ),
        m_cbDeleteOriginals( nullptr ),
        m_returnValue( COPPER_ZONE_CANCEL )
{
    m_ptr = aSettings;
    aSettings->SetupLayersList( m_layers, m_Parent, LSET::AllCuMask( aParent->GetBoard()->GetCopperLayerCount() ) );

    if( aSettings->m_TeardropType != TEARDROP_TYPE::TD_NONE )
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

    // A zone still in creation (ie: not yet in the document) can't be edited by the Zone Manager
    if( !aZone )
        m_openZoneManager->Hide();

    m_panelZoneProperties = new PANEL_ZONE_PROPERTIES( this, aParent, m_zoneSettingsBag );
    m_sizerRight->Add( m_panelZoneProperties, 1, wxEXPAND, 5 );

    SetupStandardButtons();

    finishDialogSettings();
}


DIALOG_COPPER_ZONE::~DIALOG_COPPER_ZONE()
{
    delete m_gap;
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

    m_panelZoneProperties->SetZone( m_zone );
    return true;
}


void DIALOG_COPPER_ZONE::OnUpdateUI( wxUpdateUIEvent& )
{
    if( m_gap )
        m_gap->Enable( m_rbEnvelope->GetValue() );
}


bool DIALOG_COPPER_ZONE::TransferDataFromWindow()
{
    if( m_zoneSettingsBag.GetZoneSettings( m_zone )->m_Layers.empty() )
    {
        DisplayError( this, _( "No layer selected." ) );
        return false;
    }

    if( !m_panelZoneProperties->TransferZoneSettingsFromWindow() )
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

    *m_ptr = *m_zoneSettingsBag.GetZoneSettings( m_zone );
    m_returnValue = COPPER_ZONE_OK;
    return true;
}


void DIALOG_COPPER_ZONE::OnLayerSelection( wxDataViewEvent& event )
{
    if( event.GetColumn() != 0 )
        return;

    int row = m_layers->ItemToRow( event.GetItem() );

    bool checked = m_layers->GetToggleValue( row, 0 );

    wxVariant layerID;
    m_layers->GetValue( layerID, row, 2 );

    m_zoneSettingsBag.GetZoneSettings( m_zone )->m_Layers.set( ToLAYER_ID( layerID.GetInteger() ), checked );
}


void DIALOG_COPPER_ZONE::onZoneManager( wxCommandEvent& event )
{
    if( TransferDataFromWindow() )
    {
        m_returnValue = COPPER_ZONE_OPEN_ZONE_MANAGER;
        Close();
    }
}
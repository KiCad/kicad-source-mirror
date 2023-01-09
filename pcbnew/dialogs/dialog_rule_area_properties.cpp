/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2014 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2023 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <dialog_rule_area_properties_base.h>
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
                                 CONVERT_SETTINGS* aConvertSettings );

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnLayerSelection( wxDataViewEvent& event ) override;

private:
    UNIT_BINDER       m_outlineHatchPitch;
    PCB_BASE_FRAME*   m_parent;
    ZONE_SETTINGS     m_zonesettings;    ///< the working copy of zone settings
    ZONE_SETTINGS*    m_ptr;             ///< the pointer to the zone settings of the zone to edit
    bool              m_isFpEditor;

    CONVERT_SETTINGS* m_convertSettings;
    wxRadioButton*    m_rbCenterline;
    wxRadioButton*    m_rbEnvelope;
    wxCheckBox*       m_cbDeleteOriginals;
};


int InvokeRuleAreaEditor( PCB_BASE_FRAME* aCaller, ZONE_SETTINGS* aZoneSettings,
                          CONVERT_SETTINGS* aConvertSettings )
{
    DIALOG_RULE_AREA_PROPERTIES dlg( aCaller, aZoneSettings, aConvertSettings );

    return dlg.ShowModal();
}


DIALOG_RULE_AREA_PROPERTIES::DIALOG_RULE_AREA_PROPERTIES( PCB_BASE_FRAME* aParent,
                                                          ZONE_SETTINGS* aSettings,
                                                          CONVERT_SETTINGS* aConvertSettings ) :
        DIALOG_RULE_AREA_PROPERTIES_BASE( aParent ),
        m_outlineHatchPitch( aParent, m_stBorderHatchPitchText,
                             m_outlineHatchPitchCtrl, m_outlineHatchUnits ),
        m_convertSettings( aConvertSettings ),
        m_rbCenterline( nullptr ),
        m_rbEnvelope( nullptr ),
        m_cbDeleteOriginals( nullptr )
{
    m_parent = aParent;

    m_ptr = aSettings;
    m_zonesettings = *aSettings;

    if( aConvertSettings )
    {
        wxStaticBox*      bConvertBox = new wxStaticBox( this, wxID_ANY,
                                                         _( "Conversion Settings" ) );
        wxStaticBoxSizer* bConvertSizer = new wxStaticBoxSizer( bConvertBox, wxVERTICAL  );

        m_rbCenterline = new wxRadioButton( this, wxID_ANY, _( "Use centerlines" ) );
        bConvertSizer->Add( m_rbCenterline, 0, wxLEFT|wxRIGHT, 5 );

        bConvertSizer->AddSpacer( 2 );
        m_rbEnvelope = new wxRadioButton( this, wxID_ANY, _( "Create bounding hull" ) );
        bConvertSizer->Add( m_rbEnvelope, 0, wxLEFT|wxRIGHT, 5 );

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

    m_isFpEditor = m_parent->IsType( FRAME_FOOTPRINT_EDITOR );

    BOARD* board = m_parent->GetBoard();
    LSET   layers = LSET::AllBoardTechMask() | LSET::AllCuMask( board->GetCopperLayerCount() );

    m_zonesettings.SetupLayersList( m_layers, m_parent, layers, m_isFpEditor );

    SetupStandardButtons();

    finishDialogSettings();
}


bool DIALOG_RULE_AREA_PROPERTIES::TransferDataToWindow()
{
    if( m_convertSettings )
    {
        if( m_convertSettings->m_Strategy == BOUNDING_HULL )
            m_rbEnvelope->SetValue( true );
        else
            m_rbCenterline->SetValue( true );

        m_cbDeleteOriginals->SetValue( m_convertSettings->m_DeleteOriginals );
    }

    // Init keepout parameters:
    m_cbTracksCtrl->SetValue( m_zonesettings.GetDoNotAllowTracks() );
    m_cbViasCtrl->SetValue( m_zonesettings.GetDoNotAllowVias() );
    m_cbPadsCtrl->SetValue( m_zonesettings.GetDoNotAllowPads() );
    m_cbFootprintsCtrl->SetValue( m_zonesettings.GetDoNotAllowFootprints() );
    m_cbCopperPourCtrl->SetValue( m_zonesettings.GetDoNotAllowCopperPour() );

    m_cbLocked->SetValue( m_zonesettings.m_Locked );

    m_tcName->SetValue( m_zonesettings.m_Name );

    switch( m_zonesettings.m_ZoneBorderDisplayStyle )
    {
    case ZONE_BORDER_DISPLAY_STYLE::NO_HATCH:      m_OutlineDisplayCtrl->SetSelection( 0 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE: m_OutlineDisplayCtrl->SetSelection( 1 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL: m_OutlineDisplayCtrl->SetSelection( 2 ); break;
    }

    m_outlineHatchPitch.SetValue( m_zonesettings.m_BorderHatchPitch );

    SetInitialFocus( m_OutlineDisplayCtrl );

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

    // In footprint editor, we have only 3 possible layer selection: C_Cu, inner layers, B_Cu.
    // So row LAYER_LIST_ROW_ALL_INNER_LAYERS selection is fp editor specific.
    // in board editor, this row is a normal selection
    if( m_isFpEditor && row == LAYER_LIST_ROW_ALL_INNER_LAYERS )
    {
        if( selected )
            m_zonesettings.m_Layers |= LSET::InternalCuMask();
        else
            m_zonesettings.m_Layers &= ~LSET::InternalCuMask();
    }
    else
    {
        m_zonesettings.m_Layers.set( ToLAYER_ID( layerID.GetInteger() ), selected );
    }
}


bool DIALOG_RULE_AREA_PROPERTIES::TransferDataFromWindow()
{
    if( m_convertSettings )
    {
        if( m_rbEnvelope->GetValue() )
            m_convertSettings->m_Strategy = BOUNDING_HULL;
        else
            m_convertSettings->m_Strategy = CENTERLINE;

        m_convertSettings->m_DeleteOriginals = m_cbDeleteOriginals->GetValue();
    }

    // Init keepout parameters:
    m_zonesettings.SetIsRuleArea( true );
    m_zonesettings.SetDoNotAllowTracks( m_cbTracksCtrl->GetValue() );
    m_zonesettings.SetDoNotAllowVias( m_cbViasCtrl->GetValue() );
    m_zonesettings.SetDoNotAllowCopperPour( m_cbCopperPourCtrl->GetValue() );
    m_zonesettings.SetDoNotAllowPads( m_cbPadsCtrl->GetValue() );
    m_zonesettings.SetDoNotAllowFootprints( m_cbFootprintsCtrl->GetValue() );

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
        return false;

    m_zonesettings.m_BorderHatchPitch = m_outlineHatchPitch.GetValue();

    m_zonesettings.m_Locked = m_cbLocked->GetValue();
    m_zonesettings.m_ZonePriority = 0;  // for a keepout, this param is not used.

    m_zonesettings.m_Name = m_tcName->GetValue();

    *m_ptr = m_zonesettings;
    return true;
}



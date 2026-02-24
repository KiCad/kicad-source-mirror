/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "drc_re_via_style_overlay_panel.h"
#include "drc_re_via_style_constraint_data.h"
#include "drc_rule_editor_utils.h"

#include <dialogs/rule_editor_dialog_base.h>
#include <eda_base_frame.h>
#include <base_units.h>
#include <widgets/unit_binder.h>

#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/stattext.h>

DRC_RE_VIA_STYLE_OVERLAY_PANEL::DRC_RE_VIA_STYLE_OVERLAY_PANEL(
        wxWindow* aParent, DRC_RE_VIA_STYLE_CONSTRAINT_DATA* aData, EDA_UNITS aUnits ) :
        DRC_RE_BITMAP_OVERLAY_PANEL( aParent ),
        m_data( aData ),
        m_unitsProvider( pcbIUScale, aUnits )
{
    SetBackgroundBitmap( BITMAPS::constraint_via_style );

    std::vector<DRC_RE_FIELD_POSITION> positions = m_data->GetFieldPositions();

    wxWindow* eventSource = nullptr;

    for( wxWindow* win = aParent; win; win = win->GetParent() )
    {
        if( dynamic_cast<EDA_BASE_FRAME*>( win ) )
        {
            eventSource = win;
            break;
        }
    }

    // Via type dropdown
    const DRC_RE_FIELD_POSITION& viaTypePos = positions[4];

    wxArrayString choices;
    choices.Add( _( "Any" ) );
    choices.Add( _( "Through" ) );
    choices.Add( _( "Micro" ) );
    choices.Add( _( "Blind" ) );
    choices.Add( _( "Buried" ) );

    m_viaTypeChoice = new wxChoice( this, wxID_ANY, wxPoint( viaTypePos.xStart, viaTypePos.yTop ),
                                    wxSize( viaTypePos.xEnd - viaTypePos.xStart, -1 ), choices );

    m_viaTypeChoice->SetSelection( static_cast<int>( m_data->GetViaType() ) );

    m_viaTypeLabel = new wxStaticText( this, wxID_ANY, viaTypePos.labelText );
    wxSize labelSize = m_viaTypeLabel->GetBestSize();
    m_viaTypeLabel->SetPosition(
            wxPoint( viaTypePos.xStart - labelSize.GetWidth() - 4,
                     viaTypePos.yTop + ( m_viaTypeChoice->GetBestSize().GetHeight() - labelSize.GetHeight() ) / 2 ) );

    // Create via diameter fields (min/max)
    auto* minViaDiameterField = AddField<wxTextCtrl>( wxS( "min_via_diameter" ), positions[0], wxTE_PROCESS_ENTER );
    m_minViaDiameterBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, minViaDiameterField->GetControl(),
                                           minViaDiameterField->GetLabel(), false, false );
    minViaDiameterField->SetUnitBinder( m_minViaDiameterBinder.get() );

    auto* maxViaDiameterField = AddField<wxTextCtrl>( wxS( "max_via_diameter" ), positions[1], wxTE_PROCESS_ENTER );
    m_maxViaDiameterBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, maxViaDiameterField->GetControl(),
                                           maxViaDiameterField->GetLabel(), false, false );
    maxViaDiameterField->SetUnitBinder( m_maxViaDiameterBinder.get() );

    // Create via hole size fields (min/max)
    auto* minViaHoleField = AddField<wxTextCtrl>( wxS( "min_via_hole" ), positions[2], wxTE_PROCESS_ENTER );
    m_minViaHoleSizeBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, minViaHoleField->GetControl(),
                                           minViaHoleField->GetLabel(), false, false );
    minViaHoleField->SetUnitBinder( m_minViaHoleSizeBinder.get() );

    auto* maxViaHoleField = AddField<wxTextCtrl>( wxS( "max_via_hole" ), positions[3], wxTE_PROCESS_ENTER );
    m_maxViaHoleSizeBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, maxViaHoleField->GetControl(),
                                           maxViaHoleField->GetLabel(), false, false );
    maxViaHoleField->SetUnitBinder( m_maxViaHoleSizeBinder.get() );

    // Diameter row decorations
    {
        const DRC_RE_FIELD_POSITION& minPos = positions[0];
        const DRC_RE_FIELD_POSITION& maxPos = positions[1];
        int                          fieldHeight = minViaDiameterField->GetControl()->GetBestSize().GetHeight();

        auto*  openDia = new wxStaticText( this, wxID_ANY, wxS( "(" ) );
        wxSize openSize = openDia->GetBestSize();
        openDia->SetPosition( wxPoint( minPos.xStart - openSize.GetWidth() - 2,
                                       minPos.yTop + ( fieldHeight - openSize.GetHeight() ) / 2 ) );

        auto*         dashDia = new wxStaticText( this, wxID_ANY, wxS( "-" ) );
        wxSize        dashSize = dashDia->GetBestSize();
        wxStaticText* minMmLabel = minViaDiameterField->GetLabel();
        int           afterMinLabel = minMmLabel->GetPosition().x + minMmLabel->GetBestSize().GetWidth();
        int           gapMid = ( afterMinLabel + maxPos.xStart ) / 2;
        dashDia->SetPosition(
                wxPoint( gapMid - dashSize.GetWidth() / 2, minPos.yTop + ( fieldHeight - dashSize.GetHeight() ) / 2 ) );

        auto*         closeDia = new wxStaticText( this, wxID_ANY, wxS( ")" ) );
        wxSize        closeSize = closeDia->GetBestSize();
        wxStaticText* mmLabel = maxViaDiameterField->GetLabel();
        int           afterLabel = mmLabel->GetPosition().x + mmLabel->GetBestSize().GetWidth();
        closeDia->SetPosition( wxPoint( afterLabel + 2, minPos.yTop + ( fieldHeight - closeSize.GetHeight() ) / 2 ) );
    }

    // Hole row decorations
    {
        const DRC_RE_FIELD_POSITION& minPos = positions[2];
        const DRC_RE_FIELD_POSITION& maxPos = positions[3];
        int                          fieldHeight = minViaHoleField->GetControl()->GetBestSize().GetHeight();

        auto*  openHole = new wxStaticText( this, wxID_ANY, wxS( "(" ) );
        wxSize openSize = openHole->GetBestSize();
        openHole->SetPosition( wxPoint( minPos.xStart - openSize.GetWidth() - 2,
                                        minPos.yTop + ( fieldHeight - openSize.GetHeight() ) / 2 ) );


        auto*         dashHole = new wxStaticText( this, wxID_ANY, wxS( "-" ) );
        wxSize        dashSize = dashHole->GetBestSize();
        wxStaticText* minMmLabel = minViaHoleField->GetLabel();
        int           afterMinLabel = minMmLabel->GetPosition().x + minMmLabel->GetBestSize().GetWidth();
        int           gapMid = ( afterMinLabel + maxPos.xStart ) / 2;
        dashHole->SetPosition(
                wxPoint( gapMid - dashSize.GetWidth() / 2, minPos.yTop + ( fieldHeight - dashSize.GetHeight() ) / 2 ) );

        auto*         closeHole = new wxStaticText( this, wxID_ANY, wxS( ")" ) );
        wxSize        closeSize = closeHole->GetBestSize();
        wxStaticText* mmLabel = maxViaHoleField->GetLabel();
        int           afterLabel = mmLabel->GetPosition().x + mmLabel->GetBestSize().GetWidth();
        closeHole->SetPosition( wxPoint( afterLabel + 2, minPos.yTop + ( fieldHeight - closeSize.GetHeight() ) / 2 ) );
    }

    auto notifyModified = [this]( wxCommandEvent& )
    {
        RULE_EDITOR_DIALOG_BASE* dlg = RULE_EDITOR_DIALOG_BASE::GetDialog( this );
        if( dlg )
            dlg->SetModified();
    };

    auto notifySave = [this]( wxCommandEvent& aEvent )
    {
        RULE_EDITOR_DIALOG_BASE* dlg = RULE_EDITOR_DIALOG_BASE::GetDialog( this );
        if( dlg )
            dlg->OnSave( aEvent );
    };

    minViaDiameterField->GetControl()->Bind( wxEVT_TEXT, notifyModified );
    maxViaDiameterField->GetControl()->Bind( wxEVT_TEXT, notifyModified );
    minViaHoleField->GetControl()->Bind( wxEVT_TEXT, notifyModified );
    maxViaHoleField->GetControl()->Bind( wxEVT_TEXT, notifyModified );

    minViaDiameterField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );
    maxViaDiameterField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );
    minViaHoleField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );
    maxViaHoleField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );

    m_viaTypeChoice->Bind( wxEVT_CHOICE, notifyModified );

    // Position all fields and update the panel layout
    PositionFields();
    TransferDataToWindow();
}


bool DRC_RE_VIA_STYLE_OVERLAY_PANEL::TransferDataToWindow()
{
    if( !m_data )
        return false;

    // Convert mm values to internal units and set them in the unit binders
    m_minViaDiameterBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetMinViaDiameter() ) );
    m_maxViaDiameterBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetMaxViaDiameter() ) );

    m_minViaHoleSizeBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetMinViaHoleSize() ) );
    m_maxViaHoleSizeBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetMaxViaHoleSize() ) );

    m_viaTypeChoice->SetSelection( static_cast<int>( m_data->GetViaType() ) );

    return true;
}


bool DRC_RE_VIA_STYLE_OVERLAY_PANEL::TransferDataFromWindow()
{
    if( !m_data )
        return false;

    // Read values from unit binders and convert from internal units to mm
    m_data->SetMinViaDiameter( pcbIUScale.IUTomm( m_minViaDiameterBinder->GetDoubleValue() ) );
    m_data->SetMaxViaDiameter( pcbIUScale.IUTomm( m_maxViaDiameterBinder->GetDoubleValue() ) );

    m_data->SetMinViaHoleSize( pcbIUScale.IUTomm( m_minViaHoleSizeBinder->GetDoubleValue() ) );
    m_data->SetMaxViaHoleSize( pcbIUScale.IUTomm( m_maxViaHoleSizeBinder->GetDoubleValue() ) );

    m_data->SetViaType( static_cast<VIA_STYLE_TYPE>( m_viaTypeChoice->GetSelection() ) );

    return true;
}


bool DRC_RE_VIA_STYLE_OVERLAY_PANEL::ValidateInputs( int* aErrorCount, wxString* aValidationMessage )
{
    TransferDataFromWindow();

    VALIDATION_RESULT result = m_data->Validate();

    if( !result.isValid )
    {
        *aErrorCount = result.errors.size();

        for( size_t i = 0; i < result.errors.size(); i++ )
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage( i + 1, result.errors[i] );

        return false;
    }

    return true;
}


wxString DRC_RE_VIA_STYLE_OVERLAY_PANEL::GenerateRule( const RULE_GENERATION_CONTEXT& aContext )
{
    if( !m_data )
        return wxEmptyString;

    return m_data->GenerateRule( aContext );
}

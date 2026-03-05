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

#include "drc_re_routing_width_overlay_panel.h"
#include "drc_re_routing_width_constraint_data.h"
#include "drc_rule_editor_utils.h"
#include "drc_re_validator_numeric_ctrl.h"

#include <base_units.h>
#include <eda_base_frame.h>
#include <widgets/unit_binder.h>

#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <dialogs/rule_editor_dialog_base.h>


DRC_RE_ROUTING_WIDTH_OVERLAY_PANEL::DRC_RE_ROUTING_WIDTH_OVERLAY_PANEL(
        wxWindow* aParent,
        DRC_RE_ROUTING_WIDTH_CONSTRAINT_DATA* aData,
        EDA_UNITS aUnits ) :
        DRC_RE_BITMAP_OVERLAY_PANEL( aParent ),
        m_data( aData ),
        m_unitsProvider( pcbIUScale, aUnits )
{
    SetBackgroundBitmap( BITMAPS::constraint_routing_width );

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


    // Create opt width field
    auto* optWidthField = AddField<wxTextCtrl>( wxS( "opt_width" ), positions[0], wxTE_CENTRE | wxTE_PROCESS_ENTER );
    m_optWidthBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, optWidthField->GetControl(),
                                           optWidthField->GetLabel(), false, false );
    optWidthField->SetUnitBinder( m_optWidthBinder.get() );
    optWidthField->GetControl()->SetValidator( VALIDATOR_NUMERIC_CTRL( false, false ) );

    // Create width tolerance field
    auto* widthTolField =
            AddField<wxTextCtrl>( wxS( "width_tolerance" ), positions[1], wxTE_CENTRE | wxTE_PROCESS_ENTER );
    m_widthToleranceBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, widthTolField->GetControl(),
                                           widthTolField->GetLabel(), false, false );
    widthTolField->SetUnitBinder( m_widthToleranceBinder.get() );
    widthTolField->GetControl()->SetValidator( VALIDATOR_NUMERIC_CTRL( false, false ) );

    // Add ± label between opt width and tolerance
    {
        const DRC_RE_FIELD_POSITION& optPos = positions[0];
        const DRC_RE_FIELD_POSITION& tolPos = positions[1];
        int                          fieldHeight = optWidthField->GetControl()->GetBestSize().GetHeight();

        auto*         plusMinus = new wxStaticText( this, wxID_ANY, wxS( "\u00B1" ) );
        wxSize        pmSize = plusMinus->GetBestSize();
        wxStaticText* optMmLabel = optWidthField->GetLabel();
        int           afterOptLabel = optMmLabel->GetPosition().x + optMmLabel->GetBestSize().GetWidth();
        int           gapMid = ( afterOptLabel + tolPos.xStart ) / 2;
        plusMinus->SetPosition(
                wxPoint( gapMid - pmSize.GetWidth() / 2, optPos.yTop + ( fieldHeight - pmSize.GetHeight() ) / 2 ) );
    }

    auto notifyModified = [this]( wxCommandEvent& )
    {
        RULE_EDITOR_DIALOG_BASE* dlg = RULE_EDITOR_DIALOG_BASE::GetDialog( this );
        if( dlg )
            dlg->SetModified();
    };

    optWidthField->GetControl()->Bind( wxEVT_TEXT, notifyModified );
    widthTolField->GetControl()->Bind( wxEVT_TEXT, notifyModified );

    auto notifySave = [this]( wxCommandEvent& aEvent )
    {
        RULE_EDITOR_DIALOG_BASE* dlg = RULE_EDITOR_DIALOG_BASE::GetDialog( this );
        if( dlg )
            dlg->OnSave( aEvent );
    };

    optWidthField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );
    widthTolField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );

    // Position all fields and update the panel layout
    PositionFields();
    TransferDataToWindow();
}


bool DRC_RE_ROUTING_WIDTH_OVERLAY_PANEL::TransferDataToWindow()
{
    if( !m_data )
        return false;

    m_optWidthBinder->ChangeDoubleValue( pcbIUScale.mmToIU( m_data->GetOptWidth() ) );
    m_widthToleranceBinder->ChangeDoubleValue( pcbIUScale.mmToIU( m_data->GetWidthTolerance() ) );

    return true;
}


bool DRC_RE_ROUTING_WIDTH_OVERLAY_PANEL::TransferDataFromWindow()
{
    if( !m_data )
        return false;

    m_data->SetOptWidth( pcbIUScale.IUTomm( m_optWidthBinder->GetDoubleValue() ) );
    m_data->SetWidthTolerance( pcbIUScale.IUTomm( m_widthToleranceBinder->GetDoubleValue() ) );

    return true;
}


bool DRC_RE_ROUTING_WIDTH_OVERLAY_PANEL::ValidateInputs( int* aErrorCount,
                                                         wxString* aValidationMessage )
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


wxString DRC_RE_ROUTING_WIDTH_OVERLAY_PANEL::GenerateRule( const RULE_GENERATION_CONTEXT& aContext )
{
    if( !m_data )
        return wxEmptyString;

    return m_data->GenerateRule( aContext );
}

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

#include "drc_re_abs_length_two_overlay_panel.h"
#include "drc_re_abs_length_two_constraint_data.h"
#include "drc_rule_editor_utils.h"
#include "drc_re_validator_numeric_ctrl.h"

#include <base_units.h>
#include <eda_base_frame.h>
#include <widgets/unit_binder.h>

#include <wx/textctrl.h>
#include <wx/stattext.h>
#include <dialogs/rule_editor_dialog_base.h> 

DRC_RE_ABS_LENGTH_TWO_OVERLAY_PANEL::DRC_RE_ABS_LENGTH_TWO_OVERLAY_PANEL(
        wxWindow* aParent, DRC_RE_ABSOLUTE_LENGTH_TWO_CONSTRAINT_DATA* aData, EDA_UNITS aUnits ) :
        DRC_RE_BITMAP_OVERLAY_PANEL( aParent ),
        m_data( aData ),
        m_unitsProvider( pcbIUScale, aUnits )
{
    SetBackgroundBitmap( aData->GetOverlayBitmap() );

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

    // Create opt length field
    auto* optLengthField = AddField<wxTextCtrl>( wxS( "opt_length" ), positions[0], wxTE_CENTRE | wxTE_PROCESS_ENTER );
    m_optLengthBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, optLengthField->GetControl(),
                                           optLengthField->GetLabel(), false, false );
    optLengthField->SetUnitBinder( m_optLengthBinder.get() );
    optLengthField->GetControl()->SetValidator( VALIDATOR_NUMERIC_CTRL( false, false ) );

    // Create tolerance field
    auto* toleranceField = AddField<wxTextCtrl>( wxS( "tolerance" ), positions[1], wxTE_CENTRE | wxTE_PROCESS_ENTER );
    m_toleranceBinder =
            std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, toleranceField->GetControl(),
                                           toleranceField->GetLabel(), false, false );
    toleranceField->SetUnitBinder( m_toleranceBinder.get() );
    toleranceField->GetControl()->SetValidator( VALIDATOR_NUMERIC_CTRL( false, false ) );

    // Add +/- decoration between the two fields (same pattern as via style)
    {
        const DRC_RE_FIELD_POSITION& optPos = positions[0];
        const DRC_RE_FIELD_POSITION& tolPos = positions[1];
        int                          fieldHeight = optLengthField->GetControl()->GetBestSize().GetHeight();

        auto*         plusMinus = new wxStaticText( this, wxID_ANY, wxS( "\u00B1" ) );
        wxSize        pmSize = plusMinus->GetBestSize();
        wxStaticText* optMmLabel = optLengthField->GetLabel();
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

    optLengthField->GetControl()->Bind( wxEVT_TEXT, notifyModified );
    toleranceField->GetControl()->Bind( wxEVT_TEXT, notifyModified );

    auto notifySave = [this]( wxCommandEvent& aEvent )
    {
        RULE_EDITOR_DIALOG_BASE* dlg = RULE_EDITOR_DIALOG_BASE::GetDialog( this );
        if( dlg )
            dlg->OnSave( aEvent );
    };

    optLengthField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );
    toleranceField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );

    // Position all fields and update the panel layout
    PositionFields();
    TransferDataToWindow();
}


bool DRC_RE_ABS_LENGTH_TWO_OVERLAY_PANEL::TransferDataToWindow()
{
    if( !m_data )
        return false;

    m_optLengthBinder->ChangeDoubleValue( pcbIUScale.mmToIU( m_data->GetOptimumLength() ) );
    m_toleranceBinder->ChangeDoubleValue( pcbIUScale.mmToIU( m_data->GetTolerance() ) );

    return true;
}


bool DRC_RE_ABS_LENGTH_TWO_OVERLAY_PANEL::TransferDataFromWindow()
{
    if( !m_data )
        return false;

    m_data->SetOptimumLength( pcbIUScale.IUTomm( m_optLengthBinder->GetDoubleValue() ) );
    m_data->SetTolerance( pcbIUScale.IUTomm( m_toleranceBinder->GetDoubleValue() ) );

    return true;
}


bool DRC_RE_ABS_LENGTH_TWO_OVERLAY_PANEL::ValidateInputs( int* aErrorCount,
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


wxString DRC_RE_ABS_LENGTH_TWO_OVERLAY_PANEL::GenerateRule( const RULE_GENERATION_CONTEXT& aContext )
{
    if( !m_data )
        return wxEmptyString;

    return m_data->GenerateRule( aContext );
}

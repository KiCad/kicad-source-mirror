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

#include "drc_re_numeric_input_overlay_panel.h"
#include "drc_re_numeric_input_constraint_data.h"
#include "drc_rule_editor_utils.h"
#include "drc_re_validator_numeric_ctrl.h"

#include <dialogs/rule_editor_dialog_base.h>
#include <eda_base_frame.h>
#include <base_units.h>
#include <widgets/unit_binder.h>

#include <wx/textctrl.h>


DRC_RE_NUMERIC_INPUT_OVERLAY_PANEL::DRC_RE_NUMERIC_INPUT_OVERLAY_PANEL(
        wxWindow* aParent, DRC_RE_NUMERIC_INPUT_CONSTRAINT_DATA* aData, EDA_UNITS aUnits ) :
        DRC_RE_BITMAP_OVERLAY_PANEL( aParent ),
        m_data( aData ),
        m_unitsProvider( pcbIUScale, aUnits )
{
    SetBackgroundBitmap( m_data->GetOverlayBitmap() );

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

    auto* valueField = AddField<wxTextCtrl>( wxS( "value" ), positions[0], wxTE_PROCESS_ENTER | wxTE_CENTRE );
    m_valueBinder = std::make_unique<UNIT_BINDER>( &m_unitsProvider, eventSource, nullptr, valueField->GetControl(),
                                                   valueField->GetLabel(), false, false );
    valueField->SetUnitBinder( m_valueBinder.get() );

    valueField->GetControl()->SetValidator( VALIDATOR_NUMERIC_CTRL( false, m_data->IsIntegerOnly() ) );

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

    valueField->GetControl()->Bind( wxEVT_TEXT, notifyModified );
    valueField->GetControl()->Bind( wxEVT_TEXT_ENTER, notifySave );

    PositionFields();
    TransferDataToWindow();
}


bool DRC_RE_NUMERIC_INPUT_OVERLAY_PANEL::TransferDataToWindow()
{
    if( !m_data )
        return false;

    if( m_data->IsIntegerOnly() )
    {
        auto* ctrl = dynamic_cast<wxTextCtrl*>( m_fields[0]->GetControl() );

        if( ctrl )
            ctrl->SetValue( wxString::Format( "%d", (int) m_data->GetNumericInputValue() ) );
    }
    else
    {
        m_valueBinder->SetDoubleValue( pcbIUScale.mmToIU( m_data->GetNumericInputValue() ) );
    }

    return true;
}


bool DRC_RE_NUMERIC_INPUT_OVERLAY_PANEL::TransferDataFromWindow()
{
    if( !m_data )
        return false;

    if( m_data->IsIntegerOnly() )
    {
        auto* ctrl = dynamic_cast<wxTextCtrl*>( m_fields[0]->GetControl() );

        if( ctrl )
        {
            long val = 0;
            ctrl->GetValue().Strip( wxString::both ).ToLong( &val );
            m_data->SetNumericInputValue( static_cast<double>( val ) );
        }
    }
    else
    {
        m_data->SetNumericInputValue( pcbIUScale.IUTomm( m_valueBinder->GetDoubleValue() ) );
    }

    return true;
}


bool DRC_RE_NUMERIC_INPUT_OVERLAY_PANEL::ValidateInputs( int* aErrorCount,
                                                         wxString* aValidationMessage )
{
    ClearFieldErrors();

    wxTextCtrl* ctrl = dynamic_cast<wxTextCtrl*>( m_fields[0]->GetControl() ); // "value" field is index 0

    if( ctrl )
    {
        wxValidator* validator = ctrl->GetValidator();

        if( validator && !validator->Validate( this ) )
        {
            auto* numValidator = dynamic_cast<VALIDATOR_NUMERIC_CTRL*>( validator );

            if( numValidator )
            {
                wxString errorMsg;

                switch( numValidator->GetValidationState() )
                {
                case VALIDATOR_NUMERIC_CTRL::VALIDATION_STATE::Empty: errorMsg = wxS( "Value is required." ); break;
                case VALIDATOR_NUMERIC_CTRL::VALIDATION_STATE::NotNumeric:
                    errorMsg = wxS( "Value must be a number." );
                    break;
                case VALIDATOR_NUMERIC_CTRL::VALIDATION_STATE::NotInteger:
                    errorMsg = wxS( "Value must be a whole number." );
                    break;
                case VALIDATOR_NUMERIC_CTRL::VALIDATION_STATE::NotGreaterThanZero:
                    errorMsg = wxS( "Value must be greater than 0." );
                    break;
                default: break;
                }

                if( !errorMsg.IsEmpty() )
                {
                    ( *aErrorCount )++;
                    *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage( *aErrorCount, errorMsg );
                    ShowFieldError( wxS( "value" ) );
                    return false;
                }
            }
        }
    }

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


wxString DRC_RE_NUMERIC_INPUT_OVERLAY_PANEL::GenerateRule( const RULE_GENERATION_CONTEXT& aContext )
{
    if( !m_data )
        return wxEmptyString;

    return m_data->GenerateRule( aContext );
}

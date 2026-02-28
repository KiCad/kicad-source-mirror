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

#include "drc_re_allowed_orientation_overlay_panel.h"
#include "drc_re_allowed_orientation_constraint_data.h"
#include "drc_rule_editor_utils.h"

#include <dialogs/rule_editor_dialog_base.h>
#include <wx/checkbox.h>


DRC_RE_ALLOWED_ORIENTATION_OVERLAY_PANEL::DRC_RE_ALLOWED_ORIENTATION_OVERLAY_PANEL(
        wxWindow* aParent, DRC_RE_ALLOWED_ORIENTATION_CONSTRAINT_DATA* aData ) :
        DRC_RE_BITMAP_OVERLAY_PANEL( aParent ),
        m_data( aData ),
        m_zeroDegreesCheckbox( nullptr ),
        m_ninetyDegreesCheckbox( nullptr ),
        m_oneEightyDegreesCheckbox( nullptr ),
        m_twoSeventyDegreesCheckbox( nullptr ),
        m_allDegreesCheckbox( nullptr )
{
    SetBackgroundBitmap( m_data->GetOverlayBitmap() );

    std::vector<DRC_RE_FIELD_POSITION> positions = m_data->GetFieldPositions();

    DRC_RE_OVERLAY_FIELD* field = AddCheckbox( wxS( "zero_degrees" ), positions[0] );
    m_zeroDegreesCheckbox = static_cast<wxCheckBox*>( field->GetControl() );

    field = AddCheckbox( wxS( "ninety_degrees" ), positions[1] );
    m_ninetyDegreesCheckbox = static_cast<wxCheckBox*>( field->GetControl() );

    field = AddCheckbox( wxS( "one_eighty_degrees" ), positions[2] );
    m_oneEightyDegreesCheckbox = static_cast<wxCheckBox*>( field->GetControl() );

    field = AddCheckbox( wxS( "two_seventy_degrees" ), positions[3] );
    m_twoSeventyDegreesCheckbox = static_cast<wxCheckBox*>( field->GetControl() );

    field = AddCheckbox( wxS( "all_degrees" ), positions[4] );
    m_allDegreesCheckbox = static_cast<wxCheckBox*>( field->GetControl() );

    auto notifyModified = [this]( wxCommandEvent& )
    {
        RULE_EDITOR_DIALOG_BASE* dlg = RULE_EDITOR_DIALOG_BASE::GetDialog( this );
        if( dlg )
            dlg->SetModified();
    };

    auto onIndividualCheckbox = [this, notifyModified]( wxCommandEvent& evt )
    {
        bool allChecked = m_zeroDegreesCheckbox->GetValue() && m_ninetyDegreesCheckbox->GetValue()
                          && m_oneEightyDegreesCheckbox->GetValue() && m_twoSeventyDegreesCheckbox->GetValue();
        m_allDegreesCheckbox->SetValue( allChecked );

        notifyModified( evt );
    };

    auto onAllDegreesCheckbox = [this, notifyModified]( wxCommandEvent& evt )
    {
        bool checked = m_allDegreesCheckbox->GetValue();
        m_zeroDegreesCheckbox->SetValue( checked );
        m_ninetyDegreesCheckbox->SetValue( checked );
        m_oneEightyDegreesCheckbox->SetValue( checked );
        m_twoSeventyDegreesCheckbox->SetValue( checked );

        notifyModified( evt );
    };

    m_zeroDegreesCheckbox->Bind( wxEVT_CHECKBOX, onIndividualCheckbox );
    m_ninetyDegreesCheckbox->Bind( wxEVT_CHECKBOX, onIndividualCheckbox );
    m_oneEightyDegreesCheckbox->Bind( wxEVT_CHECKBOX, onIndividualCheckbox );
    m_twoSeventyDegreesCheckbox->Bind( wxEVT_CHECKBOX, onIndividualCheckbox );
    m_allDegreesCheckbox->Bind( wxEVT_CHECKBOX, onAllDegreesCheckbox );

    PositionFields();
    TransferDataToWindow();
}


bool DRC_RE_ALLOWED_ORIENTATION_OVERLAY_PANEL::TransferDataToWindow()
{
    if( !m_data || !m_zeroDegreesCheckbox || !m_ninetyDegreesCheckbox || !m_oneEightyDegreesCheckbox
        || !m_twoSeventyDegreesCheckbox || !m_allDegreesCheckbox )
    {
        return false;
    }

    m_zeroDegreesCheckbox->SetValue( m_data->GetIsZeroDegreesAllowed() );
    m_ninetyDegreesCheckbox->SetValue( m_data->GetIsNinetyDegreesAllowed() );
    m_oneEightyDegreesCheckbox->SetValue( m_data->GetIsOneEightyDegreesAllowed() );
    m_twoSeventyDegreesCheckbox->SetValue( m_data->GetIsTwoSeventyDegreesAllowed() );
    m_allDegreesCheckbox->SetValue( m_data->GetIsAllDegreesAllowed() );

    return true;
}


bool DRC_RE_ALLOWED_ORIENTATION_OVERLAY_PANEL::TransferDataFromWindow()
{
    if( !m_data || !m_zeroDegreesCheckbox || !m_ninetyDegreesCheckbox || !m_oneEightyDegreesCheckbox
        || !m_twoSeventyDegreesCheckbox || !m_allDegreesCheckbox )
    {
        return false;
    }

    m_data->SetIsZeroDegreesAllowed( m_zeroDegreesCheckbox->GetValue() );
    m_data->SetIsNinetyDegreesAllowed( m_ninetyDegreesCheckbox->GetValue() );
    m_data->SetIsOneEightyDegreesAllowed( m_oneEightyDegreesCheckbox->GetValue() );
    m_data->SetIsTwoSeventyDegreesAllowed( m_twoSeventyDegreesCheckbox->GetValue() );
    m_data->SetIsAllDegreesAllowed( m_zeroDegreesCheckbox->GetValue() && m_ninetyDegreesCheckbox->GetValue()
                                    && m_oneEightyDegreesCheckbox->GetValue()
                                    && m_twoSeventyDegreesCheckbox->GetValue() );

    return true;
}


bool DRC_RE_ALLOWED_ORIENTATION_OVERLAY_PANEL::ValidateInputs( int* aErrorCount,
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


wxString DRC_RE_ALLOWED_ORIENTATION_OVERLAY_PANEL::GenerateRule( const RULE_GENERATION_CONTEXT& aContext )
{
    if( !m_data )
        return wxEmptyString;

    return m_data->GenerateRule( aContext );
}

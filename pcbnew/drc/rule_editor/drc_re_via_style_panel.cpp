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

#include "drc_re_via_style_panel.h"

#include <wx/log.h>


DRC_RE_VIA_STYLE_PANEL::DRC_RE_VIA_STYLE_PANEL( wxWindow* aParent, wxString* aConstraintTitle,
        std::shared_ptr<DRC_RE_VIA_STYLE_CONSTRAINT_DATA> aConstraintData ) :
        DRC_RE_VIA_STYLE_PANEL_BASE( aParent ), m_constraintData( aConstraintData )
{
    bConstraintImageSizer->Add( GetConstraintImage( this, BITMAPS::constraint_via_style ), 0,
                                wxALL | wxEXPAND, 10 );
}


DRC_RE_VIA_STYLE_PANEL::~DRC_RE_VIA_STYLE_PANEL()
{
}


bool DRC_RE_VIA_STYLE_PANEL::TransferDataToWindow()
{
    if( m_constraintData )
    {
        m_minViaDiameterTextCtrl->SetValue(
                wxString::Format( _( "%.2f" ), m_constraintData->GetMinViaDiameter() ) );
        m_maxViaDiameterTextCtrl->SetValue(
                wxString::Format( _( "%.2f" ), m_constraintData->GetMaxViaDiameter() ) );
        m_preferredViaDiameterTextCtrl->SetValue(
                wxString::Format( _( "%.2f" ), m_constraintData->GetPreferredViaDiameter() ) );

        m_minViaHoleSizeTextCtrl->SetValue(
                wxString::Format( _( "%.2f" ), m_constraintData->GetMinViaHoleSize() ) );
        m_maxViaHoleSizeTextCtrl->SetValue(
                wxString::Format( _( "%.2f" ), m_constraintData->GetMaxViaHoleSize() ) );
        m_preferredViaHoleSizeTextCtrl->SetValue(
                wxString::Format( _( "%.2f" ), m_constraintData->GetPreferredViaHoleSize() ) );
    }

    return true;
}


bool DRC_RE_VIA_STYLE_PANEL::TransferDataFromWindow()
{
    m_constraintData->SetMinViaDiameter(
            std::stod( m_minViaDiameterTextCtrl->GetValue().ToStdString() ) );
    m_constraintData->SetMaxViaDiameter(
            std::stod( m_maxViaDiameterTextCtrl->GetValue().ToStdString() ) );
    m_constraintData->SetPreferredViaDiameter(
            std::stod( m_preferredViaDiameterTextCtrl->GetValue().ToStdString() ) );

    m_constraintData->SetMinViaHoleSize(
            std::stod( m_minViaHoleSizeTextCtrl->GetValue().ToStdString() ) );
    m_constraintData->SetMaxViaHoleSize(
            std::stod( m_maxViaHoleSizeTextCtrl->GetValue().ToStdString() ) );
    m_constraintData->SetPreferredViaHoleSize(
            std::stod( m_preferredViaHoleSizeTextCtrl->GetValue().ToStdString() ) );

    return true;
}


bool DRC_RE_VIA_STYLE_PANEL::ValidateInputs( int* aErrorCount, std::string* aValidationMessage )
{
    TransferDataFromWindow();

    VALIDATION_RESULT result = m_constraintData->Validate();

    if( !result.isValid )
    {
        *aErrorCount = result.errors.size();
        for( size_t i = 0; i < result.errors.size(); i++ )
            *aValidationMessage += DRC_RULE_EDITOR_UTILS::FormatErrorMessage( i + 1, result.errors[i] );
        return false;
    }

    return true;
}


wxString DRC_RE_VIA_STYLE_PANEL::GenerateRule( const RULE_GENERATION_CONTEXT& aContext )
{
    if( !m_constraintData )
        return wxEmptyString;

    return m_constraintData->GenerateRule( aContext );
}

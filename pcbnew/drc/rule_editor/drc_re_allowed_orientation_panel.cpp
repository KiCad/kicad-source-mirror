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

#include "drc_re_allowed_orientation_panel.h"

#include <wx/arrstr.h>
#include <wx/log.h>


DRC_RE_ALLOWED_ORIENTATION_PANEL::DRC_RE_ALLOWED_ORIENTATION_PANEL( wxWindow* aParent,
        wxString* aConstraintTitle,
        std::shared_ptr<DRC_RE_ALLOWED_ORIENTATION_CONSTRAINT_DATA> aConstraintData ) :
        DRC_RE_ALLOWED_ORIENTATION_PANEL_BASE( aParent ), m_constraintData( aConstraintData )
{
    bConstraintImageSizer->Add( GetConstraintImage( this, BITMAPS::constraint_allowed_orientation ),
        0, wxALL | wxEXPAND, 10 );

    m_zeroDegreesChkCtrl->Bind( wxEVT_CHECKBOX,
                                &DRC_RE_ALLOWED_ORIENTATION_PANEL::onCheckboxClicked, this );
    m_ninetyDegreesChkCtrl->Bind( wxEVT_CHECKBOX,
                                 &DRC_RE_ALLOWED_ORIENTATION_PANEL::onCheckboxClicked, this );
    m_oneEightyDegreesChkCtrl->Bind( wxEVT_CHECKBOX,
                                     &DRC_RE_ALLOWED_ORIENTATION_PANEL::onCheckboxClicked, this );
    m_twoSeventyDegreesChkCtrl->Bind( wxEVT_CHECKBOX,
                                      &DRC_RE_ALLOWED_ORIENTATION_PANEL::onCheckboxClicked, this );
    m_allOrientationsChkCtrl->Bind( wxEVT_CHECKBOX,
                                    &DRC_RE_ALLOWED_ORIENTATION_PANEL::onAllOrientationCheckboxClicked,
                                    this );
}


DRC_RE_ALLOWED_ORIENTATION_PANEL::~DRC_RE_ALLOWED_ORIENTATION_PANEL()
{
    m_zeroDegreesChkCtrl->Unbind( wxEVT_CHECKBOX,
                                  &DRC_RE_ALLOWED_ORIENTATION_PANEL::onCheckboxClicked, this );
    m_ninetyDegreesChkCtrl->Unbind( wxEVT_CHECKBOX,
                                   &DRC_RE_ALLOWED_ORIENTATION_PANEL::onCheckboxClicked, this );
    m_oneEightyDegreesChkCtrl->Unbind( wxEVT_CHECKBOX,
                                       &DRC_RE_ALLOWED_ORIENTATION_PANEL::onCheckboxClicked, this );
    m_twoSeventyDegreesChkCtrl->Unbind( wxEVT_CHECKBOX,
                                        &DRC_RE_ALLOWED_ORIENTATION_PANEL::onCheckboxClicked, this );
    m_allOrientationsChkCtrl->Unbind( wxEVT_CHECKBOX,
                                      &DRC_RE_ALLOWED_ORIENTATION_PANEL::onAllOrientationCheckboxClicked,
                                      this );
}


bool DRC_RE_ALLOWED_ORIENTATION_PANEL::TransferDataToWindow()
{
    if( m_constraintData )
    {
        m_zeroDegreesChkCtrl->SetValue( m_constraintData->GetIsZeroDegreesAllowed() );
        m_ninetyDegreesChkCtrl->SetValue( m_constraintData->GetIsNinetyDegreesAllowed() );
        m_oneEightyDegreesChkCtrl->SetValue( m_constraintData->GetIsOneEightyDegreesAllowed() );
        m_twoSeventyDegreesChkCtrl->SetValue( m_constraintData->GetIsTwoSeventyDegreesAllowed() );
        m_allOrientationsChkCtrl->SetValue( m_constraintData->GetIsAllDegreesAllowed() );
    }

    return true;
}


bool DRC_RE_ALLOWED_ORIENTATION_PANEL::TransferDataFromWindow()
{
    m_constraintData->SetIsZeroDegreesAllowed( m_zeroDegreesChkCtrl->GetValue() );
    m_constraintData->SetIsNinetyDegreesAllowed( m_ninetyDegreesChkCtrl->GetValue() );
    m_constraintData->SetIsOneEightyDegreesAllowed( m_oneEightyDegreesChkCtrl->GetValue() );
    m_constraintData->SetIsTwoSeventyDegreesAllowed( m_twoSeventyDegreesChkCtrl->GetValue() );
    m_constraintData->SetIsAllDegreesAllowed( m_allOrientationsChkCtrl->GetValue() );

    return true;
}

void DRC_RE_ALLOWED_ORIENTATION_PANEL::onCheckboxClicked( wxCommandEvent& event )
{
    if( !m_zeroDegreesChkCtrl->IsChecked() || !m_ninetyDegreesChkCtrl->IsChecked()
        || !m_oneEightyDegreesChkCtrl->IsChecked() || !m_twoSeventyDegreesChkCtrl->IsChecked() )
    {
        m_allOrientationsChkCtrl->SetValue( false );
    }
    else
    {
        m_allOrientationsChkCtrl->SetValue( true );
    }
}

void DRC_RE_ALLOWED_ORIENTATION_PANEL::onAllOrientationCheckboxClicked( wxCommandEvent& event )
{
    bool enable = false;

    if( m_allOrientationsChkCtrl->IsChecked() )
    {
        enable = true;
    }

    m_zeroDegreesChkCtrl->SetValue( enable );
    m_ninetyDegreesChkCtrl->SetValue( enable );
    m_oneEightyDegreesChkCtrl->SetValue( enable );
    m_twoSeventyDegreesChkCtrl->SetValue( enable );
}


bool DRC_RE_ALLOWED_ORIENTATION_PANEL::ValidateInputs( int* aErrorCount,
                                                       std::string* aValidationMessage )
{
    VALIDATION_RESULT result = m_constraintData->Validate();

    if( !result.isValid )
    {
        if( aErrorCount )
            *aErrorCount += static_cast<int>( result.errors.size() );

        if( aValidationMessage )
        {
            for( const std::string& error : result.errors )
            {
                if( !aValidationMessage->empty() )
                    *aValidationMessage += "\n";

                *aValidationMessage += error;
            }
        }
    }

    return result.isValid;
}


wxString DRC_RE_ALLOWED_ORIENTATION_PANEL::GenerateRule( const RULE_GENERATION_CONTEXT& aContext )
{
    if( !m_constraintData )
        return wxEmptyString;

    wxString code = m_constraintData->GetConstraintCode();

    if( code.IsEmpty() )
        code = wxS( "allowed_orientation" );

    wxString clause;

    if( m_constraintData->GetIsAllDegreesAllowed() )
    {
        clause = wxString::Format( wxS( "(constraint %s (any true))" ), code );
    }
    else
    {
        wxArrayString angles;

        if( m_constraintData->GetIsZeroDegreesAllowed() )
            angles.Add( wxS( "0" ) );
        if( m_constraintData->GetIsNinetyDegreesAllowed() )
            angles.Add( wxS( "90" ) );
        if( m_constraintData->GetIsOneEightyDegreesAllowed() )
            angles.Add( wxS( "180" ) );
        if( m_constraintData->GetIsTwoSeventyDegreesAllowed() )
            angles.Add( wxS( "270" ) );

        if( angles.IsEmpty() )
            clause = wxString::Format( wxS( "(constraint %s (angles none))" ), code );
        else
            clause = wxString::Format( wxS( "(constraint %s (angles %s))" ), code,
                                        wxJoin( angles, ' ' ) );
    }

    wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ), wxS( "Allowed orientation clause: %s" ), clause );

    return buildRule( aContext, { clause } );
}

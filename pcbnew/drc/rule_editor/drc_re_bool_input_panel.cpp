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

#include "drc_re_bool_input_panel.h"
#include "drc_rule_editor_utils.h"

#include <wx/log.h>


const std::map<DRC_RULE_EDITOR_CONSTRAINT_NAME, BITMAPS> BoolConstraintBitMapPairs =
{
    { VIAS_UNDER_SMD, BITMAPS::constraint_vias_under_smd },
};


DRC_RE_BOOL_INPUT_PANEL::DRC_RE_BOOL_INPUT_PANEL( wxWindow* aParent,
        const DRC_RE_BOOL_INPUT_CONSTRAINT_PANEL_PARAMS& aConstraintPanelParams ) :
        DRC_RE_BOOL_INPUT_PANEL_BASE( aParent ),
        m_constraintData( aConstraintPanelParams.m_constraintData )
{
    auto it = BoolConstraintBitMapPairs.find( aConstraintPanelParams.m_constraintType );

    bConstraintImageSizer->Add( GetConstraintImage( this, it->second ), 0, wxALL | wxEXPAND, 10 );

    if( !aConstraintPanelParams.m_customLabelText.IsEmpty() )
        m_boolConstraintChkCtrl->SetLabelText( aConstraintPanelParams.m_customLabelText );
    else
        m_boolConstraintChkCtrl->SetLabelText( aConstraintPanelParams.m_constraintTitle );
}


DRC_RE_BOOL_INPUT_PANEL::~DRC_RE_BOOL_INPUT_PANEL()
{
}


bool DRC_RE_BOOL_INPUT_PANEL::TransferDataToWindow()
{
    if( m_constraintData )
    {
        m_boolConstraintChkCtrl->SetValue( m_constraintData->GetBoolInputValue() );
    }

    return true;
}


bool DRC_RE_BOOL_INPUT_PANEL::TransferDataFromWindow()
{
    m_constraintData->SetBoolInputValue( m_boolConstraintChkCtrl->GetValue() );
    return true;
}


bool DRC_RE_BOOL_INPUT_PANEL::ValidateInputs( int* aErrorCount, std::string* aValidationMessage )
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


wxString DRC_RE_BOOL_INPUT_PANEL::GenerateRule( const RULE_GENERATION_CONTEXT& aContext )
{
    if( !m_constraintData )
        return wxEmptyString;

    wxString code = m_constraintData->GetConstraintCode();

    if( code.IsEmpty() )
        code = wxS( "boolean_constraint" );

    const bool enabled = m_constraintData->GetBoolInputValue();
    const wxString state = enabled ? wxS( "true" ) : wxS( "false" );

    wxLogTrace( wxS( "KI_TRACE_DRC_RULE_EDITOR" ),
                wxS( "Generating boolean constraint '%s' with state %s." ), code, state );

    wxString clause = wxString::Format( wxS( "(constraint %s (enabled %s))" ), code, state );
    return buildRule( aContext, { clause } );
}

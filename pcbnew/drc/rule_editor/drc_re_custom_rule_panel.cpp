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

#include "drc_re_custom_rule_panel.h"

#include <wx/sizer.h>

DRC_RE_CUSTOM_RULE_PANEL::DRC_RE_CUSTOM_RULE_PANEL(
        wxWindow* aParent, std::shared_ptr<DRC_RE_CUSTOM_RULE_CONSTRAINT_DATA> aConstraintData ) :
        wxPanel( aParent ),
        m_constraintData( aConstraintData )
{
    wxBoxSizer* sizer = new wxBoxSizer( wxVERTICAL );
    m_textCtrl = new wxStyledTextCtrl( this, wxID_ANY );
    sizer->Add( m_textCtrl, 1, wxEXPAND | wxALL, 5 );
    SetSizer( sizer );
}

bool DRC_RE_CUSTOM_RULE_PANEL::TransferDataToWindow()
{
    if( m_constraintData )
        m_textCtrl->SetValue( m_constraintData->GetRuleText() );
    return true;
}

bool DRC_RE_CUSTOM_RULE_PANEL::TransferDataFromWindow()
{
    if( m_constraintData )
        m_constraintData->SetRuleText( m_textCtrl->GetValue() );
    return true;
}

bool DRC_RE_CUSTOM_RULE_PANEL::ValidateInputs( int* aErrorCount, std::string* aValidationMessage )
{
    (void) aErrorCount;
    (void) aValidationMessage;
    return true;
}


wxString DRC_RE_CUSTOM_RULE_PANEL::GenerateRule( const RULE_GENERATION_CONTEXT& aContext )
{
    (void) aContext;

    if( m_constraintData )
        return m_constraintData->GetRuleText();

    if( m_textCtrl )
        return m_textCtrl->GetValue();

    return wxEmptyString;
}

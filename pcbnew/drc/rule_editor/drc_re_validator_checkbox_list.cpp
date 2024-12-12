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

#include "drc_re_validator_checkbox_list.h"


VALIDATE_CHECKBOX_LIST::VALIDATE_CHECKBOX_LIST( const std::vector<wxCheckBox*>& aCheckboxes ) :
        m_validationState( VALIDATION_STATE::Valid ), m_checkboxes( aCheckboxes )
{
}


wxObject* VALIDATE_CHECKBOX_LIST::Clone() const
{
    return new VALIDATE_CHECKBOX_LIST( m_checkboxes );
}


bool VALIDATE_CHECKBOX_LIST::Validate( wxWindow* aParent )
{
    for( wxCheckBox* checkbox : m_checkboxes )
    {
        if( checkbox->GetValue() )
        {
            m_validationState = VALIDATION_STATE::Valid; 
            return true;
        }
    }

    m_validationState = VALIDATION_STATE::NotSelected;
    return false;
}


VALIDATE_CHECKBOX_LIST::VALIDATION_STATE VALIDATE_CHECKBOX_LIST::GetValidationState() const
{
    return m_validationState;
}
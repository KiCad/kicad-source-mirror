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

#include "drc_re_validator_combo_ctrl.h"


VALIDATOR_COMBO_CTRL::VALIDATOR_COMBO_CTRL() : m_validationState( VALIDATION_STATE::Valid )
{
}


wxObject* VALIDATOR_COMBO_CTRL::Clone() const
{
    return new VALIDATOR_COMBO_CTRL();
}


bool VALIDATOR_COMBO_CTRL::Validate( wxWindow* aParent )
{
    wxComboBox* comboCtrl = wxDynamicCast( GetWindow(), wxComboBox );

    if( !comboCtrl )
    {
        m_validationState = VALIDATION_STATE::InValidCtrl;
        return false;
    }

    if( comboCtrl->GetSelection() == wxNOT_FOUND )
    {
        m_validationState = VALIDATION_STATE::NothingSelected;
        return false;
    }

    m_validationState = VALIDATION_STATE::Valid;
    return true;
}


VALIDATOR_COMBO_CTRL::VALIDATION_STATE VALIDATOR_COMBO_CTRL::GetValidationState() const
{
    return m_validationState;
}

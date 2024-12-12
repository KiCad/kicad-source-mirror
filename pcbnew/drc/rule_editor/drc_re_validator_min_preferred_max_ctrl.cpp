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

#include "drc_re_validator_min_preferred_max_ctrl.h"


VALIDATE_MIN_PREFERRED_MAX_CTRL::VALIDATE_MIN_PREFERRED_MAX_CTRL( wxTextCtrl* aMinCtrl,
                                                                  wxTextCtrl* aPreferredCtrl,
                                                                  wxTextCtrl* aMaxCtrl ) :
        m_minCtrl( aMinCtrl ), 
        m_preferredCtrl( aPreferredCtrl ), 
        m_maxCtrl( aMaxCtrl ),
        m_minCtrlName( aMinCtrl->GetName() ), 
        m_preferredCtrlName( aPreferredCtrl->GetName() ),
        m_maxCtrlName( aMaxCtrl->GetName() ), 
        m_validationState( VALIDATION_STATE::Valid )
{
}


wxObject* VALIDATE_MIN_PREFERRED_MAX_CTRL::Clone() const
{
    return new VALIDATE_MIN_PREFERRED_MAX_CTRL( m_minCtrl, m_preferredCtrl, m_maxCtrl );
}


bool VALIDATE_MIN_PREFERRED_MAX_CTRL::Validate( wxWindow* aParent )
{
    // Assume two text controls: one for min and one for max
    wxTextCtrl* minCtrl = wxDynamicCast( aParent->FindWindowByName( m_minCtrlName ), wxTextCtrl );
    wxTextCtrl* preferredCtrl =
            wxDynamicCast( aParent->FindWindowByName( m_preferredCtrlName ), wxTextCtrl );
    wxTextCtrl* maxCtrl = wxDynamicCast( aParent->FindWindowByName( m_maxCtrlName ), wxTextCtrl );

    if( !minCtrl || !maxCtrl || !preferredCtrl )
    {
        return false; // Controls not properly set
    }

    wxString minValueStr = minCtrl->GetValue();
    wxString preferredValueStr = preferredCtrl->GetValue();
    wxString maxValueStr = maxCtrl->GetValue();

    double minValue, preferredValue, maxValue;
    minValueStr.ToDouble( &minValue );
    preferredValueStr.ToDouble( &preferredValue );
    maxValueStr.ToDouble( &maxValue );

    if( minValue > preferredValue )
    {
        m_validationState = VALIDATION_STATE::MinGreaterThanPreferred;
        return false;
    }

    if( preferredValue > maxValue )
    {
        m_validationState = VALIDATION_STATE::PreferredGreaterThanMax;
        return false;
    }

    if( minValue > maxValue )
    {
        m_validationState = VALIDATION_STATE::MinGreaterThanMax;
        return false;
    }

    m_validationState = VALIDATION_STATE::Valid;
    return true;
}


VALIDATE_MIN_PREFERRED_MAX_CTRL::VALIDATION_STATE
VALIDATE_MIN_PREFERRED_MAX_CTRL::GetValidationState() const
{
    return m_validationState;
}
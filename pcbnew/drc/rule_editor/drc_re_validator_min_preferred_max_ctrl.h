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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef DRC_RULE_EDITOR_VALIDATOR_MIN_PREFERRED_MAX_CTRL_H_
#define DRC_RULE_EDITOR_VALIDATOR_MIN_PREFERRED_MAX_CTRL_H_

#include <wx/wx.h>
#include <wx/object.h>

#include "drc_rule_editor_enums.h"


class VALIDATE_MIN_PREFERRED_MAX_CTRL : public wxValidator
{
public:
    enum class VALIDATION_STATE
    {
        Valid,
        MinGreaterThanMax,
        MinGreaterThanPreferred,
        PreferredGreaterThanMax
    };

    VALIDATE_MIN_PREFERRED_MAX_CTRL( wxTextCtrl* aMinCtrl, wxTextCtrl* aPreferredCtrl,
                                     wxTextCtrl* aMaxCtrl );

    virtual wxObject* Clone() const override;

    virtual bool Validate( wxWindow* aParent ) override;

    VALIDATION_STATE GetValidationState() const;

private:
    wxTextCtrl*      m_minCtrl;
    wxTextCtrl*      m_preferredCtrl;
    wxTextCtrl*      m_maxCtrl;
    wxString         m_minCtrlName;
    wxString         m_preferredCtrlName;
    wxString         m_maxCtrlName;
    VALIDATION_STATE m_validationState;
};

#endif // DRC_RULE_EDITOR_VALIDATOR_MIN_PREFERRED_MAX_CTRL_H_
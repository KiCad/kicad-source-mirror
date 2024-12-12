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

#ifndef DRC_RULE_EDITOR_VALIDATOR_MIN_MAX_CTRL_H_
#define DRC_RULE_EDITOR_VALIDATOR_MIN_MAX_CTRL_H_

#include <wx/wx.h>
#include <wx/object.h>

#include <string>

#include "drc_rule_editor_enums.h"


class VALIDATE_MIN_MAX_CTRL : public wxValidator
{
public:
    enum class VALIDATION_STATE
    {
        Valid, 
        MinGreaterThanMax
    };

    VALIDATE_MIN_MAX_CTRL( wxTextCtrl* aMinCtrl, wxTextCtrl* aMaxCtrl );

    virtual wxObject* Clone() const override;

    virtual bool Validate( wxWindow* aParent ) override;

    VALIDATION_STATE GetValidationState() const;

private:
    wxTextCtrl* m_minCtrl;
    wxTextCtrl* m_maxCtrl;
    std::string m_minCtrlName;
    std::string m_maxCtrlName;
    VALIDATION_STATE m_validationState;
};

#endif // DRC_RULE_EDITOR_VALIDATOR_MIN_MAX_CTRL_H_
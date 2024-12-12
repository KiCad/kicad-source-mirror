/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2024 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef DRC_RE_ALLOWED_ORIENTATION_PANEL_H
#define DRC_RE_ALLOWED_ORIENTATION_PANEL_H

#include "drc_re_allowed_orientation_panel_base.h"
#include "drc_re_content_panel_base.h"
#include "drc_re_allowed_orientation_constraint_data.h"
#include "drc_rule_editor_utils.h"


class DRC_RE_ALLOWED_ORIENTATION_PANEL : public DRC_RE_ALLOWED_ORIENTATION_PANEL_BASE,
                                         public DRC_RULE_EDITOR_CONTENT_PANEL_BASE
{
public:
    DRC_RE_ALLOWED_ORIENTATION_PANEL( wxWindow* aParent, wxString* aConstraintTitle,
            std::shared_ptr<DRC_RE_ALLOWED_ORIENTATION_CONSTRAINT_DATA> aConstraintData );

    ~DRC_RE_ALLOWED_ORIENTATION_PANEL() override;

    bool TransferDataToWindow() override;

    bool TransferDataFromWindow() override;

    bool ValidateInputs( int* aErrorCount, std::string* aValidationMessage ) override;

    wxString GenerateRule( const RULE_GENERATION_CONTEXT& aContext ) override;

private:
    void onCheckboxClicked( wxCommandEvent& aEvent );

    void onAllOrientationCheckboxClicked( wxCommandEvent& aEvent );

private:
    std::shared_ptr<DRC_RE_ALLOWED_ORIENTATION_CONSTRAINT_DATA> m_constraintData;
};

#endif // DRC_RE_ALLOWED_ORIENTATION_PANEL_H

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#pragma once

#include "dialog_resolve_field_case_conflicts_base.h"
#include <fields_data_model.h>

class SCH_EDIT_FRAME;


class DIALOG_RESOLVE_FIELD_CASE_CONFLICTS : public DIALOG_RESOLVE_FIELD_CASE_CONFLICTS_BASE
{
public:
    DIALOG_RESOLVE_FIELD_CASE_CONFLICTS( wxWindow* aParent, SCH_EDIT_FRAME* aFrame,
                                         std::vector<FIELD_CASE_CONFLICT> aConflicts );

private:
    enum ACTION
    {
        ACTION_KEEP_FIRST,
        ACTION_KEEP_SECOND,
        ACTION_JOIN
    };

    void onActionCellChanged( wxGridEvent& event ) override;
    void onApplyAndContinue( wxCommandEvent& event ) override;

    void populateGrid();
    void applyResolutions();

    std::vector<int> findSiblingRows( int aRow ) const;

    SCH_EDIT_FRAME*                  m_frame;
    std::vector<FIELD_CASE_CONFLICT> m_conflicts;
    std::vector<ACTION>              m_rowAction;
};

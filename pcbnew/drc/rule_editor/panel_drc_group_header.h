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

#ifndef PANEL_DRC_GROUP_HEADER_H
#define PANEL_DRC_GROUP_HEADER_H

#include <wx/wx.h>
#include <wx/combo.h>
#include <wx/popupwin.h>

#include <lset.h>
#include <lseq.h>

#include "panel_drc_rule_editor_base.h"
#include "drc_rule_editor_enums.h"
#include "drc_rule_editor_utils.h"
#include "panel_drc_group_header_base.h"


struct DRC_RULE_ROW
{
    wxString m_ruleType;
    wxString m_ruleName;
    wxString m_comment;
};

class PANEL_DRC_GROUP_HEADER : public PANEL_DRC_GROUP_HEADER_BASE
{
public:
    PANEL_DRC_GROUP_HEADER( wxWindow* aParent, const std::vector<DRC_RULE_ROW>& aRows );

    ~PANEL_DRC_GROUP_HEADER() override;

    bool TransferDataToWindow() override;

    bool TransferDataFromWindow() override;

protected:
    void OnGridSize( wxGridSizeEvent& event ) override;
    void OnSize( wxSizeEvent& event ) override;

private:
    void                      populateGrid();
    std::vector<DRC_RULE_ROW> m_rows;
};

#endif // PANEL_DRC_GROUP_HEADER_H

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <vector>
#include <sch_edit_frame.h>
#include <dialog_migrate_buses_base.h>

class CONNECTION_SUBGRAPH;


struct BUS_MIGRATION_STATUS
{
    const CONNECTION_SUBGRAPH* subgraph;
    std::vector<wxString>      labels;
    std::vector<wxString>      possible_labels;
    wxString                   approved_label;
    bool                       approved;
};

class DIALOG_MIGRATE_BUSES : public DIALOG_MIGRATE_BUSES_BASE
{
public:
    DIALOG_MIGRATE_BUSES( SCH_EDIT_FRAME* aParent );
    ~DIALOG_MIGRATE_BUSES();

private:
    void loadGraphData();

    void updateUi();

    std::vector<wxString> getProposedLabels( const std::vector<wxString>& aLabelList );

    void onItemSelected( wxListEvent& aEvent );

    void onAcceptClicked( wxCommandEvent& aEvent );

private:
    SCH_EDIT_FRAME* m_frame;
    unsigned        m_selected_index;

    std::vector<BUS_MIGRATION_STATUS> m_items;
};

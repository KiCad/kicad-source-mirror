/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#ifndef __DIALOG_GENERATE_MULTICHANNEL_RULE_AREAS__
#define __DIALOG_GENERATE_MULTICHANNEL_RULE_AREAS__

#include <vector>
#include <widgets/unit_binder.h>
#include <wx/grid.h>

#include <dialogs/dialog_multichannel_generate_rule_areas_base.h>


class PCB_BASE_FRAME;
class MULTICHANNEL_TOOL;

class DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS : public DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS_BASE
{
public:
    DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS(
        PCB_BASE_FRAME* aFrame,
        MULTICHANNEL_TOOL* aParentTool );
    ~DIALOG_MULTICHANNEL_GENERATE_RULE_AREAS();

    bool TransferDataFromWindow() override;
    bool TransferDataToWindow() override;

    void OnNotebookPageChanged( wxNotebookEvent& event ) override;

private:
    MULTICHANNEL_TOOL* m_parentTool;

    wxBoxSizer* m_bSizer1;
    wxBoxSizer* m_bSizer2;
    wxGrid* m_sheetGrid;
    wxGrid* m_componentClassGrid;
    wxGrid* m_groupGrid;
};

#endif


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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef MATCH_PROPERTIES_TOOL_H
#define MATCH_PROPERTIES_TOOL_H

#include <set>

#include <tools/pcb_tool_base.h>

class PCB_SELECTION_TOOL;


class MATCH_PROPERTIES_TOOL : public PCB_TOOL_BASE
{
public:
    MATCH_PROPERTIES_TOOL();

    bool Init() override;
    int Match( const TOOL_EVENT& aEvent );

protected:
    void setTransitions() override;

private:
    /// Only the running tool opens this, and only for the source it is copying from.
    void showSettingsDialog( const EDA_ITEM& aSource );

    /// The picker session.  aSourceId is niluuid when the first click still has to name it.
    int runInteractive( const TOOL_EVENT& aEvent, const KIID& aSourceId );

    const std::set<wxString>& enabledKeys();

    bool applyToTargets( const EDA_ITEM& aSource, const std::vector<EDA_ITEM*>& aTargets );

    PCB_SELECTION_TOOL* m_selectionTool = nullptr;
};

#endif

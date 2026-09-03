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
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SCH_MDW_PART_TOOL_H
#define SCH_MDW_PART_TOOL_H

#include <mdw/client/mdw_op.h>
#include <mdw/client/mdw_part_catalog.h>
#include <tools/sch_tool_base.h>

#include <optional>
#include <vector>

class KIID;
class SCH_EDIT_FRAME;
class LIB_ID;
class SCH_SYMBOL;
struct MDW_TEAM_LIBRARY;


/**
 * Links placed symbols to workspace parts. The MDW_Part field it writes holds identity only and is
 * written by this explicit action alone.
 */
class SCH_MDW_PART_TOOL : public SCH_TOOL_BASE<SCH_EDIT_FRAME>
{
public:
    SCH_MDW_PART_TOOL();

    bool Init() override;
    void Reset( RESET_REASON aReason ) override;

    /// Link the selected symbol to one of the workspace parts its library symbol belongs to
    int AssignPart( const TOOL_EVENT& aEvent );

    /// True when the source of aSymbol's library answers part requests
    bool CanAssignPart( const SCH_SYMBOL& aSymbol );

private:
    /// The library binding of aSymbol when its source answers part requests
    std::optional<MDW_TEAM_LIBRARY> partBinding( const SCH_SYMBOL& aSymbol );

    void choosePart( const KIID& aSymbol, const LIB_ID& aLibId, const std::vector<MDW_PART_INFO>& aCandidates );

    void setTransitions() override;

    MDW_OP m_resolve;
};

#endif // SCH_MDW_PART_TOOL_H

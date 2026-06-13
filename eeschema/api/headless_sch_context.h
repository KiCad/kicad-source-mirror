/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#ifndef KICAD_HEADLESS_SCH_CONTEXT_H
#define KICAD_HEADLESS_SCH_CONTEXT_H

#include <memory>

#include <wx/string.h>

#include <api/sch_context.h>

class KIWAY;
class PROJECT;
class SCHEMATIC;
class TOOL_MANAGER;


class HEADLESS_SCH_CONTEXT : public SCH_CONTEXT
{
public:
    HEADLESS_SCH_CONTEXT( SCHEMATIC* aSchematic, PROJECT* aProject, KIWAY* aKiway = nullptr );

    ~HEADLESS_SCH_CONTEXT() override;

    SCHEMATIC* GetSchematic() const override;

    PROJECT& Prj() const override;

    TOOL_MANAGER* GetToolManager() const override;

    KIWAY* GetKiway() const override { return m_kiway; }

    wxString GetCurrentFileName() const override;

    bool CanAcceptApiCommands() const override { return true; }

    std::optional<SCH_SHEET_PATH> GetCurrentSheet() const override { return std::nullopt; }

    bool SaveSchematic() override;

    bool SaveSchematicCopy( const wxString& aFileName, bool aCreateProject ) override;

private:
    // All owned by caller (the kiface)
    SCHEMATIC*                    m_schematic;
    PROJECT*                      m_project;
    KIWAY*                        m_kiway;
    std::unique_ptr<TOOL_MANAGER> m_toolManager;
};

#endif

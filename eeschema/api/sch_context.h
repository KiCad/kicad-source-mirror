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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef KICAD_SCH_CONTEXT_H
#define KICAD_SCH_CONTEXT_H

#include <memory>
#include <optional>

#include <sch_sheet_path.h>
#include <wx/string.h>

class KIWAY;
class PROJECT;
class SCHEMATIC;
class SCH_EDIT_FRAME;
class TOOL_MANAGER;


/// An interface for the frame surface that the SCH API handlers need; to enable headless mode
class SCH_CONTEXT
{
public:
    virtual ~SCH_CONTEXT() = default;

    virtual SCHEMATIC* GetSchematic() const = 0;

    virtual PROJECT& Prj() const = 0;

    virtual TOOL_MANAGER* GetToolManager() const = 0;

    virtual KIWAY* GetKiway() const = 0;

    virtual wxString GetCurrentFileName() const = 0;

    virtual bool CanAcceptApiCommands() const = 0;

    virtual std::optional<SCH_SHEET_PATH> GetCurrentSheet() const = 0;

    virtual bool SaveSchematic() = 0;

    virtual bool SaveSchematicCopy( const wxString& aFileName, bool aCreateProject ) = 0;
};


std::shared_ptr<SCH_CONTEXT> CreateSchFrameContext( SCH_EDIT_FRAME* aFrame );

#endif

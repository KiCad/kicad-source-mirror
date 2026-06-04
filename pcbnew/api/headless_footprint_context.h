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

#ifndef KICAD_HEADLESS_FOOTPRINT_CONTEXT_H
#define KICAD_HEADLESS_FOOTPRINT_CONTEXT_H

#include <memory>

#include <lib_id.h>

#include <api/footprint_context.h>

class APP_SETTINGS_BASE;
class BOARD;
class KIWAY;
class PROJECT;
class TOOL_MANAGER;


class HEADLESS_FOOTPRINT_CONTEXT : public FOOTPRINT_CONTEXT
{
public:
    HEADLESS_FOOTPRINT_CONTEXT( std::unique_ptr<FOOTPRINT> aFootprint, const LIB_ID& aFPID,
                                PROJECT* aProject, APP_SETTINGS_BASE* aSettings,
                                KIWAY* aKiway = nullptr );

    ~HEADLESS_FOOTPRINT_CONTEXT() override;

    BOARD* GetBoard() const override;

    PROJECT& Prj() const override;

    TOOL_MANAGER* GetToolManager() const override;

    KIWAY* GetKiway() const override { return m_kiway; }

    bool CanAcceptApiCommands() const override { return true; }

    LIB_ID GetLoadedFPID() const override;

    bool SaveFootprint( FOOTPRINT* aFootprint ) override;

    bool SaveFootprintInLibrary( FOOTPRINT* aFootprint, const wxString& aLibraryName ) override;

private:
    std::unique_ptr<BOARD> m_board;
    LIB_ID                 m_fpid;
    PROJECT*               m_project;
    KIWAY*                 m_kiway;
    std::unique_ptr<TOOL_MANAGER> m_toolManager;
};

#endif

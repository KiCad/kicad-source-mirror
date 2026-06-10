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

#ifndef KICAD_HEADLESS_PCB_CONTEXT_H
#define KICAD_HEADLESS_PCB_CONTEXT_H

#include <memory>

#include <wx/string.h>

#include <api/pcb_context.h>

class APP_SETTINGS_BASE;
class BOARD;
class KIWAY;
class PROJECT;
class TOOL_MANAGER;


class HEADLESS_PCB_CONTEXT : public PCB_CONTEXT
{
public:
    HEADLESS_PCB_CONTEXT( std::unique_ptr<BOARD> aBoard, PROJECT* aProject,
                            APP_SETTINGS_BASE* aSettings, KIWAY* aKiway = nullptr );

    ~HEADLESS_PCB_CONTEXT() override;

    BOARD* GetBoard() const override;

    PROJECT& Prj() const override;

    TOOL_MANAGER* GetToolManager() const override;

    KIWAY* GetKiway() const override { return m_kiway; }

    wxString GetCurrentFileName() const override;

    bool CanAcceptApiCommands() const override { return true; }

    bool SaveBoard() override;

    bool SavePcbCopy( const wxString& aFileName, bool aCreateProject, bool aHeadless ) override;

    bool ReadNetlistFromFile( const wxString& aFilename, NETLIST& aNetlist, REPORTER& aReporter ) override;

    std::unique_ptr<BOARD_NETLIST_UPDATER> MakeNetlistUpdater() override;

    void OnNetlistChanged( BOARD_NETLIST_UPDATER& aUpdater ) override;

private:
    std::unique_ptr<BOARD> m_board;
    PROJECT* m_project;
    KIWAY* m_kiway;
    std::unique_ptr<TOOL_MANAGER> m_toolManager;
};

#endif

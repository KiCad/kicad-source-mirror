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

#include <api/pcb_context.h>

#include <netlist_reader/board_netlist_updater.h>
#include <netlist_reader/pcb_netlist.h>
#include <pcb_edit_frame.h>
#include <reporter.h>


class PCB_EDIT_FRAME_CONTEXT : public PCB_CONTEXT
{
public:
    explicit PCB_EDIT_FRAME_CONTEXT( PCB_EDIT_FRAME* aFrame ) :
            m_frame( aFrame )
    {
    }

    BOARD* GetBoard() const override
    {
        return m_frame->GetBoard();
    }

    PROJECT& Prj() const override
    {
        return m_frame->Prj();
    }

    TOOL_MANAGER* GetToolManager() const override
    {
        return m_frame->GetToolManager();
    }

    KIWAY* GetKiway() const override
    {
        return &m_frame->Kiway();
    }

    wxString GetCurrentFileName() const override
    {
        return m_frame->GetCurrentFileName();
    }

    bool CanAcceptApiCommands() const override
    {
        return m_frame->CanAcceptApiCommands();
    }

    bool SaveBoard() override
    {
        return m_frame->SaveBoard();
    }

    bool SavePcbCopy( const wxString& aFileName, bool aCreateProject, bool aHeadless ) override
    {
        return m_frame->SavePcbCopy( aFileName, aCreateProject, aHeadless );
    }

    bool ReadNetlistFromFile( const wxString& aFilename, NETLIST& aNetlist, REPORTER& aReporter ) override
    {
        return m_frame->ReadNetlistFromFile( aFilename, aNetlist, aReporter );
    }

    std::unique_ptr<BOARD_NETLIST_UPDATER> MakeNetlistUpdater() override
    {
        return std::make_unique<BOARD_NETLIST_UPDATER>( m_frame, GetBoard() );
    }

    void OnNetlistChanged( BOARD_NETLIST_UPDATER& aUpdater ) override
    {
        bool runDragCommand = false;
        m_frame->OnNetlistChanged( aUpdater, &runDragCommand );
    }

private:
    PCB_EDIT_FRAME* m_frame;
};


std::shared_ptr<PCB_CONTEXT> CreatePcbFrameContext( PCB_EDIT_FRAME* aFrame )
{
    return std::make_shared<PCB_EDIT_FRAME_CONTEXT>( aFrame );
}

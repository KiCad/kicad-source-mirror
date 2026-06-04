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

#include <api/headless_pcb_context.h>
#include <board.h>
#include <board_loader.h>
#include <project.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <wx/debug.h>


HEADLESS_PCB_CONTEXT::HEADLESS_PCB_CONTEXT( std::unique_ptr<BOARD> aBoard, PROJECT* aProject,
                        APP_SETTINGS_BASE* aSettings,
                        KIWAY* aKiway ) :
        m_board( std::move( aBoard ) ),
        m_project( aProject ),
        m_kiway( aKiway ),
        m_toolManager( std::make_unique<TOOL_MANAGER>() )
{
    wxCHECK( m_board, /* void */ );
    wxCHECK( m_project, /* void */ );

    m_board->SetProject( m_project );
    m_toolManager->SetEnvironment( m_board.get(), nullptr, nullptr, aSettings, nullptr );
}


HEADLESS_PCB_CONTEXT::~HEADLESS_PCB_CONTEXT()
{
    // Sever the board↔project linkage before destruction. The PROJECT holds a raw pointer
    // (m_BoardSettings) to the board's design settings. If the board is destroyed while the
    // project still exists, that pointer becomes dangling.
    if( m_board )
        m_board->ClearProject();
}


BOARD* HEADLESS_PCB_CONTEXT::GetBoard() const
{
    return m_board.get();
}


PROJECT& HEADLESS_PCB_CONTEXT::Prj() const
{
    // Shouldn't be able to construct this without a project
    wxASSERT( m_project );
    return *m_project;
}


TOOL_MANAGER* HEADLESS_PCB_CONTEXT::GetToolManager() const
{
    return m_toolManager.get();
}


wxString HEADLESS_PCB_CONTEXT::GetCurrentFileName() const
{
    if( !m_board )
        return wxEmptyString;

    return m_board->GetFileName();
}


bool HEADLESS_PCB_CONTEXT::SaveBoard()
{
    if( !m_board )
        return false;

    wxString fileName = m_board->GetFileName();

    if( fileName.IsEmpty() )
        return false;

    bool success = BOARD_LOADER::SaveBoard( fileName, m_board.get() );

    if( success )
    {
        wxFileName pro = fileName;
        pro.SetExt( FILEEXT::ProjectFileExtension );
        pro.MakeAbsolute();

        Pgm().GetSettingsManager().SaveProjectAs( pro.GetFullPath(), m_board->GetProject() );
    }

    return success;
}


bool HEADLESS_PCB_CONTEXT::SavePcbCopy( const wxString& aFileName, bool aCreateProject, bool aHeadless )
{
    if( !m_board || aFileName.IsEmpty() )
        return false;

    wxString outPath = aFileName;

    bool success = BOARD_LOADER::SaveBoard( outPath, m_board.get() );

    if( success && aCreateProject )
    {
        wxFileName pro = aFileName;
        pro.SetExt( FILEEXT::ProjectFileExtension );
        pro.MakeAbsolute();

        Pgm().GetSettingsManager().SaveProjectAs( pro.GetFullPath(), m_board->GetProject() );
    }

    return success;
}

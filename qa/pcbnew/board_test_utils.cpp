/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2021 KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "board_test_utils.h"

#include <wx/filename.h>
#include <board.h>
#include <board_design_settings.h>
#include <settings/settings_manager.h>
#include <pcbnew_utils/board_file_utils.h>
#include <tool/tool_manager.h>
#include <zone_filler.h>

// For the temp directory logic: can be std::filesystem in C++17
#include <boost/filesystem.hpp>
#include <boost/test/unit_test.hpp>
#include <board_commit.h>

namespace KI_TEST
{

BOARD_DUMPER::BOARD_DUMPER() :
        m_dump_boards( std::getenv( "KICAD_TEST_DUMP_BOARD_FILES" ) )
{
}


void BOARD_DUMPER::DumpBoardToFile( BOARD& aBoard, const std::string& aName ) const
{
    if( !m_dump_boards )
        return;

    auto path = boost::filesystem::temp_directory_path() / aName;
    path += ".kicad_pcb";

    BOOST_TEST_MESSAGE( "Dumping board file: " << path.string() );
    ::KI_TEST::DumpBoardToFile( aBoard, path.string() );
}


void LoadBoard( SETTINGS_MANAGER& aSettingsManager, const wxString& aRelPath,
                std::unique_ptr<BOARD>& aBoard )
{
    if( aBoard )
    {
        aBoard->SetProject( nullptr );
        aBoard = nullptr;
    }

    std::string absPath = GetPcbnewTestDataDir() + aRelPath.ToStdString();
    wxFileName  projectFile( absPath + ".kicad_pro" );
    wxFileName  legacyProject( absPath + ".pro" );
    std::string boardPath = absPath + ".kicad_pcb";
    wxFileName  rulesFile( absPath + ".kicad_dru" );

    if( projectFile.Exists() )
        aSettingsManager.LoadProject( projectFile.GetFullPath() );
    else if( legacyProject.Exists() )
        aSettingsManager.LoadProject( legacyProject.GetFullPath() );

    aBoard = ReadBoardFromFileOrStream( boardPath );

    if( projectFile.Exists() || legacyProject.Exists() )
        aBoard->SetProject( &aSettingsManager.Prj() );

    auto m_DRCEngine = std::make_shared<DRC_ENGINE>( aBoard.get(), &aBoard->GetDesignSettings() );

    if( rulesFile.Exists() )
        m_DRCEngine->InitEngine( rulesFile );
    else
        m_DRCEngine->InitEngine( wxFileName() );

    aBoard->GetDesignSettings().m_DRCEngine = m_DRCEngine;
    aBoard->BuildListOfNets();
    aBoard->BuildConnectivity();
}


void FillZones( BOARD* m_board, int aFillVersion )
{
    TOOL_MANAGER toolMgr;
    toolMgr.SetEnvironment( m_board, nullptr, nullptr, nullptr, nullptr );

    BOARD_COMMIT       commit( &toolMgr );
    ZONE_FILLER        filler( m_board, &commit );
    std::vector<ZONE*> toFill;

    m_board->GetDesignSettings().m_ZoneFillVersion = aFillVersion;

    for( ZONE* zone : m_board->Zones() )
        toFill.push_back( zone );

    if( filler.Fill( toFill, false, nullptr ) )
        commit.Push( _( "Fill Zone(s)" ), false, false );
}


} // namespace KI_TEST

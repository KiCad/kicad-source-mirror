/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <local_history.h>
#include <project.h>
#include <settings/settings_manager.h>

#include <vector>

#include <wx/filename.h>
#include <wx/stdpaths.h>


BOOST_AUTO_TEST_SUITE( PcbHistoryAutosave )


/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23737
 *
 * After importing a non-KiCad board, BOARD::SaveToHistory could be invoked from the autosave
 * timer while the board's project pointer was transiently null (between unloading the previous
 * project and linking the new one). The function then dereferenced GetProject() and crashed
 * with EXCEPTION_ACCESS_VIOLATION_READ. SaveToHistory must tolerate a null project and bail
 * out cleanly without crashing.
 */
BOOST_AUTO_TEST_CASE( SaveToHistoryWithNullProjectDoesNotCrash )
{
    BOARD                          board;
    std::vector<HISTORY_FILE_DATA> fileData;

    BOOST_REQUIRE( board.GetProject() == nullptr );

    BOOST_CHECK_NO_THROW( board.SaveToHistory( wxS( "/tmp/anywhere" ), fileData ) );
    BOOST_CHECK( fileData.empty() );
}


/**
 * SaveToHistory with a real project but no board filename should not produce any file data
 * (the board is unsaved). It must also not crash.
 */
BOOST_AUTO_TEST_CASE( SaveToHistoryUnsavedBoardProducesNothing )
{
    SETTINGS_MANAGER mgr;

    wxString tempDir = wxStandardPaths::Get().GetTempDir();
    wxString projectPath = tempDir + wxFileName::GetPathSeparator() + wxS( "pcb_autosave.kicad_pro" );

    mgr.LoadProject( projectPath.ToStdString() );

    BOARD board;
    board.SetProject( &mgr.Prj() );

    std::vector<HISTORY_FILE_DATA> fileData;

    BOOST_REQUIRE( board.GetFileName().IsEmpty() );
    BOOST_CHECK_NO_THROW( board.SaveToHistory( mgr.Prj().GetProjectPath(), fileData ) );
    BOOST_CHECK( fileData.empty() );

    // Detach project before BOARD destruction so design settings ownership unwinds cleanly
    board.ClearProject();
    mgr.UnloadProject( &mgr.Prj(), false );
}


BOOST_AUTO_TEST_SUITE_END()

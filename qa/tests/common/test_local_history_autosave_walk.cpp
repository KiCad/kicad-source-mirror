/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

// Regression test for a hang when a board is opened from a directory that also
// contains a symlink escaping the project tree (e.g. a Wine prefix's
// dosdevices/z: -> /).  LOCAL_HISTORY::CollectAutosaveFilePairs walks the
// project directory looking for stale autosave files and must terminate on
// symlink cycles and refuse to follow links out of the scan root.

#include <filesystem>
#include <fstream>
#include <system_error>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>

#include <local_history.h>
#include <settings/common_settings.h>

#include <wx/filename.h>
#include <wx/utils.h>


namespace
{

// RAII temp tree.  Cleanup on destruction matters here because the trees
// contain symlinks back into themselves.
struct ScratchRoot
{
    std::filesystem::path path;

    explicit ScratchRoot( const char* aTag )
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        path = fs::temp_directory_path( ec );
        BOOST_REQUIRE_MESSAGE( !ec, ec.message() );

        path /= std::string( "kicad_qa_autosave_walk_" ) + aTag + "_"
                + std::to_string( static_cast<unsigned long>( wxGetProcessId() ) );

        fs::remove_all( path, ec );
        fs::create_directories( path, ec );
        BOOST_REQUIRE_MESSAGE( !ec, ec.message() );
    }

    ~ScratchRoot()
    {
        std::error_code ec;
        std::filesystem::remove_all( path, ec );
    }
};


bool createSymlinkOrSkip( const std::filesystem::path& aTarget, const std::filesystem::path& aLink )
{
    std::error_code ec;
    std::filesystem::create_directory_symlink( aTarget, aLink, ec );

    if( ec )
        BOOST_TEST_MESSAGE( "Skipping: symlink creation unsupported (" << ec.message() << ")" );

    return !ec;
}


bool anyEndsWith( const std::vector<std::pair<wxString, wxString>>& aPairs, const wxString& aSuffix )
{
    for( const auto& [autosave, source] : aPairs )
    {
        if( autosave.EndsWith( aSuffix ) )
            return true;
    }

    return false;
}

}  // namespace


BOOST_AUTO_TEST_SUITE( LocalHistoryAutosaveWalk )


// A self-referencing directory symlink under the scan root must not send the
// autosave walk into unbounded recursion; the real autosave file alongside it
// must still be found.
BOOST_AUTO_TEST_CASE( SelfReferencingSymlinkTerminates )
{
    namespace fs = std::filesystem;

    const ScratchRoot scratch( "cycle" );
    const wxString    root = wxString::FromUTF8( scratch.path.string() );

    std::ofstream( scratch.path / "_autosave-board.kicad_pcb" ) << "x";

    if( !createSymlinkOrSkip( ".", scratch.path / "loop" ) )
        return;

    std::vector<std::pair<wxString, wxString>> pairs =
            LOCAL_HISTORY::CollectAutosaveFilePairs( root, root, BACKUP_LOCATION::PROJECT_DIR );

    BOOST_CHECK( anyEndsWith( pairs, wxS( "_autosave-board.kicad_pcb" ) ) );
}


// A symlink whose target escapes the scan root -- the shape of a Wine prefix's
// dosdevices/z: -> / -- must not be followed, otherwise the walk enumerates the
// whole filesystem and never returns.  Files living beyond the link must not
// appear in the results.
BOOST_AUTO_TEST_CASE( RootEscapeSymlinkIgnored )
{
    namespace fs = std::filesystem;

    const ScratchRoot scratch( "escape" );
    const ScratchRoot outside( "escape_outside" );

    std::ofstream( scratch.path / "_autosave-inside.kicad_pcb" ) << "x";
    std::ofstream( outside.path / "_autosave-outside.kicad_pcb" ) << "x";

    const fs::path dosdevices = scratch.path / "wineprefix" / "dosdevices";
    std::error_code ec;
    fs::create_directories( dosdevices, ec );
    BOOST_REQUIRE_MESSAGE( !ec, ec.message() );

    if( !createSymlinkOrSkip( outside.path, dosdevices / "z:" ) )
        return;

    const wxString root = wxString::FromUTF8( scratch.path.string() );

    std::vector<std::pair<wxString, wxString>> pairs =
            LOCAL_HISTORY::CollectAutosaveFilePairs( root, root, BACKUP_LOCATION::PROJECT_DIR );

    BOOST_CHECK( anyEndsWith( pairs, wxS( "_autosave-inside.kicad_pcb" ) ) );
    BOOST_CHECK( !anyEndsWith( pairs, wxS( "_autosave-outside.kicad_pcb" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()

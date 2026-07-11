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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// Regression test for https://gitlab.com/kicad/code/kicad/-/issues/24467.
// CollectFilesLoopSafe / CollectSubdirsLoopSafe must terminate on recursive
// symlinks, refuse to escape the scan root, and still resolve legitimate
// one-level symlinks.

#include <filesystem>
#include <fstream>
#include <system_error>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>

#include <gestfich.h>

#include <wx/arrstr.h>
#include <wx/filename.h>
#include <wx/utils.h>


namespace
{

// RAII so a mid-test BOOST_REQUIRE failure (or a thrown exception from
// std::filesystem) can't leak a temp tree -- particularly important here
// because the trees contain symlinks back into themselves.
struct ScratchRoot
{
    std::filesystem::path path;

    explicit ScratchRoot( const char* aTag )
    {
        namespace fs = std::filesystem;
        std::error_code ec;
        path = fs::temp_directory_path( ec );
        BOOST_REQUIRE_MESSAGE( !ec, ec.message() );

        path /= std::string( "kicad_qa_issue24467_" ) + aTag + "_"
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


bool contains( const wxArrayString& aFiles, const std::filesystem::path& aPath )
{
    const wxString want = wxString::FromUTF8( aPath.string() );

    for( const wxString& f : aFiles )
    {
        if( f == want )
            return true;
    }

    return false;
}


// Create a directory symlink aTarget <- aLink.  Returns true on success; on a
// platform that can't make symlinks it emits a skip message so the caller can
// bail out of the test cleanly.
bool createSymlinkOrSkip( const std::filesystem::path& aTarget,
                          const std::filesystem::path& aLink )
{
    std::error_code ec;
    std::filesystem::create_directory_symlink( aTarget, aLink, ec );

    if( ec )
        BOOST_TEST_MESSAGE( "Skipping: symlink creation unsupported (" << ec.message() << ")" );

    return !ec;
}

}  // namespace


BOOST_AUTO_TEST_SUITE( GestfichLoopSafeCollect )


// A self-referencing symlink must not send CollectFilesLoopSafe into
// unbounded recursion; the legitimate file alongside it must still be found.
BOOST_AUTO_TEST_CASE( RecursiveSymlinkTerminates )
{
    namespace fs = std::filesystem;

    const ScratchRoot scratch( "loop" );
    const fs::path stepFile = scratch.path / "R_0603.step";

    std::ofstream( stepFile ) << "ISO-10303-21;\n";

    if( !createSymlinkOrSkip( ".", scratch.path / "loop" ) )
        return;

    wxArrayString files;
    CollectFilesLoopSafe( wxString::FromUTF8( scratch.path.string() ), files );

    BOOST_CHECK( contains( files, stepFile ) );
}


// A one-level symlink that points at a sibling directory containing files
// must be followed so that legitimate user library aliases keep working.
BOOST_AUTO_TEST_CASE( SingleSymlinkResolves )
{
    namespace fs = std::filesystem;

    const ScratchRoot scratch( "follow" );
    const fs::path realDir = scratch.path / "real";
    const fs::path linkedDir = scratch.path / "scan";
    const fs::path stepFile = realDir / "R_0603.step";

    std::error_code ec;
    fs::create_directories( realDir, ec );
    BOOST_REQUIRE_MESSAGE( !ec, ec.message() );

    std::ofstream( stepFile ) << "ISO-10303-21;\n";

    if( !createSymlinkOrSkip( realDir, linkedDir ) )
        return;

    wxArrayString files;
    CollectFilesLoopSafe( wxString::FromUTF8( linkedDir.string() ), files );

    BOOST_CHECK_EQUAL( files.GetCount(), 1u );
    BOOST_CHECK( contains( files, linkedDir / "R_0603.step" ) );
}


// A symlink whose target is an ancestor of the scan root (the shape of
// Wine's `dosdevices/z: -> /`) must not escape the scan tree.  Otherwise
// scanning a Wine prefix would walk all of /.
BOOST_AUTO_TEST_CASE( AncestorSymlinkDoesNotEscape )
{
    namespace fs = std::filesystem;

    const ScratchRoot scratch( "ancestor" );
    const fs::path root = scratch.path / "scan";
    const fs::path outside = scratch.path / "outside";
    const fs::path outsideFile = outside / "Outside.step";

    std::error_code ec;
    fs::create_directories( root, ec );
    BOOST_REQUIRE_MESSAGE( !ec, ec.message() );
    fs::create_directories( outside, ec );
    BOOST_REQUIRE_MESSAGE( !ec, ec.message() );

    std::ofstream( outsideFile ) << "ISO-10303-21;\n";

    if( !createSymlinkOrSkip( scratch.path, root / "up" ) )
        return;

    wxArrayString files;
    CollectFilesLoopSafe( wxString::FromUTF8( root.string() ), files );

    BOOST_CHECK( !contains( files, root / "up" / "outside" / "Outside.step" ) );
    BOOST_CHECK( !contains( files, outsideFile ) );
}


// A wildcard filter must restrict the collected files just as
// wxDir::GetAllFiles() does, while still applying loop protection.
BOOST_AUTO_TEST_CASE( FileSpecFiltersResults )
{
    namespace fs = std::filesystem;

    const ScratchRoot scratch( "spec" );

    std::ofstream( scratch.path / "R_0603.step" ) << "ISO-10303-21;\n";
    std::ofstream( scratch.path / "R_0603.wrl" ) << "#VRML\n";

    wxArrayString files;
    CollectFilesLoopSafe( wxString::FromUTF8( scratch.path.string() ), files, wxS( "*.step" ) );

    BOOST_CHECK_EQUAL( files.GetCount(), 1u );
    BOOST_CHECK( contains( files, scratch.path / "R_0603.step" ) );
}


// CollectSubdirsLoopSafe must enumerate the real subdirectories and terminate
// on a self-referencing symlink rather than recursing without bound.
BOOST_AUTO_TEST_CASE( SubdirCollectionTerminates )
{
    namespace fs = std::filesystem;

    const ScratchRoot scratch( "subdirs" );
    const fs::path subDir = scratch.path / "Resistor_SMD.3dshapes";

    std::error_code ec;
    fs::create_directories( subDir, ec );
    BOOST_REQUIRE_MESSAGE( !ec, ec.message() );

    if( !createSymlinkOrSkip( ".", scratch.path / "loop" ) )
        return;

    wxArrayString dirs;
    CollectSubdirsLoopSafe( wxString::FromUTF8( scratch.path.string() ), dirs );

    BOOST_CHECK( contains( dirs, subDir ) );
}


// The hidden-entry behaviour must track wxDir::GetAllFiles(): the default flag
// set includes dotfiles, while an explicit wxDIR_FILES | wxDIR_DIRS excludes
// them.  Footprint-library delete paths rely on the former; the 3D scan and the
// sprint enumerator rely on the latter.
BOOST_AUTO_TEST_CASE( HiddenFlagControlsDotfiles )
{
    namespace fs = std::filesystem;

    const ScratchRoot scratch( "hidden" );
    const fs::path    normalFile = scratch.path / "R_0603.step";
    const fs::path    hiddenFile = scratch.path / ".hidden.step";

    std::ofstream( normalFile ) << "ISO-10303-21;\n";
    std::ofstream( hiddenFile ) << "ISO-10303-21;\n";

#ifdef _WIN32
    SetFileAttributesW( hiddenFile.c_str(), FILE_ATTRIBUTE_HIDDEN );
#endif

    wxArrayString withHidden;
    CollectFilesLoopSafe( wxString::FromUTF8( scratch.path.string() ), withHidden );

    BOOST_CHECK( contains( withHidden, normalFile ) );
    BOOST_CHECK( contains( withHidden, hiddenFile ) );

    wxArrayString noHidden;
    CollectFilesLoopSafe( wxString::FromUTF8( scratch.path.string() ), noHidden, wxEmptyString,
                          wxDIR_FILES | wxDIR_DIRS );

    BOOST_CHECK( contains( noHidden, normalFile ) );
    BOOST_CHECK( !contains( noHidden, hiddenFile ) );
}


// A one-level child symlink pointing at an external (sibling) directory is
// intentionally followed -- this is how users alias shared library trees.  The
// loop guard only blocks re-entry and root escape (`-> /` or an ancestor of the
// scan root), not bounded targets that simply live elsewhere.
BOOST_AUTO_TEST_CASE( ChildSymlinkToExternalDirIsFollowed )
{
    namespace fs = std::filesystem;

    const ScratchRoot scratch( "external" );
    const fs::path root = scratch.path / "scan";
    const fs::path external = scratch.path / "external_lib";
    const fs::path externalFile = external / "C_0402.step";

    std::error_code ec;
    fs::create_directories( root, ec );
    BOOST_REQUIRE_MESSAGE( !ec, ec.message() );
    fs::create_directories( external, ec );
    BOOST_REQUIRE_MESSAGE( !ec, ec.message() );

    std::ofstream( externalFile ) << "ISO-10303-21;\n";

    if( !createSymlinkOrSkip( external, root / "alias" ) )
        return;

    wxArrayString files;
    CollectFilesLoopSafe( wxString::FromUTF8( root.string() ), files );

    BOOST_CHECK( contains( files, root / "alias" / "C_0402.step" ) );
}


BOOST_AUTO_TEST_SUITE_END()

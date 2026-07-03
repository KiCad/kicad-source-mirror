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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <filesystem>
#include <memory>

#ifdef __unix__
#include <unistd.h>
#endif

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <footprint.h>
#include <footprint_library_adapter.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>


namespace
{

/**
 * Exposes a way to seed a LOAD_ERROR sentinel entry into the project-scope map,
 * exactly as the async loader leaves it when a library fails to load (status set
 * to LOAD_ERROR, plugin and row left null).
 */
class TEST_FOOTPRINT_LIBRARY_ADAPTER : public FOOTPRINT_LIBRARY_ADAPTER
{
public:
    using FOOTPRINT_LIBRARY_ADAPTER::FOOTPRINT_LIBRARY_ADAPTER;

    void SeedLoadError( const wxString& aNickname )
    {
        m_libraries[aNickname].status.load_status = LOAD_STATUS::LOAD_ERROR;
    }

    /**
     * Seed a loaded library backed by a real plugin so SaveFootprint() reaches the plugin
     * write instead of bailing out as "not loaded".  The row is owned here to outlive the
     * adapter's use of it.
     */
    void SeedLoadedLibrary( const wxString& aNickname, const wxString& aUri )
    {
        m_row = std::make_unique<LIBRARY_TABLE_ROW>();
        m_row->SetNickname( aNickname );
        m_row->SetType( wxS( "KiCad" ) );
        m_row->SetURI( aUri );

        LIB_DATA& data = m_libraries[aNickname];
        data.status.load_status = LOAD_STATUS::LOADED;
        data.plugin = std::make_unique<PCB_IO_KICAD_SEXPR>();
        data.row = m_row.get();
    }

private:
    std::unique_ptr<LIBRARY_TABLE_ROW> m_row;
};

} // namespace


BOOST_AUTO_TEST_SUITE( FootprintLibraryAdapter )


/**
 * Regression test for a null-plugin dereference crash.
 *
 * A footprint library that fails to load leaves a LOAD_ERROR sentinel with a null
 * plugin in the adapter's map. IsFootprintLibWritable() is called for every table
 * row by the footprint editor and chooser, so it must report such a library as not
 * writable instead of dereferencing the null plugin.
 */
BOOST_AUTO_TEST_CASE( IsFootprintLibWritableHandlesFailedLoad )
{
    LIBRARY_MANAGER                manager;
    TEST_FOOTPRINT_LIBRARY_ADAPTER adapter( manager );

    adapter.SeedLoadError( wxS( "BadLib" ) );

    BOOST_CHECK_EQUAL( adapter.IsFootprintLibWritable( wxS( "BadLib" ) ), false );

    // A library that was never even attempted must also be safe.
    BOOST_CHECK_EQUAL( adapter.IsFootprintLibWritable( wxS( "NeverSeen" ) ), false );
}


/**
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23850
 *
 * A read-only target .kicad_mod inside a writable directory made the adapter swallow the
 * plugin's IO_ERROR and return SAVE_SKIPPED, so the editor reported success and lost the
 * user's edits.  SaveFootprint() must now let the IO_ERROR propagate.
 */
BOOST_AUTO_TEST_CASE( SaveFootprintReadOnlyFilePropagatesError )
{
#ifdef __unix__
    // The superuser ignores mode bits, so a read-only file stays writable and this path
    // cannot be exercised.
    if( ::geteuid() == 0 )
    {
        BOOST_TEST_MESSAGE( "Skipping read-only footprint save test when running as root." );
        return;
    }
#endif

    // FootprintSave validates the whole containing directory as a library, so it needs a
    // private directory no unrelated .kicad_mod can pollute.
    KI_TEST::TEMPORARY_DIRECTORY tmpLib( "kicad_qa_adapter_save_readonly", ".pretty" );

    LIBRARY_MANAGER                manager;
    TEST_FOOTPRINT_LIBRARY_ADAPTER adapter( manager );

    const wxString nickname = wxS( "scratch" );
    adapter.SeedLoadedLibrary( nickname, tmpLib.GetPath().string() );

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    FOOTPRINT* fp = new FOOTPRINT( board.get() );
    board->Add( fp );
    fp->SetFPID( LIB_ID( nickname, wxS( "readonly_fp" ) ) );

    BOOST_REQUIRE( adapter.SaveFootprint( nickname, fp ) == FOOTPRINT_LIBRARY_ADAPTER::SAVE_OK );

    auto savedFile = tmpLib.GetPath() / "readonly_fp.kicad_mod";
    BOOST_REQUIRE( std::filesystem::exists( savedFile ) );

    // Mark only the file read-only, mirroring the issue; the directory stays writable so the
    // writability gate still passes and TEMPORARY_DIRECTORY can unlink it.
    std::filesystem::permissions( savedFile,
                                  std::filesystem::perms::owner_write | std::filesystem::perms::group_write
                                          | std::filesystem::perms::others_write,
                                  std::filesystem::perm_options::remove );

    BOOST_CHECK_THROW( adapter.SaveFootprint( nickname, fp ), IO_ERROR );
}


BOOST_AUTO_TEST_SUITE_END()

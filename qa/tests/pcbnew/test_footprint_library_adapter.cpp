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

#include <atomic>
#include <filesystem>
#include <memory>
#include <thread>
#include <vector>

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


/// Absolute path to the real fixture library qa/data/libraries/Resistor_SMD.pretty.
wxString getResistorLibPath()
{
    // qa/data/pcbnew/.. -> qa/data/libraries/Resistor_SMD.pretty
    wxFileName fn( wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir() ), wxEmptyString );
    fn.RemoveLastDir();
    fn.AppendDir( wxS( "libraries" ) );
    fn.AppendDir( wxS( "Resistor_SMD.pretty" ) );
    return fn.GetPath();
}

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


// Concurrency regression for the FP_CACHE heap corruption (Sentry 6819786130 / 6322230945 /
// 7271165487): a writer churns the library while readers rebuild the cache, which races unserialized.
BOOST_AUTO_TEST_CASE( ConcurrentPluginAccessIsSerialized )
{
    // Writable copy so the writer can churn the library and force concurrent cache rebuilds.
    KI_TEST::TEMPORARY_DIRECTORY tmpLib( "kicad_qa_adapter_concurrent", ".pretty" );

    for( const auto& entry : std::filesystem::directory_iterator(
                 std::filesystem::path( getResistorLibPath().ToStdString() ) ) )
    {
        if( entry.is_regular_file() )
            std::filesystem::copy_file( entry.path(), tmpLib.GetPath() / entry.path().filename() );
    }

    LIBRARY_MANAGER                manager;
    TEST_FOOTPRINT_LIBRARY_ADAPTER adapter( manager );

    const wxString nickname = wxS( "Resistor_SMD" );
    adapter.SeedLoadedLibrary( nickname, tmpLib.GetPath().string() );

    // Readers probe this; the writer only writes scratch names, so it always resolves.
    const wxString stableFp = wxS( "R_0603_1608Metric" );

    BOOST_REQUIRE( adapter.FootprintExists( nickname, stableFp ) );

    constexpr int readerCount = 6;
    constexpr int iterations  = 40;

    std::atomic<bool> sawMissing{ false };
    std::atomic<bool> sawNullLoad{ false };
    std::atomic<int>  savedCount{ 0 };

    // Release all threads together so readers and writer overlap deterministically.
    std::atomic<int>  ready{ 0 };
    std::atomic<bool> go{ false };

    auto waitForStart = [&]()
    {
        ready.fetch_add( 1 );

        while( !go.load() )
            std::this_thread::yield();
    };

    std::vector<std::thread> workers;
    workers.reserve( readerCount + 1 );

    for( int t = 0; t < readerCount; ++t )
    {
        workers.emplace_back(
                [&]()
                {
                    waitForStart();

                    for( int i = 0; i < iterations; ++i )
                    {
                        // Base-class path whose guard this change adds.
                        adapter.IsWritable( nickname );

                        if( !adapter.FootprintExists( nickname, stableFp ) )
                            sawMissing = true;

                        std::unique_ptr<FOOTPRINT> fp{ adapter.LoadFootprint( nickname, stableFp, false ) };

                        if( !fp )
                            sawNullLoad = true;
                    }
                } );
    }

    workers.emplace_back(
            [&]()
            {
                std::unique_ptr<FOOTPRINT> seed{ adapter.LoadFootprint( nickname, stableFp, false ) };

                if( !seed )
                {
                    sawNullLoad = true;
                    return;
                }

                waitForStart();

                for( int i = 0; i < iterations; ++i )
                {
                    // Each save bumps the library timestamp, forcing the next validateCache() to rebuild.
                    seed->SetFPID( LIB_ID( nickname, wxString::Format( wxS( "scratch_%d" ), i % 4 ) ) );

                    try
                    {
                        if( adapter.SaveFootprint( nickname, seed.get(), true ) == FOOTPRINT_LIBRARY_ADAPTER::SAVE_OK )
                            savedCount.fetch_add( 1 );
                    }
                    catch( const IO_ERROR& )
                    {
                        // Transient write failures are fine; a total failure trips savedCount below.
                    }
                }
            } );

    while( ready.load() < readerCount + 1 )
        std::this_thread::yield();

    go.store( true );

    for( std::thread& worker : workers )
        worker.join();

    BOOST_CHECK( !sawMissing.load() );
    BOOST_CHECK( !sawNullLoad.load() );

    // Confirm the writer churned the cache, else the readers never raced a rebuild.
    BOOST_CHECK( savedCount.load() > 0 );
}


BOOST_AUTO_TEST_SUITE_END()

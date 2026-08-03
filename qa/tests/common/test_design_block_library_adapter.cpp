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

#include <chrono>
#include <condition_variable>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <utility>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <design_block.h>
#include <design_block_io.h>
#include <design_block_library_adapter.h>
#include <frame_type.h>
#include <ki_exception.h>
#include <kiway.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>
#include <mail_type.h>
#include <pgm_base.h>
#include <project.h>
#include <settings/settings_manager.h>


namespace
{

class TEST_DESIGN_BLOCK_LIBRARY_ADAPTER : public DESIGN_BLOCK_LIBRARY_ADAPTER
{
public:
    using DESIGN_BLOCK_LIBRARY_ADAPTER::DESIGN_BLOCK_LIBRARY_ADAPTER;

    void SeedLoadedLibrary( const wxString& aNickname, const wxString& aUri )
    {
        LIBRARY_TABLE_ROW* row = m_rows.emplace_back( std::make_unique<LIBRARY_TABLE_ROW>() ).get();
        row->SetNickname( aNickname );
        row->SetType( wxS( "KiCad" ) );
        row->SetURI( aUri );

        LIB_DATA& data = m_libraries[aNickname];
        data.status.load_status = LOAD_STATUS::LOADED;
        data.plugin = std::make_unique<DESIGN_BLOCK_IO>();
        data.row = row;
    }

private:
    std::vector<std::unique_ptr<LIBRARY_TABLE_ROW>> m_rows;
};


std::filesystem::path makeBlockLibrary( const std::string& aDirName, const std::string& aBlockName,
                                        const std::string& aJson )
{
    std::error_code       ec;
    std::filesystem::path lib = std::filesystem::temp_directory_path( ec ) / aDirName;

    std::filesystem::remove_all( lib, ec );

    std::filesystem::path block = lib / ( aBlockName + ".kicad_block" );
    std::filesystem::create_directories( block, ec );

    std::ofstream json( block / ( aBlockName + ".json" ) );
    json << aJson;

    return lib;
}


struct SCOPED_LIBRARY
{
    explicit SCOPED_LIBRARY( const std::filesystem::path& aPath ) :
            path( aPath )
    {
    }

    ~SCOPED_LIBRARY()
    {
        std::error_code ec;
        std::filesystem::remove_all( path, ec );
    }

    std::filesystem::path path;
};


/// Swallows the reload mails a preload sends when it ends, since no frame is open
class PRELOAD_KIWAY : public KIWAY
{
public:
    PRELOAD_KIWAY() :
            KIWAY( KFCTL_STANDALONE )
    {
    }

    void ExpressMail( FRAME_T, MAIL_T, std::string&, wxWindow*, bool ) override {}
};


/// Holds the first library enumeration after Hold() until Release()
class GATED_DESIGN_BLOCK_LIBRARY_ADAPTER : public DESIGN_BLOCK_LIBRARY_ADAPTER
{
public:
    using DESIGN_BLOCK_LIBRARY_ADAPTER::DESIGN_BLOCK_LIBRARY_ADAPTER;

    void Hold()
    {
        std::lock_guard lock( m_mutex );
        m_armed = true;
    }

    bool WaitUntilHeld()
    {
        std::unique_lock lock( m_mutex );
        return m_cv.wait_for( lock, std::chrono::seconds( 20 ), [&]() { return m_holding; } );
    }

    void Release()
    {
        std::lock_guard lock( m_mutex );
        m_armed = false;
        m_holding = false;
        m_cv.notify_all();
    }

protected:
    void enumerateLibrary( LIB_DATA* aLib, const wxString& aUri ) override
    {
        {
            std::unique_lock lock( m_mutex );

            if( std::exchange( m_armed, false ) )
            {
                m_holding = true;
                m_cv.notify_all();
                m_cv.wait( lock, [&]() { return !m_holding; } );
            }
        }

        DESIGN_BLOCK_LIBRARY_ADAPTER::enumerateLibrary( aLib, aUri );
    }

private:
    std::mutex              m_mutex;
    std::condition_variable m_cv;
    bool                    m_armed = false;
    bool                    m_holding = false;
};


void addGlobalRow( LIBRARY_MANAGER& aManager, const wxString& aNickname, const std::filesystem::path& aPath )
{
    std::optional<LIBRARY_TABLE*> table =
            aManager.Table( LIBRARY_TABLE_TYPE::DESIGN_BLOCK, LIBRARY_TABLE_SCOPE::GLOBAL );

    BOOST_REQUIRE( table );

    LIBRARY_TABLE_ROW& row = ( *table )->InsertRow();
    row.SetNickname( aNickname );
    row.SetType( wxS( "KiCad" ) );
    row.SetURI( aPath.string() );
    row.SetOk();
}


/// Puts a gated design block adapter on the program's manager and joins its preload on every exit
struct GATED_PRELOAD
{
    GATED_PRELOAD() :
            manager( Pgm().GetLibraryManager() )
    {
        if( std::optional<LIBRARY_MANAGER_ADAPTER*> existing = manager.Adapter( LIBRARY_TABLE_TYPE::DESIGN_BLOCK ) )
            manager.RemoveAdapter( LIBRARY_TABLE_TYPE::DESIGN_BLOCK, *existing );

        manager.RegisterAdapter( LIBRARY_TABLE_TYPE::DESIGN_BLOCK,
                                 std::make_unique<GATED_DESIGN_BLOCK_LIBRARY_ADAPTER>( manager ) );

        adapter = static_cast<GATED_DESIGN_BLOCK_LIBRARY_ADAPTER*>( kiway.Prj().DesignBlockLibs() );
        manager.LoadGlobalTables( { LIBRARY_TABLE_TYPE::DESIGN_BLOCK } );
    }

    ~GATED_PRELOAD()
    {
        adapter->Release();
        Pgm().WaitForDesignBlockPreload();
        manager.LoadGlobalTables( { LIBRARY_TABLE_TYPE::DESIGN_BLOCK } );
        manager.RemoveAdapter( LIBRARY_TABLE_TYPE::DESIGN_BLOCK, adapter );
    }

    LIBRARY_MANAGER&                    manager;
    PRELOAD_KIWAY                       kiway;
    GATED_DESIGN_BLOCK_LIBRARY_ADAPTER* adapter = nullptr;
};

} // namespace


BOOST_AUTO_TEST_SUITE( DesignBlockIoMessages )


BOOST_AUTO_TEST_CASE( LoadNamesTheBlockNotThePath )
{
    SCOPED_LIBRARY lib( makeBlockLibrary( "kicad_qa_db_absent_item", "Amp", "{}" ) );

    DESIGN_BLOCK_IO io;
    wxString        what;

    try
    {
        std::unique_ptr<DESIGN_BLOCK> loaded( io.DesignBlockLoad( lib.path.string(), wxS( "Missing" ) ) );
        BOOST_FAIL( "loading a design block that does not exist must throw" );
    }
    catch( const IO_ERROR& ioe )
    {
        what = ioe.What();
    }

    BOOST_CHECK_MESSAGE( what.Contains( wxS( "Missing" ) ) && !what.Contains( wxS( ".kicad_block" ) ),
                         "the message must name the design block, not the assembled path, got: " + what );
}


BOOST_AUTO_TEST_SUITE_END()


BOOST_AUTO_TEST_SUITE( DesignBlockLibraryAdapter )


BOOST_AUTO_TEST_CASE( LoadReportsUnreadableMetadata )
{
    SCOPED_LIBRARY lib( makeBlockLibrary( "kicad_qa_db_bad_metadata", "Amp", "{ \"description\":" ) );

    LIBRARY_MANAGER                   manager;
    TEST_DESIGN_BLOCK_LIBRARY_ADAPTER adapter( manager );

    adapter.SeedLoadedLibrary( wxS( "blocks" ), lib.path.string() );

    wxString                      error;
    std::unique_ptr<DESIGN_BLOCK> block( adapter.LoadDesignBlock( wxS( "blocks" ), wxS( "Amp" ), false, &error ) );

    BOOST_REQUIRE_MESSAGE( !block, "a design block with unreadable metadata must not load" );
    BOOST_CHECK_MESSAGE( error.Contains( wxS( "Amp.json" ) ), "the reason must name the metadata file, got: " + error );
}


BOOST_AUTO_TEST_CASE( LoadReportsLibraryNotInTable )
{
    LIBRARY_MANAGER                   manager;
    TEST_DESIGN_BLOCK_LIBRARY_ADAPTER adapter( manager );

    wxString                      error;
    std::unique_ptr<DESIGN_BLOCK> block( adapter.LoadDesignBlock( wxS( "nope" ), wxS( "Amp" ), false, &error ) );

    BOOST_REQUIRE( !block );
    BOOST_CHECK_MESSAGE( error.Contains( wxS( "nope" ) ), "the reason must name the missing library, got: " + error );
}


// A table edit while the preload runs asks for another preload, which must not be dropped
BOOST_AUTO_TEST_CASE( PreloadRequestedWhileRunningLoadsNewRows )
{
    SCOPED_LIBRARY first( makeBlockLibrary( "kicad_qa_db_preload_first", "Amp", "{}" ) );
    SCOPED_LIBRARY added( makeBlockLibrary( "kicad_qa_db_preload_added", "Amp", "{}" ) );
    GATED_PRELOAD  preload;

    addGlobalRow( preload.manager, wxS( "LoadedFirst" ), first.path );

    preload.adapter->Hold();
    Pgm().PreloadDesignBlockLibraries( &preload.kiway );
    BOOST_REQUIRE( preload.adapter->WaitUntilHeld() );

    addGlobalRow( preload.manager, wxS( "AddedDuringPreload" ), added.path );
    Pgm().PreloadDesignBlockLibraries( &preload.kiway );

    preload.adapter->Release();
    Pgm().WaitForDesignBlockPreload();

    BOOST_CHECK( preload.adapter->IsLibraryLoaded( wxS( "AddedDuringPreload" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()

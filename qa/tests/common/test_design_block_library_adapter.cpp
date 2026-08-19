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
#include <fstream>
#include <memory>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <design_block.h>
#include <design_block_io.h>
#include <design_block_library_adapter.h>
#include <ki_exception.h>
#include <libraries/library_manager.h>
#include <libraries/library_table.h>


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

} // namespace


BOOST_AUTO_TEST_SUITE( DesignBlockIoMessages )


BOOST_AUTO_TEST_CASE( LoadNamesTheBlockNotThePath )
{
    SCOPED_LIBRARY lib( makeBlockLibrary( "kicad_qa_db_absent_item", "Amp", "{}" ) );

    DESIGN_BLOCK_IO io;
    wxString        what;

    try
    {
        io.DesignBlockLoad( lib.path.string(), wxS( "Missing" ) );
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


BOOST_AUTO_TEST_SUITE_END()

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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/**
 * @file
 * Test suite for embedded files in LIB_SYMBOL objects, including derived symbols.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

// Code under test
#include <lib_symbol.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_io/sch_io_mgr.h>
#include <embedded_files.h>
#include <mmh3_hash.h>
#include <schematic.h>
#include <sch_screen.h>
#include <sch_sheet.h>
#include <sch_symbol.h>
#include <project.h>
#include <lib_id.h>

#include <wx/filename.h>
#include <wx/dir.h>

class SYMBOL_EMBEDDED_FILES_TEST_FIXTURE
{
public:
    SYMBOL_EMBEDDED_FILES_TEST_FIXTURE()
    {
        m_tempDir = wxFileName::CreateTempFileName( wxS( "kicad_test_" ) );
        wxRemoveFile( m_tempDir );
        wxFileName::Mkdir( m_tempDir );

        m_libPath = wxFileName( m_tempDir, wxS( "test_lib.kicad_sym" ) ).GetFullPath();
    }

    ~SYMBOL_EMBEDDED_FILES_TEST_FIXTURE()
    {
        // Clean up temporary directory
        if( wxFileName::DirExists( m_tempDir ) )
        {
            wxFileName::Rmdir( m_tempDir, wxPATH_RMDIR_RECURSIVE );
        }
    }

    wxString GetTempDir() const { return m_tempDir; }
    wxString GetLibPath() const { return m_libPath; }

    /**
     * Create a test embedded file with the given name and content.
     */
    EMBEDDED_FILES::EMBEDDED_FILE* CreateTestEmbeddedFile( const wxString& aName,
                                                           const std::string& aContent )
    {
        EMBEDDED_FILES::EMBEDDED_FILE* file = new EMBEDDED_FILES::EMBEDDED_FILE();
        file->name = aName;
        file->decompressedData.assign( aContent.begin(), aContent.end() );

        MMH3_HASH hash( EMBEDDED_FILES::Seed() );
        hash.add( file->decompressedData );
        file->data_hash = hash.digest().ToString();

        EMBEDDED_FILES::RETURN_CODE result = EMBEDDED_FILES::CompressAndEncode( *file );
        BOOST_REQUIRE( result == EMBEDDED_FILES::RETURN_CODE::OK );

        return file;
    }

private:
    wxString m_tempDir;
    wxString m_libPath;
};


BOOST_FIXTURE_TEST_SUITE( SymbolEmbeddedFiles, SYMBOL_EMBEDDED_FILES_TEST_FIXTURE )


/**
 * Test embedded files in parent and derived symbols:
 * 1) Create a symbol and add an embedded file and save.
 * 2) Close and reload the symbol to verify that the embedded file still exists.
 * 3) Create a derived symbol based on the first symbol and save.
 * 4) Close and re-open the derived symbol. Ensure that the embedded file does not exist
 *    in the derived symbol.
 * 5) Embed a new file in the derived symbol.
 * 6) Close and reopen the derived symbol. Ensure that the new embedded file exists.
 * 7) Add only the derived symbol to a new schematic sheet. Ensure that both embedded
 *    files are present in the schematic.
 */
BOOST_AUTO_TEST_CASE( DerivedSymbolEmbeddedFiles )
{
    // Step 1: Create a parent symbol with an embedded file and save
    std::unique_ptr<LIB_SYMBOL> parentSymbol = std::make_unique<LIB_SYMBOL>( wxS( "ParentSymbol" ) );
    parentSymbol->GetValueField().SetText( wxS( "ParentSymbol" ) );
    parentSymbol->GetReferenceField().SetText( wxS( "U" ) );

    // Add an embedded file to the parent
    EMBEDDED_FILES::EMBEDDED_FILE* parentFile =
        CreateTestEmbeddedFile( wxS( "parent_datasheet.pdf" ), "Parent datasheet content" );
    parentSymbol->AddFile( parentFile );

    BOOST_CHECK( parentSymbol->HasFile( wxS( "parent_datasheet.pdf" ) ) );
    BOOST_CHECK_EQUAL( parentSymbol->EmbeddedFileMap().size(), 1 );

    // Save the parent symbol to the library
    {
        IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        plugin->CreateLibrary( GetLibPath() );
        plugin->SaveSymbol( GetLibPath(), new LIB_SYMBOL( *parentSymbol ) );
        plugin->SaveLibrary( GetLibPath() );
    }

    // Step 2: Close and reload the symbol to verify that the embedded file still exists.
    {
        IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        LIB_SYMBOL* loadedParent = plugin->LoadSymbol( GetLibPath(), wxS( "ParentSymbol" ) );
        BOOST_REQUIRE( loadedParent );
        BOOST_CHECK( loadedParent->HasFile( wxS( "parent_datasheet.pdf" ) ) );
    }

    // Step 3: Create a derived symbol based on the first symbol and save
    {
        IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        LIB_SYMBOL* loadedParent = plugin->LoadSymbol( GetLibPath(), wxS( "ParentSymbol" ) );
        BOOST_REQUIRE( loadedParent );

        std::unique_ptr<LIB_SYMBOL> derivedSymbol = std::make_unique<LIB_SYMBOL>( wxS( "DerivedSymbol" ) );
        derivedSymbol->GetValueField().SetText( wxS( "DerivedSymbol" ) );
        derivedSymbol->SetParent( loadedParent );

        plugin->SaveSymbol( GetLibPath(), new LIB_SYMBOL( *derivedSymbol ) );
        plugin->SaveLibrary( GetLibPath() );
    }

    // Step 4: Close and re-open the derived symbol. Ensure that the embedded file does not exist in the derived symbol.
    {
        IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        LIB_SYMBOL* loadedDerived = plugin->LoadSymbol( GetLibPath(), wxS( "DerivedSymbol" ) );
        BOOST_REQUIRE( loadedDerived );

        // The derived symbol itself should not have the file in its map
        BOOST_CHECK( !loadedDerived->HasFile( wxS( "parent_datasheet.pdf" ) ) );
        BOOST_CHECK_EQUAL( loadedDerived->EmbeddedFileMap().size(), 0 );
    }

    // Step 5: Embed a new file in the derived symbol
    {
        IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        // We need to load parent to set it again, because LoadSymbol doesn't automatically link parents from disk without a library table
        LIB_SYMBOL* loadedParent = plugin->LoadSymbol( GetLibPath(), wxS( "ParentSymbol" ) );
        LIB_SYMBOL* loadedDerived = plugin->LoadSymbol( GetLibPath(), wxS( "DerivedSymbol" ) );
        BOOST_REQUIRE( loadedDerived );
        loadedDerived->SetParent( loadedParent );

        EMBEDDED_FILES::EMBEDDED_FILE* derivedFile =
            CreateTestEmbeddedFile( wxS( "derived_datasheet.pdf" ), "Derived datasheet content" );
        loadedDerived->AddFile( derivedFile );

        BOOST_CHECK( loadedDerived->HasFile( wxS( "derived_datasheet.pdf" ) ) );

        plugin->SaveSymbol( GetLibPath(), new LIB_SYMBOL( *loadedDerived ) );
        plugin->SaveLibrary( GetLibPath() );
    }

    // Step 6: Close and reopen the derived symbol. Ensure that the new embedded file exists
    {
        IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        LIB_SYMBOL* loadedDerived = plugin->LoadSymbol( GetLibPath(), wxS( "DerivedSymbol" ) );
        BOOST_REQUIRE( loadedDerived );
        BOOST_CHECK( loadedDerived->HasFile( wxS( "derived_datasheet.pdf" ) ) );
    }

    // Step 7: Add only the derived symbol to a new schematic sheet. Ensure that both embedded files are present in the schematic.
    {
        SCHEMATIC schematic( nullptr );

        schematic.Reset();
        SCH_SHEET* defaultSheet = schematic.GetTopLevelSheet( 0 );

        SCH_SHEET* rootSheet = new SCH_SHEET( &schematic );
        SCH_SCREEN* screen = new SCH_SCREEN( &schematic );
        rootSheet->SetScreen( screen );

        schematic.AddTopLevelSheet( rootSheet );
        schematic.RemoveTopLevelSheet( defaultSheet );
        delete defaultSheet;

        IO_RELEASER<SCH_IO> plugin( SCH_IO_MGR::FindPlugin( SCH_IO_MGR::SCH_KICAD ) );
        LIB_SYMBOL* loadedParent = plugin->LoadSymbol( GetLibPath(), wxS( "ParentSymbol" ) );
        LIB_SYMBOL* loadedDerived = plugin->LoadSymbol( GetLibPath(), wxS( "DerivedSymbol" ) );

        loadedDerived->SetParent( loadedParent );

        SCH_SHEET_PATH rootPath;
        rootPath.push_back( &schematic.Root() );
        rootPath.push_back( rootSheet );

        SCH_SYMBOL* schSymbol = new SCH_SYMBOL( *loadedDerived, LIB_ID( "test_lib", "DerivedSymbol" ),
                                                &rootPath, 0 );
        screen->Append( schSymbol );

        // Verify derived file
        BOOST_CHECK( schSymbol->GetLibSymbolRef()->HasFile( wxS( "derived_datasheet.pdf" ) ) );

        // Verify parent file (should be flattened into the symbol)
        BOOST_CHECK( schSymbol->GetLibSymbolRef()->HasFile( wxS( "parent_datasheet.pdf" ) ) );
    }
}


/**
 * Test that copying a symbol with embedded files preserves the files.
 */
BOOST_AUTO_TEST_CASE( CopySymbolPreservesEmbeddedFiles )
{
    // Create a symbol with an embedded file
    std::unique_ptr<LIB_SYMBOL> original = std::make_unique<LIB_SYMBOL>( wxS( "Original" ) );
    original->GetValueField().SetText( wxS( "Original" ) );

    EMBEDDED_FILES::EMBEDDED_FILE* file =
        CreateTestEmbeddedFile( wxS( "test.pdf" ), "Test content" );
    original->AddFile( file );

    BOOST_CHECK_EQUAL( original->EmbeddedFileMap().size(), 1 );

    // Copy the symbol using copy constructor
    std::unique_ptr<LIB_SYMBOL> copy = std::make_unique<LIB_SYMBOL>( *original );

    // Verify the copy has the embedded file
    BOOST_CHECK_EQUAL( copy->EmbeddedFileMap().size(), 1 );
    BOOST_CHECK( copy->HasFile( wxS( "test.pdf" ) ) );

    EMBEDDED_FILES::EMBEDDED_FILE* copiedFile = copy->GetEmbeddedFile( wxS( "test.pdf" ) );
    BOOST_REQUIRE( copiedFile );
    BOOST_CHECK( copiedFile->Validate() );
}


/**
 * Test that assignment operator preserves embedded files.
 */
BOOST_AUTO_TEST_CASE( AssignmentPreservesEmbeddedFiles )
{
    // Create a symbol with an embedded file
    std::unique_ptr<LIB_SYMBOL> source = std::make_unique<LIB_SYMBOL>( wxS( "Source" ) );
    source->GetValueField().SetText( wxS( "Source" ) );

    EMBEDDED_FILES::EMBEDDED_FILE* file =
        CreateTestEmbeddedFile( wxS( "source.pdf" ), "Source content" );
    source->AddFile( file );

    // Create a destination symbol
    std::unique_ptr<LIB_SYMBOL> dest = std::make_unique<LIB_SYMBOL>( wxS( "Dest" ) );
    dest->GetValueField().SetText( wxS( "Dest" ) );

    // Assign source to destination
    *dest = *source;

    // Verify destination has the embedded file
    BOOST_CHECK_EQUAL( dest->EmbeddedFileMap().size(), 1 );
    BOOST_CHECK( dest->HasFile( wxS( "source.pdf" ) ) );

    EMBEDDED_FILES::EMBEDDED_FILE* destFile = dest->GetEmbeddedFile( wxS( "source.pdf" ) );
    BOOST_REQUIRE( destFile );
    BOOST_CHECK( destFile->Validate() );
}


BOOST_AUTO_TEST_SUITE_END()

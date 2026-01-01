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
 * @file test_symbol_library_parse_error.cpp
 * Test suite for issue #22241: Symbol editor truncates symbol library after parse error
 *
 * When a symbol library has a parse error, only symbols before the error are loaded.
 * If the user then saves the library, symbols after the parse error are permanently lost.
 * This test verifies that the save operation is blocked when a library has parse errors.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <lib_symbol.h>
#include <richio.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr_lib_cache.h>
#include <sch_io/sch_io_mgr.h>

#include <wx/filename.h>
#include <wx/file.h>
#include <wx/stdpaths.h>

#include <fstream>


class SYMBOL_LIBRARY_PARSE_ERROR_FIXTURE
{
public:
    SYMBOL_LIBRARY_PARSE_ERROR_FIXTURE()
    {
    }

    ~SYMBOL_LIBRARY_PARSE_ERROR_FIXTURE()
    {
        // Clean up temp files
        for( const wxString& file : m_tempFiles )
        {
            if( wxFileExists( file ) )
                wxRemoveFile( file );
        }
    }

    wxString GetTempLibraryPath()
    {
        wxString tempDir = wxStandardPaths::Get().GetTempDir();
        wxString fileName = tempDir + wxFileName::GetPathSeparator()
                            + wxString::Format( "test_lib_%d.kicad_sym", rand() );
        m_tempFiles.push_back( fileName );
        return fileName;
    }

    /**
     * Create a valid symbol library file with the specified number of symbols.
     * Each symbol is named "Symbol_N" where N is 1, 2, 3, etc.
     */
    void CreateValidLibrary( const wxString& aPath, int aSymbolCount )
    {
        std::ofstream file( aPath.ToStdString() );

        file << "(kicad_symbol_lib (version 20231120) (generator \"kicad_symbol_editor\")\n";
        file << "  (generator_version \"8.0\")\n";

        for( int i = 1; i <= aSymbolCount; i++ )
        {
            file << "  (symbol \"Symbol_" << i << "\"\n";
            file << "    (exclude_from_sim no) (in_bom yes) (on_board yes)\n";
            file << "    (duplicate_pin_numbers_are_jumpers no)\n";
            file << "    (property \"Reference\" \"U\" (at 0 0 0)\n";
            file << "      (effects (font (size 1.27 1.27)))\n";
            file << "    )\n";
            file << "    (property \"Value\" \"Symbol_" << i << "\" (at 0 2.54 0)\n";
            file << "      (effects (font (size 1.27 1.27)))\n";
            file << "    )\n";
            file << "    (property \"Footprint\" \"\" (at 0 0 0)\n";
            file << "      (effects (font (size 1.27 1.27)) hide)\n";
            file << "    )\n";
            file << "    (property \"Datasheet\" \"\" (at 0 0 0)\n";
            file << "      (effects (font (size 1.27 1.27)) hide)\n";
            file << "    )\n";
            file << "    (property \"Description\" \"Test symbol " << i << "\" (at 0 0 0)\n";
            file << "      (effects (font (size 1.27 1.27)) hide)\n";
            file << "    )\n";
            file << "    (symbol \"Symbol_" << i << "_1_1\"\n";
            file << "      (rectangle (start -2.54 2.54) (end 2.54 -2.54)\n";
            file << "        (stroke (width 0.254) (type default))\n";
            file << "        (fill (type background))\n";
            file << "      )\n";
            file << "    )\n";
            file << "  )\n";
        }

        file << ")\n";
        file.close();
    }

    /**
     * Corrupt the library file by introducing a parse error after the specified symbol.
     * This simulates a bad merge or file corruption that would cause parsing to fail
     * partway through the library.
     */
    void CorruptLibraryAfterSymbol( const wxString& aPath, int aSymbolNumber )
    {
        // Read the entire file
        std::ifstream inFile( aPath.ToStdString() );
        std::string content( ( std::istreambuf_iterator<char>( inFile ) ),
                             std::istreambuf_iterator<char>() );
        inFile.close();

        // Find the position after the specified symbol
        std::string searchFor = "Symbol_" + std::to_string( aSymbolNumber ) + "_1_1";
        size_t pos = content.find( searchFor );

        if( pos != std::string::npos )
        {
            // Find the end of this symbol (look for the closing parenthesis pattern)
            // Go past the symbol definition to corrupt the next one
            pos = content.find( "  (symbol \"Symbol_" + std::to_string( aSymbolNumber + 1 ), pos );

            if( pos != std::string::npos )
            {
                // Insert garbage that will cause a parse error
                content.insert( pos + 10, "CORRUPT_DATA_HERE{{{invalid}}}syntax###" );
            }
        }

        // Write the corrupted content back
        std::ofstream outFile( aPath.ToStdString() );
        outFile << content;
        outFile.close();
    }

    std::vector<wxString> m_tempFiles;
};


BOOST_FIXTURE_TEST_SUITE( SymbolLibraryParseError, SYMBOL_LIBRARY_PARSE_ERROR_FIXTURE )


/**
 * Test that a valid library can be created, loaded, and all symbols are present.
 * This is a baseline test to ensure our test infrastructure works correctly.
 */
BOOST_AUTO_TEST_CASE( ValidLibraryLoadsCompletely )
{
    wxString libPath = GetTempLibraryPath();
    const int symbolCount = 5;

    // Create a valid library with 5 symbols
    CreateValidLibrary( libPath, symbolCount );

    // Load the library using the cache
    SCH_IO_KICAD_SEXPR_LIB_CACHE cache( libPath );

    BOOST_CHECK_NO_THROW( cache.Load() );

    // Verify all symbols are present
    const LIB_SYMBOL_MAP& symbols = cache.GetSymbolMap();
    BOOST_CHECK_EQUAL( symbols.size(), symbolCount );

    for( int i = 1; i <= symbolCount; i++ )
    {
        wxString symbolName = wxString::Format( "Symbol_%d", i );
        BOOST_CHECK_MESSAGE( symbols.find( symbolName ) != symbols.end(),
                             "Symbol " << symbolName << " should be present" );
    }
}


/**
 * Test that a library with a parse error loads all valid symbols (skipping only the bad one).
 * This is the fix for issue #22241 - we continue loading after errors instead of stopping.
 */
BOOST_AUTO_TEST_CASE( ParseErrorSkipsCorruptSymbol )
{
    wxString libPath = GetTempLibraryPath();
    const int symbolCount = 5;
    const int corruptSymbol = 3;  // Symbol_3 will be corrupted

    // Create a valid library with 5 symbols
    CreateValidLibrary( libPath, symbolCount );

    // Corrupt symbol 3 (so it will fail to parse, but symbols 4 and 5 should still load)
    CorruptLibraryAfterSymbol( libPath, corruptSymbol - 1 );

    // Try to load the library - it should throw an exception due to parse error
    // but still load all valid symbols
    SCH_IO_KICAD_SEXPR_LIB_CACHE cache( libPath );

    // The Load() should throw an IO_ERROR to notify of the parse error
    BOOST_CHECK_THROW( cache.Load(), IO_ERROR );

    // Verify that all valid symbols are present (only the corrupt one is missing)
    const LIB_SYMBOL_MAP& symbols = cache.GetSymbolMap();

    // Symbols 1 and 2 should be present (before the corrupted symbol)
    for( int i = 1; i < corruptSymbol; i++ )
    {
        wxString symbolName = wxString::Format( "Symbol_%d", i );
        BOOST_CHECK_MESSAGE( symbols.find( symbolName ) != symbols.end(),
                             "Symbol " << symbolName << " should be present (before corrupt symbol)" );
    }

    // Symbol 3 should NOT be present (it's the corrupt one)
    {
        wxString symbolName = wxString::Format( "Symbol_%d", corruptSymbol );
        BOOST_CHECK_MESSAGE( symbols.find( symbolName ) == symbols.end(),
                             "Symbol " << symbolName << " should NOT be present (corrupt symbol)" );
    }

    // Symbols 4 and 5 should be present (after the corrupted symbol - error recovery worked)
    for( int i = corruptSymbol + 1; i <= symbolCount; i++ )
    {
        wxString symbolName = wxString::Format( "Symbol_%d", i );
        BOOST_CHECK_MESSAGE( symbols.find( symbolName ) != symbols.end(),
                             "Symbol " << symbolName << " should be present (after corrupt symbol, recovered)" );
    }

    // Verify that 4 symbols are loaded (all except the corrupt one)
    BOOST_CHECK_EQUAL( symbols.size(), symbolCount - 1 );
}


/**
 * Test that saving a library after a parse error is prevented.
 * This is the core test for issue #22241 - we need to ensure that saving
 * a library with parse errors doesn't silently lose the corrupt symbol.
 */
BOOST_AUTO_TEST_CASE( SaveAfterParseErrorIsPrevented )
{
    wxString libPath = GetTempLibraryPath();
    wxString backupPath = libPath + ".backup";
    m_tempFiles.push_back( backupPath );

    const int symbolCount = 5;
    const int corruptSymbol = 3;  // Symbol_3 will be corrupted

    // Create a valid library with 5 symbols
    CreateValidLibrary( libPath, symbolCount );

    // Make a backup copy of the original file
    wxCopyFile( libPath, backupPath );

    // Corrupt symbol 3
    CorruptLibraryAfterSymbol( libPath, corruptSymbol - 1 );

    // Try to load the library
    SCH_IO_KICAD_SEXPR_LIB_CACHE cache( libPath );

    try
    {
        cache.Load();
        BOOST_FAIL( "Load should have thrown an exception due to parse error" );
    }
    catch( const IO_ERROR& )
    {
        // Expected - parse error occurred but library was still loaded
    }

    // Verify all valid symbols were loaded (only the corrupt one is missing)
    const LIB_SYMBOL_MAP& symbols = cache.GetSymbolMap();
    BOOST_CHECK_EQUAL( symbols.size(), symbolCount - 1 );  // 4 symbols loaded

    // Verify the parse error flag is set
    BOOST_CHECK_MESSAGE( cache.HasParseError(),
                         "HasParseError() should return true after a parse error" );

    // Mark the cache as modified (simulating user making a change)
    cache.SetModified( true );

    // The key test: attempting to save should throw an exception because
    // the library has parse errors. This prevents losing the corrupt symbol.
    BOOST_CHECK_THROW( cache.Save(), IO_ERROR );

    // Verify the original file was not modified (backup should match)
    // by checking that it still has the corruption (can't be loaded cleanly)
    SCH_IO_KICAD_SEXPR_LIB_CACHE reloadedCache( libPath );
    BOOST_CHECK_THROW( reloadedCache.Load(), IO_ERROR );
}


/**
 * Test that the cache correctly tracks parse error state.
 */
BOOST_AUTO_TEST_CASE( CacheTracksParseErrorState )
{
    wxString libPath = GetTempLibraryPath();
    const int symbolCount = 5;
    const int corruptAfter = 2;

    // Create and corrupt a library
    CreateValidLibrary( libPath, symbolCount );
    CorruptLibraryAfterSymbol( libPath, corruptAfter );

    SCH_IO_KICAD_SEXPR_LIB_CACHE cache( libPath );

    // Before loading, no parse error
    BOOST_CHECK( !cache.HasParseError() );

    try
    {
        cache.Load();
    }
    catch( const IO_ERROR& )
    {
        // Expected
    }

    // After failed load, parse error flag should be set
    BOOST_CHECK_MESSAGE( cache.HasParseError(),
                         "HasParseError() should return true after parse error during load" );

    // For a valid library, the state should be clean:
    wxString validLibPath = GetTempLibraryPath();
    CreateValidLibrary( validLibPath, symbolCount );

    SCH_IO_KICAD_SEXPR_LIB_CACHE validCache( validLibPath );

    // Before loading, no parse error
    BOOST_CHECK( !validCache.HasParseError() );

    BOOST_CHECK_NO_THROW( validCache.Load() );

    // After successful load, still no parse error
    BOOST_CHECK_MESSAGE( !validCache.HasParseError(),
                         "HasParseError() should return false after successful load" );

    // Verify that a valid library can be saved
    validCache.SetModified( true );
    BOOST_CHECK_NO_THROW( validCache.Save() );
}


BOOST_AUTO_TEST_SUITE_END()

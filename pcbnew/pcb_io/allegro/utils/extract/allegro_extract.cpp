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
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <fstream>
#include <stdexcept>
#include <iostream>
#include <memory>

#include <wx/init.h>

#include <argparse/argparse.hpp>
#include <magic_enum.hpp>
#include <fmt.h>

#include <core/profile.h>
#include <ki_exception.h>

#include <convert/allegro_parser.h>
#include <convert/allegro_db.h>
#include <utils/extract/allegro_ascii.h>
#include <utils/extract/extract_spec_parser.h>


static const char* traceAllegroExtract = "ALLEGRO_EXTRACT";


int main( int argc, char** argv )
{
    wxInitialize( argc, argv );

    argparse::ArgumentParser argParser( "Allegro .brd database extraction" );

    // clang-format off
    argParser.add_argument( "-v", "--verbose" )
        .help( "Verbose output" )
        .flag();

    argParser.add_argument( "-b", "--board" )
        .help( "Allegro .brd file" )
        .nargs( 1 )
        .metavar( "BRD_FILE" )
        .required();

    argParser.add_argument( "-S", "--summary" )
        .help( "Print a summary of the file" )
        .flag();

    argParser.add_argument( "-v", "--views" )
        .nargs( argparse::nargs_pattern::at_least_one )
        .help( "List of view files to extract" )
        .metavar( "VIEWS" );

    // clang-format on

    try
    {
        argParser.parse_args( argc, argv );
    }
    catch( const std::exception& err )
    {
        fmt::print( "{}\n", err.what() );

        std::cout << argParser << std::endl;
        return 1;
    }

    std::string   filename = argParser.get<std::string>( "board" );
    std::ifstream fin( filename, std::ios::binary );

    // Find out the view are all available
    std::vector<std::string> viewFiles;

    // We're going to munge all the extracted blocks from the view files into
    // a single list for extraction
    std::vector<ALLEGRO::EXTRACT_SPEC_PARSER::IR::BLOCK> extractedBlocks;

    if( argParser.is_used( "views" ) )
    {
        viewFiles = argParser.get<std::vector<std::string>>( "views" );

        for( const auto& viewFile : viewFiles )
        {
            std::ifstream vfin( viewFile );
            if( !vfin.is_open() )
            {
                fmt::print( "View file '{}' could not be opened\n", viewFile );
                return 1;
            }

            ALLEGRO::EXTRACT_SPEC_PARSER::PARSER specParser;

            std::string viewFileContents( ( std::istreambuf_iterator<char>( vfin ) ),
                                          std::istreambuf_iterator<char>() );

            auto specOrError = specParser.ParseBuffer( viewFileContents );
            if( !specOrError )
            {
                const auto& err = specOrError.error();
                fmt::print( "Error parsing view file '{}': {} at line {}\n", viewFile, err.description, err.line );
                return 1;
            }

            // Add all the blocks from this spec to our list
            const auto& spec = specOrError.value();
            extractedBlocks.insert( extractedBlocks.end(), spec.Blocks.begin(), spec.Blocks.end() );
        }

        wxLogTrace( traceAllegroExtract, "Loaded %zu views from %zu files", extractedBlocks.size(), viewFiles.size() );
    }

    // Now we're happy with the arguments, proceed to parse the file

    ALLEGRO::FILE_STREAM allegroStream( fin );

    ALLEGRO::PARSER parser( allegroStream, nullptr );
    parser.EndAtUnknownBlock( true );

    PROF_TIMER parseTimer;

    std::unique_ptr<ALLEGRO::BRD_DB> board;

    try
    {
        board = parser.Parse();
    }
    catch( const IO_ERROR& e )
    {
        fmt::print( "Error parsing Allegro file: {}\n", e.What() );
        return 1;
    }

    parseTimer.Stop();

    // At this point we now have the RAW_BOARD object, which provides
    // a "curated" view of the destructured Allegro .brd file.

    if( argParser.get<bool>( "summary" ) )
    {
        fmt::print( "Board file:       {}\n", filename );
        fmt::print( "  Parsed in:      {}\n", parseTimer.to_string() );
        fmt::print( "  Format version: {:#010x}\n", board->m_Header->m_Magic );

        // dumpObjectCounts( std::cout, boardContent );

        fmt::print( "  String map:      {} entries\n", board->m_StringTable.size() );
    }

    if( extractedBlocks.size() > 0 )
    {
        STRING_FORMATTER formatter;
        ALLEGRO::ASCII_EXTRACTOR extractor( *board, formatter );

        for( const ALLEGRO::EXTRACT_SPEC_PARSER::IR::BLOCK& block : extractedBlocks )
        {
            wxLogTrace( traceAllegroExtract, "Extracting block of type %d with %zu fields",
                        static_cast<int>( block.Type ), block.Fields.size() );

            try
            {
                extractor.Extract( block );
            }
            catch( const IO_ERROR& e )
            {
                fmt::print( "Error during extraction: {}\n", e.What() );
                return 1;
            }
        }

        fmt::print( "{}", formatter.GetString() );
    }

    wxUninitialize();

    return 0;
}

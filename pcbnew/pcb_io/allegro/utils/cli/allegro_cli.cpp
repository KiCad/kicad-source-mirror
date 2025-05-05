

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

#include <allegro_parser.h>
#include <allegro_pcb_structs.h>


int main( int argc, char** argv )
{
    wxInitialize( argc, argv );

    argparse::ArgumentParser argParser( "Allegro .brd Parser utils" );

    // clang-format off
    argParser.add_argument( "-v", "--verbose" )
        .help( "Verbose output" )
        .flag();

    argParser.add_argument( "file" )
        .help( "Allegro .brd file" )
        .metavar( "BRD_FILE" )
        .required();

    argParser.add_argument( "-S", "--summary" )
        .help( "Print a summary of the file structure" )
        .flag();

    argParser.add_argument( "--st", "--string-table" )
        .help( "Print the string table" )
        .flag();

    argParser.add_argument( "--sk", "--string-by-key" )
        .help( "Print the given string ID" )
        .scan<'i', uint32_t>()
        .metavar( "ID_32" );

    argParser.add_argument( "--sm", "--string-match" )
        .help( "Print strings that match a substring" )
        .metavar( "SUBSTRING" );

    argParser.add_argument( "--bc", "--block-counts" )
        .help( "Print counts of each discovered block type" )
        .flag();
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

    std::string   filename = argParser.get<std::string>( "file" );
    std::ifstream fin( filename, std::ios::binary );

    // kaitai::kstream ks( &fin );
    ALLEGRO::FILE_STREAM allegroStream( fin );

    ALLEGRO::PARSER parser( allegroStream );
    parser.EndAtUnknownBlock( true );

    PROF_TIMER parseTimer;

    std::unique_ptr<ALLEGRO::RAW_BOARD> board;

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

        fmt::print( "  String map: {} entries", board->m_StringTable.size() );
    }

    if( argParser.get<bool>( "string-table" ) )
    {
        for( const auto& [id, str] : board->m_StringTable )
        {
            fmt::print( "{:#010x}: {}\n", id, str );
        }
    }
    else if( argParser.is_used( "string-match" ) )
    {
        const std::string pat = argParser.get<std::string>( "string-match" );

        const auto& stringTable = board->m_StringTable;

        for( const auto& [id, str] : stringTable )
        {
            if( str.find( pat ) != std::string::npos )
            {
                fmt::print( "{:#010x}: {}\n", id, str );
            }
        }
    }
    else if( argParser.is_used( "string-by-key" ) )
    {
        uint32_t stringId = argParser.get<uint32_t>( "string-by-key" );

        const auto& stringTable = board->m_StringTable;

        if( stringTable.count( stringId ) )
        {
            fmt::print( "String ID {:#010x}: {}\n", stringId, stringTable.at( stringId ) );
        }
        else
        {
            fmt::print( "String ID {:#010x} not found\n", stringId );
        }
    }
    else if( argParser.get<bool>( "block-counts" ) )
    {
        std::map<uint8_t, size_t> blockCounts;

        for( const auto& obj : board->m_ObjectLists )
        {
            blockCounts[obj.first] = obj.second.size();
        }

        for( const auto& [blockType, count] : blockCounts )
        {
            fmt::print( "Block type {:#04x}: {:5d}\n", blockType, count );
        }
    }

    wxUninitialize();

    return 0;
}

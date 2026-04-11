/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <qa_utils/utility_registry.h>

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include <wx/cmdline.h>
#include <wx/msgout.h>

#include <fmt/format.h>

#include <board.h>
#include <board_item.h>
#include <common.h>
#include <core/profile.h>

#include <pcb_io/allegro/pcb_io_allegro.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr_parser.h>

#include <qa_utils/stdstream_line_reader.h>


using PARSE_DURATION = std::chrono::microseconds;


/**
 * Generic board parser - this makes no assumption about what the source data might be.
 */
class BOARD_PARSER
{
public:
    virtual ~BOARD_PARSER() = default;

    /**
     * Actually perform the parsing and return a BOARD_ITEM if successful, or nullptr if not.
     *
     * @throw IO_ERROR if there is a parse failure
     */
    virtual std::unique_ptr<BOARD_ITEM> Parse() = 0;
};


/**
 * Provide the BOARD_PARSER interface wrapping a normal PCB_IO file-based plugin lookup
 */
class FILE_PARSER : public BOARD_PARSER
{
public:
    FILE_PARSER( PCB_IO_MGR::PCB_FILE_T aFileType, const wxString& aFileName ) :
            m_fileType( aFileType ),
            m_fileName( aFileName )
    {
    }

    std::unique_ptr<BOARD_ITEM> Parse() override
    {
        BOARD* board = PCB_IO_MGR::Load( m_fileType, m_fileName, nullptr, {}, nullptr, nullptr );
        return std::unique_ptr<BOARD_ITEM>( board );
    }

private:
    PCB_IO_MGR::PCB_FILE_T m_fileType;
    wxString               m_fileName;
};


/**
 * In order to support fuzz testing, we need to be able to parse from stdin.
 * The PCB_IO interface is designed to parse from a file, so this class is a simple
 * wrapper that needs to be implemented to wrap a plugin's core parser function.
 */
class STREAM_PARSER : public BOARD_PARSER
{
public:
    /**
     * Take some input stream and prepare it for parsing.  This may involve reading
     * the whole stream into memory, or it may involve setting up some kind of streaming
     * reader.
     *
     * @throw IO_ERROR if there is a problem setting up from the stream
     */
    virtual void PrepareStream( std::istream& aStream ) = 0;
};


class SEXPR_STREAM_PARSER : public STREAM_PARSER
{
public:
    void PrepareStream( std::istream& aStream ) override { m_reader.SetStream( aStream ); }

    std::unique_ptr<BOARD_ITEM> Parse() override
    {
        PCB_IO_KICAD_SEXPR_PARSER parser( &m_reader, nullptr, nullptr );
        return std::unique_ptr<BOARD_ITEM>{ parser.Parse() };
    }

private:
    STDISTREAM_LINE_READER m_reader;
};


class ALLEGRO_BRD_STREAM_PARSER : public STREAM_PARSER
{
public:
    void PrepareStream( std::istream& aStream ) override
    {
        // Allegro parser expects to mmap a file, so we need to
        // dump it all in memory to simulate that.
        m_buffer.assign( std::istreambuf_iterator<char>( aStream ), std::istreambuf_iterator<char>() );

        // Check if stream reading failed
        if( aStream.fail() && !aStream.eof() )
        {
            THROW_IO_ERROR( _( "Failed to read from input stream" ) );
        }
    }

    std::unique_ptr<BOARD_ITEM> Parse() override
    {
        PCB_IO_ALLEGRO         allegroParser;
        std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

        if( !allegroParser.LoadBoardFromData( m_buffer.data(), m_buffer.size(), *board ) )
        {
            return nullptr;
        }

        return board;
    }

private:
    std::vector<uint8_t> m_buffer;
};


/**
 * Runs a BOARD_PARSER against a filename or stream and reports results.
 */
class PCB_PARSE_RUNNER
{
public:
    PCB_PARSE_RUNNER( PCB_IO_MGR::PCB_FILE_T aPluginType, bool aVerbose ) :
            m_pluginType( aPluginType ),
            m_verbose( aVerbose )
    {
    }

    bool Parse( std::istream& aStream )
    {
        std::unique_ptr<STREAM_PARSER> parser;

        switch( m_pluginType )
        {
        case PCB_IO_MGR::KICAD_SEXP: parser = std::make_unique<SEXPR_STREAM_PARSER>(); break;
        case PCB_IO_MGR::ALLEGRO: parser = std::make_unique<ALLEGRO_BRD_STREAM_PARSER>(); break;
        default:
            std::cerr << fmt::format( "Unsupported plugin type for streaming input: {}",
                                      static_cast<int>( m_pluginType ) )
                      << std::endl;
            return false;
        }

        wxCHECK( parser, false );

        try
        {
            parser->PrepareStream( aStream );
        }
        catch( const IO_ERROR& e )
        {
            std::cerr << fmt::format( "Error preparing stream: {}", e.What().ToStdString() ) << std::endl;
            return false;
        }

        return doParse( *parser );
    }

    bool Parse( const wxString& aFilename )
    {
        FILE_PARSER parser( m_pluginType, aFilename );
        return doParse( parser );
    }

private:
    bool doParse( BOARD_PARSER& aParser )
    {
        std::unique_ptr<BOARD_ITEM> board;
        PARSE_DURATION              duration{};

        try
        {
            PROF_TIMER timer;
            board = aParser.Parse();
            duration = timer.SinceStart<PARSE_DURATION>();
        }
        catch( const IO_ERROR& e )
        {
            std::cerr << "Parsing failed: " << e.What() << std::endl;
        }

        if( m_verbose )
        {
            std::cout << fmt::format( "Took: {}us", duration.count() ) << std::endl;

            if( board )
                std::cout << fmt::format( "  {} nets", board->GetBoard()->GetNetCount() ) << std::endl;
        }

        return board != nullptr;
    }

    PCB_IO_MGR::PCB_FILE_T m_pluginType;
    bool                   m_verbose;
};


static const wxCmdLineEntryDesc g_cmdLineDesc[] = {
    {
            wxCMD_LINE_SWITCH,
            "h",
            "help",
            _( "displays help on the command line parameters" ).mb_str(),
            wxCMD_LINE_VAL_NONE,
            wxCMD_LINE_OPTION_HELP,
    },
    {
            wxCMD_LINE_SWITCH,
            "v",
            "verbose",
            _( "print parsing information" ).mb_str(),
    },
    {
            wxCMD_LINE_OPTION,
            "p",
            "plugin",
            _( "parser plugin to use (kicad, allegro, etc.)" ).mb_str(),
            wxCMD_LINE_VAL_STRING,
    },
    {
            wxCMD_LINE_SWITCH,
            nullptr,
            "list-plugins",
            _( "list available plugins and exit" ).mb_str(),
    },
    {
            wxCMD_LINE_OPTION,
            "l",
            "loop",
            _( "number of times to loop when parsing from stdin (for AFL)" ).mb_str(),
            wxCMD_LINE_VAL_NUMBER,
    },
    {
            wxCMD_LINE_PARAM,
            nullptr,
            nullptr,
            _( "input file" ).mb_str(),
            wxCMD_LINE_VAL_STRING,
            wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE,
    },
    {
            wxCMD_LINE_NONE,
    }
};


enum PARSER_RET_CODES
{
    PARSE_FAILED = KI_TEST::RET_CODES::TOOL_SPECIFIC,
};


/**
 * Map from command line keys to plugin types
 */
static const std::map<std::string, PCB_IO_MGR::PCB_FILE_T> pluginTypeMap = {
    { "kicad", PCB_IO_MGR::KICAD_SEXP },
    { "legacy", PCB_IO_MGR::LEGACY },
    { "allegro", PCB_IO_MGR::ALLEGRO },
    { "altium", PCB_IO_MGR::ALTIUM_DESIGNER },
    { "cadstar", PCB_IO_MGR::CADSTAR_PCB_ARCHIVE },
    { "eagle", PCB_IO_MGR::EAGLE },
    { "easyeda", PCB_IO_MGR::EASYEDA },
    { "easyedapro", PCB_IO_MGR::EASYEDAPRO },
    { "fabmaster", PCB_IO_MGR::FABMASTER },
    { "geda", PCB_IO_MGR::GEDA_PCB },
    { "pads", PCB_IO_MGR::PADS },
    { "pcad", PCB_IO_MGR::PCAD },
    { "solidworks", PCB_IO_MGR::SOLIDWORKS_PCB },
    // { "ipc2581", PCB_IO_MGR::IPC2581 }, // readers
    // { "odbpp", PCB_IO_MGR::ODBPP },
};


static PCB_IO_MGR::PCB_FILE_T FindPluginTypeFromParams( const wxString& aExplicitPlugin, const wxString& aPath )
{
    if( aExplicitPlugin == "auto" )
    {
        // Try to guess the plugin type from the first file
        return PCB_IO_MGR::FindPluginTypeFromBoardPath( aPath );
    }

    auto pluginIt = pluginTypeMap.find( aExplicitPlugin.ToStdString() );
    if( pluginIt == pluginTypeMap.end() )
    {
        return PCB_IO_MGR::FILE_TYPE_NONE;
    }
    return pluginIt->second;
}


int pcb_parser_main_func( int argc, char** argv )
{
#ifdef __AFL_COMPILER
    __AFL_INIT();
#endif

    wxMessageOutput::Set( new wxMessageOutputStderr );
    wxCmdLineParser cl_parser( argc, argv );
    cl_parser.SetDesc( g_cmdLineDesc );
    cl_parser.AddUsageText( _( "This program parses PCB files, either from the stdin stream or "
                               "from the given filenames. This can be used either for standalone "
                               "testing of the parser or for fuzz testing." ) );

    int cmd_parsed_ok = cl_parser.Parse();
    if( cmd_parsed_ok != 0 )
    {
        // Help and invalid input both stop here
        return ( cmd_parsed_ok == -1 ) ? KI_TEST::RET_CODES::OK : KI_TEST::RET_CODES::BAD_CMDLINE;
    }

    const bool   verbose = cl_parser.Found( "verbose" );

    if( cl_parser.Found( "list-plugins" ) )
    {
        for( const auto& [name, type] : pluginTypeMap )
        {
            std::cout << name << std::endl;
        }
        std::cout << "auto" << std::endl;

        return KI_TEST::RET_CODES::OK;
    }

    bool         ok = true;
    const size_t file_count = cl_parser.GetParamCount();

    wxString plugin( "auto" );
    cl_parser.Found( "plugin", &plugin );

    long aflLoopCount = 1;
    cl_parser.Found( "loop", &aflLoopCount );

    if( file_count == 0 && plugin == "auto" )
    {
        std::cerr << "When parsing from stdin, you must specify the plugin type with -p" << std::endl;
        return KI_TEST::RET_CODES::BAD_CMDLINE;
    }

    const PCB_IO_MGR::PCB_FILE_T pluginType =
            FindPluginTypeFromParams( plugin, file_count > 0 ? cl_parser.GetParam( 0 ) : wxString( "" ) );

    if( pluginType == PCB_IO_MGR::FILE_TYPE_NONE )
    {
        std::cerr << fmt::format( "Failed to determine plugin type for input using plugin {}", plugin.ToStdString() )
                  << std::endl;
        return KI_TEST::RET_CODES::BAD_CMDLINE;
    }

    if( verbose )
    {
        std::cout << "Using plugin type: " << PCB_IO_MGR::ShowType( pluginType ) << std::endl;
    }

    PCB_PARSE_RUNNER runner( pluginType, verbose );

    std::vector<std::string> failedFiles;

    if( file_count == 0 )
    {
        // Parse the file provided on stdin - used by AFL to drive the
        // program
#ifdef __AFL_COMPILER
        while( __AFL_LOOP( aflLoopCount ) )
#endif
        {
            ok = runner.Parse( std::cin );
        }
    }
    else
    {
        // Parse 'n' files given on the command line
        // (this is useful for input minimisation (e.g. afl-tmin) as
        // well as manual testing
        for( size_t i = 0; i < file_count; i++ )
        {
            const wxString filename = cl_parser.GetParam( i );

            if( verbose )
                std::cout << fmt::format( "Parsing: {}", filename.ToStdString() ) << std::endl;

            if( !runner.Parse( filename ) )
            {
                ok = false;
                failedFiles.push_back( filename.ToStdString() );
            }
        }
    }

    for( const auto& failedFile : failedFiles )
    {
        std::cerr << fmt::format( "Failed to parse: {}", failedFile ) << std::endl;
    }

    if( !ok )
        return PARSER_RET_CODES::PARSE_FAILED;

    return KI_TEST::RET_CODES::OK;
}


static bool registered = UTILITY_REGISTRY::Register( { "pcb_parser",
                                                       "Parse a PCB file",
                                                       pcb_parser_main_func } );

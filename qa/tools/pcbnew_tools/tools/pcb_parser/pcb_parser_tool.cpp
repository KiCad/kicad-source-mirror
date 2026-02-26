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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <qa_utils/utility_registry.h>

#include <cstdio>
#include <string>

#include <wx/cmdline.h>
#include <wx/msgout.h>

#include <fmt/format.h>

#include <board.h>
#include <board_item.h>
#include <common.h>
#include <core/profile.h>
#include <richio.h>

#include <pcb_io/allegro/pcb_io_allegro.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr.h>
#include <pcb_io/kicad_sexpr/pcb_io_kicad_sexpr_parser.h>

#include <qa_utils/stdstream_line_reader.h>


using PARSE_DURATION = std::chrono::microseconds;


/**
 * In order to support fuzz testing, we need to be able to parse from stdin.
 * The PCB_IO interface is designed to parse from a file, so this class is a simple
 * wrapper that needs to be implemented to wrap a plugin's core parser function.
 */
class STREAM_PARSER
{
public:
    virtual ~STREAM_PARSER() = default;

    /**
     * Take some input stream and prepare it for parsing.  This may involve reading
     * the whole stream into memory, or it may involve setting up some kind of streaming
     * reader.
     *
     * @throw IO_ERROR if there is a problem reading from the stream
     */
    virtual void PrepareStream( std::istream& aStream ) = 0;

    /**
     * Actually perform the parsing and return a BOARD_ITEM if successful, or nullptr if not.
     *
     * @throw IO_ERROR if there is a parse failure
     */
    virtual std::unique_ptr<BOARD_ITEM> Parse() = 0;
};


class SEXPR_STREAM_PARSER : public STREAM_PARSER
{
public:
    void PrepareStream( std::istream& aStream ) override
    {
        m_reader.SetStream( aStream );

        m_parser = std::make_unique<PCB_IO_KICAD_SEXPR_PARSER>( &m_reader, nullptr, nullptr );
    }

    std::unique_ptr<BOARD_ITEM> Parse() override
    {
        return std::unique_ptr<BOARD_ITEM>{ m_parser->Parse() };
    }

private:
    STDISTREAM_LINE_READER                     m_reader;
    std::unique_ptr<PCB_IO_KICAD_SEXPR_PARSER> m_parser;
};


class ALLEGRO_BRD_STREAM_PARSER : public STREAM_PARSER
{
public:
    void PrepareStream( std::istream& aStream ) override
    {
        // Allegro parser expects to mmap the stream, so we need to
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


enum class PLUGIN_TYPE
{
    KICAD_SEXPR,
    ALLEGRO
};


/**
 * Parse a PCB or footprint file from the given input stream
 *
 * @param aStream the input stream to read from
 * @return success, duration (in us)
 */
bool parse( std::istream& aStream, PLUGIN_TYPE aPluginType, bool aVerbose )
{
    std::unique_ptr<STREAM_PARSER> parser;

    switch( aPluginType )
    {
    case PLUGIN_TYPE::KICAD_SEXPR:
        parser = std::make_unique<SEXPR_STREAM_PARSER>();
        break;
    case PLUGIN_TYPE::ALLEGRO:
        parser = std::make_unique<ALLEGRO_BRD_STREAM_PARSER>();
        break;
    }

    try
    {
        parser->PrepareStream( aStream );
    }
    catch( const IO_ERROR& e )
    {
        if( aVerbose )
        {
            std::cerr << fmt::format( "Error preparing stream: {}", e.What().ToStdString() ) << std::endl;
        }
        return false;
    }

    std::unique_ptr<BOARD_ITEM> board = nullptr;

    PARSE_DURATION duration{};

    try
    {
        PROF_TIMER timer;
        board = parser->Parse();

        duration = timer.SinceStart<PARSE_DURATION>();
    }
    catch( const IO_ERROR& )
    {
    }

    if( aVerbose )
    {
        std::cout << fmt::format( "Took: {}us", duration.count() ) << std::endl;

        if( !board )
        {
            std::cout << "Parsing failed" << std::endl;
        }
        else
        {
            std::cout << fmt::format( "  {} nets", board->GetBoard()->GetNetCount() ) << std::endl;
        }
    }

    return board != nullptr;
}


static const wxCmdLineEntryDesc g_cmdLineDesc[] = {
    { wxCMD_LINE_SWITCH, "h", "help", _( "displays help on the command line parameters" ).mb_str(),
            wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
    { wxCMD_LINE_SWITCH, "v", "verbose", _( "print parsing information" ).mb_str() },
    { wxCMD_LINE_OPTION, "p", "plugin", _( "parser plugin to use (kicad, allegro)" ).mb_str(),
            wxCMD_LINE_VAL_STRING, wxCMD_LINE_OPTION_MANDATORY },
    { wxCMD_LINE_OPTION, "l", "loop", _( "number of times to loop when parsing from stdin (for AFL)" ).mb_str(),
            wxCMD_LINE_VAL_NUMBER },
    { wxCMD_LINE_PARAM, nullptr, nullptr, _( "input file" ).mb_str(), wxCMD_LINE_VAL_STRING,
            wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE },
    { wxCMD_LINE_NONE }
};


enum PARSER_RET_CODES
{
    PARSE_FAILED = KI_TEST::RET_CODES::TOOL_SPECIFIC,
};


static const std::map<std::string, PLUGIN_TYPE> pluginTypeMap = {
    { "kicad", PLUGIN_TYPE::KICAD_SEXPR },
    { "allegro", PLUGIN_TYPE::ALLEGRO },
};


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
    bool         ok = true;
    const size_t file_count = cl_parser.GetParamCount();

    wxString plugin;
    cl_parser.Found( "plugin", &plugin );

    auto pluginIt = pluginTypeMap.find( plugin.ToStdString() );
    if( pluginIt == pluginTypeMap.end() )
    {
        std::cerr << fmt::format( "Unknown plugin: {}", plugin.ToStdString() ) << std::endl;
        return KI_TEST::RET_CODES::BAD_CMDLINE;
    }
    const PLUGIN_TYPE pluginType = pluginIt->second;

    long              aflLoopCount = 1;
    cl_parser.Found( "loop", &aflLoopCount );

    if( file_count == 0 )
    {
        // Parse the file provided on stdin - used by AFL to drive the
        // program
#ifdef __AFL_COMPILER
        while( __AFL_LOOP( aflLoopCount ) )
#endif
        {
            ok = parse( std::cin, pluginType, verbose );
        }
    }
    else
    {
        // Parse 'n' files given on the command line
        // (this is useful for input minimisation (e.g. afl-tmin) as
        // well as manual testing
        for( unsigned i = 0; i < file_count; i++ )
        {
            const std::string filename = cl_parser.GetParam( i ).ToStdString();

            if( verbose )
                std::cout << fmt::format( "Parsing: {}", filename ) << std::endl;

            std::ifstream fin;
            fin.open( filename );

            ok = ok && parse( fin, pluginType, verbose );
        }
    }

    if( !ok )
        return PARSER_RET_CODES::PARSE_FAILED;

    return KI_TEST::RET_CODES::OK;
}


static bool registered = UTILITY_REGISTRY::Register( { "pcb_parser",
                                                       "Parse a PCB file",
                                                       pcb_parser_main_func } );

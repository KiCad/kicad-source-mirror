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
#include <iostream>
#include <set>
#include <string>
#include <vector>

#include <fmt/format.h>
#include <wx/cmdline.h>
#include <wx/msgout.h>

#include <common.h>
#include <core/profile.h>
#include <ki_exception.h>
#include <libraries/library_table.h>
#include <lib_symbol.h>
#include <paths.h>
#include <richio.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr.h>
#include <sch_io/kicad_sexpr/sch_io_kicad_sexpr_parser.h>
#include <settings/common_settings.h>
#include <settings/settings_manager.h>
#include <symbol_library_common.h>
#include <wx_filename.h>


using PARSE_DURATION = std::chrono::microseconds;


/**
 * Load the KiCad common settings and set any configured environment variables
 * (e.g. KICAD10_SYMBOL_DIR) so that library table URI references like
 * ${KICAD10_SYMBOL_DIR}/Device.kicad_sym can be resolved.
 */
static void loadKiCadEnvVars()
{
    SETTINGS_MANAGER mgr;
    COMMON_SETTINGS* cfg = mgr.GetCommonSettings();

    if( !cfg )
        return;

    // Trigger loading of the settings from disk
    mgr.Load( cfg );

    for( const auto& [name, item] : cfg->m_Env.vars )
    {
        // Don't overwrite variables that are already defined in the system environment
        if( item.GetDefinedExternally() )
            continue;

        wxString existing;

        if( !wxGetEnv( name, &existing ) )
            wxSetEnv( name, item.GetValue() );
    }
}


/**
 * Parse a single library file path and return the number of symbols loaded.
 *
 * @return the number of symbols found, or -1 on failure.
 */
static int parseLibraryFile( const wxString& aPath, bool aVerbose )
{
    // The s-expression library parser requires an absolute path.
    wxFileName fn( aPath );
    fn.MakeAbsolute();
    wxString absPath = fn.GetFullPath();

    if( aVerbose )
    {
        std::cout << fmt::format( "Parsing library: {}", absPath.ToStdString() ) << std::endl;
    }

    PROF_TIMER               timer;
    SCH_IO_KICAD_SEXPR       io;
    std::vector<LIB_SYMBOL*> symbols;

    try
    {
        io.EnumerateSymbolLib( symbols, absPath );
    }
    catch( const IO_ERROR& e )
    {
        if( aVerbose )
        {
            auto duration = timer.SinceStart<PARSE_DURATION>();
            std::cerr << fmt::format( "Failed to parse library '{}' after {} us: {}", absPath.ToStdString(),
                                      duration.count(), e.What().ToStdString() )
                      << std::endl;
        }
        else
        {
            std::cerr << fmt::format( "Failed to parse library '{}': {}", absPath.ToStdString(),
                                      e.What().ToStdString() )
                      << std::endl;
        }
        return -1;
    }

    if( aVerbose )
    {
        auto duration = timer.SinceStart<PARSE_DURATION>();
        std::cout << fmt::format( "  {} symbols", symbols.size() ) << std::endl;
        std::cout << fmt::format( "  Took: {} us", duration.count() ) << std::endl;
    }

    return static_cast<int>( symbols.size() );
}


/**
 * Parse symbol library content from stdin (for fuzzing).
 *
 * @return true if parsing succeeded.
 */
static bool parseStdin()
{
    // Read all of stdin into a string
    std::string content( ( std::istreambuf_iterator<char>( std::cin ) ), std::istreambuf_iterator<char>() );

    if( content.empty() )
        return true; // empty input is not a parse error (important for fuzzing)

    try
    {
        LIB_SYMBOL_MAP            symbolMap;
        STRING_LINE_READER        reader( content, wxS( "<stdin>" ) );
        SCH_IO_KICAD_SEXPR_PARSER parser( &reader );

        parser.ParseLib( symbolMap );
    }
    catch( const IO_ERROR& )
    {
        return false;
    }

    return true;
}


/**
 * Load libraries from a symbol library table file.
 *
 * @param aVisited set of already-visited table paths
 * @return the number of libraries successfully parsed, or -1 if the table
 *         itself could not be loaded.
 */
static int parseLibTable( const wxString& aTablePath, bool aVerbose, std::set<wxString>& aVisited )
{
    // Resolve symlinks so that the visited-set key is canonical
    wxFileName tableFn( aTablePath );
    tableFn.MakeAbsolute();
    WX_FILENAME::ResolvePossibleSymlinks( tableFn );
    wxString canonicalPath = tableFn.GetFullPath();

    // Prevent infinite recursion through nested tables
    if( aVisited.count( canonicalPath ) )
        return 0;

    aVisited.insert( canonicalPath );

    LIBRARY_TABLE table( tableFn, LIBRARY_TABLE_SCOPE::PROJECT, LIBRARY_TABLE_TYPE::SYMBOL );

    if( !table.IsOk() )
    {
        std::cerr << fmt::format( "Failed to load library table '{}': {}", canonicalPath.ToStdString(),
                                  table.ErrorDescription().ToStdString() )
                  << std::endl;
        return -1;
    }

    int  okCount = 0;
    bool hadRows = false;

    for( const auto& row : table.Rows() )
    {
        if( row.Disabled() || row.Hidden() )
            continue;

        hadRows = true;

        wxString uri = row.URI();

        // Expand environment variables in the URI
        uri = ExpandEnvVarSubstitutions( uri, nullptr );

        // Resolve URIs relative to the table file's directory
        {
            wxFileName uriFn( uri );
            uriFn.MakeAbsolute( tableFn.GetPath() );
            uri = uriFn.GetFullPath();
        }

        if( aVerbose )
        {
            std::cout << fmt::format( "Parsing library '{}': {}", row.Nickname().ToStdString(), uri.ToStdString() )
                      << std::endl;
        }

        // A row with type "Table" points to a nested sym-lib-table
        if( row.Type() == LIBRARY_TABLE_ROW::TABLE_TYPE_NAME )
        {
            int nestedCount = parseLibTable( uri, aVerbose, aVisited );

            if( nestedCount >= 0 )
                okCount += nestedCount;
        }
        else if( row.Type() == wxS( "KiCad" ) )
        {
            int count = parseLibraryFile( uri, aVerbose );

            if( count >= 0 )
                okCount++;
        }
        else
        {
            // Skip unsupported library types (Legacy, Database, HTTP, etc.)
            if( aVerbose )
            {
                std::cerr << fmt::format( "Skipping unsupported library type '{}' for '{}'", row.Type().ToStdString(),
                                          row.Nickname().ToStdString() )
                          << std::endl;
            }
        }
    }

    if( !hadRows )
        return 0;

    return okCount;
}


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
            nullptr,
            "lib-table",
            _( "path to a symbol library table file" ).mb_str(),
            wxCMD_LINE_VAL_STRING,
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
            _( "library file" ).mb_str(),
            wxCMD_LINE_VAL_STRING,
            wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE,
    },
    {
            wxCMD_LINE_NONE,
    }
};


enum LIB_PARSER_RET_CODES
{
    PARSE_FAILED = KI_TEST::RET_CODES::TOOL_SPECIFIC,
};


int lib_parser_main_func( int argc, char** argv )
{
#ifdef __AFL_COMPILER
    __AFL_INIT();
#endif

    wxMessageOutput::Set( new wxMessageOutputStderr );
    wxCmdLineParser cl_parser( argc, argv );
    cl_parser.SetDesc( g_cmdLineDesc );
    cl_parser.AddUsageText( "This program parses schematic symbol library files, either from the stdin "
                            "stream, from individual library files, or from libraries listed in a symbol "
                            "library table. This can be used for profiling, fuzz testing, etc.\n" );

    cl_parser.AddUsageText( "If no library files or library table are specified, the program will read from stdin. "
                            "This is useful for fuzz testing, for example.\n" );

    cl_parser.AddUsageText( "Fuzzing with AFL:\n" );

    cl_parser.AddUsageText( "  afl-fuzz -i <input_dir> -o <output_dir> -- qa_eeschema_tools lib_parser --loop 1000\n" );

    cl_parser.AddUsageText( "Some fuzzing seeds for the -i option can be found in the KiCad source tree under"
                            " qa/data/fuzzing/kicad_sym\n" );

    cl_parser.AddUsageText( "See the fuzzing documentation in dev-docs for more information." );

    int cmd_parsed_ok = cl_parser.Parse();

    if( cmd_parsed_ok != 0 )
    {
        // Help and invalid input both stop here
        return ( cmd_parsed_ok == -1 ) ? KI_TEST::RET_CODES::OK : KI_TEST::RET_CODES::BAD_CMDLINE;
    }

    // Load KiCad-configured environment variables so that library table URI
    // references like ${KICAD10_SYMBOL_DIR} can be resolved.
    loadKiCadEnvVars();

    const bool verbose = cl_parser.Found( "verbose" );
    bool       ok = true;

    // Collect library paths from positional arguments
    std::vector<wxString> libPaths;

    for( size_t i = 0; i < cl_parser.GetParamCount(); i++ )
        libPaths.push_back( cl_parser.GetParam( i ) );

    // Handle --lib-table
    wxString tablePath;

    if( cl_parser.Found( "lib-table", &tablePath ) )
    {
        std::set<wxString> visited;
        int                count = parseLibTable( tablePath, verbose, visited );

        if( count < 0 )
            return LIB_PARSER_RET_CODES::PARSE_FAILED;

        if( count == 0 && !verbose )
        {
            // Only print this when not in verbose mode, since in verbose mode
            // each failed library already printed its own error.
            std::cerr << fmt::format( "No libraries successfully parsed from table '{}'", tablePath.ToStdString() )
                      << std::endl;
        }
    }

    long aflLoopCount = 1;
    cl_parser.Found( "loop", &aflLoopCount );

    if( libPaths.empty() )
    {
        // If --lib-table was specified, we're done (no stdin fallback).
        // If nothing was specified, parse from stdin (for fuzzing, probably).
        if( cl_parser.Found( "lib-table" ) )
            return KI_TEST::RET_CODES::OK;

#ifdef __AFL_COMPILER
        while( __AFL_LOOP( aflLoopCount ) )
#endif
        {
            ok = parseStdin();
        }
    }
    else
    {
        // Parse each library file given on the command line
        for( const auto& path : libPaths )
        {
            int count = parseLibraryFile( path, verbose );

            if( count < 0 )
                ok = false;
        }
    }

    if( !ok )
        return LIB_PARSER_RET_CODES::PARSE_FAILED;

    return KI_TEST::RET_CODES::OK;
}


static bool registered = UTILITY_REGISTRY::Register( {
        "lib_parser",
        "Parse schematic symbol library files",
        lib_parser_main_func,
} );

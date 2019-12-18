/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see CHANGELOG.TXT for contributors.
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


#include <common.h>
#include <profile.h>

#include <class_module.h>
#include <fp_lib_table.h>
#include <kicad_plugin.h>

#include <qa_utils/utility_registry.h>

#include <wx/cmdline.h>

#include <cstdio>
#include <string>
#include <thread>


using PARSE_DURATION = std::chrono::microseconds;


static const wxCmdLineEntryDesc g_cmdLineDesc[] = {
    { wxCMD_LINE_SWITCH, "h", "help", _( "displays help on the command line parameters" ).mb_str(),
            wxCMD_LINE_VAL_NONE, wxCMD_LINE_OPTION_HELP },
    { wxCMD_LINE_SWITCH, "v", "verbose", _( "print loading information" ).mb_str() },
    { wxCMD_LINE_SWITCH, "C", "concurrent", _( "load libraries concurrently" ).mb_str() },
    { wxCMD_LINE_OPTION, "t", "table",
            _( "load libraries by nickname from a given table, rather than by path" ).mb_str(),
            wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_OPTION, "r", "repeat", _( "times to load each library (default=1)" ).mb_str(),
            wxCMD_LINE_VAL_NUMBER, wxCMD_LINE_PARAM_OPTIONAL },
    { wxCMD_LINE_PARAM, nullptr, nullptr, _( "input libraries" ).mb_str(), wxCMD_LINE_VAL_STRING,
            wxCMD_LINE_PARAM_OPTIONAL | wxCMD_LINE_PARAM_MULTIPLE },
    { wxCMD_LINE_NONE }
};


enum LOADER_RET_CODES
{
    PLUGIN_NOT_FOUND = KI_TEST::RET_CODES::TOOL_SPECIFIC,
};


struct LIB_LOAD_THREAD_CONTEXT
{
    // Library plugin - do not own this
    PLUGIN* m_plugin;

    // The lib path to load FPs from
    wxString m_lib_path;

    // How many times to repeatedly load FPs
    long m_reps;
};


struct TABLE_LOAD_THREAD_CONTEXT
{
    // Library plugin - do not own this
    FP_LIB_TABLE& m_table;

    // The lib nickname to load FPs from
    wxString m_nickname;

    // How many times to repeatedly load FPs
    long m_reps;
};


template <typename CTX_T, typename CALLBACK_T>
void load_libraries( std::vector<CTX_T>& aContexts, CALLBACK_T aCallback, bool aConcurrently )
{
    std::vector<std::thread> threads;

    for( auto& ctx : aContexts )
    {
        if( aConcurrently )
        {
            std::thread thread( aCallback, std::ref( ctx ) );

            // keep the thread so we can join it
            threads.push_back( std::move( thread ) );
        }
        else
        {
            // not concurrent
            // we could also force each thread to join in this loop,
            // but this way gives simpler stacks for debugging
            aCallback( ctx );
        }
    }

    // join all the threads
    for( auto& thread : threads )
    {
        if( thread.joinable() )
            thread.join();
    }
}


void lib_load_thread_fn( LIB_LOAD_THREAD_CONTEXT& aContext )
{
    for( int i = 0; i < aContext.m_reps; ++i )
    {
        std::cout << "Loading from: " << aContext.m_lib_path << std::endl;

        wxArrayString footprint_names;
        aContext.m_plugin->FootprintEnumerate(
                footprint_names, aContext.m_lib_path, true, nullptr );

        if( footprint_names.size() == 0 )
        {
            std::cerr << "Library is empty: " << aContext.m_lib_path << std::endl;
        }

        for( const auto fp_name : footprint_names )
        {
            std::unique_ptr<MODULE> mod{ aContext.m_plugin->FootprintLoad(
                    aContext.m_lib_path, fp_name, nullptr ) };

            if( !mod )
            {
                // This is odd, as the FP was reported by FootprintEnumerate
                std::cerr << "Module not found: " << fp_name << std::endl;
            }
            else
            {
                // std::cout << "Found module" << std::endl;
            }
        }
    }
}


int do_raw_library_loads( const wxArrayString& aLibPaths, long aReps, bool aConcurrently )
{
    // Manually loading plugins, use this to manage the lifetimes
    std::vector<PLUGIN::RELEASER> releasers;

    std::vector<LIB_LOAD_THREAD_CONTEXT> contexts;

    for( const wxString& lib_path : aLibPaths )
    {
        const IO_MGR::PCB_FILE_T io_type = IO_MGR::GuessPluginTypeFromLibPath( lib_path );

        PLUGIN::RELEASER plugin( IO_MGR::PluginFind( io_type ) );

        if( !plugin )
        {
            std::cerr << "Plugin not found for library: " << lib_path;
            return PLUGIN_NOT_FOUND;
        }

        contexts.push_back( {
                (PLUGIN*) plugin,
                lib_path,
                aReps,
        } );

        releasers.push_back( { std::move( plugin ) } );
    }

    load_libraries( contexts, lib_load_thread_fn, aConcurrently );

    return KI_TEST::RET_CODES::OK;
}


void table_load_thread_fn( TABLE_LOAD_THREAD_CONTEXT& aContext )
{
    for( int i = 0; i < aContext.m_reps; ++i )
    {
        std::cout << "Loading from: " << aContext.m_nickname << std::endl;

        wxArrayString footprint_names;
        aContext.m_table.FootprintEnumerate( footprint_names, aContext.m_nickname, true );

        if( footprint_names.size() == 0 )
        {
            std::cerr << "Library is empty: " << aContext.m_nickname << std::endl;
        }

        for( const auto fp_name : footprint_names )
        {
            std::unique_ptr<MODULE> mod{ aContext.m_table.FootprintLoad(
                    aContext.m_nickname, fp_name ) };

            if( !mod )
            {
                // This is odd, as the FP was reported by FootprintEnumerate
                std::cerr << "Module not found: " << fp_name << std::endl;
            }
            else
            {
                // std::cout << "Found module" << std::endl;
            }
        }
    }
}


int do_table_library_loads( const wxString& aTableFile, const wxArrayString& aLibNickames,
        long aReps, bool aConcurrently )
{
    FP_LIB_TABLE fp_lib_table( nullptr );
    fp_lib_table.Load( aTableFile );

    std::vector<TABLE_LOAD_THREAD_CONTEXT> contexts;

    for( const wxString& lib_nickname : aLibNickames )
    {
        contexts.push_back( {
                fp_lib_table,
                lib_nickname,
                aReps,
        } );
    }

    load_libraries( contexts, table_load_thread_fn, aConcurrently );

    return KI_TEST::RET_CODES::OK;
}


int lib_loader_main_func( int argc, char** argv )
{
    wxMessageOutput::Set( new wxMessageOutputStderr );
    wxCmdLineParser cl_parser( argc, argv );
    cl_parser.SetDesc( g_cmdLineDesc );
    cl_parser.AddUsageText(
            _( "This program loads footprints from libraries. This allows "
               "profiling, benchmarking and debugging of library loading. "
               "For loading of individual footprints, rather than whole "
               "libraries, use the pcb_parser tool.\n\n"
               "You can either load directly from the libraries on disk, "
               "or you can load by nickname from a library table (in which "
               "case, you may need to set env vars)" ) );

    int cmd_parsed_ok = cl_parser.Parse();
    if( cmd_parsed_ok != 0 )
    {
        // Help and invalid input both stop here
        return ( cmd_parsed_ok == -1 ) ? KI_TEST::RET_CODES::OK : KI_TEST::RET_CODES::BAD_CMDLINE;
    }

    wxString     lib_table_file;
    cl_parser.Found( "table", &lib_table_file );

    long reps = 1;
    cl_parser.Found( "repeat", &reps );

    wxArrayString params;
    for( unsigned i = 0; i < cl_parser.GetParamCount(); ++i )
    {
        params.push_back( cl_parser.GetParam( i ) );
    }

    const bool concurrently = cl_parser.Found( "concurrent" );

    int ret;

    if( lib_table_file.size() == 0 )
    {
        ret = do_raw_library_loads( params, reps, concurrently );
    }
    else
    {
        ret = do_table_library_loads( lib_table_file, params, reps, concurrently );
    }


    return ret;
}


static bool registered = UTILITY_REGISTRY::Register( {
        "lib_loader",
        "Load libraries",
        lib_loader_main_func,
} );
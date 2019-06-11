/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see CHANGELOG.TXT for contributors.
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

#include "drc_tool.h"

#include <cstdio>
#include <string>

#include <common.h>
#include <profile.h>

#include <wx/cmdline.h>

#include <pcbnew_utils/board_file_utils.h>

// DRC
#include <drc/courtyard_overlap.h>
#include <drc/drc_marker_factory.h>

#include <qa_utils/stdstream_line_reader.h>


using DRC_DURATION = std::chrono::microseconds;

/**
 * DRC runner: provides a simple framework to run some DRC checks on #BOARDS.
 * The DRC_RUNNER can be set up as needed to instantiate a #DRC_PROVIDER to
 * perform the desired DRC on the #BOARD and provide some basic information
 * about what happened.
 */
class DRC_RUNNER
{
public:
    /**
     * How the DRC runner behaves (e.g. what is printed and how)
     */
    struct EXECUTION_CONTEXT
    {
        bool m_verbose;
        bool m_print_times;
        bool m_print_markers;
    };

    DRC_RUNNER( const EXECUTION_CONTEXT& aExecCtx ) : m_exec_context( aExecCtx )
    {
    }

    void Execute( BOARD& aBoard )
    {
        if( m_exec_context.m_verbose )
        {
            std::cout << "Running DRC check: " << getRunnerIntro() << std::endl;
        }

        aBoard.SetDesignSettings( getDesignSettings() );

        std::vector<std::unique_ptr<MARKER_PCB>> markers;

        auto marker_handler = [&]( MARKER_PCB* aMarker ) {
            markers.push_back( std::unique_ptr<MARKER_PCB>( aMarker ) );
        };

        std::unique_ptr<DRC_PROVIDER> drc_prov = createDrcProvider( aBoard, marker_handler );

        DRC_DURATION duration;
        {
            SCOPED_PROF_COUNTER<DRC_DURATION> timer( duration );
            drc_prov->RunDRC( aBoard );
        }

        // report results
        if( m_exec_context.m_print_times )
            reportDuration( duration );

        if( m_exec_context.m_print_markers )
            reportMarkers( markers );
    }

protected:
    const DRC_MARKER_FACTORY& getMarkerFactory() const
    {
        return m_marker_factory;
    }

private:
    /**
     * Get the introduction text for this DRC runner
     */
    virtual std::string getRunnerIntro() const = 0;

    /**
     * Get suitable design settings for this DRC runner
     */
    virtual BOARD_DESIGN_SETTINGS getDesignSettings() const = 0;

    virtual std::unique_ptr<DRC_PROVIDER> createDrcProvider(
            BOARD& aBoard, DRC_PROVIDER::MARKER_HANDLER aHandler ) = 0;

    void reportDuration( const DRC_DURATION& aDuration ) const
    {
        std::cout << "Took: " << aDuration.count() << "us" << std::endl;
    }

    void reportMarkers( const std::vector<std::unique_ptr<MARKER_PCB>>& aMarkers ) const
    {
        std::cout << "DRC markers: " << aMarkers.size() << std::endl;

        int index = 0;
        for( const auto& m : aMarkers )
        {
            std::cout << index++ << ": " << m->GetReporter().ShowReport( EDA_UNITS_T::MILLIMETRES );
        }

        if( index )
        {
            std::cout << std::endl;
        }
    }

    const EXECUTION_CONTEXT m_exec_context;
    DRC_MARKER_FACTORY      m_marker_factory;
};


/**
 * DRC runner to run only DRC courtyard-overlap checks
 */
class DRC_COURTYARD_OVERLAP_RUNNER : public DRC_RUNNER
{
public:
    DRC_COURTYARD_OVERLAP_RUNNER( const EXECUTION_CONTEXT& aCtx ) : DRC_RUNNER( aCtx )
    {
    }

private:
    std::string getRunnerIntro() const override
    {
        return "Courtyard overlap";
    }

    BOARD_DESIGN_SETTINGS getDesignSettings() const override
    {
        BOARD_DESIGN_SETTINGS des_settings;
        des_settings.m_RequireCourtyards = false;
        des_settings.m_ProhibitOverlappingCourtyards = true;

        return des_settings;
    }

    std::unique_ptr<DRC_PROVIDER> createDrcProvider(
            BOARD& aBoard, DRC_PROVIDER::MARKER_HANDLER aHandler ) override
    {
        return std::make_unique<DRC_COURTYARD_OVERLAP>( getMarkerFactory(), aHandler );
    }
};


/**
 * DRC runner to run only DRC courtyard-missing checks
 */
class DRC_COURTYARD_MISSING_RUNNER : public DRC_RUNNER
{
public:
    DRC_COURTYARD_MISSING_RUNNER( const EXECUTION_CONTEXT& aCtx ) : DRC_RUNNER( aCtx )
    {
    }

private:
    std::string getRunnerIntro() const override
    {
        return "Courtyard missing";
    }

    BOARD_DESIGN_SETTINGS getDesignSettings() const override
    {
        BOARD_DESIGN_SETTINGS des_settings;
        des_settings.m_RequireCourtyards = true;
        des_settings.m_ProhibitOverlappingCourtyards = false;

        return des_settings;
    }

    std::unique_ptr<DRC_PROVIDER> createDrcProvider(
            BOARD& aBoard, DRC_PROVIDER::MARKER_HANDLER aHandler ) override
    {
        return std::make_unique<DRC_COURTYARD_OVERLAP>( getMarkerFactory(), aHandler );
    }
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
            wxCMD_LINE_SWITCH,
            "t",
            "timings",
            _( "print DRC timings" ).mb_str(),
    },
    {
            wxCMD_LINE_SWITCH,
            "m",
            "print-markers",
            _( "print DRC marker information" ).mb_str(),
    },
    {
            wxCMD_LINE_SWITCH,
            "A",
            "all-checks",
            _( "perform all available DRC checks" ).mb_str(),
    },
    {
            wxCMD_LINE_SWITCH,
            "C",
            "courtyard-overlap",
            _( "perform courtyard-overlap (and malformation) checking" ).mb_str(),
    },
    {
            wxCMD_LINE_SWITCH,
            "c",
            "courtyard-missing",
            _( "perform courtyard-missing checking" ).mb_str(),
    },
    {
            wxCMD_LINE_PARAM,
            nullptr,
            nullptr,
            _( "input file" ).mb_str(),
            wxCMD_LINE_VAL_STRING,
            wxCMD_LINE_PARAM_OPTIONAL,
    },
    { wxCMD_LINE_NONE }
};

/**
 * Tool=specific return codes
 */
enum PARSER_RET_CODES
{
    PARSE_FAILED = KI_TEST::RET_CODES::TOOL_SPECIFIC,
};


int drc_main_func( int argc, char** argv )
{
#ifdef __AFL_COMPILER
    __AFL_INIT();
#endif

    wxMessageOutput::Set( new wxMessageOutputStderr );
    wxCmdLineParser cl_parser( argc, argv );
    cl_parser.SetDesc( g_cmdLineDesc );
    cl_parser.AddUsageText(
            _( "This program runs DRC tools on given PCB files. "
               "This can be used for debugging, fuzz testing or development, etc." ) );

    int cmd_parsed_ok = cl_parser.Parse();
    if( cmd_parsed_ok != 0 )
    {
        // Help and invalid input both stop here
        return ( cmd_parsed_ok == -1 ) ? KI_TEST::RET_CODES::OK : KI_TEST::RET_CODES::BAD_CMDLINE;
    }

    const bool verbose = cl_parser.Found( "verbose" );

    std::string filename;

    if( cl_parser.GetParamCount() )
    {
        filename = cl_parser.GetParam( 0 ).ToStdString();
    }

    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( filename );

    if( !board )
        return PARSER_RET_CODES::PARSE_FAILED;

    DRC_RUNNER::EXECUTION_CONTEXT exec_context{
        verbose,
        cl_parser.Found( "timings" ),
        cl_parser.Found( "print-markers" ),
    };

    const bool all = cl_parser.Found( "all-checks" );

    // Run the DRC on the board
    if( all || cl_parser.Found( "courtyard-overlap" ) )
    {
        DRC_COURTYARD_OVERLAP_RUNNER runner( exec_context );
        runner.Execute( *board );
    }

    if( all || cl_parser.Found( "courtyard-missing" ) )
    {
        DRC_COURTYARD_MISSING_RUNNER runner( exec_context );
        runner.Execute( *board );
    }

    return KI_TEST::RET_CODES::OK;
}


/*
 * Define the tool interface
 */
KI_TEST::UTILITY_PROGRAM drc_tool = {
    "drc",
    "Run selected DRC function on a PCB",
    drc_main_func,
};
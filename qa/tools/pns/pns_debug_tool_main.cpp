/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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


#include <wx/cmdline.h>

#include <qa_utils/utility_registry.h>
#include <advanced_config.h>

#include "pns_log_file.h"
#include "pns_log_viewer_frame.h"


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
            wxCMD_LINE_OPTION,
            "i",
            "iteration-limit",
            _( "Max number of iterations" ).mb_str(),
            wxCMD_LINE_VAL_NUMBER,
            wxCMD_LINE_PARAM_OPTIONAL,
    },
    {
            wxCMD_LINE_OPTION,
            "s",
            "steps-limit",
            _( "execute log up to steps-limit events" ).mb_str(),
            wxCMD_LINE_VAL_NUMBER,
            wxCMD_LINE_PARAM_OPTIONAL,
    },
    {
            wxCMD_LINE_PARAM,
            "filename",
            "filename",
            _( "log file name (no extensions)" ).mb_str(),
            wxCMD_LINE_VAL_STRING,
            wxCMD_LINE_PARAM_OPTIONAL,
    },
    { wxCMD_LINE_NONE }
};

int replay_main_func( int argc, char* argv[] )
{
    wxCmdLineParser cl_parser( argc, argv );
    cl_parser.SetDesc( g_cmdLineDesc );
    cl_parser.AddUsageText(
            _( "PNS Log (Re)player. Allows to step through the log written by the ROUTER_TOOL "
                "in debug KiCad builds." ) );

    int cmd_parsed_ok = cl_parser.Parse();

    if( cl_parser.Found("help") )
    {
        return 0;
    }

    if( cmd_parsed_ok != 0 )
    {
        printf("P&S Log Replay/Debug tool. For command line options, call %s -h.\n\n", argv[0] );
        return ( cmd_parsed_ok == -1 ) ? KI_TEST::RET_CODES::OK : KI_TEST::RET_CODES::BAD_CMDLINE;
    }


#if 0
    long iter_limit = 256;
    long steps_limit = -1;
    cl_parser.Found( "iteration-limit", &iter_limit );
    cl_parser.Found( "steps-limit", &steps_limit );
#endif

    auto frame = new PNS_LOG_VIEWER_FRAME( nullptr );

    if( cl_parser.GetParamCount() > 0 )
    {
        frame->LoadLogFile( cl_parser.GetParam( 0 ) );
    }

    return 0;
}


static bool registered = UTILITY_REGISTRY::Register( {
        "replay",
        "PNS Log Player",
        replay_main_func,
} );

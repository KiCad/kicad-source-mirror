#include <wx/cmdline.h>

#include <qa_utils/utility_registry.h>

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
            wxCMD_LINE_OPTION_MANDATORY,
    },
    { wxCMD_LINE_NONE }
};

int replay_main_func( int argc, char* argv[] )
{
    wxCmdLineParser cl_parser( argc, argv );
    cl_parser.SetDesc( g_cmdLineDesc );
    cl_parser.AddUsageText( _( "PNS Log (Re)player. Allows to step through the log written by the ROUTER_TOOL "
                "in debug KiCad builds." ) );

    int cmd_parsed_ok = cl_parser.Parse();
    
    if( cl_parser.Found("help") )
    {
        return 0;
    }

    if( cmd_parsed_ok != 0 )
    {
        printf("P&S Log Replay/Debug tool. For command line options, call %s -h.\n\n", argv[0] );
        // Help and invalid input both stop here
        return ( cmd_parsed_ok == -1 ) ? KI_TEST::RET_CODES::OK : KI_TEST::RET_CODES::BAD_CMDLINE;
    }

    wxString filename;

    long iter_limit = 256;
    long steps_limit = -1;
    cl_parser.Found( "iteration-limit", &iter_limit );
    cl_parser.Found( "steps-limit", &steps_limit );
    filename = cl_parser.GetParam(0);

    printf("iters %d steps %d file '%s'\n", iter_limit, steps_limit, (const char *) filename.c_str() );

    auto frame = new PNS_LOG_VIEWER_FRAME( nullptr );

    PNS_LOG_FILE* logFile = new PNS_LOG_FILE;
    logFile->Load( wxFileName( argv[1] ) );
    frame->SetLogFile( logFile );

    return 0;
}


static bool registered = UTILITY_REGISTRY::Register( {
        "replay",
        "PNS Log Player",
        replay_main_func,
} );

#include <qa_utils/utility_registry.h>

#include "pns_log_file.h"
#include "pns_log_viewer_frame.h"

int replay_main_func( int argc, char* argv[] )
{
    auto frame = new PNS_LOG_VIEWER_FRAME( nullptr );

    //  drcCreateTestsProviderClearance();
    //  drcCreateTestsProviderEdgeClearance();

    if( argc >= 2 && std::string( argv[1] ) == "-h" )
    {
        printf( "PNS Log (Re)player. Allows to step through the log written by the ROUTER_TOOL "
                "in debug KiCad builds. " );
        printf( "Requires a board file with UUIDs and a matching log file. Both are written to "
                "/tmp when you press '0' during routing." );
        return 0;
    }

    if( argc < 3 )
    {
        printf( "Expected parameters: log_file.log board_file.dump\n" );
        return 0;
    }

    PNS_LOG_FILE* logFile = new PNS_LOG_FILE;
    logFile->Load( argv[1], argv[2] );

    frame->SetLogFile( logFile );

    return 0;
}


static bool registered = UTILITY_REGISTRY::Register( {
        "replay",
        "PNS Log Player",
        replay_main_func,
} );

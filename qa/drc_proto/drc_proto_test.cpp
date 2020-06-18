#include <string>

#include <common.h>
#include <profile.h>

#include <wx/cmdline.h>

#include <pcbnew_utils/board_file_utils.h>
#include <drc_proto/drc_engine.h>

#include <reporter.h>
#include <widgets/progress_reporter.h>

int main( int argc, char *argv[] )
{
    PROPERTY_MANAGER& propMgr = PROPERTY_MANAGER::Instance();
    propMgr.Rebuild();

    STDOUT_REPORTER msgReporter;

    auto brd =  KI_TEST::ReadBoardFromFileOrStream(argv[1]);

    test::DRC_ENGINE drcEngine( brd.get(), &brd->GetDesignSettings() );

    drcEngine.SetLogReporter( &msgReporter );

    try 
    {
        drcEngine.LoadRules( wxString( argv[2] ) );
    }
    catch( PARSE_ERROR& err )
    {
        printf("Can't load DRC rules: %s\n", (const char*) err.What().c_str() );
    }

    drcEngine.RunTests();

    return 0;
}
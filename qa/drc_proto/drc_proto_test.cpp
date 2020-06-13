#include <string>

#include <common.h>
#include <profile.h>

#include <wx/cmdline.h>

#include <pcbnew_utils/board_file_utils.h>
#include <drc_proto/drc_engine.h>

//#include <qa_utils/utility_registry.h>

int main( int argc, char *argv[] )
{
    auto brd =  KI_TEST::ReadBoardFromFileOrStream(argv[1]);

    test::DRC_ENGINE drcEngine( brd.get(), &brd->GetDesignSettings() );

    try {
    drcEngine.LoadRules( wxString( argv[2] ) );
    } catch( PARSE_ERROR& err )
    {
        printf("Exception %s\n", (const char*) err.What().c_str() );
    }

    drcEngine.RunTests();

    return 0;
}
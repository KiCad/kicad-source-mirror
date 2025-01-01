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

#include <functional>

#include <tool/coroutine.h>

#include <qa_utils/utility_registry.h>

#include <common.h>

#include <wx/cmdline.h>

#include <cstdio>
#include <string>


typedef COROUTINE<int, int> MyCoroutine;

/**
 * A simple harness that counts to a preset value in a couroutine, yielding
 * each value.
 *
 * This is a user-facing version of the "Increment" unit test in the "Coroutine"
 * suite, in qa_common.
 */
class CoroutineExample
{
public:
    CoroutineExample( int aCount ) : m_count( aCount )
    {
    }

    int CountTo( int n )
    {
        for( int i = 1; i <= n; i++ )
        {
            m_cofunc->KiYield( i );
        }

        return 0;
    }

    void Run()
    {
        m_cofunc = std::make_unique<MyCoroutine>( this, &CoroutineExample::CountTo );
        m_cofunc->Call( m_count );

        while( m_cofunc->Running() )
        {
            m_cofunc->Resume();
        }
    }

    std::unique_ptr<MyCoroutine> m_cofunc;
    int                          m_count;
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
            wxCMD_LINE_OPTION,
            "c",
            "count",
            _( "how high to count" ).mb_str(),
            wxCMD_LINE_VAL_NUMBER,
            wxCMD_LINE_PARAM_OPTIONAL,
    },
    { wxCMD_LINE_NONE }
};


static int coroutine_main_func( int argc, char** argv )
{
    wxCmdLineParser cl_parser( argc, argv );
    cl_parser.SetDesc( g_cmdLineDesc );
    cl_parser.AddUsageText( _( "Test a simple coroutine that yields a given number of times" ) );

    int cmd_parsed_ok = cl_parser.Parse();
    if( cmd_parsed_ok != 0 )
    {
        // Help and invalid input both stop here
        return ( cmd_parsed_ok == -1 ) ? KI_TEST::RET_CODES::OK : KI_TEST::RET_CODES::BAD_CMDLINE;
    }

    long count = 5;
    cl_parser.Found( "count", &count );

    CoroutineExample obj( (int) count );

    obj.Run();

    return KI_TEST::RET_CODES::OK;
}


/*
 * Define the tool interface
 */
static bool registered = UTILITY_REGISTRY::Register( {
        "coroutine",
        "Test a simple coroutine",
        coroutine_main_func,
} );

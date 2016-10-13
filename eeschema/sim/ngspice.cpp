/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * https://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "ngspice.h"
#include "spice_reporter.h"

#include <common.h>     // LOCALE_IO
#include <wx/stdpaths.h>
#include <wx/dir.h>

#include <sstream>

using namespace std;

NGSPICE::NGSPICE()
{
    init();
}


NGSPICE::~NGSPICE()
{
}


void NGSPICE::Init()
{
    Command( "reset" );
}


vector<COMPLEX> NGSPICE::GetPlot( const string& aName, int aMaxLen )
{
    LOCALE_IO c_locale;       // ngspice works correctly only with C locale
    vector<COMPLEX> data;
    vector_info* vi = ngGet_Vec_Info( (char*) aName.c_str() );

    if( vi )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_realdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( COMPLEX( vi->v_realdata[i], 0.0 ) );
        }
        else if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( COMPLEX( vi->v_compdata[i].cx_real, vi->v_compdata[i].cx_imag ) );
        }
    }

    return data;
}


vector<double> NGSPICE::GetRealPlot( const string& aName, int aMaxLen )
{
    LOCALE_IO c_locale;       // ngspice works correctly only with C locale
    vector<double> data;
    vector_info* vi = ngGet_Vec_Info( (char*) aName.c_str() );

    if( vi )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_realdata )
        {
            for( int i = 0; i < length; i++ )
            {
                data.push_back( vi->v_realdata[i] );
            }
        }
        else if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
            {
                assert( vi->v_compdata[i].cx_imag == 0.0 );
                data.push_back( vi->v_compdata[i].cx_real );
            }
        }
    }

    return data;
}


vector<double> NGSPICE::GetImagPlot( const string& aName, int aMaxLen )
{
    LOCALE_IO c_locale;       // ngspice works correctly only with C locale
    vector<double> data;
    vector_info* vi = ngGet_Vec_Info( (char*) aName.c_str() );

    if( vi )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
            {
                data.push_back( vi->v_compdata[i].cx_imag );
            }
        }
    }

    return data;
}


vector<double> NGSPICE::GetMagPlot( const string& aName, int aMaxLen )
{
    LOCALE_IO c_locale;       // ngspice works correctly only with C locale
    vector<double> data;
    vector_info* vi = ngGet_Vec_Info( (char*) aName.c_str() );

    if( vi )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_realdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( vi->v_realdata[i] );
        }
        else if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( hypot( vi->v_compdata[i].cx_real, vi->v_compdata[i].cx_imag ) );
        }
    }

    return data;
}


vector<double> NGSPICE::GetPhasePlot( const string& aName, int aMaxLen )
{
    LOCALE_IO c_locale;       // ngspice works correctly only with C locale
    vector<double> data;
    vector_info* vi = ngGet_Vec_Info( (char*) aName.c_str() );

    if( vi )
    {
        int length = aMaxLen < 0 ? vi->v_length : std::min( aMaxLen, vi->v_length );
        data.reserve( length );

        if( vi->v_realdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( 0.0 );      // well, that's life
        }
        else if( vi->v_compdata )
        {
            for( int i = 0; i < length; i++ )
                data.push_back( atan2( vi->v_compdata[i].cx_imag, vi->v_compdata[i].cx_real ) );
        }
    }

    return data;
}


bool NGSPICE::LoadNetlist( const string& aNetlist )
{
    LOCALE_IO c_locale;       // ngspice works correctly only with C locale
    vector<char*> lines;
    stringstream ss( aNetlist );

    while( !ss.eof() )
    {
        char line[1024];
        ss.getline( line, sizeof( line ) );
        lines.push_back( strdup( line ) );
    }

    lines.push_back( nullptr );

    ngSpice_Circ( lines.data() );

    for( auto line : lines )
        delete line;

    return true;
}


bool NGSPICE::Run()
{
    LOCALE_IO c_locale;               // ngspice works correctly only with C locale
    return Command( "bg_run" );     // bg_* commands execute in a separate thread
}


bool NGSPICE::Stop()
{
    LOCALE_IO c_locale;               // ngspice works correctly only with C locale
    return Command( "bg_halt" );    // bg_* commands execute in a separate thread
}


bool NGSPICE::IsRunning()
{
    LOCALE_IO c_locale;               // ngspice works correctly only with C locale
    return ngSpice_running();
}


bool NGSPICE::Command( const string& aCmd )
{
    LOCALE_IO c_locale;               // ngspice works correctly only with C locale
    ngSpice_Command( (char*) aCmd.c_str() );

    return true;
}


string NGSPICE::GetXAxis( SIM_TYPE aType ) const
{
    switch( aType )
    {
        case ST_AC:
        case ST_NOISE:
            return string( "frequency" );
            break;

        case ST_DC:
            return string( "v-sweep" );
            break;

        case ST_TRANSIENT:
            return string( "time" );
            break;

        default:
            break;
    }

    return string( "" );
}


void NGSPICE::init()
{
    if( m_initialized )
        return;

    LOCALE_IO c_locale;               // ngspice works correctly only with C locale
    ngSpice_Init( &cbSendChar, &cbSendStat, &cbControlledExit, NULL, NULL, &cbBGThreadRunning, this );

    // Load a custom spinit file, to fix the problem with loading .cm files
    // Switch to the executable directory, so the relative paths are correct
    const wxStandardPaths& paths = wxStandardPaths::Get();
    wxString cwd( wxGetCwd() );
    wxFileName exeDir( paths.GetExecutablePath() );
    wxSetWorkingDirectory( exeDir.GetPath() );

    // Find *.cm files
    string cmPath = findCmPath();

    // __CMPATH is used in custom spinit file to point to the codemodels directory
    if( !cmPath.empty() )
        Command( "set __CMPATH=\"" + cmPath + "\"" );

    // Possible relative locations for spinit file
    const vector<string> spiceinitPaths =
    {
        ".",
        "../share/kicad",
        "../share",
        "../../share/kicad",
        "../../share"
    };

    bool foundSpiceinit = false;

    for( const auto& path : spiceinitPaths )
    {
        if( loadSpinit( path + "/spiceinit" ) )
        {
            foundSpiceinit = true;
            break;
        }
    }

    // Last chance to load codemodel files, we have not found
    // spiceinit file, but we know the path to *.cm files
    if( !foundSpiceinit && !cmPath.empty() )
        loadCodemodels( cmPath );

    // Restore the working directory
    wxSetWorkingDirectory( cwd );

    // Workarounds to avoid hang ups on certain errors,
    // they have to be called, no matter what is in the spinit file
    Command( "unset interactive" );
    Command( "set noaskquit" );
    Command( "set nomoremode" );

    m_initialized = true;
}


bool NGSPICE::loadSpinit( const string& aFileName )
{
    if( !wxFileName::FileExists( aFileName ) )
        return false;

    wxTextFile file;

    if( !file.Open( aFileName ) )
        return false;

    for( auto cmd = file.GetFirstLine(); !file.Eof(); cmd = file.GetNextLine() )
        Command( cmd.ToStdString() );

    return true;
}


string NGSPICE::findCmPath() const
{
    const vector<string> cmPaths =
    {
#ifdef __APPLE__
        "/Applications/ngspice/lib/ngspice",
#endif /* __APPLE__ */
        "../lib/ngspice",
        "../../lib/ngspice"
        "lib/ngspice",
        "ngspice"
    };

    for( const auto& path : cmPaths )
    {
        if( wxFileName::DirExists( path ) )
            return path;
    }

    return string();
}


bool NGSPICE::loadCodemodels( const string& aPath )
{
    wxArrayString cmFiles;
    size_t count = wxDir::GetAllFiles( aPath, &cmFiles );

    for( const auto& cm : cmFiles )
        Command( "codemodel " + cm.ToStdString() );

    return count != 0;
}


int NGSPICE::cbSendChar( char* what, int id, void* user )
{
    NGSPICE* sim = reinterpret_cast<NGSPICE*>( user );

    if( sim->m_reporter )
    {
        // strip stdout/stderr from the line
        if( ( strncasecmp( what, "stdout ", 7 ) == 0 )
                || ( strncasecmp( what, "stderr ", 7 ) == 0 ) )
            what += 7;

        sim->m_reporter->Report( what );
    }

    return 0;
}


int NGSPICE::cbSendStat( char* what, int id, void* user )
{
/*    NGSPICE* sim = reinterpret_cast<NGSPICE*>( user );
    if( sim->m_consoleReporter )
        sim->m_consoleReporter->Report( what );*/

    return 0;
}


int NGSPICE::cbBGThreadRunning( bool is_running, int id, void* user )
{
    NGSPICE* sim = reinterpret_cast<NGSPICE*>( user );

    if( sim->m_reporter )
        // I know the test below seems like an error, but well, it works somehow..
        sim->m_reporter->OnSimStateChange( sim, is_running ? SIM_IDLE : SIM_RUNNING );

    return 0;
}


int NGSPICE::cbControlledExit( int status, bool immediate, bool exit_upon_quit, int id, void* user )
{
    // Something went wrong, reload the dll
    //NGSPICE* sim = reinterpret_cast<NGSPICE*>( user );
    //sim->m_initialized = false;
    //printf("stat %d immed %d quit %d\n", status, !!immediate, !!exit_upon_quit);

    return 0;
}


bool NGSPICE::m_initialized = false;

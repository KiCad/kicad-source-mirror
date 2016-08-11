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

#include <wx/dynlib.h>
#include <wx/log.h>
#include <sstream>
#include <stdexcept>

using namespace std;

NGSPICE::NGSPICE()
{
#ifdef __WINDOWS__
    m_dll = new wxDynamicLibrary( "libngspice-0.dll" );
#else
    m_dll = new wxDynamicLibrary( wxDynamicLibrary::CanonicalizeName( "ngspice" ) );
#endif

    if( !m_dll || !m_dll->IsLoaded() )
        throw std::runtime_error( "Missing ngspice shared library" );

    // Obtain function pointers
    m_ngSpice_Init = (ngSpice_Init) m_dll->GetSymbol( "ngSpice_Init" );
    m_ngSpice_Circ = (ngSpice_Circ) m_dll->GetSymbol( "ngSpice_Circ" );
    m_ngSpice_Command = (ngSpice_Command) m_dll->GetSymbol( "ngSpice_Command" );
    m_ngGet_Vec_Info = (ngGet_Vec_Info) m_dll->GetSymbol( "ngGet_Vec_Info" );
    m_ngSpice_AllPlots = (ngSpice_AllPlots) m_dll->GetSymbol( "ngSpice_AllPlots" );
    m_ngSpice_AllVecs = (ngSpice_AllVecs) m_dll->GetSymbol( "ngSpice_AllVecs" );
    m_ngSpice_Running = (ngSpice_Running) m_dll->GetSymbol( "ngSpice_running" );
}


NGSPICE::~NGSPICE()
{
    delete m_dll;
}


void NGSPICE::Init()
{
    setlocale( LC_ALL, "C" );
    m_ngSpice_Init( &cbSendChar, &cbSendStat, &cbControlledExit, NULL, NULL, &cbBGThreadRunning, this );
    setlocale( LC_ALL, "" );
}


vector<COMPLEX> NGSPICE::GetPlot( const string& aName, int aMaxLen )
{
    vector<COMPLEX> data;
    setlocale( LC_ALL, "C" );
    vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );
    setlocale( LC_ALL, "" );

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
    vector<double> data;
    setlocale( LC_ALL, "C" );
    vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );
    setlocale( LC_ALL, "" );

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
    vector<double> data;

    setlocale( LC_ALL, "C" );
    vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );
    setlocale( LC_ALL, "" );

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
    vector<double> data;

    setlocale( LC_ALL, "C" );
    vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );
    setlocale( LC_ALL, "" );

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
    vector<double> data;

    setlocale( LC_ALL, "C" );
    vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );
    setlocale( LC_ALL, "" );

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
    vector<char*> lines;
    stringstream ss( aNetlist );

    while( !ss.eof() )
    {
        char line[1024];
        ss.getline( line, sizeof( line ) );
        lines.push_back( strdup( line ) );
    }

    lines.push_back( nullptr );

    setlocale( LC_ALL, "C" );
    m_ngSpice_Circ( lines.data() );
    setlocale( LC_ALL, "" );

    for( auto line : lines )
        delete line;

    return true;
}


bool NGSPICE::Run()
{
    setlocale( LC_ALL, "C" );
    bool rv = Command( "bg_run" );  // bg_* commands execute in a separate thread
    setlocale( LC_ALL, "" );
    return rv;
}


bool NGSPICE::Stop()
{
    setlocale( LC_ALL, "C" );
    bool rv = Command( "bg_halt" ); // bg_* commands execute in a separate thread
    setlocale( LC_ALL, "" );
    return rv;
}


bool NGSPICE::IsRunning()
{
    setlocale( LC_ALL, "C" );
    bool rv = m_ngSpice_Running();
    setlocale( LC_ALL, "" );
    return rv;
}


bool NGSPICE::Command( const string& aCmd )
{
    setlocale( LC_ALL, "C" );
    m_ngSpice_Command( (char*) aCmd.c_str() );
    setlocale( LC_ALL, "" );

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


int NGSPICE::cbControlledExit( int status, bool immediate, bool exit_upon_quit, int id, void* user )
{
    //printf("stat %d immed %d quit %d\n", status, !!immediate, !!exit_upon_quit);
    return 0;
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

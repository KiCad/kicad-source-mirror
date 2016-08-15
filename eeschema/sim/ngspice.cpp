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

#include <common.h>

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
    if( m_error )
        init();

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
    m_error = false;

    LOCALE_IO c_locale;               // ngspice works correctly only with C locale
    ngSpice_Init( &cbSendChar, &cbSendStat, &cbControlledExit, NULL, NULL, &cbBGThreadRunning, this );

    // Workaround to avoid hang ups on certain errors
    Command( "unset interactive" );
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
    NGSPICE* sim = reinterpret_cast<NGSPICE*>( user );
    sim->m_error = true;
    //printf("stat %d immed %d quit %d\n", status, !!immediate, !!exit_upon_quit);

    return 0;
}

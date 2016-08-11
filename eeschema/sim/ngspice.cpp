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
    m_dll = new wxDynamicLibrary( "libngspice.so" );
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
    m_ngSpice_Init( &cbSendChar, &cbSendStat, NULL, NULL, NULL, &cbBGThreadRunning, this );
}


vector<COMPLEX> NGSPICE::GetPlot( const string& aName, int aMaxLen )
{
    vector<COMPLEX> data;
    vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );

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
    vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );

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
    vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );

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
    vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );

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
    vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );

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
    // TODO remove the hard limit
    char* lines[16384];
    stringstream ss( aNetlist );
    int n = 0;

    while( !ss.eof() && n < 16384 )
    {
        char line[1024];
        ss.getline( line, sizeof(line) );
        lines[n++] = strdup(line);
    }

    lines[n] = NULL;
    m_ngSpice_Circ( lines );

    for(int i = 0; i < n; i++)
        delete lines[i];

    return true;
}


bool NGSPICE::Run()
{
    return Command( "bg_run" );
}


bool NGSPICE::Stop()
{
    return Command( "bg_halt" );
}


bool NGSPICE::IsRunning()
{
    return m_ngSpice_Running();
}


bool NGSPICE::Command( const string& aCmd )
{
    m_ngSpice_Command( (char*) aCmd.c_str() );

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


void NGSPICE::dump()
{
//    m_ngSpice_Command("run\n");
    char** plots = m_ngSpice_AllPlots();

    for( int i = 0; plots[i]; ++i )
    {
        wxLogDebug( "-> plot : %s", plots[i] );
        char** vecs = m_ngSpice_AllVecs( plots[i] );

        for( int j = 0; vecs[j]; j++ )
        {
            wxLogDebug( "   - vector %s", vecs[j] );

            vector_info* vi = m_ngGet_Vec_Info( vecs[j] );

            wxLogDebug( "       - v_type %x", vi->v_type );
            wxLogDebug( "       - v_flags %x", vi->v_flags );
            wxLogDebug( "       - v_length %d", vi->v_length );
        }
    }
}


#if 0
static string loadFile(const string& filename)
{

    FILE *f=fopen(filename.c_str(),"rb");
    char buf[10000];
    int n = fread(buf, 1, 10000, f);
    fclose(f);
    buf[n] = 0;
    return buf;
}


main()
{
    NGSPICE spice;
    spice.Init();
    spice.LoadNetlist(loadFile("1.ckt"));

    spice.Command("tran .05 1");
    spice.Command("save all");

    spice.Run();
    vector<double> t = spice.GetPlot("time");
    vector<double> v1 = spice.GetPlot("V(1)");
    vector<double> v2 = spice.GetPlot("V(2)");

    // Prepare data.

    // Plot line from given x and y data. Color is selected automatically.
    plt::plot(t, v1);
    // Plot a red dashed line from given x and y data.
    plt::plot(t, v2,"r--");

    for(int i=0;i<v1.size();i++)
        wxLogDebug("%.10f\n",v2[i]);

    // Add graph title
    plt::title("Sample figure");
    // Enable legend.
    plt::legend();
    // save figure
    plt::show();


//    spice.Run();
}

#endif


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

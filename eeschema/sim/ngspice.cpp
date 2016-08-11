/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "ngspice.h"

#include <wx/dynlib.h>
#include <wx/log.h>
#include <reporter.h>
#include <sstream>

// TODO cmake modules to add include directory for ngspice

using namespace std;

NGSPICE::NGSPICE()
{
    m_dll = new wxDynamicLibrary( "libngspice.so" );
    assert( m_dll );

    // Obtain function pointers
    m_ngSpice_Init = (ngSpice_Init) m_dll->GetSymbol( "ngSpice_Init" );
    m_ngSpice_Circ = (ngSpice_Circ) m_dll->GetSymbol( "ngSpice_Circ" );
    m_ngSpice_Command = (ngSpice_Command) m_dll->GetSymbol( "ngSpice_Command" );
    m_ngGet_Vec_Info = (ngGet_Vec_Info) m_dll->GetSymbol( "ngGet_Vec_Info" );
    m_ngSpice_AllPlots = (ngSpice_AllPlots) m_dll->GetSymbol( "ngSpice_AllPlots" );
    m_ngSpice_AllVecs = (ngSpice_AllVecs) m_dll->GetSymbol( "ngSpice_AllVecs" );
}


NGSPICE::~NGSPICE()
{
    delete m_dll;
}


void NGSPICE::Init()
{
    m_ngSpice_Init( &cbSendChar, &cbSendStat, NULL, NULL, NULL, NULL, this );
}


const vector<double> NGSPICE::GetPlot( const string& aName, int aMaxLen )
{
    vector<double> data;

    vector_info* vi = m_ngGet_Vec_Info( (char*) aName.c_str() );

    if( vi->v_realdata )
    {
        for( int i = 0; i < vi->v_length; i++ )
            data.push_back( vi->v_realdata[i] );
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
        wxLogDebug( "l '%s'\n", line );
    }

    lines[n] = NULL;
    m_ngSpice_Circ( lines );

    for(int i = 0; i < n; i++)
        delete lines[i];

    return true;
}


bool NGSPICE::Run()
{
    return Command( "run\n" );
}


bool NGSPICE::Command( const string& aCmd )
{
    m_ngSpice_Command( (char*)( aCmd + string( "\n" ) ).c_str() );
    dump();

    return true;
}


void NGSPICE::dump()
{
//    m_ngSpice_Command("run\n");
    char** plots = m_ngSpice_AllPlots();

    for( int i = 0; plots[i]; ++i )
    {
        wxLogDebug( "-> plot : %s\n", plots[i] );
        char** vecs = m_ngSpice_AllVecs( plots[i] );

        for( int j = 0; vecs[j]; j++ )
        {
            wxLogDebug( "   - vector %s\n", vecs[j] );

            vector_info* vi = m_ngGet_Vec_Info( vecs[j] );

            wxLogDebug( "       - v_type %x\n", vi->v_type );
            wxLogDebug( "       - v_flags %x\n", vi->v_flags );
            wxLogDebug( "       - v_length %d\n", vi->v_length );
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



string NGSPICE::GetConsole() const {
    return "";
}


int NGSPICE::cbSendChar( char* what, int id, void* user )
{
    NGSPICE* sim = reinterpret_cast<NGSPICE*>( user );

    wxLogDebug( "sim %p cr %p\n", sim, sim->m_consoleReporter );

    if( sim->m_consoleReporter )
        sim->m_consoleReporter->Report( what );

    return 0;
}


int NGSPICE::cbSendStat( char* what, int id, void* user )
{
/*    NGSPICE* sim = reinterpret_cast<NGSPICE*>( user );
    if( sim->m_consoleReporter )
        sim->m_consoleReporter->Report( what );*/
    return 0;
}

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

#include <schframe.h>
#include <kiway.h>

#include <netlist_exporter_kicad.h>
#include <netlist_exporters/netlist_exporter_pspice.h>

#include <reporter.h>

#include "sim_plot_frame.h"
#include "sim_plot_panel.h"
#include "spice_simulator.h"

#ifdef KICAD_SCRIPTING
 #include <python_scripting.h>
#endif


class SIM_REPORTER : public REPORTER
{
public:
    SIM_REPORTER( wxRichTextCtrl* aConsole )
    {
        m_console = aConsole;
    }

    ~SIM_REPORTER()
    {
    }

    virtual REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_UNDEFINED )
    {
        m_console->WriteText( aText );
        m_console->Newline();
        return *this;
    }

private:
    wxRichTextCtrl* m_console;
};


SIM_PLOT_FRAME::SIM_PLOT_FRAME( KIWAY* aKiway, wxWindow* aParent )
    : SIM_PLOT_FRAME_BASE( aKiway, aParent )
{
    m_exporter = NULL;
    m_simulator = NULL;
    m_currentPlot = NULL;
    m_pyConsole = NULL;

    NewPlot();
    TogglePythonConsole();
}


SIM_PLOT_FRAME::~SIM_PLOT_FRAME()
{
}


void SIM_PLOT_FRAME::StartSimulation()
{
    if( m_exporter )
        delete m_exporter;

    if( m_simulator )
        delete m_simulator;

    m_simulator = SPICE_SIMULATOR::CreateInstance( "ngspice" );
    m_simulator->SetConsoleReporter( new SIM_REPORTER( m_simConsole ) );
    m_simulator->Init();
    //m_simulator->SetConsoleReporter( , this );

    NETLIST_OBJECT_LIST* net_atoms = m_schematicFrame->BuildNetListBase();
    m_exporter = new NETLIST_EXPORTER_PSPICE ( net_atoms, Prj().SchLibs() );
    STRING_FORMATTER formatter;

    m_exporter->Format( &formatter, GNL_ALL );
    //m_plotPanel->DeleteTraces();

    wxLogDebug( "*******************\n%s\n", (const char *)formatter.GetString().c_str() );

    m_simulator->LoadNetlist( formatter.GetString() );
    m_simulator->Command("run\n");

    auto mapping = m_exporter->GetNetIndexMap();
//    auto data_t = m_simulator->GetPlot("time");

    for(auto name : m_exporter->GetProbeList())
    {
        char spiceName[1024];

        sprintf(spiceName,"V(%d)", mapping[name] );
        //printf("probe %s->%s\n", (const char *) name.c_str(), spiceName);
    //    auto data_y = m_simulator->GetPlot(spiceName);

        //printf("%d - %d data points\n", data_t.size(), data_y.size() );
    //    m_plotPanel->AddTrace(wxT("V(") + name + wxT(")"), data_t.size(), data_t.data(), data_y.data(), 0);
    }

    delete m_simulator;
    m_simulator = NULL;
    //m_simulator->Command("quit\n");
}


void SIM_PLOT_FRAME::NewPlot()
{
    SIM_PLOT_PANEL* plot = new SIM_PLOT_PANEL( this, wxID_ANY );
    m_plotNotebook->AddPage( plot, wxT( "Plot1" ), true );
    m_currentPlot = plot;
}

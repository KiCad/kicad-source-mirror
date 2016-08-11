#include <schframe.h>
#include <fctsys.h>
#include <kiface_i.h>
#include <pgm_base.h>
#include <gr_basic.h>
#include <class_drawpanel.h>
#include <gestfich.h>
#include <confirm.h>
#include <base_units.h>
#include <msgpanel.h>
#include <html_messagebox.h>

#include <general.h>
#include <eeschema_id.h>
#include <netlist.h>
#include <lib_pin.h>
#include <class_library.h>
#include <schframe.h>
#include <sch_component.h>

#include <dialog_helpers.h>
#include <libeditframe.h>
#include <viewlib_frame.h>
#include <hotkeys.h>
#include <eeschema_config.h>
#include <sch_sheet.h>
#include <sch_sheet_path.h>

#include <invoke_sch_dialog.h>
#include <dialogs/dialog_schematic_find.h>

#include <wx/display.h>
#include <build_version.h>
#include <wildcards_and_files_ext.h>

#include <netlist_exporter_kicad.h>
#include <kiway.h>

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
    SIM_REPORTER( wxRichTextCtrl* console )
    {
        m_console = console;

    }

    ~SIM_REPORTER()
    {
    }

    virtual REPORTER& Report( const wxString& aText, SEVERITY aSeverity = RPT_UNDEFINED )
    {
        m_console->WriteText(aText);
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

    printf("*******************\n%s\n", (const char *)formatter.GetString().c_str());

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

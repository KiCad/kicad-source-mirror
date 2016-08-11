#include "sim_plot_panel.h"
#include <boost/foreach.hpp>

static SIM_PLOT_PANEL *panel = NULL;

static int drawPlotFunc( mglGraph *graph )
{

	printf("DrawPlot [%d traces]!\n", panel->m_traces.size());

	graph->Clf();
	//graph->SetRanges(-10e-3,10e-3,-2,2);
	graph->Axis("x");
	graph->Label('x',"Time",0);
	graph->SetRange('x', 0, 10e-3);

	graph->Axis("y");
	graph->Label('y',"Voltage",0);
	graph->SetRange('y', -1.5, 1.5);


	for(auto t : panel->m_traces)
	{
			graph->AddLegend((const char *)t.name.c_str(),"");
			graph->Plot(t.y);
	}

	graph->Box();
	graph->Grid();
	if ( panel->m_traces.size() )
		graph->Legend(1,"-#");


	return 0;
}


SIM_PLOT_PANEL::SIM_PLOT_PANEL( wxWindow *  	parent,
				wxWindowID  	id,
				const wxPoint &  	pos,
				const wxSize &  	size,
			    	long  	style,
				const wxString &  	name )
    : wxMathGL ( parent, id, pos, size, style, name )
{
	panel = this;

	AutoResize = true;
	SetDraw( drawPlotFunc );
//	Update();
}


SIM_PLOT_PANEL::~SIM_PLOT_PANEL()
{

}

void SIM_PLOT_PANEL::AddTrace(const wxString& name, int n_points, double *t, double *x, int flags )
{
	Trace trace;

	trace.name = name;
	trace.x.Set(t, n_points);
	trace.y.Set(x, n_points);

	m_traces.push_back(trace);
	Update();
}

void SIM_PLOT_PANEL::DeleteTraces()
{
	m_traces.clear();
	Update();
}

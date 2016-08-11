#ifndef __SIM_PLOT_PANEL_H
#define __SIM_PLOT_PANEL_H

#include "mgl2/canvas_wnd.h"
#include "mgl2/wx.h"


class SIM_PLOT_PANEL : public wxMathGL
{
public:
    SIM_PLOT_PANEL( 	wxWindow *  	parent,
		wxWindowID  	id,
		const wxPoint &  	pos = wxDefaultPosition,
		const wxSize &  	size = wxDefaultSize,
		long  	style = 0,
		const wxString &  	name = wxPanelNameStr );

    ~SIM_PLOT_PANEL();




    struct Trace {
        wxString name;
        mglData x, y;
    };

    std::vector<Trace> m_traces;

    void AddTrace(const wxString& name, int n_points, double *t, double *x, int flags = 0 );
    void DeleteTraces();
};

#endif

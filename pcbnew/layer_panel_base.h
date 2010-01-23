///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec 29 2008)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __layer_panel_base__
#define __layer_panel_base__

#include <wx/intl.h>

#include <wx/sizer.h>
#include <wx/gdicmn.h>
#include <wx/scrolwin.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
//#include <wx/notebook.h>
#include <wx/aui/auibook.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class LAYER_PANEL_BASE
///////////////////////////////////////////////////////////////////////////////
class LAYER_PANEL_BASE : public wxPanel
{
    private:

    protected:
//		wxNotebook* m_notebook;
        wxAuiNotebook* m_notebook;

        wxPanel* m_LayerPanel;
        wxScrolledWindow* m_LayerScrolledWindow;
        wxFlexGridSizer* m_LayersFlexGridSizer;
        wxPanel* m_RenderingPanel;
        wxScrolledWindow* m_RenderScrolledWindow;
        wxFlexGridSizer* m_RenderFlexGridSizer;

        // Virtual event handlers, overide them in your derived class
        virtual void OnLeftDownLayers( wxMouseEvent& event ){ event.Skip(); }
        virtual void OnRightDownLayers( wxMouseEvent& event ){ event.Skip(); }


    public:
        LAYER_PANEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 200,200 ), long style = wxTAB_TRAVERSAL );
        ~LAYER_PANEL_BASE();

};

#endif //__layer_panel_base__

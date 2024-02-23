///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class BITMAP_BUTTON;
class WX_GRID;

#include "widgets/wx_panel.h"
#include <wx/scrolwin.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/grid.h>
#include <wx/splitter.h>
#include <wx/notebook.h>
#include <wx/choice.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class APPEARANCE_CONTROLS_BASE
///////////////////////////////////////////////////////////////////////////////
class APPEARANCE_CONTROLS_BASE : public WX_PANEL
{
	private:

	protected:
		wxBoxSizer* m_sizerOuter;
		wxNotebook* m_notebook;
		wxPanel* m_panelLayers;
		wxBoxSizer* m_panelLayersSizer;
		wxScrolledCanvas* m_windowLayers;
		wxPanel* m_panelObjects;
		wxBoxSizer* m_objectsPanelSizer;
		wxScrolledCanvas* m_windowObjects;
		wxPanel* m_panelNetsAndClasses;
		wxBoxSizer* m_netsTabOuterSizer;
		wxSplitterWindow* m_netsTabSplitter;
		wxPanel* m_panelNets;
		wxStaticText* m_staticTextNets;
		wxTextCtrl* m_txtNetFilter;
		BITMAP_BUTTON* m_btnNetInspector;
		WX_GRID* m_netsGrid;
		wxPanel* m_panelNetclasses;
		wxStaticText* m_staticTextNetClasses;
		BITMAP_BUTTON* m_btnConfigureNetClasses;
		wxScrolledWindow* m_netclassScrolledWindow;
		wxBoxSizer* m_netclassOuterSizer;
		wxStaticText* m_presetsLabel;
		wxChoice* m_cbLayerPresets;
		wxStaticText* m_viewportsLabel;
		wxChoice* m_cbViewports;

		// Virtual event handlers, override them in your derived class
		virtual void OnSetFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnSize( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnNotebookPageChanged( wxNotebookEvent& event ) { event.Skip(); }
		virtual void OnNetGridClick( wxGridEvent& event ) { event.Skip(); }
		virtual void OnNetGridDoubleClick( wxGridEvent& event ) { event.Skip(); }
		virtual void OnNetGridRightClick( wxGridEvent& event ) { event.Skip(); }
		virtual void onLayerPresetChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void onViewportChanged( wxCommandEvent& event ) { event.Skip(); }


	public:

		APPEARANCE_CONTROLS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~APPEARANCE_CONTROLS_BASE();

		void m_netsTabSplitterOnIdle( wxIdleEvent& )
		{
			m_netsTabSplitter->SetSashPosition( 300 );
			m_netsTabSplitter->Disconnect( wxEVT_IDLE, wxIdleEventHandler( APPEARANCE_CONTROLS_BASE::m_netsTabSplitterOnIdle ), NULL, this );
		}

};


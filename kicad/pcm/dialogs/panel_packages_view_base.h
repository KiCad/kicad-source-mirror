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
class HTML_WINDOW;
class WX_PANEL;

#include "widgets/wx_grid.h"
#include "widgets/wx_splitter_window.h"
#include <wx/string.h>
#include <wx/srchctrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>
#include <wx/html/htmlwin.h>
#include <wx/grid.h>
#include <wx/checkbox.h>
#include <wx/splitter.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PACKAGES_VIEW_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PACKAGES_VIEW_BASE : public wxPanel
{
	private:

	protected:
		WX_SPLITTER_WINDOW* m_splitter1;
		WX_PANEL* m_panelList;
		wxSearchCtrl* m_searchCtrl;
		wxButton* m_buttonUpdateAll;
		wxScrolledWindow* m_packageListWindow;
		wxPanel* m_panelDetails;
		wxScrolledWindow* m_infoScrollWindow;
		HTML_WINDOW* m_infoText;
		wxBoxSizer* m_sizerVersions;
		WX_GRID* m_gridVersions;
		wxCheckBox* m_showAllVersions;
		wxButton* m_buttonDownload;
		wxButton* m_buttonAction;

		// Virtual event handlers, override them in your derived class
		virtual void OnUpdateAllClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSizeInfoBox( wxSizeEvent& event ) { event.Skip(); }
		virtual void OnURLClicked( wxHtmlLinkEvent& event ) { event.Skip(); }
		virtual void OnInfoMouseWheel( wxMouseEvent& event ) { event.Skip(); }
		virtual void OnVersionsCellClicked( wxGridEvent& event ) { event.Skip(); }
		virtual void OnShowAllVersionsClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDownloadVersionClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnVersionActionClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_PACKAGES_VIEW_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_PACKAGES_VIEW_BASE();

		void m_splitter1OnIdle( wxIdleEvent& )
		{
			m_splitter1->SetSashPosition( 0 );
			m_splitter1->Disconnect( wxEVT_IDLE, wxIdleEventHandler( PANEL_PACKAGES_VIEW_BASE::m_splitter1OnIdle ), NULL, this );
		}

};


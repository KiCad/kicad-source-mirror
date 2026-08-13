///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class WX_GRID;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/hyperlink.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/grid.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/panel.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_MDW_CHANGE_ORDERS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_MDW_CHANGE_ORDERS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxNotebook* m_notebook;
		wxPanel* m_ecoPanel;
		wxStaticText* m_ecoMessage;
		wxHyperlinkCtrl* m_ecoRetry;
		wxCheckBox* m_showClosed;
		WX_GRID* m_ecoGrid;
		wxButton* m_ecoDetails;
		wxPanel* m_reviewPanel;
		wxStaticText* m_reviewStatus;
		wxStaticText* m_reviewMessage;
		wxHyperlinkCtrl* m_reviewRetry;
		wxCheckBox* m_thisRevisionOnly;
		WX_GRID* m_reviewGrid;
		wxButton* m_reviewDetails;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;

		// Virtual event handlers, override them in your derived class
		virtual void OnEcoRetry( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void OnShowClosed( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEcoActivated( wxGridEvent& event ) { event.Skip(); }
		virtual void OnEcoDetails( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnReviewRetry( wxHyperlinkEvent& event ) { event.Skip(); }
		virtual void OnThisRevisionOnly( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnReviewActivated( wxGridEvent& event ) { event.Skip(); }
		virtual void OnReviewDetails( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_MDW_CHANGE_ORDERS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Change Orders and Reviews"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_MDW_CHANGE_ORDERS_BASE();

};


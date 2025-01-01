///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_ABOUT_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_ABOUT_BASE : public wxDialog
{
	private:

	protected:
		wxStaticBitmap* m_bitmapApp;
		wxStaticText* m_staticTextAppTitle;
		wxStaticText* m_staticTextBuildVersion;
		wxStaticText* m_staticTextLibVersion;
		wxButton* m_btCopyVersionInfo;
		wxButton* m_btReportBug;
		wxButton* m_btDonate;
		wxNotebook* m_notebook;
		wxButton* m_btOk;

		// Virtual event handlers, override them in your derived class
		virtual void onCopyVersionInfo( wxCommandEvent& event ) { event.Skip(); }
		virtual void onReportBug( wxCommandEvent& event ) { event.Skip(); }
		virtual void onDonateClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnNotebookPageChanged( wxNotebookEvent& event ) { event.Skip(); }


	public:

		DIALOG_ABOUT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("About"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 570,500 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_ABOUT_BASE();

};


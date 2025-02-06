///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-282-g1fa54006)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/choice.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PRINTER_LIST_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PRINTER_LIST_BASE : public wxPanel
{
	private:

	protected:
		wxStaticBoxSizer* m_sbSizerPrinters;
		wxChoice* m_choicePrinter;
		wxStaticText* m_staticTextPrn;
		wxStaticText* m_stPrinterState;

		// Virtual event handlers, override them in your derived class
		virtual void onPrinterChoice( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_PRINTER_LIST_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 378,109 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_PRINTER_LIST_BASE();

};


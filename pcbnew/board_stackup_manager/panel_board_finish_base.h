///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_BOARD_FINISH_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_BOARD_FINISH_BASE : public wxPanel
{
	private:

	protected:
		wxCheckBox* m_cbEgdesPlated;
		wxStaticText* m_staticTextFinish;
		wxChoice* m_choiceFinish;
		wxStaticText* m_staticTextEdgeConn;
		wxChoice* m_choiceEdgeConn;

	public:

		PANEL_SETUP_BOARD_FINISH_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_BOARD_FINISH_BASE();

};


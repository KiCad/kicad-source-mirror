///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.0)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PRODUCE_PCB_BASE
///////////////////////////////////////////////////////////////////////////////
class PRODUCE_PCB_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* m_mainSizer;
		wxStaticText* m_label;
		wxStaticText* m_label1;
		wxStaticText* m_label2;
		wxStaticText* m_label3;
		wxStaticText* m_label31;
		wxStaticText* m_label32;

	public:

		PRODUCE_PCB_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Send PCB for production"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~PRODUCE_PCB_BASE();

};


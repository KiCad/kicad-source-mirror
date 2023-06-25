///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b3)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/radiobut.h>
#include <wx/filepicker.h>
#include <wx/sizer.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_GLOBAL_LIB_TABLE_CONFIG_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_GLOBAL_LIB_TABLE_CONFIG_BASE : public wxPanel
{
	private:

	protected:
		wxBoxSizer* bSizer2;
		wxStaticText* m_staticText1;
		wxRadioButton* m_defaultRb;
		wxRadioButton* m_customRb;
		wxRadioButton* m_emptyRb;
		wxStaticText* m_staticText2;
		wxFilePickerCtrl* m_filePicker1;

		// Virtual event handlers, override them in your derived class
		virtual void onUpdateDefaultSelection( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onUpdateFilePicker( wxUpdateUIEvent& event ) { event.Skip(); }


	public:

		PANEL_GLOBAL_LIB_TABLE_CONFIG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_GLOBAL_LIB_TABLE_CONFIG_BASE();

};


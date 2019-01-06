///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Nov 23 2018)
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
#include <wx/radiobut.h>
#include <wx/filepicker.h>
#include <wx/statline.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG_BASE : public DIALOG_SHIM
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
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;

		// Virtual event handlers, overide them in your derived class
		virtual void onUpdateDefaultSelection( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onUpdateFilePicker( wxUpdateUIEvent& event ) { event.Skip(); }


	public:

		DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Configure Global Symbol Library Table"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxCAPTION|wxRESIZE_BORDER );
		~DIALOG_GLOBAL_SYM_LIB_TABLE_CONFIG_BASE();

};


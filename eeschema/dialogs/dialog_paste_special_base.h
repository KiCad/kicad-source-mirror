///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Oct 26 2018)
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
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PASTE_SPECIAL_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PASTE_SPECIAL_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticText7;
		wxCheckBox* m_keepAnnotations;
		wxCheckBox* m_dropAnnotations;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnKeepAnnotations( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDropAnnotations( wxCommandEvent& event ) { event.Skip(); }
		virtual void onOKButton( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_PASTE_SPECIAL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Paste Special"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_PASTE_SPECIAL_BASE();

};


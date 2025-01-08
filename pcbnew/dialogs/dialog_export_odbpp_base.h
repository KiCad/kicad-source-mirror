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
class STD_BITMAP_BUTTON;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/spinctrl.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXPORT_ODBPP_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXPORT_ODBPP_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxBoxSizer* bSizerTop;
		wxStaticText* m_lblBrdFile;
		wxTextCtrl* m_outputFileName;
		STD_BITMAP_BUTTON* m_browseButton;
		wxStaticText* m_lblUnits;
		wxChoice* m_choiceUnits;
		wxStaticText* m_lblPrecision;
		wxSpinCtrl* m_precision;
		wxStaticText* m_lblCompress;
		wxChoice* m_choiceCompress;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onFormatChoice( wxCommandEvent& event ) { event.Skip(); }
		virtual void onOKClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_EXPORT_ODBPP_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Export ODB++"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxBORDER_DEFAULT );

		~DIALOG_EXPORT_ODBPP_BASE();

};


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
class BITMAP_BUTTON;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stc/stc.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/hyperlink.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_TEXT_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_TEXT_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		enum
		{
			wxID_SIZE = 1000
		};

		wxFlexGridSizer* m_textEntrySizer;
		wxStaticText* m_textLabel;
		wxStyledTextCtrl* m_textCtrl;
		wxStaticText* m_textSizeLabel;
		wxTextCtrl* m_textSizeCtrl;
		wxStaticText* m_textSizeUnits;
		BITMAP_BUTTON* m_separator1;
		BITMAP_BUTTON* m_bold;
		BITMAP_BUTTON* m_italic;
		BITMAP_BUTTON* m_separator2;
		BITMAP_BUTTON* m_spin0;
		BITMAP_BUTTON* m_spin1;
		BITMAP_BUTTON* m_spin2;
		BITMAP_BUTTON* m_spin3;
		BITMAP_BUTTON* m_separator3;
		wxHyperlinkCtrl* m_syntaxHelp;
		wxStaticLine* m_staticline;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, overide them in your derived class
		virtual void onMultiLineTCLostFocus( wxFocusEvent& event ) { event.Skip(); }
		virtual void OnFormattingHelp( wxHyperlinkEvent& event ) { event.Skip(); }


	public:

		DIALOG_TEXT_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Text Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_TEXT_PROPERTIES_BASE();

};


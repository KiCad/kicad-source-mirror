///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 3.10.1-0-g8feb16b)
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
#include <wx/listbox.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_BOM_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_BOM_BASE : public DIALOG_SHIM
{
	private:

	protected:
		enum
		{
			IN_NAMELINE = 1000,
			ID_CMDLINE
		};

		wxStaticText* m_staticTextGeneratorTitle;
		wxListBox* m_lbGenerators;
		wxStaticText* m_staticTextName;
		wxTextCtrl* m_textCtrlName;
		wxTextCtrl* m_Messages;
		STD_BITMAP_BUTTON* m_buttonAddGenerator;
		STD_BITMAP_BUTTON* m_buttonEdit;
		STD_BITMAP_BUTTON* m_buttonDelGenerator;
		wxStaticText* m_staticTextCmd;
		wxTextCtrl* m_textCtrlCommand;
		wxCheckBox* m_checkBoxShowConsole;
		wxButton* m_buttonReset;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;
		wxButton* m_sdbSizerHelp;

		// Virtual event handlers, override them in your derived class
		virtual void OnIdle( wxIdleEvent& event ) { event.Skip(); }
		virtual void OnGeneratorSelected( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnNameEdited( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddGenerator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnEditGenerator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRemoveGenerator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCommandLineEdited( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnShowConsoleChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnHelp( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnRunGenerator( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_BOM_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Bill of Materials"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_BOM_BASE();

};


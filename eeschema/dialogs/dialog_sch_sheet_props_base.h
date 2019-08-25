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
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_SCH_SHEET_PROPS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_SCH_SHEET_PROPS_BASE : public DIALOG_SHIM
{
	DECLARE_EVENT_TABLE()
	private:

		// Private event handlers
		void _wxFB_OnBrowseClicked( wxCommandEvent& event ){ OnBrowseClicked( event ); }


	protected:
		enum
		{
			ID_BUTTON_BROWSE_SHEET = 1000
		};

		wxStaticText* m_filenameLabel;
		wxTextCtrl* m_textFileName;
		wxBitmapButton* m_browseButton;
		wxStaticText* m_filenameSizeLabel;
		wxTextCtrl* m_filenameSizeCtrl;
		wxStaticText* m_filenameSizeUnits;
		wxStaticText* m_sheetnameLabel;
		wxTextCtrl* m_textSheetName;
		wxStaticText* m_sheetnameSizeLabel;
		wxTextCtrl* m_sheetnameSizeCtrl;
		wxStaticText* m_sheetnameSizeUnits;
		wxStaticText* m_staticTextTimeStamp;
		wxTextCtrl* m_textCtrlTimeStamp;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnBrowseClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_SCH_SHEET_PROPS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Schematic Sheet Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_SCH_SHEET_PROPS_BASE();

};


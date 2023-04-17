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
class PANEL_PRINTER_LIST;

#include "dialog_shim.h"
#include <wx/panel.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/gbsizer.h>
#include <wx/statbox.h>
#include <wx/radiobut.h>
#include <wx/textctrl.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PRINT_GENERIC_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PRINT_GENERIC_BASE : public DIALOG_SHIM
{
	private:

	protected:
		enum
		{
			wxID_FRAME_SEL = 8200,
			wxID_PRINT_OPTIONS,
		};

		PANEL_PRINTER_LIST* m_panelPrinters;
		wxBoxSizer* m_bUpperSizer;
		wxStaticBoxSizer* m_sbOptionsSizer;
		wxGridBagSizer* m_gbOptionsSizer;
		wxStaticText* m_outputModeLabel;
		wxChoice* m_outputMode;
		wxCheckBox* m_titleBlock;
		wxRadioButton* m_scale1;
		wxRadioButton* m_scaleFit;
		wxRadioButton* m_scaleCustom;
		wxTextCtrl* m_scaleCustomText;
		wxStaticText* m_infoText;
		wxButton* m_buttonOption;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Apply;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void onClose( wxCloseEvent& event ) { event.Skip(); }
		virtual void onSetCustomScale( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPageSetup( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPrintPreview( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCancelButtonClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onPrintButtonClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_PRINT_GENERIC_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Print"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_PRINT_GENERIC_BASE();

};


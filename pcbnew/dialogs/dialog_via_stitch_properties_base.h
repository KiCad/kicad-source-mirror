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
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <widgets/net_selector.h>
#include <wx/textctrl.h>
#include <wx/combobox.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <pcb_base_frame.h>
#include <pcb_draw_panel_gal.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_VIA_STITCH_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_VIA_STITCH_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		enum
		{
			wxID_DIALOG_EDIT_PAD = 10000,
		};

		wxBoxSizer* m_middleBoxSizer;
		wxStaticText* m_netLabel;
		NET_SELECTOR* m_netSelector;
		wxStaticText* m_posXLabel;
		wxTextCtrl* m_posXCtrl;
		wxStaticText* m_posXUnits;
		wxStaticText* m_posYLabel;
		wxTextCtrl* m_posYCtrl;
		wxStaticText* m_posYUnits;
		wxStaticText* m_patternLabel;
		wxComboBox* m_patternCombo;
		wxStaticText* m_pitchLabel;
		wxTextCtrl* m_pitchCtrl;
		wxStaticText* m_textSizeUnits;
		wxStaticText* m_seedLabel;
		wxTextCtrl* m_seedCtrl;
		wxStaticText* m_pitchLabel1;
		wxButton* m_viaProperties;
		wxStaticText* m_modeLabel;
		wxComboBox* m_modeCombo;
		PCB_DRAW_PANEL_GAL* m_panelShowPreview;
		KIGFX::GAL_DISPLAY_OPTIONS m_galOptions;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void OnValuesChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnViaConfigureClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancel( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_VIA_STITCH_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_DIALOG_EDIT_PAD, const wxString& title = _("Via Stitch Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_VIA_STITCH_PROPERTIES_BASE();

};


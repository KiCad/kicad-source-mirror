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
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/radiobut.h>
#include <wx/stattext.h>
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/gbsizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_TUNING_PATTERN_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_TUNING_PATTERN_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticBitmap* m_legend;
		wxRadioButton* m_radioBtnLength;
		wxStaticText* m_targetLengthLabel;
		wxTextCtrl* m_targetLengthCtrl;
		wxStaticText* m_targetLengthUnits;
		wxRadioButton* m_radioBtnDelay;
		wxStaticText* m_targetDelayLabel;
		wxTextCtrl* m_targetDelayCtrl;
		wxStaticText* m_targetDelayUnits;
		wxCheckBox* m_overrideCustomRules;
		wxStaticText* m_sourceInfo;
		wxStaticText* m_track_minALabel;
		wxTextCtrl* m_minACtrl;
		wxStaticText* m_minAUnits;
		wxStaticText* m_maxALabel;
		wxTextCtrl* m_maxACtrl;
		wxStaticText* m_maxAUnits;
		wxStaticText* m_spacingLabel;
		wxTextCtrl* m_spacingCtrl;
		wxStaticText* m_spacingUnits;
		wxStaticText* m_cornerLabel;
		wxChoice* m_cornerCtrl;
		wxStaticText* m_rLabel;
		wxTextCtrl* m_rCtrl;
		wxStaticText* m_rUnits;
		wxCheckBox* m_singleSided;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onRadioBtnTargetLengthClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onRadioBtnTargetDelayClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onOverrideCustomRules( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_TUNING_PATTERN_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Tuning Pattern Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_TUNING_PATTERN_PROPERTIES_BASE();

};


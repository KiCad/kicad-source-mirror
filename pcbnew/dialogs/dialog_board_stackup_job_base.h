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
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_BOARD_STACKUP_JOB_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_BOARD_STACKUP_JOB_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_textOutputPath;
		wxTextCtrl* m_textCtrlOutputPath;
		wxStaticText* m_labelFormat;
		wxChoice* m_choiceFormat;
		wxStaticText* m_labelUnits;
		wxChoice* m_choiceUnits;
		wxStaticBoxSizer* m_sbSizerFields;
		wxCheckBox* m_cbThickness;
		wxCheckBox* m_cbMaterial;
		wxCheckBox* m_cbColor;
		wxCheckBox* m_cbEpsilonR;
		wxCheckBox* m_cbLossTangent;
		wxCheckBox* m_cbFinish;
		wxCheckBox* m_cbBoardOptions;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnFormatChoice( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_BOARD_STACKUP_JOB_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_BOARD_STACKUP_JOB_BASE();

};


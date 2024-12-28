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
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/radiobox.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_RENDER_JOB_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_RENDER_JOB_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticText9;
		wxPanel* m_panel9;
		wxStaticText* m_textOutputPath;
		wxTextCtrl* m_textCtrlOutputFile;
		wxStaticText* m_staticText16;
		wxSpinCtrl* m_spinCtrlWidth;
		wxStaticText* m_staticText17;
		wxStaticText* m_staticText19;
		wxSpinCtrl* m_spinCtrlHeight;
		wxStaticText* m_staticText182;
		wxStaticText* m_staticText18;
		wxChoice* m_choiceFormat;
		wxStaticText* m_staticText181;
		wxChoice* m_choiceQuality;
		wxStaticText* m_staticText1811;
		wxChoice* m_choiceBgStyle;
		wxStaticText* m_staticText18111;
		wxChoice* m_choiceSide;
		wxCheckBox* m_cbFloor;
		wxStaticText* m_staticText11;
		wxRadioBox* m_radioProjection;
		wxStaticText* m_staticText15;
		wxSpinCtrlDouble* m_spinCtrlZoom;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnFormatChoice( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_RENDER_JOB_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxEmptyString, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );

		~DIALOG_RENDER_JOB_BASE();

};


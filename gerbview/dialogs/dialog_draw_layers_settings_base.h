///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
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
#include <wx/sizer.h>
#include <wx/textctrl.h>
#include <wx/statline.h>
#include <wx/radiobox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_DRAW_LAYERS_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_DRAW_LAYERS_SETTINGS_BASE : public DIALOG_SHIM
{
	private:
		wxBoxSizer* m_namiSizer;

	protected:
		wxStaticText* m_stLayerNameTitle;
		wxStaticText* m_stLayerName;
		wxStaticText* m_stOffsetX;
		wxTextCtrl* m_tcOffsetX;
		wxStaticText* m_stUnitX;
		wxStaticText* m_stOffsetY;
		wxTextCtrl* m_tcOffsetY;
		wxStaticText* m_stUnitY;
		wxStaticText* m_stLayerRot;
		wxTextCtrl* m_tcRotation;
		wxStaticText* m_stUnitRot;
		wxStaticLine* m_staticline1;
		wxRadioBox* m_rbScope;
		wxStaticLine* m_staticline2;
		wxStdDialogButtonSizer* m_sdbSizerStdButtons;
		wxButton* m_sdbSizerStdButtonsOK;
		wxButton* m_sdbSizerStdButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnInitDlg( wxInitDialogEvent& event ) { event.Skip(); }
		virtual void OnUpdateUI( wxUpdateUIEvent& event ) { event.Skip(); }


	public:

		DIALOG_DRAW_LAYERS_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Layers Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_DRAW_LAYERS_SETTINGS_BASE();

};


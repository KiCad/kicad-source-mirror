///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.2.1-0-g80c4cb6a-dirty)
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
#include <wx/radiobut.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PNS_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PNS_SETTINGS_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxRadioButton* m_rbMarkObstacles;
		wxCheckBox* m_freeAngleMode;
		wxCheckBox* m_violateDrc;
		wxRadioButton* m_rbShove;
		wxCheckBox* m_shoveVias;
		wxCheckBox* m_backPressure;
		wxRadioButton* m_rbWalkaround;
		wxCheckBox* m_removeLoops;
		wxCheckBox* m_smartPads;
		wxCheckBox* m_smoothDragged;
		wxCheckBox* m_suggestEnding;
		wxCheckBox* m_optimizeEntireDraggedTrack;
		wxCheckBox* m_autoPosture;
		wxCheckBox* m_fixAllSegments;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onModeChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onFreeAngleModeChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_PNS_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Interactive Router Settings"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_PNS_SETTINGS_BASE();

};


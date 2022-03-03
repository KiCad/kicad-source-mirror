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
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_TRACK_VIA_SIZE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_TRACK_VIA_SIZE_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_trackWidthLabel;
		wxTextCtrl* m_trackWidthText;
		wxStaticText* m_trackWidthUnits;
		wxStaticText* m_viaDiameterLabel;
		wxTextCtrl* m_viaDiameterText;
		wxStaticText* m_viaDiameterUnits;
		wxStaticText* m_viaDrillLabel;
		wxTextCtrl* m_viaDrillText;
		wxStaticText* m_viaDrillUnits;
		wxStaticLine* m_staticline1;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;

	public:

		DIALOG_TRACK_VIA_SIZE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Track and Via Dimensions"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~DIALOG_TRACK_VIA_SIZE_BASE();

};


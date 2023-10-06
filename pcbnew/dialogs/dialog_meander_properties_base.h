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
class TEXT_CTRL_EVAL;

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
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_MEANDER_PROPERTIES_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_MEANDER_PROPERTIES_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticBitmap* m_legend;
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
		TEXT_CTRL_EVAL* m_rCtrl;
		wxStaticText* m_rUnits;
		wxCheckBox* m_singleSided;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;

	public:

		DIALOG_MEANDER_PROPERTIES_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Meander Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_MEANDER_PROPERTIES_BASE();

};


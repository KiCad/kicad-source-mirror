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
class PCB_LAYER_BOX_SELECTOR;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/bmpcbox.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_MICROVIA_STACK_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_MICROVIA_STACK_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_startLayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_startLayer;
		wxStaticText* m_endLayerLabel;
		PCB_LAYER_BOX_SELECTOR* m_endLayer;
		wxStaticText* m_typeLabel;
		wxChoice* m_typeChoice;
		wxStaticText* m_viaSizeLabel;
		wxTextCtrl* m_viaSizeCtrl;
		wxStaticText* m_viaSizeUnit;
		wxStaticText* m_viaDrillLabel;
		wxTextCtrl* m_viaDrillCtrl;
		wxStaticText* m_viaDrillUnit;
		wxCheckBox* m_useNetclass;
		wxCheckBox* m_filled;
		wxCheckBox* m_capped;
		wxStaticText* m_pitchLabel;
		wxTextCtrl* m_pitchCtrl;
		wxStaticText* m_pitchUnit;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;

	public:

		DIALOG_MICROVIA_STACK_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Microvia Stack"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxDEFAULT_DIALOG_STYLE );

		~DIALOG_MICROVIA_STACK_BASE();

};


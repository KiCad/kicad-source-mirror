///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jun  5 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE_H__
#define __DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
class DIALOG_SHIM;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/button.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE : public DIALOG_SHIM
{
	private:
	
	protected:
		wxStaticText* m_staticText4;
		wxChoice* m_choicePathFrom;
		wxStaticText* m_staticText15;
		wxChoice* m_choice4;
		wxStaticText* m_staticText3;
		wxChoice* m_constraintSource;
		wxStaticText* m_targetLengthLabel;
		wxTextCtrl* m_targetLengthText;
		wxStaticText* m_targetLengthUnit;
		wxStaticBitmap* m_legend;
		wxStaticText* m_staticText9;
		wxTextCtrl* m_minAmplText;
		wxStaticText* m_minAmplUnit;
		wxStaticText* m_staticText91;
		wxTextCtrl* m_maxAmplText;
		wxStaticText* m_maxAmplUnit;
		wxStaticText* m_staticText11;
		wxTextCtrl* m_spacingText;
		wxStaticText* m_spacingUnit;
		wxStaticText* m_staticText13;
		wxTextCtrl* m_radiusText;
		wxStaticText* m_radiusUnit;
		wxStaticText* m_staticText14;
		wxChoice* m_miterStyle;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnOkClick( wxCommandEvent& event ) { event.Skip(); }
		
	
	public:
		
		DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Trace length tuning"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 404,565 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER ); 
		~DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE();
	
};

#endif //__DIALOG_PNS_LENGTH_TUNING_SETTINGS_BASE_H__

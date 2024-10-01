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
#include "widgets/resettable_panel.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/textctrl.h>
#include <wx/gbsizer.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/checkbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SYM_EDITING_OPTIONS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SYM_EDITING_OPTIONS_BASE : public RESETTABLE_PANEL
{
	private:

	protected:
		wxStaticText* m_defaultsLabel;
		wxStaticLine* m_staticline1;
		wxStaticText* m_lineWidthLabel;
		wxTextCtrl* m_lineWidthCtrl;
		wxStaticText* m_lineWidthUnits;
		wxStaticText* m_widthHelpText;
		wxStaticText* m_textSizeLabel;
		wxTextCtrl* m_textSizeCtrl;
		wxStaticText* m_textSizeUnits;
		wxStaticText* m_pinLengthLabel;
		wxTextCtrl* m_pinLengthCtrl;
		wxStaticText* m_pinLengthUnits;
		wxStaticText* m_pinNumSizeLabel;
		wxTextCtrl* m_pinNumSizeCtrl;
		wxStaticText* m_pinNumSizeUnits;
		wxStaticText* m_pinNameSizeLabel;
		wxTextCtrl* m_pinNameSizeCtrl;
		wxStaticText* m_pinNameSizeUnits;
		wxStaticText* m_repeatLabel;
		wxStaticLine* m_staticline2;
		wxStaticText* m_pinPitchLabel;
		wxTextCtrl* m_pinPitchCtrl;
		wxStaticText* m_pinPitchUnits;
		wxStaticText* m_labelIncrementLabel1;
		wxSpinCtrl* m_spinRepeatLabel;
		wxStaticText* m_generalOption1;
		wxStaticLine* m_staticline4;
		wxCheckBox* m_dragPinsWithEdges;

		// Virtual event handlers, override them in your derived class
		virtual void onKillFocusPinPitch( wxFocusEvent& event ) { event.Skip(); }


	public:

		PANEL_SYM_EDITING_OPTIONS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SYM_EDITING_OPTIONS_BASE();

};


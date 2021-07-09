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
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/textctrl.h>
#include <wx/checkbox.h>
#include <wx/radiobut.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_FORMATTING_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_FORMATTING_BASE : public wxPanel
{
	DECLARE_EVENT_TABLE()
	private:

		// Private event handlers
		void _wxFB_onCheckBoxIref( wxCommandEvent& event ){ onCheckBoxIref( event ); }


	protected:
		wxStaticText* m_staticText26;
		wxChoice* m_choiceSeparatorRefId;
		wxStaticText* m_textSizeLabel;
		wxTextCtrl* m_textSizeCtrl;
		wxStaticText* m_textSizeUnits;
		wxStaticText* m_textOffsetRatioLabel;
		wxTextCtrl* m_textOffsetRatioCtrl;
		wxStaticText* m_offsetRatioUnits;
		wxStaticText* m_lineWidthLabel;
		wxTextCtrl* m_lineWidthCtrl;
		wxStaticText* m_lineWidthUnits;
		wxStaticText* m_pinSymbolSizeLabel;
		wxTextCtrl* m_pinSymbolSizeCtrl;
		wxStaticText* m_pinSymbolSizeUnits;
		wxStaticText* m_staticText261;
		wxChoice* m_choiceJunctionDotSize;
		wxCheckBox* m_showIntersheetsReferences;
		wxCheckBox* m_listOwnPage;
		wxRadioButton* m_radioFormatStandard;
		wxRadioButton* m_radioFormatAbbreviated;
		wxStaticText* m_prefixLabel;
		wxTextCtrl* m_prefixCtrl;
		wxStaticText* m_suffixLabel;
		wxTextCtrl* m_suffixCtrl;

		// Virtual event handlers, overide them in your derived class
		virtual void onCheckBoxIref( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_FORMATTING_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_SETUP_FORMATTING_BASE();

};


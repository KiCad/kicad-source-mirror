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
class VIA_CROSS_SECTION_PREVIEW;

#include <wx/string.h>
#include <wx/checkbox.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/stattext.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_BOARD_FINISH_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_BOARD_FINISH_BASE : public wxPanel
{
	private:

	protected:
		wxCheckBox* m_cbEgdesPlated;
		wxStaticText* m_staticTextFinish;
		wxChoice* m_choiceFinish;
		wxStaticText* m_staticTextEdgeConn;
		wxChoice* m_choiceEdgeConn;
		wxStaticText* m_viaProtectionHint;
		wxStaticText* m_fillViasLabel;
		wxCheckBox* m_fillVias;
		wxStaticText* m_capViasLabel;
		wxCheckBox* m_capVias;
		wxStaticText* m_plugViasLabel;
		wxCheckBox* m_plugViasFront;
		wxCheckBox* m_plugViasBack;
		wxStaticText* m_tentViasLabel;
		wxCheckBox* m_tentViasFront;
		wxCheckBox* m_tentViasBack;
		wxStaticText* m_coverViasLabel;
		wxCheckBox* m_coverViasFront;
		wxCheckBox* m_coverViasBack;
		VIA_CROSS_SECTION_PREVIEW* m_viaPreview;

	public:

		PANEL_SETUP_BOARD_FINISH_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SETUP_BOARD_FINISH_BASE();

};


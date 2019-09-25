///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Jul 10 2019)
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
#include <wx/textctrl.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/statbmp.h>
#include <wx/scrolwin.h>
#include <wx/radiobox.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/statbox.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SETUP_BOARD_STACKUP_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SETUP_BOARD_STACKUP_BASE : public wxPanel
{
	private:

	protected:
		wxStaticText* m_thicknessLabel;
		wxTextCtrl* m_thicknessCtrl;
		wxStaticText* m_staticTextCT;
		wxTextCtrl* m_tcCTValue;
		wxButton* m_buttonSetDielectricThickness;
		wxStaticLine* m_staticline;
		wxBoxSizer* m_sizerStackup;
		wxScrolledWindow* m_scGridWin;
		wxFlexGridSizer* m_fgGridSizer;
		wxStaticText* m_staticTextLayer;
		wxStaticText* m_staticTextLayerId;
		wxStaticText* m_staticTextType;
		wxStaticText* m_staticTextMaterial;
		wxStaticText* m_staticTextThickness;
		wxStaticBitmap* m_bitmapLockThickness;
		wxStaticText* m_staticTextColor;
		wxStaticText* m_staticTextEpsilonR;
		wxStaticText* m_staticTextLossTg;
		wxRadioBox* m_rbDielectricConstraint;
		wxCheckBox* m_cbCastellatedPads;
		wxCheckBox* m_cbEgdesPlated;
		wxStaticText* m_staticTextFinish;
		wxChoice* m_choiceFinish;
		wxStaticText* m_staticTextEdgeConn;
		wxChoice* m_choiceEdgeConn;
		wxButton* m_buttonExport;

		// Virtual event handlers, overide them in your derived class
		virtual void onUpdateThicknessValue( wxUpdateUIEvent& event ) { event.Skip(); }
		virtual void onCalculateDielectricThickness( wxCommandEvent& event ) { event.Skip(); }
		virtual void onExportToClipboard( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_SETUP_BOARD_STACKUP_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );
		~PANEL_SETUP_BOARD_STACKUP_BASE();

};


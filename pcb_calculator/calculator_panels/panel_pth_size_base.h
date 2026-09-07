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
#include "calculator_panels/calculator_panel.h"
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
#include <wx/checkbox.h>
#include <wx/bmpbuttn.h>
#include <wx/button.h>
#include <wx/grid.h>
#include <wx/panel.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_PTH_SIZE_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_PTH_SIZE_BASE : public CALCULATOR_PANEL
{
	private:

	protected:
		wxStaticText* m_staticTextLeadShape;
		wxChoice* m_shapeChoice;
		wxStaticText* m_staticTextMin;
		wxStaticText* m_staticTextMax;
		wxStaticText* m_staticTextSizeX;
		wxTextCtrl* m_sizeXMinCtrl;
		wxTextCtrl* m_sizeXMaxCtrl;
		wxStaticText* m_staticTextSizeXUnit;
		wxStaticText* m_staticTextSizeY;
		wxTextCtrl* m_sizeYMinCtrl;
		wxTextCtrl* m_sizeYMaxCtrl;
		wxStaticText* m_staticTextSizeYUnit;
		wxStaticText* m_staticTextStandard;
		wxChoice* m_standardChoice;
		wxStaticText* m_staticTextDensity;
		wxChoice* m_levelChoice;
		wxStaticText* m_staticTextRoundTo;
		wxTextCtrl* m_sizeRoundingCtrl;
		wxStaticText* m_staticTextRoundingUnit;
		wxStaticText* m_staticTextRoundType;
		wxChoice* m_choiceRounding;
		wxStaticText* m_staticTextAnnular;
		wxTextCtrl* m_annularCtrl;
		wxStaticText* m_staticTextAnnularUnit;
		wxStaticBitmap* m_bitmapPreview;
		wxCheckBox* m_cbShowMinLeadSize;
		wxCheckBox* m_cbShowMinHoleSize;
		wxCheckBox* m_cbShowMaxHoleSize;
		wxStaticText* m_staticTextHoleSizeResultTitle;
		wxTextCtrl* m_holeSizeCtrl;
		wxStaticText* m_staticTextHoleSizeResultUnit;
		wxStaticText* m_staticTextPadSizeResultTitle;
		wxTextCtrl* m_padSizeCtrl;
		wxStaticText* m_staticTextPadSizeResultUnit;
		wxTextCtrl* m_ResultReport;
		wxBitmapButton* m_bpCopyAll;
		wxStaticText* m_staticTextSummaryTitle;
		wxGrid* m_ipc2222Summary;

		// Virtual event handlers, override them in your derived class
		virtual void OnControlsChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnValueChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPreviewSettingCb( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCopyReportText( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_PTH_SIZE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_PTH_SIZE_BASE();

};


///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version 4.0.0-0-g0efcecf)
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
#include <wx/dataview.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/sizer.h>
#include <wx/panel.h>
#include <wx/bmpbuttn.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_SYNC_SHEET_PINS_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_SYNC_SHEET_PINS_BASE : public wxPanel
{
	private:

	protected:
		wxPanel* m_panel11;
		wxStaticText* m_labelSymName;
		wxDataViewCtrl* m_viewSheetPins;
		wxButton* m_btnAddLabels;
		wxButton* m_btnRmPins;
		wxPanel* m_panel1;
		wxStaticText* m_labelSheetName;
		wxDataViewCtrl* m_viewSheetLabels;
		wxButton* m_btnAddSheetPins;
		wxButton* m_btnRmLabels;
		wxPanel* m_panel3;
		wxPanel* m_panel8;
		wxBitmapButton* m_btnUseLabelAsTemplate;
		wxBitmapButton* m_btnUsePinAsTemplate;
		wxBitmapButton* m_btnUndo;
		wxPanel* m_panel4;
		wxDataViewCtrl* m_viewAssociated;

		// Virtual event handlers, override them in your derived class
		virtual void OnViewSheetPinCellClicked( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnBtnAddLabelsClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBtnRmPinsClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnViewSheetLabelCellClicked( wxDataViewEvent& event ) { event.Skip(); }
		virtual void OnBtnAddSheetPinsClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBtnRmLabelsClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBtnUseLabelAsTemplateClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBtnUsePinAsTemplateClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBtnUndoClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnViewMatchedCellClicked( wxDataViewEvent& event ) { event.Skip(); }


	public:

		PANEL_SYNC_SHEET_PINS_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 666,414 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_SYNC_SHEET_PINS_BASE();

};


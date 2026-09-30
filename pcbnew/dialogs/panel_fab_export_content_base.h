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
#include "widgets/wx_panel.h"
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/statline.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/radiobox.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/panel.h>
#include <wx/dataview.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class PANEL_FAB_EXPORT_CONTENT_BASE
///////////////////////////////////////////////////////////////////////////////
class PANEL_FAB_EXPORT_CONTENT_BASE : public WX_PANEL
{
	private:

	protected:
		wxStaticText* m_columnsLabel;
		wxStaticLine* m_staticline2;
		wxStaticText* m_lblDataSet;
		wxChoice* m_choiceDataSet;
		wxStaticText* m_lblNetNames;
		wxChoice* m_choiceNetNames;
		wxStaticText* m_lblVariant;
		wxChoice* m_choiceVariant;
		wxStaticText* m_lblRefDes;
		wxChoice* m_choiceRefDes;
		wxRadioBox* m_variantOutput;
		wxStaticText* m_variantHint;
		wxStaticText* m_lblIncludes;
		wxBoxSizer* bSizerContentButtons;
		wxButton* m_btnCustomize;
		wxButton* m_btnBomFields;

		// Virtual event handlers, override them in your derived class
		virtual void onDataSetChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onVariantChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onVariantOutputChange( wxCommandEvent& event ) { event.Skip(); }
		virtual void onCustomizeClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void onBomFieldsClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		PANEL_FAB_EXPORT_CONTENT_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~PANEL_FAB_EXPORT_CONTENT_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_FAB_CUSTOMIZE_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_FAB_CUSTOMIZE_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_intro;
		wxDataViewListCtrl* m_sections;
		wxStdDialogButtonSizer* m_stdButtons;
		wxButton* m_stdButtonsOK;
		wxButton* m_stdButtonsCancel;

	public:

		DIALOG_FAB_CUSTOMIZE_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Customize Content"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_FAB_CUSTOMIZE_BASE();

};


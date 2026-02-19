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
#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>
#include <wx/srchctrl.h>
#include <wx/choice.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dialog.h>
#include <wx/statbmp.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_TEMPLATE_SELECTOR_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_TEMPLATE_SELECTOR_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxPanel* m_panelMRU;
		wxStaticText* m_labelRecentTemplates;
		wxScrolledWindow* m_scrolledMRU;
		wxBoxSizer* m_sizerMRU;
		wxPanel* m_panelTemplates;
		wxSearchCtrl* m_searchCtrl;
		wxChoice* m_filterChoice;
		wxScrolledWindow* m_scrolledTemplates;
		wxBoxSizer* m_sizerTemplateList;
		wxPanel* m_panelPreview;
		wxPanel* m_webviewPlaceholder;
		wxBoxSizer* m_sizerButtons;
		wxButton* m_btnBack;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnSearchCtrlCancel( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnSearchCtrl( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnFilterChanged( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnBackClicked( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_TEMPLATE_SELECTOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Project Template Selector"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 900,600 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_TEMPLATE_SELECTOR_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class TEMPLATE_SELECTION_PANEL_BASE
///////////////////////////////////////////////////////////////////////////////
class TEMPLATE_SELECTION_PANEL_BASE : public wxPanel
{
	private:

	protected:

	public:
		wxBoxSizer* m_SizerBase;
		wxScrolledWindow* m_scrolledWindow;
		wxBoxSizer* m_SizerChoice;

		TEMPLATE_SELECTION_PANEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL|wxBORDER_NONE, const wxString& name = wxEmptyString );

		~TEMPLATE_SELECTION_PANEL_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class TEMPLATE_WIDGET_BASE
///////////////////////////////////////////////////////////////////////////////
class TEMPLATE_WIDGET_BASE : public wxPanel
{
	private:

	protected:
		wxStaticBitmap* m_bitmapIcon;
		wxStaticText* m_titleLabel;
		wxStaticText* m_descLabel;

	public:

		TEMPLATE_WIDGET_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxBORDER_THEME|wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~TEMPLATE_WIDGET_BASE();

};


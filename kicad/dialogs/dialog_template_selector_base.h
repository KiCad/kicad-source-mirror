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
class STD_BITMAP_BUTTON;
class WEBVIEW_PANEL;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/bmpbuttn.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/dialog.h>
#include <wx/scrolwin.h>
#include <wx/statbmp.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_TEMPLATE_SELECTOR_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_TEMPLATE_SELECTOR_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextTpath;
		wxTextCtrl* m_tcTemplatePath;
		STD_BITMAP_BUTTON* m_browseButton;
		STD_BITMAP_BUTTON* m_reloadButton;
		wxNotebook* m_notebook;
		WEBVIEW_PANEL* m_webviewPanel;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void onDirectoryBrowseClicked( wxCommandEvent& event ) { event.Skip(); }
		virtual void onReload( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnPageChange( wxNotebookEvent& event ) { event.Skip(); }


	public:

		DIALOG_TEMPLATE_SELECTOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Project Template Selector"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 513,523 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

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
		wxStaticText* m_staticTitle;

	public:

		TEMPLATE_WIDGET_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxTAB_TRAVERSAL, const wxString& name = wxEmptyString );

		~TEMPLATE_WIDGET_BASE();

};


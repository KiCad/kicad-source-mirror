///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Apr 10 2012)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __DIALOG_TEMPLATE_SELECTOR_BASE_H__
#define __DIALOG_TEMPLATE_SELECTOR_BASE_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include "dialog_shim.h"
#include <wx/gdicmn.h>
#include <wx/notebook.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/html/htmlwin.h>
#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/scrolwin.h>
#include <wx/panel.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_TEMPLATE_SELECTOR_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_TEMPLATE_SELECTOR_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxNotebook* m_notebook;
		wxHtmlWindow* m_htmlWin;
		wxStdDialogButtonSizer* m_sdbSizer1;
		wxButton* m_sdbSizer1OK;
		wxButton* m_sdbSizer1Cancel;

	public:

		DIALOG_TEMPLATE_SELECTOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("Project Template Selector"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 640,480 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
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
		wxScrolledWindow* m_scrolledWindow1;
		wxBoxSizer* m_SizerChoice;

		TEMPLATE_SELECTION_PANEL_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,140 ), long style = wxNO_BORDER|wxTAB_TRAVERSAL );
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

		TEMPLATE_WIDGET_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 74,-1 ), long style = wxTAB_TRAVERSAL );
		~TEMPLATE_WIDGET_BASE();

};

#endif //__DIALOG_TEMPLATE_SELECTOR_BASE_H__

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
class WX_HTML_REPORT_PANEL;

#include "dialog_shim.h"
#include <wx/string.h>
#include <wx/stattext.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/button.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/dialog.h>

///////////////////////////////////////////////////////////////////////////

#define ID_CHANGE_NOTEBOOK_PAGE 6000
#define ID_ADD_PLUGIN 6001
#define ID_DEL_PLUGIN 6002
#define wxID_BROWSE_PLUGINS 6003

///////////////////////////////////////////////////////////////////////////////
/// Class DIALOG_EXPORT_NETLIST_BASE
///////////////////////////////////////////////////////////////////////////////
class DIALOG_EXPORT_NETLIST_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextOutputPath;
		wxTextCtrl* m_outputPath;
		wxNotebook* m_NoteBook;
		WX_HTML_REPORT_PANEL* m_MessagesBox;
		wxBoxSizer* m_buttonSizer;
		wxButton* m_buttonAddGenerator;
		wxButton* m_buttonDelGenerator;
		wxStdDialogButtonSizer* m_sdbSizer2;
		wxButton* m_sdbSizer2OK;
		wxButton* m_sdbSizer2Cancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnNetlistTypeSelection( wxNotebookEvent& event ) { event.Skip(); }
		virtual void OnAddGenerator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDelGenerator( wxCommandEvent& event ) { event.Skip(); }


	public:

		DIALOG_EXPORT_NETLIST_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Export Netlist"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~DIALOG_EXPORT_NETLIST_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class NETLIST_DIALOG_ADD_GENERATOR_BASE
///////////////////////////////////////////////////////////////////////////////
class NETLIST_DIALOG_ADD_GENERATOR_BASE : public DIALOG_SHIM
{
	private:

	protected:
		wxStaticText* m_staticTextName;
		wxTextCtrl* m_textCtrlName;
		wxStaticText* m_staticTextCmd;
		wxTextCtrl* m_textCtrlCommand;
		wxButton* m_buttonGenerator;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, override them in your derived class
		virtual void OnBrowseGenerators( wxCommandEvent& event ) { event.Skip(); }


	public:

		NETLIST_DIALOG_ADD_GENERATOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Add Script-based Netlist Exporter"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );

		~NETLIST_DIALOG_ADD_GENERATOR_BASE();

};


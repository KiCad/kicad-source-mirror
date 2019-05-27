///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Dec  1 2018)
// http://www.wxformbuilder.org/
//
// PLEASE DO *NOT* EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#pragma once

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include "dialog_shim.h"
#include <wx/gdicmn.h>
#include <wx/notebook.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/sizer.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/button.h>
#include <wx/dialog.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/statline.h>

///////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
/// Class NETLIST_DIALOG_BASE
///////////////////////////////////////////////////////////////////////////////
class NETLIST_DIALOG_BASE : public DIALOG_SHIM
{
	DECLARE_EVENT_TABLE()
	private:

		// Private event handlers
		void _wxFB_OnNetlistTypeSelection( wxNotebookEvent& event ){ OnNetlistTypeSelection( event ); }
		void _wxFB_GenNetlist( wxCommandEvent& event ){ GenNetlist( event ); }
		void _wxFB_OnCancelClick( wxCommandEvent& event ){ OnCancelClick( event ); }
		void _wxFB_OnAddGenerator( wxCommandEvent& event ){ OnAddGenerator( event ); }
		void _wxFB_OnDelGenerator( wxCommandEvent& event ){ OnDelGenerator( event ); }


	protected:
		enum
		{
			ID_CHANGE_NOTEBOOK_PAGE = 1000,
			ID_CREATE_NETLIST,
			ID_ADD_PLUGIN,
			ID_DEL_PLUGIN
		};

		wxNotebook* m_NoteBook;
		wxButton* m_buttonNetlist;
		wxButton* m_buttonCancel;
		wxButton* m_buttonAddGenerator;
		wxButton* m_buttonDelGenerator;

		// Virtual event handlers, overide them in your derived class
		virtual void OnNetlistTypeSelection( wxNotebookEvent& event ) { event.Skip(); }
		virtual void GenNetlist( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnAddGenerator( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnDelGenerator( wxCommandEvent& event ) { event.Skip(); }


	public:

		NETLIST_DIALOG_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Netlist"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( -1,-1 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~NETLIST_DIALOG_BASE();

};

///////////////////////////////////////////////////////////////////////////////
/// Class NETLIST_DIALOG_ADD_GENERATOR_BASE
///////////////////////////////////////////////////////////////////////////////
class NETLIST_DIALOG_ADD_GENERATOR_BASE : public DIALOG_SHIM
{
	DECLARE_EVENT_TABLE()
	private:

		// Private event handlers
		void _wxFB_OnBrowseGenerators( wxCommandEvent& event ){ OnBrowseGenerators( event ); }
		void _wxFB_OnCancelClick( wxCommandEvent& event ){ OnCancelClick( event ); }
		void _wxFB_OnOKClick( wxCommandEvent& event ){ OnOKClick( event ); }


	protected:
		enum
		{
			wxID_BROWSE_PLUGINS = 1000
		};

		wxStaticText* m_staticTextCmd;
		wxTextCtrl* m_textCtrlCommand;
		wxStaticText* m_staticTextName;
		wxTextCtrl* m_textCtrlName;
		wxStaticLine* m_staticline1;
		wxButton* m_buttonGenerator;
		wxStdDialogButtonSizer* m_sdbSizer;
		wxButton* m_sdbSizerOK;
		wxButton* m_sdbSizerCancel;

		// Virtual event handlers, overide them in your derived class
		virtual void OnBrowseGenerators( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnCancelClick( wxCommandEvent& event ) { event.Skip(); }
		virtual void OnOKClick( wxCommandEvent& event ) { event.Skip(); }


	public:

		NETLIST_DIALOG_ADD_GENERATOR_BASE( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("Script Generator Properties"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 359,170 ), long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER );
		~NETLIST_DIALOG_ADD_GENERATOR_BASE();

};

